#ifndef ASSETS_FORMATS_ANIM_H
#define ASSETS_FORMATS_ANIM_H

#include "assets/formats/anim_globals.h"
#include "engine/animation/anim_types.h"

// Initialize all GameKeyFrameChunk pointers to null.
// Must be called before any animation loading.
// uc_orig: ANIM_init (fallen/Source/Anim.cpp)
void ANIM_init(void);

// Free memory for all loaded animation chunks (Darci, Roper, Roper2, CIV).
// uc_orig: ANIM_fini (fallen/Source/Anim.cpp)
void ANIM_fini(void);

// Free all heap allocations inside a GameKeyFrameChunk and null its pointers.
// uc_orig: free_game_chunk (fallen/Source/Anim.cpp)
void free_game_chunk(GameKeyFrameChunk* the_chunk);

// Initialize multi-object anim data in all anim_chunk slots.
// uc_orig: init_anim_prims (fallen/Source/Anim.cpp)
void init_anim_prims(void);

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

#endif // ASSETS_FORMATS_ANIM_H
