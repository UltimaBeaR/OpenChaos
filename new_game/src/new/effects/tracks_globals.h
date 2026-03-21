#ifndef EFFECTS_TRACKS_GLOBALS_H
#define EFFECTS_TRACKS_GLOBALS_H

#include "effects/tracks.h"

// Ring buffer of all active tyre/footprint track decals.
// uc_orig: tracks (fallen/Source/tracks.cpp)
extern Track* tracks;
// uc_orig: track_head (fallen/Source/tracks.cpp)
extern UWORD track_head;
// uc_orig: track_tail (fallen/Source/tracks.cpp)
extern UWORD track_tail;
// uc_orig: track_eob (fallen/Source/tracks.cpp)
extern UWORD track_eob;

#endif // EFFECTS_TRACKS_GLOBALS_H
