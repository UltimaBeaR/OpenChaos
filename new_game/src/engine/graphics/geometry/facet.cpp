#include "engine/platform/uc_common.h"

#include <math.h>

#include "engine/core/macros.h"                      // UC_INFINITY, QDIST2, WITHIN, ASSERT
#include "engine/core/matrix.h"                     // MATRIX_calc
#include "game/game_types.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/geometry/facet.h"
#include "engine/graphics/geometry/facet_globals.h"
#include "engine/graphics/geometry/sprite.h"
#include "engine/graphics/geometry/mesh.h"
#include "engine/graphics/geometry/superfacet.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"   // NIGHT_dfcache
#include "engine/graphics/lighting/crinkle.h"
#include "engine/input/keyboard_globals.h"   // ControlFlag, Keys, ShiftFlag
#include "map/pap.h"
#include "map/pap_globals.h"           // PAP_hi
#include "map/supermap_globals.h"      // next_dfacet, next_dbuilding
#include "navigation/inside2.h"
#include "navigation/inside2_globals.h"
#include "buildings/building.h"         // building API
#include "buildings/building_globals.h" // dx_textures_xy, dbuildings, dwalkables, roof_faces4
#include "map/level_pools.h"               // dstyles, dstoreys, paint_mem, inside_storeys, inside_stairs
#include "assets/texture.h"
#include "buildings/ware.h"          // WARE_Ware struct
#include "buildings/ware_globals.h"  // WARE_ware, WARE_rooftex, WARE_ware_upto
#include "game/game_tick_globals.h"             // allow_debug_keys
#include "camera/fc_globals.h"            // FC_cam
#include "engine/graphics/lighting/smap.h"            // SMAP_Link, SMAP_project_onto_poly
#include "buildings/prim_types.h"    // PrimFace4, PrimFace3, RFACE_FLAG_*, ROOF_SHIFT, RoofFace4

// POLY_set_local_rotation_none is a no-op on PC. On Dreamcast it reset the local
// rotation matrix before DrawIndexedPrimitive. Defined as a macro here to match
// the aeng.cpp usage pattern.
// uc_orig: POLY_set_local_rotation_none (fallen/DDEngine/Source/facet.cpp) [macro definition]
#define POLY_set_local_rotation_none() \
    {                                  \
    }

// AENG_detail_crinkles is defined in aeng.cpp (not yet migrated).
// uc_orig: AENG_detail_crinkles (fallen/DDEngine/Source/aeng.cpp)
extern int AENG_detail_crinkles;

// AENG_transparent_warehouses is defined in aeng.cpp (not yet migrated).
// uc_orig: AENG_transparent_warehouses (fallen/DDEngine/Source/aeng.cpp)
extern UBYTE AENG_transparent_warehouses;

// fade_black is defined in aeng.cpp (not yet migrated). Used in chunks 2+.
// uc_orig: fade_black (fallen/DDEngine/Source/aeng.cpp)
extern UWORD fade_black;

// get_fence_hole and get_fence_hole_next are defined in collide.cpp. Used in chunk 2+.
// uc_orig: get_fence_hole (fallen/Source/collide.cpp)
extern SLONG get_fence_hole(struct DFacet* p_facet);
// uc_orig: get_fence_hole_next (fallen/Source/collide.cpp)
extern SLONG get_fence_hole_next(struct DFacet* p_facet, SLONG along);

// AENG_drawing_a_warehouse is defined in aeng.cpp. Used in chunk 2+.
// uc_orig: AENG_drawing_a_warehouse (fallen/DDEngine/Source/aeng.cpp)
extern SLONG AENG_drawing_a_warehouse;

// Gap geometry constants for fence construction.
// uc_orig: GAP_HEIGHT (fallen/DDEngine/Source/facet.cpp)
#define GAP_HEIGHT 96.0
// uc_orig: GAP_WIDTH_PERC (fallen/DDEngine/Source/facet.cpp)
#define GAP_WIDTH_PERC 0.2

// Inline helpers for terrain height sampling.

// uc_orig: grid_height_at (fallen/DDEngine/Source/facet.cpp)
// Returns the world-space Y height at map tile (mx, mz).
static float grid_height_at(SLONG mx, SLONG mz)
{
    float dy;
    PAP_Hi* pap;
    pap = &PAP_hi[mx][mz];
    dy = (float)(pap->Alt << 3);
    return (dy);
}

// uc_orig: grid_height_at_world (fallen/DDEngine/Source/facet.cpp)
// Returns world-space Y height at world-space (x, z) coordinates.
float grid_height_at_world(float x, float z)
{
    return (grid_height_at(((SLONG)x) >> 8, ((SLONG)z) >> 8));
}

// apply_cloud — removed duplicate (defined in aeng.cpp, declared in poly.h).

// uc_orig: facet_rand (fallen/DDEngine/Source/facet.cpp)
ULONG facet_rand(void)
{
    facet_seed = (facet_seed * 69069) + 1;
    return (facet_seed >> 7);
}

// uc_orig: set_facet_seed (fallen/DDEngine/Source/facet.cpp)
void set_facet_seed(SLONG seed)
{
    facet_seed = seed;
}

// uc_orig: texture_quad (fallen/DDEngine/Source/facet.cpp)
// Selects the D3D texture page for one wall quad and sets u/v coords.
// texture_style < 0 uses per-segment paint overrides from paint_mem[].
// texture_style > 0 uses the standard tiled style from dx_textures_xy[][].
// Sets global `flip` as a side effect for FillFacetPoints to read.
SLONG texture_quad(POLY_Point* quad[4], SLONG texture_style, SLONG pos, SLONG count, SLONG flipx)
{
    SLONG page;
    SLONG texture_piece;
    SLONG rand;
    SLONG random = 0;

    rand = facet_rand() & 0x3;

    if (pos == 0)
        texture_piece = flipx ? TEXTURE_PIECE_LEFT : TEXTURE_PIECE_RIGHT;
    else if (pos == count - 2)
        texture_piece = flipx ? TEXTURE_PIECE_RIGHT : TEXTURE_PIECE_LEFT;
    else {
        static const UBYTE choice[4] = {
            TEXTURE_PIECE_MIDDLE,
            TEXTURE_PIECE_MIDDLE,
            TEXTURE_PIECE_MIDDLE1,
            TEXTURE_PIECE_MIDDLE2
        };

        texture_piece = choice[rand];
        if (rand > 1)
            random = 1;
    }

    if (texture_style < 0) {
        SLONG index;
        struct DStorey* p_storey;

        flip = 0;
        p_storey = &dstoreys[-texture_style];

        index = p_storey->Index;
        ASSERT(p_storey->Count);

        if (pos < p_storey->Count) {
            page = paint_mem[index + pos];

            if (page & 0x80) {

                flip = 1;
                page &= 0x7f;
                if (ControlFlag && allow_debug_keys) {
                    quad[0]->colour = 0xff0000;
                    quad[0]->specular = 0xff000000;
                    quad[1]->colour = 0xff0000;
                    quad[1]->specular = 0xff000000;
                    quad[2]->colour = 0xff0000;
                    quad[2]->specular = 0xff000000;
                    quad[3]->colour = 0xff0000;
                    quad[3]->specular = 0xff000000;
                    page = POLY_PAGE_COLOUR;
                }
            } else {
                if (ControlFlag && allow_debug_keys) {
                    quad[0]->colour = 0xff00;
                    quad[0]->specular = 0xff000000;
                    quad[1]->colour = 0xff00;
                    quad[1]->specular = 0xff000000;
                    quad[2]->colour = 0xff00;
                    quad[2]->specular = 0xff000000;
                    quad[3]->colour = 0xff00;
                    quad[3]->specular = 0xff000000;
                    page = POLY_PAGE_COLOUR;
                }
            }

            if ((page & 0x7f) == 0) {
                texture_style = p_storey->Style;
                if (ControlFlag && allow_debug_keys) {
                    quad[0]->colour = 0xff;
                    quad[0]->specular = 0xff000000;
                    quad[1]->colour = 0xff;
                    quad[1]->specular = 0xff000000;
                    quad[2]->colour = 0xff;
                    quad[2]->specular = 0xff000000;
                    quad[3]->colour = 0xff;
                    quad[3]->specular = 0xff000000;
                    page = POLY_PAGE_COLOUR;
                }
            }
        } else {
            texture_style = p_storey->Style;
        }
    }

    if (texture_style >= 0) {
        if (texture_style == 0)
            texture_style = 1;
        page = dx_textures_xy[texture_style][texture_piece].Page;
        flip = dx_textures_xy[texture_style][texture_piece].Flip;
        if (ControlFlag && allow_debug_keys && random) {
            quad[0]->colour = 0xffff;
            quad[0]->specular = 0xff000000;
            quad[1]->colour = 0xffff;
            quad[1]->specular = 0xff000000;
            quad[2]->colour = 0xffff;
            quad[2]->specular = 0xff000000;
            quad[3]->colour = 0xffff;
            quad[3]->specular = 0xff000000;
            page = POLY_PAGE_COLOUR;
        }
    }

    switch (flip) {
    case 0:
        quad[1]->u = 1.0;
        quad[1]->v = 0.0;
        quad[0]->u = 0.0;
        quad[0]->v = 0.0;
        quad[3]->u = 1.0;
        quad[3]->v = 1.0;
        quad[2]->u = 0.0;
        quad[2]->v = 1.0;
        break;
    case 1: // flip x
        quad[1]->u = 0.0;
        quad[1]->v = 0.0;
        quad[0]->u = 1.0;
        quad[0]->v = 0.0;
        quad[3]->u = 0.0;
        quad[3]->v = 1.0;
        quad[2]->u = 1.0;
        quad[2]->v = 1.0;
        break;
    case 2: // flip y
        quad[1]->u = 1.0;
        quad[1]->v = 1.0;
        quad[0]->u = 0.0;
        quad[0]->v = 1.0;
        quad[3]->u = 1.0;
        quad[3]->v = 0.0;
        quad[2]->u = 0.0;
        quad[2]->v = 0.0;
        break;
    case 3: // flip x+y
        quad[1]->u = 0.0;
        quad[1]->v = 1.0;
        quad[0]->u = 1.0;
        quad[0]->v = 1.0;
        quad[3]->u = 0.0;
        quad[3]->v = 0.0;
        quad[2]->u = 1.0;
        quad[2]->v = 0.0;
        break;
    }

    return (page);
}

// uc_orig: get_texture_page (fallen/DDEngine/Source/facet.cpp)
// Returns page index and writes *rflip without modifying u/v coords.
SLONG get_texture_page(SLONG texture_style, SLONG pos, SLONG count, UBYTE* rflip)
{
    SLONG page;
    SLONG texture_piece;
    SLONG rand;
    SLONG flip;

    rand = facet_rand() & 0x3;

    if (pos == 0)
        texture_piece = TEXTURE_PIECE_RIGHT;
    else if (pos == count - 2)
        texture_piece = TEXTURE_PIECE_LEFT;
    else {
        static const UBYTE choice[4] = {
            TEXTURE_PIECE_MIDDLE,
            TEXTURE_PIECE_MIDDLE,
            TEXTURE_PIECE_MIDDLE1,
            TEXTURE_PIECE_MIDDLE2
        };

        texture_piece = choice[rand];
    }

    if (texture_style < 0) {
        SLONG index;
        struct DStorey* p_storey;

        flip = 0;
        p_storey = &dstoreys[-texture_style];

        index = p_storey->Index;
        ASSERT(p_storey->Count);

        if (pos < p_storey->Count) {
            page = paint_mem[index + pos];

            if (page & 0x80) {
                flip = 1;
                page &= 0x7f;
            }
            if ((page & 0x7f) == 0) {
                texture_style = p_storey->Style;
            }
        } else {
            texture_style = p_storey->Style;
        }
    }

    if (texture_style >= 0) {
        if (texture_style == 0)
            texture_style = 1;
        page = dx_textures_xy[texture_style][texture_piece].Page;
        flip = dx_textures_xy[texture_style][texture_piece].Flip;
    }

    *rflip = UBYTE(flip);
    return (page);
}

// uc_orig: texture_quad2 (fallen/DDEngine/Source/facet.cpp)
// Sets u/v for a quad using a direct texture_piece slot (no positional randomisation).
SLONG texture_quad2(POLY_Point* quad[4], SLONG texture_style, SLONG texture_piece)
{
    SLONG page;

    if (texture_style == 0)
        texture_style = 1;
    page = dx_textures_xy[texture_style][texture_piece].Page;
    switch (dx_textures_xy[texture_style][texture_piece].Flip) {
    case 0:
        quad[1]->u = 1.0;
        quad[1]->v = 0.0;
        quad[0]->u = 0.0;
        quad[0]->v = 0.0;
        quad[3]->u = 1.0;
        quad[3]->v = 1.0;
        quad[2]->u = 0.0;
        quad[2]->v = 1.0;
        break;
    case 1: // flip x
        quad[1]->u = 0.0;
        quad[1]->v = 0.0;
        quad[0]->u = 1.0;
        quad[0]->v = 0.0;
        quad[3]->u = 0.0;
        quad[3]->v = 1.0;
        quad[2]->u = 1.0;
        quad[2]->v = 1.0;
        break;
    case 2: // flip y
        quad[1]->u = 1.0;
        quad[1]->v = 1.0;
        quad[0]->u = 0.0;
        quad[0]->v = 1.0;
        quad[3]->u = 1.0;
        quad[3]->v = 0.0;
        quad[2]->u = 0.0;
        quad[2]->v = 0.0;
        break;
    case 3: // flip x+y
        quad[1]->u = 0.0;
        quad[1]->v = 1.0;
        quad[0]->u = 1.0;
        quad[0]->v = 1.0;
        quad[3]->u = 0.0;
        quad[3]->v = 0.0;
        quad[2]->u = 1.0;
        quad[2]->v = 0.0;
        break;
    }

    return (page);
}

