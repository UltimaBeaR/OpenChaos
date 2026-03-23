#ifndef ENGINE_AUDIO_SOUND_GLOBALS_H
#define ENGINE_AUDIO_SOUND_GLOBALS_H

#include "core/types.h"

// uc_orig: SOUNDFXG (fallen/Headers/Sound.h)
typedef UWORD SOUNDFXG[2];

// Internal biome category for ambient sound selection. Used by sound.cpp logic.
// uc_orig: WorldType (fallen/Source/Sound.cpp)
enum WorldType {
    Jungle = 0,
    Snow,
    Estate,
    BusyCity,
    QuietCity
};

// uc_orig: wtype (fallen/Source/Sound.cpp)
extern WorldType wtype;

// uc_orig: SOUND_FXMapping (fallen/Source/Sound.cpp)
extern UBYTE* SOUND_FXMapping;

// uc_orig: SOUND_FXGroups (fallen/Source/Sound.cpp)
extern SOUNDFXG* SOUND_FXGroups;

// uc_orig: world_type (fallen/Source/Sound.cpp)
extern SWORD world_type;

// uc_orig: music_id (fallen/Source/Sound.cpp)
extern SLONG music_id;

// uc_orig: creature_time (fallen/Source/Sound.cpp)
extern SLONG creature_time;

// uc_orig: siren_time (fallen/Source/Sound.cpp)
extern SLONG siren_time;

// uc_orig: in_sewer_time (fallen/Source/Sound.cpp)
extern SLONG in_sewer_time;

// uc_orig: thunder_time (fallen/Source/Sound.cpp)
extern SLONG thunder_time;

// uc_orig: wind_id (fallen/Source/Sound.cpp)
extern SLONG wind_id;

// uc_orig: tick_tock (fallen/Source/Sound.cpp)
extern SLONG tick_tock;

// uc_orig: indoors_id (fallen/Source/Sound.cpp)
extern SLONG indoors_id;

// uc_orig: outdoors_id (fallen/Source/Sound.cpp)
extern SLONG outdoors_id;

// uc_orig: rain_id (fallen/Source/Sound.cpp)
extern SLONG rain_id;

// uc_orig: rain_id2 (fallen/Source/Sound.cpp)
extern SLONG rain_id2;

// uc_orig: thunder_id (fallen/Source/Sound.cpp)
extern SLONG thunder_id;

// uc_orig: indoors_vol (fallen/Source/Sound.cpp)
extern SLONG indoors_vol;

// uc_orig: outdoors_vol (fallen/Source/Sound.cpp)
extern SLONG outdoors_vol;

// uc_orig: weather_vol (fallen/Source/Sound.cpp)
extern SLONG weather_vol;

// uc_orig: music_vol (fallen/Source/Sound.cpp)
extern SLONG music_vol;

// uc_orig: next_music (fallen/Source/Sound.cpp)
extern SLONG next_music;

#endif // ENGINE_AUDIO_SOUND_GLOBALS_H
