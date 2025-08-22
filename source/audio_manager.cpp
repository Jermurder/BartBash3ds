#include "audio_manager.h"

#include <3ds.h>
#include <opusfile.h>

#include <vector>
#include <memory>
#include <mutex>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <algorithm>
#include <unordered_map>

// Configuration
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define BUF_MS 30
#define BUF_SAMPLES (SAMPLE_RATE * BUF_MS / 1000) // frames per buffer (per channel)
#define BUF_COUNT 3
#define BUF_SIZE (BUF_SAMPLES * CHANNELS * sizeof(int16_t))
#define STACK_SIZE (32 * 1024)
#define MAX_CHANNELS 4 // NDSP channels we'll use; adjust to platform

using AudioHandle = int;

struct PreloadedAudio {
    int16_t* audioBuf = nullptr;
    int numSamples = 0;
    // You can extend with ndspWaveBuf waveBufs[BUF_COUNT] if needed for playback
};

namespace {

// Per-instance structure
struct AudioInstance {
    int id = 0;
    int channel = -1;
    OggOpusFile* opusFile = nullptr;
    ndspWaveBuf waveBufs[BUF_COUNT];
    int16_t* audioBuf = nullptr; // linearAlloc memory pointer
    float pitch = 1.0f;
    float volume = 1.0f;
    float pan = 0.0f;
    bool loop = false;
    std::atomic<bool> quitting{false};
    LightEvent event{};
    Thread threadId = 0;
};

// Globals
std::vector<std::unique_ptr<AudioInstance>> g_instances;
std::mutex g_instancesMutex;

LightEvent g_audioEvent;
std::atomic<bool> g_initialized(false);
std::atomic<int> g_nextId(1);
bool g_callbackRegistered = false;
bool g_channelUsed[MAX_CHANNELS] = {false};

// Convert volume+pan to stereo mix
static inline void volumePanToMix(float vol, float pan, float outMix[2]) {
    float left = vol * (pan <= 0.0f ? 1.0f : 1.0f - pan);
    float right = vol * (pan >= 0.0f ? 1.0f : 1.0f + pan);
    outMix[0] = left;
    outMix[1] = right;
}

static const char* opusStrError(int error) {
    switch (error) {
        case OP_EBADPACKET: return "Bad packet";
        case OP_EINVAL:     return "Invalid argument";
        case OP_ENOTFORMAT: return "Not Opus format";
        default:            return "Unknown error";
    }
}

static void audioNDSPCallback(void* /*data*/) {
    if (g_initialized.load()) {
        LightEvent_Signal(&g_audioEvent);
    }
}

static int allocateChannel() {
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (!g_channelUsed[i]) {
            g_channelUsed[i] = true;
            return i;
        }
    }
    return -1;
}

static void freeChannel(int ch) {
    if (ch >= 0 && ch < MAX_CHANNELS) g_channelUsed[ch] = false;
}

static bool fillInstanceBuffer(AudioInstance* inst, ndspWaveBuf* buf) {
    int totalFrames = 0;
    while (totalFrames < (int)BUF_SAMPLES) {
        int16_t* dest = reinterpret_cast<int16_t*>(buf->data_pcm16) + totalFrames * CHANNELS;
        int frames = op_read_stereo(inst->opusFile, dest, static_cast<int>(BUF_SAMPLES - totalFrames));
        if (frames < 0) {
            printf("[AudioManager] Opus decode error %d\n", frames);
            return false;
        }
        if (frames == 0) {
            if (inst->loop) {
                op_pcm_seek(inst->opusFile, 0);
                continue; // restart
            }
            break; // EOF
        }
        totalFrames += frames;
    }

    if (totalFrames == 0) return false;

    buf->nsamples = totalFrames;
    DSP_FlushDataCache(buf->data_pcm16, totalFrames * CHANNELS * sizeof(int16_t));
    ndspChnWaveBufAdd(inst->channel, buf);
    return true;
}