// uc_orig: texture_tri2 (fallen/DDEngine/Source/facet.cpp)
// Like texture_quad2 but for triangles (3 points).
SLONG texture_tri2(POLY_Point* quad[3], SLONG texture_style, SLONG texture_piece)
{
    SLONG page;

    if (texture_style == 0)
        texture_style = 1;
    page = dx_textures_xy[texture_style][texture_piece].Page;
    switch (dx_textures_xy[texture_style][texture_piece].Flip) {
    case 0:
        quad[1]->u = 1.0;
        quad[1]->v = 0.0;
        quad[0]->u = 0.0;
        quad[0]->v = 0.0;
        quad[2]->u = 0.0;
        quad[2]->v = 1.0;
        break;
    case 1: // flip x
        quad[1]->u = 0.0;
        quad[1]->v = 0.0;
        quad[0]->u = 1.0;
        quad[0]->v = 0.0;
        quad[2]->u = 1.0;
        quad[2]->v = 1.0;
        break;
    case 2: // flip y
        quad[1]->u = 1.0;
        quad[1]->v = 1.0;
        quad[0]->u = 0.0;
        quad[0]->v = 1.0;
        quad[2]->u = 0.0;
        quad[2]->v = 0.0;
        break;
    case 3: // flip x+y
        quad[1]->u = 0.0;
        quad[1]->v = 1.0;
        quad[0]->u = 1.0;
        quad[0]->v = 1.0;
        quad[2]->u = 1.0;
        quad[2]->v = 0.0;
        break;
    }

    return (page);
}

// uc_orig: build_fence_poles (fallen/DDEngine/Source/facet.cpp)
// Draws the 3D fence post columns along a wall segment as triangular prisms.
// Outputs: *rdx/*rdz = unit direction vector along the fence.
void build_fence_poles(float sx, float sy, float sz, float fdx, float fdz, SLONG count, float* rdx, float* rdz, SLONG style)
{
    float x[13], y[13], z[13];
    float dx, dz, nx, nz, dist;
    float gx, gy, gz;
    POLY_Point* quad[4];
    POLY_Point* tri[3];
    POLY_Point* pp;

    float dy;

    NIGHT_Colour col;

    col.red = 64;
    col.green = 64;
    col.blue = 64;

    x[0] = 0;
    y[0] = 0;
    z[0] = 0;

    dist = sqrt(fdx * fdx + fdz * fdz);
    if (dist == 0.0f)
        return;

    dx = (fdx) / dist;
    dz = (fdz) / dist;

    *rdx = dx;
    *rdz = dz;

    nx = (dz * 10.0f);
    nz = -(dx * 10.0f);

    x[1] = dx * 20.0f;
    y[1] = 0.0f;
    z[1] = dz * 20.0f;

    x[2] = dx * 10.0f + nx;
    y[2] = 0.0f;
    z[2] = dz * 10.0f + nz;

    gx = sx;
    gy = sy;
    gz = sz;

    while (count-- > 0) {
        SLONG c0;

        POLY_buffer_upto = 0;
        pp = &POLY_buffer[0];

        dy = grid_height_at_world(gx, gz);

        for (c0 = 0; c0 < 3; c0++) {
            POLY_transform(gx + x[c0], gy + y[c0] + dy, gz + z[c0], pp);

            if (pp->MaybeValid()) {
                NIGHT_get_d3d_colour(
                    col,
                    &pp->colour,
                    &pp->specular);
            }
            pp++;
        }

        y[2] = -10.0;
        for (c0 = 0; c0 < 3; c0++) {
            POLY_transform(gx + x[c0], gy + y[c0] + dy + 200.0f, gz + z[c0], pp);

            if (pp->MaybeValid()) {
                NIGHT_get_d3d_colour(
                    col,
                    &pp->colour,
                    &pp->specular);
            }
            pp++;
        }
        y[2] = 0.0;

        POLY_transform(gx + x[2] + nx * 5.0f, gy + y[2] + dy + 250.0f, gz + z[2] + nz * 5.0f, pp);
        if (pp->MaybeValid()) {
            NIGHT_get_d3d_colour(
                col,
                &pp->colour,
                &pp->specular);
        }
        pp++;

        {
            SLONG q, t;
            SLONG q_lookup[] = { 1, 2, 0 };

            for (q = 0; q < 3; q++) {

                quad[0] = &POLY_buffer[3 + q];
                quad[1] = &POLY_buffer[3 + q_lookup[q]];
                quad[2] = &POLY_buffer[0 + q];
                quad[3] = &POLY_buffer[0 + q_lookup[q]];

                if (POLY_valid_quad(quad)) {
                    SLONG page;
                    page = texture_quad2(quad, dstyles[style], TEXTURE_PIECE_MIDDLE2);
                    POLY_add_quad(quad, page, 1);
                }
            }

            tri[2] = &POLY_buffer[6];
            for (t = 0; t < 3; t++) {

                tri[1] = &POLY_buffer[3 + t];
                tri[0] = &POLY_buffer[3 + q_lookup[t]];

                if (POLY_valid_triangle(tri)) {
                    SLONG page = 0;
                    page = texture_tri2(tri, dstyles[style], TEXTURE_PIECE_MIDDLE2);
                    POLY_add_triangle(tri, page, 1);
                }
            }
        }

        gx += fdx;
        gz += fdz;
    }
}

// uc_orig: cable_draw (fallen/DDEngine/Source/facet.cpp)
// Renders a hanging cable/wire as billboard quads with catenary sag.
// Two render passes: first solid black (shadow), then POLY_PAGE_ENVMAP (metallic sheen).
void cable_draw(struct DFacet* p_facet)
{
    static const float width = 4.0F * 65536.0F / 3.0F;

    float x1, y1, z1;
    float x2, y2, z2;
    float dx, dy, dz;
    SLONG count;
    float cx, cy, cz;

    SLONG angle;
    SLONG dangle1;
    SLONG dangle2;
    SLONG sag;

    POLY_flush_local_rot();

    x1 = p_facet->x[0] * 256;
    y1 = p_facet->Y[0];
    z1 = p_facet->z[0] * 256;

    x2 = p_facet->x[1] * 256;
    y2 = p_facet->Y[1];
    z2 = p_facet->z[1] * 256;

    count = p_facet->Height;

    dx = (x2 - x1) / count;
    dy = (y2 - y1) / count;
    dz = (z2 - z1) / count;

    cx = x1;
    cy = y1;
    cz = z1;

    angle = -512;
    dangle1 = (SWORD)p_facet->StyleIndex;
    dangle2 = (SWORD)p_facet->Building;
    sag = p_facet->FHeight * 64;

    POLY_Point* pp;
    POLY_Point qp[4];
    POLY_Point* pqp[4];

    SLONG ii;

    POLY_buffer_upto = 0;
    pp = &POLY_buffer[0];

    float u = 0.0F;
    float du = 1.0F / count;

    for (ii = 0; ii <= count; ii++) {
        float sagy = float((COS((angle + 2048) & 2047) * sag) >> 16);

        POLY_transform(cx, cy - sagy, cz, pp);
        pp->u = u;
        pp->v = 0.0;
        NIGHT_get_d3d_colour(
            NIGHT_get_light_at(cx, cy - sagy, cz),
            &pp->colour,
            &pp->specular);

        pp++;

        cx += dx;
        cy += dy;
        cz += dz;

        angle += dangle1;
        if (angle >= -30)
            dangle1 = dangle2;

        u += du;
    }

    pqp[0] = &qp[3];
    pqp[1] = &qp[1];
    pqp[2] = &qp[2];
    pqp[3] = &qp[0];

    pp = &POLY_buffer[0];

    POLY_Point old[2];
    bool old_set = false;

    for (ii = 0; ii < count; ii++) {
        if (pp[0].IsValid() && pp[1].IsValid()) {
            POLY_create_cylinder_points(&pp[0], &pp[1], width, &qp[0]);

            qp[0].v = 0.0F;
            qp[1].v = 1.0F;
            qp[2].v = 0.0F;
            qp[3].v = 1.0F;

            if (old_set) {
                qp[0].X = old[0].X;
                qp[0].Y = old[0].Y;
                qp[1].X = old[1].X;
                qp[1].Y = old[1].Y;
            }

            if (POLY_valid_quad(pqp)) {
                qp[0].colour = 0;
                qp[1].colour = 0;
                qp[2].colour = 0;
                qp[3].colour = 0;

                POLY_add_quad(pqp, POLY_PAGE_COLOUR, false, true);

                qp[0].colour = pp[0].colour | 0xFF000000;
                qp[1].colour = pp[0].colour | 0xFF000000;
                qp[2].colour = pp[1].colour | 0xFF000000;
                qp[3].colour = pp[1].colour | 0xFF000000;
                POLY_add_quad(pqp, POLY_PAGE_ENVMAP, false, true);
            }

            old[0].X = qp[2].X;
            old[0].Y = qp[2].Y;
            old[1].X = qp[3].X;
            old[1].Y = qp[3].Y;
            old_set = true;
        } else {
            old_set = false;
        }
        pp++;
    }
}

// uc_orig: DRAW_stairs (fallen/DDEngine/Source/facet.cpp)
void DRAW_stairs(SLONG stair, SLONG storey, UBYTE fade)
{
    SLONG x, y, z;
    SLONG prim = 0;
    SLONG dir, angle;

    x = inside_stairs[stair].X << 8;
    z = inside_stairs[stair].Z << 8;

    y = inside_storeys[storey].StoreyY;

    if (inside_stairs[stair].UpInside)
        prim |= 1;

    if (inside_stairs[stair].DownInside)
        prim |= 2;

    switch (prim) {
    case 1:
        prim = 27;
        break;
    case 2:
        prim = 29;
        break;
    case 3:
        prim = 28;
        break;
    }

    dir = GET_STAIR_DIR(inside_stairs[stair].Flags);
    switch (dir) {
    case 0:
        angle = 0;
        break;
    case 1:
        angle = 2048 - 512;
        break;
    case 2:
        angle = 1024;
        break;
    case 3:
        angle = 512;
        break;
    }

    if (prim) {
        MESH_draw_poly(prim, x, y, z, angle, 0, 0, 0, fade);
    }
}

// uc_orig: draw_insides (fallen/DDEngine/Source/facet.cpp)
void draw_insides(SLONG indoor_index, SLONG room, UBYTE fade)
{
    struct InsideStorey* p_inside;
    SLONG c0;
    static int recursive = 0;
    SLONG stair;
    ASSERT(recursive == 0);

    recursive++;
    p_inside = &inside_storeys[indoor_index];
    for (c0 = p_inside->FacetStart; c0 < p_inside->FacetEnd; c0++) {
        FACET_draw(c0, fade);
    }
    stair = inside_storeys[indoor_index].StairCaseHead;
    while (stair) {
        DRAW_stairs(stair, indoor_index, fade);
        stair = inside_stairs[stair].NextStairs;
    }
    recursive--;
}

// uc_orig: DRAW_door (fallen/DDEngine/Source/facet.cpp)
// Draws a 3-panel door as a row of quads spanning the wall opening.
void DRAW_door(float sx, float sy, float sz, float fx, float fy, float fz, float block_height, SLONG count, ULONG fade_alpha, SLONG style, SLONG flipx)
{
    float dy, dx, dz;
    SLONG page;
    UBYTE flip;

    POLY_Point *pp, *ps;
    POLY_Point* quad[4];
    SLONG c0, face;

    dy = block_height * 0.7f;

    dx = fx * 0.3f;
    dz = fz * 0.3f;
    pp = &POLY_buffer[POLY_buffer_upto];
    ps = pp;

    POLY_transform(sx, sy, sz, pp++);
    POLY_transform(sx, sy + block_height, sz, pp++);
    POLY_transform(sx + fx, sy + block_height, sz + fz, pp++);
    POLY_transform(sx + fx, sy, sz + fz, pp++);
    POLY_transform(sx + fx - dx, sy, sz + fz - dz, pp++);
    POLY_transform(sx + fx - dx, sy + dy, sz + fz - dz, pp++);
    POLY_transform(sx + dx, sy + dy, sz + dz, pp++);
    POLY_transform(sx + dx, sy, sz + dz, pp++);

    POLY_buffer_upto += 8;

    for (c0 = 0; c0 < 8; c0++) {
        ps[c0].colour = 0xffffff | fade_alpha;
        ps[c0].specular = 0xff000000;
    }

    page = get_texture_page(style, 0, 1, &flip);

    ps[0].u = 0.0f;
    ps[0].v = 1.0f - 0.0f;

    ps[1].u = 0.0f;
    ps[1].v = 1.0f - 1.0f;

    ps[2].u = 1.0f;
    ps[2].v = 1.0f - 1.0f;

    ps[3].u = 1.0f;
    ps[3].v = 1.0f - 0.0f;

    ps[4].u = 0.7f;
    ps[4].v = 1.0f - 0.0f;

    ps[5].u = 0.7f;
    ps[5].v = 1.0f - 0.7f;

    ps[6].u = 0.3f;
    ps[6].v = 1.0f - 0.7f;

    ps[7].u = 0.3f;
    ps[7].v = 1.0f - 0.0f;

    if (flipx)
        for (c0 = 0; c0 < 8; c0++) {
            ps[c0].u = 1.0f - ps[c0].u;
        }

    for (face = 0; face < 3; face++) {
        for (c0 = 0; c0 < 4; c0++) {
            quad[c0] = &ps[door_poly[face][c0]];
        }
        if (POLY_valid_quad(quad)) {
            POLY_add_quad(quad, page, UC_FALSE);
        }
    }
}

