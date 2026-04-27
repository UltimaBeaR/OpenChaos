#ifndef BUILDINGS_PRIM_H
#define BUILDINGS_PRIM_H

#include "engine/core/types.h"
#include "engine/core/vector.h" // SVector
#include "buildings/prim_types.h" // PrimObject, PrimFace3/4, PrimPoint, PrimNormal, PrimInfo, FACE_FLAG_*, PRIM_OBJ_*, etc.

struct Thing; // forward declaration for fn_anim_prim_normal / set_anim_prim_anim parameters

// ---- Pool management ----

// Remove a contiguous block of vertices from prim_points[] by shifting left.
// uc_orig: delete_prim_points_block (fallen/Source/Prim.cpp)
void delete_prim_points_block(SLONG start, SLONG count);

// uc_orig: delete_prim_faces3_block (fallen/Source/Prim.cpp)
void delete_prim_faces3_block(SLONG start, SLONG count);

// uc_orig: delete_prim_faces4_block (fallen/Source/Prim.cpp)
void delete_prim_faces4_block(SLONG start, SLONG count);

// Patch all face point references after a block deletion.
// uc_orig: fix_faces_for_del_points (fallen/Source/Prim.cpp)
void fix_faces_for_del_points(SLONG start, SLONG count);

// uc_orig: fix_objects_for_del_points (fallen/Source/Prim.cpp)
void fix_objects_for_del_points(SLONG start, SLONG count);

// uc_orig: fix_objects_for_del_faces3 (fallen/Source/Prim.cpp)
void fix_objects_for_del_faces3(SLONG start, SLONG count);

// uc_orig: fix_objects_for_del_faces4 (fallen/Source/Prim.cpp)
void fix_objects_for_del_faces4(SLONG start, SLONG count);

// Remove unreferenced vertices and faces from the pools. Editor GC pass.
// uc_orig: compress_prims (fallen/Source/Prim.cpp)
void compress_prims(void);

// Reset the prim database for level load.
// uc_orig: clear_prims (fallen/Source/Prim.cpp)
void clear_prims(void);

// ---- Smooth lighting ----

// uc_orig: sum_shared_brightness_prim (fallen/Source/Prim.cpp)
SLONG sum_shared_brightness_prim(SWORD shared_point, struct PrimObject* p_obj);

// uc_orig: set_shared_brightness_prim (fallen/Source/Prim.cpp)
void set_shared_brightness_prim(SWORD shared_point, SWORD bright, struct PrimObject* p_obj);

// Average brightness across all shared vertices in a prim.
// uc_orig: smooth_a_prim (fallen/Source/Prim.cpp)
void smooth_a_prim(SLONG prim);

// ---- Editor helpers ----

// Duplicate a prim's mesh data to the top of the pool (editor scratch copy).
// uc_orig: copy_prim_to_end (fallen/Source/Prim.cpp)
SLONG copy_prim_to_end(UWORD prim, UWORD direct, SWORD thing);

// uc_orig: delete_prim_points (fallen/Source/Prim.cpp)
void delete_prim_points(SLONG start, SLONG end);

// uc_orig: delete_prim_faces3 (fallen/Source/Prim.cpp)
void delete_prim_faces3(SLONG start, SLONG end);

// uc_orig: delete_prim_faces4 (fallen/Source/Prim.cpp)
void delete_prim_faces4(SLONG start, SLONG end);

// uc_orig: delete_prim_objects (fallen/Source/Prim.cpp)
void delete_prim_objects(SLONG start, SLONG end);

// uc_orig: delete_last_prim (fallen/Source/Prim.cpp)
void delete_last_prim(void);

// uc_orig: delete_a_prim (fallen/Source/Prim.cpp)
void delete_a_prim(UWORD prim);

// ---- Normal calculation ----

// Compute the face normal for a triangle (face < 0) or quad (face >= 0).
// Result normalised to length 256. Output written to *p_normal.
// uc_orig: calc_normal (fallen/Source/Prim.cpp)
void calc_normal(SWORD face, struct SVector* p_normal);

// Unnormalised cross product of face edges (sign test only, no length).
// uc_orig: quick_normal (fallen/Source/Prim.cpp)
void quick_normal(SWORD face, SLONG* nx, SLONG* ny, SLONG* nz);

// ---- Lighting ----

// Bake a directional light into the face brightness values of a prim.
// One-shot pre-process, not per-frame.
// uc_orig: apply_ambient_light_to_object (fallen/Source/Prim.cpp)
UWORD apply_ambient_light_to_object(UWORD object, SLONG lnx, SLONG lny, SLONG lnz, UWORD intense);

// ---- Bounding info ----

// Post-load pass: compute runtime bounding box, radius, and flags for each prim.
// Calls VEH_init_vehinfo() at the end to set up vehicle crumple zones.
// uc_orig: calc_prim_info (fallen/Source/Prim.cpp)
void calc_prim_info(void);

// Compute per-vertex normals for all loaded prims (for env-mapped lighting).
// uc_orig: calc_prim_normals (fallen/Source/Prim.cpp)
void calc_prim_normals(void);

