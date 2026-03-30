#ifndef MISSIONS_MEMORY_H
#define MISSIONS_MEMORY_H

// Game-level memory management and level save/load system.
//
// Manages the central memory allocation table (save_table[]) which describes
// every major game array (things, people, vehicles, EWAY data, etc.).
// On level load: allocates a single flat block and partitions it among all arrays.
// On save/load: serializes and restores all array contents plus pointer-to-index fixups.
//
// Also contains the quick save/load system (F5/F9 in PC build) via MEMORY_quick_*.

#include "engine/core/types.h"
#include "things/core/thing.h"
#include "engine/animation/anim_types.h"
#include "missions/memory_globals.h"

// Memory table type flags used in save_table[] entries.
// uc_orig: MEM_DYNAMIC (fallen/Source/memory.cpp)
#define MEM_DYNAMIC 1
// uc_orig: MEM_STATIC (fallen/Source/memory.cpp)
#define MEM_STATIC 2

// Internal helpers: pointer/index conversion for individual keyframe structures.
// Used by save_whole_anims() and load_whole_anims().
// uc_orig: convert_keyframe_to_index (fallen/Source/memory.cpp)
void convert_keyframe_to_index(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count);
// uc_orig: convert_animlist_to_index (fallen/Source/memory.cpp)
void convert_animlist_to_index(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count);
// uc_orig: convert_fightcol_to_index (fallen/Source/memory.cpp)
void convert_fightcol_to_index(GameFightCol* p, GameFightCol* p_fight, SLONG count);
// uc_orig: convert_keyframe_to_pointer (fallen/Source/memory.cpp)
void convert_keyframe_to_pointer(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count);
// uc_orig: convert_animlist_to_pointer (fallen/Source/memory.cpp)
void convert_animlist_to_pointer(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count);
// uc_orig: convert_fightcol_to_pointer (fallen/Source/memory.cpp)
void convert_fightcol_to_pointer(GameFightCol* p, GameFightCol* p_fight, SLONG count);

// Internal helpers for per-thing pointer/index conversion.
// uc_orig: convert_drawtype_to_index (fallen/Source/memory.cpp)
void convert_drawtype_to_index(Thing* p_thing, SLONG meshtype);
// uc_orig: convert_thing_to_index (fallen/Source/memory.cpp)
void convert_thing_to_index(Thing* p_thing);

// Internal helpers for per-thing index-to-pointer conversion (inverse of above).
// uc_orig: convert_drawtype_to_pointer (fallen/Source/memory.cpp)
void convert_drawtype_to_pointer(Thing* p_thing, SLONG meshtype);
// uc_orig: convert_thing_to_pointer (fallen/Source/memory.cpp)
void convert_thing_to_pointer(Thing* p_thing);

// Level memory table initialization — allocates all game arrays from a single flat block.
// uc_orig: init_memory (fallen/Source/memory.cpp)
void init_memory(void);

// Frees all per-level memory allocations.
// uc_orig: release_memory (fallen/Source/memory.cpp)
void release_memory(void);

// Packs prim normals into compressed 15-bit format for Darci's body.
// uc_orig: set_darci_normals (fallen/Source/memory.cpp)
void set_darci_normals(void);

// Pointer/index conversion for saving: replaces all pointer fields in Things
// with integer offsets relative to their respective arrays.
// uc_orig: convert_pointers_to_index (fallen/Source/memory.cpp)
void convert_pointers_to_index(void);

// Pointer/index conversion for loading: restores all integer offsets back to pointers.
// uc_orig: convert_index_to_pointers (fallen/Source/memory.cpp)
void convert_index_to_pointers(void);

// Serializes all animation chunk data (game_chunk + anim_chunk) to a file handle.
// uc_orig: save_whole_anims (fallen/Source/memory.cpp)
void save_whole_anims(MFFileHandle handle);

// Deserializes animation chunk data from a raw memory buffer.
// uc_orig: load_whole_anims (fallen/Source/memory.cpp)
void load_whole_anims(UBYTE* p_all);

// Finds or creates a reusable animation origin offset in the anim_mids[] table.
// Searches by position similarity; returns index into anim_mids[].
// uc_orig: find_best_anim_offset (fallen/Source/memory.cpp)
SLONG find_best_anim_offset(SLONG mx, SLONG my, SLONG mz, SLONG anim, struct GameKeyFrameChunk* gc);

// Older version of find_best_anim_offset using simple distance to existing mids.
// uc_orig: find_best_anim_offset_old (fallen/Source/memory.cpp)
SLONG find_best_anim_offset_old(SLONG mx, SLONG my, SLONG mz, SLONG bdist);

// Flags all near-vertical prim faces (FaceFlags |= FACE_FLAG_VERTICAL).
// uc_orig: flag_v_faces (fallen/Source/memory.cpp)
void flag_v_faces(void);

// Saves the full game state to a file (Dreamcast .dad format / old PC save format).
// uc_orig: save_whole_game (fallen/Source/memory.cpp)
void save_whole_game(CBYTE* gamename);

// Loads the full game state from a file (old PC/Dreamcast binary format).
// uc_orig: load_whole_game (fallen/Source/memory.cpp)
void load_whole_game(CBYTE* gamename);

// Resets the dynamic lighting caches for all facets.
// uc_orig: uncache (fallen/Source/memory.cpp)
void uncache(void);

// Quick save to "data/quicksave.dat" using stdio (fast, PC-only).
// uc_orig: MEMORY_quick_init (fallen/Headers/memory.h)
void MEMORY_quick_init(void);

// uc_orig: MEMORY_quick_save (fallen/Headers/memory.h)
void MEMORY_quick_save(void);

// uc_orig: MEMORY_quick_load_available (fallen/Headers/memory.h)
SLONG MEMORY_quick_load_available(void);

// uc_orig: MEMORY_quick_load (fallen/Headers/memory.h)
SLONG MEMORY_quick_load(void);

#endif // MISSIONS_MEMORY_H