// uc_orig: draw_wall_thickness (fallen/DDEngine/Source/facet.cpp)
// Draws the horizontal top-cap quad for an inside-facing wall segment.
void draw_wall_thickness(struct DFacet* p_facet, ULONG fade_alpha)
{
    POLY_Point* pp;
    POLY_Point* quad[4];
    float x1, z1;
    float x2, z2;
    float y;
    float dx, dz;

    static const ULONG colour = 0xFF8844;

    y = float(p_facet->Y[0] + 256);

    x1 = float(p_facet->x[0] << 8);
    z1 = float(p_facet->z[0] << 8);

    x2 = float(p_facet->x[1] << 8);
    z2 = float(p_facet->z[1] << 8);

    dx = x2 - x1;
    dz = z2 - z1;

    if (dz == 0) {
        dx = (dx < 0) ? -10 : +10;
    } else if (dx == 0) {
        dz = (dz < 0) ? -10 : +10;
    } else {
        float scalar = 10.0f / (float)sqrt(dx * dx + dz * dz);
        dx *= scalar;
        dz *= scalar;
    }

    pp = &POLY_buffer[POLY_buffer_upto++];

    POLY_transform(x1 - dx + dz, y, z1 - dz - dx, pp);
    pp->colour = colour | fade_alpha;
    pp->specular = 0xff000000;
    quad[0] = pp++;

    POLY_transform(x1 - dx - dz, y, z1 - dz + dx, pp);
    pp->colour = colour | fade_alpha;
    pp->specular = 0xff000000;
    quad[1] = pp++;

    POLY_transform(x2 + dx + dz, y, z2 + dz - dx, pp);
    pp->colour = colour | fade_alpha;
    pp->specular = 0xff000000;
    quad[2] = pp++;

    POLY_transform(x2 + dx - dz, y, z2 + dz + dx, pp);
    pp->colour = colour | fade_alpha;
    pp->specular = 0xff000000;
    quad[3] = pp;

    if (POLY_valid_quad(quad)) {
        POLY_add_quad(quad, POLY_PAGE_COLOUR, UC_FALSE);
    }
}

// uc_orig: FACET_barbedwire_top (fallen/DDEngine/Source/facet.cpp)
// Draws barbed wire sprites along the top edge of a fence or wall.
void FACET_barbedwire_top(struct DFacet* p_facet)
{
    float dx = (p_facet->x[1] - p_facet->x[0] << 8);
    float dz = (p_facet->z[1] - p_facet->z[0] << 8);
    float mag = sqrt((dx * dx) + (dz * dz));
    float stepx = (dx / mag) * 10;
    float stepz = (dz / mag) * 10;
    SLONG cx = p_facet->x[0] << 8;
    SLONG cz = p_facet->z[0] << 8;
    SLONG seed = 54321678;
    float base = 0;
    SLONG contour = 0;
    float block_height = 256.0;
    float height;
    ULONG normal = 0;

    POLY_set_local_rotation_none();

    if (p_facet->FacetType == STOREY_TYPE_NORMAL) {
        block_height = p_facet->BlockHeight << 4;
        height = (p_facet->Height * block_height) * 0.25f;
        normal = 1;
        contour = 90;
    } else {
        height = (64 * p_facet->Height);
    }

    while (base < mag) {

        seed *= 31415965;
        seed += 123456789;

        if (!normal) {

            if (!(p_facet->FacetFlags & FACET_FLAG_ONBUILDING)) {
                contour = PAP_calc_height_noroads(cx, cz);
            } else {
                contour = p_facet->Y[0];
            }
        }

        SPRITE_draw_tex(
            cx,
            height - 64 + ((seed >> 8) & 0xf) + contour,
            cz,
            50,
            0xffffff,
            0,
            POLY_PAGE_BARBWIRE,
            0.0, 0.0, 1.0, 1.0,
            SPRITE_SORT_NORMAL);

        base += 10;
        cx += stepx;
        cz += stepz;
    }
}

// uc_orig: MakeFacetPoints (fallen/DDEngine/Source/facet.cpp)
// Transforms a wall segment into a 2D grid of screen-space points stored in POLY_buffer[].
// Builds FacetRows[] entries: FacetRows[hf] = first buffer index in tier hf.
// 'foundation'=2: bottom vertices snap to terrain and UVs are stretched to avoid swim.
// 'hug' (HUG_FLOOR): base of each column snaps individually to terrain height.
void MakeFacetPoints(float sx, float sy, float sz, float dx, float dz, float block_height,
    SLONG height, SLONG max_height, NIGHT_Colour* col, SLONG foundation, SLONG count, SLONG invisible, SLONG hug)
{
    SLONG hf = 0;

    ASSERT(POLY_buffer_upto == 0);

    while (height >= 0) {
        float x = sx;
        float y = sy;
        float z = sz;

        FacetRows[hf] = POLY_buffer_upto;

        for (SLONG c0 = count; c0 > 0; c0--) {
            float ty;

            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            POLY_Point* pp = &POLY_buffer[POLY_buffer_upto++];

            if (hug) {
                ty = float(PAP_2HI(SLONG(x) >> 8, SLONG(z) >> 8).Alt << 3);
                ty += y;
            } else if (foundation != 2) {
                ty = y;
            } else {
                ty = float(PAP_2HI(SLONG(x) >> 8, SLONG(z) >> 8).Alt << 3);
                FacetDiffY[POLY_buffer_upto - 1] = ((y - ty) * (1.0f / 256.0f)) + 1.0f;
            }

            POLY_transform_c_saturate_z(x, ty, z, pp);

            if (pp->MaybeValid()) {
                if (INDOORS_INDEX) {
                    NIGHT_get_d3d_colour_dim(*col, &pp->colour, &pp->specular);
                } else {
                    NIGHT_get_d3d_colour(*col, &pp->colour, &pp->specular);
                }
            }

            x += dx;
            z += dz;
            col++;
        }
        sy += block_height;
        height -= 4;
        hf++;
        foundation--;
        if (sy > max_height)
            break;
    }
    FacetRows[hf] = POLY_buffer_upto;
}

// uc_orig: FillFacetPoints (fallen/DDEngine/Source/facet.cpp)
// Converts two rows of screen-space points in POLY_buffer[] into a row of textured quads.
// Applies crinkle bump-mapping when enabled and geometry is close enough.
void FillFacetPoints(SLONG count, ULONG base_row, SLONG foundation, SLONG facet_backwards, SLONG style_index, float block_height, SLONG reverse_texture)
{
    POLY_Point* quad[4];

    SLONG row1 = FacetRows[base_row];
    SLONG row2 = FacetRows[base_row + 1];

    SLONG c0;
    for (c0 = 0; c0 < row2 - row1 - 1; c0++) {
        if (facet_backwards) {
            quad[0] = &POLY_buffer[row2 + c0];
            quad[1] = &POLY_buffer[row2 + c0 + 1];
            quad[2] = &POLY_buffer[row1 + c0];
            quad[3] = &POLY_buffer[row1 + c0 + 1];
        } else {
            quad[0] = &POLY_buffer[row2 + c0 + 1];
            quad[1] = &POLY_buffer[row2 + c0];
            quad[2] = &POLY_buffer[row1 + c0 + 1];
            quad[3] = &POLY_buffer[row1 + c0];
        }

        if (POLY_valid_quad(quad)) {
            SLONG page;
            if (reverse_texture)
                page = texture_quad(quad, dstyles[style_index], count - 2 - c0, count, 1);
            else
                page = texture_quad(quad, dstyles[style_index], c0, count);

            if (block_height != 256.0f) {
                if (block_height == 192.0f) {
                    quad[2]->v = 0.75f;
                    quad[3]->v = 0.75f;
                } else if (block_height == 128.0f) {
                    quad[2]->v = 0.5f;
                    quad[3]->v = 0.5f;
                } else if (block_height == 64.0f) {
                    quad[2]->v = 0.25f;
                    quad[3]->v = 0.25f;
                }
            }

            if (foundation == 2) {
                quad[3]->v = FacetDiffY[c0];
                quad[2]->v = FacetDiffY[c0 + 1];

                if (quad[3]->v > 1.0f) {
                    quad[3]->v = 1.0f;
                }
                if (quad[2]->v > 1.0f) {
                    quad[2]->v = 1.0f;
                }
            }

            if (AENG_detail_crinkles) {
                if (page < 64 * 8) {
                    if (TEXTURE_crinkle[page]) {
                        if (quad[0]->z > 0.6F) {
                            // Too far away to crinkle.
                        } else if (quad[0]->z < 0.3F) {
                            CRINKLE_do(
                                TEXTURE_crinkle[page],
                                page,
                                1.0F,
                                quad,
                                flip);
                            goto added_crinkle;
                        } else {
                            float extrude;
                            float av_z;

                            av_z = quad[0]->z;

                            extrude = av_z - 0.5F;
                            extrude *= 1.0F / (0.4F - 0.5F);

                            if (extrude > 0.0F) {
                                if (extrude > 1.0F) {
                                    extrude = 1.0F;
                                }

                                CRINKLE_do(
                                    TEXTURE_crinkle[page],
                                    page,
                                    extrude,
                                    quad,
                                    flip);

                                goto added_crinkle;
                            }
                        }
                    }
                }
            }

            POLY_add_quad(quad, page, UC_TRUE);

        added_crinkle:;
        } else {
            facet_rand();
        }
    }
    for (; c0 < count; c0++) {
        facet_rand();
    }
}

// uc_orig: MakeFacetPointsCommon (fallen/DDEngine/Source/facet.cpp)
// Optimised version of MakeFacetPoints for the common NORMAL storey case
// (no max_height clamping, no invisible flag).
void MakeFacetPointsCommon(float sx, float sy, float sz, float dx, float dz, float block_height,
    SLONG height, NIGHT_Colour* col, SLONG foundation, SLONG count, SLONG hug)
{
    SLONG hf = 0;

    ASSERT(POLY_buffer_upto == 0);

    while (height >= 0) {
        float x = sx;
        float y = sy;
        float z = sz;

        FacetRows[hf] = POLY_buffer_upto;

        for (SLONG c0 = count; c0 > 0; c0--) {
            float ty;

            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            POLY_Point* pp = &POLY_buffer[POLY_buffer_upto++];

            if (hug) {
                ty = float(PAP_2HI(SLONG(x) >> 8, SLONG(z) >> 8).Alt << 3);
                ty += y;
            } else if (foundation != 2) {
                ty = y;
            } else {
                ty = float(PAP_2HI(SLONG(x) >> 8, SLONG(z) >> 8).Alt << 3);
                FacetDiffY[POLY_buffer_upto - 1] = ((y - ty) * (1.0f / 256.0f)) + 1.0f;
            }

            POLY_transform_c_saturate_z(x, ty, z, pp);

            if (pp->MaybeValid()) {
                if (INDOORS_INDEX) {
                    NIGHT_get_d3d_colour_dim(*col, &pp->colour, &pp->specular);
                } else {
                    NIGHT_get_d3d_colour(*col, &pp->colour, &pp->specular);
                }
            }

            x += dx;
            z += dz;
            col++;
        }
        sy += block_height;
        height -= 4;
        hf++;
        foundation--;
    }
    FacetRows[hf] = POLY_buffer_upto;
}

// uc_orig: FillFacetPointsCommon (fallen/DDEngine/Source/facet.cpp)
// Optimised version of FillFacetPoints for the common non-backwards non-reversed case.
void FillFacetPointsCommon(SLONG count, ULONG base_row, SLONG foundation, SLONG style_index, float block_height)
{
    POLY_Point* quad[4];

    SLONG row1 = FacetRows[base_row];
    SLONG row2 = FacetRows[base_row + 1];

    SLONG c0;
    for (c0 = 0; c0 < row2 - row1 - 1; c0++) {
        {
            quad[0] = &POLY_buffer[row2 + c0 + 1];
            quad[1] = &POLY_buffer[row2 + c0];
            quad[2] = &POLY_buffer[row1 + c0 + 1];
            quad[3] = &POLY_buffer[row1 + c0];
        }

        if (POLY_valid_quad(quad)) {
            SLONG page;
            page = texture_quad(quad, dstyles[style_index], c0, count);

            if (block_height != 256.0f) {
                float fTemp = (float)block_height * (1.0f / 256.0f);
                quad[2]->v = fTemp;
                quad[3]->v = fTemp;
            }

            if (foundation == 2) {
                quad[3]->v = FacetDiffY[c0];
                quad[2]->v = FacetDiffY[c0 + 1];

                POLY_Page[page].RS.WrapJustOnce();
            }

            // Pre-release: (GAME_TURN & 0x20) toggled crinkles on/off every 32 ticks
            // as a performance optimization for 1999 hardware — caused visible flickering.
            if (AENG_detail_crinkles) {
                if (page < 64 * 8) {
                    if (TEXTURE_crinkle[page]) {
                        if (quad[0]->z > 0.6F) {
                            // Too far away to crinkle.
                        } else if (quad[0]->z < 0.3F) {
                            CRINKLE_do(
                                TEXTURE_crinkle[page],
                                page,
                                1.0F,
                                quad,
                                flip);
                            goto added_crinkle_common;
                        } else {
                            float extrude;
                            float av_z;

                            av_z = quad[0]->z;

                            extrude = av_z - 0.5F;
                            extrude *= 1.0F / (0.4F - 0.5F);

                            if (extrude > 0.0F) {
                                if (extrude > 1.0F) {
                                    extrude = 1.0F;
                                }

                                CRINKLE_do(
                                    TEXTURE_crinkle[page],
                                    page,
                                    extrude,
                                    quad,
                                    flip);

                                goto added_crinkle_common;
                            }
                        }
                    }
                }
            }

            POLY_add_quad(quad, page, UC_FALSE);

        added_crinkle_common:;
        } else {
            facet_rand();
        }
    }
    for (; c0 < count; c0++) {
        facet_rand();
    }
}

