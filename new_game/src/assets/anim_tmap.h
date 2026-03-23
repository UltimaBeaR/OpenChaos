#ifndef ASSETS_ANIM_TMAP_H
#define ASSETS_ANIM_TMAP_H

// Animated texture map system.
// A texture can cycle through up to 16 UV frames at runtime.
// The data is loaded from "data/tmap.ani" at level start.

#include "core/types.h"

// uc_orig: MAX_TMAP_FRAMES (fallen/Headers/animtmap.h)
#define MAX_TMAP_FRAMES 16
// uc_orig: MAX_ANIM_TMAPS (fallen/Headers/animtmap.h)
#define MAX_ANIM_TMAPS 16

// Describes one animated texture: up to 16 UV frames with per-frame delay and page.
// Flags != 0 means the slot is active.
// uc_orig: AnimTmap (fallen/Headers/animtmap.h)
struct AnimTmap {
    UBYTE UV[MAX_TMAP_FRAMES][4][2]; // UV coordinates for each corner of each frame
    SBYTE Delay[MAX_TMAP_FRAMES];    // tick delay before advancing to next frame
    UBYTE Page[MAX_TMAP_FRAMES];     // texture page index for each frame
    UWORD Current;                    // index of the currently displayed frame
    UWORD Timer;                      // ticks elapsed on current frame
    UWORD Flags;                      // non-zero = slot in use
};

// The global pool of animated texture entries.
// uc_orig: anim_tmaps (fallen/Headers/animtmap.h)
extern struct AnimTmap anim_tmaps[MAX_ANIM_TMAPS];

// Resets Current and Timer on all active entries (called after level load).
// uc_orig: sync_animtmaps (fallen/Source/animtmap.cpp)
void sync_animtmaps(void);

// Advances all active entries by one tick, cycling frames as their delays expire.
// uc_orig: animate_texture_maps (fallen/Source/animtmap.cpp)
void animate_texture_maps(void);

// Loads animated texture data from "data/tmap.ani".
// uc_orig: load_animtmaps (fallen/Source/animtmap.cpp)
void load_animtmaps(void);

// Saves animated texture data to "data/tmap.ani".
// uc_orig: save_animtmaps (fallen/Source/animtmap.cpp)
void save_animtmaps(void);

#endif // ASSETS_ANIM_TMAP_H
