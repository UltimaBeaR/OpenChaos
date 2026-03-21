#include <string.h>
#include <MFStdLib.h>
#include "assets/anim_globals.h"

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