// uc_orig: FACET_draw_quick (fallen/DDEngine/Source/facet.cpp)
// LOD (level-of-detail) renderer for distant buildings. Renders the entire facet
// as a single flat-coloured untextured quad pushed to z=0.99999 (back of depth buffer).
// Acts as a silhouette fill for buildings beyond the full-detail threshold.
// Only handles STOREY_TYPE_NORMAL, STOREY_TYPE_FENCE, and STOREY_TYPE_FENCE_FLAT.
// D3D-specific: the z=0.99999 trick is not portable to OpenGL/Vulkan.
void FACET_draw_quick(SLONG facet, UBYTE alpha)
{
    POLY_Point* pp;
    POLY_Point* quad[4];
    float fx1, fx2, fz1, fz2, fy1, fy2;
    ULONG col = 0xff000000;
    struct DFacet* p_facet;

    p_facet = &dfacets[facet];
    if (allow_debug_keys)
        if (Keys[KB_P1]) {
            col = 0xFF200000;
        }

    switch (p_facet->FacetType) {
    case STOREY_TYPE_FENCE:
    case STOREY_TYPE_FENCE_FLAT:
    case STOREY_TYPE_NORMAL:

        fx1 = p_facet->x[0] << 8;
        fx2 = p_facet->x[1] << 8;
        fz1 = p_facet->z[0] << 8;
        fz2 = p_facet->z[1] << 8;

        fy1 = p_facet->Y[0];
        fy2 = fy1 + p_facet->Height * p_facet->BlockHeight * 4;

        POLY_buffer_upto = 0;
        pp = &POLY_buffer[POLY_buffer_upto];

        POLY_transform(fx2, fy2, fz2, pp);
        if (pp->clip & POLY_CLIP_NEAR)
            return;
        if (pp->MaybeValid()) {
            pp->colour = col;
            pp->specular = 0;
            pp->z = 0.99999f;
            pp->Z = 0.00001f;
            pp->u = 0.5f;
            pp->v = 0.5f;
        }
        pp++;
        POLY_transform(fx1, fy2, fz1, pp);
        if (pp->clip & POLY_CLIP_NEAR)
            return;
        if (pp->MaybeValid()) {
            pp->colour = col;
            pp->specular = 0;
            pp->z = 0.99999f;
            pp->Z = 0.00001f;
            pp->u = 0.5f;
            pp->v = 0.5f;
        }
        pp++;
        POLY_transform(fx2, fy1, fz2, pp);
        if (pp->clip & POLY_CLIP_NEAR)
            return;
        if (pp->MaybeValid()) {
            pp->colour = col;
            pp->specular = 0;
            pp->z = 0.99999f;
            pp->Z = 0.00001f;
            pp->u = 0.5f;
            pp->v = 0.5f;
        }
        pp++;
        POLY_transform(fx1, fy1, fz1, pp);
        if (pp->clip & POLY_CLIP_NEAR)
            return;
        if (pp->MaybeValid()) {
            pp->colour = col;
            pp->specular = 0;
            pp->z = 0.99999f;
            pp->Z = 0.00001f;
            pp->u = 0.5f;
            pp->v = 0.5f;
        }
        pp++;

        quad[0] = &POLY_buffer[0];
        quad[1] = &POLY_buffer[1];
        quad[2] = &POLY_buffer[2];
        quad[3] = &POLY_buffer[3];

        // No back-face cull and no clip flag re-generation:
        // clip flags are already correct, and back-face culling breaks building
        // tops which have no plotted geometry.
        if (POLY_valid_quad(quad))
            POLY_add_quad(quad, POLY_PAGE_COLOUR_WITH_FOG, 0, 0);
        break;
    }
}

