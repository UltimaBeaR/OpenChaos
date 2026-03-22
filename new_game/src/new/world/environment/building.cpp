// Temporary: game.h brings in ASSERT, MSG_add, Root, SDIST2, WITHIN, ELE_SHIFT, MAP_INDEX,
// MAP_WIDTH, MAP_HEIGHT, FLOOR_HEIGHT_SHIFT, CLASS_BUILDING, DT_BUILDING, DT_NONE,
// TO_THING, alloc_primary_thing, add_thing_to_map, and the Thing struct.
#include "game.h"
// Temporary: pap.h needed for PAP_2HI, MAP2, MAP macros (supermap abstraction)
#include "fallen/Headers/pap.h"
// Temporary: supermap.h needed for MAP, MAP2 struct access patterns
#include "fallen/Headers/supermap.h"
// Temporary: memory.h needed for MemAlloc, MemFree, prim_faces4, prim_points, AENG_dx_prim_points
#include "fallen/Headers/memory.h"
// Temporary: pap.h exposes PAP_calc_height_at
#include "pap.h"
// Temporary: collide.h exposes walk_links, next_walk_link, MAX_WALK_POOL, FACE_FLAG_WALKABLE
#include "fallen/Headers/collide.h"
// Temporary: aeng.h exposes AENG_dx_prim_points, SVector_F
#include "engine/graphics/pipeline/aeng.h"
// Temporary: prim.h exposes OR_SORT_LEVEL, FACE_FLAG_NON_PLANAR (FACE_FLAG_WALKABLE via interact chain)
#include "fallen/Headers/prim.h"
// Needed for OUTLINE_Outline, OUTLINE_create, OUTLINE_add_line, OUTLINE_overlap
#include "world/environment/outline.h"
// Needed for calc_ladder_ends (used by calc_ladder_pos in this file)
#include "world/environment/build2.h"

#include "world/environment/building.h"
#include "world/environment/building_globals.h"

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
    SLONG px, py, pz;
    SLONG face_x, face_y, face_z;
    SLONG c0;
    SLONG s, t, step_s, step_t;
    SLONG vx, vy, vz, wx, wy, wz;
    struct DepthStrip* me;
    SLONG prev_x, prev_z;
    SLONG quad;
    SLONG len;
    UBYTE info = 0;

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

    prev_x = -1;
    prev_z = -1;
    for (s = 5; s < (255); s += step_s)
        for (t = 5; t < (255) && ((s + t) < (256)); t += step_t) {
            px = face_x + ((s * vx) >> 8) + ((t * wx) >> 8);
            pz = face_z + ((s * vz) >> 8) + ((t * wz) >> 8);
            if ((px >> 8) != prev_x || (pz >> 8) != prev_z) {
                py = face_y + ((s * vy) >> 8) + ((t * wy) >> 8);

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
// Stub: the body is commented out in the original codebase ("wrong wrong wrong").
void add_tri_to_walkable_list(SWORD face)
{
    /* wrong wrong wrong
    SLONG	x,z;
    SLONG	p0;
    struct PrimFace3 *p_f3;

    p_f3=&prim_faces3[face];
    //for now just take one corner and add it to map at that corner
    p0=p_f3->Points[0];
    x=prim_points[p0].X>>ELE_SHIFT;
    z=prim_points[p0].Z>>ELE_SHIFT;
    add_walk_face_to_map(-face,x,z);
    */
}

// uc_orig: place_building_at (fallen/Source/Building.cpp)
// Creates a game Thing (CLASS_BUILDING) for the building and places it at (x,y,z).
// In DX mode: allocates a primary Thing, sets DrawType=DT_BUILDING, links to building_list[].
SLONG place_building_at(UWORD building, UWORD prim, SLONG x, SLONG y, SLONG z)
{
    UWORD map_thing;

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
    SLONG ny;

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
    SLONG x, z;
    SWORD type;

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
    SLONG x, z;
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
    SLONG dtx_across, dty_across;

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
    SLONG pp, p0, p1, p2, p3;
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
    SLONG flatten = 0;
    SLONG maxy = -9999;

    SLONG sp, ep, sf4, ef4;

    sp = next_prim_point;
    sf4 = next_prim_face4;

    for (z = 0; z < depth; z++) {
        SLONG polarity = 0;
        SLONG edge;
        SLONG dy = 0;
        SLONG prev_x_in = 0;
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
    SLONG width, depth;
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

    width = (max_x - min_x) >> ELE_SHIFT;
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
        SLONG prev_x_in = 0;
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
                        struct DepthStrip* me;

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
        SLONG polarity = 0;
        SLONG edge;
        edge = edge_heads_ptr[z];
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
    SLONG width, depth;
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

    width = (max_x - min_x) >> ELE_SHIFT;
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
                        struct DepthStrip* me;

                        set_map_flag(
                            x >> PAP_SHIFT_HI,
                            z + (edge_min_z >> PAP_SHIFT_HI),
                            flags);
                    }
                    done = 1;
                } else if (x == edge_pool_ptr[edge].X) {
                    polarity += edge_pool_ptr[edge].Count;
                    if (polarity & 1) {
                        struct DepthStrip* me;

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

    if (0) // lower
    {
        for (z = 0; z < depth; z++) {
            UWORD pfu, pfl;
            pfu = get_map_flags(min_x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI));
            pfl = get_map_flags(min_x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI) + 1);

            for (x = min_x + (1 << PAP_SHIFT_HI); x < max_x - (1 << PAP_SHIFT_HI); x += (1 << PAP_SHIFT_HI)) {
                UWORD fu, fl;
                fu = get_map_flags(x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI));
                fl = get_map_flags(x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI) + 1);
                if ((fu & fl & pfu & pfl & PAP_FLAG_HIDDEN)) {
                    set_map_height((x >> PAP_SHIFT_HI) + 1, z + (edge_min_z >> PAP_SHIFT_HI) + 1, -256 >> 2);
                }

                pfu = fu;
                pfl = fl;
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
    SLONG p0 = 0;
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
    SLONG page_to;
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
    SLONG page_to;

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
    SLONG dx, dz;
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
    SLONG px, pz, rx, rz;
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
    SLONG count = 0;
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
    SLONG px, pz, rx, rz;
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
