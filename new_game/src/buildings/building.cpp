// MAP_WIDTH, MAP_HEIGHT, FLOOR_HEIGHT_SHIFT, CLASS_BUILDING, DT_BUILDING, DT_NONE,
// TO_THING, alloc_primary_thing, add_thing_to_map, and the Thing struct.
#include "game/game_types.h"
#include "map/map_constants.h"
// Needed for PAP_SIZE_LO, PAP_Lo type (for clear_map2 which zeros PAP_lo array)
#include "map/pap_globals.h"
#include "assets/formats/anim_globals.h"
// Needed for each_point[] (used in calc_building_normals)
#include "buildings/prim_globals.h"
// DAG violation (world should not depend on missions) — needs reclassification in a later pass.
#include "map/level_pools.h"
// Needed for load_all_individual_prims (called in create_city)
#include "assets/formats/level_loader.h"
#include "map/pap_globals.h"
#include "assets/formats/anim_globals.h"
#include "map/supermap.h"

#include "map/pap_globals.h"
#include "assets/formats/anim_globals.h"
#include "engine/physics/collide.h"
#include "engine/physics/collide_globals.h"
#include "engine/graphics/pipeline/aeng.h"
#include "buildings/prim_types.h"     // OR_SORT_LEVEL, FACE_FLAG_NON_PLANAR, FACE_FLAG_WALKABLE, etc.
#include "buildings/building_types.h" // STOREY_TYPE_*, FACET_FLAG_*, FBuilding etc.
#include "buildings/prim.h"           // calc_normal, get_rotated_point_world_pos, etc.
// Needed for OUTLINE_Outline, OUTLINE_create, OUTLINE_add_line, OUTLINE_overlap
#include "buildings/outline.h"
// Needed for calc_ladder_ends (used by calc_ladder_pos in this file)
#include "buildings/build2.h"

// Needed for MiniTextureBits (packed texture descriptor used in building face data).
#include "assets/texture.h"
#include "buildings/building.h"
#include "buildings/building_globals.h"

#pragma warning(disable : 4244)

// Forward declarations for functions defined later in this file and in Building.cpp (remaining chunks).
// uc_orig: insert_point (fallen/Source/Building.cpp)
static void insert_point(SLONG z, SLONG x, SWORD type);

// uc_orig: fn_building_normal (fallen/Source/Building.cpp)
void fn_building_normal(Thing* b_thing);

// uc_orig: get_map_height (fallen/Source/Building.cpp) — forward for use in build_bottom_edge_list
SLONG get_map_height(SLONG x, SLONG z);

// uc_orig: insert_collision_vect (fallen/Source/Building.cpp)
// Stub: in this pre-release codebase does nothing — the full game version registers a wall/ramp/ladder
// collision vector into the physics system. prim_type = STOREY_TYPE_* constant.
SLONG insert_collision_vect(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2,
                             UBYTE prim_type, UBYTE prim_extra, SWORD face)
{
    return 0;
}

// uc_orig: add_page_countxy (fallen/Source/Building.cpp)
// Increments the texture page usage counter for the given (tx,ty) within a page.
// Tracks how many distinct pages are used in each of the two halves (below/above page 256).
void add_page_countxy(SLONG tx, SLONG ty, SLONG page)
{
    page = page * 64 + tx + ty * 8;
    page_count[page]++;
    if (page_count[page] == 1) {
        if (page < 4 * 64)
            diff_page_count1++;
        else
            diff_page_count2++;
    }
}

// uc_orig: add_bound_box (fallen/Source/Building.cpp)
// Appends a roof bounding box entry and returns its index (1-based). Returns 0 if pool is full.
SLONG add_bound_box(UBYTE minx, UBYTE maxx, UBYTE minz, UBYTE maxz, SWORD y)
{
    if (next_roof_bound > MAX_ROOF_BOUND - 2)
        return (0);
    roof_bounds[next_roof_bound].MinX = minx;
    roof_bounds[next_roof_bound].MaxX = maxx;
    roof_bounds[next_roof_bound].MinZ = minz;
    roof_bounds[next_roof_bound].MaxZ = maxz;
    roof_bounds[next_roof_bound].Y = y;
    next_roof_bound++;
    return (next_roof_bound - 1);
}

// uc_orig: get_map_walkable (fallen/Source/Building.cpp)
// Returns the walkability value for the tile at (x,z). In DX mode reads MAP2.Walkable.
SLONG get_map_walkable(SLONG x, SLONG z)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        return (MAP2(x, z).Walkable);
        break;
    }
    return (0);
}

// uc_orig: set_map_walkable (fallen/Source/Building.cpp)
void set_map_walkable(SLONG x, SLONG z, SLONG walkable)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        MAP2(x, z).Walkable = walkable;
        break;
    }
}

// uc_orig: get_map_texture (fallen/Source/Building.cpp)
SLONG get_map_texture(SLONG x, SLONG z)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        return PAP_2HI(x, z).Texture;
        break;
    }
    return (0);
}

// uc_orig: set_map_texture (fallen/Source/Building.cpp)
void set_map_texture(SLONG x, SLONG z, SLONG texture)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        PAP_2HI(x, z).Texture = texture;
        break;
    }
}

// uc_orig: get_map_height (fallen/Source/Building.cpp)
// Returns the height (Alt) of the map tile at (x,z) in PAP_2HI units.
SLONG get_map_height(SLONG x, SLONG z)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        return PAP_2HI(x, z).Alt;
        break;
    }
    return (0);
}

// uc_orig: get_roof_height (fallen/Source/Building.cpp)
// Returns 0 in DX mode (roof height not stored separately in PAP in this codebase).
SLONG get_roof_height(SLONG x, SLONG z)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        return 0; // PAP_2HI(x,z).Alt;
        break;
    }
    return (0);
}

// uc_orig: set_map_flag (fallen/Source/Building.cpp)
SLONG set_map_flag(SLONG x, SLONG z, SLONG flag)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        PAP_2HI(x, z).Flags |= flag;
        break;
    }
    return (0);
}

// uc_orig: mask_map_flag (fallen/Source/Building.cpp)
SLONG mask_map_flag(SLONG x, SLONG z, SLONG flag)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        PAP_2HI(x, z).Flags &= ~flag;
        break;
    }
    return (0);
}

// uc_orig: get_map_flags (fallen/Source/Building.cpp)
SLONG get_map_flags(SLONG x, SLONG z)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        return PAP_2HI(x, z).Flags;
        break;
    }
    return (0);
}

// uc_orig: set_map_height (fallen/Source/Building.cpp)
void set_map_height(SLONG x, SLONG z, SLONG y)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        PAP_2HI(x, z).Alt = y;
        break;
    }
}

// uc_orig: in_map_range (fallen/Source/Building.cpp)
SLONG in_map_range(SLONG x, SLONG z)
{
    if (x < 0 || z < 0)
        return (0);
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        if (x > MAP_WIDTH || z > MAP_HEIGHT)
            return (0);
        else
            return (1);
        break;
    }
    return (0);
}

// uc_orig: place_thing_on_map (fallen/Source/Building.cpp)
// In DX mode adds the existing Thing to the map grid.
void place_thing_on_map(SLONG x, SLONG z, SLONG thing)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX:
        add_thing_to_map(TO_THING(thing));
        break;
    }
}

// uc_orig: set_vect_floor_height (fallen/Source/Building.cpp)
// Walks along the vector from (x1,z1) to (x2,z2) and sets the map height at each tile to y.
void set_vect_floor_height(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG y)
{
    SLONG step_x, step_z;
    SLONG len, count;

    step_x = x2 - x1;
    step_z = z2 - z1;
    len = Root(step_x * step_x + step_z * step_z);

    step_x = (step_x << 16) / len;
    step_z = (step_z << 16) / len;

    count = len >> 8;

    x1 <<= 8;
    z1 <<= 8;
    set_map_height(x1 >> 16, z1 >> 16, y);
    while (count) {
        x1 += step_x;
        z1 += step_z;
        set_map_height(x1 >> 16, z1 >> 16, y);
        count--;
    }
}

// build_seed is defined in building_globals.cpp (per globals rule).

// uc_orig: build_rand (fallen/Source/Building.cpp)
// Simple deterministic random number generator for procedural building geometry.
// Multiply-add LCG; returns the high 16 bits of the updated seed.
SLONG build_rand(void)
{
    build_seed = (build_seed * 12345678) + 12345678;
    return (build_seed >> 16);
}

// uc_orig: set_build_seed (fallen/Source/Building.cpp)
void set_build_seed(SLONG seed)
{
    build_seed = seed;
}

// uc_orig: add_walk_face_to_map (fallen/Source/Building.cpp)
// Links a renderable face index into the walkable-face linked list for the map tile at (x,z).
// Skips duplicate faces (same face already in the list for this tile).
void add_walk_face_to_map(SWORD face, SLONG x, SLONG z)
{
    if (next_walk_link >= (MAX_WALK_POOL - 4)) {
        ASSERT(0);
        return;
    }

    // If this face is already in the walkable list for this square, don't add it again.
    {
        SLONG index = MAP[MAP_INDEX(x, z)].Walkable;
        while (index) {
            if (walk_links[index].Face == face) {
                return;
            }
            index = walk_links[index].Next;
        }
    }

    walk_links[next_walk_link].Face = face;
    walk_links[next_walk_link].Next = get_map_walkable(x, z);
    set_map_walkable(x, z, next_walk_link);
    next_walk_link++;
}

// uc_orig: scan_walk_triangle (fallen/Source/Building.cpp)
// Rasterizes a 3D triangle into walkable map tiles using parametric sampling.
// For each sample along (s,t) barycentric coordinates, maps to world (px,pz) and registers.
void scan_walk_triangle(SLONG x0, SLONG y0, SLONG z0,
                        SLONG x1, SLONG y1, SLONG z1,
                        SLONG x2, SLONG y2, SLONG z2, SLONG face)
{
    SLONG px, pz;
    SLONG face_x, face_z;
    SLONG s, t, step_s, step_t;
    SLONG vx, vy, vz, wx, wy, wz;
    SLONG prev_x, prev_z;
    SLONG len;

    face_x = x0;
    face_z = z0;

    vx = x1 - x0;
    vy = y1 - y0; // vector from point 0 to point 1
    vz = z1 - z0;

    wx = x2 - x0; // vector from point 1 to point 2
    wy = y2 - y0;
    wz = z2 - z0;

    len = (Root(vx * vx + vy * vy + vz * vz) >> 7);
    if (len < 2)
        len = 2;
    step_s = (1 << 7) / len;
    len = (Root(wx * wx + wy * wy + wz * wz) >> 7);
    if (len < 2)
        len = 2;
    step_t = (1 << 7) / len;

    if (step_s == 0)
        step_s = 256;
    if (step_t == 0)
        step_t = 256;

    prev_x = -1;
    prev_z = -1;
    for (s = 5; s < (255); s += step_s)
        for (t = 5; t < (255) && ((s + t) < (256)); t += step_t) {
            px = face_x + ((s * vx) >> 8) + ((t * wx) >> 8);
            pz = face_z + ((s * vz) >> 8) + ((t * wz) >> 8);
            if ((px >> 8) != prev_x || (pz >> 8) != prev_z) {
                if (WITHIN(px >> ELE_SHIFT, 0, MAP_WIDTH - 1) && WITHIN(pz >> ELE_SHIFT, 0, MAP_HEIGHT - 1)) {
                    add_walk_face_to_map(face, px >> 8, pz >> 8);
                }
                prev_x = px >> 8;
                prev_z = pz >> 8;
            }
        }
}

// uc_orig: add_quad_to_walkable_list (fallen/Source/Building.cpp)
// Splits the quad into two triangles and rasterizes both into the walkable tile map.
// Also sets FACE_FLAG_WALKABLE on the face.
void add_quad_to_walkable_list(SWORD face)
{
    SLONG x[4], y[4], z[4];
    SLONG c0, p0;
    struct PrimFace4* p_f4;
    p_f4 = &prim_faces4[face];

    for (c0 = 0; c0 < 4; c0++) {
        p0 = p_f4->Points[c0];
        x[c0] = prim_points[p0].X;
        y[c0] = prim_points[p0].Y;
        z[c0] = prim_points[p0].Z;
    }
    scan_walk_triangle(x[0], y[0], z[0], x[1], y[1], z[1], x[2], y[2], z[2], face);
    scan_walk_triangle(x[1], y[1], z[1], x[3], y[3], z[3], x[2], y[2], z[2], face);

    // Mark the face as walkable.
    prim_faces4[face].FaceFlags |= FACE_FLAG_WALKABLE;
}

// uc_orig: add_tri_to_walkable_list (fallen/Source/Building.cpp)
void add_tri_to_walkable_list(SWORD face)
{
    // Stub — walkable list generation was cut.
}

// uc_orig: place_building_at (fallen/Source/Building.cpp)
// Creates a game Thing (CLASS_BUILDING) for the building and places it at (x,y,z).
// In DX mode: allocates a primary Thing, sets DrawType=DT_BUILDING, links to building_list[].
SLONG place_building_at(UWORD building, UWORD prim, SLONG x, SLONG y, SLONG z)
{
    switch (build_mode) {
    case BUILD_MODE_EDITOR:
        break;
    case BUILD_MODE_DX: {
        Thing* p_thing;
        SLONG new_thing;

        new_thing = alloc_primary_thing(CLASS_BUILDING);
        if (new_thing) {
            p_thing = TO_THING(new_thing);

            p_thing->WorldPos.X = x << 8;
            p_thing->WorldPos.Y = y << 8;
            p_thing->WorldPos.Z = z << 8;
            p_thing->StateFn = fn_building_normal;
            p_thing->DrawType = DT_NONE;
            p_thing->Flags = 0;

            p_thing->Index = prim;
            p_thing->DrawType = DT_BUILDING;
            p_thing->Class = CLASS_BUILDING;
            MSG_add(" create building  prim %d at %d %d %d building %d \n", prim, x, y, z, building);
            add_thing_to_map(p_thing);

            // Link the building to the thing.
            building_list[building].ThingIndex = new_thing;

            return new_thing;
        }
    } break;
    }
    return (0);
}

// uc_orig: add_point (fallen/Source/Building.cpp)
// Appends a vertex to both the integer prim_points[] and the float AENG_dx_prim_points[] arrays.
void add_point(SLONG x, SLONG y, SLONG z)
{
    AENG_dx_prim_points[next_prim_point].X = (float)x;
    AENG_dx_prim_points[next_prim_point].Y = (float)y;
    AENG_dx_prim_points[next_prim_point].Z = (float)z;

    prim_points[next_prim_point].X = x;
    prim_points[next_prim_point].Y = y;
    prim_points[next_prim_point++].Z = z;
}

// uc_orig: build_row_wall_points_at_y (fallen/Source/Building.cpp)
// Generates points along a wall segment at height y.
// If FLAG_WALL_AUTO_WINDOWS: alternates solid sections and window openings.
// Returns index of the first generated point.
SLONG build_row_wall_points_at_y(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall)
{
    SLONG wcount, wwidth, wallwidth, dx, dz, dist;
    SLONG start_point;

    start_point = next_prim_point;
    wwidth = BLOCK_SIZE;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));
    if (dist == 0)
        return (0);

    if (wall_list[wall].WallFlags & FLAG_WALL_AUTO_WINDOWS) {
        wcount = dist / (BLOCK_SIZE * 4);
        wwidth = dist / (wcount * 2 + 1);
        WindowCount = wcount;
    } else {
        wcount = 0;
        wwidth = BLOCK_SIZE;
    }

    dx = (x2 - x1);
    dz = (z2 - z1);

    if (wcount < 0)
        return (0);

    wallwidth = (dist - (wcount * wwidth)) / (wcount + 1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    add_point(x1, y, z1);

    while (wcount) {
        x1 = x1 + ((dx * wallwidth) >> 10);
        z1 = z1 + ((dz * wallwidth) >> 10);
        add_point(x1, y, z1);

        x1 = x1 + ((dx * wwidth) >> 10);
        z1 = z1 + ((dz * wwidth) >> 10);
        add_point(x1, y, z1);

        wcount--;
    }
    x1 = x1 + ((dx * wallwidth) >> 10);
    z1 = z1 + ((dz * wallwidth) >> 10);
    add_point(x1, y, z1);

    return (start_point);
}

// uc_orig: build_row_wall_points_at_floor_alt (fallen/Source/Building.cpp)
// Like build_row_wall_points_at_y but the y coordinate is read from the terrain height (PAP_calc_height_at).
SLONG build_row_wall_points_at_floor_alt(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall)
{
    SLONG wcount, wwidth, wallwidth, dx, dz, dist;
    SLONG start_point;

    start_point = next_prim_point;
    wwidth = BLOCK_SIZE;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));
    if (dist == 0)
        return (0);

    if (wall_list[wall].WallFlags & FLAG_WALL_AUTO_WINDOWS) {
        wcount = dist / (BLOCK_SIZE * 4);
        wwidth = dist / (wcount * 2 + 1);
        WindowCount = wcount;
    } else {
        wcount = 0;
        wwidth = BLOCK_SIZE;
    }

    dx = (x2 - x1);
    dz = (z2 - z1);

    if (wcount < 0)
        return (0);

    wallwidth = (dist - (wcount * wwidth)) / (wcount + 1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    y = PAP_calc_height_at(x1, z1);
    add_point(x1, y, z1);

    while (wcount) {
        x1 = x1 + ((dx * wallwidth) >> 10);
        z1 = z1 + ((dz * wallwidth) >> 10);
        y = PAP_calc_height_at(x1, z1);
        add_point(x1, y, z1);

        x1 = x1 + ((dx * wwidth) >> 10);
        z1 = z1 + ((dz * wwidth) >> 10);
        y = PAP_calc_height_at(x1, z1);
        add_point(x1, y, z1);

        wcount--;
    }
    x1 = x1 + ((dx * wallwidth) >> 10);
    z1 = z1 + ((dz * wallwidth) >> 10);
    y = PAP_calc_height_at(x1, z1);
    add_point(x1, y, z1);

    return (start_point);
}

// uc_orig: build_row_wall_only_points_at_y (fallen/Source/Building.cpp)
// Generates evenly-spaced wall points without window splits at fixed height y.
SLONG build_row_wall_only_points_at_y(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall)
{
    SLONG wcount, wwidth, dx, dz, dist;
    SLONG start_point;

    start_point = next_prim_point;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));
    if (dist == 0)
        return (0);

    wcount = (dist / (BLOCK_SIZE * 4));
    if (wcount == 0)
        wcount = 1;
    wwidth = dist / (wcount);
    WindowCount = wcount;

    dx = (x2 - x1);
    dz = (z2 - z1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    add_point(x1, y, z1);
    wcount--;

    while (wcount) {
        x1 = x1 + ((dx * wwidth) >> 10);
        z1 = z1 + ((dz * wwidth) >> 10);
        add_point(x1, y, z1);
        wcount--;
    }
    add_point(x2, y, z2); // ensure the last point is exactly on the endpoint

    return (start_point);
}

// uc_orig: build_row_wall_only_points_at_floor_alt (fallen/Source/Building.cpp)
// Like build_row_wall_only_points_at_y but y is derived from terrain height + dy offset.
SLONG build_row_wall_only_points_at_floor_alt(SLONG dy, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall)
{
    SLONG wcount, wwidth, dx, dz, dist;
    SLONG start_point;
    SLONG y;

    start_point = next_prim_point;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));
    if (dist == 0)
        return (0);

    wcount = (dist / (BLOCK_SIZE * 4));
    if (wcount == 0)
        wcount = 1;
    wwidth = dist / (wcount);
    WindowCount = wcount;

    dx = (x2 - x1);
    dz = (z2 - z1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    y = PAP_calc_height_at(x1, z1);
    y += dy;
    add_point(x1, y, z1);
    wcount--;

    while (wcount) {
        x1 = x1 + ((dx * wwidth) >> 10);
        z1 = z1 + ((dz * wwidth) >> 10);
        y = PAP_calc_height_at(x1, z1);
        y += dy;
        add_point(x1, y, z1);
        wcount--;
    }
    y = PAP_calc_height_at(x2, z2);
    y += dy;
    add_point(x2, y, z2); // ensure last point is exactly on endpoint

    return (start_point);
}

// uc_orig: build_row_window_depth_points_at_y (fallen/Source/Building.cpp)
// Generates inset points along a wall for window depth geometry (pushes points inward by 20 units).
SLONG build_row_window_depth_points_at_y(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall)
{
    SLONG wcount, wwidth, wallwidth, dx, dz, dist;
    SLONG pdx, pdz;
    SLONG start_point;

    start_point = next_prim_point;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));
    if (dist == 0)
        return (0);

    if (wall_list[wall].WallFlags & FLAG_WALL_AUTO_WINDOWS) {
        wcount = dist / (BLOCK_SIZE * 4);
        wwidth = dist / (wcount * 2 + 1);
        WindowCount = wcount;
    } else {
        wcount = 0;
        wwidth = BLOCK_SIZE;
    }

    dx = (x2 - x1);
    dz = (z2 - z1);

    if (wcount < 0)
        return (0);

    wallwidth = (dist - (wcount * wwidth)) / (wcount + 1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    // Perpendicular inset (window depth) of 20 units inward
    pdx = -dz;
    pdz = dx;
    pdx = (pdx * 20) >> 10;
    pdz = (pdz * 20) >> 10;

    x1 += pdx;
    z1 += pdz;

    while (wcount) {
        x1 = x1 + ((dx * wallwidth) >> 10);
        z1 = z1 + ((dz * wallwidth) >> 10);
        add_point(x1, y, z1);

        x1 = x1 + ((dx * wwidth) >> 10);
        z1 = z1 + ((dz * wwidth) >> 10);
        add_point(x1, y, z1);

        wcount--;
    }
    x1 = x1 + ((dx * wallwidth) >> 10);
    z1 = z1 + ((dz * wallwidth) >> 10);

    return (start_point);
}

// All edge-list scanner globals (edge_heads_ptr, next_edge, edge_min_z, flag_blocks,
// flag_blocks2, cut_blocks, global_y) are defined in building_globals.cpp.

// uc_orig: insert_point (fallen/Source/Building.cpp)
// Inserts an X-crossing at scanline row z into the sorted linked list (sorted by X ascending).
// Increments Count if an entry with the same X already exists.
static void insert_point(SLONG z, SLONG x, SWORD type)
{
    SLONG edge;

    if (x < block_min_x || x > block_max_x)
        return;

    edge_pool_ptr[next_edge].X = x;
    edge_pool_ptr[next_edge].Count = 1;
    edge_pool_ptr[next_edge].Type = type;
    ASSERT(z >= 0);
    ASSERT(z < MAX_BOUND_SIZE);
    ASSERT(x >= 0);
    ASSERT(x < MAP_WIDTH * 256);

    edge = edge_heads_ptr[z];
    if (edge) {
        while (edge) {
            if (edge_pool_ptr[edge].X > x) {
                SLONG prev;
                prev = edge_pool_ptr[edge].Prev;

                if (prev) {
                    // insert between current one and previous
                    edge_pool_ptr[prev].Next = next_edge;
                    edge_pool_ptr[edge].Prev = next_edge;
                    edge_pool_ptr[next_edge].Next = edge;
                    edge_pool_ptr[next_edge].Prev = prev;
                    next_edge++;
                } else {
                    // insert before current one (new head of list)
                    edge_heads_ptr[z] = next_edge;
                    edge_pool_ptr[edge].Prev = next_edge;
                    edge_pool_ptr[next_edge].Next = edge;
                    edge_pool_ptr[next_edge].Prev = 0;
                    next_edge++;
                }
                return;
            } else if (edge_pool_ptr[edge].X == x) {
                edge_pool_ptr[edge].Count++;
                return;
            }

            if (edge_pool_ptr[edge].Next == 0) {
                // append after current
                edge_pool_ptr[edge].Next = next_edge;
                edge_pool_ptr[next_edge].Next = 0;
                edge_pool_ptr[next_edge].Prev = edge;
                next_edge++;
                return;
            }
            edge = edge_pool_ptr[edge].Next;
        }
    } else {
        edge_heads_ptr[z] = next_edge;
        edge_pool_ptr[next_edge].Prev = 0;
        edge_pool_ptr[next_edge].Next = 0;
        next_edge++;
    }
}

// uc_orig: set_cut_blocks (fallen/Source/Building.cpp)
// Marks the top/bottom cut-block boundary for the tile at pixel x, scanline-block z.
void set_cut_blocks(SLONG x, SLONG z)
{
    cut_blocks[(x >> ELE_SHIFT) * 4 + ((z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP)] = x;
    cut_blocks[(x >> ELE_SHIFT) * 4 + ((z - 1) * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM)] = x;
}

// uc_orig: set_cut_blocks_z (fallen/Source/Building.cpp)
// Marks the left/right cut-block boundary for the tile at tile x, pixel z.
void set_cut_blocks_z(SLONG x, SLONG z)
{
    cut_blocks[(x) * 4 + (((z >> ELE_SHIFT) - 0) * MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT)] = z;
    cut_blocks[(x - 1) * 4 + (((z >> ELE_SHIFT) - 0) * MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT)] = z;
}

// uc_orig: scan_line_z (fallen/Source/Building.cpp)
// Rasterizes a line segment in Z-major order, recording cut-block Z boundaries.
void scan_line_z(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG flag)
{
    SLONG dx, dz, count;
    SLONG z;

    dz = z2 - z1;
    dx = x2 - x1;
    {
        x1 >>= ELE_SHIFT;
        x2 >>= ELE_SHIFT;
        dx = x2 - x1;

        z = z1 << 16;
        count = dx;

        if (count < 0) {
            dz = -(dz << 16) / count;
            x1--;
            z += dz;

            while (count) {
                if (dz) {
                    if ((z >> 16) & 0xff) {
                        set_cut_blocks_z(x1, z >> 16);
                    }
                }

                z += dz;
                x1--;
                count++;
            }
        }

        if (count > 0) {
            dz = (dz << 16) / count;

            while (count) {
                if (dz) {
                    if ((z >> 16) & 0xff) {
                        set_cut_blocks_z(x1, z >> 16);
                    }
                }
                z += dz;
                x1++;
                count--;
            }
        }
    }
}

// uc_orig: scan_line (fallen/Source/Building.cpp)
// Rasterizes a line segment in Z-major order, inserting X-crossing events into the edge list.
// Returns 1 if both dx and dz are non-zero (diagonal line), 0 otherwise (axis-aligned).
UBYTE scan_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG flag)
{
    SLONG dx, dz, count;
    SLONG x;
    SWORD type;

    if (z1 == z2)
        type = SIDEWAY_EDGE;
    else
        type = NORMAL_EDGE;

    dz = z2 - z1;
    dx = x2 - x1;
    {
        z1 >>= ELE_SHIFT;
        z2 >>= ELE_SHIFT;
        dz = z2 - z1;

        x = x1 << 16;
        count = dz;

        if (count < 0) {
            dx = -(dx << 16) / count;
            z1--;
            x += dx;

            while (count) {
                insert_point(z1, x >> 16, type);
                if (dx) {
                    if ((x >> 16) & 0xff) {
                        set_cut_blocks(x >> 16, z1);
                    }
                }
                x += dx;
                z1--;
                count++;
            }
        }

        if (count > 0) {
            dx = (dx << 16) / count;

            while (count) {
                insert_point(z1, x >> 16, type);
                if (dx) {
                    if ((x >> 16) & 0xff) {
                        set_cut_blocks(x >> 16, z1);
                    }
                }
                x += dx;
                z1++;
                count--;
            }
        }
    }
    /*
            else
            {
                    x1>>=ELE_SHIFT;
                    x2>>=ELE_SHIFT;
                    dx=x2-x1;

                    z=z1<<16;
                    count=dx;

                    if(count<0)
                    {
                            dz=-(dz<<16)/count;
                            while(count<0)
                            {
                                    insert_point((z>>16)>>ELE_SHIFT,x1<<ELE_SHIFT,type);
                                    z+=dz;
                                    x1--;
                                    count++;
                            }
                    }

                    if(count>0)
                    {
                            dz=(dz<<16)/count;

                            while(count)
                            {
                                    insert_point((z>>16)>>ELE_SHIFT,x1<<ELE_SHIFT,type);
                                    z+=dz;
                                    x1++;
                                    count--;
                            }
                    }
            }
    */
    if (dx && dz)
        return (1);
    else
        return (0);
}