// uc_orig: FACET_draw_rare (fallen/DDEngine/Source/facet.cpp)
// Slow-path renderer for all non-plain-wall facet types. Called by FACET_draw
// when any of the following conditions hold:
//   - Player is indoors (INDOORS_INDEX != 0)
//   - FACET_FLAG_BARB_TOP, FACET_FLAG_2SIDED, or FACET_FLAG_INSIDE are set
//   - FacetType != STOREY_TYPE_NORMAL (fence, door, cable, inside-wall, ladder)
// Dispatches to cable_draw() for cables, DRAW_door() for inside doors,
// MakeFacetPoints/FillFacetPoints for fences/inside walls, DRAW_ladder for ladders.
void FACET_draw_rare(SLONG facet, UBYTE alpha)
{
    struct DFacet* p_facet;
    static SWORD rows[100];
    SLONG c0, count;
    SLONG dx, dz;
    float x, y, z, sx, sy, sz, fdx, fdz;
    SLONG height;
    POLY_Point* pp;
    SLONG hf;
    POLY_Point* quad[4];
    SLONG style_index;
    NIGHT_Colour* col;
    SLONG max_height;
    SLONG foundation = 0;
    static float diff_y[128];
    float block_height = 256.0;

    SLONG diag = 0;
    SLONG facet_backwards = 0;
    ULONG fade_alpha = alpha << 24;
    SLONG inside_clip = 0;
    SLONG reverse_textures = 0;
    SLONG style_index_offset = 1;
    SLONG style_index_step = 2;
    SLONG flipx = 0;

// uc_orig: MAX_SHAKE (fallen/DDEngine/Source/facet.cpp)
#define MAX_SHAKE 32

    POLY_set_local_rotation_none();
    POLY_flush_local_rot();

    // Shake displacement table used for electrified/damaged fence vibration.
    // uc_orig: shake (fallen/DDEngine/Source/facet.cpp)
    static float shake[MAX_SHAKE] = {
        0,  0,  0,  0,
        +1, -2, +1, -1,
        +3, -3, +2, -2,
        +4, -3, +2, -4,
        +5, -5, +5, -4,
        +6, -4, +7, -6,
        +7, -6, +8, -9,
        +9, -7, +8, -9,
    };

    ASSERT(facet > 0 && facet < next_dfacet);
    p_facet = &dfacets[facet];

    if (p_facet->FacetFlags & FACET_FLAG_INVISIBLE) {
        return;
    }

    if (INDOORS_DBUILDING == p_facet->Building && INDOORS_INDEX)
        inside_clip = 1;

    // Cable facets use their own self-contained draw path.
    switch (p_facet->FacetType) {
    case STOREY_TYPE_CABLE:
        cable_draw(p_facet);
        return;
        break;
    }

    // Double-sided and symmetrical facet types cannot be back-face culled.
    if ((p_facet->FacetType == STOREY_TYPE_FENCE || p_facet->FacetType == STOREY_TYPE_FENCE_FLAT || p_facet->FacetType == STOREY_TYPE_FENCE_BRICK || p_facet->FacetType == STOREY_TYPE_INSIDE ||
            p_facet->FacetType == STOREY_TYPE_INSIDE_DOOR || p_facet->FacetType == STOREY_TYPE_OUTSIDE_DOOR || p_facet->FacetType == STOREY_TYPE_LADDER)
        && !(p_facet->FacetFlags & FACET_FLAG_2SIDED)) {
        // These are inherently double-sided.
    } else if (p_facet->FacetType == STOREY_TYPE_JUST_COLLISION) {
        return;
    } else {
        // 2D cross-product back-face cull.
        float x1, z1;
        float x2, z2;
        float vec1x, vec1z;
        float vec2x, vec2z;
        float cprod;

        x1 = float(p_facet->x[0] << 8);
        z1 = float(p_facet->z[0] << 8);
        x2 = float(p_facet->x[1] << 8);
        z2 = float(p_facet->z[1] << 8);

        vec1x = x2 - x1;
        vec1z = z2 - z1;
        vec2x = POLY_cam_x - x1;
        vec2z = POLY_cam_z - z1;

        cprod = vec1x * vec2z - vec1z * vec2x;

        if (cprod >= 0) {
            if ((p_facet->FacetFlags & FACET_FLAG_2SIDED) || p_facet->FacetType == STOREY_TYPE_OINSIDE) {
                facet_backwards = 1;
            } else {
                if (p_facet->FacetFlags & FACET_FLAG_BARB_TOP)
                    FACET_barbedwire_top(p_facet);
                return;
            }
        } else {
            if (p_facet->FacetType == STOREY_TYPE_OINSIDE) {
                draw_wall_thickness(p_facet, fade_alpha);
                return;
            }
        }
    }

    // AABB frustum rejection (skipped for OUTSIDE_DOOR which may be rotated open).
    if (p_facet->FacetType == STOREY_TYPE_OUTSIDE_DOOR) {
        // Can't reject: door rotation changes the AABB.
    } else {
        SLONG i;
        ULONG clip, clip_and, clip_or;
        POLY_Point bound;
        float x, y, z;

        float x1 = float(p_facet->x[0] << 8);
        float y1 = float(p_facet->Y[0]);
        float z1 = float(p_facet->z[0] << 8);
        float x2 = float(p_facet->x[1] << 8);
        float y2 = float(p_facet->Y[1]) + float(p_facet->Height * 64);
        float z2 = float(p_facet->z[1] << 8);

        clip_or = 0x00000000;
        clip_and = 0xffffffff;

        for (i = 0; i < 4; i++) {
            x = (i & 0x1) ? x1 : x2;
            y = (i & 0x2) ? y1 : y2;
            z = (i & 0x1) ? z1 : z2;

            POLY_transform_c_saturate_z(x, y, z, &bound);

            clip = bound.clip;

            if ((clip & POLY_CLIP_TRANSFORMED) && !(clip & POLY_CLIP_OFFSCREEN)) {
                goto draw_the_facet;
            }

            clip_and &= clip;
            clip_or |= clip;
        }

        if (clip_and & POLY_CLIP_OFFSCREEN) {
            return;
        }

        if (!(clip_or & POLY_CLIP_TRANSFORMED)) {
            if (clip_and & (POLY_CLIP_NEAR | POLY_CLIP_FAR)) {
                return;
            }
        }

    draw_the_facet:;
    }

    dfacets_drawn_this_gameturn += 1;

    if (p_facet->FacetType == STOREY_TYPE_LADDER) {
        POLY_flush_local_rot();
        DRAW_ladder(p_facet);
        return;
    }

    POLY_buffer_upto = 0;

    style_index = p_facet->StyleIndex;

    set_facet_seed(p_facet->x[0] * p_facet->z[0] + p_facet->Y[0]);

    if (GAME_FLAGS & GF_INDOORS) {
        max_height = INDOORS_HEIGHT_CEILING;
    } else {
        max_height = UC_INFINITY;
    }

    // Create per-facet lighting cache on first use.
    if (p_facet->Dfcache == 0) {
        p_facet->Dfcache = NIGHT_dfcache_create(facet);
        if (p_facet->Dfcache == NULL) {
            return;
        }
    }

    ASSERT(WITHIN(p_facet->Dfcache, 1, NIGHT_MAX_DFCACHES - 1));

    col = NIGHT_dfcache[p_facet->Dfcache].colour;

    dx = (p_facet->x[1] - p_facet->x[0]) << 8;
    dz = (p_facet->z[1] - p_facet->z[0]) << 8;

    sx = float(p_facet->x[0] << 8);
    sy = float(p_facet->Y[0]);
    sz = float(p_facet->z[0] << 8);

    height = p_facet->Height;

    if (dx && dz) {
        // Diagonal facet: compute approximate length and step per unit.
        if (p_facet->FacetType == STOREY_TYPE_NORMAL) {
            ASSERT(0);
        }

        {
            SLONG len;
            SLONG adx, adz;

            adx = abs(dx);
            adz = abs(dz);
            len = QDIST2(adx, adz);
            count = len >> 8;
            if (count == 0)
                count = 1;

            fdx = (float)dx / (float)count;
            fdz = (float)dz / (float)count;

            diag = 0;
        }
    } else {
        // Axis-aligned: normalize step to +/-256 world units.
        if (dx) {
            count = abs(dx) >> 8;
            if (dx > 0)
                dx = 256;
            else
                dx = -256;
        } else {
            count = abs(dz) >> 8;
            if (dz > 0)
                dz = 256;
            else
                dz = -256;
        }

        fdx = (float)dx;
        fdz = (float)dz;

        // Rotate direction for open doors.
        if (p_facet->Open) {
            float rdx, rdz;
            float angle = float(p_facet->Open) * (PI / 256.0F);

            rdx = cos(angle) * fdx + sin(angle) * fdz;
            rdz = -sin(angle) * fdx + cos(angle) * fdz;

            fdx = rdx;
            fdz = rdz;
        }
    }

    count++;

    // Pre-compute per-column terrain height offsets for ground-hugging fence types.
    switch (p_facet->FacetType) {
    case STOREY_TYPE_FENCE:
    case STOREY_TYPE_FENCE_FLAT:
    case STOREY_TYPE_FENCE_BRICK:
    case STOREY_TYPE_OUTSIDE_DOOR:
    {
        float* p_diffy;
        float dy;

        p_diffy = diff_y;

        x = sx;
        y = sy;
        z = sz;
        c0 = count;
        while (c0-- > 0) {
            if (p_facet->FacetFlags & FACET_FLAG_ONBUILDING) {
                *p_diffy++ = p_facet->Y[0];
            } else {
                if (diag) {
                    dy = (float)PAP_calc_height_noroads((SLONG)x, (SLONG)z);
                } else {
                    dy = grid_height_at_world(x, z);
                }
                *p_diffy = dy;
                p_diffy++;
                x += fdx;
                z += fdz;
            }
        }
    } break;
    }

    switch (p_facet->FacetType) {
    case STOREY_TYPE_CABLE:
        cable_draw(p_facet);
        break;

    case STOREY_TYPE_INSIDE_DOOR:
        if (facet_backwards && (p_facet->FacetFlags & FACET_FLAG_2SIDED))
            style_index++;
        if (facet_backwards) {
            DRAW_door(sx, sy, sz, fdx, 256.0, fdz, block_height, count, fade_alpha, dstyles[style_index], 1);
        } else {
            DRAW_door(sx, sy, sz, fdx, 256.0, fdz, block_height, count, fade_alpha, dstyles[style_index], 0);
        }
        break;

    case STOREY_TYPE_OINSIDE:
        style_index--;
        // FALLTHROUGH to STOREY_TYPE_INSIDE

    case STOREY_TYPE_INSIDE:
        draw_wall_thickness(p_facet, fade_alpha);
        if (facet_backwards) {
            flipx = 1;
            if (!ShiftFlag)
                style_index++;
        }

        hf = 0;
        while (height >= 0) {
            x = sx;
            y = sy;
            z = sz;
            rows[hf] = POLY_buffer_upto;
            c0 = count;
            while (c0-- > 0) {
                ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

                pp = &POLY_buffer[POLY_buffer_upto++];
                POLY_transform(x, y, z, pp);

                if (pp->MaybeValid()) {
                    NIGHT_get_d3d_colour(*col, &pp->colour, &pp->specular);
                    pp->colour |= fade_alpha;
                }

                x += fdx;
                z += fdz;
                col += 1;
            }

            if (hf > 0) {
                SLONG row1, row2;
                row1 = rows[hf - 1];
                row2 = rows[hf];

                c0 = 0;
                while (c0 < count - 1) {
                    quad[0] = &POLY_buffer[row2 + c0 + 1];
                    quad[1] = &POLY_buffer[row2 + c0];
                    quad[2] = &POLY_buffer[row1 + c0 + 1];
                    quad[3] = &POLY_buffer[row1 + c0];

                    if (POLY_valid_quad(quad)) {
                        SLONG page;
                        page = texture_quad(quad, dstyles[style_index], c0, count, flipx);

                        if (foundation == 1) {
                            quad[3]->v = diff_y[c0] / 256.0f;
                            quad[2]->v = diff_y[c0 + 1] / 256.0f;
                        }

                        POLY_add_quad(quad, page, UC_FALSE);
                    } else {
                        facet_rand();
                    }

                    c0++;
                }
            }

            foundation--;
            sy += block_height;
            height -= 4;
            hf += 1;

            if (sy > max_height) {
                break;
            }
        }
        break;

    case STOREY_TYPE_TRENCH:
    case STOREY_TYPE_NORMAL:
    case STOREY_TYPE_DOOR:

        if (p_facet->FacetFlags & FACET_FLAG_BARB_TOP)
            FACET_barbedwire_top(p_facet);

        // Warehouses can be double-sided STOREY_TYPE_NORMAL.
        if (facet_backwards && !(p_facet->FacetFlags & FACET_FLAG_HUG_FLOOR)) {
            style_index++;
        }

        block_height = p_facet->BlockHeight << 4;

        if (inside_clip) {
            SLONG top;
            top = block_height;
            top = (top * height) >> 2;
            top += sy;
            if (top >= (inside_storeys[INDOORS_INDEX].StoreyY + 256)) {
                {
                    height -= ((top + 4) - (inside_storeys[INDOORS_INDEX].StoreyY + 256)) / (p_facet->BlockHeight << 2);
                }
            }
        }

        if (p_facet->FHeight)
            foundation = 2;

        MakeFacetPoints(sx, sy, sz, fdx, fdz, block_height, height, max_height, col, foundation, count, p_facet->FacetFlags & (FACET_FLAG_INVISIBLE), p_facet->FacetFlags & FACET_FLAG_HUG_FLOOR);

        if (p_facet->FacetFlags & (FACET_FLAG_INSIDE)) {
            reverse_textures = 1;
            style_index--;
        } else if (p_facet->FacetFlags & (FACET_FLAG_2TEXTURED)) {
            style_index--;
        }

        if (!(p_facet->FacetFlags & FACET_FLAG_HUG_FLOOR) && (p_facet->FacetFlags & (FACET_FLAG_2TEXTURED | FACET_FLAG_2SIDED))) {
            style_index_offset = 1;
            style_index_step = 2;
        } else {
            style_index_offset = 1;
            style_index_step = 1;
        }

        hf = 0;
        while (height >= 0) {
            if (hf) {
                FillFacetPoints(count, hf - 1, foundation + 1, facet_backwards, style_index - style_index_offset, block_height, reverse_textures);
            }

            foundation--;
            sy += block_height;
            height -= 4;
            hf++;
            style_index += style_index_step;

            if (sy > max_height) {
                break;
            }
        }
        break;

    case STOREY_TYPE_FENCE_BRICK:
        // Hardwire height for barbwire pass, then fall through to FENCE.
        p_facet->Height += 1;
        FACET_barbedwire_top(p_facet);
        p_facet->Height -= 1;
        // FALLTHROUGH

    case STOREY_TYPE_FENCE:
        POLY_set_local_rotation_none();

        if (p_facet->FacetType == STOREY_TYPE_FENCE) {
            // Build the angled cap quad at the top of the wire fence.
            float dx, dz, nx, nz;
            float tsx, tsy, tsz;
            sy = 0;
            build_fence_poles(sx, sy, sz, fdx, fdz, count, &dx, &dz, style_index);

            tsx = sx;
            tsy = sy;
            tsz = sz;

            nx = dz;
            nz = -dx;

            sx += nx * 10.0f;
            sz += nz * 10.0f;
            sy += 210.0f;

            hf = 0;
            while (hf <= 1) {
                float* p_diffy = diff_y;
                x = sx;
                y = sy;
                z = sz;
                rows[hf] = POLY_buffer_upto;
                c0 = count;
                while (c0-- > 0) {
                    float dy = 0;

                    ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

                    pp = &POLY_buffer[POLY_buffer_upto++];

                    dy = *p_diffy++;

                    POLY_transform_c_saturate_z(x, y + dy, z, pp);

                    if (pp->MaybeValid()) {
                        NIGHT_get_d3d_colour(*col, &pp->colour, &pp->specular);
                    }

                    pp->colour |= fade_alpha;
                    x += fdx;
                    z += fdz;
                }
                sy += 40.0f;
                sx += nx * 50.0f;
                sz += nz * 50.0f;
                hf++;
            }

            {
                SLONG row1, row2;
                row1 = rows[0];
                row2 = rows[1];

                c0 = 0;
                while (c0 < count - 1) {
                    quad[0] = &POLY_buffer[row2 + c0 + 1];
                    quad[1] = &POLY_buffer[row2 + c0];
                    quad[2] = &POLY_buffer[row1 + c0 + 1];
                    quad[3] = &POLY_buffer[row1 + c0];

                    if (POLY_valid_quad(quad)) {
                        SLONG page;
                        page = texture_quad(quad, dstyles[style_index], c0, count);
                        POLY_add_quad(quad, page, 0);
                    } else {
                        facet_rand();
                    }

                    c0++;
                }
            }

            sx = tsx;
            sy = tsy;
            sz = tsz;

            height = 3; // fence body is 3/4 of full height
        }

        // Proceed to draw the fence body panels.

    case STOREY_TYPE_OUTSIDE_DOOR:
    case STOREY_TYPE_FENCE_FLAT:

        hf = 0;

        if (p_facet->Shake) {
            p_facet->Shake -= p_facet->Shake >> 2;
            p_facet->Shake -= 1;
        }

        // Fences start at y=0 and ride terrain via diff_y[].
        sy = 0;
        y = sy;

        while (hf <= 1) {
            float* p_diffy = diff_y;
            x = sx;
            z = sz;
            rows[hf] = POLY_buffer_upto;
            c0 = count;
            while (c0-- > 0) {
                float dy = 0;

                ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

                pp = &POLY_buffer[POLY_buffer_upto++];

                dy = *p_diffy++;

                POLY_transform_c_saturate_z(
                    x + shake[(Random() & 3) + ((p_facet->Shake >> 3) & ~0x3)],
                    y + dy,
                    z + shake[(Random() & 3) + ((p_facet->Shake >> 3) & ~0x3)],
                    pp);

                if (pp->MaybeValid()) {
                    NIGHT_get_d3d_colour(*col, &pp->colour, &pp->specular);
                    pp->colour |= fade_alpha;
                }

                x += fdx;
                z += fdz;
                col += 1;
            }
            if (height == 2)
                y += 102; // 256.0 / 3.0
            else
                y += height * BLOCK_SIZE;

            hf++;
        }

        {
            SLONG row1, row2;
            row1 = rows[0];
            row2 = rows[1];

            c0 = 0;
            while (c0 < count - 1) {
                quad[0] = &POLY_buffer[row2 + c0 + 1];
                quad[1] = &POLY_buffer[row2 + c0];
                quad[2] = &POLY_buffer[row1 + c0 + 1];
                quad[3] = &POLY_buffer[row1 + c0];

                {
                    if (POLY_valid_quad(quad)) {
                        SLONG page;
                        page = texture_quad(quad, dstyles[style_index], c0, count);
                        POLY_add_quad(quad, page, 0);
                    } else {
                        facet_rand();
                    }
                }

                c0++;
            }
        }

        break;

        break;
    }
    return;
}

