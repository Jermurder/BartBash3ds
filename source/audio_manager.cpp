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

// Configuration
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define BUF_MS 120
#define BUF_SAMPLES (SAMPLE_RATE * BUF_MS / 1000) // frames per buffer (per channel)
#define BUF_COUNT 3
#define BUF_SIZE (BUF_SAMPLES * CHANNELS * sizeof(int16_t))
#define STACK_SIZE (32 * 1024)
#define MAX_CHANNELS 16 // NDSP channels we'll use; adjust to platform

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

// Helper: convert volume+pan to mix[2]
static inline void volumePanToMix(float vol, float pan, float outMix[2]) {
    // simple linear pan:
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

// Renamed callback to avoid symbol ambiguity.
static void audioNDSPCallback(void* /*data*/) {
    if (g_initialized.load()) {
        LightEvent_Signal(&g_audioEvent);
    }
}

// Channel allocation helpers
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

// Fill one ndspWaveBuf for the given instance. Returns false on EOF (and not looping) or error.
static bool fillInstanceBuffer(AudioInstance* inst, ndspWaveBuf* buf) {
    int totalFrames = 0;
    if (buf->data_pcm16 == nullptr) {
        buf->data_pcm16 = const_cast<int16_t*>(reinterpret_cast<const int16_t*>(buf->data_vaddr));
        printf("[AudioManager] Set data_pcm16 from data_vaddr\n");
    }

    printf("[AudioManager] Starting buffer fill: BUF_SAMPLES=%d\n", (int)BUF_SAMPLES);

    while (totalFrames < (int)BUF_SAMPLES) {
        int16_t* dest = reinterpret_cast<int16_t*>(buf->data_pcm16) + totalFrames * CHANNELS;
        int frames = op_read_stereo(inst->opusFile, dest, static_cast<int>(BUF_SAMPLES - totalFrames));
        printf("[AudioManager] op_read_stereo returned %d frames (requested %d)\n", frames, (int)(BUF_SAMPLES - totalFrames));
        if (frames < 0) {
            printf("[AudioManager] Opus decode error %d\n", frames);
            return false;
        }
        if (frames == 0) {
            printf("[AudioManager] Opus EOF reached (totalFrames=%d)\n", totalFrames);
            // Accept partial buffer if any frames were read
            break;
        }
        totalFrames += frames;
    }

    if (totalFrames == 0) {
        printf("[AudioManager] No audio frames decoded, aborting\n");
        return false;
    }
    buf->nsamples = totalFrames;
    DSP_FlushDataCache(buf->data_pcm16, totalFrames * CHANNELS * sizeof(int16_t));
    ndspChnWaveBufAdd(inst->channel, buf);
    printf("[AudioManager] Buffer submitted to NDSP, nsamples=%d\n", totalFrames);
    return true;
}

// Thread that manages a single instance's buffers
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
        // wait for NDSP callback signal (global)
        LightEvent_Wait(&g_audioEvent);
    }
    // signal any waiter
    LightEvent_Signal(&inst->event);
}

// Helpers to find and remove instances
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

// Public API
namespace AudioManager {

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
    // stop instances
    {
        std::lock_guard<std::mutex> lk(g_instancesMutex);
        for (auto& inst : g_instances) {
            if (inst) inst->quitting.store(true);
        }
    }
    // wake threads so they can exit
    LightEvent_Signal(&g_audioEvent);

    // join and cleanup
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

    // Clear the event (libctru provides LightEvent_Clear). No destructor needed.
    LightEvent_Clear(&g_audioEvent);

    ndspExit();
    g_initialized.store(false);
}