// uc_orig: build_edge_list (fallen/Source/Building.cpp)
// Allocates all edge-list pools and populates them by scanning the storey's wall polygon.
// Returns 1 if any diagonal edges were found (angles flag), 0 if all edges are axis-aligned.
SLONG build_edge_list(SLONG storey, SLONG flag)
{
    UWORD wall;
    SLONG px, pz;
    SLONG angles = 0;

    next_edge = 1;

    if (edge_pool_ptr) {
        MemFree((UBYTE*)edge_pool_ptr);
        edge_pool_ptr = 0;
    }
    if (edge_heads_ptr) {
        MemFree((UBYTE*)edge_heads_ptr);
        edge_heads_ptr = 0;
    }
    if (flag_blocks) {
        MemFree((UBYTE*)flag_blocks);
        flag_blocks = 0;
    }
    if (flag_blocks2) {
        flag_blocks2 = 0;
    }
    if (cut_blocks) {
        MemFree((UBYTE*)cut_blocks);
        cut_blocks = 0;
    }
    edge_pool_ptr = (struct Edge*)MemAlloc(sizeof(struct Edge) * MAX_BOUND_SIZE * 5);
    edge_heads_ptr = (UWORD*)MemAlloc(sizeof(UWORD) * MAX_BOUND_SIZE);
    flag_blocks = (SLONG*)MemAlloc(sizeof(SLONG) * MAX_BOUND_SIZE * MAX_BOUND_SIZE);
    cut_blocks = (UWORD*)MemAlloc(sizeof(UWORD) * MAX_BOUND_SIZE * MAX_BOUND_SIZE * 4);

    memset((UBYTE*)edge_heads_ptr, 0, sizeof(UWORD) * MAX_BOUND_SIZE);
    memset((UBYTE*)flag_blocks, 0, sizeof(SLONG) * MAX_BOUND_SIZE * MAX_BOUND_SIZE);
    memset((UBYTE*)cut_blocks, 0, sizeof(UWORD) * MAX_BOUND_SIZE * MAX_BOUND_SIZE * 4);

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ - edge_min_z;
    wall = storey_list[storey].WallHead;

    while (wall) {
        if (scan_line(px, pz, wall_list[wall].DX, wall_list[wall].DZ - edge_min_z, flag)) {
            scan_line_z(px, pz, wall_list[wall].DX, wall_list[wall].DZ - edge_min_z, flag);
            angles = 1;
        }
        px = wall_list[wall].DX;
        pz = wall_list[wall].DZ - edge_min_z;
        wall = wall_list[wall].Next;
    }
    return (angles);
}

// uc_orig: build_more_edge_list (fallen/Source/Building.cpp)
// Adds additional wall edges to an already-initialized edge list (used for partial z-range scanning).
void build_more_edge_list(SLONG min_z, SLONG max_z, SLONG storey, SLONG flag)
{
    UWORD wall;
    SLONG px, pz;

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    if (pz > (max_z << ELE_SHIFT)) {
        pz = (max_z << ELE_SHIFT);
    }

    pz -= edge_min_z;
    ASSERT(pz >= 0);

    wall = storey_list[storey].WallHead;

    while (wall) {
        SLONG z;
        z = wall_list[wall].DZ;

        if (z > edge_min_z && z < max_z) {
            if (scan_line(px, pz, wall_list[wall].DX, z - edge_min_z, flag)) {
                scan_line_z(px, pz, wall_list[wall].DX, z - edge_min_z, flag);
            }
        }
        px = wall_list[wall].DX;
        pz = z - edge_min_z;
        wall = wall_list[wall].Next;
    }
}

// uc_orig: scan_bottom_line (fallen/Source/Building.cpp)
// Adds floor vertices for all tiles along the horizontal span from x to x2 at scanline row z.
void scan_bottom_line(SLONG x, SLONG z, SLONG x2, SLONG y)
{
    SLONG dy;

    x2 = x2 >> ELE_SHIFT;
    x = (x >> ELE_SHIFT);
    if (x > x2) {
        SLONG temp;
        temp = x;
        x = x2;
        x2 = temp;
    }

    for (; x <= x2; x++) {
        if (flag_blocks[(x) + (z >> ELE_SHIFT) * MAX_BOUND_SIZE] == 0) {
            dy = get_map_height(x, (z + edge_min_z) >> ELE_SHIFT) << FLOOR_HEIGHT_SHIFT;
            add_point(x << ELE_SHIFT, y + dy, (z) + edge_min_z);
            flag_blocks[(x) + (z >> ELE_SHIFT) * MAX_BOUND_SIZE] = next_prim_point - 1;
        }
    }
}