// uc_orig: FACET_draw (fallen/DDEngine/Source/facet.cpp)
// Fast-path renderer for STOREY_TYPE_NORMAL plain wall facets.
// Skips if invisible, INDOORS, or flags require special handling (redirects to FACET_draw_rare).
// Performs 2D backface cull + AABB frustum reject before transforming geometry.
// Uses cached per-vertex lighting (NIGHT_dfcache), then MakeFacetPointsCommon + FillFacetPointsCommon.
void FACET_draw(SLONG facet, UBYTE alpha)
{
    struct DFacet* p_facet;
    SLONG count;
    SLONG dx, dz;
    float sx, sy, sz, fdx, fdz;
    SLONG height;
    SLONG hf;
    SLONG style_index;
    NIGHT_Colour* col;
    SLONG foundation = 0;
    float block_height = 256.0;

    // SLONG		style_index_offset=1;
    SLONG style_index_step = 2;

    ASSERT(facet > 0 && facet < next_dfacet);
    p_facet = &dfacets[facet];

    if (p_facet->FacetFlags & FACET_FLAG_INVISIBLE) {
        return;
    }

    // Alpha seems to be never used, and always set to 0
    ASSERT(alpha == 0);

    // Now spot the odder forms of facet, and use the _rare routine to draw them instead.
    if ((p_facet->FacetType != STOREY_TYPE_NORMAL) || (INDOORS_INDEX) || ((p_facet->FacetFlags & (FACET_FLAG_BARB_TOP | FACET_FLAG_2SIDED | FACET_FLAG_INSIDE)) != 0)) {
        // Use it!
        FACET_draw_rare(facet, alpha);
        return;
    }

    ASSERT((p_facet->FacetType == STOREY_TYPE_NORMAL));
    ASSERT((p_facet->FacetFlags & FACET_FLAG_INVISIBLE) == 0);
    ASSERT((p_facet->FacetFlags & FACET_FLAG_BARB_TOP) == 0);
    ASSERT((p_facet->FacetFlags & FACET_FLAG_2SIDED) == 0);
    ASSERT((p_facet->FacetFlags & FACET_FLAG_INSIDE) == 0);
    ASSERT(!INDOORS_INDEX);

    //
    // Should we bother drawing this facet?
    //

    {
        //
        // Backface cull the entire facet?
        //

        float x1, z1;
        float x2, z2;

        float vec1x;
        float vec1z;

        float vec2x;
        float vec2z;

        float cprod;

        x1 = float(p_facet->x[0] << 8);
        z1 = float(p_facet->z[0] << 8);

        x2 = float(p_facet->x[1] << 8);
        z2 = float(p_facet->z[1] << 8);

        vec1x = x2 - x1;
        vec1z = z2 - z1;

        vec2x = POLY_cam_x - x1;
        vec2z = POLY_cam_z - z1;

        cprod = vec1x * vec2z - vec1z * vec2x;

        if (cprod >= 0) {
            //
            // We've got rid of a whole facet :o)
            //
            return;
        }
    }

    //
    // Transform the bounding box of the facet to quickly try and reject the
    // entire facet.
    //

    ULONG clip_or; // Used below.
    {

        SLONG i;

        ULONG clip;
        ULONG clip_and;

        POLY_Point bound;

        float x;
        float y;
        float z;

        float x1 = float(p_facet->x[0] << 8);
        float y1 = float(p_facet->Y[0]);
        float z1 = float(p_facet->z[0] << 8);

        float x2 = float(p_facet->x[1] << 8);
        float y2 = float(p_facet->Y[1]) + float(p_facet->Height * 64);
        float z2 = float(p_facet->z[1] << 8);

        clip_or = 0x00000000;
        clip_and = 0xffffffff;

        for (i = 0; i < 4; i++) {
            x = (i & 0x1) ? x1 : x2;
            y = (i & 0x2) ? y1 : y2;
            z = (i & 0x1) ? z1 : z2;

            POLY_transform_c_saturate_z(x, y, z, &bound);

            clip = bound.clip;

            if ((clip & POLY_CLIP_TRANSFORMED) && !(clip & POLY_CLIP_OFFSCREEN)) {
                //
                // Draw the whole facet because this point is on-screen.
                //

                // But this frags the near-plane clip detection.
                // Don't need it any more. Hooray!
                goto draw_the_facet_common;
            }

            clip_and &= clip;
            clip_or |= clip;
        }

        if (clip_and & POLY_CLIP_OFFSCREEN) {
            //
            // Reject the whole facet.
            //

            return;
        }

        if (!(clip_or & POLY_CLIP_TRANSFORMED)) {
            //
            // Reject the whole facet if all points are too far or all points are too near
            //
            if (clip_and & (POLY_CLIP_NEAR | POLY_CLIP_FAR)) {
                return;
            }
        }

    draw_the_facet_common:;
    }

    dfacets_drawn_this_gameturn += 1;

    {
        float yaw;

        if (p_facet->z[0] == p_facet->z[1]) {
            if (p_facet->x[0] < p_facet->x[1]) {
                yaw = 0.0F;
            } else {
                yaw = PI;
            }
        } else {
            if (p_facet->z[0] > p_facet->z[1]) {
                yaw = 3 * PI / 2;
            } else {
                yaw = PI / 2;
            }
        }

        MATRIX_calc(
            FACET_direction_matrix,
            yaw,
            0.0F,
            0.0F);
    }

    // Can't do these for release yet - no glowing windows, and the fog doesn't work.
    // Fog works, and Mark says he's done the glowing windows. Hooray!
    // #define DO_SUPERFACETS_PLEASE_BOB defined

    //
    // Draw the facet.
    //

    p_facet->FacetFlags &= ~FACET_FLAG_DLIT;

    POLY_buffer_upto = 0;

    style_index = p_facet->StyleIndex;

    //
    // Should this be passed an x,y,z to be relative to? Nah!
    //

    set_facet_seed(p_facet->x[0] * p_facet->z[0] + p_facet->Y[0]);

    ASSERT((GAME_FLAGS & GF_INDOORS) == 0);

    //
    // If there is no cached lighting for this facet, then we
    // must make some.
    //

    if (p_facet->Dfcache == 0) {
        p_facet->Dfcache = NIGHT_dfcache_create(facet);

        if (p_facet->Dfcache == NULL) {
            return;
        }
    }

    ASSERT(WITHIN(p_facet->Dfcache, 1, NIGHT_MAX_DFCACHES - 1));

    col = NIGHT_dfcache[p_facet->Dfcache].colour;

    dx = (p_facet->x[1] - p_facet->x[0]) << 8;
    dz = (p_facet->z[1] - p_facet->z[0]) << 8;

    sx = float(p_facet->x[0] << 8);
    sy = float(p_facet->Y[0]);
    sz = float(p_facet->z[0] << 8);

    height = p_facet->Height;

    // No diagonal walls allowed.
    ASSERT(!(dx && dz));

    {
        if (dx) {
            count = abs(dx) >> 8;
            if (dx > 0)
                dx = 256;
            else
                dx = -256;
        } else {
            count = abs(dz) >> 8;
            if (dz > 0)
                dz = 256;
            else
                dz = -256;
        }

        fdx = (float)dx;
        fdz = (float)dz;

        if (p_facet->Open) {
            float rdx;
            float rdz;
            float angle = float(p_facet->Open) * (PI / 256.0F);

            //
            // Open the facet!
            //

            rdx = cos(angle) * fdx + sin(angle) * fdz;
            rdz = -sin(angle) * fdx + cos(angle) * fdz;

            fdx = rdx;
            fdz = rdz;
        }
    }

    count++;

    ASSERT(p_facet->FacetType == STOREY_TYPE_NORMAL);

    ASSERT((p_facet->FacetFlags & FACET_FLAG_BARB_TOP) == 0);

    //
    //	warehouses can be double sided and storey_type_normal
    //

    ASSERT(facet_backwards == 0);

    block_height = p_facet->BlockHeight << 4;

    ASSERT(inside_clip == 0);

    if (p_facet->FHeight) {
        foundation = 2;
    }

    MakeFacetPointsCommon(sx, sy, sz, fdx, fdz, block_height, height, col, foundation, count, p_facet->FacetFlags & FACET_FLAG_HUG_FLOOR);

    ASSERT((p_facet->FacetFlags & (FACET_FLAG_INSIDE)) == 0);
    if (p_facet->FacetFlags & (FACET_FLAG_2TEXTURED)) {
        style_index--;
    }

    if (!(p_facet->FacetFlags & FACET_FLAG_HUG_FLOOR) && (p_facet->FacetFlags & (FACET_FLAG_2TEXTURED | FACET_FLAG_2SIDED))) {
        style_index_step = 2;
    } else {
        style_index_step = 1;
    }

    ASSERT(reverse_textures == 0);

    hf = 0;
    while (height >= 0) {
        if (hf) {
            ASSERT(facet_backwards == 0);
            ASSERT(reverse_textures == 0);
            FillFacetPointsCommon(count, hf - 1, foundation + 1, style_index - 1, block_height);
        }

        foundation--;
        sy += block_height;
        height -= 4;
        hf++;
        style_index += style_index_step;
    }

    return;
}

// Forward declarations for ladder helpers (file-private, called only from DRAW_ladder).
// uc_orig: DRAW_ladder_rungs (fallen/DDEngine/Source/facet.cpp)
static void DRAW_ladder_rungs(float x1, float z1, float x2, float z2, struct DFacet* p_facet, float dx, float dz, ULONG colour, ULONG specular);
// uc_orig: DRAW_ladder_sides (fallen/DDEngine/Source/facet.cpp)
static void DRAW_ladder_sides(float x1, float z1, float x2, float z2, struct DFacet* p_facet, float dx, float dz, ULONG colour, ULONG specular);

// uc_orig: FACET_draw_walkable (fallen/DDEngine/Source/facet.cpp)
// Draws the roof/walkable surface geometry for a building.
// Iterates DWalkable linked list for the building and renders each RoofFace4 quad.
// Warehouse roofs use the per-face rooftex array; non-warehouse roofs use the PAP hi-map texture.
// Shadow sub-quads are split into 2 triangles using the RFACE_FLAG_SHADOW_* pattern.
void FACET_draw_walkable(SLONG build)
{
    SLONG i;
    SLONG j;

    SLONG red;
    SLONG green;
    SLONG blue;

    SLONG page;
    SLONG warehouse;

    SLONG walkable;

    struct RoofFace4* p_f4;
    struct DWalkable* p_walk;
    struct DBuilding* p_dbuilding;

    POLY_Point* pp;

    POLY_Point* tri[3];
    POLY_Point* quad[4];

    ASSERT(WITHIN(build, 1, next_dbuilding - 1));

    p_dbuilding = &dbuildings[build];

    UWORD* rooftex = NULL;

    warehouse = (p_dbuilding->Type == BUILDING_TYPE_WAREHOUSE);

    if (warehouse) {
        ASSERT(WITHIN(p_dbuilding->Ware, 0, WARE_ware_upto - 1));

        rooftex = &WARE_rooftex[WARE_ware[p_dbuilding->Ware].rooftex];

        // FONT2D_DrawString("Warehouse walkables", 50 + (rand() & 0xf), 50, 0xffffff);

    } else {
        // FONT2D_DrawString("Non-warehouse walkables", 50 + (rand() & 0xf), 70, 0xffffff);
    }

    for (walkable = p_dbuilding->Walkable; walkable; walkable = p_walk->Next) {
        p_walk = &dwalkables[walkable];

        {
            POLY_buffer_upto = 0;
            POLY_shadow_upto = 0;

            for (i = p_walk->StartFace4; i < p_walk->EndFace4; i++, rooftex++) {
                float px, py, pz, sy;
                p_f4 = &roof_faces4[i];

                if (p_f4->DrawFlags & RFACE_FLAG_NODRAW) {
                    continue;
                }

                pp = &POLY_buffer[0];

                px = (float)((p_f4->RX & 127) << 8);
                pz = (float)((p_f4->RZ & 127) << 8);
                py = (float)(p_f4->Y);
                sy = py;
                POLY_transform(px, py, pz, pp);

                if (pp->MaybeValid()) {
                    NIGHT_get_d3d_colour(
                        NIGHT_ROOF_WALKABLE_POINT(i, 0),
                        &pp->colour,
                        &pp->specular);

                    // apply_cloud((SLONG)px,(SLONG)py,(SLONG) pz,&pp->colour);

                    // POLY_fadeout_point(pp);
                }
                pp++;
                px += 256.0f;
                py += p_f4->DY[0] << ROOF_SHIFT;
                POLY_transform(px, py, pz, pp);

                if (pp->MaybeValid()) {
                    NIGHT_get_d3d_colour(
                        NIGHT_ROOF_WALKABLE_POINT(i, 1),
                        &pp->colour,
                        &pp->specular);

                    // apply_cloud((SLONG)px,(SLONG)py,(SLONG) pz,&pp->colour);

                    // POLY_fadeout_point(pp);
                }
                pp++;
                pz += 256.0f;
                py = sy + (p_f4->DY[1] << ROOF_SHIFT);
                POLY_transform(px, py, pz, pp);

                if (pp->MaybeValid()) {
                    NIGHT_get_d3d_colour(
                        NIGHT_ROOF_WALKABLE_POINT(i, 3),
                        &pp->colour,
                        &pp->specular);

                    // apply_cloud((SLONG)px,(SLONG)py,(SLONG) pz,&pp->colour);

                    // POLY_fadeout_point(pp);
                }
                pp++;
                px -= 256.0f;
                py = sy + (p_f4->DY[2] << ROOF_SHIFT);
                POLY_transform(px, py, pz, pp);

                if (pp->MaybeValid()) {
                    NIGHT_get_d3d_colour(
                        NIGHT_ROOF_WALKABLE_POINT(i, 2),
                        &pp->colour,
                        &pp->specular);

                    // apply_cloud((SLONG)px,(SLONG)py,(SLONG) pz,&pp->colour);

                    // POLY_fadeout_point(pp);
                }

                quad[0] = &POLY_buffer[0];
                quad[1] = &POLY_buffer[1];
                quad[2] = &POLY_buffer[3];
                quad[3] = &POLY_buffer[2];

                if (POLY_valid_quad(quad)) {
                    //					#if DRAW_THIS_DEBUG_STUFF

                    if (ControlFlag && allow_debug_keys) {
                        SLONG x, z, y;

                        x = (p_f4->RX & 127) << 8;
                        z = (p_f4->RZ & 127) << 8;
                        y = p_f4->Y;

                        if (p_f4->DrawFlags & (RFACE_FLAG_SLIDE_EDGE_0)) {
                            AENG_world_line(x, y, z, 4, 0xffffff, x + 256, y, z, 4, 0xffffff, 1);
                        }
                        if (p_f4->DrawFlags & (RFACE_FLAG_SLIDE_EDGE_1)) {
                            AENG_world_line(x + 256, y, z, 4, 0xffffff, x + 256, y, z + 256, 4, 0xffffff, 1);
                        }
                        if (p_f4->DrawFlags & (RFACE_FLAG_SLIDE_EDGE_2)) {
                            AENG_world_line(x + 256, y, z + 256, 4, 0xffffff, x, y, z + 256, 4, 0xffffff, 1);
                        }
                        if (p_f4->DrawFlags & (RFACE_FLAG_SLIDE_EDGE_3)) {
                            AENG_world_line(x, y, z + 256, 4, 0xffffff, x, y, z, 4, 0xffffff, 1);
                        }
                    }

                    //					#endif

                    if (warehouse) {
                        // If this face is above the camera it must be the ceiling — draw it black.
                        if (p_f4->Y > (FC_cam[0].y >> 8)) {
                            quad[0]->colour = 0x00000000;
                            quad[0]->specular = 0xff000000;

                            quad[1]->colour = 0x00000000;
                            quad[1]->specular = 0xff000000;

                            quad[2]->colour = 0x00000000;
                            quad[2]->specular = 0xff000000;

                            quad[3]->colour = 0x00000000;
                            quad[3]->specular = 0xff000000;

                            POLY_add_quad(quad, POLY_PAGE_COLOUR, UC_FALSE);

                            continue;
                        } else {
                            TEXTURE_get_minitexturebits_uvs(
                                *rooftex,
                                &page,
                                &quad[0]->u,
                                &quad[0]->v,
                                &quad[1]->u,
                                &quad[1]->v,
                                &quad[2]->u,
                                &quad[2]->v,
                                &quad[3]->u,
                                &quad[3]->v);
                        }
                    } else {
                        PAP_Hi* ph;
                        ph = &PAP_2HI(p_f4->RX & 127, p_f4->RZ & 127);

                        TEXTURE_get_minitexturebits_uvs(
                            ph->Texture,
                            &page,
                            &quad[0]->u,
                            &quad[0]->v,
                            &quad[1]->u,
                            &quad[1]->v,
                            &quad[2]->u,
                            &quad[2]->v,
                            &quad[3]->u,
                            &quad[3]->v);
                    }

                    if (page > POLY_NUM_PAGES - 2)
                        page = 0;

                    {
                        if (!AENG_drawing_a_warehouse && (p_f4->DrawFlags & (RFACE_FLAG_SHADOW_1 | RFACE_FLAG_SHADOW_2 | RFACE_FLAG_SHADOW_3))) {
                            POLY_Point pshad[4];

                            pshad[0] = *(quad[0]);
                            pshad[1] = *(quad[1]);
                            pshad[2] = *(quad[2]);
                            pshad[3] = *(quad[3]);

                            for (j = 0; j < 4; j++) {
                                red = (pshad[j].colour >> 16) & 0xff;
                                green = (pshad[j].colour >> 8) & 0xff;
                                blue = (pshad[j].colour >> 0) & 0xff;

                                red -= 130;
                                green -= 130;
                                blue -= 130;

                                if (red < 0) {
                                    red = 0;
                                }
                                if (green < 0) {
                                    green = 0;
                                }
                                if (blue < 0) {
                                    blue = 0;
                                }

                                pshad[j].colour = (red << 16) | (green << 8) | (blue << 0) | 0xff000000;
                            }

                            ASSERT(FACE_FLAG_SHADOW_1 == 1 << 2);

                            switch ((p_f4->DrawFlags & (RFACE_FLAG_SHADOW_1 | RFACE_FLAG_SHADOW_2 | RFACE_FLAG_SHADOW_3))) {
                            case 0:
                                ASSERT(0);
                                break;

                            case 1:

                                tri[0] = &pshad[0];
                                tri[1] = quad[1];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = quad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                break;

                            case 2:

                                tri[0] = &pshad[0];
                                tri[1] = quad[1];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                break;

                            case 3:

                                // pshad[2].colour += 0x00101010;

                                tri[0] = quad[0];
                                tri[1] = quad[1];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                break;

                            case 4:

                                tri[0] = quad[0];
                                tri[1] = quad[1];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[1];
                                tri[1] = &pshad[3];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                break;

                            case 5:

                                tri[0] = &pshad[0];
                                tri[1] = quad[1];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[1];
                                tri[1] = &pshad[3];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                break;

                            case 6:

                                tri[0] = &pshad[0];
                                tri[1] = quad[1];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                break;

                            case 7:

                                tri[0] = quad[0];
                                tri[1] = quad[1];
                                tri[2] = quad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[1];
                                tri[1] = &pshad[3];
                                tri[2] = &pshad[2];

                                POLY_add_triangle(tri, page, !warehouse);

                                break;

                            default:
                                ASSERT(0);
                                break;
                            }
                        } else {
                            if (p_f4->RX & (1 << 7)) {
                                tri[0] = quad[0];
                                tri[1] = quad[1];
                                tri[2] = quad[3];

                                POLY_add_triangle(tri, page, !warehouse);

                                tri[0] = quad[3];
                                tri[1] = quad[2];
                                tri[2] = quad[0];

                                POLY_add_triangle(tri, page, !warehouse);
                            } else {
                                POLY_add_quad(quad, page, !warehouse);
                            }
                        }
                    }
                }
            }
        }
    }
}

