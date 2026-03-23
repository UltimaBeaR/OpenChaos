#ifndef ASSETS_ANIM_LOADER_GLOBALS_H
#define ASSETS_ANIM_LOADER_GLOBALS_H

#include "core/types.h"

// Total number of keyframes accumulated during load_multi_vue().
// uc_orig: key_frame_count (fallen/Source/io.cpp)
extern SLONG key_frame_count;

// Next write position into the_elements[] during load_multi_vue().
// uc_orig: current_element (fallen/Source/io.cpp)
extern SLONG current_element;

// Bounding-box centre of the last loaded anim mesh (used for pivoting in load_key_frame_chunks()).
// uc_orig: x_centre (fallen/Source/io.cpp)
extern SLONG x_centre;
// uc_orig: y_centre (fallen/Source/io.cpp)
extern SLONG y_centre;
// uc_orig: z_centre (fallen/Source/io.cpp)
extern SLONG z_centre;

#endif // ASSETS_ANIM_LOADER_GLOBALS_H
