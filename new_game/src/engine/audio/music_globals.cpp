#include "engine/audio/music_globals.h"

// uc_orig: music_current_wave (fallen/Source/music.cpp)
UWORD music_current_wave = 0;
// uc_orig: music_max_gain (fallen/Source/music.cpp)
UWORD music_max_gain = 128 << 8;
// uc_orig: music_current_mode (fallen/Source/music.cpp)
UBYTE music_current_mode = 0;
// uc_orig: music_request_mode (fallen/Source/music.cpp)
UBYTE music_request_mode = 0;
// uc_orig: music_current_level (fallen/Source/music.cpp)
SLONG music_current_level = 0;
// uc_orig: music_mode_override (fallen/Source/music.cpp)
UBYTE music_mode_override = 0;
// uc_orig: MUSIC_bodge_code (fallen/Source/music.cpp)
UBYTE MUSIC_bodge_code = 0;
// uc_orig: just_asked_for_mode_now (fallen/Source/music.cpp)
UBYTE just_asked_for_mode_now = 0;
// uc_orig: just_asked_for_mode_number (fallen/Source/music.cpp)
UBYTE just_asked_for_mode_number = 0;
