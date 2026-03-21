#ifndef WORLD_ENVIRONMENT_ID_H
#define WORLD_ENVIRONMENT_ID_H

#include "core/types.h"

// Interior design module: generates floorplans (rooms, furniture, stairs) for buildings.
// Given the outline of a building storey (walls), produces a layout of rooms and stair positions.
// Also handles collision for inside walls and floor drawing data.

// uc_orig: ID_FLOOR_SIZE (fallen/Headers/id.h)
#define ID_FLOOR_SIZE 256
// uc_orig: ID_PLAN_SIZE (fallen/Headers/id.h)
#define ID_PLAN_SIZE 32

// uc_orig: ID_clear_floorplan (fallen/Headers/id.h)
void ID_clear_floorplan(void);
// uc_orig: ID_set_outline (fallen/Headers/id.h)
void ID_set_outline(
    SLONG x1, SLONG z1, SLONG x2, SLONG z2,
    SLONG id,
    SLONG num_blocks);

// uc_orig: ID_BLOCK_TYPE_WALL (fallen/Headers/id.h)
#define ID_BLOCK_TYPE_WALL 1
// uc_orig: ID_BLOCK_TYPE_WINDOW (fallen/Headers/id.h)
#define ID_BLOCK_TYPE_WINDOW 2
// uc_orig: ID_BLOCK_TYPE_DOOR (fallen/Headers/id.h)
#define ID_BLOCK_TYPE_DOOR 3

// uc_orig: ID_set_get_type_func (fallen/Headers/id.h)
void ID_set_get_type_func(SLONG (*get_type)(SLONG id, SLONG block));

// uc_orig: ID_STAIR_TYPE_BOTTOM (fallen/Headers/id.h)
#define ID_STAIR_TYPE_BOTTOM 1
// uc_orig: ID_STAIR_TYPE_MIDDLE (fallen/Headers/id.h)
#define ID_STAIR_TYPE_MIDDLE 2
// uc_orig: ID_STAIR_TYPE_TOP (fallen/Headers/id.h)
#define ID_STAIR_TYPE_TOP 3

// One staircase returned by STAIR_get() and ID_generate_floorplan().
// Occupies two adjacent grid tiles (x1,z1) and (x2,z2).
// uc_orig: ID_Stair (fallen/Headers/id.h)
typedef struct
{
    UBYTE type;         // ID_STAIR_TYPE_BOTTOM/_MIDDLE/_TOP
    UBYTE id;
    UWORD shit;
    SLONG handle_up;    // Floor handle above (if going up)
    SLONG handle_down;  // Floor handle below (if going down)
    UBYTE x1;
    UBYTE z1;
    UBYTE x2;
    UBYTE z2;
} ID_Stair;

// uc_orig: ID_STOREY_TYPE_HOUSE_GROUND (fallen/Headers/id.h)
#define ID_STOREY_TYPE_HOUSE_GROUND 1
// uc_orig: ID_STOREY_TYPE_HOUSE_UPPER (fallen/Headers/id.h)
#define ID_STOREY_TYPE_HOUSE_UPPER 2
// uc_orig: ID_STOREY_TYPE_OFFICE_GROUND (fallen/Headers/id.h)
#define ID_STOREY_TYPE_OFFICE_GROUND 3
// uc_orig: ID_STOREY_TYPE_OFFICE_UPPER (fallen/Headers/id.h)
#define ID_STOREY_TYPE_OFFICE_UPPER 4
// uc_orig: ID_STOREY_TYPE_WAREHOUSE (fallen/Headers/id.h)
#define ID_STOREY_TYPE_WAREHOUSE 5
// uc_orig: ID_STOREY_TYPE_APARTEMENT_GROUND (fallen/Headers/id.h)
#define ID_STOREY_TYPE_APARTEMENT_GROUND 6
// uc_orig: ID_STOREY_TYPE_APARTEMENT_UPPER (fallen/Headers/id.h)
#define ID_STOREY_TYPE_APARTEMENT_UPPER 7

// Generates the floorplan. Returns seed used (negative = error).
// uc_orig: ID_generate_floorplan (fallen/Headers/id.h)
SLONG ID_generate_floorplan(
    SLONG type,
    ID_Stair stair[],
    SLONG num_stairs,
    UWORD seed,
    UBYTE find_good_layout,
    UBYTE furnished);

// uc_orig: ID_wall_colvects_insert (fallen/Headers/id.h)
void ID_wall_colvects_insert(void);
// uc_orig: ID_wall_colvects_remove (fallen/Headers/id.h)
void ID_wall_colvects_remove(void);

// uc_orig: ID_remove_inside_things (fallen/Headers/id.h)
void ID_remove_inside_things(void);

// uc_orig: ID_get_mapsquare_room (fallen/Headers/id.h)
UBYTE ID_get_mapsquare_room(SLONG x, SLONG z);

// uc_orig: ID_get_room_camera (fallen/Headers/id.h)
void ID_get_room_camera(UBYTE room, SLONG* x, SLONG* y, SLONG* z);

