#include "audio_manager.h"
#include <3ds.h>
#include <opusfile.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define BUF_COUNT 3
#define BUF_SAMPLES (SAMPLE_RATE * 120 / 1000)
#define BUF_SIZE (BUF_SAMPLES * CHANNELS * sizeof(int16_t))
#define STACK_SIZE (32 * 1024)

static ndspWaveBuf waveBufs[BUF_COUNT];
static int16_t* audioBuf = nullptr;
static LightEvent audioEvent;
static Thread audioThreadId = 0;
static OggOpusFile* opusFile = nullptr;
static volatile bool quit = false;
 
static const char* opusStrError(int error) {
    switch(error) {
        case OP_EBADPACKET: return "Bad packet";
        case OP_EINVAL: return "Invalid argument";
        case OP_ENOTFORMAT: return "Not Opus format";
        default: return "Unknown error";
    }
}

static bool fillBuffer(ndspWaveBuf* buf) {
    int total = 0;
    while (total < BUF_SAMPLES) {
        int16_t* dest = reinterpret_cast<int16_t*>(buf->data_pcm16) + total * CHANNELS;
        int samples = op_read_stereo(opusFile, dest, (BUF_SAMPLES - total) * CHANNELS);
        if (samples <= 0) {
            printf("Opus decode returned %d (EOF or error)\n", samples);
            return false;
        }
        total += samples;
    }

    buf->nsamples = total;
    DSP_FlushDataCache(buf->data_pcm16, total * CHANNELS * sizeof(int16_t));
    ndspChnWaveBufAdd(0, buf);
    return true;
}

static void audioNDSPCallback(void*) {
    if (!quit) LightEvent_Signal(&audioEvent);
}

static void audioThreadFunc(void*) {
    while (!quit) {
        for (int i = 0; i < BUF_COUNT; ++i) {
            if (waveBufs[i].status == NDSP_WBUF_DONE) {
                if (!fillBuffer(&waveBufs[i])) {
                    printf("Failed to refill buffer %d\n", i);
                    return;
                }
            }
        }
        LightEvent_Wait(&audioEvent);
    }
}

bool audioManagerInit() {
    if (ndspInit() != 0) {
        printf("ndspInit failed\n");
        return false;
    }

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(0, SAMPLE_RATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    float mix[2] = { 1.0f, 1.0f }; // Left and right volume
    ndspChnSetMix(0, mix);
    ndspSetMasterVol(1.0f);

    ndspSetCallback(audioNDSPCallback, nullptr);
    LightEvent_Init(&audioEvent, RESET_ONESHOT);

    audioBuf = static_cast<int16_t*>(linearAlloc(BUF_COUNT * BUF_SIZE));
    if (!audioBuf) {
        printf("Failed to allocate audio buffer\n");
        return false;
    }

    std::memset(waveBufs, 0, sizeof(waveBufs));
    for (int i = 0; i < BUF_COUNT; ++i) {
        waveBufs[i].data_vaddr = audioBuf + (i * BUF_SIZE / sizeof(int16_t));
    }

    return true;
}

void audioManagerExit() {
    audioManagerStop();
    if (audioBuf) {
        linearFree(audioBuf);
        audioBuf = nullptr;
    }
    ndspExit();
}

bool audioManagerPlay(const char* path) {
    int err;
    opusFile = op_open_file(path, &err);
    if (!opusFile) {
        printf("Failed to open Opus file: %s (%d)\n", opusStrError(err), err);
        return false;
    }

    printf("Playing Opus file: %s\n", path);

    quit = false;

    // Pre-fill all buffers
    for (int i = 0; i < BUF_COUNT; ++i) {
        if (!fillBuffer(&waveBufs[i])) {
            printf("Initial buffer fill failed\n");
            op_free(opusFile);
            opusFile = nullptr;
            return false;
        }
    }

    // Start audio thread
    s32 prio;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    prio = prio > 0x18 ? prio - 1 : prio;

    audioThreadId = threadCreate(audioThreadFunc, nullptr, STACK_SIZE, prio, -1, false);
    if (audioThreadId == 0) {
        op_free(opusFile);
        opusFile = nullptr;
        return false;
    }

    return true;
}

void audioManagerStop() {
    if (!opusFile) return;

    quit = true;
    LightEvent_Signal(&audioEvent);
    threadJoin(audioThreadId, UINT64_MAX);
    threadFree(audioThreadId);
    audioThreadId = 0;

    op_free(opusFile);
    opusFile = nullptr;
}
