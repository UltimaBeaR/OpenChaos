#ifndef ASSETS_ANIM_H
#define ASSETS_ANIM_H

#include "assets/anim_globals.h"
#include "engine/animation/anim_types.h"

// Initialize all GameKeyFrameChunk pointers to null.
// Must be called before any animation loading.
// uc_orig: ANIM_init (fallen/Source/Anim.cpp)
void ANIM_init(void);

// Free memory for all loaded animation chunks (Darci, Roper, Roper2, CIV).
// uc_orig: ANIM_fini (fallen/Source/Anim.cpp)
void ANIM_fini(void);

// Convert compressed matrix data from source chunk into game chunk element array.
// uc_orig: convert_elements (fallen/Source/Anim.cpp)
void convert_elements(KeyFrameChunk* the_chunk, GameKeyFrameChunk* game_chunk, UWORD* p_reloc, SLONG max_ele);

// Free all heap allocations inside a GameKeyFrameChunk and null its pointers.
// uc_orig: free_game_chunk (fallen/Source/Anim.cpp)
void free_game_chunk(GameKeyFrameChunk* the_chunk);

// Set next_prim_point to the given value.
// uc_orig: set_next_prim_point (fallen/Source/Anim.cpp)
void set_next_prim_point(SLONG v);

// Initialize multi-object anim data in all anim_chunk slots.
// uc_orig: init_anim_prims (fallen/Source/Anim.cpp)
void init_anim_prims(void);

// Reset editor-side animation state (test_chunk).
// uc_orig: reset_anim_stuff (fallen/Source/Anim.cpp)
void reset_anim_stuff(void);

// Load all .all animation files for all character types used at runtime.
// Called once at game startup before setup_global_anim_array().
// uc_orig: setup_people_anims (fallen/Source/Anim.cpp)
void setup_people_anims(void);

// Build the global_anim_array dispatch table.
// Must be called after setup_people_anims().
// uc_orig: setup_global_anim_array (fallen/Source/Anim.cpp)
void setup_global_anim_array(void);

// Load van animation chunk (slot 5) — currently a no-op placeholder.
// uc_orig: setup_extra_anims (fallen/Source/Anim.cpp)
void setup_extra_anims(void);

// Compress a CMatrix33 into GameKeyFrameElementComp encoding:
// stores 2 of 3 matrix row elements; the 3rd is reconstructed at runtime.
// uc_orig: SetCMatrixComp (fallen/Source/Anim.cpp)
void SetCMatrixComp(GameKeyFrameElementComp* e, CMatrix33* cm);

// Get bone position (offset only) from a keyframe for the given sub-object index.
// Does not apply any tween interpolation.
// uc_orig: calc_sub_objects_position_no_tween (fallen/Source/Anim.cpp)
void calc_sub_objects_position_no_tween(GameKeyFrame* cur_frame, UWORD object, SLONG* x, SLONG* y, SLONG* z);

// Stub: fix multi-object positions for an anim (body not used in current build).
// uc_orig: fix_multi_object_for_anim (fallen/Source/Anim.cpp)
void fix_multi_object_for_anim(UWORD obj, struct PrimMultiAnim* p_anim);

#endif // ASSETS_ANIM_H
