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