static void instanceThreadFunc(void* arg) {
    AudioInstance* inst = reinterpret_cast<AudioInstance*>(arg);
    while (!inst->quitting.load()) {
        for (int i = 0; i < BUF_COUNT; ++i) {
            if (inst->waveBufs[i].status == NDSP_WBUF_DONE) {
                if (!fillInstanceBuffer(inst, &inst->waveBufs[i])) {
                    inst->quitting.store(true);
                    break;
                }
            }
        }
        LightEvent_Wait(&g_audioEvent);
    }
    LightEvent_Signal(&inst->event);
}

static std::unique_ptr<AudioInstance> takeInstanceById(int id) {
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    for (auto it = g_instances.begin(); it != g_instances.end(); ++it) {
        if ((*it)->id == id) {
            std::unique_ptr<AudioInstance> inst = std::move(*it);
            g_instances.erase(it);
            return inst;
        }
    }
    return nullptr;
}

static AudioInstance* findInstanceByIdLocked(int id) {
    for (auto& up : g_instances) {
        if (up->id == id) return up.get();
    }
    return nullptr;
}

} // anonymous namespace

namespace AudioManager {

std::unordered_map<std::string, PreloadedAudio> g_preloadedAudio;

bool Init() {
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    if (g_initialized.load()) return true;

    if (ndspInit() != 0) {
        printf("ndspInit failed\n");
        return false;
    }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    LightEvent_Init(&g_audioEvent, RESET_ONESHOT);
    ndspSetCallback(audioNDSPCallback, nullptr);
    g_callbackRegistered = true;

    g_initialized.store(true);
    return true;
}

void Exit() {
    {
        std::lock_guard<std::mutex> lk(g_instancesMutex);
        for (auto& inst : g_instances) {
            inst->quitting.store(true);
        }
    }
    LightEvent_Signal(&g_audioEvent);

    std::vector<std::unique_ptr<AudioInstance>> cleanup;
    {
        std::lock_guard<std::mutex> lk(g_instancesMutex);
        cleanup.swap(g_instances);
    }

    for (auto& inst : cleanup) {
        if (!inst) continue;
        if (inst->threadId) {
            threadJoin(inst->threadId, UINT64_MAX);
            threadFree(inst->threadId);
            inst->threadId = 0;
        }
        if (inst->audioBuf) {
            linearFree(inst->audioBuf);
            inst->audioBuf = nullptr;
        }
        if (inst->opusFile) {
            op_free(inst->opusFile);
            inst->opusFile = nullptr;
        }
        if (inst->channel >= 0) {
            ndspChnReset(inst->channel);
            freeChannel(inst->channel);
            inst->channel = -1;
        }
    }

    if (g_callbackRegistered) {
        ndspSetCallback(nullptr, nullptr);
        g_callbackRegistered = false;
    }
    LightEvent_Clear(&g_audioEvent);
    ndspExit();
    g_initialized.store(false);
}

AudioHandle Play(const char* path, float pitch, bool loop, float volume, float pan) {
    auto it = g_preloadedAudio.find(path);
    if (it != g_preloadedAudio.end()) {
        PreloadedAudio& pl = it->second;

        int ch = allocateChannel();
        if (ch < 0) {
            printf("No free NDSP channels for preloaded\n");
            return 0;
        }

        ndspChnReset(ch);
        ndspChnSetInterp(ch, NDSP_INTERP_POLYPHASE);
        ndspChnSetRate(ch, (u32)(SAMPLE_RATE * pitch));
        ndspChnSetFormat(ch, NDSP_FORMAT_STEREO_PCM16);

        float mix[2];
        volumePanToMix(volume, pan, mix);
        ndspChnSetMix(ch, mix);

        // Make AudioInstance to track this
        auto inst = std::make_unique<AudioInstance>();
        int id = g_nextId.fetch_add(1);
        inst->id = id;
        inst->channel = ch;
        inst->pitch = pitch;
        inst->volume = volume;
        inst->pan = pan;
        inst->loop = loop;
        inst->opusFile = nullptr;
        inst->audioBuf = nullptr;
        inst->threadId = 0;
        inst->quitting.store(false);

        // Configure one wave buffer
        ndspWaveBuf* wb = &inst->waveBufs[0];
        memset(wb, 0, sizeof(ndspWaveBuf));
        wb->data_pcm16 = pl.audioBuf;
        wb->nsamples   = pl.numSamples;
        wb->looping    = loop ? 1 : 0;
        wb->status     = NDSP_WBUF_DONE;

        // Flush cache BEFORE adding
        DSP_FlushDataCache(wb->data_pcm16, pl.numSamples * CHANNELS * sizeof(int16_t));
        ndspChnWaveBufAdd(ch, wb);

        printf("[AudioManager] Playing preloaded '%s' (%d samples)\n", path, pl.numSamples);

        {
            std::lock_guard<std::mutex> lk(g_instancesMutex);
            g_instances.push_back(std::move(inst));
        }

        return id;
    } 

    // ---- Streaming fallback path (unchanged) ----
    int err = 0;
    OggOpusFile* of = op_open_file(path, &err);
    if (!of) {
        printf("[AudioManager] Failed to open Opus file '%s' (%s, %d)\n", path, opusStrError(err), err);
        return 0;
    }

    auto inst = std::make_unique<AudioInstance>();
    int id = g_nextId.fetch_add(1);
    inst->id = id;
    inst->opusFile = of;
    inst->pitch = pitch;
    inst->loop = loop;
    inst->volume = volume;
    inst->pan = pan;
    LightEvent_Init(&inst->event, RESET_ONESHOT);

    int ch = allocateChannel();
    if (ch < 0) {
        printf("No free NDSP channels\n");
        op_free(of);
        return 0;
    }
    inst->channel = ch;

    ndspChnReset(ch);
    ndspChnSetInterp(ch, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(ch, static_cast<unsigned int>(SAMPLE_RATE * inst->pitch));
    ndspChnSetFormat(ch, NDSP_FORMAT_STEREO_PCM16);
    float mix[2];
    volumePanToMix(inst->volume, inst->pan, mix);
    ndspChnSetMix(ch, mix);

    inst->audioBuf = static_cast<int16_t*>(linearAlloc(BUF_COUNT * BUF_SIZE));
    if (!inst->audioBuf) {
        printf("linearAlloc failed\n");
        op_free(of);
        freeChannel(ch);
        return 0;
    }

    for (int i = 0; i < BUF_COUNT; ++i) {
        inst->waveBufs[i].data_pcm16 = inst->audioBuf + i * BUF_SAMPLES * CHANNELS;
        inst->waveBufs[i].nsamples = 0;
        inst->waveBufs[i].status = NDSP_WBUF_DONE;
        inst->waveBufs[i].next = nullptr;
    }

    // Fill initial buffers
    for (int i = 0; i < BUF_COUNT; ++i) {
        if (!fillInstanceBuffer(inst.get(), &inst->waveBufs[i])) {
            printf("[AudioManager] Failed to fill initial buffer\n");
            linearFree(inst->audioBuf);
            op_free(of);
            freeChannel(ch);
            return 0;
        }
    }

    ndspChnWaveBufAdd(ch, &inst->waveBufs[0]);

    inst->threadId = threadCreate(instanceThreadFunc, inst.get(), STACK_SIZE, 0x30, -2, false);
    if (!inst->threadId) {
        printf("Failed to create audio thread\n");
        linearFree(inst->audioBuf);
        op_free(of);
        freeChannel(ch);
        return 0;
    }

    {
        std::lock_guard<std::mutex> lk(g_instancesMutex);
        g_instances.push_back(std::move(inst));
    }
    return id;
}


bool StopAudio(AudioHandle handle) {
    auto inst = takeInstanceById(handle);
    if (!inst) return false;

    inst->quitting.store(true);
    LightEvent_Signal(&g_audioEvent);
    if (inst->threadId) {
        threadJoin(inst->threadId, UINT64_MAX);
        threadFree(inst->threadId);
        inst->threadId = 0;
    }

    if (inst->audioBuf) {
        linearFree(inst->audioBuf);
        inst->audioBuf = nullptr;
    }
    if (inst->opusFile) {
        op_free(inst->opusFile);
        inst->opusFile = nullptr;
    }
    if (inst->channel >= 0) {
        ndspChnReset(inst->channel);
        freeChannel(inst->channel);
        inst->channel = -1;
    }
    return true;
}

bool IsPlaying(AudioHandle id) {
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(id);
    if (!inst) return false;
    return !inst->quitting.load();
}

bool SetVolume(AudioHandle id, float volume) {
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(id);
    if (!inst) return false;
    inst->volume = volume;
    float mix[2];
    volumePanToMix(inst->volume, inst->pan, mix);
    ndspChnSetMix(inst->channel, mix);
    return true;
}

bool SetPan(AudioHandle id, float pan) {
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(id);
    if (!inst) return false;
    inst->pan = pan;
    float mix[2];
    volumePanToMix(inst->volume, inst->pan, mix);
    ndspChnSetMix(inst->channel, mix);
    return true;
}

bool SetPitch(AudioHandle id, float pitch) {
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(id);
    if (!inst) return false;
    inst->pitch = pitch;
    ndspChnSetRate(inst->channel, static_cast<unsigned int>(SAMPLE_RATE * pitch));
    return true;
}



void UnloadPreloadAudio(const char* path) {
    auto it = g_preloadedAudio.find(path);
    if (it != g_preloadedAudio.end()) {
        if (it->second.audioBuf) free(it->second.audioBuf);
        g_preloadedAudio.erase(it);
        printf("[AudioManager] Unloaded preloaded '%s'\n", path);
    }
}


void CleanupFinishedInstances() {
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    auto it = g_instances.begin();
    while (it != g_instances.end()) {
        AudioInstance* inst = it->get();

        // --- For preloaded sounds ---
        if (inst->opusFile == nullptr && inst->threadId == 0) {
            // Check waveBuf[0] status
            if (inst->waveBufs[0].status == NDSP_WBUF_DONE) {
                printf("[AudioManager] Preloaded sound %d finished (ch=%d)\n", inst->id, inst->channel);
                inst->quitting.store(true);
            }
        }

        if (inst->quitting.load()) {
            if (inst->threadId) {
                threadJoin(inst->threadId, UINT64_MAX);
                threadFree(inst->threadId);
                inst->threadId = 0;
            }
            if (inst->audioBuf) {
                linearFree(inst->audioBuf);
                inst->audioBuf = nullptr;
            }
            if (inst->opusFile) {
                op_free(inst->opusFile);
                inst->opusFile = nullptr;
            }
            if (inst->channel >= 0) {
                ndspChnReset(inst->channel);
                freeChannel(inst->channel);
                inst->channel = -1;
            }
            it = g_instances.erase(it);
        } else {
            ++it;
        }
    }
}

bool PreloadAudio(const char* path) {
    if (g_preloadedAudio.find(path) != g_preloadedAudio.end())
        return true; // already loaded

    int err = 0;
    OggOpusFile* of = op_open_file(path, &err);
    if (!of) {
        printf("[AudioManager] Failed to open Opus file '%s' for preload (%s)\n", path, opusStrError(err));
        return false;
    }

    // Find length in samples
    ogg_int64_t pcmTotal = op_pcm_total(of, -1);
    if (pcmTotal <= 0) {
        op_free(of);
        printf("[AudioManager] Invalid Opus length in '%s'\n", path);
        return false;
    }

    int numSamples = static_cast<int>(pcmTotal);

    // Allocate in linear memory (DSP safe, 32-byte aligned)
    int16_t* buf = (int16_t*)linearAlloc(numSamples * CHANNELS * sizeof(int16_t));
    if (!buf) {
        op_free(of);
        printf("[AudioManager] linearAlloc failed for preload\n");
        return false;
    }

    // Decode into the buffer
    int16_t* writePtr = buf;
    int totalRead = 0;
    while (totalRead < numSamples) {
        int frames = op_read_stereo(of, writePtr, numSamples - totalRead);
        if (frames <= 0) break;
        totalRead += frames;
        writePtr += frames * CHANNELS;
    }

    op_free(of);

    // Flush cache so DSP sees it
    DSP_FlushDataCache(buf, totalRead * CHANNELS * sizeof(int16_t));

    PreloadedAudio pl;
    pl.audioBuf = buf;
    pl.numSamples = totalRead;

    g_preloadedAudio[path] = pl;
    printf("[AudioManager] Preloaded '%s' (%d samples)\n", path, totalRead);
    return true;
}



} // namespace AudioManager
