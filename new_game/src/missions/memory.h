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
// Used by load_whole_anims().
// uc_orig: convert_keyframe_to_pointer (fallen/Source/memory.cpp)
void convert_keyframe_to_pointer(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count);
// uc_orig: convert_animlist_to_pointer (fallen/Source/memory.cpp)
void convert_animlist_to_pointer(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count);
// uc_orig: convert_fightcol_to_pointer (fallen/Source/memory.cpp)
void convert_fightcol_to_pointer(GameFightCol* p, GameFightCol* p_fight, SLONG count);

// Internal helpers for per-thing index-to-pointer conversion.
// uc_orig: convert_drawtype_to_pointer (fallen/Source/memory.cpp)
void convert_drawtype_to_pointer(Thing* p_thing, SLONG meshtype);
// uc_orig: convert_thing_to_pointer (fallen/Source/memory.cpp)
void convert_thing_to_pointer(Thing* p_thing);

// Level memory table initialization — allocates all game arrays from a single flat block.
// uc_orig: init_memory (fallen/Source/memory.cpp)
void init_memory(void);

// Pointer/index conversion for loading: restores all integer offsets back to pointers.
// uc_orig: convert_index_to_pointers (fallen/Source/memory.cpp)
void convert_index_to_pointers(void);

// Deserializes animation chunk data from a raw memory buffer.
// uc_orig: load_whole_anims (fallen/Source/memory.cpp)
void load_whole_anims(UBYTE* p_all);

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

// uc_orig: MEMORY_quick_load (fallen/Headers/memory.h)
SLONG MEMORY_quick_load(void);

#endif // MISSIONS_MEMORY_H
