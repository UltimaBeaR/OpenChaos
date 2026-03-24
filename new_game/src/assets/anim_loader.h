#ifndef ASSETS_ANIM_LOADER_H
#define ASSETS_ANIM_LOADER_H

#include "engine/core/types.h"
#include "engine/io/file.h"
#include <stdio.h>

// Forward declarations for types defined in old/ headers that are not yet migrated.
struct KeyFrameChunk;
struct GameKeyFrameChunk;
struct Matrix33;
struct KeyFrameElement;

// Load frame ordering from a .TXT file alongside a .VUE animation file.
// The .TXT controls which frames to include and their order in the final animation.
// If the .TXT is missing, frames are loaded in sequential order (0..max-1).
// uc_orig: load_frame_numbers (fallen/Source/io.cpp)
void load_frame_numbers(CBYTE* vue_name, UWORD* frames, SLONG max_frames);

// Sort body part sub-objects within an animation chunk by matching them to named body parts.
// Currently a no-op body (code commented out in the original).
// uc_orig: sort_multi_object (fallen/Source/io.cpp)
void sort_multi_object(struct KeyFrameChunk* the_chunk);

// Initialise all PeopleTypes[] entries in a KeyFrameChunk with defaults ("Undefined").
// Called before loading VUE animation data so unspecified slots have predictable values.
// uc_orig: set_default_people_types (fallen/Source/io.cpp)
void set_default_people_types(struct KeyFrameChunk* the_chunk);

// Pack a 3×3 floating-point rotation matrix into a compressed KeyFrameElement CMatrix.
// Three rows are encoded at 10 bits each into three 32-bit words.
// uc_orig: make_compress_matrix (fallen/Source/io.cpp)
void make_compress_matrix(struct KeyFrameElement* the_element, struct Matrix33* matrix);

// Load a .VUE text-format animation file into a KeyFrameChunk.
// Parses frame/transform records, builds KeyFrame and KeyFrameElement arrays in place.
// uc_orig: load_multi_vue (fallen/Source/io.cpp)
void load_multi_vue(struct KeyFrameChunk* the_chunk, float shrink_me);

// Load a .moj multi-prim file by name into the prim database.
// Returns the new next_prim_multi_object-1 index, or 0 on failure.
// uc_orig: load_anim_mesh (fallen/Source/io.cpp)
SLONG load_anim_mesh(CBYTE* fname, float scale);

// Load all mesh (.moj) and animation (.VUE) data for a KeyFrameChunk by name.
// Constructs Data\ paths, loads all body-part variants, then calls load_multi_vue().
// uc_orig: load_key_frame_chunks (fallen/Source/io.cpp)
void load_key_frame_chunks(KeyFrameChunk* the_chunk, CBYTE* vue_name, float scale = 1.0);

// Read one sub-object block from an open .moj or embedded .all file handle.
// Appends points, tri faces, and quad faces to the global prim arrays.
// uc_orig: read_a_prim (fallen/Source/io.cpp)
void read_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type);

// Load a standalone .moj file by name into the prim database.
// Returns new next_prim_multi_object-1 on success, 0 on failure.
// uc_orig: load_a_multi_prim (fallen/Source/io.cpp)
SLONG load_a_multi_prim(CBYTE* name);

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

#endif // ASSETS_ANIM_LOADER_H
