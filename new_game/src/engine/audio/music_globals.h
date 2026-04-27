#ifndef ENGINE_AUDIO_MUSIC_GLOBALS_H
#define ENGINE_AUDIO_MUSIC_GLOBALS_H

#include "engine/core/types.h"

// uc_orig: music_current_wave (fallen/Source/music.cpp)
extern UWORD music_current_wave;
// uc_orig: music_max_gain (fallen/Source/music.cpp)
extern UWORD music_max_gain;
// uc_orig: music_current_mode (fallen/Source/music.cpp)
extern UBYTE music_current_mode;
// uc_orig: music_request_mode (fallen/Source/music.cpp)
extern UBYTE music_request_mode;
// uc_orig: music_current_level (fallen/Source/music.cpp)
extern SLONG music_current_level;
// uc_orig: music_mode_override (fallen/Source/music.cpp)
extern UBYTE music_mode_override;
// uc_orig: MUSIC_bodge_code (fallen/Source/music.cpp)
extern UBYTE MUSIC_bodge_code;
// uc_orig: just_asked_for_mode_now (fallen/Source/music.cpp)
extern UBYTE just_asked_for_mode_now;
// uc_orig: just_asked_for_mode_number (fallen/Source/music.cpp)
extern UBYTE just_asked_for_mode_number;

#endif // ENGINE_AUDIO_MUSIC_GLOBALS_H
