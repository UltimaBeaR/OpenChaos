#ifndef ENGINE_AUDIO_MFX_GLOBALS_H
#define ENGINE_AUDIO_MFX_GLOBALS_H

#include "engine/audio/mfx.h"
#include <AL/alc.h>
#include "things/core/thing.h"
#include "things/core/thing_globals.h"

// Internal types used by the audio globals below.

// uc_orig: COORDINATE_UNITS (fallen/DDLibrary/Source/MFX.cpp)
#define COORDINATE_UNITS float(1.0 / 256.0)

// uc_orig: SampleType (fallen/DDLibrary/Source/MFX.cpp)
enum SampleType {
    // uc_orig: SMP_Ambient (fallen/DDLibrary/Source/MFX.cpp)
    SMP_Ambient = 0,
    // uc_orig: SMP_Music (fallen/DDLibrary/Source/MFX.cpp)
    SMP_Music,
    // uc_orig: SMP_Effect (fallen/DDLibrary/Source/MFX.cpp)
    SMP_Effect
};

// uc_orig: MFX_Sample (fallen/DDLibrary/Source/MFX.cpp)
struct MFX_Sample {
    MFX_Sample* prev_lru;
    MFX_Sample* next_lru;
    char* fname;
    unsigned int handle;
    bool is3D;
    int size;
    int usecount;
    int type;
    float linscale;
    bool loading;
};

// uc_orig: MAX_SAMPLE (fallen/DDLibrary/Source/MFX.cpp)
#define MAX_SAMPLE 552
// uc_orig: MAX_SAMPLE_MEM (fallen/DDLibrary/Source/MFX.cpp)
#define MAX_SAMPLE_MEM 64 * 1024 * 1024

// uc_orig: MFX_QWave (fallen/DDLibrary/Source/MFX.cpp)
struct MFX_QWave {
    MFX_QWave* next;
    ULONG wave;
    ULONG flags;
    bool is3D;
    SLONG x, y, z;
    float gain;
};

// uc_orig: MAX_QWAVE (fallen/DDLibrary/Source/MFX.cpp)
#define MAX_QWAVE 32
// uc_orig: MAX_QVOICE (fallen/DDLibrary/Source/MFX.cpp)
#define MAX_QVOICE 5

// uc_orig: MFX_Voice (fallen/DDLibrary/Source/MFX.cpp)
struct MFX_Voice {
    UWORD id;
    unsigned int handle;
    ULONG wave;
    ULONG flags;
    bool is3D;
    SLONG x, y, z;
    Thing* thing;
    MFX_QWave* queue;
    SLONG queuesz;
    MFX_Sample* smp;
    bool playing;
    float ratemult;
    float gain;
};

// uc_orig: MAX_VOICE (fallen/DDLibrary/Source/MFX.cpp)
#define MAX_VOICE 64
// uc_orig: VOICE_MSK (fallen/DDLibrary/Source/MFX.cpp)
#define VOICE_MSK 63

// uc_orig: alDevice (fallen/DDLibrary/Source/MFX.cpp)
extern ALCdevice* alDevice;
// uc_orig: alContext (fallen/DDLibrary/Source/MFX.cpp)
extern ALCcontext* alContext;

// uc_orig: Voices (fallen/DDLibrary/Source/MFX.cpp)
extern MFX_Voice Voices[MAX_VOICE];
// uc_orig: QWaves (fallen/DDLibrary/Source/MFX.cpp)
extern MFX_QWave QWaves[MAX_QWAVE];
// uc_orig: QFree (fallen/DDLibrary/Source/MFX.cpp)
extern MFX_QWave* QFree;
// uc_orig: QFreeLast (fallen/DDLibrary/Source/MFX.cpp)
extern MFX_QWave* QFreeLast;

// uc_orig: Samples (fallen/DDLibrary/Source/MFX.cpp)
extern MFX_Sample Samples[MAX_SAMPLE];
// uc_orig: TalkSample (fallen/DDLibrary/Source/MFX.cpp)
extern MFX_Sample TalkSample;
// uc_orig: NumSamples (fallen/DDLibrary/Source/MFX.cpp)
extern int NumSamples;
// uc_orig: LRU (fallen/DDLibrary/Source/MFX.cpp)
extern MFX_Sample LRU;

// uc_orig: MinDist (fallen/DDLibrary/Source/MFX.cpp)
extern const float MinDist;
// uc_orig: MaxDist (fallen/DDLibrary/Source/MFX.cpp)
extern const float MaxDist;
// uc_orig: Gain2D (fallen/DDLibrary/Source/MFX.cpp)
extern float Gain2D;
// uc_orig: LX (fallen/DDLibrary/Source/MFX.cpp)
extern float LX;
// uc_orig: LY (fallen/DDLibrary/Source/MFX.cpp)
extern float LY;
// uc_orig: LZ (fallen/DDLibrary/Source/MFX.cpp)
extern float LZ;

// uc_orig: Volumes (fallen/DDLibrary/Source/MFX.cpp)
extern float Volumes[3];

// uc_orig: AllocatedRAM (fallen/DDLibrary/Source/MFX.cpp)
extern int AllocatedRAM;

#endif // ENGINE_AUDIO_MFX_GLOBALS_H
