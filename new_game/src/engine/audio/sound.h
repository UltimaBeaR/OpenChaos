#ifndef ENGINE_AUDIO_SOUND_H
#define ENGINE_AUDIO_SOUND_H

#include "engine/audio/mfx.h"
#include "engine/audio/sound_globals.h"
#include "assets/sound_id.h"

// WAVE_PLAY_* constants for MFX flags (corrected spelling from MFStdLib.h versions).
// uc_orig: WAVE_PLAY_INTERRUPT (fallen/Headers/Sound.h)
#define WAVE_PLAY_INTERRUPT 0

// uc_orig: WAVE_PLAY_NO_INTERRUPT (fallen/Headers/Sound.h)
#define WAVE_PLAY_NO_INTERRUPT 1

// uc_orig: WAVE_PLAY_INTERRUPT_LOOPS (fallen/Headers/Sound.h)
#define WAVE_PLAY_INTERRUPT_LOOPS 4

// World biome type constants used for ambient sound selection.
// uc_orig: WORLD_TYPE_CITY_POP (fallen/Headers/Sound.h)
#define WORLD_TYPE_CITY_POP 1

// uc_orig: WORLD_TYPE_CITY_UNPOP (fallen/Headers/Sound.h)
#define WORLD_TYPE_CITY_UNPOP 2

// uc_orig: WORLD_TYPE_FOREST (fallen/Headers/Sound.h)
#define WORLD_TYPE_FOREST 3

// uc_orig: WORLD_TYPE_SNOW (fallen/Headers/Sound.h)
#define WORLD_TYPE_SNOW 4

// uc_orig: WORLD_TYPE_DESERT (fallen/Headers/Sound.h)
#define WORLD_TYPE_DESERT 5

// Reserved audio channel IDs above the Thing index range.
// uc_orig: WIND_REF (fallen/Headers/Sound.h)
#define WIND_REF MAX_THINGS + 100

// uc_orig: WEATHER_REF (fallen/Headers/Sound.h)
#define WEATHER_REF (WIND_REF + 1)

// uc_orig: THUNDER_REF (fallen/Headers/Sound.h)
#define THUNDER_REF (WIND_REF + 2)

// uc_orig: SIREN_REF (fallen/Headers/Sound.h)
#define SIREN_REF (WIND_REF + 3)

// uc_orig: AMBIENT_EFFECT_REF (fallen/Headers/Sound.h)
#define AMBIENT_EFFECT_REF (WIND_REF + 4)

// uc_orig: MUSIC_REF (fallen/Headers/Sound.h)
#define MUSIC_REF (WIND_REF + 5)

// Returns a random sound ID in [start, end].
// uc_orig: SOUND_Range (fallen/Headers/Sound.h)
inline SLONG SOUND_Range(SLONG start, SLONG end)
{
    SLONG diff = (end - start) + 1;
    return start + (rand() % diff);
}

// uc_orig: play_glue_wave (fallen/Source/Sound.cpp)
void play_glue_wave(UWORD type, UWORD id, SLONG x = 0, SLONG y = 0, SLONG z = 0);

// uc_orig: process_ambient_effects (fallen/Source/Sound.cpp)
void process_ambient_effects(void);

// uc_orig: process_weather (fallen/Source/Sound.cpp)
void process_weather(void);

// uc_orig: SOUND_reset (fallen/Source/Sound.cpp)
void SOUND_reset();

// uc_orig: SOUND_SewerPrecalc (fallen/Source/Sound.cpp)
void SOUND_SewerPrecalc();

// uc_orig: SOUND_InitFXGroups (fallen/Source/Sound.cpp)
void SOUND_InitFXGroups(CBYTE* fn);

// uc_orig: PainSound (fallen/Source/Sound.cpp)
void PainSound(Thing* p_thing);

// uc_orig: EffortSound (fallen/Source/Sound.cpp)
void EffortSound(Thing* p_thing);

// uc_orig: MinorEffortSound (fallen/Source/Sound.cpp)
void MinorEffortSound(Thing* p_thing);

// uc_orig: ScreamFallSound (fallen/Source/Sound.cpp)
void ScreamFallSound(Thing* p_thing);

// uc_orig: StopScreamFallSound (fallen/Source/Sound.cpp)
void StopScreamFallSound(Thing* p_thing);

// uc_orig: SOUND_Curious (fallen/Source/Sound.cpp)
void SOUND_Curious(Thing* p_thing);

// uc_orig: SOUND_Gender (fallen/Source/Sound.cpp)
UBYTE SOUND_Gender(Thing* p_thing);

// uc_orig: init_ambient (fallen/Source/Sound.cpp)
void init_ambient(void);

// uc_orig: SND_BeginAmbient (fallen/Source/Sound.cpp)
void SND_BeginAmbient();

#endif // ENGINE_AUDIO_SOUND_H
