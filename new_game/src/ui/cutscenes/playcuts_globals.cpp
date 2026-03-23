#include "ui/cutscenes/playcuts_globals.h"

// uc_orig: text_disp (fallen/Source/playcuts.cpp)
CBYTE* text_disp = 0;
// uc_orig: PLAYCUTS_fade_level (fallen/Source/playcuts.cpp)
UBYTE PLAYCUTS_fade_level = 255;
// uc_orig: PLAYCUTS_slomo (fallen/Source/playcuts.cpp)
BOOL PLAYCUTS_slomo = 0;
// uc_orig: PLAYCUTS_playing (fallen/Source/playcuts.cpp)
BOOL PLAYCUTS_playing = 0;
// uc_orig: PLAYCUTS_slomo_ctr (fallen/Source/playcuts.cpp)
UBYTE PLAYCUTS_slomo_ctr = 0;
// uc_orig: no_more_packets (fallen/Source/playcuts.cpp)
UBYTE no_more_packets = 0;
// uc_orig: PLAYCUTS_text_data (fallen/Source/playcuts.cpp)
CBYTE PLAYCUTS_text_data[MAX_CUTSCENE_TEXT];
// uc_orig: PLAYCUTS_text_ptr (fallen/Source/playcuts.cpp)
CBYTE* PLAYCUTS_text_ptr = PLAYCUTS_text_data;
// uc_orig: PLAYCUTS_cutscenes (fallen/Source/playcuts.cpp)
CPData PLAYCUTS_cutscenes[MAX_CUTSCENES];
// uc_orig: PLAYCUTS_packets (fallen/Source/playcuts.cpp)
CPPacket PLAYCUTS_packets[MAX_CUTSCENE_PACKETS];
// uc_orig: PLAYCUTS_tracks (fallen/Source/playcuts.cpp)
CPChannel PLAYCUTS_tracks[MAX_CUTSCENE_TRACKS];
// uc_orig: PLAYCUTS_cutscene_ctr (fallen/Source/playcuts.cpp)
UWORD PLAYCUTS_cutscene_ctr = 0;
// uc_orig: PLAYCUTS_packet_ctr (fallen/Source/playcuts.cpp)
UWORD PLAYCUTS_packet_ctr = 0;
// uc_orig: PLAYCUTS_track_ctr (fallen/Source/playcuts.cpp)
UWORD PLAYCUTS_track_ctr = 0;
// uc_orig: PLAYCUTS_text_ctr (fallen/Source/playcuts.cpp)
UWORD PLAYCUTS_text_ctr = 0;
