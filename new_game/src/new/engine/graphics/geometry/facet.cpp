#include <MFStdLib.h>
#include <DDLib.h>
#include <math.h>

#include "fallen/Headers/game.h"             // Temporary: GAME_TURN, INDOORS_INDEX, the_game
#include "engine/graphics/pipeline/aeng.h"   // Temporary: AENG_detail_crinkles (defined in aeng.cpp)
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/geometry/facet.h"
#include "engine/graphics/geometry/facet_globals.h"
#include "engine/graphics/geometry/sprite.h"
#include "engine/graphics/geometry/mesh.h"
#include "engine/graphics/geometry/superfacet.h"
#include "engine/lighting/night.h"
#include "engine/lighting/crinkle.h"
#include "engine/input/keyboard_globals.h"   // ControlFlag
#include "world/map/pap.h"
#include "world/map/pap_globals.h"           // PAP_hi
#include "world/navigation/inside2.h"
#include "world/navigation/inside2_globals.h"
#include "world/environment/building.h"      // Temporary: DFacet, TEXTURE_PIECE_*, building types
#include "world/environment/building_globals.h" // dx_textures_xy
#include "missions/memory_globals.h"         // dstyles, dstoreys, paint_mem, inside_storeys, inside_stairs
#include "assets/texture.h"                  // Temporary: TEXTURE_crinkle
#include "ui/controls_globals.h"             // allow_debug_keys

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
inline float grid_height_at(SLONG mx, SLONG mz)
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

// uc_orig: apply_cloud (fallen/DDEngine/Source/facet.cpp)
// Cloud shadow darkening (PC only). Stub — cloud drawing was removed from this path.
inline void apply_cloud(SLONG x, SLONG y, SLONG z, ULONG* col)
{
    return;
}

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
    SLONG start_index;
    SLONG page;
    UBYTE flip;

    POLY_Point *pp, *ps;
    POLY_Point* quad[4];
    SLONG c0, face;

    start_index = POLY_buffer_upto;
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
    float dy = (p_facet->Y[1] - p_facet->Y[0]);
    float dz = (p_facet->z[1] - p_facet->z[0] << 8);
    float mag = sqrt((dx * dx) + (dz * dz));
    float stepx = (dx / mag) * 10;
    float stepy = (dy / mag) * 10;
    float stepz = (dz / mag) * 10;
    SLONG cx = p_facet->x[0] << 8;
    SLONG cy = p_facet->Y[0];
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
        cy += stepy;
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

            if (AENG_detail_crinkles && (GAME_TURN & 0x20)) {
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
