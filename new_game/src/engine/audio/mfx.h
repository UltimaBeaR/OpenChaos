#ifndef ENGINE_AUDIO_MFX_H
#define ENGINE_AUDIO_MFX_H

#include "engine/core/types.h"

struct Thing;

// uc_orig: MFX_LOOPED (fallen/DDLibrary/Headers/MFX.h)
#define MFX_LOOPED (1)
// uc_orig: MFX_MOVING (fallen/DDLibrary/Headers/MFX.h)
#define MFX_MOVING (2)
// uc_orig: MFX_REPLACE (fallen/DDLibrary/Headers/MFX.h)
#define MFX_REPLACE (4)
// uc_orig: MFX_OVERLAP (fallen/DDLibrary/Headers/MFX.h)
#define MFX_OVERLAP (8)
// uc_orig: MFX_QUEUED (fallen/DDLibrary/Headers/MFX.h)
#define MFX_QUEUED (16)
// uc_orig: MFX_CAMERA (fallen/DDLibrary/Headers/MFX.h)
#define MFX_CAMERA (32)
// uc_orig: MFX_HI_PRIORITY (fallen/DDLibrary/Headers/MFX.h)
#define MFX_HI_PRIORITY (64)
// uc_orig: MFX_LO_PRIORITY (fallen/DDLibrary/Headers/MFX.h)
#define MFX_LO_PRIORITY (128)
// uc_orig: MFX_LOCKY (fallen/DDLibrary/Headers/MFX.h)
#define MFX_LOCKY (256)
// uc_orig: MFX_SHORT_QUEUE (fallen/DDLibrary/Headers/MFX.h)
#define MFX_SHORT_QUEUE (512)
// uc_orig: MFX_PAIRED_TRK1 (fallen/DDLibrary/Headers/MFX.h)
#define MFX_PAIRED_TRK1 (1024)
// uc_orig: MFX_PAIRED_TRK2 (fallen/DDLibrary/Headers/MFX.h)
#define MFX_PAIRED_TRK2 (2048)
// uc_orig: MFX_EARLY_OUT (fallen/DDLibrary/Headers/MFX.h)
#define MFX_EARLY_OUT (4096)
// uc_orig: MFX_NEVER_OVERLAP (fallen/DDLibrary/Headers/MFX.h)
#define MFX_NEVER_OVERLAP (8192)

// uc_orig: MFX_ENV_NONE (fallen/DDLibrary/Headers/MFX.h)
#define MFX_ENV_NONE (0)
// uc_orig: MFX_ENV_ALLEY (fallen/DDLibrary/Headers/MFX.h)
#define MFX_ENV_ALLEY (1)
// uc_orig: MFX_ENV_SEWER (fallen/DDLibrary/Headers/MFX.h)
#define MFX_ENV_SEWER (2)
// uc_orig: MFX_ENV_STADIUM (fallen/DDLibrary/Headers/MFX.h)
#define MFX_ENV_STADIUM (3)

// uc_orig: MFX_CHANNEL_ALL (fallen/DDLibrary/Headers/MFX.h)
#define MFX_CHANNEL_ALL (0x010000)
// uc_orig: MFX_WAVE_ALL (fallen/DDLibrary/Headers/MFX.h)
#define MFX_WAVE_ALL (0x010000)

// uc_orig: MFX_get_volumes (fallen/DDLibrary/Headers/MFX.h)
void MFX_get_volumes(SLONG* fx, SLONG* amb, SLONG* mus);
// uc_orig: MFX_set_volumes (fallen/DDLibrary/Headers/MFX.h)
void MFX_set_volumes(SLONG fx, SLONG amb, SLONG mus);

// uc_orig: MFX_play_xyz (fallen/DDLibrary/Headers/MFX.h)
void MFX_play_xyz(UWORD channel_id, ULONG wave, ULONG flags, SLONG x, SLONG y, SLONG z);
// uc_orig: MFX_play_thing (fallen/DDLibrary/Headers/MFX.h)
void MFX_play_thing(UWORD channel_id, ULONG wave, ULONG flags, Thing* p);
// uc_orig: MFX_play_ambient (fallen/DDLibrary/Headers/MFX.h)
void MFX_play_ambient(UWORD channel_id, ULONG wave, ULONG flags);
// uc_orig: MFX_play_stereo (fallen/DDLibrary/Headers/MFX.h)
UBYTE MFX_play_stereo(UWORD channel_id, ULONG wave, ULONG flags);

// uc_orig: MFX_stop (fallen/DDLibrary/Headers/MFX.h)
void MFX_stop(SLONG channel_id, ULONG wave);
// uc_orig: MFX_stop_attached (fallen/DDLibrary/Headers/MFX.h)
void MFX_stop_attached(Thing* p);

// uc_orig: MFX_set_pitch (fallen/DDLibrary/Headers/MFX.h)
void MFX_set_pitch(UWORD channel_id, ULONG wave, SLONG pitchbend);
// uc_orig: MFX_set_gain (fallen/DDLibrary/Headers/MFX.h)
void MFX_set_gain(UWORD channel_id, ULONG wave, UBYTE gain);
// uc_orig: MFX_set_queue_gain (fallen/DDLibrary/Headers/MFX.h)
void MFX_set_queue_gain(UWORD channel_id, ULONG wave, UBYTE gain);

// uc_orig: MFX_set_listener (fallen/DDLibrary/Headers/MFX.h)
void MFX_set_listener(SLONG x, SLONG y, SLONG z, SLONG heading, SLONG roll, SLONG pitch);

// uc_orig: MFX_load_wave_list (fallen/DDLibrary/Headers/MFX.h)
void MFX_load_wave_list();
// uc_orig: MFX_free_wave_list (fallen/DDLibrary/Headers/MFX.h)
void MFX_free_wave_list();

// uc_orig: MFX_get_wave (fallen/DDLibrary/Headers/MFX.h)
UWORD MFX_get_wave(UWORD channel_id, UBYTE index = 0);

// uc_orig: MFX_update (fallen/DDLibrary/Headers/MFX.h)
void MFX_update(void);

// uc_orig: MFX_init (fallen/DDLibrary/Headers/MFX.h)
void MFX_init();
// uc_orig: MFX_term (fallen/DDLibrary/Headers/MFX.h)
void MFX_term();

// uc_orig: MFX_QUICK_play (fallen/DDLibrary/Headers/MFX.h)
SLONG MFX_QUICK_play(CBYTE* str, SLONG x = 0, SLONG y = 0, SLONG z = 0);
// uc_orig: MFX_QUICK_wait (fallen/DDLibrary/Headers/MFX.h)
void MFX_QUICK_wait(void);
// uc_orig: MFX_QUICK_stop (fallen/DDLibrary/Headers/MFX.h)
void MFX_QUICK_stop();
// uc_orig: MFX_QUICK_still_playing (fallen/DDLibrary/Headers/MFX.h)
SLONG MFX_QUICK_still_playing(void);

#endif // ENGINE_AUDIO_MFX_H