// uc_orig: ID_change_floor (fallen/Headers/id.h)
SLONG ID_change_floor(
    SLONG x,
    SLONG z,
    SLONG* new_x,
    SLONG* new_z,
    SLONG* handle);

// uc_orig: ID_get_floorplan_bounding_box (fallen/Headers/id.h)
void ID_get_floorplan_bounding_box(
    SLONG* x1,
    SLONG* z1,
    SLONG* x2,
    SLONG* z2);

// uc_orig: ID_am_i_completely_outside (fallen/Headers/id.h)
SLONG ID_am_i_completely_outside(SLONG x, SLONG z);
// uc_orig: ID_should_i_draw_mapsquare (fallen/Headers/id.h)
SLONG ID_should_i_draw_mapsquare(SLONG x, SLONG z);
// uc_orig: ID_get_mapsquare_texture (fallen/Headers/id.h)
SLONG ID_get_mapsquare_texture(SLONG x, SLONG z,
    float* u0, float* v0,
    float* u1, float* v1,
    float* u2, float* v2,
    float* u3, float* v3);

// uc_orig: ID_this_is_where_i_am (fallen/Headers/id.h)
void ID_this_is_where_i_am(SLONG x, SLONG z);
// uc_orig: ID_should_i_draw (fallen/Headers/id.h)
SLONG ID_should_i_draw(SLONG x, SLONG z);

// uc_orig: ID_get_first_face (fallen/Headers/id.h)
SLONG ID_get_first_face(SLONG x, SLONG z);
// uc_orig: ID_is_face_a_quad (fallen/Headers/id.h)
SLONG ID_is_face_a_quad(SLONG face);
// uc_orig: ID_get_next_face (fallen/Headers/id.h)
SLONG ID_get_next_face(SLONG face);

// uc_orig: ID_get_face_texture (fallen/Headers/id.h)
SLONG ID_get_face_texture(SLONG face,
    float* u0, float* v0,
    float* u1, float* v1,
    float* u2, float* v2,
    float* u3, float* v3);

// Furniture descriptor: position, prim index, and yaw angle.
// uc_orig: ID_Finfo (fallen/Headers/id.h)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    UWORD prim;
    UWORD yaw;
} ID_Finfo;

// uc_orig: ID_get_num_furn (fallen/Headers/id.h)
SLONG ID_get_num_furn(void);
// uc_orig: ID_get_furn (fallen/Headers/id.h)
ID_Finfo* ID_get_furn(SLONG number);

// uc_orig: ID_clear_indices (fallen/Headers/id.h)
void ID_clear_indices(void);
// uc_orig: ID_is_point_a_mapsquare (fallen/Headers/id.h)
SLONG ID_is_point_a_mapsquare(SLONG face, SLONG point);
// uc_orig: ID_get_point_mapsquare (fallen/Headers/id.h)
void ID_get_point_mapsquare(SLONG face, SLONG point, SLONG* x, SLONG* z);
// uc_orig: ID_get_point_position (fallen/Headers/id.h)
void ID_get_point_position(SLONG face, SLONG point, SLONG* x, SLONG* y, SLONG* z);
// uc_orig: ID_get_point_index (fallen/Headers/id.h)
UWORD ID_get_point_index(SLONG face, SLONG point);
// uc_orig: ID_set_point_index (fallen/Headers/id.h)
void ID_set_point_index(SLONG face, SLONG point, UWORD index);

// uc_orig: ID_collide_3d (fallen/Headers/id.h)
SLONG ID_collide_3d(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2);

// uc_orig: ID_calc_height_at (fallen/Headers/id.h)
SLONG ID_calc_height_at(SLONG x, SLONG z);

// uc_orig: ID_collide_2d (fallen/Headers/id.h)
SLONG ID_collide_2d(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG Z2,
    SLONG radius,
    SLONG* slide_x,
    SLONG* slide_z);

// Editor types.
// uc_orig: ID_Wallinfo (fallen/Headers/id.h)
typedef struct
{
    UBYTE door[4];
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;
} ID_Wallinfo;

// uc_orig: ID_Roominfo (fallen/Headers/id.h)
typedef struct
{
    SLONG x;
    SLONG z;
    CBYTE* what;
} ID_Roominfo;

// uc_orig: ID_Stairinfo (fallen/Headers/id.h)
typedef struct
{
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;
} ID_Stairinfo;

// uc_orig: ID_editor_start_get_rooms (fallen/Headers/id.h)
void ID_editor_start_get_rooms(void);
// uc_orig: ID_editor_start_get_walls (fallen/Headers/id.h)
void ID_editor_start_get_walls(void);
// uc_orig: ID_editor_start_get_stairs (fallen/Headers/id.h)
void ID_editor_start_get_stairs(void);

// uc_orig: ID_editor_get_room (fallen/Headers/id.h)
SLONG ID_editor_get_room(ID_Roominfo* ans);
// uc_orig: ID_editor_get_wall (fallen/Headers/id.h)
SLONG ID_editor_get_wall(ID_Wallinfo* ans);
// uc_orig: ID_editor_get_stair (fallen/Headers/id.h)
SLONG ID_editor_get_stair(ID_Stairinfo* ans);

#endif // WORLD_ENVIRONMENT_ID_H