// Return pointer to prim_info[prim]. Asserts prim is in [1..255].
// uc_orig: get_prim_info (fallen/Source/Prim.cpp)
PrimInfo* get_prim_info(SLONG prim);

// ---- Slide edges ----

// Flag which edges of roof walkable faces are grabbable by the player.
// uc_orig: calc_slide_edges_roof (fallen/Source/Prim.cpp)
void calc_slide_edges_roof(void);

// Flag which edges of prim walkable quad faces are grabbable.
// uc_orig: calc_slide_edges (fallen/Source/Prim.cpp)
void calc_slide_edges(void);

// ---- Collision ----

// Convert a prim-local vertex to world position using the prim's YPR rotation.
// Pass point == -1 to pick a random vertex.
// uc_orig: get_rotated_point_world_pos (fallen/Source/Prim.cpp)
void get_rotated_point_world_pos(
    SLONG point,
    SLONG prim,
    SLONG prim_x, SLONG prim_y, SLONG prim_z,
    SLONG prim_yaw, SLONG prim_pitch, SLONG prim_roll,
    SLONG* px, SLONG* py, SLONG* pz);

// Slide movement vector around the prim's bounding box. Returns UC_TRUE if collision.
// uc_orig: slide_along_prim (fallen/Source/Prim.cpp)
SLONG slide_along_prim(
    SLONG prim,
    SLONG prim_x, SLONG prim_y, SLONG prim_z, SLONG prim_yaw,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG radius, SLONG shrink, SLONG dont_slide);

// Return the collision model type (PRIM_COLLIDE_*) for a prim.
// uc_orig: prim_get_collision_model (fallen/Source/Prim.cpp)
UBYTE prim_get_collision_model(SLONG prim);

// Return the shadow type for a prim.
// uc_orig: prim_get_shadow_type (fallen/Source/Prim.cpp)
UBYTE prim_get_shadow_type(SLONG prim);

// Returns UC_TRUE if a fence lies along the given orthogonal world-space line.
// uc_orig: does_fence_lie_along_line (fallen/Source/Prim.cpp)
SLONG does_fence_lie_along_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// Clear FACE_FLAG_WMOVE from all prim faces (called before re-building the WMOVE list).
// uc_orig: clear_all_wmove_flags (fallen/Source/Prim.cpp)
void clear_all_wmove_flags(void);

// ---- Animated prims ----

// Per-frame update for an animated world prim Thing.
// uc_orig: fn_anim_prim_normal (fallen/Source/Prim.cpp)
void fn_anim_prim_normal(Thing* p_thing);

// Spawn an animated world object Thing at the given world position.
// uc_orig: create_anim_prim (fallen/Source/Prim.cpp)
void create_anim_prim(SLONG x, SLONG y, SLONG z, SLONG prim, SLONG yaw);

// Restart a specific animation on an anim-prim Thing.
// uc_orig: set_anim_prim_anim (fallen/Source/Prim.cpp)
void set_anim_prim_anim(SLONG anim_prim, SLONG anim);

// Map anim-prim index to its behaviour class (ANIM_PRIM_TYPE_*).
// uc_orig: get_anim_prim_type (fallen/Source/Prim.cpp)
SLONG get_anim_prim_type(SLONG anim_prim);

// Find the nearest anim-prim of the given type(s) within range.
// type_bit_field: bitmask of (1 << ANIM_PRIM_TYPE_*) flags to include.
// Returns THING_INDEX of best match, or NULL if none found.
// uc_orig: find_anim_prim (fallen/Source/Prim.cpp)
SLONG find_anim_prim(SLONG x, SLONG y, SLONG z, SLONG range, ULONG type_bit_field);

// Player-triggered toggle for a switch-type anim prim.
// Ignored if the prim is currently animating (debounce).
// uc_orig: toggle_anim_prim_switch_state (fallen/Source/Prim.cpp)
void toggle_anim_prim_switch_state(SLONG anim_prim_thing_index);

// Expand an (min/max) bounding box to contain all vertices of an animated prim pose.
// uc_orig: expand_anim_prim_bbox (fallen/Source/Prim.cpp)
void expand_anim_prim_bbox(
    SLONG prim,
    struct GameKeyFrameElement* anim_info,
    SLONG* min_x, SLONG* min_y, SLONG* min_z,
    SLONG* max_x, SLONG* max_y, SLONG* max_z);

// Compute bounding boxes for all loaded anim-prims from their initial pose.
// uc_orig: find_anim_prim_bboxes (fallen/Source/Prim.cpp)
void find_anim_prim_bboxes(void);

// Mark all prim_objects slots as unloaded (zeroes them).
// uc_orig: mark_prim_objects_as_unloaded (fallen/Source/Prim.cpp)
void mark_prim_objects_as_unloaded(void);

// Shift all vertices of a prim by (dx, dy, dz) in local prim space.
// uc_orig: re_center_prim (fallen/Source/Prim.cpp)
void re_center_prim(SLONG prim, SLONG dx, SLONG dy, SLONG dz);

#endif // BUILDINGS_PRIM_H