// uc_orig: build_bottom_edge_list (fallen/Source/Building.cpp)
// Scans horizontal edges of a storey and generates floor vertices for each covered tile.
void build_bottom_edge_list(SLONG storey, SLONG y)
{
    UWORD wall;
    SLONG px, pz;

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ - edge_min_z;
    wall = storey_list[storey].WallHead;

    while (wall) {
        // ensure corner points get floor vertices too
        if (flag_blocks[(px >> ELE_SHIFT) + (pz >> ELE_SHIFT) * MAX_BOUND_SIZE] == 0) {
            SLONG dy;
            dy = get_map_height(px >> ELE_SHIFT, ((pz + edge_min_z) >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT;
            add_point(px, y + dy, (pz) + edge_min_z);
            flag_blocks[(px >> ELE_SHIFT) + (pz >> ELE_SHIFT) * MAX_BOUND_SIZE] = next_prim_point - 1;
        }
        if (pz == wall_list[wall].DZ - edge_min_z)
            scan_bottom_line(px, pz, wall_list[wall].DX, y);
        px = wall_list[wall].DX;
        pz = wall_list[wall].DZ - edge_min_z;
        wall = wall_list[wall].Next;
    }
}

// uc_orig: bin_edge_list (fallen/Source/Building.cpp)
// Frees all dynamically allocated edge-list pools.
void bin_edge_list(void)
{
    if (edge_pool_ptr) {
        MemFree((UBYTE*)edge_pool_ptr);
        edge_pool_ptr = 0;
    }
    if (edge_heads_ptr) {
        MemFree((UBYTE*)edge_heads_ptr);
        edge_heads_ptr = 0;
    }
    if (flag_blocks) {
        MemFree((UBYTE*)flag_blocks);
        flag_blocks = 0;
    }
    if (flag_blocks2) {
        // MemFree((UBYTE*)flag_blocks2);  // intentionally skipped in original
        flag_blocks = 0;
    }
    if (cut_blocks) {
        MemFree((UBYTE*)cut_blocks);
        cut_blocks = 0;
    }
}

// uc_orig: dump_edge_list (fallen/Source/Building.cpp)
// Debug helper: walks all edge lists for 'size' scanlines (no visible output in release).
void dump_edge_list(UWORD size)
{
    SLONG c0;
    SLONG edge;

    for (c0 = 0; c0 < size; c0++) {
        edge = edge_heads_ptr[c0];
        while (edge) {
            edge = edge_pool_ptr[edge].Next;
        }
    }
}

// Macro to set four UV coordinates at once into a local UV[4][2] array.
// Parameter order: (x0,y0) = corner 0, (x1,y1) = corner 1, (x2,y2) = corner 2, (x3,y3) = corner 3.
// Note: UV[2] and UV[3] are swapped relative to parameter order (x3,y3→[2], x2,y2→[3]).
// uc_orig: set_UV4 (fallen/Source/Building.cpp)
#define set_UV4(x0, y0, x1, y1, x2, y2, x3, y3) \
    UV[0][0] = (x0);                              \
    UV[0][1] = (y0);                              \
    UV[1][0] = (x1);                              \
    UV[1][1] = (y1);                              \
    UV[2][0] = (x3);                              \
    UV[2][1] = (y3);                              \
    UV[3][0] = (x2);                              \
    UV[3][1] = (y2);

// uc_orig: build_free_tri_texture_info (fallen/Source/Building.cpp)
// Sets UV coordinates on a free-standing (non-wall) triangle face based on the texture
// at map tile (mx, mz). Bilinearly interpolates UVs across the triangle vertices
// using the tile's texture rotation.
void build_free_tri_texture_info(struct PrimFace3* p_f3, SLONG mx, SLONG mz)
{
    UBYTE tx, ty, page;
    SLONG tsize;
    SLONG rot;
    UBYTE UV[4][2];
    UWORD texture, p;

    SLONG dtx_down, dty_down;
    SLONG dtx_down_r, dty_down_r;

    texture = get_map_texture(mx, mz);

    tx = ((struct MiniTextureBits*)(&texture))->X << 5;
    ty = ((struct MiniTextureBits*)(&texture))->Y << 5;
    page = (UBYTE)(((struct MiniTextureBits*)(&texture))->Page);
    tsize = 31;
    rot = ((struct MiniTextureBits*)(&texture))->Rot;
    switch (rot) {
    case 0:
        set_UV4(tx, ty, tx + tsize, ty, tx, ty + tsize, tx + tsize, ty + tsize);
        break;
    case 1:
        set_UV4(tx + tsize, ty, tx + tsize, ty + tsize, tx, ty, tx, ty + tsize);
        break;
    case 2:
        set_UV4(tx + tsize, ty + tsize, tx, ty + tsize, tx + tsize, ty, tx, ty);
        break;
    case 3:
        set_UV4(tx, ty + tsize, tx, ty, tx + tsize, ty + tsize, tx + tsize, ty);
        break;
    }

    dtx_down = UV[3][0] - UV[0][0];
    dty_down = UV[3][1] - UV[0][1];

    dtx_down_r = UV[2][0] - UV[1][0];
    dty_down_r = UV[2][1] - UV[1][1];

    p_f3->TexturePage = page;

    for (p = 0; p < 3; p++) {
        SLONG x1, z1;
        SLONG lx, ly;
        SLONG rx, ry;

        x1 = prim_points[p_f3->Points[p]].X - (mx << ELE_SHIFT);
        z1 = prim_points[p_f3->Points[p]].Z - (mz << ELE_SHIFT);

        lx = (z1 * dtx_down) >> 8;
        ly = (z1 * dty_down) >> 8;
        lx += UV[0][0];
        ly += UV[0][1];

        rx = (z1 * dtx_down_r) >> 8;
        ry = (z1 * dty_down_r) >> 8;

        rx += UV[1][0];
        ry += UV[1][1];

        p_f3->UV[p][0] = lx + (((rx - lx) * x1) >> 8);
        p_f3->UV[p][1] = ly + (((ry - ly) * x1) >> 8);
    }
}

// uc_orig: build_free_quad_texture_info (fallen/Source/Building.cpp)
// Sets UV coordinates on a free-standing quad face based on the texture at map tile (mx, mz).
// Bilinearly interpolates UVs across all four quad vertices using the tile's texture rotation.
void build_free_quad_texture_info(struct PrimFace4* p_f4, SLONG mx, SLONG mz)
{
    UBYTE tx, ty, page;
    SLONG tsize;
    SLONG rot;
    UBYTE UV[4][2];
    UWORD texture, p;

    SLONG dtx_down, dty_down;
    SLONG dtx_down_r, dty_down_r;

    texture = get_map_texture(mx, mz);

    tx = ((struct MiniTextureBits*)(&texture))->X << 5;
    ty = ((struct MiniTextureBits*)(&texture))->Y << 5;
    page = (UBYTE)(((struct MiniTextureBits*)(&texture))->Page);
    tsize = 31;
    rot = ((struct MiniTextureBits*)(&texture))->Rot;
    switch (rot) {
    case 0:
        set_UV4(tx, ty, tx + tsize, ty, tx, ty + tsize, tx + tsize, ty + tsize);
        break;
    case 1:
        set_UV4(tx + tsize, ty, tx + tsize, ty + tsize, tx, ty, tx, ty + tsize);
        break;
    case 2:
        set_UV4(tx + tsize, ty + tsize, tx, ty + tsize, tx + tsize, ty, tx, ty);
        break;
    case 3:
        set_UV4(tx, ty + tsize, tx, ty, tx + tsize, ty + tsize, tx + tsize, ty);
        break;
    }

    dtx_down = UV[3][0] - UV[0][0];
    dty_down = UV[3][1] - UV[0][1];

    dtx_down_r = UV[2][0] - UV[1][0];
    dty_down_r = UV[2][1] - UV[1][1];

    p_f4->TexturePage = page;

    for (p = 0; p < 4; p++) {
        SLONG x1, z1;
        SLONG lx, ly;
        SLONG rx, ry;

        x1 = prim_points[p_f4->Points[p]].X - (mx << ELE_SHIFT);
        z1 = prim_points[p_f4->Points[p]].Z - (mz << ELE_SHIFT);

        lx = (z1 * dtx_down) >> 8;
        ly = (z1 * dty_down) >> 8;
        lx += UV[0][0];
        ly += UV[0][1];

        rx = (z1 * dtx_down_r) >> 8;
        ry = (z1 * dty_down_r) >> 8;

        rx += UV[1][0];
        ry += UV[1][1];

        p_f4->UV[p][0] = lx + (((rx - lx) * x1) >> 8);
        p_f4->UV[p][1] = ly + (((ry - ly) * x1) >> 8);
    }
}

// uc_orig: scan_45 (fallen/Source/Building.cpp)
// Creates triangle faces along a 45-degree diagonal line in the building footprint.
// Used for angled roof sections. dx/dz define the diagonal direction (±1 per tile).
// flag_blocks[] must be pre-populated with point indices for each tile corner.
void scan_45(SLONG x1, SLONG z1, SLONG dx, SLONG dz)
{
    UBYTE type = 0;
    SLONG count;
    SLONG pp, p1, p2, p3;
    struct PrimFace3* p_f3;

    count = abs(dx) >> ELE_SHIFT;
    x1 = x1 >> ELE_SHIFT;
    z1 = z1 >> ELE_SHIFT;

    if (dx < 0) {
        dx = -1;
        type |= 1;
    } else {
        dx = 1;
    }

    if (dz < 0) {
        type |= 2;
        dz = -1;
    } else {
        dz = 1;
    }

    pp = flag_blocks[(x1) + z1 * MAX_BOUND_SIZE];
    while (count) {
        x1 += dx;
        z1 += dz;

        p1 = flag_blocks[(x1) + z1 * MAX_BOUND_SIZE];
        p2 = flag_blocks[(x1 - dx) + z1 * MAX_BOUND_SIZE];
        p3 = flag_blocks[(x1) + (z1 - dz) * MAX_BOUND_SIZE];
        switch (type) {
        case 0: // SE
            p_f3 = create_a_tri(p2, p1, pp, 0, 0);
            build_free_tri_texture_info(p_f3, x1 - dx, z1 - dz + (edge_min_z >> ELE_SHIFT));
            break;
        case 1: // SW
            p_f3 = create_a_tri(p1, pp, p3, 0, 0);
            build_free_tri_texture_info(p_f3, x1, z1 - dz + (edge_min_z >> ELE_SHIFT));
            break;
        case 2: // NE
            p_f3 = create_a_tri(pp, p3, p1, 0, 0);
            build_free_tri_texture_info(p_f3, x1 - dx, z1 + (edge_min_z >> ELE_SHIFT));
            break;
        case 3: // NW
            p_f3 = create_a_tri(p1, pp, p2, 0, 0);
            build_free_tri_texture_info(p_f3, x1, z1 + (edge_min_z >> ELE_SHIFT));
            break;
        }

        pp = p1;
        count--;
    }
}

// uc_orig: build_storey_lip (fallen/Source/Building.cpp)
// Builds a decorative roof rim / parapet edge at height y for the given storey.
// The lip style (out, height, dip) is determined by FLAG_STOREY_ROOF_RIM / ROOF_RIM2 flags.
// Returns the new top y after the ledge is built.
SLONG build_storey_lip(SLONG storey, SLONG y)
{
    SLONG flag = 0;
    SLONG out, height, dip;

    if (storey_list[storey].StoreyFlags & (FLAG_STOREY_ROOF_RIM))
        flag |= 1;
    if (storey_list[storey].StoreyFlags & (FLAG_STOREY_ROOF_RIM2))
        flag |= 2;

    switch (flag) {
    case 0:
        break;
    case 1:
        out = BLOCK_SIZE;
        height = BLOCK_SIZE + (BLOCK_SIZE >> 1);
        dip = 20;
        break;
    case 2:
        out = BLOCK_SIZE >> 1;
        height = (BLOCK_SIZE >> 1);
        dip = 0;
        break;
    case 3:
        out = BLOCK_SIZE;
        height = (BLOCK_SIZE);
        dip = 0;
        break;
    }

    y = build_ledge2(y, storey, out, height, dip);

    return (y);
}

// uc_orig: create_walkable_structure (fallen/Source/Building.cpp)
// Registers a rectangular roof/ledge region as a walkable area in the DWalkable array.
// Records the point and face ranges so the renderer can identify which geometry belongs
// to each walkable platform.
void create_walkable_structure(SLONG left, SLONG right, SLONG top, SLONG bottom, SLONG y, SLONG sp, SLONG sf4, SLONG dy)
{
    struct DWalkable* p_w;

    if (next_dwalkable > MAX_DWALKABLES - 5)
        ASSERT(0);

    p_w = &dwalkables[next_dwalkable];

    p_w->X1 = left;
    p_w->X2 = right;
    p_w->Z1 = top;
    p_w->Z2 = bottom;

    if (y < 0)
        y = 0;

    p_w->Y = y >> 5;
    ASSERT(p_w->Y != 255);
    p_w->StoreyY = dy;

    p_w->StartPoint = sp;
    p_w->EndPoint = next_prim_point;

    p_w->StartFace4 = sf4;
    p_w->EndFace4 = next_prim_face4;

    p_w->Next = building_list[current_building].Walkable;
    p_w->Building = current_building;
    building_list[current_building].Walkable = next_dwalkable;

    {
        SLONG c0;
        for (c0 = sf4; c0 < next_prim_face4; c0++) {
        }
    }

    next_dwalkable++;
}

// uc_orig: calc_face_split (fallen/Source/Building.cpp)
// Sets FACE_FLAG_OTHER_SPLIT on a quad if its four Y coordinates are not all equal,
// indicating the quad needs to be split for correct rendering.
void calc_face_split(struct PrimFace4* p_f4)
{
    SLONG y0, y1, y2, y3;

    y0 = prim_points[p_f4->Points[0]].Y;
    y1 = prim_points[p_f4->Points[1]].Y;
    y2 = prim_points[p_f4->Points[2]].Y;
    y3 = prim_points[p_f4->Points[3]].Y;

    if (y1 == y2 && y2 == y3 && y0 != y1) {
        p_f4->FaceFlags |= FACE_FLAG_OTHER_SPLIT;
        return;
    }
    if (y0 == y1 && y1 == y2 && y3 != y1) {
        p_f4->FaceFlags |= FACE_FLAG_OTHER_SPLIT;
        return;
    }
}

// uc_orig: build_easy_roof (fallen/Source/Building.cpp)
// Builds flat or gently sloped roof geometry for a building storey.
// Uses the scanline edge list (built by build_edge_list) to determine which map tiles
// are inside the building footprint. For each inside tile, creates a roof quad using
// four corner points from flag_blocks[]. If flag==0, per-tile roof height from the
// map is used (sloped); if flag==1, all tiles are at the same height.
// After geometry creation, applies lookup_roof[] to set the correct auto-tiled roof
// texture for each tile based on its 8 surrounding neighbours.
// Registers the result as a walkable structure and returns a bounding box handle.
SLONG build_easy_roof(SLONG min_x, SLONG edge_min_z, SLONG max_x, SLONG depth, SLONG y, SLONG face_wall, SLONG flag)
{
    SLONG x, z;
    SLONG valid_roof = 0;
    struct PrimFace4* p_f4;
    SLONG small_dy = 9999999;
    SLONG lmin_x = 9999999, lmax_x = -9999999, lmin_z = 9999999, lmax_z = -9999999;
    SLONG maxy = -9999;

    SLONG sp, sf4;

    sp = next_prim_point;
    sf4 = next_prim_face4;

    for (z = 0; z < depth; z++) {
        SLONG polarity = 0;
        SLONG edge;
        SLONG dy = 0;
        edge = edge_heads_ptr[z];

        for (x = min_x - 256 + 128; x < max_x + 256 && edge; x += ELE_SIZE) {

            if ((x) > edge_pool_ptr[edge].X) {
                polarity += edge_pool_ptr[edge].Count;
                edge = edge_pool_ptr[edge].Next;
            }

            if (polarity & 1) {
                SLONG tl, tr, bl, br;
                SLONG texture;

                if (x < lmin_x)
                    lmin_x = x;

                if (z < lmin_z)
                    lmin_z = z;

                if (x > lmax_x)
                    lmax_x = x;

                if (z > lmax_z)
                    lmax_z = z;
                valid_roof = 1;

                tl = flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE];
                if (!tl) {
                    if (flag == 0)
                        dy = (get_roof_height((x >> ELE_SHIFT), z + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT);

                    if (y + dy > maxy)
                        maxy = y + dy;

                    add_point(x - 128, y + dy, (z << ELE_SHIFT) + edge_min_z);
                    flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE] = next_prim_point - 1;
                    tl = next_prim_point - 1;
                }

                tr = flag_blocks[(x >> ELE_SHIFT) + 1 + z * MAX_BOUND_SIZE];
                if (!tr) {
                    if (flag == 0)
                        dy = (get_roof_height((x >> ELE_SHIFT) + 1, z + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT);

                    if (y + dy > maxy)
                        maxy = y + dy;

                    add_point(x - 128 + ELE_SIZE, y + dy, (z << ELE_SHIFT) + edge_min_z);
                    flag_blocks[(x >> ELE_SHIFT) + 1 + z * MAX_BOUND_SIZE] = next_prim_point - 1;
                    tr = next_prim_point - 1;
                }

                bl = flag_blocks[(x >> ELE_SHIFT) + (z + 1) * MAX_BOUND_SIZE];
                if (!bl) {
                    if (flag == 0)
                        dy = (get_roof_height((x >> ELE_SHIFT), z + 1 + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT);

                    if (y + dy > maxy)
                        maxy = y + dy;
                    add_point(x - 128, y + dy, (z << ELE_SHIFT) + edge_min_z + ELE_SIZE);
                    flag_blocks[(x >> ELE_SHIFT) + (z + 1) * MAX_BOUND_SIZE] = next_prim_point - 1;
                    bl = next_prim_point - 1;
                }
                br = flag_blocks[(x >> ELE_SHIFT) + 1 + (z + 1) * MAX_BOUND_SIZE];
                if (!br) {
                    if (flag == 0)
                        dy = (get_roof_height((x >> ELE_SHIFT) + 1, z + 1 + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT);
                    if (y + dy > maxy)
                        maxy = y + dy;

                    add_point(x - 128 + 256, y + dy, (z << ELE_SHIFT) + edge_min_z + ELE_SIZE);
                    flag_blocks[(x >> ELE_SHIFT) + 1 + (z + 1) * MAX_BOUND_SIZE] = next_prim_point - 1;
                    br = next_prim_point - 1;
                }
                if (dy < small_dy)
                    small_dy = dy;

                if (bl < 0)
                    bl = -bl;

                if (tl < 0)
                    tl = -tl;

                if (br < 0)
                    br = -br;

                if (tr < 0)
                    tr = -tr;

                p_f4 = create_a_quad(tr, tl, br, bl, 0, 0);

                if (p_f4) {
                    p_f4->ThingIndex = face_wall;
                    if (p_f4->FaceFlags & FACE_FLAG_NON_PLANAR)
                        calc_face_split(p_f4);
                    add_quad_to_walkable_list(next_prim_face4 - 1);

                    texture = get_map_texture(x >> ELE_SHIFT, z + (edge_min_z >> ELE_SHIFT));
                    ((struct MiniTextureBits*)(&texture))->Rot += 1;
                    build_face_texture_info(p_f4, texture);
                }

            } else {
                if (flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE])
                    flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE] = -flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE];
            }
        }
    }
    lmax_x += 256;
    lmax_z++;

    if (valid_roof) {
        SLONG t_min_x, t_max_x;
        SLONG dx, dz;
        SLONG oz = (edge_min_z >> ELE_SHIFT);

        t_min_x = lmin_x >> 8;
        t_max_x = lmax_x >> 8;

        for (z = lmin_z; z < lmax_z; z++) {
            for (x = t_min_x; x <= t_max_x; x++) {
                SLONG point;

                point = flag_blocks[x + z * MAX_BOUND_SIZE];
                if (point > 0) {
                    SLONG bits = 0;
                    SLONG count = 0, data;

                    if ((edit_map[x][z + oz].Texture & 0x3ff) == 0) {
                        for (dz = -1; dz < 2; dz++)
                            for (dx = -1; dx < 2; dx++) {
                                if (dx || dz) {
                                    SLONG mx, mz;
                                    mx = x + dx;
                                    mz = z + dz + oz;

                                    if (mx < 0 || mx >= 128 || mz < 0 || mz > 128 || mx < t_min_x || mx > t_max_x || mz < oz || mz >= lmax_z + oz) {
                                    } else {
                                        if (flag_blocks[mx + (mz - oz) * MAX_BOUND_SIZE] > 0) {
                                            bits |= 1 << count;
                                        }
                                    }
                                    count++;
                                }
                            }
                        ASSERT(bits < 256);

                        if (data = lookup_roof[bits]) {
                            if (data) {
                                edit_map[x][z + oz].Texture = (data & 0xff) + 6 * 64;
                                edit_map[x][z + oz].Texture |= (data >> 8) << 0xa;
                                edit_map[x][z + oz].Texture |= 1 << 0xe;
                            }
                        }
                    }
                }
            }
        }

        {
            SLONG left, top, right, bottom;

            left = lmin_x >> 8;
            right = lmax_x >> 8;
            top = lmin_z + (edge_min_z >> 8);
            bottom = lmax_z + (edge_min_z >> 8);
            create_walkable_structure(left, right, top, bottom, y + small_dy, sp, sf4, storey_list[wall_list[-face_wall].StoreyHead].DY >> 6);

            return (add_bound_box(left, right, top, bottom, y + small_dy));
        }
    } else
        return (0);
}

// uc_orig: clear_reflective_flag (fallen/Source/Building.cpp)
// Clears PAP_FLAG_REFLECTIVE on all tiles in the given world-coordinate bounding box.
// Called before set_floor_hidden() to wipe the previous reflective state.
void clear_reflective_flag(SLONG min_x, SLONG min_z, SLONG max_x, SLONG max_z)
{
    SLONG minx, maxx, minz, maxz;
    SLONG x, z;

    minx = min_x >> ELE_SHIFT;
    maxx = max_x >> ELE_SHIFT;

    minz = min_z >> ELE_SHIFT;
    maxz = max_z >> ELE_SHIFT;

    SATURATE(minx, 0, 127);
    SATURATE(maxx, 0, 127);
    SATURATE(minz, 0, 127);
    SATURATE(maxz, 0, 127);

    for (x = minx; x < maxx; x++)
        for (z = minz; z < maxz; z++) {
        }
}

// uc_orig: get_storey_outline (fallen/Source/Building.cpp)
// Builds a polygon outline from a circular storey's wall list.
// Returns NULL for non-circular storeys (fire escapes, ladders).
OUTLINE_Outline* get_storey_outline(SLONG storey)
{
    OUTLINE_Outline* oo;

    SLONG wall;
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;

    if (!is_storey_circular(storey)) {
        return NULL;
    }

    oo = OUTLINE_create(128);

    x1 = storey_list[storey].DX >> 8;
    z1 = storey_list[storey].DZ >> 8;

    wall = storey_list[storey].WallHead;

    while (wall) {
        x2 = wall_list[wall].DX >> 8;
        z2 = wall_list[wall].DZ >> 8;

        OUTLINE_add_line(oo, x1, z1, x2, z2);

        x1 = x2;
        z1 = z2;

        wall = wall_list[wall].Next;
    }

    return oo;
}

// uc_orig: do_storeys_overlap (fallen/Source/Building.cpp)
// Returns true if two storeys overlap in the XZ plane (used to cut roof holes for upper storeys).
SLONG do_storeys_overlap(SLONG s1, SLONG s2)
{
    OUTLINE_Outline* oos;
    OUTLINE_Outline* ool;
    oos = get_storey_outline(s1);
    if (oos == NULL)
        return (0);
    ool = get_storey_outline(s2);
    if (ool == NULL)
        return (0);

    if (OUTLINE_overlap(oos, ool)) {
        return (1);
    } else {
        return (0);
    }
}

// uc_orig: build_roof_grid (fallen/Source/Building.cpp)
// Top-level roof builder. Computes the storey bounding box, runs the scanline edge list,
// then either calls build_easy_roof (for axis-aligned buildings) or does a per-tile
// scanline fill for buildings with 45-degree walls.
// Returns a bounding box handle (from add_bound_box), or 0 if no geometry was produced.
SLONG build_roof_grid(SLONG storey, SLONG y, SLONG flat_flag)
{
    SLONG min_x = 9999999, max_x = 0, min_z = 9999999, max_z = 0;
    SLONG depth;
    SLONG x, z;

    SLONG wall;
    struct PrimFace4* p_f4;
    struct PrimFace3* p_f3;
    SLONG face_wall;
    SLONG angles, sstorey;
    SLONG building;

    building = storey_list[storey].BuildingHead;
    if (storey_list[storey].StoreyFlags & (FLAG_STOREY_ROOF_RIM | FLAG_STOREY_ROOF_RIM2))
        y = build_storey_lip(storey, y);
    face_wall = -storey_list[storey].WallHead;

    global_y = get_map_height((storey_list[storey].DX >> ELE_SHIFT), storey_list[storey].DZ >> ELE_SHIFT) << FLOOR_HEIGHT_SHIFT;

    BOUNDS(storey_list[storey].DX, storey_list[storey].DZ);

    sstorey = building_list[building].StoreyHead;
    while (sstorey) {
        wall = storey_list[sstorey].WallHead;
        BOUNDS(storey_list[sstorey].DX, storey_list[sstorey].DZ);
        while (wall) {
            BOUNDS(wall_list[wall].DX, wall_list[wall].DZ);
            wall = wall_list[wall].Next;
        }
        sstorey = storey_list[sstorey].Next;
    }

    block_min_x = min_x;
    block_max_x = max_x;

    min_x -= ELE_SIZE;
    min_z -= ELE_SIZE;
    max_x += ELE_SIZE;
    max_z += ELE_SIZE;

    depth = (max_z - min_z) >> ELE_SHIFT;

    edge_min_z = min_z;

    clear_reflective_flag(min_x, min_z, max_x, max_z);
    set_floor_hidden(storey, 0, PAP_FLAG_REFLECTIVE);

    angles = build_edge_list(storey, 0);

    dump_edge_list(depth);
    if (storey_list[storey].Next) {
        SLONG s;
        SLONG storey_height;
        storey_height = storey_list[storey].Height;

        s = building_list[building].StoreyHead;
        while (s) {
            if (do_storeys_overlap(s, storey) && (storey_list[s].DY == storey_list[storey].DY + storey_height) && (storey_list[s].StoreyType == STOREY_TYPE_SKYLIGHT || storey_list[s].StoreyType == STOREY_TYPE_NORMAL)) {
                build_more_edge_list(min_z, max_z, s, 0);
            }

            s = storey_list[s].Next;
        }
    }

    if (storey == 3) {
    }

    if (!angles) {
        SLONG bound;
        dump_edge_list(depth);
        bound = build_easy_roof(min_x, edge_min_z, max_x, depth, y, face_wall, flat_flag);
        bin_edge_list();
        clear_reflective_flag(min_x, min_z, max_x, max_z);
        return (bound);
    }

    for (z = 0; z < depth; z++) {
        SLONG polarity = 0;
        SLONG edge;
        SLONG dy;
        edge = edge_heads_ptr[z];

        for (x = min_x - 256; x < max_x; x += ELE_SIZE) {
            SLONG done = 0;
            while (!done && edge) {
                if (x < edge_pool_ptr[edge].X) {
                    if (polarity & 1) {
                        dy = get_map_height((x >> ELE_SHIFT), z + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT;
                        add_point(x, y + dy, (z << ELE_SHIFT) + edge_min_z);

                        flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE] = next_prim_point - 1;
                    }
                    done = 1;
                } else if (x == edge_pool_ptr[edge].X) {
                    polarity++;
                    {
                        dy = get_map_height((x >> ELE_SHIFT), z + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT - global_y;
                        add_point(x, y, (z << ELE_SHIFT) + edge_min_z);
                        flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE] = next_prim_point - 1;
                    }

                    edge = edge_pool_ptr[edge].Next;
                    done = 1;
                } else if (x > edge_pool_ptr[edge].X) {
                    polarity++;
                    edge = edge_pool_ptr[edge].Next;
                    if (edge == 0) {
                    }
                }
            }
        }
    }

    build_bottom_edge_list(storey, y);

    {
        SLONG wall;
        SLONG px, pz;
        px = storey_list[storey].DX;
        pz = storey_list[storey].DZ - edge_min_z;
        wall = storey_list[storey].WallHead;
        while (wall) {
            SLONG x, z;
            SLONG dx, dz;
            x = wall_list[wall].DX;
            z = wall_list[wall].DZ - edge_min_z;

            dx = x - px;
            dz = z - pz;
            if (abs(dx) == abs(dz) && dx) {
                scan_45(px, pz, dx, dz);
            }
            px = x;
            pz = z;
            wall = wall_list[wall].Next;
        }
    }

    for (z = 0; z < depth; z++) {
        for (x = min_x >> ELE_SHIFT; x < max_x >> ELE_SHIFT; x++) {
            SLONG p0, p1, p2, p3;
            // p0   p1
            // p3   p2
            p0 = flag_blocks[x + z * MAX_BOUND_SIZE];
            p1 = flag_blocks[x + 1 + z * MAX_BOUND_SIZE];
            p2 = flag_blocks[x + 1 + (z + 1) * MAX_BOUND_SIZE];
            p3 = flag_blocks[x + (z + 1) * MAX_BOUND_SIZE];
            if (p0 && p1 && p2 && p3) {
                SLONG texture;
                p_f4 = create_a_quad(p0, p3, p1, p2, 0, 0);
                if (p_f4) {
                    p_f4->ThingIndex = face_wall;
                    add_quad_to_walkable_list(next_prim_face4 - 1);
                    texture = get_map_texture(x, z + (edge_min_z >> ELE_SHIFT));
                    build_face_texture_info(p_f4, texture);
                }
            } else if (p0 || p1 || p2 || p3) {
                UBYTE exist_flags = 0;

// uc_orig: TL (fallen/Source/Building.cpp)
#define TL (1)
// uc_orig: TR (fallen/Source/Building.cpp)
#define TR (2)
// uc_orig: BL (fallen/Source/Building.cpp)
#define BL (4)
// uc_orig: BR (fallen/Source/Building.cpp)
#define BR (8)
                if (p0)
                    exist_flags |= TL;
                if (p1)
                    exist_flags |= TR;
                if (p2)
                    exist_flags |= BR;
                if (p3)
                    exist_flags |= BL;

                switch (exist_flags) {
                    SLONG xt, xb;
                    SLONG zl, zr;
                    SLONG pa, pb;

                case (TR + BR):
                    xt = cut_blocks[x * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    xb = cut_blocks[x * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];

                    if (xt && xb) {
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f4 = create_a_quad(pa, pb, p1, p2, 0, 0);
                        if (p_f4) {
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    } else if (xt) {
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p2, p1, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xb) {
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p2, p1, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }

                    break;
                case (BL + BR):
                    zl = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (zl && zr) {
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f4 = create_a_quad(pa, p3, pb, p2, 0, 0);
                        if (p_f4) {
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    } else if (zl) {
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        p_f3 = create_a_tri(p3, p2, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (zr) {
                        pa = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f3 = create_a_tri(p3, p2, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;
                case (TL + BL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    xb = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];

                    if (xt && xb) {
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f4 = create_a_quad(p0, p3, pa, pb, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xt) {
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p3, pa, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xb) {
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p3, pa, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;
                case (TL + TR):
                    zl = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (zl && zr) {
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f4 = create_a_quad(p0, pa, p1, pb, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (zl) {
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        p_f3 = create_a_tri(pa, p1, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (zr) {
                        pa = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f3 = create_a_tri(pa, p1, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;

                case (TR + BR + BL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    if (xt && zl) {
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f4 = create_a_quad(pa, pb, p1, p3, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p3, p2, p1, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xt || zl) {
                        if (xt == (x + 1) << ELE_SHIFT || xt == 0) {
                            pb = next_prim_point;
                            add_point((x) << ELE_SHIFT, y, zl + edge_min_z);
                            p_f4 = create_a_quad(pb, p3, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));

                        } else if (zl == (z + 1) << ELE_SHIFT || zl == 0) {
                            pa = next_prim_point;
                            add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(pa, p3, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    }

                    break;
                case (TL + BR + BL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (xt && zr) {
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f4 = create_a_quad(pb, pa, p2, p0, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p3, p2, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xt || zr) {
                        if (xt == x << ELE_SHIFT || xt == 0) {
                            pb = next_prim_point;
                            add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, pb, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));

                        } else if (zr == ((z + 1) << ELE_SHIFT) || zr == 0) {
                            pb = next_prim_point;
                            add_point((xt), y, (z << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, pa, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    }

                    break;
                case (TL + TR + BL):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (xb && zr) {
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f4 = create_a_quad(pa, pb, p3, p1, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p3, p1, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xb || zr) {
                        if (zr == ((z + 1) << ELE_SHIFT) || zr == 0) {
                            pa = next_prim_point;
                            add_point((xb), y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, p1, pa, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));

                        } else if (xb == (x << ELE_SHIFT) || xb == 0) {
                            pb = next_prim_point;
                            add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, p1, pb, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    }

                    break;

                case (TL + TR + BR):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];

                    if (xb && zl) {
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f4 = create_a_quad(pb, pa, p0, p2, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p0, p2, p1, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xb || zl) {
                        if (xb == (x + 1) << ELE_SHIFT || xb == 0) {
                            pb = next_prim_point;
                            add_point((x) << ELE_SHIFT, y, zl + edge_min_z);
                            p_f4 = create_a_quad(p0, pb, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));

                        } else if (zl == (z) << ELE_SHIFT || zl == 0) {
                            pa = next_prim_point;
                            add_point((xb), y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(p0, pb, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    }
                    break;
                case (TL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];

                    if (xt && zl) {
                        pa = next_prim_point;
                        add_point(xt, y, ((z) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f3 = create_a_tri(pb, pa, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;
                case (TR):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (xt && zr) {
                        pa = next_prim_point;
                        add_point(xt, y, ((z) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f3 = create_a_tri(pa, pb, p1, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;
                case (BR):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (xb && zr) {
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f3 = create_a_tri(pb, pa, p2, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;
                case (BL):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];

                    if (xb && zl) {
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f3 = create_a_tri(p3, pa, pb, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }

    bin_edge_list();
    return (0);
}

// uc_orig: is_storey_circular (fallen/Source/Building.cpp)
// Returns true if the storey's wall list closes back to the storey's own start point (DX,DZ).
// Non-circular storeys (fire escapes, ladder sections) don't have roof grids or outlines.
SLONG is_storey_circular(SLONG storey)
{
    SLONG sx, sz, wall;
    sx = storey_list[storey].DX;
    sz = storey_list[storey].DZ;
    wall = storey_list[storey].WallHead;

    while (wall) {
        if (sx == wall_list[wall].DX && sz == wall_list[wall].DZ) {
            return (1);
        }
        wall = wall_list[wall].Next;
    }
    return (0);
}

// uc_orig: set_floor_hidden (fallen/Source/Building.cpp)
// Marks PAP map tiles inside a storey's footprint with the given flags.
// Uses the edge-list scanner to fill the storey polygon. lower=0 disables the
// lower-floor logic (dead code guarded by if(0) in the original).
void set_floor_hidden(SLONG storey, UWORD lower, UWORD flags)
{
    SLONG min_x = 9999999, max_x = 0, min_z = 9999999, max_z = 0;
    SLONG depth;
    SLONG x, z;

    SLONG wall;
    if (!is_storey_circular(storey)) {
        return;
    }

    BOUNDS(storey_list[storey].DX, storey_list[storey].DZ);
    wall = storey_list[storey].WallHead;
    while (wall) {
        BOUNDS(wall_list[wall].DX, wall_list[wall].DZ);
        wall = wall_list[wall].Next;
    }

    block_min_x = min_x;
    block_max_x = max_x;

    min_x -= ELE_SIZE;
    min_z -= ELE_SIZE;
    max_x += ELE_SIZE;
    max_z += ELE_SIZE;

    depth = (max_z - min_z) >> ELE_SHIFT;

    edge_min_z = min_z;

    build_edge_list(storey, 0);

    for (z = 0; z < depth; z++) {
        SLONG polarity = 0;
        SLONG edge;
        edge = edge_heads_ptr[z];
        for (x = min_x; x < max_x; x += ELE_SIZE) {
            SLONG done = 0;
            while (!done && edge) {
                if (x < edge_pool_ptr[edge].X) {
                    if (polarity & 1) {
                        set_map_flag(
                            x >> PAP_SHIFT_HI,
                            z + (edge_min_z >> PAP_SHIFT_HI),
                            flags);
                    }
                    done = 1;
                } else if (x == edge_pool_ptr[edge].X) {
                    polarity += edge_pool_ptr[edge].Count;
                    if (polarity & 1) {
                        set_map_flag(
                            x >> PAP_SHIFT_HI,
                            z + (edge_min_z >> PAP_SHIFT_HI),
                            flags);
                    }
                    edge = edge_pool_ptr[edge].Next;
                    done = 1;
                } else if (x > edge_pool_ptr[edge].X) {
                    polarity += edge_pool_ptr[edge].Count;
                    edge = edge_pool_ptr[edge].Next;
                }
            }
        }
    }

    bin_edge_list();
}

// uc_orig: build_fe_mid_points (fallen/Source/Building.cpp)
// Adds intermediate spine points along the midline of a fire escape section at a given y.
// flag==0 adds both endpoints; flag!=0 adds only the first (avoids duplicate at section joins).
void build_fe_mid_points(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG flag)
{
    SLONG dx, dz, dist;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));

    if (dist == 0)
        return;

    dx = (dx * BLOCK_SIZE) / dist;
    dz = (dz * BLOCK_SIZE) / dist;

    add_point(x1 + dx, y, z1 + dz);
    if (flag == 0)
        add_point(x2 - dx, y, z2 - dz);
}

// uc_orig: build_fire_escape_points (fallen/Source/Building.cpp)
// Generates geometry points for a triangular fire escape landing section.
// flag==0: emits all 10 points (full landing); flag!=0: emits only the midpoints.
void build_fire_escape_points(UWORD storey, SLONG y, SLONG flag)
{
    SLONG walls[3], count = 0, wall;
    SLONG mx, mz, mx2, mz2;
    if (flag == 0) {
        add_point(storey_list[storey].DX, y, storey_list[storey].DZ);
    }
    wall = storey_list[storey].WallHead;
    while (wall && count < 3) {
        walls[count++] = wall;
        if (flag == 0) {
            add_point(wall_list[wall].DX, y, wall_list[wall].DZ);
        }
        wall = wall_list[wall].Next;
    }

    mx = (storey_list[storey].DX + wall_list[walls[2]].DX) >> 1;
    mz = (storey_list[storey].DZ + wall_list[walls[2]].DZ) >> 1;

    mx2 = (wall_list[walls[0]].DX + wall_list[walls[1]].DX) >> 1;
    mz2 = (wall_list[walls[0]].DZ + wall_list[walls[1]].DZ) >> 1;

    if (flag == 0) {
        add_point(mx, y, mz);
        add_point(mx2, y, mz2);
    }
    build_fe_mid_points(y, mx, mz, mx2, mz2, flag);
    build_fe_mid_points(y, wall_list[walls[2]].DX, wall_list[walls[2]].DZ, wall_list[walls[1]].DX, wall_list[walls[1]].DZ, flag);
}

// uc_orig: PsetUV4 (fallen/Source/Building.cpp)
// Local UV-setter macro for build_face_texture_info only.
#define PsetUV4(p_f4, x0, y0, x1, y1, x2, y2, x3, y3, page) \
    p_f4->UV[0][0] = (x0);                                   \
    p_f4->UV[0][1] = (y0);                                   \
    p_f4->UV[1][0] = (x1);                                   \
    p_f4->UV[1][1] = (y1);                                   \
    p_f4->UV[2][0] = (x2);                                   \
    p_f4->UV[2][1] = (y2);                                   \
    p_f4->UV[3][0] = (x3);                                   \
    p_f4->UV[3][1] = (y3);                                   \
    p_f4->TexturePage = page;

// uc_orig: build_face_texture_info (fallen/Source/Building.cpp)
// Applies a MiniTextureBits-encoded texture to a quad face.
// MiniTextureBits layout: X(3b), Y(3b), Page(4b), Rot(2b), Size(2b).
// Rotation 0-3 maps to different UV coordinate orderings. Rot is counter-rotated (rot+3)&3.
void build_face_texture_info(struct PrimFace4* p_f4, UWORD texture)
{
    UBYTE tx, ty, page;
    SLONG tsize;
    SLONG rot;

    tx = ((struct MiniTextureBits*)(&texture))->X << 5;
    ty = ((struct MiniTextureBits*)(&texture))->Y << 5;
    page = (UBYTE)(((struct MiniTextureBits*)(&texture))->Page);
    tsize = 31;
    rot = ((struct MiniTextureBits*)(&texture))->Rot;
    rot = (rot + 3) & 3;
    switch (rot) {
    case 0:
        PsetUV4(p_f4, tx, ty, tx + tsize, ty, tx, ty + tsize, tx + tsize, ty + tsize, page);
        break;
    case 1:
        PsetUV4(p_f4, tx + tsize, ty, tx + tsize, ty + tsize, tx, ty, tx, ty + tsize, page);
        break;
    case 2:
        PsetUV4(p_f4, tx + tsize, ty + tsize, tx, ty + tsize, tx + tsize, ty, tx, ty, page);
        break;
    case 3:
        PsetUV4(p_f4, tx, ty + tsize, tx, ty, tx + tsize, ty + tsize, tx + tsize, ty, page);
        break;
    }
}

// uc_orig: set_quad_planar_flag (fallen/Source/Building.cpp)
// Computes normals for both triangles of a quad. If they differ, sets FACE_FLAG_NON_PLANAR.
// Non-planar quads are split into two triangles during rendering (see calc_face_split).
// Normals are normalised to length 64 to avoid integer overflow in the comparison.
void set_quad_planar_flag(struct PrimFace4* pf4)
{
    SLONG p0, p1, p2, p3;
    SLONG nx, ny, nz, mx, my, mz;
    SLONG vx, vy, vz, wx, wy, wz;

    p0 = pf4->Points[0];
    p1 = pf4->Points[1];
    p2 = pf4->Points[2];
    p3 = pf4->Points[3];

    vx = prim_points[p0].X - prim_points[p2].X;
    vy = prim_points[p0].Y - prim_points[p2].Y;
    vz = prim_points[p0].Z - prim_points[p2].Z;

    wx = prim_points[p1].X - prim_points[p0].X;
    wy = prim_points[p1].Y - prim_points[p0].Y;
    wz = prim_points[p1].Z - prim_points[p0].Z;

    nx = vy * wz - vz * wy;
    ny = vz * wx - vx * wz;
    nz = vx * wy - vy * wx;

    {
        SLONG len;

        len = Root(nx * nx + ny * ny + nz * nz);
        if (len == 0)
            len = 1;
        nx = (nx * 64) / len;
        ny = (ny * 64) / len;
        nz = (nz * 64) / len;
    }

    vx = prim_points[p3].X - prim_points[p1].X;
    vy = prim_points[p3].Y - prim_points[p1].Y;
    vz = prim_points[p3].Z - prim_points[p1].Z;

    wx = prim_points[p2].X - prim_points[p3].X;
    wy = prim_points[p2].Y - prim_points[p3].Y;
    wz = prim_points[p2].Z - prim_points[p3].Z;

    mx = vy * wz - vz * wy;
    my = vz * wx - vx * wz;
    mz = vx * wy - vy * wx;

    {
        SLONG len;

        len = Root(mx * mx + my * my + mz * mz);
        if (len == 0)
            len = 1;
        mx = (mx * 64) / len;
        my = (my * 64) / len;
        mz = (mz * 64) / len;
    }

    if ((nx != mx) || (ny != my) || (nz != mz)) {
        pf4->FaceFlags |= FACE_FLAG_NON_PLANAR;
    }
}

// uc_orig: create_a_quad (fallen/Source/Building.cpp)
// Allocates a PrimFace4 and assigns points p0(TL), p1(TR), p2(BL), p3(BR).
// texture_style==0: uses texture_xy2[] (simple indexed); >0: uses textures_xy[style][piece].
// TEXTURE_PIECE_MIDDLE has a 25% chance of being replaced with MIDDLE1 or MIDDLE2 (visual variety).
// flipx XORs the table flip flag for UV mirroring.
struct PrimFace4* create_a_quad(UWORD p1, UWORD p0, UWORD p3, UWORD p2, SWORD texture_style, SWORD texture_piece, SLONG flipx)
{
    struct PrimFace4* p4;
    SLONG tx, ty;
    SLONG theight = 31;
    SLONG add_page = 1;
    SLONG flip;

    if (texture_style == 0)
        add_page = 0;

    p4 = &prim_faces4[next_prim_face4];
    next_prim_face4++;

    p4->Points[0] = p0;
    p4->Points[1] = p1;
    p4->Points[2] = p2;
    p4->Points[3] = p3;

    p4->DrawFlags = POLY_GT;
    p4->FaceFlags = 0;

    set_quad_planar_flag(p4);

    if (texture_style) {
        if (texture_piece == TEXTURE_PIECE_MIDDLE) {
            if ((build_rand() & 3) == 0) {
                if (build_rand() & 1)
                    texture_piece = TEXTURE_PIECE_MIDDLE1;
                else
                    texture_piece = TEXTURE_PIECE_MIDDLE2;
            }
        }

        tx = textures_xy[texture_style][texture_piece].Tx << 5;
        ty = textures_xy[texture_style][texture_piece].Ty << 5;
        p4->TexturePage = textures_xy[texture_style][texture_piece].Page;
        flip = textures_xy[texture_style][texture_piece].Flip;

        if (add_page)
            add_page_countxy(tx >> 5, ty >> 5, p4->TexturePage);

        p4->DrawFlags = textures_flags[texture_style][texture_piece];
    } else {
        tx = texture_xy2[texture_piece].Tx;
        ty = texture_xy2[texture_piece].Ty;
        p4->TexturePage = texture_xy2[texture_piece].Page;
        flip = textures_xy[texture_style][texture_piece].Flip;
        if (add_page)
            add_page_countxy(tx >> 5, ty >> 5, p4->TexturePage);
        ASSERT(p4->TexturePage < 15);
    }

    if (flipx)
        flip ^= 1;

    switch (flip) {
    case 0:
        p4->UV[0][0] = tx;
        p4->UV[0][1] = ty;
        p4->UV[1][0] = tx + 31;
        p4->UV[1][1] = ty;
        p4->UV[2][0] = tx;
        p4->UV[2][1] = ty + theight;
        p4->UV[3][0] = tx + 31;
        p4->UV[3][1] = ty + 31;
        break;
    case 1: // flip x
        p4->UV[0][0] = tx + 31;
        p4->UV[0][1] = ty;
        p4->UV[1][0] = tx;
        p4->UV[1][1] = ty;
        p4->UV[2][0] = tx + 31;
        p4->UV[2][1] = ty + theight;
        p4->UV[3][0] = tx;
        p4->UV[3][1] = ty + theight;
        break;
    case 2: // flip y
        p4->UV[0][0] = tx;
        p4->UV[0][1] = ty + 31;
        p4->UV[1][0] = tx + 31;
        p4->UV[1][1] = ty + theight;
        p4->UV[2][0] = tx;
        p4->UV[2][1] = ty;
        p4->UV[3][0] = tx + 31;
        p4->UV[3][1] = ty;
        break;
    case 3: // flip x+y
        p4->UV[0][0] = tx + 31;
        p4->UV[0][1] = ty + 31;
        p4->UV[1][0] = tx;
        p4->UV[1][1] = ty + 31;
        p4->UV[2][0] = tx + theight;
        p4->UV[2][1] = ty;
        p4->UV[3][0] = tx;
        p4->UV[3][1] = ty;
        break;
    }

    return (p4);
}

// uc_orig: create_a_quad_tex (fallen/Source/Building.cpp)
// Allocates a PrimFace4 using a raw packed texture word (not the building texture table).
// Texture format: bits 0-2=X, 3-5=Y, 6=page lsb, 7=flip.
struct PrimFace4* create_a_quad_tex(UWORD p1, UWORD p0, UWORD p3, UWORD p2, UWORD texture, SLONG flipx)
{
    struct PrimFace4* p4;
    SLONG tx, ty;
    SLONG flip;

    p4 = &prim_faces4[next_prim_face4];
    next_prim_face4++;

    p4->Points[0] = p0;
    p4->Points[1] = p1;
    p4->Points[2] = p2;
    p4->Points[3] = p3;

    p4->DrawFlags = POLY_GT;
    p4->FaceFlags = 0;

    tx = (texture & 7) << 5;
    ty = ((texture >> 3) & 7) << 5;
    flip = (texture & 0x80) >> 7;
    p4->TexturePage = (texture & 0x7f) >> 6;
    add_page_countxy(tx >> 5, ty >> 5, p4->TexturePage);

    p4->DrawFlags = POLY_GT;
    if (flipx)
        flip ^= 1;

    if (flip) {
        p4->UV[1][0] = tx;
        p4->UV[1][1] = ty;
        p4->UV[0][0] = tx + 31;
        p4->UV[0][1] = ty;
        p4->UV[3][0] = tx;
        p4->UV[3][1] = ty + 31;
        p4->UV[2][0] = tx + 31;
        p4->UV[2][1] = ty + 31;

    } else {
        p4->UV[0][0] = tx;
        p4->UV[0][1] = ty;
        p4->UV[1][0] = tx + 31;
        p4->UV[1][1] = ty;
        p4->UV[2][0] = tx;
        p4->UV[2][1] = ty + 31;
        p4->UV[3][0] = tx + 31;
        p4->UV[3][1] = ty + 31;
    }
    return (p4);
}

// uc_orig: create_a_tri (fallen/Source/Building.cpp)
// Allocates a PrimFace3 from prim_faces3[] and applies texture from texture_xy2[] lookup.
struct PrimFace3* create_a_tri(UWORD p2, UWORD p1, UWORD p0, SWORD texture_id, SWORD texture_piece)
{
    struct PrimFace3* p3;
    SLONG tx, ty;
    texture_id = texture_id;
    p3 = &prim_faces3[next_prim_face3];
    next_prim_face3++;

    p3->Points[0] = p0;
    p3->Points[1] = p1;
    p3->Points[2] = p2;

    p3->DrawFlags = POLY_GT;

    tx = texture_xy2[texture_piece].Tx;
    ty = texture_xy2[texture_piece].Ty;
    p3->UV[0][0] = tx;
    p3->UV[0][1] = ty;
    p3->UV[1][0] = tx + 31;
    p3->UV[1][1] = ty;
    p3->UV[2][0] = tx;
    p3->UV[2][1] = ty + 31;

    p3->TexturePage = texture_xy2[texture_piece].Page;
    ASSERT(p3->TexturePage < 15);
    return (p3);
}

// uc_orig: set_texture_fe (fallen/Source/Building.cpp)
// Applies a hardcoded fire escape texture to a quad: type 0 = grating, type 1 = metal plate.
void set_texture_fe(struct PrimFace4* p4, SLONG xw, SLONG xh, SLONG type)
{
    SLONG tx, ty;
    switch (type) {
    case 0:
        tx = 0;
        ty = 6 * 32;
        break;
    case 1:
        tx = 5 * 32;
        ty = 4 * 32;
        break;
    }

    xw = 1;
    xh = 1;

    p4->UV[0][0] = tx;
    p4->UV[0][1] = ty;
    p4->UV[1][0] = tx + 32 * xw;
    p4->UV[1][1] = ty;
    p4->UV[2][0] = tx;
    p4->UV[2][1] = ty + 32 * xh;
    p4->UV[3][0] = tx + 32 * xw;
    p4->UV[3][1] = ty + 32 * xh;
    p4->TexturePage = 1;
}

// Fire escape face ID constants (local to chunk; not exposed in header).
// uc_orig: FE_FIRST_SLOPE (fallen/Source/Building.cpp)
#define FE_FIRST_SLOPE 1
// uc_orig: FE_PLINTH1 (fallen/Source/Building.cpp)
#define FE_PLINTH1 2
// uc_orig: FE_WALKWAY1 (fallen/Source/Building.cpp)
#define FE_WALKWAY1 3
// uc_orig: FE_PLINTH2 (fallen/Source/Building.cpp)
#define FE_PLINTH2 4
// uc_orig: FE_SLOPE2 (fallen/Source/Building.cpp)
#define FE_SLOPE2 5

// uc_orig: FE_FIRST_SLOPE_RAIL (fallen/Source/Building.cpp)
#define FE_FIRST_SLOPE_RAIL -6
// uc_orig: FE_PLINTH1_RAIL_A (fallen/Source/Building.cpp)
#define FE_PLINTH1_RAIL_A -7
// uc_orig: FE_PLINTH1_RAIL_B (fallen/Source/Building.cpp)
#define FE_PLINTH1_RAIL_B -8
// uc_orig: FE_WALKWAY1_RAIL (fallen/Source/Building.cpp)
#define FE_WALKWAY1_RAIL -9
// uc_orig: FE_PLINTH2_RAIL_A (fallen/Source/Building.cpp)
#define FE_PLINTH2_RAIL_A -10
// uc_orig: FE_PLINTH2_RAIL_B (fallen/Source/Building.cpp)
#define FE_PLINTH2_RAIL_B -11
// uc_orig: FE_SLOPE2_RAIL (fallen/Source/Building.cpp)
#define FE_SLOPE2_RAIL -12

// uc_orig: next_connected_face (fallen/Source/Building.cpp)
// Returns the face index offset for the given (type, id, count) slot in a fire escape face chain.
SLONG next_connected_face(SLONG type, SLONG id, SLONG count)
{
    switch (type) {
    case FACE_TYPE_FIRE_ESCAPE:
        SLONG start;

        start = id_offset[id];
        return (face_offsets[start + count]);
        break;
    }
    return (0);
}

// uc_orig: build_firescape (fallen/Source/Building.cpp)
// Builds the complete multi-storey exterior fire escape geometry for a storey.
// Each level creates: walkway platforms, plinths, sloped ramps, and banisters (transparent rails).
// Walkable faces are registered via add_quad_to_walkable_list().
// SORT_LEVEL_FIRE_ESCAPE=3 controls rendering order for these transparent faces.
void build_firescape(SLONG storey)
{
    SLONG y = 0;
    SLONG count = 0;
    struct PrimFace4* p4;
    SLONG wall;

    wall = -storey_list[storey].WallHead;

    while (count < storey_list[storey].Height) {

        start_point[count] = next_prim_point;
        if (count == 0) {
            build_fire_escape_points(storey, y, 1);
            build_fire_escape_points(storey, y + BLOCK_SIZE, 1);
        } else {
            build_fire_escape_points(storey, y, 0);
            build_fire_escape_points(storey, y + BLOCK_SIZE, 0);
        }

        if (count > 0) {
            // banisters
            p4 = create_a_quad(start_point[count] + 3 + 10, start_point[count] + 0 + 10, start_point[count] + 3, start_point[count] + 4, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 8 + 10, start_point[count] + 3 + 10, start_point[count] + 8, start_point[count] + 3, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 2 + 10, start_point[count] + 9 + 10, start_point[count] + 2, start_point[count] + 9, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 1 + 10, start_point[count] + 2 + 10, start_point[count] + 1, start_point[count] + 2, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 7 + 10, start_point[count] + 6 + 10, start_point[count] + 7, start_point[count] + 6, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            // floors
            p4 = create_a_quad(start_point[count], start_point[count] + 1, start_point[count] + 4, start_point[count] + 5, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_WALKWAY1;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            p4 = create_a_quad(start_point[count] + 4, start_point[count] + 6, start_point[count] + 3, start_point[count] + 8, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_PLINTH2;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            p4 = create_a_quad(start_point[count] + 7, start_point[count] + 5, start_point[count] + 9, start_point[count] + 2, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_PLINTH1;
            add_quad_to_walkable_list(next_prim_face4 - 1);
        }

        if (count == 1) {
            // first slope
            insert_collision_vect(prim_points[start_point[count - 1]].X, prim_points[start_point[count - 1]].Y, prim_points[start_point[count - 1]].Z,
                prim_points[start_point[count - 1] + 1].X, prim_points[start_point[count - 1] + 1].Y, prim_points[start_point[count - 1] + 1].Z, 0, 0, next_prim_face4);

            p4 = create_a_quad(start_point[count - 1], start_point[count] + 7, start_point[count - 1] + 1, start_point[count] + 9, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->ThingIndex = wall;
            p4->FaceFlags |= FACE_FLAG_WALKABLE;
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_FIRST_SLOPE;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            // bannister
            p4 = create_a_quad(start_point[count] + 9 + 10, start_point[count - 1] + 3, start_point[count] + 9, start_point[count - 1] + 1, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_FIRST_SLOPE_RAIL;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        } else if (count > 1) {
            // continue slope
            p4 = create_a_quad(start_point[count - 1] + 6, start_point[count] + 7, start_point[count - 1] + 8, start_point[count] + 9, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_SLOPE2;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            // rail
            p4 = create_a_quad(start_point[count] + 9 + 10, start_point[count - 1] + 8 + 10, start_point[count] + 9, start_point[count - 1] + 8, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_SLOPE2_RAIL;
        }

        count++;
        y += BLOCK_SIZE * 4;
    }
}

// uc_orig: LADDER_SPINE_WIDTH (fallen/Source/Building.cpp)
#define LADDER_SPINE_WIDTH 12

// uc_orig: build_ladder_points (fallen/Source/Building.cpp)
// Generates the prim points for one ladder spine segment.
// flag==1: emits 6 points (full cross-section); flag==0: emits 2 reduced-width points.
void build_ladder_points(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG y, SLONG flag)
{
    SLONG dx, dz;

    dx = x2 - x1;
    dz = z2 - z1;

    if (dx > 0)
        dx = LADDER_SPINE_WIDTH;
    else if (dx < 0)
        dx = -LADDER_SPINE_WIDTH;

    if (dz > 0)
        dz = LADDER_SPINE_WIDTH;
    else if (dz < 0)
        dz = -LADDER_SPINE_WIDTH;

    if (flag == 1) {
        add_point(x1 - dz, y, z1 + dx);
        add_point(x1, y, z1);
        add_point(x1 + dx, y, z1 + dz);
        add_point(x2 - dx, y, z2 - dz);
        add_point(x2, y, z2);
        add_point(x2 - dz, y, z2 + dx);

    } else {
        dx = (dx * 3) >> 2;
        dz = (dz * 3) >> 2;
        add_point(x1 + dx, y, z1 + dz);
        add_point(x2 - dx, y, z2 - dz);
    }
}

// uc_orig: calc_ladder_pos (fallen/Source/Building.cpp)
// Computes the world position and height for a ladder by querying the terrain height map.
// If y==0, picks the minimum terrain height across both endpoints as the base y.
// extra_height accounts for terrain slope below the ladder.
void calc_ladder_pos(SLONG* x1, SLONG* z1, SLONG* x2, SLONG* z2, SLONG* y, SLONG* extra_height)
{
    *extra_height = 0;

    calc_ladder_ends(x1, z1, x2, z2);

    if (*y == 0) {
        SLONG min_y, ty;

        min_y = PAP_calc_height_at(*x1, *z1);
        ty = PAP_calc_height_at(*x2, *z2);

        if (ty < min_y)
            min_y = ty;

        *y = min_y;
        *extra_height = abs(min_y) >> 6;
    } else
        *y += build_max_y;
}

// uc_orig: flat_fill_a_quad_of_points (fallen/Source/Building.cpp)
// Fills a MAX_SIZE x MAX_SIZE grid of quad faces by constructing interior points
// and connecting them to the perimeter already placed in prim_points[].
// Used to tile the flat top surface of a skylight rim.
SLONG flat_fill_a_quad_of_points(SLONG start_point, SLONG w, SLONG h, SLONG texture_style, SLONG wall)
{
    SLONG ax, az;
    SLONG y;
    SLONG c0;
    SLONG texture = TEXTURE_PIECE_RIGHT;
    struct PrimFace4* p_f4;

    y = prim_points[start_point].Y;

    for (c0 = 0; c0 < w; c0++) {
        building_sp[c0][0] = start_point + c0;
        building_sp[c0][h - 1] = start_point + w + h - 2 + w - c0 - 1;

        building_xp[c0] = prim_points[start_point + c0].X;
    }

    building_zp[0] = prim_points[start_point].Z;
    for (c0 = 1; c0 < h - 1; c0++) {
        building_sp[w - 1][c0] = start_point + w + c0 - 1;
        building_sp[0][c0] = start_point + w + w + h - 2 + h - c0 - 2;

        building_zp[c0] = prim_points[start_point + w - 1 + c0].Z;
    }

    for (ax = 1; ax < w - 1; ax++)
        for (az = 1; az < h - 1; az++) {
            building_sp[ax][az] = next_prim_point;
            add_point(building_xp[ax], y, building_zp[az]);
        }

    for (az = 0; az < h - 1; az++) {
        for (ax = 0; ax < w - 1; ax++) {
            p_f4 = create_a_quad(building_sp[ax][az + 1], building_sp[ax + 1][az + 1],
                                  building_sp[ax][az], building_sp[ax + 1][az], texture_style, texture);
            p_f4->ThingIndex = -wall;
            p_f4->Type = FACE_TYPE_SKYLIGHT;
            p_f4->ID = 0;
            add_quad_to_walkable_list(next_prim_face4 - 1);
        }
    }
    return 1;
}

// uc_orig: build_skylight (fallen/Source/Building.cpp)
// Builds geometry for a STOREY_TYPE_SKYLIGHT: a raised rim around a roof hole.
// Creates an outer ring at roof height (y) and an inner ring raised by 50 and
// inset by 50 units. Connects them with quads, then fills the inner rectangle.
// Returns y + 50 (the elevated top surface height).
SLONG build_skylight(SLONG storey)
{
    SLONG count = 0;
    SLONG index, c0;
    SLONG dx, dz;
    SLONG x, y, z, wall;
    SLONG ox, oz;
    SLONG pcount;
    SLONG in = -50;
    SLONG up = 50;
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;

    x = storey_list[storey].DX;
    y = storey_list[storey].DY;
    z = storey_list[storey].DZ;

    y += build_max_y;

    wall = storey_list[storey].WallHead;

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;
    texture = TEXTURE_PIECE_MIDDLE;

    start_point[0] = next_prim_point;
    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        building_numb[count] = create_strip_points(ox, y, oz, dx, y, dz, 256, 0, 0);
        count++;
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    pcount = build_outline(&building_sx[0], &building_sz[0], storey, wall, y + up, in);

    start_point[1] = next_prim_point;
    count = 0;
    for (c0 = 0; c0 < pcount; c0++) {
        create_strip_points(building_sx[c0], y + up, building_sz[c0],
                            building_sx[c0 + 1], y + up, building_sz[c0 + 1], 256, building_numb[count], 0);
        count++;
    }
    count = start_point[1] - start_point[0];

    for (c0 = 0; c0 < count - 1; c0++) {
        p_f4 = create_a_quad(start_point[1] + c0, start_point[1] + c0 + 1,
                              start_point[0] + c0, start_point[0] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->Type = FACE_TYPE_SKYLIGHT;
        p_f4->ID = 0;
        add_quad_to_walkable_list(next_prim_face4 - 1);
    }
    p_f4 = create_a_quad(start_point[1] + c0, start_point[1],
                          start_point[0] + c0, start_point[0], texture_style, texture);
    p_f4->ThingIndex = -wall;
    p_f4->Type = FACE_TYPE_SKYLIGHT;
    p_f4->ID = 0;
    add_quad_to_walkable_list(next_prim_face4 - 1);

    flat_fill_a_quad_of_points(start_point[1], building_numb[0] + 1, building_numb[1] + 1, texture_style, wall);

    return y + up;
}

// uc_orig: build_ladder (fallen/Source/Building.cpp)
// Builds climbable ladder geometry for a STOREY_TYPE_LADDER storey.
// Registers three collision vectors (the ladder surface + two side guards to prevent squeezing).
// Produces vertical spine strips (build_ladder_points flag=1) and horizontal rungs (flag=0,
// one rung per BLOCK_SIZE of height).
void build_ladder(SLONG storey)
{
    SLONG y = 0, c0;
    struct PrimFace4* p4;
    SLONG wall;
    SLONG extra_height;

    SLONG x1, z1, x2, z2;
    SLONG wx1, wz1, wx2, wz2;
    SLONG texture_style;

    wall = storey_list[storey].WallHead;
    texture_style = wall_list[wall].TextureStyle;

    wx1 = x1 = storey_list[storey].DX;
    wz1 = z1 = storey_list[storey].DZ;

    wx2 = x2 = wall_list[wall].DX;
    wz2 = z2 = wall_list[wall].DZ;

    wall = -wall;
    y = storey_list[storey].DY;
    calc_ladder_pos(&x1, &z1, &x2, &z2, &y, &extra_height);

    insert_collision_vect(x1, y, z1, x2, y, z2, STOREY_TYPE_LADDER, storey_list[storey].Height, wall);

    // Extra collision vectors prevent the player from squeezing between wall and ladder.
    insert_collision_vect(wx1, y, wz1, x1, y, z1, STOREY_TYPE_LADDER, storey_list[storey].Height, wall);
    insert_collision_vect(x2, y, z2, wx2, y, wz2, STOREY_TYPE_LADDER, storey_list[storey].Height, wall);
    insert_collision_vect(wx1, y, wz1, x1, y, z1, STOREY_TYPE_LADDER, 0, wall);
    insert_collision_vect(x2, y, z2, wx2, y, wz2, STOREY_TYPE_LADDER, 0, wall);

    {
        SLONG height, size, count;

        height = (storey_list[storey].Height) * 64;

        count = height >> 8;
        if (count == 0) {
            count += 1;
        }
        size = height / count;

        start_point[0] = next_prim_point;
        build_ladder_points(x1, z1, x2, z2, y, 1);

        for (c0 = 1; c0 <= count; c0++) {
            start_point[c0] = next_prim_point;
            build_ladder_points(x1, z1, x2, z2, y + size * c0, 1);

            p4 = create_a_quad(start_point[c0] + 0, start_point[c0] + 1,
                                start_point[c0 - 1] + 0, start_point[c0 - 1] + 1, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[c0] + 1, start_point[c0] + 2,
                                start_point[c0 - 1] + 1, start_point[c0 - 1] + 2, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[c0] + 3, start_point[c0] + 4,
                                start_point[c0 - 1] + 3, start_point[c0 - 1] + 4, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[c0] + 4, start_point[c0] + 5,
                                start_point[c0 - 1] + 4, start_point[c0 - 1] + 5, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }
    }

    for (c0 = 0; c0 < storey_list[storey].Height; c0++) {
        SLONG spr;
        spr = next_prim_point;

        build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1) - 8, 0);
        build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1), 0);

        p4 = create_a_quad(spr + 2, spr + 2 + 1, spr + 0, spr + 1, texture_style, TEXTURE_PIECE_LEFT);
        p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
        p4->ThingIndex = wall;
        OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
    }
}

// uc_orig: build_ladder_old (fallen/Source/Building.cpp)
// Older ladder builder using 4-block-height segments. Unused in normal gameplay.
void build_ladder_old(SLONG storey)
{
    SLONG y = 0, c0;
    SLONG count = 0;
    struct PrimFace4* p4;
    SLONG wall;
    SLONG extra_height;

    SLONG x1, z1, x2, z2;

    wall = storey_list[storey].WallHead;

    x1 = storey_list[storey].DX;
    z1 = storey_list[storey].DZ;

    x2 = wall_list[wall].DX;
    z2 = wall_list[wall].DZ;

    wall = -wall;
    y = storey_list[storey].DY;
    calc_ladder_pos(&x1, &z1, &x2, &z2, &y, &extra_height);

    insert_collision_vect(x1, y, z1, x2, y, z2, STOREY_TYPE_LADDER, 0, wall);

    while (count < (storey_list[storey].Height)) {

        start_point[count] = next_prim_point;
        if (count == 0) {
            build_ladder_points(x1, z1, x2, z2, y, 1);
            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * 4, 1);
        } else {
            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * 4, 1);
        }

        if (count == 0) {
            p4 = create_a_quad(start_point[count] + 0 + 6, start_point[count] + 1 + 6,
                                start_point[count] + 0 + 0, start_point[count] + 1 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 1 + 6, start_point[count] + 2 + 6,
                                start_point[count] + 1 + 0, start_point[count] + 2 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 3 + 6, start_point[count] + 4 + 6,
                                start_point[count] + 3 + 0, start_point[count] + 4 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 4 + 6, start_point[count] + 5 + 6,
                                start_point[count] + 4 + 0, start_point[count] + 5 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }
        if (count) {
            p4 = create_a_quad(start_point[count] + 0, start_point[count] + 1,
                                start_point[count - 1] + 0, start_point[count - 1] + 1, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 1, start_point[count] + 2,
                                start_point[count - 1] + 1, start_point[count - 1] + 2, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 3, start_point[count] + 4,
                                start_point[count - 1] + 3, start_point[count - 1] + 4, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 4, start_point[count] + 5,
                                start_point[count - 1] + 4, start_point[count - 1] + 5, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }

        for (c0 = 0; c0 < 4; c0++) {
            SLONG spr;
            spr = next_prim_point;

            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1) - 8, 0);
            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1), 0);

            p4 = create_a_quad(spr + 2, spr + 2 + 1, spr + 0, spr + 1, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }

        count++;
        y += BLOCK_SIZE * 4;
    }
}

// uc_orig: calc_sin_from_cos (fallen/Source/Building.cpp)
// Computes sin from cos using fixed-point integer sqrt: sin = sqrt(1 - cos^2).
// Input/output in 14-bit fixed-point (1.0 = 1<<14). Result always positive.
SLONG calc_sin_from_cos(SLONG sin)
{
    SLONG cos;

    cos = Root((1 << 14) - ((sin * sin) >> 14));
    cos = cos << 7;
    return cos;
}

// uc_orig: calc_new_corner_point (fallen/Source/Building.cpp)
// Computes the miter (bisector) corner point where two wall segments meet.
// Given three wall vertices x1,z1 -> x2,z2 -> x3,z3, places a new point
// at distance 'width' from x2,z2 along the bisector of the corner angle.
// Result written to *res_x, *res_z.
void calc_new_corner_point(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG x3, SLONG z3,
                            SLONG width, SLONG* res_x, SLONG* res_z)
{
    SLONG vx, vz, dist;
    SLONG wx, wz;
    SLONG ax, az;
    SLONG angle;

    SLONG z;

    vx = x2 - x1;
    vz = z2 - z1;

    wx = -(x3 - x2);
    wz = -(z3 - z2);

    z = vx * wz - vz * wx;

    dist = Root(SDIST2(vx, vz));
    if (dist == 0)
        return;

    vx = (vx << 14) / dist;
    vz = (vz << 14) / dist;

    dist = Root(SDIST2(wx, wz));
    if (dist == 0)
        return;

    wx = (wx << 14) / dist;
    wz = (wz << 14) / dist;

    ax = (vx + wx);
    az = (vz + wz);

    dist = Root(SDIST2(ax, az));
    if (dist == 0) {
        // Vectors cancel — perpendicular offset instead.
        *res_x = ((vz * width) >> 14) + x2;
        *res_z = -((vx * width) >> 14) + z2;
        return;
    }

    ax = (ax << 14) / dist;
    az = (az << 14) / dist;

    angle = (vx * ax + vz * az) >> 14;
    angle = calc_sin_from_cos(angle);

    if (angle == 0)
        dist = width;
    else
        dist = (width << 14) / angle;

    if (z > 0)
        dist = -dist;

    *res_x = ((ax * dist) >> 14) + x2;
    *res_z = ((az * dist) >> 14) + z2;
}

// uc_orig: build_ledge (fallen/Source/Building.cpp)
// Builds a three-layer inward-offset ledge around a storey's perimeter.
// Creates three rings of points: the wall outline, an inset copy, and a raised inset copy.
// Connects them with quads to form the ledge faces.
void build_ledge(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG count = 0;
    SLONG index, c0;
    SLONG dx, dz;
    SLONG px, pz, rx, rz;

    storey = storey; // suppress unused warning (mirrors original)

    start_point[0] = next_prim_point;
    add_point(x, y, z);
    index = wall;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        add_point(dx, y, dz);
        index = wall_list[index].Next;
        count++;
    }
    start_point[1] = next_prim_point;
    add_point(x, y, z);

    px = x;
    pz = z;
    index = wall;
    while (index) {
        SLONG next;
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        next = wall_list[index].Next;
        if (next) {
            calc_new_corner_point(px, pz, dx, dz, wall_list[next].DX, wall_list[next].DZ, BLOCK_SIZE, &rx, &rz);
            add_point(rx, y, rz);
        } else {
            calc_new_corner_point(px, pz, dx, dz, wall_list[wall].DX, wall_list[wall].DZ, BLOCK_SIZE, &rx, &rz);
            add_point(rx, y, rz);
            prim_points[start_point[1]].X = rx;
            prim_points[start_point[1]].Z = rz;
        }

        px = dx;
        pz = dz;
        index = wall_list[index].Next;
    }

    y += height;
    start_point[2] = next_prim_point;
    for (c0 = start_point[1]; c0 < start_point[2]; c0++) {
        prim_points[next_prim_point].X = prim_points[c0].X;
        prim_points[next_prim_point].Y = y + 2;
        prim_points[next_prim_point].Z = prim_points[c0].Z;
        next_prim_point++;
    }

    start_point[3] = next_prim_point;
    add_point(x, y, z);
    index = wall;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        add_point(dx, y + 2, dz);
        index = wall_list[index].Next;
    }

    for (c0 = 0; c0 <= count; c0++) {
        create_a_quad(start_point[1] + c0, start_point[1] + c0 + 1,
                      start_point[0] + c0, start_point[0] + c0 + 1, 0, 18);
        create_a_quad(start_point[2] + c0, start_point[2] + c0 + 1,
                      start_point[1] + c0, start_point[1] + c0 + 1, 0, 18);
        create_a_quad(start_point[3] + c0, start_point[3] + c0 + 1,
                      start_point[2] + c0, start_point[2] + c0 + 1, 0, 18);
    }
}

// uc_orig: create_strip_points (fallen/Source/Building.cpp)
// Generates 'count' evenly-spaced prim points along the segment (x1,y1,z1)→(x2,y2,z2).
// If numb > 0, uses that as the subdivision count; otherwise divides by 'len'.
// If end_flag is set, snaps the final point exactly to (x2,y2,z2).
// Returns the number of intervals (points - 1).
SLONG create_strip_points(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2,
                           SLONG len, SLONG numb, SLONG end_flag)
{
    SLONG count;
    SLONG dist;
    SLONG dx, dz;
    SLONG wwidth, wcount;

    dx = x2 - x1;
    dz = z2 - z1;

    dist = Root(dx * dx + dz * dz);

    if (dist == 0)
        dist = 1;

    if (numb)
        count = numb;
    else
        count = dist / len;

    wcount = count;

    wwidth = dist / wcount;

    dx = (x2 - x1);
    dz = (z2 - z1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    add_point(x1, y1, z1);
    count--;

    while (count) {
        x1 = x1 + ((dx * wwidth) >> 10);
        z1 = z1 + ((dz * wwidth) >> 10);
        add_point(x1, y1, z1);
        count--;
    }
    if (end_flag)
        add_point(x2, y1, z2);
    return wcount;
}

// uc_orig: build_outline (fallen/Source/Building.cpp)
// Builds the miter-corner polygon outline for a storey at height y, offset by 'out' units.
// Writes vertex X/Z positions into sx[]/sz[] (caller-allocated).
// Returns the number of vertices written.
SLONG build_outline(SLONG* sx, SLONG* sz, SLONG storey, SLONG wall, SLONG y, SLONG out)
{
    SLONG offset;
    SLONG px;
    SLONG pz;
    SLONG index;
    SLONG dx, dz;
    SLONG rx, rz;

    offset = 1;

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;

    index = wall;
    while (index) {
        SLONG next;
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        next = wall_list[index].Next;
        if (next) {
            calc_new_corner_point(px, pz, dx, dz, wall_list[next].DX, wall_list[next].DZ, out, &rx, &rz);
            sx[offset] = rx;
            sz[offset] = rz;
        } else {
            calc_new_corner_point(px, pz, dx, dz, wall_list[wall].DX, wall_list[wall].DZ, out, &rx, &rz);
            sx[0] = rx;
            sz[0] = rz;
            sx[offset] = rx;
            sz[offset] = rz;
        }

        px = dx;
        pz = dz;
        index = wall_list[index].Next;
        offset++;
    }
    return offset - 1;
}

// uc_orig: ladder_on_block (fallen/Source/Building.cpp)
// Returns 1 if the centroid of the quad formed by prim points p0–p3 lies over a FLOOR_LADDER tile.
SLONG ladder_on_block(SLONG p0, SLONG p1, SLONG p2, SLONG p3)
{
    SLONG ax, az;
    SLONG flags;

    ax = prim_points[p0].X;
    ax += prim_points[p1].X;
    ax += prim_points[p2].X;
    ax += prim_points[p3].X;

    az = prim_points[p0].Z;
    az += prim_points[p1].Z;
    az += prim_points[p2].Z;
    az += prim_points[p3].Z;

    ax >>= 2;
    az >>= 2;

    flags = get_map_flags(ax >> PAP_SHIFT_HI, az >> PAP_SHIFT_HI);

    if (flags & FLOOR_LADDER)
        return 1;
    else
        return 0;
}

// build_ledge_around_ladder — dead code in the original (entire body in /* */), not migrated.

// uc_orig: build_ledge2 (fallen/Source/Building.cpp)
// Builds a five-ring outward-projecting ledge around the top of a storey.
// Rings: wall outline at y, outer ring at y, outer ring raised, inner ring raised, inner ring at half-height.
// Skips faces where a FLOOR_LADDER tile is detected (to leave space for ladder access).
// Returns y + height - dip (the new effective top-surface height).
SLONG build_ledge2(SLONG y, SLONG storey, SLONG out, SLONG height, SLONG dip)
{
    SLONG count = 0;
    SLONG index, c0;
    SLONG dx, dz;
    SLONG x, z, wall;
    SLONG ox, oz;
    SLONG pcount;

    x = storey_list[storey].DX;
    z = storey_list[storey].DZ;
    wall = storey_list[storey].WallHead;

    storey = storey; // suppress unused warning (mirrors original)

    start_point[0] = next_prim_point;
    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        building_numb[count] = create_strip_points(ox, y, oz, dx, y, dz, 256, 0, 1);
        count++;
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    pcount = build_outline(&building_sx_l2[0], &building_sz_l2[0], storey, wall, y, out);

    start_point[1] = next_prim_point;
    count = 0;
    for (c0 = 0; c0 < pcount; c0++) {
        create_strip_points(building_sx_l2[c0], y, building_sz_l2[c0],
                            building_sx_l2[c0 + 1], y, building_sz_l2[c0 + 1], 256, building_numb[count], 1);
        count++;
    }

    start_point[2] = next_prim_point;

    pcount = build_outline(&building_sx_l2[0], &building_sz_l2[0], storey, wall, y + height, out);

    count = 0;
    for (c0 = 0; c0 < pcount; c0++) {
        create_strip_points(building_sx_l2[c0], y + height, building_sz_l2[c0],
                            building_sx_l2[c0 + 1], y + height, building_sz_l2[c0 + 1], 256, building_numb[count], 1);
        count++;
    }

    start_point[3] = next_prim_point;

    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        create_strip_points(ox, y + height, oz, dx, y + height, dz, 256, building_numb[count], 1);
        count++;
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    start_point[4] = next_prim_point;

    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        create_strip_points(ox, y + height - dip, oz, dx, y + height - dip, dz, 256, building_numb[count], 1);
        count++;
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    count = start_point[1] - start_point[0];
    for (c0 = 0; c0 < count - 1; c0++) {
        SLONG p0, p1, p2, p3, p4;
        p0 = start_point[0] + c0;
        p1 = start_point[1] + c0;
        p2 = start_point[2] + c0;
        p3 = start_point[3] + c0;
        p4 = start_point[4] + c0;

        if (!ladder_on_block(p1, p1 + 1, p0, p0 + 1)) {
            create_a_quad(p1, p1 + 1, p0, p0 + 1, 0, 18);
            create_a_quad(p2, p2 + 1, p1, p1 + 1, 0, 18);
            create_a_quad(p3, p3 + 1, p2, p2 + 1, 0, 18);
            create_a_quad(p3 + 1, p3, p4 + 1, p4, 0, 18);
        } else {
            create_a_quad(p2, p3, p1, p0, 0, 18);
            create_a_quad(p3 + 1, p2 + 1, p0 + 1, p1 + 1, 0, 18);
            create_a_quad(p4, p4 + 1, p0, p0 + 1, 0, 18);
        }
    }
    return y + height - dip;
}

// uc_orig: append_recessed_wall_prim (fallen/Source/Building.cpp)
// Builds a recessed-window wall face: one set of wall-only points is offset inward by RECESS_SIZE,
// then quads are created between the outer and inner point rows on each side and the face itself.
#define RECESS_SIZE (32)
void append_recessed_wall_prim(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG x2, z2;
    SLONG dx, dz, len;
    SLONG texture, texture_style;
    SLONG texture_style2;
    struct PrimFace4* p_f4;
    SLONG c0;
    UBYTE* ptexture1;
    UBYTE* ptexture2;
    SLONG tcount2;

    ptexture1 = wall_list[wall].Textures;
    ptexture2 = wall_list[wall].Textures2;
    tcount2 = wall_list[wall].Tcount2;

    texture_style = wall_list[wall].TextureStyle;
    texture_style2 = wall_list[wall].TextureStyle2;
    if (texture_style == 0)
        texture_style = 1;
    if (texture_style2 == 0)
        texture_style2 = 1;

    x2 = wall_list[wall].DX;
    z2 = wall_list[wall].DZ;

    dx = x2 - x;
    dz = z2 - z;

    len = Root(dx * dx + dz * dz);
    if (len == 0)
        len = 1;

    dx = (dx * RECESS_SIZE) / len;
    dz = (dz * RECESS_SIZE) / len;

    start_point[0] = next_prim_point;
    add_point(x, y, z);
    add_point(x, y + height, z);

    add_point(x + dz, y, z - dx);
    add_point(x + dz, y + height, z - dx);

    start_point[1] = next_prim_point;
    add_point(x2, y, z2);
    add_point(x2, y + height, z2);

    add_point(x2 + dz, y, z2 - dx);
    add_point(x2 + dz, y + height, z2 - dx);

    texture = TEXTURE_PIECE_LEFT;

    texture_style = wall_list[wall].TextureStyle;

    if (texture_style == 0)
        texture_style = 1;

    p_f4 = create_a_quad(start_point[0] + 1, start_point[0] + 3, start_point[0] + 0, start_point[0] + 2, texture_style, texture); // left
    p_f4->ThingIndex = -wall;

    texture = TEXTURE_PIECE_RIGHT;
    p_f4 = create_a_quad(start_point[1] + 3, start_point[1] + 1, start_point[1] + 2, start_point[1] + 0, texture_style, texture); // right
    p_f4->ThingIndex = -wall;

    texture = TEXTURE_PIECE_MIDDLE2;
    p_f4 = create_a_quad(start_point[0], start_point[0] + 2, start_point[1] + 0, start_point[1] + 2, texture_style, texture); // floor
    p_f4->ThingIndex = -wall;

    texture = TEXTURE_PIECE_MIDDLE1;
    p_f4 = create_a_quad(start_point[0] + 3, start_point[0] + 1, start_point[1] + 3, start_point[1] + 1, texture_style, texture); // roof
    p_f4->ThingIndex = -wall;

    x += dz;
    z += -dx;

    x2 += dz;
    z2 += -dx;

    start_point[0] = build_row_wall_only_points_at_y(y + height + 2, x, z, x2, z2, wall);
    start_point[1] = build_row_wall_only_points_at_y(y, x, z, x2, z2, wall);
    for (c0 = 0; c0 < WindowCount; c0++) {
        texture = TEXTURE_PIECE_MIDDLE;
        if (c0 == 0) {
            texture = TEXTURE_PIECE_RIGHT;
        } else if (c0 == WindowCount - 1) {

            texture = TEXTURE_PIECE_LEFT;
        } else {
            texture = TEXTURE_PIECE_MIDDLE;
        }

        if (ptexture1 && (c0 < WindowCount) && ptexture1[c0]) {
            p_f4 = create_a_quad_tex(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, ptexture1[c0]);
            p_f4->ThingIndex = -wall;
        } else {
            {
                p_f4 = create_a_quad(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, texture_style, texture);
                p_f4->ThingIndex = -wall;
            }
        }
        {
            if (ptexture2 && (c0 < tcount2) && ptexture2[c0]) {
                p_f4 = create_a_quad_tex(start_point[0] + c0 + 1, start_point[0] + c0, start_point[1] + c0 + 1, start_point[1] + c0, ptexture2[c0]);
                p_f4->ThingIndex = -wall;
            } else {
                p_f4 = create_a_quad(start_point[0] + c0 + 1, start_point[0] + c0, start_point[1] + c0 + 1, start_point[1] + c0, texture_style2, texture);
                p_f4->ThingIndex = -wall;
            }
            p_f4->FaceFlags |= FACE_FLAG_TEX2;
        }
    }
}

// uc_orig: append_foundation_wall (fallen/Source/Building.cpp)
// Builds the ground-level (foundation) wall face between y=0 and y+2. Uses only
// wall-only points (no window depth offset). The inner window loop body is intentionally
// dead in the current code (WindowCount is 0 before this is called).
void append_foundation_wall(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG c0;
    SLONG start_point_local[10];
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;
    UBYTE* ptexture;
    SLONG tcount, count;

    /*
            if(wall_list[wall].WallFlags&FLAG_WALL_RECESSED)
            {
                    append_recessed_wall_prim(x,y,z, wall,storey,height);
                    return;

            }
    */
    ptexture = wall_list[wall].Textures;
    tcount = wall_list[wall].Tcount;

    set_build_seed(x * z + y);

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    if (!(wall_list[wall].WallFlags & FLAG_WALL_AUTO_WINDOWS) && WindowCount == 0) {
        start_point_local[0] = build_row_wall_only_points_at_y(y + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        start_point_local[1] = build_row_wall_only_points_at_floor_alt(0, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        count = 0;
        for (c0 = 0; c0 < WindowCount; c0++) {
            texture = TEXTURE_PIECE_MIDDLE;
            if (c0 == 0) {
                texture = TEXTURE_PIECE_RIGHT;
            } else if (c0 == WindowCount - 1) {

                texture = TEXTURE_PIECE_LEFT;
            } else {
                texture = TEXTURE_PIECE_MIDDLE;
            }
            if (ptexture && (count < tcount) && ptexture[count]) {
                p_f4 = create_a_quad_tex(start_point_local[0] + c0, start_point_local[0] + c0 + 1, start_point_local[1] + c0, start_point_local[1] + c0 + 1, ptexture[count]);
                p_f4->ThingIndex = -wall;
            } else {
                p_f4 = create_a_quad(start_point_local[0] + c0, start_point_local[0] + c0 + 1, start_point_local[1] + c0, start_point_local[1] + c0 + 1, texture_style, texture);
                p_f4->ThingIndex = -wall;
            }
            // Fix texture V extents to match actual wall height
            {
                SLONG dy;
                dy = prim_points[start_point_local[0] + c0 + 1].Y - prim_points[start_point_local[1] + c0 + 1].Y;
                if (dy > 256)
                    dy = 256;
                dy = (31 * dy) >> 8;
                p_f4->UV[2][1] = p_f4->UV[0][1] + dy;

                dy = prim_points[start_point_local[0] + c0].Y - prim_points[start_point_local[1] + c0].Y;
                if (dy > 256)
                    dy = 256;
                dy = (31 * dy) >> 8;
                p_f4->UV[3][1] = p_f4->UV[1][1] + dy;
            }
            count++;
        }
        WindowCount = 0;
    }
}

// uc_orig: append_wall_prim (fallen/Source/Building.cpp)
// Builds the standard wall face for a storey wall segment. Handles single-sided (exterior) and
// double-sided (warehouse / interior-partition) variants. At heights < 256 the UV extents are
// adjusted to avoid texture stretching.
void append_wall_prim(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG c0;
    SLONG start_point_local[10];
    SLONG texture, texture_style, texture_style2;
    SLONG count;
    struct PrimFace4* p_f4;
    UBYTE* ptexture1;
    SLONG tcount1;
    UBYTE* ptexture2;
    SLONG tcount2;
    SLONG no_draw = 0;
    SLONG circular;

    circular = is_storey_circular(storey);

    if (next_prim_point > 50000)
        no_draw = 1;

    if (wall_list[wall].WallFlags & FLAG_WALL_RECESSED) {
        append_recessed_wall_prim(x, y, z, wall, storey, height);
        return;
    }
    WindowCount = 0;

    ptexture1 = wall_list[wall].Textures;
    tcount1 = wall_list[wall].Tcount;
    ptexture2 = wall_list[wall].Textures2;
    tcount2 = wall_list[wall].Tcount2;

    set_build_seed(x * z + y);

    texture_style = wall_list[wall].TextureStyle;
    texture_style2 = wall_list[wall].TextureStyle2;
    if (texture_style == 0)
        texture_style = 1;
    if (texture_style2 == 0)
        texture_style2 = 1;

    if (!(wall_list[wall].WallFlags & FLAG_WALL_AUTO_WINDOWS) && WindowCount == 0) {
        if (!no_draw) {
            if (!circular) {
                start_point_local[0] = build_row_wall_only_points_at_floor_alt(y + height + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
                start_point_local[1] = build_row_wall_only_points_at_floor_alt(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
            } else {
                start_point_local[0] = build_row_wall_only_points_at_y(y + height + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
                start_point_local[1] = build_row_wall_only_points_at_y(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
            }
        } else {
            start_point_local[0] = 0;
            start_point_local[1] = 0;
        }
        count = 0;
        for (c0 = 0; c0 < WindowCount; c0++) {
            texture = TEXTURE_PIECE_MIDDLE;
            if (c0 == 0) {
                texture = TEXTURE_PIECE_RIGHT;
            } else if (c0 == WindowCount - 1) {

                texture = TEXTURE_PIECE_LEFT;
            } else {
                texture = TEXTURE_PIECE_MIDDLE;
            }

            {

                if (ptexture1 && (count < tcount1) && ptexture1[count]) {
                    p_f4 = create_a_quad_tex(start_point_local[0] + c0, start_point_local[0] + c0 + 1, start_point_local[1] + c0, start_point_local[1] + c0 + 1, ptexture1[count]);
                    if (p_f4)
                        p_f4->ThingIndex = -wall;
                } else {
                    p_f4 = create_a_quad(start_point_local[0] + c0, start_point_local[0] + c0 + 1, start_point_local[1] + c0, start_point_local[1] + c0 + 1, texture_style, texture);
                    if (p_f4)
                        p_f4->ThingIndex = -wall;
                }
                if (no_draw)
                    next_prim_face4--;

                if (!circular) {
                    p_f4->DrawFlags |= POLY_FLAG_DOUBLESIDED;
                }

                // All warehouses and inside-storey walls are drawn two-sided
                if (building_list[storey_list[storey].BuildingHead].BuildingType == BUILDING_TYPE_WAREHOUSE || storey_list[storey].InsideIDIndex || storey_list[storey].InsideStorey || storey_list[storey].StoreyType == STOREY_TYPE_PARTITION) {
                    if (ptexture2 && (count < tcount2) && ptexture2[count]) {
                        p_f4 = create_a_quad_tex(start_point_local[0] + c0 + 1, start_point_local[0] + c0, start_point_local[1] + c0 + 1, start_point_local[1] + c0, ptexture2[count]);
                        if (p_f4)
                            p_f4->ThingIndex = -wall;
                    } else {
                        p_f4 = create_a_quad(start_point_local[0] + c0 + 1, start_point_local[0] + c0, start_point_local[1] + c0 + 1, start_point_local[1] + c0, texture_style2, texture, 1);
                        if (p_f4)
                            p_f4->ThingIndex = -wall;
                    }
                    if (p_f4)
                        p_f4->FaceFlags |= FACE_FLAG_TEX2;
                    if (no_draw)
                        next_prim_face4--;
                }

                // Adjust UV extents for short walls to avoid texture stretching
                if (p_f4)
                    if (height < 256) {
                        if (height == 64) {
                            p_f4->UV[2][1] -= 24;
                            p_f4->UV[3][1] -= 24;
                        } else if (height == 128) {
                            p_f4->UV[2][1] -= 16;
                            p_f4->UV[3][1] -= 16;
                        } else if (height == 128 + 64) {
                            p_f4->UV[2][1] -= 8;
                            p_f4->UV[3][1] -= 8;
                        }
                    }
            }

            count++;
        }
        WindowCount = 0;
    } else {

        /*
                        start_point[0]=build_row_wall_points_at_y(y+height      ,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[1]=build_row_wall_points_at_y(y+(height*2)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[2]=build_row_wall_points_at_y(y+(height*1)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[3]=build_row_wall_points_at_y(y             ,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);

                        start_point[4]=build_row_window_depth_points_at_y(y+(height*2)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[5]=build_row_window_depth_points_at_y(y+(height*1)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);

                        for(c0=0;c0<WindowCount*2+1;c0++)
                        {
                                p_f4=create_a_quad(start_point[0]+c0,start_point[0]+c0+1,start_point[1]+c0,start_point[1]+c0+1,0,0);
                                if(p_f4)
                                        p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[2]+c0,start_point[2]+c0+1,start_point[3]+c0,start_point[3]+c0+1,0,0);
                                if(p_f4)
                                        p_f4->ThingIndex=-wall;
                                if(!(c0&1))
                                {
                                        p_f4=create_a_quad(start_point[1]+c0,start_point[1]+c0+1,start_point[2]+c0,start_point[2]+c0+1,0,0);
                                        p_f4->ThingIndex=-wall;
                                }

                        }
                        for(c0=0;c0<WindowCount;c0++)
                        {
                                p_f4=create_a_quad(start_point[1]+c0*2+1,start_point[1]+c0*2+2,start_point[4]+c0*2,start_point[4]+c0*2+1,0,0);  //lid
                                p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[1]+c0*2+1,start_point[4]+c0*2,start_point[2]+c0*2+1,start_point[5]+c0*2,0,0);  //side1
                                p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[4]+c0*2+1,start_point[1]+c0*2+2,start_point[5]+c0*2+1,start_point[2]+c0*2+2,0,0);  //side2
                                p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[5]+c0*2,start_point[5]+c0*2+1,start_point[2]+c0*2+1,start_point[2]+c0*2+2,0,0);  //base
                                p_f4->ThingIndex=-wall;
                        }
        */
    }
}

// uc_orig: find_near_prim_point (fallen/Source/Building.cpp)
// Linear search for the prim_point in [sp, ep) whose (X,Z) is closest to (x,z).
SLONG find_near_prim_point(SLONG x, SLONG z, SLONG sp, SLONG ep)
{
    SLONG best, best_dist = 0x7fffffff, dx, dz, dist, c0;

    for (c0 = sp; c0 < ep; c0++) {
        dx = (prim_points[c0].X - x);
        dz = (prim_points[c0].Z - z);
        dist = SDIST2(dx, dz);
        if (dist < best_dist) {
            best_dist = dist;
            best = c0;
        }
    }

    return (best);
}

// uc_orig: create_recessed_storey_points (fallen/Source/Building.cpp)
// Walks the wall list for a storey and emits a corner-miter offset ring of prim_points at height y.
// 'size' is the inward/outward offset: positive = outward, negative = inward.
void create_recessed_storey_points(SLONG y, SLONG storey, SLONG count, SLONG size)
{

    SLONG px, pz, index, dx, dz;
    SLONG rx, rz;
    SLONG wall;
    SLONG sp;

    count = count;
    sp = next_prim_point;

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    add_point(px, y, pz); // this gets replaced later
    index = storey_list[storey].WallHead;
    wall = index;

    while (index) {
        SLONG next;
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        next = wall_list[index].Next;
        if (next) {
            calc_new_corner_point(px, pz, dx, dz, wall_list[next].DX, wall_list[next].DZ, size, &rx, &rz);
            add_point(rx, y, rz);
        } else {
            calc_new_corner_point(px, pz, dx, dz, wall_list[wall].DX, wall_list[wall].DZ, size, &rx, &rz);
            add_point(rx, y, rz);
            // Fix up the placeholder point added at the start
            prim_points[sp].X = rx;
            prim_points[sp].Z = rz;
        }

        px = dx;
        pz = dz;
        index = wall_list[index].Next;
    }
}

// uc_orig: scan_triangle (fallen/Source/Building.cpp)
// Rasterizes a triangle (x0,y0,z0)-(x1,y1,z1)-(x2,y2,z2) into the depth-strip tile map
// by sampling at regular intervals along two edge vectors.
void scan_triangle(SLONG x0, SLONG y0, SLONG z0, SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG flag)
{

    SLONG px, py, pz;
    SLONG face_x, face_y, face_z;
    SLONG s, t, step_s, step_t;
    SLONG vx, vy, vz, wx, wy, wz;
    struct DepthStrip* me;
    SLONG len;

    face_x = x0;
    face_y = y0;
    face_z = z0;

    vx = x1 - x0;
    vy = y1 - y0; // vector from point 0 to point 1
    vz = z1 - z0;

    wx = x2 - x0; // vector from point 1 to point 2
    wy = y2 - y0;
    wz = z2 - z0;

    len = (Root(vx * vx + vy * vy + vz * vz) >> 7);
    if (len < 2)
        len = 2;
    step_s = (1 << 7) / len;
    len = (Root(wx * wx + wy * wy + wz * wz) >> 7);
    if (len < 2)
        len = 2;
    step_t = (1 << 7) / len;

    if (step_s == 0)
        step_s = 256;
    if (step_t == 0)
        step_t = 256;

    for (s = 0; s <= (1 << 7); s += step_s) {
        for (t = 0; t <= (1 << 7) - s; t += step_t) {
            px = face_x + (vx * s >> 7) + (wx * t >> 7);
            py = face_y + (vy * s >> 7) + (wy * t >> 7);
            pz = face_z + (vz * s >> 7) + (wz * t >> 7);

            if (WITHIN(px >> ELE_SHIFT, 0, MAP_WIDTH - 1) && WITHIN(pz >> ELE_SHIFT, 0, MAP_WIDTH - 1)) {
                if (build_mode == BUILD_MODE_EDITOR) {
                    me = &edit_map[px >> ELE_SHIFT][pz >> ELE_SHIFT];
                    // DepthStrip.Y is the height field (SBYTE); was .Height in an earlier pre-release struct layout.
                    if (me->Y < py) {
                        me->Y = (SBYTE)py;
                    }
                } else {
                    // MapElement no longer has a Height field in this pre-release build (was Alt, then removed).
                    // Height tracking via MAP2 is unavailable; floor heights are managed via the PAP supermap instead.
                    // Original pre-release would have written to MapElement.Height here.
                }
            }
        }
    }
}

// uc_orig: flag_floor_tiles_for_quad (fallen/Source/Building.cpp)
// Flags the floor tiles under the quad formed by prim_points[p0..p3] by rasterizing as two tris.
void flag_floor_tiles_for_quad(SLONG p0, SLONG p1, SLONG p2, SLONG p3)
{
    SLONG x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;

    x0 = prim_points[p0].X;
    y0 = prim_points[p0].Y;
    z0 = prim_points[p0].Z;

    x1 = prim_points[p1].X;
    y1 = prim_points[p1].Y;
    z1 = prim_points[p1].Z;

    x2 = prim_points[p2].X;
    y2 = prim_points[p2].Y;
    z2 = prim_points[p2].Z;

    x3 = prim_points[p3].X;
    y3 = prim_points[p3].Y;
    z3 = prim_points[p3].Z;

    scan_triangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, 0);
    scan_triangle(x1, y1, z1, x3, y3, z3, x2, y2, z2, 0);
}

// uc_orig: flag_floor_tiles_for_tri (fallen/Source/Building.cpp)
// Flags the floor tiles under the triangle formed by prim_points[p0..p2].
void flag_floor_tiles_for_tri(SLONG p0, SLONG p1, SLONG p2)
{
    SLONG x0, y0, z0, x1, y1, z1, x2, y2, z2;

    x0 = prim_points[p0].X;
    y0 = prim_points[p0].Y;
    z0 = prim_points[p0].Z;

    x1 = prim_points[p1].X;
    y1 = prim_points[p1].Y;
    z1 = prim_points[p1].Z;

    x2 = prim_points[p2].X;
    y2 = prim_points[p2].Y;
    z2 = prim_points[p2].Z;

    scan_triangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, 0);
}

// uc_orig: build_roof (fallen/Source/Building.cpp)
// Builds the roof geometry for a storey. The FLAG_STOREY_ROOF_RIM branch is locked out
// by "&&0" so it is dead in both pre-release and final. Falls through to build_roof_grid().
SLONG build_roof(UWORD storey, SLONG y, SLONG flat_flag)
{
    SLONG overlap_height = BLOCK_SIZE >> 1;

    /*
            roof=storey_list[storey].Roof;
            roof_flags=storey_list[roof].StoreyFlags;

            if(roof_flags&FLAG_ROOF_WALLED)
                    overlap_height=BLOCK_SIZE;

            if(roof_flags&FLAG_ROOF_FLAT)
                    roof_height=0;

            overlap=BLOCK_SIZE>>1;

            if(roof_flags&FLAG_ROOF_OVERLAP_SMALL)
                    overlap=BLOCK_SIZE>>1;
            else
            if(roof_flags&FLAG_ROOF_OVERLAP_MEDIUM)
                    overlap=BLOCK_SIZE;
    */

    overlap_height = 0;
    return (build_roof_grid(storey, y + overlap_height, flat_flag));
}

// uc_orig: area_of_quad (fallen/Source/Building.cpp)
// Returns width * depth of the quad's axis-aligned bounding box (X spread * Z spread).
SLONG area_of_quad(SLONG p0, SLONG p1, SLONG p2, SLONG p3)
{

    SLONG dx, dz;
    dx = abs(prim_points[p0].X - prim_points[p1].X);
    dz = abs(prim_points[p0].Z - prim_points[p2].Z);
    return (dx * dz);
}

// uc_orig: create_split_quad_into_4 (fallen/Source/Building.cpp)
// Subdivides a flat quad into a 2x2 grid of 4 quads, each added to the walkable list.
void create_split_quad_into_4(SLONG p0, SLONG p1, SLONG p2, SLONG p3, SLONG wall, SLONG y)
{

    SLONG p01, p13, p32, p20, p03;
    SLONG x, z;
    struct PrimFace4* p_f4;

    SWORD texture_style;
    texture_style = wall_list[wall].TextureStyle;

    p01 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p0].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p0].Z) >> 1;
    add_point(x, y, z);

    p13 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p3].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p3].Z) >> 1;
    add_point(x, y, z);

    p32 = next_prim_point;
    x = (prim_points[p3].X + prim_points[p2].X) >> 1;
    z = (prim_points[p3].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p20 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p2].X) >> 1;
    z = (prim_points[p0].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p03 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p1].X + prim_points[p2].X + prim_points[p3].X) >> 2;
    z = (prim_points[p0].Z + prim_points[p1].Z + prim_points[p2].Z + prim_points[p3].Z) >> 2;
    add_point(x, y, z);

    p_f4 = create_a_quad(p0, p01, p20, p03, texture_style, 0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;

    p_f4 = create_a_quad(p20, p03, p2, p32, texture_style, 0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;

    p_f4 = create_a_quad(p01, p1, p03, p13, texture_style, 0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;

    p_f4 = create_a_quad(p03, p13, p32, p3, texture_style, 0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;
}

// uc_orig: create_split_quad_into_16 (fallen/Source/Building.cpp)
// Subdivides a flat quad into 16 quads by recursively applying create_split_quad_into_4.
void create_split_quad_into_16(SLONG p0, SLONG p1, SLONG p2, SLONG p3, SLONG wall, SLONG y)
{
    SLONG p01, p13, p32, p20, p03;
    SLONG x, z;

    p01 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p0].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p0].Z) >> 1;
    add_point(x, y, z);

    p13 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p3].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p3].Z) >> 1;
    add_point(x, y, z);

    p32 = next_prim_point;
    x = (prim_points[p3].X + prim_points[p2].X) >> 1;
    z = (prim_points[p3].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p20 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p2].X) >> 1;
    z = (prim_points[p0].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p03 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p1].X + prim_points[p2].X + prim_points[p3].X) >> 2;
    z = (prim_points[p0].Z + prim_points[p1].Z + prim_points[p2].Z + prim_points[p3].Z) >> 2;
    add_point(x, y, z);

    create_split_quad_into_4(p0, p01, p20, p03, wall, y);
    create_split_quad_into_4(p20, p03, p2, p32, wall, y);
    create_split_quad_into_4(p01, p1, p03, p13, wall, y);
    create_split_quad_into_4(p03, p13, p32, p3, wall, y);
}

// uc_orig: create_split_quad_into_48 (fallen/Source/Building.cpp)
// Subdivides a flat quad into 48 quads by recursively applying create_split_quad_into_16.
void create_split_quad_into_48(SLONG p0, SLONG p1, SLONG p2, SLONG p3, SLONG wall, SLONG y)
{
    SLONG p01, p13, p32, p20, p03;
    SLONG x, z;

    p01 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p0].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p0].Z) >> 1;
    add_point(x, y, z);

    p13 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p3].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p3].Z) >> 1;
    add_point(x, y, z);

    p32 = next_prim_point;
    x = (prim_points[p3].X + prim_points[p2].X) >> 1;
    z = (prim_points[p3].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p20 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p2].X) >> 1;
    z = (prim_points[p0].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p03 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p1].X + prim_points[p2].X + prim_points[p3].X) >> 2;
    z = (prim_points[p0].Z + prim_points[p1].Z + prim_points[p2].Z + prim_points[p3].Z) >> 2;
    add_point(x, y, z);

    create_split_quad_into_16(p0, p01, p20, p03, wall, y);
    create_split_quad_into_16(p20, p03, p2, p32, wall, y);
    create_split_quad_into_16(p01, p1, p03, p13, wall, y);
    create_split_quad_into_16(p03, p13, p32, p3, wall, y);
}

// uc_orig: build_roof_quad (fallen/Source/Building.cpp)
// Builds a single flat-top quad (or subdivided quad) for a storey's roof.
// The main roof-building branch (storey_list[storey].WallHead && 0) is dead in the original.
void build_roof_quad(UWORD storey, SLONG y)
{
    SLONG wall;
    SLONG roof;
    SLONG p0, p1, p2, p3;
    SLONG count = 0;
    SLONG roof_height = 0;
    struct PrimFace4* p_f4;
    SWORD texture_style;

    if (storey_list[storey].WallHead && 0)
        ;
    {
        SLONG area;
        SLONG npp;

        npp = next_prim_point;

        roof = 0;
        start_point[0] = next_prim_point;
        wall = storey_list[roof].WallHead;
        if (wall_list[wall].WallFlags & FLAG_ROOF_FLAT)
            roof_height = 0;

        if (wall)
            add_point(storey_list[roof].DX, y - roof_height, storey_list[roof].DZ);
        while (wall && count < 3) {

            add_point(wall_list[wall].DX, y - roof_height, wall_list[wall].DZ);
            wall = wall_list[wall].Next;
            count++;
        }

        if (count < 3) {
            next_prim_point = npp;
            return;
        }

        wall = storey_list[roof].WallHead;
        texture_style = wall_list[wall].TextureStyle;
        p0 = start_point[0] + 3;
        p1 = start_point[0] + 2;
        p2 = start_point[0] + 0;
        p3 = start_point[0] + 1;
        area = area_of_quad(p0, p1, p2, p3);
        if (area > 8 * 256 * 8 * 256)
            create_split_quad_into_48(p0, p1, p2, p3, wall, y + roof_height);
        else if (area > 4 * 256 * 4 * 256)
            create_split_quad_into_16(p0, p1, p2, p3, wall, y + roof_height);
        else if (area > 2 * 256 * 2 * 256)
            create_split_quad_into_4(p0, p1, p2, p3, wall, y + roof_height);
        else {
            p_f4 = create_a_quad(p0, p1, p2, p3, texture_style, 0);
            add_quad_to_walkable_list(next_prim_face4 - 1);
            p_f4->ThingIndex = -wall;
        }
    }
}

// uc_orig: center_object (fallen/Source/Building.cpp)
// Computes the centroid of prim_points[sp..ep) and stores it in build_x, build_z.
void center_object(SLONG sp, SLONG ep)
{
    SLONG c0;
    SLONG az = 0, ax = 0;
    if (ep - sp < 0) {
        return;
    }
    if ((ep - sp) == 0) {
        return;
    }

    for (c0 = sp; c0 < ep; c0++) {
        ax += prim_points[c0].X;
        az += prim_points[c0].Z;
    }

    ax /= (ep - sp);
    az /= (ep - sp);
    build_x = ax;
    build_z = az;
    build_y = 0;
}

// uc_orig: center_object_about_xz (fallen/Source/Building.cpp)
// Stores the explicit (x,z) as the current building centroid.
void center_object_about_xz(SLONG sp, SLONG ep, SLONG x, SLONG z)
{
    if (ep - sp < 0) {
        return;
    }
    if ((ep - sp) == 0) {
        return;
    }

    build_x = x;
    build_z = z;
    build_y = 0;
}

// uc_orig: build_facet (fallen/Source/Building.cpp)
// Fills in the next building_facets[] slot with the current prim state.
// Verifies that all face4 point indices are valid before recording.
SLONG build_facet(SLONG sp, SLONG mp, SLONG sf3, SLONG sf4, SLONG mf4, SLONG prev_facet, UWORD flags, UWORD col_vect)
{
    struct BuildingFacet* p_obj;
    p_obj = &building_facets[next_building_facet];
    next_building_facet++;

    SLONG c0;
    for (c0 = sf4; c0 < next_prim_face4; c0++) {
        ASSERT(prim_faces4[c0].Points[0] < next_prim_point);
        ASSERT(prim_faces4[c0].Points[1] < next_prim_point);
        ASSERT(prim_faces4[c0].Points[2] < next_prim_point);
        ASSERT(prim_faces4[c0].Points[3] < next_prim_point);
    }

    p_obj->StartPoint = sp;
    p_obj->MidPoint = mp;
    p_obj->EndPoint = next_prim_point;

    ASSERT(sf4 >= 0);
    ASSERT(sf4 <= 65535);
    p_obj->StartFace4 = sf4;
    p_obj->MidFace4 = mf4;
    p_obj->EndFace4 = next_prim_face4;
    p_obj->StartFace3 = sf3;
    p_obj->EndFace3 = next_prim_face3;
    p_obj->NextFacet = prev_facet;
    p_obj->FacetFlags = flags;
    p_obj->ColVect = col_vect;

    ASSERT(p_obj->StartFace4 >= 0);
    ASSERT(p_obj->StartFace3 >= 0);
    ASSERT(p_obj->EndFace4 >= 0);
    ASSERT(p_obj->EndFace3 >= 0);

    return (next_building_facet - 1);
}

// uc_orig: build_building (fallen/Source/Building.cpp)
// Fills in the next building_objects[] slot from the current prim state.
// The centroid is computed from the point range via center_object().
SLONG build_building(SLONG sp, SLONG sf3, SLONG sf4, SLONG prev_facet)
{
    struct BuildingObject* p_bobj;
    p_bobj = &building_objects[next_building_object];
    next_building_object++;

    p_bobj->StartPoint = sp;
    p_bobj->EndPoint = next_prim_point;

    p_bobj->StartFace4 = sf4;
    p_bobj->EndFace4 = next_prim_face4;

    p_bobj->StartFace3 = sf3;
    p_bobj->EndFace3 = next_prim_face3;
    p_bobj->FacetHead = prev_facet;
    center_object(p_bobj->StartPoint, p_bobj->EndPoint);

    return (next_building_object - 1);
}

// uc_orig: build_building2 (fallen/Source/Building.cpp)
// Fills in the next building_objects[] slot; uses explicit (cx, cz) centroid.
SLONG build_building2(SLONG sp, SLONG sf3, SLONG sf4, SLONG prev_facet, SLONG cx, SLONG cz)
{
    struct BuildingObject* p_bobj;
    p_bobj = &building_objects[next_building_object];
    next_building_object++;

    p_bobj->StartPoint = sp;
    p_bobj->EndPoint = next_prim_point;

    p_bobj->StartFace4 = sf4;
    p_bobj->EndFace4 = next_prim_face4;

    p_bobj->StartFace3 = sf3;
    p_bobj->EndFace3 = next_prim_face3;
    p_bobj->FacetHead = prev_facet;
    center_object_about_xz(p_bobj->StartPoint, p_bobj->EndPoint, cx, cz);

    return (next_building_object - 1);
}

// uc_orig: build_prim_object (fallen/Source/Building.cpp)
// Fills in the next prim_objects[] slot from the current prim state.
SLONG build_prim_object(SLONG sp, SLONG sf3, SLONG sf4)
{
    struct PrimObject* p_obj;
    p_obj = &prim_objects[next_prim_object];
    next_prim_object++;

    p_obj->StartPoint = sp;
    p_obj->EndPoint = next_prim_point;

    p_obj->StartFace4 = sf4;
    p_obj->EndFace4 = next_prim_face4;

    p_obj->StartFace3 = sf3;
    p_obj->EndFace3 = next_prim_face3;
    center_object(p_obj->StartPoint, p_obj->EndPoint);
    return (next_prim_object - 1);
}

// uc_orig: find_next_last_coord (fallen/Source/Building.cpp)
// Walks the wall chain and writes the endpoint of the second-to-last wall to (*x, *z).
void find_next_last_coord(SWORD wall, SLONG* x, SLONG* z)
{
    SLONG next_wall;
    while (wall) {
        next_wall = wall_list[wall].Next;
        if (wall_list[next_wall].Next == 0) {
            *x = wall_list[wall].DX;
            *z = wall_list[wall].DZ;
            return;
        }
        wall = next_wall;
    }
}

// uc_orig: build_single_ledge (fallen/Source/Building.cpp)
// Emits three quad faces that form a single ledge strip between corners X2/Z2 and X3/Z3.
void build_single_ledge(struct LedgeInfo* p_ledge)
{

    SLONG sp[4];
    SLONG rx, rz, rx2, rz2;

    SLONG y, height;
    struct PrimFace4* p4;

    height = BLOCK_SIZE >> 1;
    y = p_ledge->Y;

    sp[0] = next_prim_point;
    add_point(p_ledge->X2, y + 2, p_ledge->Z2);
    add_point(p_ledge->X3, y + 2, p_ledge->Z3);
    calc_new_corner_point(p_ledge->X1, p_ledge->Z1, p_ledge->X2, p_ledge->Z2, p_ledge->X3, p_ledge->Z3, BLOCK_SIZE, &rx, &rz);
    calc_new_corner_point(p_ledge->X2, p_ledge->Z2, p_ledge->X3, p_ledge->Z3, p_ledge->X4, p_ledge->Z4, BLOCK_SIZE, &rx2, &rz2);
    sp[1] = next_prim_point;
    add_point(rx, y + 2, rz);
    add_point(rx2, y + 2, rz2);

    y += height;
    sp[2] = next_prim_point;
    add_point(rx, y + 2, rz);
    add_point(rx2, y + 2, rz2);

    sp[3] = next_prim_point;
    add_point(p_ledge->X2, y + 2, p_ledge->Z2);
    add_point(p_ledge->X3, y + 2, p_ledge->Z3);

    p4 = create_a_quad(sp[1], sp[1] + 1, sp[0], sp[0] + 1, 0, 18);
    OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_LONG_LEDGE);
    p4 = create_a_quad(sp[2], sp[2] + 1, sp[1], sp[1] + 1, 0, 18);
    OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_LONG_LEDGE);
    p4 = create_a_quad(sp[3], sp[3] + 1, sp[2], sp[2] + 1, 0, 18);
    OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_LONG_LEDGE);
}

// uc_orig: dist_between_vertex_and_vector (fallen/Source/Building.cpp)
// Computes approximate distance from the midpoint of edge (x1,y1)-(x2,y2) to point (px,py).
SLONG dist_between_vertex_and_vector(SLONG x1, SLONG y1, SLONG x2, SLONG y2, SLONG px, SLONG py)
{
    SLONG dist, dx, dy;

    x1 = (x1 + x2) >> 1;
    y1 = (y1 + y2) >> 1;

    dx = abs(x1 - px);
    dy = abs(y1 - py);

    dist = QDIST2(dx, dy);
    return (dist);
}

// uc_orig: find_wall_for_fe (fallen/Source/Building.cpp)
// Finds the 0-based wall index closest to (fe_x, fe_y) in the storey's wall list.
// Skips ladder/fire-escape/staircase/fence/outside-door storeys to reach a building storey.
SLONG find_wall_for_fe(SLONG fe_x, SLONG fe_y, SLONG storey)
{
    SLONG wall = 0;
    SLONG px, pz, x1, z1;
    SLONG best_wall = -1, best_dist = 0x7fffffff, dist;
    SLONG wall_count = 0;

    while (storey_list[storey].StoreyType == STOREY_TYPE_LADDER || storey_list[storey].StoreyType == STOREY_TYPE_FIRE_ESCAPE || storey_list[storey].StoreyType == STOREY_TYPE_STAIRCASE || storey_list[storey].StoreyType == STOREY_TYPE_FENCE_BRICK || storey_list[storey].StoreyType == STOREY_TYPE_FENCE_FLAT || storey_list[storey].StoreyType == STOREY_TYPE_OUTSIDE_DOOR || storey_list[storey].StoreyType == STOREY_TYPE_FENCE) {
        storey = storey_list[storey].Next;
    }
    wall = storey_list[storey].WallHead;
    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    while (wall) {
        x1 = wall_list[wall].DX;
        z1 = wall_list[wall].DZ;

        dist = dist_between_vertex_and_vector(px, pz, x1, z1, fe_x, fe_y);
        if (dist < best_dist) {
            best_wall = wall_count;
            best_dist = dist;
        }

        wall_count++;
        wall = wall_list[wall].Next;
        px = x1;
        pz = z1;
    }
    return (best_wall);
}

// uc_orig: build_staircase (fallen/Source/Building.cpp)
// Generates staircase geometry for the given storey. Emits step treads, risers, and side panels.
// The staircase direction is determined from the wall vectors: the first (wall_count/2) walls define
// one edge, the second half define the opposite edge in reverse.
void build_staircase(SLONG storey)
{
    SLONG wall;
    SLONG wall_count = 0;
    SLONG count;
    SLONG step_count, step_size, step_height, len, step_pos, step_length;
    SLONG row = 0;
    SLONG c0, c1;
    SLONG y, start_y = 0;
    struct PrimFace4* p4;
    SLONG step_pos_old;
    SLONG last;
    SLONG x1, z1;

    struct StairVect {
        SLONG X1, Z1, X2, Z2;
    };

    struct StairVect s_vects[50];

    wall = storey_list[storey].WallHead;
    x1 = storey_list[storey].DX;
    z1 = storey_list[storey].DZ;
    while (wall) {
        SLONG x2, z2;
        wall_count++;
        x2 = wall_list[wall].DX;
        z2 = wall_list[wall].DZ;
        insert_collision_vect(x1, start_y, z1, x2, start_y, z2, STOREY_TYPE_STAIRCASE, 0, -wall);
        set_vect_floor_height(x1, z1, x2, z2, (start_y + build_min_y) >> ALT_SHIFT);
        wall = wall_list[wall].Next;
        x1 = x2;
        z1 = z2;
    }

    start_y += build_min_y;

    if ((wall_count < 4) || (wall_count & 1))
        return;

    count = 1;
    wall = storey_list[storey].WallHead;
    while (wall) {
        if (count <= (wall_count >> 1)) {
            s_vects[count - 1].X1 = wall_list[wall].DX;
            s_vects[count - 1].Z1 = wall_list[wall].DZ;

        } else {
            SLONG pos;
            pos = (wall_count) - (count) + 1;

            s_vects[pos - 1].X2 = wall_list[wall].DX;
            s_vects[pos - 1].Z2 = wall_list[wall].DZ;
        }

        count++;
        wall = wall_list[wall].Next;
    }

    {
        SLONG dx, dz;

        dx = abs(s_vects[0].X1 - s_vects[0].X2);
        dz = abs(s_vects[0].Z1 - s_vects[0].Z2);
        len = QDIST2(dx, dz);
    }

    if (len == 0)
        return;

    step_height = BLOCK_SIZE >> 1;
    step_length = BLOCK_SIZE >> 1;
    step_count = len / step_length;
    if (step_count == 0)
        return;
    step_size = (len << 8) / step_count;

    step_pos = len << 8;
    y = start_y;

    step_pos_old = step_pos;
    sp_stairs[99] = next_prim_point;
    while (step_pos >= 0) {
        SLONG x, z;
        if (storey_list[storey].Info1 && step_pos < storey_list[storey].Info1 * step_size) {
            step_pos = 0;
        }
        last = (wall_count >> 1) - 1;
        x = s_vects[0].X1 + ((s_vects[0].X2 - s_vects[0].X1) * step_pos) / (len << 8);
        z = s_vects[0].Z1 + ((s_vects[0].Z2 - s_vects[0].Z1) * step_pos) / (len << 8);
        add_point(x, start_y, z);
        x = s_vects[last].X1 + ((s_vects[last].X2 - s_vects[last].X1) * step_pos) / (len << 8);
        z = s_vects[last].Z1 + ((s_vects[last].Z2 - s_vects[last].Z1) * step_pos) / (len << 8);
        add_point(x, start_y, z);
        step_pos -= step_size;
    }
    step_pos = step_pos_old;

    while (step_pos >= 0) {

        if (storey_list[storey].Info1 && step_pos < storey_list[storey].Info1 * step_size) {
            step_pos = 0;
            /*
                                    sp_stairs[row]=next_prim_point;
                                    for(c0=0;c0<(wall_count>>1);c0++)
                                    {
                                            SLONG	x,z;

                                            x=s_vects[c0].X1;
                                            z=s_vects[c0].Z1;

                                            add_point(x,y,z);
                                    }
                                    row++;
                                    step_pos=-1;
            */
        }
        {

            sp_stairs[row] = next_prim_point;
            for (c0 = 0; c0 < (wall_count >> 1); c0++) {
                SLONG x, z;
                x = s_vects[c0].X1 + ((s_vects[c0].X2 - s_vects[c0].X1) * step_pos) / (len << 8);
                z = s_vects[c0].Z1 + ((s_vects[c0].Z2 - s_vects[c0].Z1) * step_pos) / (len << 8);
                add_point(x, y, z);
            }
            sp_stairs[row + 1] = next_prim_point;
            step_pos -= step_size;
            if (step_pos >= 0) {
                for (c0 = sp_stairs[row]; c0 < sp_stairs[row] + (wall_count >> 1); c0++) {
                    add_point(prim_points[c0].X, y + step_height, prim_points[c0].Z);
                }

                y += step_height;
                row += 2;
            } else {

                y += step_height;
                row += 1;
            }
        }
    }

    wall = storey_list[storey].WallHead;

    for (c1 = 0; c1 < (row - 1); c1++) {
        for (c0 = 0; c0 < ((wall_count >> 1) - 1); c0++) {
            p4 = create_a_quad(sp_stairs[c1 + 1] + 1 + c0, sp_stairs[c1 + 1] + c0, sp_stairs[c1] + 1 + c0, sp_stairs[c1] + c0, 0, 18 + ((c1 & 1) ? 5 : 0));
            p4->ThingIndex = -wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            add_quad_to_walkable_list(next_prim_face4 - 1);
        }
        if (c1 & 1) {
            p4 = create_a_quad(sp_stairs[c1], sp_stairs[c1 + 1], sp_stairs[99] + ((c1 - 1) & 0xfffe), sp_stairs[99] + 2 + ((c1 - 1) & 0xfffe), 0, 23);
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(sp_stairs[c1 + 1] + last, sp_stairs[c1] + last, sp_stairs[99] + 3 + ((c1 - 1) & 0xfffe), sp_stairs[99] + 1 + ((c1 - 1) & 0xfffe), 0, 23);
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }
    }
}

// uc_orig: get_wall_start_and_end (fallen/Source/Building.cpp)
// Walks the wall list of the storey owning 'want_wall' and returns the XZ start and end coordinates.
void get_wall_start_and_end(SLONG want_wall, SLONG* x1, SLONG* z1, SLONG* x2, SLONG* z2)
{
    SLONG wall;

    FWall* p_wall;
    FStorey* p_storey;

    p_storey = &storey_list[wall_list[want_wall].StoreyHead];

    *x1 = p_storey->DX;
    *z1 = p_storey->DZ;

    wall = p_storey->WallHead;

    while (wall) {
        p_wall = &wall_list[wall];

        *x2 = p_wall->DX;
        *z2 = p_wall->DZ;

        if (wall == want_wall) {
            return;
        }

        *x1 = *x2;
        *z1 = *z2;

        wall = p_wall->Next;
    }

    // The wall wasn't found in its own storey — data corruption
    ASSERT(0);
}

// ============================================================
// Chunk 6: cable, fence, brick wall, storey setup
// ============================================================

// uc_orig: make_cable_taut_along (fallen/Source/Building.cpp)
// Deforms a cable building's point array into a taut triangle shape as if someone
// is hanging at 'along' (0–256) percent of the straight-line distance.
// Returns the world-space hanging point in (x_middle, y_middle, z_middle).
void make_cable_taut_along(SLONG along, SLONG building, SLONG* x_middle, SLONG* y_middle, SLONG* z_middle)
{
    SLONG i;
    SLONG x, y, z;
    SLONG dx, dy, dz;
    SLONG h;
    SLONG V, v1, v2;
    SLONG L, l1;
    SLONG x1, y1, z1;
    SLONG x2, y2, z2;
    SLONG xm, ym, zm;
    SLONG middle;
    SLONG num_points;
    SLONG pstart, pend;
    SLONG p1, p2;
    BuildingFacet* bf;

    bf = &building_facets[building_objects[building].FacetHead];

    if (!(bf->FacetFlags & FACET_FLAG_CABLE))
        return;

    pstart = bf->StartPoint;
    pend = bf->EndPoint - 2;

    x1 = prim_points[pstart].X;
    y1 = prim_points[pstart].Y;
    z1 = prim_points[pstart].Z;

    x2 = prim_points[pend].X;
    y2 = prim_points[pend].Y;
    z2 = prim_points[pend].Z;

    L = 0;
    for (i = bf->StartPoint; i + 2 < bf->EndPoint; i += 2) {
        p1 = i;
        p2 = i + 2;
        dx = abs(prim_points[p2].X - prim_points[p1].X);
        dy = abs(prim_points[p2].Y - prim_points[p1].Y);
        dz = abs(prim_points[p2].Z - prim_points[p1].Z);
        L += QDIST3(dx, dy, dz);
    }
    L -= L >> 2;

    ASSERT(L > 0);

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    dz = abs(z2 - z1);
    V = QDIST3(dx, dy, dz);
    L = V + 50;

    v1 = V * along >> 8;
    v2 = V - v1;
    l1 = (L * L + v1 * v1 - v2 * v2) / (2 * L);
    h = Root(l1 * l1 - v1 * v1);

    num_points = bf->EndPoint - bf->StartPoint >> 1;
    ASSERT(num_points >= 3);

    xm = x1 + ((x2 - x1) * along >> 8);
    ym = y1 + ((y2 - y1) * along >> 8);
    zm = z1 + ((z2 - z1) * along >> 8);
    ym -= h;

    middle = num_points * along >> 8;
    if (middle == 0)
        middle = 1;
    if (middle == num_points - 1)
        middle -= 1;

    dx = (xm - x1) / middle;
    dy = (ym - y1) / middle;
    dz = (zm - z1) / middle;

    x = x1;
    y = y1;
    z = z1;

    for (i = 0; i < num_points; i++) {
        if (i == middle) {
            dx = (x2 - xm) / (num_points - middle - 1);
            dy = (y2 - ym) / (num_points - middle - 1);
            dz = (z2 - zm) / (num_points - middle - 1);
            x = xm;
            y = ym;
            z = zm;
        }
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 0].X = x;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 0].Y = y;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 0].Z = z;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 1].X = x;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 1].Y = y + 8;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 1].Z = z;
        x += dx;
        y += dy;
        z += dz;
    }

    *x_middle = xm;
    *y_middle = ym;
    *z_middle = zm;
}

// uc_orig: make_cable_flabby (fallen/Source/Building.cpp)
// Restores a cable building's float point buffer (AENG_dx_prim_points) from the
// integer ground-truth (prim_points), undoing any taut deformation.
void make_cable_flabby(SLONG building)
{
    SLONG i;
    BuildingFacet* bf = &building_facets[building_objects[building].FacetHead];

    for (i = bf->StartPoint; i < bf->EndPoint; i++) {
        AENG_dx_prim_points[i].X = float(prim_points[i].X);
        AENG_dx_prim_points[i].Y = float(prim_points[i].Y);
        AENG_dx_prim_points[i].Z = float(prim_points[i].Z);
    }
}

// File-local macros for create_suspended_light.
// uc_orig: LIGHT_SIZE (fallen/Source/Building.cpp)
#define LIGHT_SIZE BLOCK_SIZE
// uc_orig: CONE_MULT (fallen/Source/Building.cpp)
#define CONE_MULT 5

// uc_orig: create_suspended_light (fallen/Source/Building.cpp)
// Builds a tiny pyramidal prim object for a hanging light fitting.
// The larger cone geometry is commented out in the original (unfinished feature).
SLONG create_suspended_light(SLONG x, SLONG y, SLONG z, SLONG flags)
{
    SLONG p1;
    struct PrimFace3* p_f3;

    flags = flags;
    p1 = next_prim_point;
    add_point(x, y, z);
    add_point(x - LIGHT_SIZE, y - LIGHT_SIZE, z - LIGHT_SIZE);
    add_point(x + LIGHT_SIZE, y - LIGHT_SIZE, z - LIGHT_SIZE);
    add_point(x + LIGHT_SIZE, y - LIGHT_SIZE, z + LIGHT_SIZE);
    add_point(x - LIGHT_SIZE, y - LIGHT_SIZE, z + LIGHT_SIZE);

    p_f3 = create_a_tri(p1 + 2, p1 + 1, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0;
    p_f3 = create_a_tri(p1 + 3, p1 + 2, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0;
    p_f3 = create_a_tri(p1 + 4, p1 + 3, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0;
    p_f3 = create_a_tri(p1 + 1, p1 + 4, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0;

    return (0);
}

// uc_orig: build_cable (fallen/Source/Building.cpp)
// Builds a sagging cable between two world points as a quad-strip in the prim buffer.
// 'saggysize' controls the depth of the sag arc; 'wall' is recorded on each face.
void build_cable(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SWORD wall, SWORD type, SLONG saggysize)
{
    SLONG p1;
    struct PrimFace4* p_f4;
    SLONG len, dx, dy, dz, count;
    SLONG px, py, pz;
    SLONG c0;
    SLONG step_angle1, step_angle2, angle;

    wall = wall;
    type = type;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    dz = abs(z2 - z1);
    len = QDIST3(dx, dy, dz);
    count = (len << 1) / ELE_SIZE;
    dx = (x2 - x1);
    dy = (y2 - y1);
    dz = (z2 - z1);

    px = x1;
    py = y1;
    pz = z1;
    add_point(px, py, pz);
    add_point(px, py + 8, pz);

    if (dy == 0) {
        step_angle1 = 1024 / count;
        step_angle2 = -step_angle1;
    } else {
        SLONG c1, c2;
        SLONG m;
        SLONG d1, d2;
        if (len == 0)
            len = 1;
        m = (abs(dy) * 190) / len;
        c1 = 128;
        c2 = 128;
        if (dy < 0) {
            c1 = c1 + m;
            c2 = c2 - m;
        } else {
            c1 = c1 - m;
            c2 = c2 + m;
        }
        if (c1 < 0) c1 = 0;
        if (c1 > 256) c1 = 256;
        if (c2 < 0) c2 = 0;
        if (c2 > 256) c2 = 256;
        d1 = ((count * c1) >> 8);
        d2 = ((count * c2) >> 8);
        if (d1 == 0) d1 = 1;
        if (d2 == 0) d2 = 1;
        step_angle1 = 512 / d1;
        step_angle2 = -512 / d2;
    }
    angle = -512;
    for (c0 = 1; c0 <= count; c0++) {
        SLONG ex, ey, ez;
        angle += step_angle1;
        if (angle >= -30) {
            step_angle1 = step_angle2;
        }
        ex = x1 + (c0 * dx) / count;
        ey = y1 + (c0 * dy) / count;
        ey -= (COS((angle + 2048) & 2047) * saggysize) >> 16;
        ez = z1 + (c0 * dz) / count;
        p1 = next_prim_point;
        add_point(ex, ey, ez);
        add_point(ex, ey + 8, ez);
        p_f4 = create_a_quad(p1 - 1, p1 + 1, p1 - 2, p1, 0, 0);
        p_f4->DrawFlags = POLY_FLAG_DOUBLESIDED;
        p_f4->Type = FACE_TYPE_CABLE;
        p_f4->ThingIndex = -wall;
        add_quad_to_walkable_list(next_prim_face4 - 1);
        p_f4->FaceFlags &= ~FACE_FLAG_WALKABLE;
        px = ex;
        py = ey;
        pz = ez;
    }
}

// uc_orig: build_cable_old (fallen/Source/Building.cpp)
// Older cable builder using a symmetric cosine sag (simpler than build_cable).
// No saggysize parameter; sag depth is hardcoded from cos curve.
void build_cable_old(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SWORD wall, SWORD type)
{
    SLONG p1;
    struct PrimFace4* p_f4;
    SLONG len, dx, dy, dz, count;
    SLONG px, py, pz;
    SLONG c0;

    wall = wall;
    type = type;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    dz = abs(z2 - z1);
    len = QDIST3(dx, dy, dz);
    count = (len << 1) / ELE_SIZE;
    dx = (x2 - x1);
    dy = (y2 - y1);
    dz = (z2 - z1);

    px = x1;
    py = y1;
    pz = z1;
    add_point(px, py, pz);
    add_point(px, py + 8, pz);

    for (c0 = 1; c0 <= count; c0++) {
        SLONG ex, ey, ez;
        SLONG angle;
        ex = x1 + (c0 * dx) / count;
        ey = y1 + (c0 * dy) / count;
        angle = ((c0 - (count >> 1)) * 1024) / count;
        angle = (angle + 2048) & 2047;
        ey -= COS(angle) >> 9;
        ez = z1 + (c0 * dz) / count;
        p1 = next_prim_point;
        add_point(ex, ey, ez);
        add_point(ex, ey + 8, ez);
        p_f4 = create_a_quad(p1 - 1, p1 + 1, p1 - 2, p1, 0, 0);
        p_f4->DrawFlags = POLY_FLAG_DOUBLESIDED;
        p_f4->Type = FACE_TYPE_CABLE;
        p_f4->ThingIndex = -wall;
        add_quad_to_walkable_list(next_prim_face4 - 1);
        px = ex;
        py = ey;
        pz = ez;
    }
}

// uc_orig: build_cables (fallen/Source/Building.cpp)
// Iterates the wall list of a STOREY_TYPE_CABLE storey to build all cable segments.
// Creates one BuildingObject per segment and registers a sky-height collision vector.
SLONG build_cables(SWORD storey, SLONG prev_facet)
{
    SLONG wall;
    SLONG x1, y1, z1, x2, y2, z2;
    SLONG start_point, start_face3, start_face4;
    SLONG prim;
    SLONG building;

    wall = storey_list[storey].WallHead;
    x1 = storey_list[storey].DX;
    y1 = storey_list[storey].DY;
    z1 = storey_list[storey].DZ;
    y1 += PAP_calc_height_at(x1, z1);

    while (wall) {
        x2 = wall_list[wall].DX;
        y2 = wall_list[wall].DY;
        z2 = wall_list[wall].DZ;
        y2 += PAP_calc_height_at(x2, z2);

        start_point = next_prim_point;
        start_face3 = next_prim_face3;
        start_face4 = next_prim_face4;

        build_cable(x1, y1, z1, x2, y2, z2, wall, 0, wall_list[wall].TextureStyle2 * 64);
        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, 0, FACET_FLAG_CABLE, 0);
        prim = build_building2(start_point, start_face3, start_face4, prev_facet, storey_list[storey].DX, storey_list[storey].DZ);
        building = storey_list[storey].BuildingHead;

        storey_list[storey].Info1 = prim;
        THING_INDEX new_thing = place_building_at(building, prim, build_x, 0, build_z);
        insert_collision_vect(x1, y1 + 5000, z1, x2, y2 + 5000, z2, STOREY_TYPE_CABLE, 1, new_thing);

        if (new_thing) {
            TO_THING(new_thing)->Flags |= FLAGS_CABLE_BUILDING;
        }

        wall = wall_list[wall].Next;
        x1 = x2;
        y1 = y2;
        z1 = z2;
    }
    return (0);
}

// uc_orig: build_fence_points_and_faces (fallen/Source/Building.cpp)
// Builds a chain-link fence quad-strip between two endpoints.
// 'posts' controls whether post geometry is included (unused — always 1 in callers).
void build_fence_points_and_faces(SLONG y1, SLONG y2, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall, UBYTE posts)
{
    SLONG wcount, wwidth, dx, dz, dist;
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;
    SLONG px, pz;
    SLONG ya, yb;

    ya = PAP_calc_height_at(x1, z1);
    yb = PAP_calc_height_at(x2, z2);

    insert_collision_vect(x1, ya, z1, x2, yb, z2, STOREY_TYPE_FENCE, 4, -wall);
    insert_collision_vect(x2, yb, z2, x1, ya, z1, STOREY_TYPE_FENCE, 4, -wall);

    texture_style = wall_list[wall].TextureStyle;
    texture = TEXTURE_PIECE_MIDDLE;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);
    dist = Root(SDIST2(dx, dz));
    if (dist == 0)
        return;

    wcount = (dist / (BLOCK_SIZE * 4));
    if (wcount == 0)
        wcount = 1;
    wwidth = dist / (wcount);

    dx = (x2 - x1);
    dz = (z2 - z1);
    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;
    px = ((dz * (10)) >> 10);
    pz = -((dx * (10)) >> 10);

    while (wcount) {
        SLONG p, p1;
        SLONG floor_1, floor_2;
        p = next_prim_point;
        floor_1 = PAP_calc_height_at(x1, z1);
        add_point(x1, y2 + floor_1, z1);
        add_point(x1, y1 + floor_1, z1);
        add_point(x1 + px * 5, y2 + 45 + floor_1, z1 + pz * 5);
        add_point(x1 + px * 1, y2 + 10 + floor_1, z1 + pz * 1);
        x1 = x1 + ((dx * (wwidth - 20)) >> 10);
        z1 = z1 + ((dz * (wwidth - 20)) >> 10);
        floor_2 = PAP_calc_height_at(x1, z1);
        p1 = next_prim_point;
        add_point(x1, y2 + floor_2, z1);
        add_point(x1, y1 + floor_2, z1);
        add_point(x1 + px * 4, y2 + 45 + floor_2, z1 + pz * 4);
        add_point(x1 + px * 1, y2 + 10 + floor_2, z1 + pz * 1);
        p_f4 = create_a_quad(p, p1 + 0, p + 1, p1 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);
        p_f4 = create_a_quad(p + 2, p1 + 2, p + 3, p1 + 3, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);

        p = next_prim_point;
        add_point(x1 + px, y2 + floor_2, z1 + pz);
        add_point(x1 + px, y1 + floor_2, z1 + pz);
        add_point(x1 - px, y2 + floor_2, z1 - pz);
        add_point(x1 - px, y1 + floor_2, z1 - pz);
        add_point(x1 + px * 6, y2 + 50 + floor_2, z1 + pz * 6);
        add_point(x1 + px * 4, y2 + 50 + floor_2, z1 + pz * 4);

        x1 = x1 + ((dx * (20)) >> 10);
        z1 = z1 + ((dz * (20)) >> 10);

        p1 = next_prim_point;
        add_point(x1 + px, y2 + floor_2, z1 + pz);
        add_point(x1 + px, y1 + floor_2, z1 + pz);
        add_point(x1 - px, y2 + floor_2, z1 - pz);
        add_point(x1 - px, y1 + floor_2, z1 - pz);
        add_point(x1 + px * 6, y2 + 50 + floor_2, z1 + pz * 6);
        add_point(x1 + px * 4, y2 + 50 + floor_2, z1 + pz * 4);

        p_f4 = create_a_quad(p, p1, p + 1, p1 + 1, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p1 + 2, p + 2, p1 + 3, p + 3, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p + 2, p + 0, p + 3, p + 1, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p1, p1 + 2, p1 + 1, p1 + 3, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p + 4, p1 + 4, p, p1, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p1 + 5, p + 5, p1 + 2, p + 2, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p1 + 4, p1 + 5, p1, p1 + 2, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p + 5, p + 4, p + 2, p, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(p + 4, p + 5, p1 + 4, p1 + 5, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        wcount--;
    }
}

// uc_orig: build_high_chain_fence (fallen/Source/Building.cpp)
// Builds a two-panel high chain-link fence (two rows of fence quads) with collision vectors.
void build_high_chain_fence(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height, UBYTE alt_mode)
{
    SLONG c0;
    SLONG sp[10];
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    if (alt_mode == 1) {
        sp[0] = build_row_wall_only_points_at_floor_alt(height * 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_floor_alt(height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[2] = build_row_wall_only_points_at_floor_alt(0, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    } else {
        sp[0] = build_row_wall_only_points_at_y(y + height * 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_y(y + height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[2] = build_row_wall_only_points_at_y(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    }

    for (c0 = 0; c0 < WindowCount; c0++) {
        texture = TEXTURE_PIECE_MIDDLE;
        if (c0 == 0) {
            texture = TEXTURE_PIECE_RIGHT;
        } else if (c0 == WindowCount - 1) {
            texture = TEXTURE_PIECE_LEFT;
        } else {
            texture = TEXTURE_PIECE_MIDDLE;
        }
        p_f4 = create_a_quad(sp[0] + c0, sp[0] + c0 + 1, sp[1] + c0, sp[1] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);
        p_f4->Type = FACE_TYPE_FENCE;
        p_f4 = create_a_quad(sp[1] + c0, sp[1] + c0 + 1, sp[2] + c0, sp[2] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);
        p_f4->Type = FACE_TYPE_FENCE;
    }
    {
        SLONG ya, yb;
        ya = PAP_calc_height_at(x, z);
        yb = PAP_calc_height_at(wall_list[wall].DX, wall_list[wall].DZ);
        insert_collision_vect(x, ya, z, wall_list[wall].DX, yb, wall_list[wall].DZ, STOREY_TYPE_FENCE_FLAT, 8, -wall);
        insert_collision_vect(wall_list[wall].DX, yb, wall_list[wall].DZ, x, ya, z, STOREY_TYPE_FENCE_FLAT, 8, -wall);
    }
}

// uc_orig: build_height_fence (fallen/Source/Building.cpp)
// Builds a single-panel height fence with per-segment texture lookup.
void build_height_fence(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height, SLONG alt_mode)
{
    SLONG c0;
    SLONG sp[10];
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;
    UBYTE* ptexture1;
    SLONG tcount1;
    SLONG count;

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    ptexture1 = wall_list[wall].Textures;
    tcount1 = wall_list[wall].Tcount;

    if (alt_mode == 1) {
        sp[0] = build_row_wall_only_points_at_floor_alt(height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_floor_alt(0, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    } else {
        sp[0] = build_row_wall_only_points_at_y(y + height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_y(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    }

    count = 0;
    for (c0 = 0; c0 < WindowCount; c0++) {
        texture = TEXTURE_PIECE_MIDDLE;
        if (c0 == 0) {
            texture = TEXTURE_PIECE_RIGHT;
        } else if (c0 == WindowCount - 1) {
            texture = TEXTURE_PIECE_LEFT;
        } else {
            texture = TEXTURE_PIECE_MIDDLE;
        }
        if (ptexture1 && (count < tcount1) && ptexture1[count]) {
            p_f4 = create_a_quad_tex(sp[0] + c0, sp[0] + c0 + 1, sp[1] + c0, sp[1] + c0 + 1, ptexture1[count]);
        } else {
            p_f4 = create_a_quad(sp[0] + c0, sp[0] + c0 + 1, sp[1] + c0, sp[1] + c0 + 1, texture_style, texture);
        }
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);
        p_f4->Type = FACE_TYPE_FENCE;
        count++;
    }
    {
        SLONG ya, yb;
        ya = PAP_calc_height_at(x, z);
        yb = PAP_calc_height_at(wall_list[wall].DX, wall_list[wall].DZ);
        insert_collision_vect(x, ya, z, wall_list[wall].DX, yb, wall_list[wall].DZ, STOREY_TYPE_FENCE_FLAT, height / 64, -wall);
        insert_collision_vect(wall_list[wall].DX, yb, wall_list[wall].DZ, x, ya, z, STOREY_TYPE_FENCE_FLAT, height / 64, -wall);
    }
}

// File-local macro for thick brick wall width.
// uc_orig: WALL_WIDTH (fallen/Source/Building.cpp)
#define WALL_WIDTH (60)

// uc_orig: build_thick_wall_polys (fallen/Source/Building.cpp)
// Builds a thick wall section (3 quads: two faces + top) given 4 corner XZ positions.
// 'flag' == 1 adds a closing face on the left end; flag == 2 on the right end.
void build_thick_wall_polys(SLONG* x, SLONG* z, SLONG y, SLONG height, SLONG flag, SLONG storey, SLONG wall)
{
    SLONG sp[8];
    SLONG c0;
    struct PrimFace4* p_f4;
    SLONG texture, texture_style;

    insert_collision_vect(x[1], y, z[1], x[0], y, z[0], STOREY_TYPE_NORMAL, 1, -wall);
    insert_collision_vect(x[2], y, z[2], x[3], y, z[3], STOREY_TYPE_NORMAL, 1, -wall);

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    sp[0] = build_row_wall_only_points_at_y(y + height, x[0], z[0], x[1], z[1], wall);
    sp[1] = build_row_wall_only_points_at_y(y, x[0], z[0], x[1], z[1], wall);
    sp[2] = build_row_wall_only_points_at_y(y + height, x[2], z[2], x[3], z[3], wall);
    sp[3] = build_row_wall_only_points_at_y(y, x[2], z[2], x[3], z[3], wall);
    sp[4] = next_prim_point;

    for (c0 = 0; c0 < WindowCount; c0++) {
        texture = TEXTURE_PIECE_MIDDLE;
        p_f4 = create_a_quad(sp[0] + c0 + 1, sp[0] + c0, sp[1] + c0 + 1, sp[1] + c0, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(sp[2] + c0, sp[2] + c0 + 1, sp[3] + c0, sp[3] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4 = create_a_quad(sp[0] + c0, sp[0] + c0 + 1, sp[2] + c0, sp[2] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        add_quad_to_walkable_list(next_prim_face4 - 1);
    }
    if (flag == 1) {
        p_f4 = create_a_quad(sp[0], sp[2], sp[1], sp[3], texture_style, texture);
        p_f4->ThingIndex = -wall;
        insert_collision_vect(x[0], y, z[0], x[2], y, z[2], STOREY_TYPE_FENCE_BRICK, 4, -wall);
    }
    if (flag == 2) {
        p_f4 = create_a_quad(sp[3] - 1, sp[1] - 1, sp[4] - 1, sp[2] - 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        insert_collision_vect(x[1], y, z[1], x[3], y, z[3], STOREY_TYPE_FENCE_BRICK, 4, -wall);
    }
}

// uc_orig: build_brick_wall (fallen/Source/Building.cpp)
// Builds a thick brick wall storey by iterating corner-miter segments.
// Each wall section is built by build_thick_wall_polys with mitered corners.
SLONG build_brick_wall(SLONG storey)
{
    SLONG x[4], z[4];
    SLONG wall, nwall;
    SLONG dx, dz, len;
    SLONG y, height;
    SLONG start_point, start_face3, start_face4;
    SLONG prev_facet;
    SLONG prim;
    SLONG building;

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    y = storey_list[storey].DY;
    height = storey_list[storey].Height;

    x[0] = storey_list[storey].DX;
    z[0] = storey_list[storey].DZ;

    wall = storey_list[storey].WallHead;
    nwall = wall_list[wall].Next;

    x[1] = wall_list[wall].DX;
    z[1] = wall_list[wall].DZ;

    dz = -(x[1] - x[0]);
    dx = (z[1] - z[0]);
    len = Root(dx * dx + dz * dz);
    if (len == 0) len = 1;
    dx = (dx * WALL_WIDTH) / len;
    dz = (dz * WALL_WIDTH) / len;

    x[2] = x[0] + dx;
    z[2] = z[0] + dz;
    calc_new_corner_point(x[0], z[0], x[1], z[1], wall_list[nwall].DX, wall_list[nwall].DZ, WALL_WIDTH, &x[3], &z[3]);

    build_thick_wall_polys(&x[0], &z[0], y, height, 1, storey, wall);
    wall = nwall;
    nwall = wall_list[wall].Next;

    while (wall && nwall) {
        x[0] = x[1];
        x[2] = x[3];
        z[0] = z[1];
        z[2] = z[3];
        x[1] = wall_list[wall].DX;
        z[1] = wall_list[wall].DZ;
        calc_new_corner_point(x[0], z[0], x[1], z[1], wall_list[nwall].DX, wall_list[nwall].DZ, WALL_WIDTH, &x[3], &z[3]);
        build_thick_wall_polys(&x[0], &z[0], y, height, 0, storey, wall);
        wall = nwall;
        nwall = wall_list[wall].Next;
    }

    // Last segment.
    x[0] = x[1];
    x[2] = x[3];
    z[0] = z[1];
    z[2] = z[3];
    x[1] = wall_list[wall].DX;
    z[1] = wall_list[wall].DZ;
    dz = -(x[1] - x[0]);
    dx = (z[1] - z[0]);
    len = Root(dx * dx + dz * dz);
    if (len == 0) len = 1;
    dx = (dx * WALL_WIDTH) / len;
    dz = (dz * WALL_WIDTH) / len;
    x[3] = x[1] + dx;
    z[3] = z[1] + dz;
    build_thick_wall_polys(&x[0], &z[0], y, height, 2, storey, wall);

    prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, 0, FACET_FLAG_NON_SORT, 0);
    prim = build_building(start_point, start_face3, start_face4, prev_facet);
    building = storey_list[storey].BuildingHead;
    place_building_at(building, prim, build_x, 0, build_z);

    return (prev_facet);
}

// uc_orig: build_fence (fallen/Source/Building.cpp)
// Dispatches to the correct fence builder based on storey type and height.
// Also clears WindowCount after building.
void build_fence(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG alt_mode = 1; // Stick to floor by default.

    if (storey_list[storey].ExtraFlags & FLAG_STOREY_EXTRA_ONBUILDING) {
        alt_mode = 0;
    }

    switch (storey_list[storey].StoreyType) {
    case STOREY_TYPE_FENCE:
        build_fence_points_and_faces(y, y + ((height * 3) >> 2) + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall, 1);
        break;
    case STOREY_TYPE_FENCE_BRICK:
        build_height_fence(x, y, z, wall, storey, height, alt_mode);
        break;
    case STOREY_TYPE_FENCE_FLAT:
    case STOREY_TYPE_OUTSIDE_DOOR:
        if (height == 512)
            build_high_chain_fence(x, y, z, wall, storey, BLOCK_SIZE * 4, alt_mode);
        else
            build_height_fence(x, y, z, wall, storey, height, alt_mode);
        break;
    }
    WindowCount = 0;
}

// uc_orig: build_whole_fence (fallen/Source/Building.cpp)
// Builds all fence segments for a storey into a single non-sorted facet/building object.
SLONG build_whole_fence(SLONG storey)
{
    SLONG wall, px, pz;
    SLONG height, y;
    SLONG prev_facet;
    SLONG prim;
    SLONG building;
    SLONG start_point, start_face3, start_face4;

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    height = storey_list[storey].Height;
    y = storey_list[storey].DY;
    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;

    wall = storey_list[storey].WallHead;
    while (wall) {
        build_fence(px, y, pz, wall, storey, height);
        px = wall_list[wall].DX;
        pz = wall_list[wall].DZ;
        wall = wall_list[wall].Next;
    }

    prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, 0, FACET_FLAG_NON_SORT, 0);
    prim = build_building(start_point, start_face3, start_face4, prev_facet);
    building = storey_list[storey].BuildingHead;
    place_building_at(building, prim, build_x, 0, build_z);
    return (prev_facet);
}

// uc_orig: process_external_pieces (fallen/Source/Building.cpp)
// Processes all non-normal storeys (cables, fences) for a building before the main storey loop.
// Also tracks the Y range across ground-level normal storeys for offset calculations.
SLONG process_external_pieces(UWORD building)
{
    SLONG storey, c0 = 0;
    SLONG prev_facet = 0;

    storey = building_list[building].StoreyHead;
    while (storey && c0 < 400) {
        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_CABLE:
            prev_facet = build_cables(storey, prev_facet);
            break;
        case STOREY_TYPE_FENCE:
        case STOREY_TYPE_FENCE_FLAT:
        case STOREY_TYPE_OUTSIDE_DOOR:
            if (storey_list[storey].DY == 0)
                prev_facet = build_whole_fence(storey);
            break;
        case STOREY_TYPE_FENCE_BRICK:
            if (storey_list[storey].DY == 0)
                prev_facet = build_whole_fence(storey);
            break;
        case STOREY_TYPE_NORMAL:
            if (storey_list[storey].DY == 0) {
                SLONG wall;
                SLONG x, y, z;
                wall = storey_list[storey].WallHead;
                x = storey_list[storey].DX;
                z = storey_list[storey].DZ;
                y = PAP_calc_height_at(x, z);
                if (y < build_min_y)
                    build_min_y = y;
                if (y > build_max_y)
                    build_max_y = y;
                while (wall) {
                    x = wall_list[wall].DX;
                    z = wall_list[wall].DZ;
                    y = PAP_calc_height_at(x, z);
                    if (y < build_min_y)
                        build_min_y = y;
                    if (y > build_max_y)
                        build_max_y = y;
                    wall = wall_list[wall].Next;
                }
            }
        }
        storey = storey_list[storey].Next;
        c0++;
    }
    if (build_max_y != build_min_y) {
        build_max_y += (ELE_SIZE >> 2) - 1;
        build_max_y &= ~63;
    }
    return (prev_facet);
}

// uc_orig: mark_map_with_ladder (fallen/Source/Building.cpp)
// Sets the FLOOR_LADDER flag on the map tile at the 1/3 point along the first wall
// of a ladder storey, offset slightly to the side of the wall.
void mark_map_with_ladder(SLONG storey)
{
    SLONG x1, z1, x2, z2;
    SLONG dx, dz;
    SLONG wall;

    x1 = storey_list[storey].DX;
    z1 = storey_list[storey].DZ;
    wall = storey_list[storey].WallHead;
    x2 = wall_list[wall].DX;
    z2 = wall_list[wall].DZ;

    dx = x2 - x1;
    dz = z2 - z1;
    x1 += dx / 3;
    z1 += dz / 3;
    x1 += dz >> 3;
    z1 -= dx >> 3;

    set_map_flag(x1 >> ELE_SHIFT, z1 >> ELE_SHIFT, FLOOR_LADDER);
}

// uc_orig: setup_storey_data (fallen/Source/Building.cpp)
// Pre-pass: clears facet-linked flags on all normal/fence storeys and their walls,
// and records the wall index for any ladder/fire-escape storeys.
void setup_storey_data(UWORD building, SWORD* wall_for_ladder)
{
    SLONG wall, storey;

    storey = building_list[building].StoreyHead;
    while (storey) {
        storey_list[storey].StoreyFlags &= ~FLAG_STOREY_FACET_LINKED;
        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_NORMAL:
        case STOREY_TYPE_FENCE:
        case STOREY_TYPE_FENCE_FLAT:
        case STOREY_TYPE_FENCE_BRICK:
        case STOREY_TYPE_OUTSIDE_DOOR:
            wall = storey_list[storey].WallHead;
            while (wall) {
                wall_list[wall].WallFlags &= ~FLAG_WALL_FACET_LINKED;
                wall = wall_list[wall].Next;
            }
            break;
        case STOREY_TYPE_LADDER:
            wall = find_wall_for_fe(storey_list[storey].DX, storey_list[storey].DZ, building_list[building].StoreyHead);
            if (wall >= 0)
                wall_for_ladder[wall] = storey;
            mark_map_with_ladder(storey);
            break;
        }
        storey = storey_list[storey].Next;
    }
}

// uc_orig: find_connect_wall (fallen/Source/Building.cpp)
// Searches the next STOREY_TYPE_NORMAL storey above 'storey' for a wall that matches
// the given (x1,z1)→(x2,z2) segment. Returns that wall index, or 0 if not found.
// Used to connect recessed-window geometry to the storey above.
SLONG find_connect_wall(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG* connect_storey, SLONG storey, UBYTE** ret_tex, UWORD* ret_tcount)
{
    SLONG found = 0;
    SLONG wall;
    SLONG fx1, fz1, fx2, fz2;

    storey = storey_list[storey].Next;
    while (storey && !found) {
        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_NORMAL:
            found = 1;
            break;
        default:
            storey = storey_list[storey].Next;
        }
    }
    if (found) {
        fx1 = storey_list[storey].DX;
        fz1 = storey_list[storey].DZ;
        wall = storey_list[storey].WallHead;
        while (wall) {
            fx2 = wall_list[wall].DX;
            fz2 = wall_list[wall].DZ;
            if (fx1 == x1 && fz1 == z1 && fx2 == x2 && fz2 == z2) {
                *connect_storey = storey;
                *ret_tex = wall_list[wall].Textures;
                *ret_tcount = wall_list[wall].Tcount;
                return (wall);
            }
            fx1 = fx2;
            fz1 = fz2;
            wall = wall_list[wall].Next;
        }
    } else
        return (0);

    return (0);
}

// uc_orig: insert_recessed_wall_vect (fallen/Source/Building.cpp)
// Registers four collision vectors around the inward recess of a recessed window:
// the main wall face plus two short side returns at RECESS_SIZE depth.
void insert_recessed_wall_vect(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2,
    UBYTE storey_type,
    UBYTE height,
    SLONG wall)
{
    SLONG x1o, z1o;
    SLONG x2o, z2o;
    SLONG dx, dz;
    SLONG len;

    dx = x2 - x1;
    dz = z2 - z1;
    len = Root(dx * dx + dz * dz);
    if (len == 0)
        len = 1;
    dx = (dx * RECESS_SIZE) / len;
    dz = (dz * RECESS_SIZE) / len;

    x1o = x1 + dz + (dx >> 1);
    z1o = z1 - dx + (dz >> 1);
    x2o = x2 + dz - (dx >> 1);
    z2o = z2 - dx - (dz >> 1);

    insert_collision_vect(x1, y1, z1, x2, y2, z2, storey_type, height * 4, wall);
    insert_collision_vect(x1, y1, z1, x1o, y1, z1o, storey_type, height * 4, wall);
    insert_collision_vect(x1o, y1, z1o, x2o, y2, z2o, storey_type, height * 4, wall);
    insert_collision_vect(x2o, y2, z2o, x2, y2, z2, storey_type, height * 4, wall);
}

// uc_orig: build_storey_floor (fallen/Source/Building.cpp)
// Builds floor/roof geometry for a storey: finds bounding box, calls build_easy_roof.
// Returns the bound facet index, or 0 if edges are not rectangular.
SLONG build_storey_floor(SLONG storey, SLONG y, SLONG flag)
{
    SLONG min_x = 9999999, max_x = 0, min_z = 9999999, max_z = 0;
    SLONG depth;

    SLONG wall;
    SLONG face_wall;
    SLONG angles;

    face_wall = -storey_list[storey].WallHead;

    global_y = get_map_height((storey_list[storey].DX >> ELE_SHIFT), storey_list[storey].DZ >> ELE_SHIFT) << FLOOR_HEIGHT_SHIFT;

    BOUNDS(storey_list[storey].DX, storey_list[storey].DZ);
    wall = storey_list[storey].WallHead;
    while (wall) {
        BOUNDS(wall_list[wall].DX, wall_list[wall].DZ);
        wall = wall_list[wall].Next;
    }

    block_min_x = min_x;
    block_max_x = max_x;

    min_x -= ELE_SIZE;
    min_z -= ELE_SIZE;
    max_x += ELE_SIZE;
    max_z += ELE_SIZE;

    depth = (max_z - min_z) >> ELE_SHIFT;

    edge_min_z = min_z;

    angles = build_edge_list(storey, 0);
    dump_edge_list(depth);

    if (!angles) {
        SLONG bound;
        dump_edge_list(depth);
        bound = build_easy_roof(min_x, edge_min_z, max_x, depth, y, face_wall, flag);
        bin_edge_list();
        return (bound);
    }
    return (0);
}

// uc_orig: build_trench (fallen/Source/Building.cpp)
// Builds geometry and collision vectors for a trench storey (sunken courtyard).
// Returns the updated prev_facet index after appending the trench facet.
SLONG build_trench(SLONG prev_facet, SLONG storey)
{
    SLONG start_point, start_face3, start_face4;
    SLONG wall;
    SLONG px, pz;
    SLONG col_vect;
    SLONG bound;
    SLONG min_y;

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    wall = storey_list[storey].WallHead;
    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    min_y = PAP_calc_height_at(px, pz);
    while (wall) {
        SLONG y;
        px = wall_list[wall].DX;
        pz = wall_list[wall].DZ;
        y = PAP_calc_height_at(px, pz);
        if (y < min_y)
            min_y = y;
        wall = wall_list[wall].Next;
    }

    set_floor_hidden(storey, 1, FLOOR_TRENCH | PAP_FLAG_HIDDEN);
    wall = storey_list[storey].WallHead;
    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    while (wall) {
        SLONG x2, z2;

        x2 = wall_list[wall].DX;
        z2 = wall_list[wall].DZ;
        append_wall_prim(px, min_y, pz, wall, storey, -256);
        set_vect_floor_height(px, pz, x2, z2, min_y >> ALT_SHIFT);

        // Insert col vect backwards for trenches so we can enter but not leave.
        col_vect = insert_collision_vect(x2, min_y - 256, z2, px, min_y - 256, pz, STOREY_TYPE_TRENCH, 0, -wall);
        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, 0, col_vect);

        start_point = next_prim_point;
        start_face3 = next_prim_face3;
        start_face4 = next_prim_face4;

        wall = wall_list[wall].Next;
        px = x2;
        pz = z2;
    }
    bound = build_storey_floor(storey, min_y - 256 + 2, 1);
    prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_ROOF, -bound);

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;
    return (prev_facet);
}

// uc_orig: wall_for_fe (fallen/Source/Building.cpp)
// File-private arrays: wall-index to fire-escape/ladder storey mappings.
// Used within create_building_prim only.
static SWORD wall_for_fe[100];
// uc_orig: wall_for_ladder (fallen/Source/Building.cpp)
static SWORD wall_for_ladder_local[100];

// uc_orig: create_building_prim (fallen/Source/Building.cpp)
// Main building geometry builder: iterates all storeys, appends wall/roof/staircase prims,
// creates facets and collision vectors. Returns prim object index or 0 on failure.
SLONG create_building_prim(UWORD building, SLONG* small_y)
{
    UBYTE pass2 = 0;
    SLONG storey;
    SLONG wall;

    SLONG start_point, start_face3, start_face4;
    SLONG mid_point, mid_face4;
    SLONG y = 0, offset_y = 0;

    ULONG obj_start_point;
    ULONG obj_start_face3, obj_start_face4;
    ULONG prev_facet = 0;
    SLONG wall_count = 0;
    SLONG first = 0;
    SLONG valid = 0;
    SLONG col_vect;

    SLONG circular;

    if (storey_list[building_list[building].StoreyHead].StoreyType != STOREY_TYPE_NORMAL) {
        circular = 1;
    } else {
        circular = is_storey_circular(building_list[building].StoreyHead);
    }

    build_min_y = 999999;
    build_max_y = -999999;

    current_building = building;
    *small_y = 99999;

    building_list[building].Walkable = 0;
    memset((UBYTE*)wall_for_fe, 0, 200);
    memset((UBYTE*)wall_for_ladder_local, 0, 200);

    process_external_pieces(building);

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    obj_start_point = start_point;
    obj_start_face3 = start_face3;
    obj_start_face4 = start_face4;

    setup_storey_data(building, &wall_for_ladder_local[0]);
    storey = building_list[building].StoreyHead;

    if (circular) {
        if (build_min_y != build_max_y)
            build_max_y -= 256;
        offset_y = build_max_y;
    } else {
        offset_y = 0;
        build_min_y = 0;
        build_max_y = 0;
    }

    building_list[building].OffsetY = build_max_y;
    while (storey) {
        SLONG x1, z1, x2, z2;

        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_FENCE_FLAT:
        case STOREY_TYPE_OUTSIDE_DOOR:
        case STOREY_TYPE_FENCE:
        case STOREY_TYPE_FENCE_BRICK:
            if (storey_list[storey].DY) {
                x1 = storey_list[storey].DX;
                z1 = storey_list[storey].DZ;
                y = storey_list[storey].DY;
                wall = storey_list[storey].WallHead;
                while (wall) {

                    build_fence(x1, y + offset_y, z1, wall, storey, storey_list[storey].Height);

                    x1 = wall_list[wall].DX;
                    z1 = wall_list[wall].DZ;

                    wall = wall_list[wall].Next;

                    prev_facet = build_facet(start_point, mid_point, start_face3, start_face4, mid_face4, prev_facet, 0, col_vect);

                    start_point = next_prim_point;
                    start_face3 = next_prim_face3;
                    start_face4 = next_prim_face4;
                }
            }

            break;
        case STOREY_TYPE_NORMAL:
            valid = 1;
            if (first == 0) {
                set_floor_hidden(storey, 0, PAP_FLAG_HIDDEN);
                first = 1;
            }

            x1 = storey_list[storey].DX;
            z1 = storey_list[storey].DZ;

            wall_count = 0;
            wall = storey_list[storey].WallHead;
            while (wall) {
                SLONG connect_wall, connect_storey, connect_count = 0;

                x2 = wall_list[wall].DX;
                z2 = wall_list[wall].DZ;
                if (!(wall_list[wall].WallFlags & FLAG_WALL_FACET_LINKED)) {
                    y = storey_list[storey].DY;

                    if (y == 0) {
                        SLONG temp_y;
                        temp_y = PAP_calc_height_at(x1, z1);
                        if (temp_y < *small_y)
                            *small_y = temp_y;
                    }

                    wall_list[wall].WallFlags |= FLAG_WALL_FACET_LINKED;
                    if ((y) == 0 && (build_min_y != build_max_y) && circular)
                        append_foundation_wall(x1, y + offset_y + 256, z1, wall, storey, storey_list[storey].Height);
                    else
                        append_wall_prim(x1, y + offset_y, z1, wall, storey, storey_list[storey].Height);

                    {
                        UBYTE* tex = 0;
                        UWORD tcount = 0;
                        connect_wall = find_connect_wall(x1, z1, x2, z2, &connect_storey, storey, &tex, &tcount);
                        connect_count = 1;
                        if (connect_wall) {
                            while (connect_wall) {
                                SLONG ty;
                                connect_count++;
                                ty = storey_list[connect_storey].DY;
                                wall_list[connect_wall].WallFlags |= FLAG_WALL_FACET_LINKED;
                                append_wall_prim(x1, ty + offset_y, z1, connect_wall, connect_storey, storey_list[connect_storey].Height);
                                connect_wall = find_connect_wall(x1, z1, x2, z2, &connect_storey, connect_storey, &tex, &tcount);
                            }
                        }
                    }

                    if (wall_list[wall].WallFlags & FLAG_WALL_RECESSED) {
                        insert_recessed_wall_vect(
                            x1, y + offset_y, z1,
                            x2, y + offset_y, z2,
                            STOREY_TYPE_NORMAL,
                            connect_count,
                            -wall);
                    } else {
                        if (y == 0) {
                            col_vect = insert_collision_vect(x1, y + offset_y, z1, x2, y + offset_y, z2, STOREY_TYPE_NORMAL_FOUNDATION, connect_count * 4, -wall);
                        } else
                            col_vect = insert_collision_vect(x1, y + offset_y, z1, x2, y + offset_y, z2, STOREY_TYPE_NORMAL, connect_count * 4, -wall);
                    }

                    mid_point = next_prim_point;
                    mid_face4 = next_prim_face4;
                    if (wall_for_fe[wall_count] && pass2 == 0) {
                        build_firescape(wall_for_fe[wall_count]);
                    }

                    prev_facet = build_facet(start_point, mid_point, start_face3, start_face4, mid_face4, prev_facet, 0, col_vect);

                    start_point = next_prim_point;
                    start_face3 = next_prim_face3;
                    start_face4 = next_prim_face4;
                }

                x1 = x2;
                z1 = z2;

                wall = wall_list[wall].Next;
                wall_count++;
            }
            pass2 = 1;

            break;
        case STOREY_TYPE_LADDER:
            break;
        case STOREY_TYPE_FIRE_ESCAPE:
            wall = find_wall_for_fe(storey_list[storey].DX, storey_list[storey].DZ, building_list[building].StoreyHead);
            if (wall >= 0)
                wall_for_fe[wall] = storey;
            break;
        case STOREY_TYPE_TRENCH:
            prev_facet = build_trench(prev_facet, storey);
            start_point = next_prim_point;
            start_face3 = next_prim_face3;
            start_face4 = next_prim_face4;
            break;
        case STOREY_TYPE_STAIRCASE:
            build_staircase(storey);
            prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, 0, 0);

            start_point = next_prim_point;
            start_face3 = next_prim_face3;
            start_face4 = next_prim_face4;
            break;

            /* moved to setup_storey_data
                            case STOREY_TYPE_LADDER:
                                    wall=find_wall_for_fe(storey_list[storey].DX,storey_list[storey].DZ,building_list[building].StoreyHead);
                                    if(wall>=0)
                                            wall_for_ladder[wall]=storey;
                                    ladder_count++;
                                    break;
            */

        default:
            break;
        }

        storey = storey_list[storey].Next;
    }

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    storey = building_list[building].StoreyHead;
    while (storey) {
        SLONG bound;

        if ((storey_list[storey].StoreyFlags & (FLAG_STOREY_TILED_ROOF | FLAG_STOREY_FLAT_TILED_ROOF)) && (storey_list[storey].StoreyType == STOREY_TYPE_NORMAL)) {
            SLONG flat = 0;
            if (storey_list[storey].StoreyFlags & (FLAG_STOREY_FLAT_TILED_ROOF)) {
                flat = 1;
            }
            bound = build_roof(storey, storey_list[storey].DY + offset_y + storey_list[storey].Height, flat);
        }

        if (0) {
            /*
                            switch(storey_list[storey_list[storey].Roof].StoreyType)
                            {
                                    case STOREY_TYPE_ROOF:
                                                    if(building==3)
                                                    build_roof(storey,storey_list[storey_list[storey].Roof].DY+offset_y);
                                                    break;
                                    case STOREY_TYPE_ROOF_QUAD:
                                                    if(building==3)
                                                    build_roof_quad(storey,storey_list[storey_list[storey].Roof].DY+offset_y);
                                                    break;
                                    case STOREY_TYPE_LADDER:
                                                    if(building==3)
                                                    build_ladder(storey);
                                    break;

                            }
            */
        } else {
            switch (storey_list[storey].StoreyType) {
            case STOREY_TYPE_LADDER:
                build_ladder(storey);
                break;
            case STOREY_TYPE_SKYLIGHT:
                build_skylight(storey);
                break;
            }
        }

        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_ROOF, 0);
        start_point = next_prim_point;
        start_face3 = next_prim_face3;
        start_face4 = next_prim_face4;

        if (storey_list[storey].InsideStorey) {
            SLONG istorey, iwall;
            SLONG added_wall = 0;

            istorey = storey_list[storey].InsideStorey;
            while (istorey) {
                SLONG x1, y1, z1;

                iwall = storey_list[istorey].WallHead;
                x1 = storey_list[istorey].DX;
                y1 = storey_list[istorey].DY;
                z1 = storey_list[istorey].DZ;

                while (iwall) {
                    added_wall = 1;
                    append_wall_prim(x1, y1 + offset_y, z1, iwall, istorey, 256);
                    x1 = wall_list[iwall].DX;
                    z1 = wall_list[iwall].DZ;
                    iwall = wall_list[iwall].Next;
                }
                istorey = storey_list[istorey].Next;
            }

            if (added_wall) {
                prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_INSIDE, -bound);
                start_point = next_prim_point;
                start_face3 = next_prim_face3;
                start_face4 = next_prim_face4;
            }
        }
        storey = storey_list[storey].Next;
    }

    if (valid) {
        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_ROOF, 0);
    }
    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    if (valid) {
        return (build_building(obj_start_point, obj_start_face3, obj_start_face4, prev_facet));
    } else {
        return (0);
    }
}

// uc_orig: copy_to_game_map (fallen/Source/Building.cpp)
// Stub: was intended to copy editor map to game map. Body is empty in original.
void copy_to_game_map(void)
{
}

// uc_orig: clear_map2 (fallen/Source/Building.cpp)
// Resets all building/prim/map counters and zeroes prim arrays for a fresh city rebuild.
void clear_map2(void)
{
    SLONG x, z;

    memset((UBYTE*)&PAP_lo[0][0], 0, sizeof(PAP_Lo) * PAP_SIZE_LO * PAP_SIZE_LO);

    for (x = 0; x < EDIT_MAP_WIDTH; x++)
        for (z = 0; z < EDIT_MAP_DEPTH; z++) {
            mask_map_flag(x, z, (FLOOR_HIDDEN | FLOOR_LADDER | FLOOR_TRENCH));
        }

    next_building_object = 1;
    next_building_facet = 1;
    next_col_vect = 1;
    next_col_vect_link = 1;
    next_walk_link = 1;

    next_dbuilding = 1;
    next_dwalkable = 1;
    next_dfacet = 1;
    next_dstyle = 1;
    next_facet_link = 1;
    facet_link_count = 0;

    memset((UBYTE*)prim_points, 0, sizeof(struct PrimPoint) * MAX_PRIM_POINTS);
    memset((UBYTE*)prim_faces3, 0, sizeof(struct PrimFace3) * MAX_PRIM_FACES3);
    memset((UBYTE*)prim_faces4, 0, sizeof(struct PrimFace4) * MAX_PRIM_FACES4);
    memset((UBYTE*)prim_objects, 0, sizeof(struct PrimObject) * MAX_PRIM_OBJECTS);
    memset((UBYTE*)prim_multi_objects, 0, sizeof(struct PrimMultiObject) * MAX_PRIM_MOBJECTS);
}

// uc_orig: clear_floor_ladder (fallen/Source/Building.cpp)
// Stub: was intended to clear ladder floor flags. Body is empty in original.
void clear_floor_ladder(void)
{
}

// uc_orig: wibble_floor (fallen/Source/Building.cpp)
// Debug/editor function to animate floor heights with sin/cos. Always returns immediately.
void wibble_floor(void)
{
    return;
}

// uc_orig: clip_building_prim (fallen/Source/Building.cpp)
// Adjusts building prim point Y values upward to avoid clipping below terrain height.
void clip_building_prim(SLONG prim, SLONG x, SLONG y, SLONG z)
{
    SLONG index;
    struct BuildingFacet* p_facet;
    SLONG sp, ep;
    SLONG c0;

    index = building_objects[prim].FacetHead;
    while (index) {
        p_facet = &building_facets[index];

        if (p_facet->EndFace4)
            for (c0 = p_facet->StartFace4; c0 < p_facet->EndFace4; c0++) {
            }

        if (p_facet->EndFace3)
            for (c0 = p_facet->StartFace3; c0 < p_facet->EndFace3; c0++) {
            }

        sp = p_facet->StartPoint;
        ep = p_facet->EndPoint;
        for (c0 = sp; c0 < ep; c0++) {
            SLONG px, py, pz;
            SLONG fy;

            px = prim_points[c0].X + x;
            py = prim_points[c0].Y + y;
            pz = prim_points[c0].Z + z;

            fy = PAP_calc_height_at(px, pz);

            if (py < fy) {
                prim_points[c0].Y += fy - py;
            }
        }

        index = building_facets[index].NextFacet;
    }
}

// uc_orig: fix_furniture (fallen/Source/Building.cpp)
// Snaps all furniture Things to terrain height.
// Called commented-out at end of create_city — kept 1:1.
void fix_furniture(void)
{
    Thing* p_thing;
    SLONG c0;
    p_thing = TO_THING(0);
    for (c0 = 1; c0 < MAX_THINGS; c0++) {
        switch (p_thing->Class) {
        case CLASS_FURNITURE:
            p_thing->WorldPos.Y = PAP_calc_height_at(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Z >> 8) << 8;
            break;
        }
    }
}

// uc_orig: count_floor (fallen/Source/Building.cpp)
// Tallies texture page usage across the 128x128 floor map for stats.
void count_floor(void)
{
    SLONG x, z;
    SLONG page, tx, ty;
    UWORD texture;
    for (x = 0; x < 128; x++)
        for (z = 0; z < 128; z++) {
            texture = get_map_texture(x, z);

            tx = ((struct MiniTextureBits*)(&texture))->X;
            ty = ((struct MiniTextureBits*)(&texture))->Y;
            page = (UBYTE)(((struct MiniTextureBits*)(&texture))->Page);
            add_page_countxy(tx, ty, page);
        }
}

// uc_orig: create_city (fallen/Source/Building.cpp)
// Main city build pass: calls create_building_prim for every building, places them,
// calculates normals, and attaches walkable faces to map.
void create_city(UBYTE mode)
{
    SLONG c0;

    diff_page_count1 = 0;
    diff_page_count2 = 0;
    memset(page_count, 0, 8 * 64);
    count_floor();

    build_mode = mode;
    if (mode == BUILD_MODE_EDITOR) {
        copy_to_game_map();
        clear_map2();
        load_all_individual_prims();
    }

    next_building_object = 1;
    next_building_facet = 1;
    next_col_vect = 1;
    next_col_vect_link = 1;
    next_walk_link = 1;

    for (c0 = 1; c0 < MAX_BUILDINGS; c0++) {
        SLONG prim;
        if (building_list[c0].BuildingFlags) {
            SLONG y;
            prim = create_building_prim(c0, &y);
            y = 0;
            if (prim) {
                place_building_at(c0, prim, build_x, y, build_z);

                // save_asc: editor-only function defined in io.cpp (not yet migrated),
                // kept as inline extern to preserve the call site pattern from the original.
                extern void save_asc(UWORD building, UWORD version);
            }
        }
    }
    // apply_global_amb_to_map: defined in night.cpp, extern decl kept 1:1 from original.
    extern void apply_global_amb_to_map(void);

    calc_building_normals();
    clear_floor_ladder();

    for (c0 = 1; c0 < next_dwalkable; c0++) {
        SLONG face;

        for (face = dwalkables[c0].StartFace4; face < dwalkables[c0].EndFace4; face++) {
            prim_faces4[face].ThingIndex = c0;
            // attach_walkable_to_map is defined in build2.cpp (already migrated).
            void attach_walkable_to_map(SLONG face);

            attach_walkable_to_map(face);
            prim_faces4[face].FaceFlags |= FACE_FLAG_WALKABLE;
        }
    }
}

// uc_orig: offset_buildings (fallen/Source/Building.cpp)
// Shifts all storey and wall positions by (x,y,z). Editor tool for moving buildings.
void offset_buildings(SLONG x, SLONG y, SLONG z)
{
    SLONG c0;
    for (c0 = 1; c0 < MAX_STOREYS; c0++) {
        if (storey_list[c0].StoreyFlags) {
            storey_list[c0].DX += (SWORD)x;
            storey_list[c0].DY += (SWORD)y;
            storey_list[c0].DZ += (SWORD)z;
        }
    }
    for (c0 = 1; c0 < MAX_WALLS; c0++) {
        if (wall_list[c0].WallFlags) {
            wall_list[c0].DX += (SWORD)x;
            wall_list[c0].DY += (SWORD)y;
            wall_list[c0].DZ += (SWORD)z;
        }
    }
}

// uc_orig: is_it_clockwise (fallen/Source/Building.cpp)
// Returns 1 if screen-projected triangle (p0,p1,p2) is clockwise, 0 otherwise.
// Reads from global_res[] transformed-point buffer.
UWORD is_it_clockwise(SLONG p0, SLONG p1, SLONG p2)
{
    SLONG z;
    SLONG vx, vy, wx, wy;

    vx = global_res[p1].X - global_res[p0].X;
    wx = global_res[p2].X - global_res[p1].X;
    vy = global_res[p1].Y - global_res[p0].Y;
    wy = global_res[p2].Y - global_res[p1].Y;

    z = vx * wy - vy * wx;

    if (z > 0)
        return 1;
    else
        return 0;
}

// uc_orig: MAX_POINTS_PER_BUILDING (fallen/Source/Building.cpp)
// Maximum vertices per building object for normal averaging buffer.
#define MAX_POINTS_PER_BUILDING 12560

// uc_orig: calc_building_normals (fallen/Source/Building.cpp)
// Computes per-vertex normals for all building objects by averaging face normals.
// Normals stored in prim_normal[], normalised to length 256.
void calc_building_normals(void)
{
    SLONG i;
    SLONG j;
    SLONG k;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG num_points;
    SVector fnormal;
    SLONG p_index;

    BuildingObject* p_obj;
    PrimFace3* p_f3;
    PrimFace4* p_f4;

    for (i = 1; i < next_building_object; i++) {
        p_obj = &building_objects[i];

        num_points = p_obj->EndPoint - p_obj->StartPoint;

        ASSERT(num_points <= MAX_POINTS_PER_BUILDING);

        memset(each_point, 0, sizeof(UBYTE) * num_points);

        for (j = p_obj->StartFace3; j < p_obj->EndFace3; j++) {
            p_f3 = &prim_faces3[j];

            calc_normal(-j, &fnormal);

            for (k = 0; k < 3; k++) {
                p_index = p_f3->Points[k] - p_obj->StartPoint;

                ASSERT(WITHIN(p_index, 0, MAX_POINTS_PER_BUILDING - 1));

                if (each_point[p_index] == 0) {
                    prim_normal[p_f3->Points[k]].X = fnormal.X;
                    prim_normal[p_f3->Points[k]].Y = fnormal.Y;
                    prim_normal[p_f3->Points[k]].Z = fnormal.Z;

                    each_point[p_index] = 1;
                } else {
                    prim_normal[p_f3->Points[k]].X *= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Y *= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Z *= each_point[p_index];

                    prim_normal[p_f3->Points[k]].X += fnormal.X;
                    prim_normal[p_f3->Points[k]].Y += fnormal.Y;
                    prim_normal[p_f3->Points[k]].Z += fnormal.Z;

                    each_point[p_index] += 1;

                    prim_normal[p_f3->Points[k]].X /= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Y /= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Z /= each_point[p_index];
                }
            }
        }

        for (j = p_obj->StartFace4; j < p_obj->EndFace4; j++) {
            p_f4 = &prim_faces4[j];

            calc_normal(j, &fnormal);

            for (k = 0; k < 4; k++) {
                p_index = p_f4->Points[k] - p_obj->StartPoint;

                ASSERT(WITHIN(p_index, 0, MAX_POINTS_PER_BUILDING - 1));

                if (each_point[p_index] == 0) {
                    prim_normal[p_f4->Points[k]].X = fnormal.X;
                    prim_normal[p_f4->Points[k]].Y = fnormal.Y;
                    prim_normal[p_f4->Points[k]].Z = fnormal.Z;

                    each_point[p_index] = 1;
                } else {
                    prim_normal[p_f4->Points[k]].X *= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Y *= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Z *= each_point[p_index];

                    prim_normal[p_f4->Points[k]].X += fnormal.X;
                    prim_normal[p_f4->Points[k]].Y += fnormal.Y;
                    prim_normal[p_f4->Points[k]].Z += fnormal.Z;

                    each_point[p_index] += 1;

                    prim_normal[p_f4->Points[k]].X /= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Y /= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Z /= each_point[p_index];
                }
            }
        }

        for (j = p_obj->StartPoint; j < p_obj->EndPoint; j++) {
            dx = abs(prim_normal[j].X);
            dy = abs(prim_normal[j].Y);
            dz = abs(prim_normal[j].Z);

            dist = dx * dx + dy * dy + dz * dz;
            dist = Root(dist);
            dist += 1;

            prim_normal[j].X <<= 8;
            prim_normal[j].Y <<= 8;
            prim_normal[j].Z <<= 8;

            prim_normal[j].X /= dist;
            prim_normal[j].Y /= dist;
            prim_normal[j].Z /= dist;

            if ((prim_normal[j].X * prim_normal[j].X + prim_normal[j].Y * prim_normal[j].Y + prim_normal[j].Z * prim_normal[j].Z) > 67536) {
                ASSERT(0);
            }
        }
    }
}

// uc_orig: fn_building_normal (fallen/Source/Building.cpp)
void fn_building_normal(Thing* b_thing)
{
    // Stub — building state machine was cut.
}
