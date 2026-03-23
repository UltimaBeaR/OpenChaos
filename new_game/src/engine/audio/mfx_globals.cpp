#include "engine/audio/mfx_globals.h"

// uc_orig: alDevice (fallen/DDLibrary/Source/MFX.cpp)
ALCdevice* alDevice;
// uc_orig: alContext (fallen/DDLibrary/Source/MFX.cpp)
ALCcontext* alContext;

// uc_orig: Voices (fallen/DDLibrary/Source/MFX.cpp)
MFX_Voice Voices[MAX_VOICE];
// uc_orig: QWaves (fallen/DDLibrary/Source/MFX.cpp)
MFX_QWave QWaves[MAX_QWAVE];
// uc_orig: QFree (fallen/DDLibrary/Source/MFX.cpp)
MFX_QWave* QFree;
// uc_orig: QFreeLast (fallen/DDLibrary/Source/MFX.cpp)
MFX_QWave* QFreeLast;

// uc_orig: Samples (fallen/DDLibrary/Source/MFX.cpp)
MFX_Sample Samples[MAX_SAMPLE];
// uc_orig: TalkSample (fallen/DDLibrary/Source/MFX.cpp)
MFX_Sample TalkSample;
// uc_orig: NumSamples (fallen/DDLibrary/Source/MFX.cpp)
int NumSamples;
// uc_orig: LRU (fallen/DDLibrary/Source/MFX.cpp)
MFX_Sample LRU;

// uc_orig: MinDist (fallen/DDLibrary/Source/MFX.cpp)
const float MinDist = 512;
// uc_orig: MaxDist (fallen/DDLibrary/Source/MFX.cpp)
const float MaxDist = (64 << 8);
// uc_orig: Gain2D (fallen/DDLibrary/Source/MFX.cpp)
float Gain2D = 1.0;
// uc_orig: LX (fallen/DDLibrary/Source/MFX.cpp)
float LX;
// uc_orig: LY (fallen/DDLibrary/Source/MFX.cpp)
float LY;
// uc_orig: LZ (fallen/DDLibrary/Source/MFX.cpp)
float LZ;

// uc_orig: Volumes (fallen/DDLibrary/Source/MFX.cpp)
float Volumes[3];

// uc_orig: AllocatedRAM (fallen/DDLibrary/Source/MFX.cpp)
int AllocatedRAM = 0;
