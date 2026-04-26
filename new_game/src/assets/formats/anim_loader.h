#ifndef ASSETS_FORMATS_ANIM_LOADER_H
#define ASSETS_FORMATS_ANIM_LOADER_H

#include "engine/core/types.h"
#include "engine/io/file.h"
#include <stdio.h>

// Forward declarations for types defined in old/ headers that are not yet migrated.
struct KeyFrameChunk;
struct GameKeyFrameChunk;
struct Matrix33;
struct KeyFrameElement;

// Read one sub-object block from an open .moj or embedded .all file handle.
// Appends points, tri faces, and quad faces to the global prim arrays.
// uc_orig: read_a_prim (fallen/Source/io.cpp)
void read_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type);

// Load a 256-entry 8-bit palette file into ENGINE_palette[].
// On read failure, fills palette entries with random colours.
// uc_orig: load_palette (fallen/Source/io.cpp)
void load_palette(CBYTE* palette);

// Load the GameKeyFrameChunk section from an open .all file handle.
// Allocates all sub-arrays (PeopleTypes, AnimKeyFrames, AnimList, TheElements, FightCols)
// and relocates stored runtime pointers to the new allocations.
// uc_orig: load_insert_game_chunk (fallen/Source/io.cpp)
SLONG load_insert_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk);

// Read one .moj multi-prim block from an already-open .all file handle (no filename).
// Used for loading embedded .moj blocks from within a .all bundle file.
// uc_orig: load_insert_a_multi_prim (fallen/Source/io.cpp)
SLONG load_insert_a_multi_prim(MFFileHandle handle);

// Load a complete .all animation bundle (mesh + keyframe data) into a GameKeyFrameChunk.
// type controls which skin variants to load (0=all, 1=people, 2=thugs).
// uc_orig: load_anim_system (fallen/Source/io.cpp)
SLONG load_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG type = 0);

// Append additional animation data from a .all file into an existing GameKeyFrameChunk
// without freeing the existing data. Used for combining multiple animation sets.
// uc_orig: load_append_game_chunk (fallen/Source/io.cpp)
SLONG load_append_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk, SLONG start_frame);

// Skip past one sub-object block in a .all or .moj file without loading data.
// Used to conditionally skip unwanted skin variants.
// uc_orig: skip_a_prim (fallen/Source/io.cpp)
void skip_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type);

// Skip past one entire .moj block in an open .all file handle.
// uc_orig: skip_load_a_multi_prim (fallen/Source/io.cpp)
void skip_load_a_multi_prim(MFFileHandle handle);

// Append animation data from a .all file into an existing GameKeyFrameChunk,
// optionally loading mesh data. Starts filling AnimList at start_anim.
// uc_orig: append_anim_system (fallen/Source/io.cpp)
SLONG append_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG start_anim, SLONG load_mesh);

#endif // ASSETS_FORMATS_ANIM_LOADER_H
