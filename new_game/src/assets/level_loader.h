#ifndef ASSETS_LEVEL_LOADER_H
#define ASSETS_LEVEL_LOADER_H

#include "core/types.h"
#include <stdio.h>

// In-map record for an animated prim placement loaded from the .iam file.
// Used by load_game_map() when reading MAP_THING_TYPE_ANIM_PRIM entries.
// uc_orig: LoadGameThing (fallen/Headers/io.h)
struct LoadGameThing {
    UWORD Type;
    UWORD SubStype;
    SLONG X;
    SLONG Y;
    SLONG Z;
    ULONG Flags;
    UWORD IndexOther;
    UWORD AngleX;
    UWORD AngleY;
    UWORD AngleZ;
    ULONG Dummy[4];
};

// Directory path buffers for game resource lookups.
// Set at startup based on whether a server mount is available.
// uc_orig: EXTRAS_DIR (fallen/Source/io.cpp)
extern CBYTE EXTRAS_DIR[100];
// uc_orig: PRIM_DIR (fallen/Source/io.cpp)
extern CBYTE PRIM_DIR[100];
// uc_orig: DATA_DIR (fallen/Source/io.cpp)
extern CBYTE DATA_DIR[100];
// uc_orig: LEVELS_DIR (fallen/Source/io.cpp)
extern CBYTE LEVELS_DIR[100];
// uc_orig: TEXTURE_WORLD_DIR (fallen/Source/io.cpp)
extern CBYTE TEXTURE_WORLD_DIR[100];

// Save a snapshot of prim database cursor positions (for rollback on load failure).
// uc_orig: record_prim_status (fallen/Source/io.cpp)
void record_prim_status(void);

// Replace the extension of 'name' with 'add' (3-char string), writing result to 'new_name'.
// uc_orig: change_extension (fallen/Source/io.cpp)
void change_extension(CBYTE* name, CBYTE* add, CBYTE* new_name);

// Load instyle.tma: interior texture UV table used for building interiors.
// uc_orig: load_texture_instyles (fallen/Source/io.cpp)
void load_texture_instyles(UBYTE editor, UBYTE world);

// Load style.tma: exterior city tile texture UV and flag table (200 styles × 5 slots).
// uc_orig: load_texture_styles (fallen/Source/io.cpp)
void load_texture_styles(UBYTE editor, UBYTE world);

// Load animNNN.all for a single animated world object slot (bat/gargoyle/balrog/bane etc.).
// Short-circuits if the slot is already loaded.
// uc_orig: load_anim_prim_object (fallen/Source/io.cpp)
SLONG load_anim_prim_object(SLONG prim);

// Load anim prims required by EWAY waypoints (EWAY_DO_CREATE_ANIMAL events).
// uc_orig: load_level_anim_prims (fallen/Source/io.cpp)
void load_level_anim_prims(void);

// Load the .iam map file: PAP grid, anim prim placements, OB objects, super map.
// uc_orig: load_game_map (fallen/Source/io.cpp)
void load_game_map(CBYTE* name);

// Legacy bulk loader: loads all 265 static prims from a pre-built binary dump (unused at runtime).
// uc_orig: load_all_prims (fallen/Source/io.cpp)
SLONG load_all_prims(CBYTE* name);

// Load a single static prim (.prm file) by prim ID into the global prim database.
// Returns UC_TRUE on success. Short-circuits if already loaded.
// uc_orig: load_prim_object (fallen/Source/io.cpp)
SLONG load_prim_object(SLONG prim);

// Editor-mode: clear prim database and reload all 265 static + 255 animated prims.
// uc_orig: load_all_individual_prims (fallen/Source/io.cpp)
void load_all_individual_prims(void);

// Read a quoted object name string from a text file handle.
// uc_orig: read_object_name (fallen/Source/io.cpp)
void read_object_name(FILE* file_handle, CBYTE* dest_string);

#endif // ASSETS_LEVEL_LOADER_H
