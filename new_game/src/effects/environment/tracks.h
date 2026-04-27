#ifndef EFFECTS_ENVIRONMENT_TRACKS_H
#define EFFECTS_ENVIRONMENT_TRACKS_H

#include "engine/core/types.h"

struct Thing;

// Maximum number of simultaneous track slots.
// uc_orig: TRACK_BUFFER_LENGTH (fallen/Headers/tracks.h)
#define TRACK_BUFFER_LENGTH 300

// uc_orig: TRACK_SURFACE_NONE (fallen/Headers/tracks.h)
#define TRACK_SURFACE_NONE 0
// uc_orig: TRACK_SURFACE_MUDDY (fallen/Headers/tracks.h)
#define TRACK_SURFACE_MUDDY 1
// uc_orig: TRACK_SURFACE_WATER (fallen/Headers/tracks.h)
#define TRACK_SURFACE_WATER 2
// uc_orig: TRACK_SURFACE_ONSNOW (fallen/Headers/tracks.h)
#define TRACK_SURFACE_ONSNOW 3

// uc_orig: TRACK_TYPE_TYRE (fallen/Headers/tracks.h)
#define TRACK_TYPE_TYRE 1
// uc_orig: TRACK_TYPE_TYRE_SKID (fallen/Headers/tracks.h)
#define TRACK_TYPE_TYRE_SKID 2
// uc_orig: TRACK_TYPE_LEFT_PRINT (fallen/Headers/tracks.h)
#define TRACK_TYPE_LEFT_PRINT 3
// uc_orig: TRACK_TYPE_RIGHT_PRINT (fallen/Headers/tracks.h)
#define TRACK_TYPE_RIGHT_PRINT 4

// uc_orig: TRACK_FLAGS_FLIPABLE (fallen/Headers/tracks.h)
#define TRACK_FLAGS_FLIPABLE 1
// uc_orig: TRACK_FLAGS_SPLUTTING (fallen/Headers/tracks.h)
#define TRACK_FLAGS_SPLUTTING 2
// uc_orig: TRACK_FLAGS_INVALPHA (fallen/Headers/tracks.h)
#define TRACK_FLAGS_INVALPHA 4

// One tyre track or footprint decal entry. Linked to a Thing for map placement.
// uc_orig: Track (fallen/Headers/tracks.h)
struct Track {
    SLONG dx, dy, dz;       // direction vector
    SLONG page, colour;     // texture page and ARGB colour
    UWORD thing;            // THING_INDEX of the owning secondary Thing
    SWORD sx, sz;           // half-width offsets perpendicular to direction
    UWORD padtolong;
    UBYTE flip;
    UBYTE flags;            // TRACK_FLAGS_*
    UBYTE splut;            // current splut fade counter
    UBYTE splutmax;         // max splut value (fades alpha over time)
};

// uc_orig: TrackPtr (fallen/Headers/tracks.h)
typedef Track* TrackPtr;

// uc_orig: tracks (fallen/Headers/tracks.h)
extern Track* tracks;
// uc_orig: track_head (fallen/Headers/tracks.h)
extern UWORD track_head;
// uc_orig: track_tail (fallen/Headers/tracks.h)
extern UWORD track_tail;
// uc_orig: track_eob (fallen/Headers/tracks.h)
extern UWORD track_eob;

// uc_orig: TO_TRACK (fallen/Headers/tracks.h)
#define TO_TRACK(x) (&tracks[x])
// uc_orig: TRACK_NUMBER (fallen/Headers/tracks.h)
#define TRACK_NUMBER(x) (UWORD)(x - TO_TRACK(0))

// Initialise track ring buffer (called once at startup).
// uc_orig: TRACKS_InitOnce (fallen/Headers/tracks.h)
void TRACKS_InitOnce(SWORD size = TRACK_BUFFER_LENGTH);

// Draw a single track Thing (called from drawxtra.cpp).
// uc_orig: TRACKS_DrawTrack (fallen/Headers/tracks.h)
void TRACKS_DrawTrack(Thing* p_thing);

// Compute perpendicular half-width offsets (sx/sz) for a track given its width.
// uc_orig: TRACKS_CalcDiffs (fallen/Headers/tracks.h)
void TRACKS_CalcDiffs(Track& track, UBYTE width);

// Add a track decal by supplying all parameters directly.
// uc_orig: TRACKS_AddQuad (fallen/Headers/tracks.h)
void TRACKS_AddQuad(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, SLONG page, SLONG colour, UBYTE width, UBYTE flip, UBYTE flags);

// Add a pre-filled Track entry to the ring buffer.
// uc_orig: TRACKS_AddTrack (fallen/Headers/tracks.h)
void TRACKS_AddTrack(Track& track);

// Intelligently add a track at position given type; returns updated (age<<8|lastkind) state.
// uc_orig: TRACKS_Add (fallen/Headers/tracks.h)
UWORD TRACKS_Add(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, UBYTE type, UWORD last);

// Determine ground surface type (water/mud/gravel/etc.) at world position XZ.
// uc_orig: TRACKS_GroundAtXZ (fallen/Headers/tracks.h)
SLONG TRACKS_GroundAtXZ(SLONG X, SLONG Z);

// Add a blood splatter track at the bleeder's foot position.
// uc_orig: TRACKS_Bleed (fallen/Headers/tracks.h)
void TRACKS_Bleed(Thing* bleeder);

// Add a blood pool track at the bleeder's pelvis position.
// uc_orig: TRACKS_Bloodpool (fallen/Headers/tracks.h)
void TRACKS_Bloodpool(Thing* bleeder);

#endif // EFFECTS_ENVIRONMENT_TRACKS_H
