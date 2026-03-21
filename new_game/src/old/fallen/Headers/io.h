#ifndef FALLEN_HEADERS_IO_H
#define FALLEN_HEADERS_IO_H

// Redirects to migrated level_loader for functions moved to new/assets/.
#include "assets/level_loader.h"
#include "assets/level_loader_globals.h"

// Functions still in old/fallen/Source/io.cpp (not yet migrated).
extern SLONG load_a_multi_prim(CBYTE* name);
extern void load_palette(CBYTE* palette);
extern void load_key_frame_chunks(KeyFrameChunk* the_chunk, CBYTE* vue_name, float shrink = 1.0);
extern SLONG save_anim_system(struct GameKeyFrameChunk* game_chunk, CBYTE* name);

#endif // FALLEN_HEADERS_IO_H