// uc_orig: FACET_draw_walkable_old (fallen/DDEngine/Source/facet.cpp)
// Older walkable surface draw path using PrimFace4/PrimFace3 (pre-RoofFace4 data format).
// Kept 1:1 for reference; the current path is FACET_draw_walkable.
void FACET_draw_walkable_old(SLONG build)
{
    SLONG i;
    SLONG j;

    SLONG sp;
    SLONG ep;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    SLONG red;
    SLONG green;
    SLONG blue;

    SLONG page;

    SLONG walkable;

    PrimFace4* p_f4;
    struct DWalkable* p_walk;
    struct DBuilding* p_dbuilding;

    POLY_Point* pp;

    POLY_Point* tri[3];
    POLY_Point* quad[4];

    ASSERT(WITHIN(build, 1, next_dbuilding - 1));

    p_dbuilding = &dbuildings[build];

    for (walkable = p_dbuilding->Walkable; walkable; walkable = p_walk->Next) {
        p_walk = &dwalkables[walkable];

        if ((build != INDOORS_DBUILDING) || p_walk->StoreyY * 256 < inside_storeys[INDOORS_INDEX].StoreyY) {
            sp = p_walk->StartPoint;
            ep = p_walk->EndPoint;

            POLY_buffer_upto = 0;
            POLY_shadow_upto = 0;

            for (i = sp; i < ep; i++) {
                ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

                pp = &POLY_buffer[POLY_buffer_upto++];

                POLY_transform(
                    AENG_dx_prim_points[i].X,
                    AENG_dx_prim_points[i].Y,
                    AENG_dx_prim_points[i].Z,
                    pp);

                if (pp->MaybeValid()) {

                    NIGHT_get_d3d_colour(
                        NIGHT_WALKABLE_POINT(i),
                        &pp->colour,
                        &pp->specular);
                    // apply_cloud((SLONG)AENG_dx_prim_points[i].X,(SLONG)AENG_dx_prim_points[i].Y,(SLONG)AENG_dx_prim_points[i].Z,&pp->colour);

                    // POLY_fadeout_point(pp);
                    //					pp->colour|=fade_alpha;
                }
            }

            for (i = p_walk->StartFace4; i < p_walk->EndFace4; i++) {
                p_f4 = &prim_faces4[i];

                p0 = p_f4->Points[0] - sp;
                p1 = p_f4->Points[1] - sp;
                p2 = p_f4->Points[2] - sp;
                p3 = p_f4->Points[3] - sp;

                ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));
                ASSERT(WITHIN(p3, 0, POLY_buffer_upto - 1));

                quad[0] = &POLY_buffer[p0];
                quad[1] = &POLY_buffer[p1];
                quad[2] = &POLY_buffer[p2];
                quad[3] = &POLY_buffer[p3];

                if (POLY_valid_quad(quad)) {

                    if (p_f4->DrawFlags & POLY_FLAG_TEXTURED) {
                        quad[0]->u = float(p_f4->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                        quad[0]->v = float(p_f4->UV[0][1]) * (1.0F / 32.0F);

                        quad[1]->u = float(p_f4->UV[1][0]) * (1.0F / 32.0F);
                        quad[1]->v = float(p_f4->UV[1][1]) * (1.0F / 32.0F);

                        quad[2]->u = float(p_f4->UV[2][0]) * (1.0F / 32.0F);
                        quad[2]->v = float(p_f4->UV[2][1]) * (1.0F / 32.0F);

                        quad[3]->u = float(p_f4->UV[3][0]) * (1.0F / 32.0F);
                        quad[3]->v = float(p_f4->UV[3][1]) * (1.0F / 32.0F);

                        page = p_f4->UV[0][0] & 0xc0;
                        page <<= 2;
                        page |= p_f4->TexturePage;

                        if (page > POLY_NUM_PAGES - 2)
                            page = 0;

                        if (p_f4->FaceFlags & (FACE_FLAG_SHADOW_1 | FACE_FLAG_SHADOW_2 | FACE_FLAG_SHADOW_3)) {
                            POLY_Point ps[4];

                            ps[0] = *(quad[0]);
                            ps[1] = *(quad[1]);
                            ps[2] = *(quad[2]);
                            ps[3] = *(quad[3]);

                            for (j = 0; j < 4; j++) {
                                red = (ps[j].colour >> 16) & 0xff;
                                green = (ps[j].colour >> 8) & 0xff;
                                blue = (ps[j].colour >> 0) & 0xff;

                                red -= 130;
                                green -= 130;
                                blue -= 130;

                                if (red < 0) {
                                    red = 0;
                                }
                                if (green < 0) {
                                    green = 0;
                                }
                                if (blue < 0) {
                                    blue = 0;
                                }

                                ps[j].colour = (red << 16) | (green << 8) | (blue << 0) | 0xff000000;
                            }

                            ASSERT(FACE_FLAG_SHADOW_1 == 1 << 2);

                            switch ((p_f4->FaceFlags & (FACE_FLAG_SHADOW_1 | FACE_FLAG_SHADOW_2 | FACE_FLAG_SHADOW_3)) >> 2) {
                            case 0:
                                ASSERT(0);
                                break;

                            case 1:

                                tri[0] = &ps[0];
                                tri[1] = quad[1];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = quad[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                break;

                            case 2:

                                tri[0] = &ps[0];
                                tri[1] = quad[1];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                break;

                            case 3:

                                ps[2].colour += 0x00202020;

                                tri[0] = quad[0];
                                tri[1] = quad[1];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                break;

                            case 4:

                                tri[0] = quad[0];
                                tri[1] = quad[1];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                tri[0] = quad[1];
                                tri[1] = &ps[3];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                break;

                            case 5:

                                tri[0] = &ps[0];
                                tri[1] = quad[1];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                tri[0] = quad[1];
                                tri[1] = &ps[3];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                break;

                            case 6:

                                tri[0] = &ps[0];
                                tri[1] = quad[1];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                tri[0] = quad[1];
                                tri[1] = quad[3];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                break;

                            case 7:

                                tri[0] = quad[0];
                                tri[1] = quad[1];
                                tri[2] = quad[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                tri[0] = quad[1];
                                tri[1] = &ps[3];
                                tri[2] = &ps[2];

                                POLY_add_triangle(tri, page, UC_TRUE);

                                break;

                            default:
                                ASSERT(0);
                                break;
                            }
                        } else {
                            POLY_add_quad(quad, page, UC_TRUE);
                        }
                    } else {
                        POLY_add_quad(quad, POLY_PAGE_COLOUR, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
                    }
                }
            }
        }
    }
}

// Width of each ladder spine in world units (half the step width used for depth).
// uc_orig: LADDER_SPINE_WIDTH (fallen/DDEngine/Source/facet.cpp)
#define LADDER_SPINE_WIDTH 12

// uc_orig: DRAW_ladder_rungs (fallen/DDEngine/Source/facet.cpp)
// Draws horizontal rungs of a ladder as a series of quads (POLY_PAGE_LADDER texture).
// Insets rung ends by 3/4 of (dx,dz) from the spine edges.
static void DRAW_ladder_rungs(float x1, float z1, float x2, float z2, struct DFacet* p_facet, float dx, float dz, ULONG colour, ULONG specular)
{
    SLONG count;
    float y;
    POLY_Point* quad[4];
    POLY_Point* pp;

    // Signed shift so alpha channel stays at 0xff or 0x00.
    ULONG dcolour = ((signed)colour >> 2) & 0xff3f3f3f;

    x1 += dx * (3.0f / 4.0f);
    z1 += dz * (3.0f / 4.0f);

    x2 -= dx * (3.0f / 4.0f);
    z2 -= dz * (3.0f / 4.0f);

    y = (float)p_facet->Y[0];

    count = p_facet->Height;

    quad[0] = &POLY_buffer[0];
    quad[1] = &POLY_buffer[1];
    quad[2] = &POLY_buffer[2];
    quad[3] = &POLY_buffer[3];

    while (count--) {
        y += BLOCK_SIZE;

        POLY_buffer_upto = 0;
        pp = &POLY_buffer[0];

        POLY_transform(x1, y - 8, z1, pp);
        pp->colour = dcolour;
        pp->specular = specular;
        pp->u = 0.0F;
        pp->v = 0.2F;

        // POLY_fadeout_point(pp);

        POLY_transform(x2, y - 8, z2, ++pp);
        pp->colour = dcolour;
        pp->specular = specular;
        pp->u = 2.0F;
        pp->v = 0.2F;

        // POLY_fadeout_point(pp);

        POLY_transform(x1, y, z1, ++pp);
        pp->colour = colour;
        pp->specular = specular;
        pp->u = 0.0F;
        pp->v = 0.8F;

        // POLY_fadeout_point(pp);

        POLY_transform(x2, y, z2, ++pp);
        pp->colour = colour;
        pp->specular = specular;
        pp->u = 2.0F;
        pp->v = 0.8F;

        // POLY_fadeout_point(pp);

        if (POLY_valid_quad(quad)) {
            SLONG page;

            page = POLY_PAGE_LADDER;

            POLY_add_quad(quad, page, UC_FALSE);
        }
    }
}

// uc_orig: DRAW_ladder_sides (fallen/DDEngine/Source/facet.cpp)
// Draws the two vertical spines of a ladder as cross-sectional quads (POLY_PAGE_LADDER).
// Each spine is a 3-vertex cross-section; generates 4 quad strips per segment.
static void DRAW_ladder_sides(float x1, float z1, float x2, float z2, struct DFacet* p_facet, float dx, float dz, ULONG colour, ULONG specular)
{
    SLONG count;
    float y;
    POLY_Point* quad[4];
    POLY_Point* pp;
    float height;
    UWORD sp[64];

    // Signed shift so alpha channel stays at 0xff or 0x00.
    ULONG dcolour = ((signed)colour >> 2) & 0xff3f3f3f;

    float v;

    y = (float)p_facet->Y[0];
    count = (p_facet->Height * BLOCK_SIZE) / 256;
    if (count == 0)
        height = 256;
    else
        height = (float)((p_facet->Height * BLOCK_SIZE) / count);
    count++;

    {
        float x1mdz, x1pdx, x2mdx, x2mdz;
        float z1pdx, z1pdz, z2mdz, z2pdx;
        SLONG c0 = 0;

        x1mdz = x1 - dz;
        x1pdx = x1 + dx;
        x2mdx = x2 - dx;
        x2mdz = x2 - dz;

        z1pdx = z1 + dx;
        z1pdz = z1 + dz;
        z2mdz = z2 - dz;
        z2pdx = z2 + dx;

        POLY_buffer_upto = 0;
        pp = &POLY_buffer[0];
        while (c0 < count) {
            v = y * (1.0F / 64.0F);

            sp[c0] = POLY_buffer_upto;

            POLY_transform(x1mdz, y, z1pdx, pp);
            pp->colour = dcolour;
            pp->specular = specular;
            pp->u = 0.0F;
            pp->v = v;

            // POLY_fadeout_point(pp);

            POLY_transform(x1, y, z1, ++pp);
            pp->colour = colour;
            pp->specular = specular;
            pp->u = 0.5F;
            pp->v = v;

            // POLY_fadeout_point(pp);

            POLY_transform(x1pdx, y, z1pdz, ++pp);
            pp->colour = dcolour;
            pp->specular = specular;
            pp->u = 1.0F;
            pp->v = v;

            // POLY_fadeout_point(pp);

            POLY_transform(x2mdx, y, z2mdz, ++pp);
            pp->colour = dcolour;
            pp->specular = specular;
            pp->u = 0.0F;
            pp->v = v;

            // POLY_fadeout_point(pp);

            POLY_transform(x2, y, z2, ++pp);
            pp->colour = colour;
            pp->specular = specular;
            pp->u = 0.5F;
            pp->v = v;

            // POLY_fadeout_point(pp);

            POLY_transform(x2mdz, y, z2pdx, ++pp);
            pp->colour = dcolour;
            pp->specular = specular;
            pp->u = 1.0F;
            pp->v = v;

            // POLY_fadeout_point(pp);

            pp++;

            POLY_buffer_upto += 6;

            y += height;

            if (c0 > 0) {
                quad[0] = &POLY_buffer[sp[c0]];
                quad[1] = &POLY_buffer[sp[c0] + 1];
                quad[2] = &POLY_buffer[sp[c0 - 1]];
                quad[3] = &POLY_buffer[sp[c0 - 1] + 1];

                if (POLY_valid_quad(quad)) {
                    SLONG page;

                    page = POLY_PAGE_LADDER;

                    POLY_add_quad(quad, page, 0);
                }

                quad[0] = &POLY_buffer[sp[c0] + 1];
                quad[1] = &POLY_buffer[sp[c0] + 2];
                quad[2] = &POLY_buffer[sp[c0 - 1] + 1];
                quad[3] = &POLY_buffer[sp[c0 - 1] + 2];

                if (POLY_valid_quad(quad)) {
                    SLONG page;

                    page = POLY_PAGE_LADDER;

                    POLY_add_quad(quad, page, 0);
                }

                quad[0] = &POLY_buffer[sp[c0] + 3];
                quad[1] = &POLY_buffer[sp[c0] + 4];
                quad[2] = &POLY_buffer[sp[c0 - 1] + 3];
                quad[3] = &POLY_buffer[sp[c0 - 1] + 4];

                if (POLY_valid_quad(quad)) {
                    SLONG page;

                    // page = texture_quad(quad,dstyles[0],0,0);
                    // page=texture_quad(quad,dstyles[p_facet->StyleIndex],1,5);
                    page = POLY_PAGE_LADDER;

                    POLY_add_quad(quad, page, 0);
                }

                quad[0] = &POLY_buffer[sp[c0] + 4];
                quad[1] = &POLY_buffer[sp[c0] + 5];
                quad[2] = &POLY_buffer[sp[c0 - 1] + 4];
                quad[3] = &POLY_buffer[sp[c0 - 1] + 5];

                if (POLY_valid_quad(quad)) {
                    SLONG page;

                    // page = texture_quad(quad,dstyles[0],0,0);
                    // page=texture_quad(quad,dstyles[p_facet->StyleIndex],1,5);
                    page = POLY_PAGE_LADDER;

                    POLY_add_quad(quad, page, 0);
                }
            }
            c0++;
        }
    }
}

// uc_orig: DRAW_ladder (fallen/DDEngine/Source/facet.cpp)
// Draws a full ladder for a facet: computes spine positions, samples lighting, calls
// DRAW_ladder_rungs and DRAW_ladder_sides, then projects a ladder shadow if inside hidden area.
void DRAW_ladder(struct DFacet* p_facet)
{
    SLONG dx, dz;
    SLONG dx3, dz3;
    SLONG x1, z1, x2, z2;

    ULONG colour;
    ULONG specular;

    // Don't bother drawing the sewer ladders when we're not in the sewers.
    if (p_facet->FacetFlags & FACET_FLAG_LADDER_LINK) {
        // These facets are always drawn.
    } else {
        if (GAME_FLAGS & GF_SEWERS) {
            return;
        }
    }

    x1 = p_facet->x[0] << 8;
    x2 = p_facet->x[1] << 8;
    z1 = p_facet->z[0] << 8;
    z2 = p_facet->z[1] << 8;

    dx = x2 - x1;
    dz = z2 - z1;

    dx3 = (dx * 21845) >> 16; //   divide by 3
    dz3 = (dz * 21845) >> 16; //   divide by 3

    x1 += dx3;
    z1 += dz3;

    x2 -= dx3;
    z2 -= dz3;

    dx >>= 3;
    dz >>= 3;

    x1 += dz;
    x2 += dz;

    z1 -= dx;
    z2 -= dx;

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

    NIGHT_Colour col = NIGHT_get_light_at(
        ((x1 + x2) >> 1) + (dz << 3),
        p_facet->Y[0] + p_facet->Y[1] >> 1,
        ((z1 + z2) >> 1) - (dz << 3));

    NIGHT_get_d3d_colour(
        col,
        &colour,
        &specular);

    colour |= 0x3f3f3f; // Always have a bit of colour!

    DRAW_ladder_rungs((float)x1, (float)z1, (float)x2, (float)z2, p_facet, (float)dx, (float)dz, colour, specular);
    DRAW_ladder_sides((float)x1, (float)z1, (float)x2, (float)z2, p_facet, (float)dx, (float)dz, colour, specular);

    // Draw the ladder shadow.
    SLONG bx;
    SLONG bz;

    bx = (x1 + x2 - (dz << 3) >> 9);
    bz = (z1 + z2 + (dx << 3) >> 9);

    if (!WITHIN(bx, 0, PAP_SIZE_HI - 1) || !WITHIN(bz, 0, PAP_SIZE_HI - 1)) {
        return;
    }

    if (PAP_2HI(bx, bz).Flags & PAP_FLAG_HIDDEN) {
        float height = p_facet->Height * BLOCK_SIZE;

        POLY_Point pp[4];
        POLY_Point* quad[4];

        quad[0] = &pp[0];
        quad[1] = &pp[1];
        quad[2] = &pp[2];
        quad[3] = &pp[3];

        dx >>= 3;
        dz >>= 3;

        POLY_transform(
            (p_facet->x[0] << 8) + dz,
            (p_facet->Y[0]),
            (p_facet->z[0] << 8) - dx,
            &pp[0]);

        POLY_transform(
            (p_facet->x[1] << 8) + dz,
            (p_facet->Y[0]),
            (p_facet->z[1] << 8) - dx,
            &pp[1]);

        POLY_transform(
            (p_facet->x[0] << 8) + dz,
            (p_facet->Y[0]) + height,
            (p_facet->z[0] << 8) - dx,
            &pp[2]);

        POLY_transform(
            (p_facet->x[1] << 8) + dz,
            (p_facet->Y[0]) + height,
            (p_facet->z[1] << 8) - dx,
            &pp[3]);

        if (POLY_valid_quad(quad)) {
            float top_v = p_facet->Height;

            pp[0].colour = 0xffffffff;
            pp[0].specular = 0xff000000;
            pp[0].u = 0.0F;
            pp[0].v = 0.0F;

            pp[1].colour = 0xffffffff;
            pp[1].specular = 0xff000000;
            pp[1].u = 1.0F;
            pp[1].v = 0.0F;

            pp[2].colour = 0xffffffff;
            pp[2].specular = 0xff000000;
            pp[2].u = 0.0F;
            pp[2].v = top_v;

            pp[3].colour = 0xffffffff;
            pp[3].specular = 0xff000000;
            pp[3].u = 1.0F;
            pp[3].v = top_v;

            POLY_add_quad(quad, POLY_PAGE_LADSHAD, UC_TRUE);
        }
    }
}

// uc_orig: FACET_project_crinkled_shadow (fallen/DDEngine/Source/facet.cpp)
// Projects a crinkle (bump-map) shadow texture onto a facet wall surface.
// Reconstructs the facet geometry into a local SVector_F grid, then calls
// SMAP_project_onto_poly / CRINKLE_project for each quad face.
void FACET_project_crinkled_shadow(SLONG facet)
{
    SLONG i;
    SLONG page;
    SLONG style_index;

    DFacet* p_facet;

    ASSERT(WITHIN(facet, 1, next_dfacet - 1));

    p_facet = &dfacets[facet];

    // Ignore double-sided and inside-facing facets.
    if ((p_facet->FacetFlags & FACET_FLAG_2SIDED) || p_facet->FacetType == STOREY_TYPE_OINSIDE) {
        return;
    }

    SLONG dx = p_facet->x[1] - p_facet->x[0];
    SLONG dz = p_facet->z[1] - p_facet->z[0];

    float sx = float(p_facet->x[0] << 8);
    float sy = float(p_facet->Y[0]);
    float sz = float(p_facet->z[0] << 8);

    float fdx = SIGN(dx) * 256.0F;
    float fdz = SIGN(dz) * 256.0F;

    float block_height = float(p_facet->BlockHeight << 4);
    SLONG height = p_facet->Height;

    SLONG foundation = (p_facet->FHeight) ? 2 : 0;

    SLONG count = abs(dx) + abs(dz);

    style_index = p_facet->StyleIndex;

    set_facet_seed(p_facet->x[0] * p_facet->z[0] + p_facet->Y[0]);

// uc_orig: MAX_FACET_POINTS (fallen/DDEngine/Source/facet.cpp)
#define MAX_FACET_POINTS 512
// uc_orig: MAX_FACET_ROWS (fallen/DDEngine/Source/facet.cpp)
#define MAX_FACET_ROWS 32

    SVector_F facet_point[MAX_FACET_POINTS];
    SLONG facet_point_upto;

    SVector_F* facet_row[MAX_FACET_ROWS];
    SLONG facet_row_upto;

    SVector_F* sv;

    // Build a grid of world-space points representing the facet wall surface,
    // mirroring the logic in MakeFacetPoints.
    facet_row_upto = 0;
    facet_point_upto = 0;

    float x;
    float y;
    float z;

    y = sy;

    while (height >= 0) {
        x = sx;
        z = sz;

        ASSERT(WITHIN(facet_row_upto, 0, MAX_FACET_ROWS - 1));

        facet_row[facet_row_upto] = &facet_point[facet_point_upto];

        for (i = 0; i <= count; i++) {
            sv = &facet_point[facet_point_upto++];

            float ty;

            if (foundation == 2) {
                ty = float(PAP_2HI(SLONG(x) >> 8, SLONG(z) >> 8).Alt << 3);
            } else {
                ty = y;
            }

            sv->X = x;
            sv->Y = ty;
            sv->Z = z;

            x += fdx;
            z += fdz;
        }

        y += block_height;
        height -= 4;
        facet_row_upto += 1;
        foundation -= 1;
    }

    SLONG base_row;
    SVector_F poly[4];

    height = p_facet->Height;
    foundation = (p_facet->FHeight) ? 2 : 0;

    base_row = 0;
    height -= 4;

    while (height >= 0) {
        {
            UBYTE rflip;

            ASSERT(WITHIN(base_row, 0, facet_row_upto - 1));
            ASSERT(WITHIN(base_row + 1, 0, facet_row_upto - 1));

            SVector_F* row1 = facet_row[base_row];
            SVector_F* row2 = facet_row[base_row + 1];

            for (i = 0; i < count; i++) {
                poly[0] = row2[1];
                poly[1] = row2[0];
                poly[2] = row1[1];
                poly[3] = row1[0];

                row1 += 1;
                row2 += 1;

                page = get_texture_page(dstyles[style_index], i, count, &rflip);

                SMAP_Link* sl = SMAP_project_onto_poly(poly, 4);

                if (sl) {
                    extern int AENG_detail_crinkles;

                    if (AENG_detail_crinkles) {
                        if (page < 64 * 8) {
                            if (TEXTURE_crinkle[page]) {
                                POLY_Point pp;

                                POLY_transform(
                                    poly[0].X,
                                    poly[0].Y,
                                    poly[0].Z,
                                    &pp);

                                if (pp.z > 0.6F) {
                                    // Too far away to be crinkled.
                                } else if (pp.z < 0.3F) {
                                    // Maximum crinkle.
                                    CRINKLE_project(
                                        TEXTURE_crinkle[page],
                                        1.0F,
                                        poly,
                                        rflip);

                                    goto added_crinkle;
                                } else {
                                    float extrude;
                                    float av_z;

                                    // Intermediate crinkle extrusion.
                                    av_z = pp.z;

                                    extrude = av_z - 0.5F;
                                    extrude *= 1.0F / (0.4F - 0.5F);

                                    if (extrude > 0.0F) {
                                        if (extrude > 1.0F) {
                                            extrude = 1.0F;
                                        }

                                        CRINKLE_project(
                                            TEXTURE_crinkle[page],
                                            extrude,
                                            poly,
                                            rflip);

                                        goto added_crinkle;
                                    }
                                }
                            }
                        }
                    }

                added_crinkle:;
                }
            }
        }

        foundation -= 1;
        height -= 4;
        i += 1;
        style_index += 1;
        base_row += 1;
    }
}
