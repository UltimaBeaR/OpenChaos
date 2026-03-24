#include <string.h>
#include "engine/platform/uc_common.h"
#include "assets/formats/anim_globals.h"

// uc_orig: global_anim_array (fallen/Source/interact.cpp)
struct GameKeyFrame* global_anim_array[4][450];

// uc_orig: test_chunk (fallen/Source/interact.cpp)
struct KeyFrameChunk* test_chunk;
// uc_orig: test_chunk2 (fallen/Source/interact.cpp)
struct KeyFrameChunk test_chunk2;
// uc_orig: test_chunk3 (fallen/Source/interact.cpp)
struct KeyFrameChunk test_chunk3;
// uc_orig: thug_chunk (fallen/Source/interact.cpp)
struct KeyFrameChunk thug_chunk;
// uc_orig: the_elements (fallen/Source/interact.cpp)
struct KeyFrameElement* the_elements;

// uc_orig: game_chunk (fallen/Source/interact.cpp)
struct GameKeyFrameChunk game_chunk[MAX_GAME_CHUNKS];
// uc_orig: anim_chunk (fallen/Source/interact.cpp)
struct GameKeyFrameChunk anim_chunk[MAX_ANIM_CHUNKS];

// uc_orig: next_game_chunk (fallen/Source/interact.cpp)
SLONG next_game_chunk = 0;
// uc_orig: next_anim_chunk (fallen/Source/interact.cpp)
SLONG next_anim_chunk = 0;

// uc_orig: anim_prim_bbox (fallen/Source/interact.cpp)
AnimPrimBbox anim_prim_bbox[MAX_ANIM_CHUNKS];

// ---- Globals from Anim.cpp ----

// uc_orig: next_prim_point (fallen/Source/Anim.cpp)
UWORD next_prim_point = 1;
// uc_orig: next_prim_face4 (fallen/Source/Anim.cpp)
UWORD next_prim_face4 = 1;
// uc_orig: next_prim_face3 (fallen/Source/Anim.cpp)
UWORD next_prim_face3 = 1;
// uc_orig: next_prim_object (fallen/Source/Anim.cpp)
UWORD next_prim_object = 1;
// uc_orig: next_prim_multi_object (fallen/Source/Anim.cpp)
UWORD next_prim_multi_object = 1;

// uc_orig: roper_pickup (fallen/Source/Anim.cpp)
UBYTE roper_pickup = 0;

// uc_orig: prim_multi_anims (fallen/Source/Anim.cpp)
struct PrimMultiAnim prim_multi_anims[10000];
// uc_orig: next_prim_multi_anim (fallen/Source/Anim.cpp)
UWORD next_prim_multi_anim = 1;

// uc_orig: estate (fallen/Source/Anim.cpp)
UBYTE estate = 0;
// uc_orig: semtex (fallen/Source/Anim.cpp)
UBYTE semtex = 0;
