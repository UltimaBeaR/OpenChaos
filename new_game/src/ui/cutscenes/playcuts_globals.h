#ifndef UI_CUTSCENES_PLAYCUTS_GLOBALS_H
#define UI_CUTSCENES_PLAYCUTS_GLOBALS_H

#include <windows.h>
#include "ui/cutscenes/playcuts.h"

// Current text string to display on screen during playback (NULL = none).
// uc_orig: text_disp (fallen/Source/playcuts.cpp)
extern CBYTE* text_disp;

// Global fade level: 255 = fully opaque, 0 = black overlay.
// uc_orig: PLAYCUTS_fade_level (fallen/Source/playcuts.cpp)
extern UBYTE PLAYCUTS_fade_level;

// uc_orig: PLAYCUTS_slomo (fallen/Source/playcuts.cpp)
extern BOOL PLAYCUTS_slomo;

// Set while a cutscene is running.
// uc_orig: PLAYCUTS_playing (fallen/Source/playcuts.cpp)
extern BOOL PLAYCUTS_playing;

// uc_orig: PLAYCUTS_slomo_ctr (fallen/Source/playcuts.cpp)
extern UBYTE PLAYCUTS_slomo_ctr;

// Set when all channels have passed their last packet.
// uc_orig: no_more_packets (fallen/Source/playcuts.cpp)
extern UBYTE no_more_packets;

// Static pools for loaded cutscene data (pool-based; reset with PLAYCUTS_Reset).
// uc_orig: PLAYCUTS_text_data (fallen/Source/playcuts.cpp)
extern CBYTE PLAYCUTS_text_data[MAX_CUTSCENE_TEXT];
// uc_orig: PLAYCUTS_text_ptr (fallen/Source/playcuts.cpp)
extern CBYTE* PLAYCUTS_text_ptr;
// uc_orig: PLAYCUTS_cutscenes (fallen/Source/playcuts.cpp)
extern CPData PLAYCUTS_cutscenes[MAX_CUTSCENES];
// uc_orig: PLAYCUTS_packets (fallen/Source/playcuts.cpp)
extern CPPacket PLAYCUTS_packets[MAX_CUTSCENE_PACKETS];
// uc_orig: PLAYCUTS_tracks (fallen/Source/playcuts.cpp)
extern CPChannel PLAYCUTS_tracks[MAX_CUTSCENE_TRACKS];
// uc_orig: PLAYCUTS_cutscene_ctr (fallen/Source/playcuts.cpp)
extern UWORD PLAYCUTS_cutscene_ctr;
// uc_orig: PLAYCUTS_packet_ctr (fallen/Source/playcuts.cpp)
extern UWORD PLAYCUTS_packet_ctr;
// uc_orig: PLAYCUTS_track_ctr (fallen/Source/playcuts.cpp)
extern UWORD PLAYCUTS_track_ctr;
// uc_orig: PLAYCUTS_text_ctr (fallen/Source/playcuts.cpp)
extern UWORD PLAYCUTS_text_ctr;

#endif // UI_CUTSCENES_PLAYCUTS_GLOBALS_H