AudioHandle Play(const char* path, float pitch, bool loop, float volume, float pan) {
    if (!g_initialized.load()) {
        if (!Init()) return 0;
    }

    int err = 0;
    OggOpusFile* of = op_open_file(path, &err);
    if (!of) {
        printf("[AudioManager] Failed to open Opus file '%s' (%s, %d)\n", path, opusStrError(err), err);
        return 0;
    }
    printf("[AudioManager] Opus file opened successfully\n");

    // allocate instance
    auto inst = std::make_unique<AudioInstance>();
    int id = g_nextId.fetch_add(1);
    inst->id = id;
    inst->opusFile = of;
    inst->pitch = pitch;
    inst->loop = loop;
    inst->volume = volume;
    inst->pan = pan;
    LightEvent_Init(&inst->event, RESET_ONESHOT);

    // allocate channel
    int ch = allocateChannel();
    if (ch < 0) {
        printf("No free NDSP channels\n");
        op_free(of);
        return 0;
    }
    inst->channel = ch;

    // configure NDSP channel
    ndspChnReset(ch);
    ndspChnSetInterp(ch, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(ch, static_cast<unsigned int>(SAMPLE_RATE * inst->pitch));
    ndspChnSetFormat(ch, NDSP_FORMAT_STEREO_PCM16);
    float mix[2];
    volumePanToMix(inst->volume, inst->pan, mix);
    ndspChnSetMix(ch, mix);

    // allocate linear buffer (BUF_COUNT * BUF_SIZE)
    inst->audioBuf = static_cast<int16_t*>(linearAlloc(BUF_COUNT * BUF_SIZE));
    if (!inst->audioBuf) {
        printf("linearAlloc failed for audio buffer\n");
        freeChannel(ch);
        op_free(of);
        return 0;
    }

    // initialize wavebufs and pointers
    std::memset(inst->waveBufs, 0, sizeof(inst->waveBufs));
    for (int i = 0; i < BUF_COUNT; ++i) {
        void* v = reinterpret_cast<void*>(inst->audioBuf + (i * BUF_SIZE / sizeof(int16_t)));
        inst->waveBufs[i].data_vaddr = v; // non-const void*
        // set pcm pointer explicitly to avoid const issues
        inst->waveBufs[i].data_pcm16 = reinterpret_cast<int16_t*>(v);
        inst->waveBufs[i].nsamples = 0;
        inst->waveBufs[i].status = 0;
        inst->waveBufs[i].looping = false;
    }

    // pre-fill buffers
    bool ok = true;
    for (int i = 0; i < BUF_COUNT; ++i) {
        if (!fillInstanceBuffer(inst.get(), &inst->waveBufs[i])) {
            ok = false;
            break;
        }
    }
    if (!ok) {
        printf("Initial buffer fill failed for '%s'\n", path);
        if (inst->audioBuf) linearFree(inst->audioBuf);
        freeChannel(ch);
        op_free(of);
        return 0;
    }

    // create thread
    s32 prio;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    s32 tprio = prio > 0x18 ? prio - 1 : prio;
    inst->quitting.store(false);
    inst->threadId = threadCreate(instanceThreadFunc, inst.get(), STACK_SIZE, tprio, -1, false);
    if (inst->threadId == 0) {
        printf("threadCreate failed\n");
        if (inst->audioBuf) linearFree(inst->audioBuf);
        freeChannel(ch);
        op_free(of);
        return 0;
    }

    // add to global list
    {
        std::lock_guard<std::mutex> lk(g_instancesMutex);
        g_instances.push_back(std::move(inst));
    }

    printf("[AudioManager] Play: path='%s', pitch=%f, loop=%d, volume=%f, pan=%f\n", path, pitch, loop, volume, pan);

    return id;
}

void Stop(AudioHandle handle) {
    if (handle == 0) return;
    auto inst = takeInstanceById(handle);
    if (!inst) return;

    inst->quitting.store(true);
    // wake thread
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
}

bool SetPitch(AudioHandle handle, float pitch) {
    if (handle == 0) return false;
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(handle);
    if (!inst) return false;
    inst->pitch = pitch;
    if (inst->channel >= 0) {
        ndspChnSetRate(inst->channel, static_cast<unsigned int>(SAMPLE_RATE * inst->pitch));
    }
    return true;
}

bool SetVolume(AudioHandle handle, float volume) {
    if (handle == 0) return false;
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(handle);
    if (!inst) return false;
    inst->volume = volume;
    if (inst->channel >= 0) {
        float mix[2];
        volumePanToMix(inst->volume, inst->pan, mix);
        ndspChnSetMix(inst->channel, mix);
    }
    return true;
}

bool SetPan(AudioHandle handle, float pan) {
    if (handle == 0) return false;
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(handle);
    if (!inst) return false;
    inst->pan = pan;
    if (inst->channel >= 0) {
        float mix[2];
        volumePanToMix(inst->volume, inst->pan, mix);
        ndspChnSetMix(inst->channel, mix);
    }
    return true;
}

bool IsPlaying(AudioHandle handle) {
    if (handle == 0) return false;
    std::lock_guard<std::mutex> lk(g_instancesMutex);
    AudioInstance* inst = findInstanceByIdLocked(handle);
    if (!inst) return false;
    return !inst->quitting.load();
}

} // namespace AudioManager