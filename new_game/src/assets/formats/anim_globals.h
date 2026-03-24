#ifndef ASSETS_FORMATS_ANIM_GLOBALS_H
#define ASSETS_FORMATS_ANIM_GLOBALS_H

#include <string.h>
#include "engine/platform/uc_common.h"
#include "engine/animation/anim_types.h"

// ---- Globals from Anim.cpp ----

// Next free slot in prim_points[] (world/character vertex positions).
// Initialized to 1; slot 0 is reserved as an invalid sentinel.
// uc_orig: next_prim_point (fallen/Source/Anim.cpp)
extern UWORD next_prim_point;
// uc_orig: next_prim_face4 (fallen/Source/Anim.cpp)
extern UWORD next_prim_face4;
// uc_orig: next_prim_face3 (fallen/Source/Anim.cpp)
extern UWORD next_prim_face3;
// uc_orig: next_prim_object (fallen/Source/Anim.cpp)
extern UWORD next_prim_object;
// uc_orig: next_prim_multi_object (fallen/Source/Anim.cpp)
extern UWORD next_prim_multi_object;

// Set when the Roper pickup animation has been loaded.
// uc_orig: roper_pickup (fallen/Source/Anim.cpp)
extern UBYTE roper_pickup;

// Multi-part animation data for character body variants.
// uc_orig: prim_multi_anims (fallen/Source/Anim.cpp)
extern struct PrimMultiAnim prim_multi_anims[10000];
// uc_orig: next_prim_multi_anim (fallen/Source/Anim.cpp)
extern UWORD next_prim_multi_anim;

// Set when the current level uses the estate variant (banesuit models).
// uc_orig: estate (fallen/Source/Anim.cpp)
extern UBYTE estate;

// Set when the current level is semtex.ucm (affects combat animation setup).
// uc_orig: semtex (fallen/Source/Anim.cpp)
extern UBYTE semtex;

// Maximum number of animation chunks in the runtime system.
// uc_orig: MAX_GAME_CHUNKS (fallen/Headers/interact.h)
#define MAX_GAME_CHUNKS 5
// uc_orig: MAX_ANIM_CHUNKS (fallen/Headers/interact.h)
#define MAX_ANIM_CHUNKS 16

// Bounding box of an animation prim in its initial pose.
// Computed by find_anim_prim_bboxes() during loading.
// uc_orig: AnimPrimBbox (fallen/Headers/interact.h)
typedef struct {
    SLONG minx;
    SLONG miny;
    SLONG minz;
    SLONG maxx;
    SLONG maxy;
    SLONG maxz;
} AnimPrimBbox;

// 2D table mapping [character type][action] to a keyframe list pointer.
// Filled by setup_global_anim_array() during startup.
// uc_orig: global_anim_array (fallen/Source/interact.cpp)
extern struct GameKeyFrame* global_anim_array[4][450];

// Legacy chunk structures (used by older animation loading code paths).
// uc_orig: test_chunk (fallen/Source/interact.cpp)
extern struct KeyFrameChunk* test_chunk;
// uc_orig: test_chunk2 (fallen/Source/interact.cpp)
extern struct KeyFrameChunk test_chunk2;
// uc_orig: test_chunk3 (fallen/Source/interact.cpp)
extern struct KeyFrameChunk test_chunk3;
// uc_orig: thug_chunk (fallen/Source/interact.cpp)
extern struct KeyFrameChunk thug_chunk;
// uc_orig: the_elements (fallen/Source/interact.cpp)
extern struct KeyFrameElement* the_elements;

// Runtime animation data for the four character type slots.
// uc_orig: game_chunk (fallen/Source/interact.cpp)
extern struct GameKeyFrameChunk game_chunk[MAX_GAME_CHUNKS];
// Editor-side animation chunks (not used at runtime).
// uc_orig: anim_chunk (fallen/Source/interact.cpp)
extern struct GameKeyFrameChunk anim_chunk[MAX_ANIM_CHUNKS];

// Next free slot index in game_chunk[] and anim_chunk[].
// uc_orig: next_game_chunk (fallen/Source/interact.cpp)
extern SLONG next_game_chunk;
// uc_orig: next_anim_chunk (fallen/Source/interact.cpp)
extern SLONG next_anim_chunk;

// Bounding boxes for animating prims, indexed by anim_chunk slot.
// uc_orig: anim_prim_bbox (fallen/Source/interact.cpp)
extern AnimPrimBbox anim_prim_bbox[MAX_ANIM_CHUNKS];

#endif // ASSETS_FORMATS_ANIM_GLOBALS_H
