// Chunk 1: globals init/fini, cloud system, world lines, view frustum gamut, camera, lighting cache.
// aeng.cpp = "Another Engine" — the main 3D scene renderer.
// All Direct3D rendering will be replaced in Stage 7; for now this is a 1:1 port.

#include "engine/graphics/pipeline/aeng_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/polypage.h"   // Temporary: PolyPage, D3DMULTIMATRIX, DrawIndPrimMM, SET_MM_INDEX
#include "engine/graphics/pipeline/polypoint.h"  // Temporary: PolyPoint2D (AENG_draw_some_polys)
#include "engine/graphics/geometry/mesh.h"
#include "engine/lighting/ngamut.h"
#include "engine/lighting/ngamut_globals.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"
#include "engine/graphics/pipeline/sky.h"
#include "assets/compression.h"
#include "assets/tga.h"
#include "assets/texture.h"
#include "assets/texture_globals.h"
#include "core/matrix.h"

// Temporary: FMatrix, FC_cam, Keys, HEAP, prim_points, save_table, NIGHT_cache
#include "fallen/Headers/FMatrix.h"
#include "fallen/Headers/fc.h"   // Temporary: FC_cam
#include "fallen/Headers/Game.h"
#include "fallen/Headers/building.h"
#include "fallen/Headers/memory.h"
#include "fallen/Headers/inside2.h"  // Temporary: InsideStorey
#include "fallen/Headers/pap.h"
#include "fallen/Headers/mav.h"      // Temporary: MAVHEIGHT (draw_quick_floor)
#include "fallen/DDLibrary/Headers/DDLib.h"
#include "missions/memory_globals.h" // Temporary: inside_storeys

#include <MFStdLib.h>
#include <math.h>
#include <stdio.h>

#include "engine/animation/figure.h"
#include "engine/animation/figure_globals.h"  // kludge_shrink
#include "engine/graphics/graphics_api/gd_display.h"  // the_display
#include "engine/graphics/geometry/shape.h"
#include "engine/lighting/smap.h"
#include "engine/lighting/smap_globals.h"
#include "engine/audio/sound.h"   // WORLD_TYPE_SNOW
#include "engine/audio/sound_globals.h" // world_type
#include "assets/anim_globals.h"  // estate
#include "effects/drip.h"
#include "effects/drip_globals.h"
#include "effects/fire.h"
#include "effects/fire_globals.h"
#include "effects/spark.h"
#include "effects/spark_globals.h"
#include "effects/glitter.h"
#include "effects/glitter_globals.h"
#include "effects/dirt.h"
#include "effects/dirt_globals.h"
#include "effects/pow.h"
#include "actors/items/hook.h"
#include "fallen/Headers/prim.h"  // Temporary: PRIM_OBJ_CAN, PRIM_OBJ_HOOK, PRIM_OBJ_ITEM_AMMO_SHOTGUN
#include "actors/items/balloon_globals.h"  // Temporary: BALLOON_balloon, BALLOON_balloon_upto
#include "core/timer.h"                    // Temporary: StartStopwatch, StopStopwatch (AENG_draw_some_polys)
#include "engine/io/env.h"                 // Temporary: ENV_set_value_number (AENG_set/guess_detail_levels)

// uc_orig: POLY_set_local_rotation_none (fallen/DDEngine/Source/aeng.cpp)
#define POLY_set_local_rotation_none() \
    {                                  \
    }

// uc_orig: AENG_MAX_BBOXES (fallen/DDEngine/Source/aeng.cpp)
#define AENG_MAX_BBOXES 8

// uc_orig: AENG_BBOX_PUSH_IN (fallen/DDEngine/Source/aeng.cpp)
#define AENG_BBOX_PUSH_IN 16

// uc_orig: AENG_BBOX_PUSH_OUT (fallen/DDEngine/Source/aeng.cpp)
#define AENG_BBOX_PUSH_OUT 4

// uc_orig: ALT_SHIFT (fallen/DDEngine/Source/aeng.cpp)
#define ALT_SHIFT (3)

// Forward declarations for private helpers defined later in the file.
// uc_orig: AENG_draw_far_facets (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_far_facets(void);
// uc_orig: AENG_draw_box_around_recessed_door (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_box_around_recessed_door(struct DFacet* df, SLONG inside_out);
// uc_orig: AENG_get_rid_of_unused_dfcache_lighting (fallen/DDEngine/Source/aeng.cpp)
void AENG_get_rid_of_unused_dfcache_lighting(SLONG splitscreen);
// uc_orig: AENG_draw_inside_floor (fallen/DDEngine/Source/aeng.cpp)
void AENG_draw_inside_floor(UWORD inside_index, UWORD inside_room, UBYTE fade);

// uc_orig: AENG_DRAW_DIST (fallen/DDEngine/Source/aeng.cpp)
// Draw distance in full map tiles (integer, not scaled).
#define AENG_DRAW_DIST (CurDrawDistance >> 8)

// uc_orig: AENG_DRAW_DIST_PRECISE (fallen/DDEngine/Source/aeng.cpp)
#define AENG_DRAW_DIST_PRECISE (CurDrawDistance)

// uc_orig: AENG_DRAW_PEOPLE_DIST (fallen/DDEngine/Source/aeng.cpp)
#define AENG_DRAW_PEOPLE_DIST (CurDrawDistance + 128)

// uc_orig: AENG_LENS (fallen/DDEngine/Source/aeng.cpp)
#define AENG_LENS (AENG_lens)

// uc_orig: SWAP_FRAME (fallen/DDEngine/Source/aeng.cpp)
// Swaps two COMP_Frame pointers (used to double-buffer movie playback).
#define SWAP_FRAME(a, b)   \
    {                      \
        COMP_Frame* spare; \
        spare = (a);       \
        (a) = (b);         \
        (b) = spare;       \
    }

// uc_orig: MIN_CLOUD (fallen/DDEngine/Source/aeng.cpp)
#define MIN_CLOUD 48

// uc_orig: dfacets_drawn_this_gameturn (fallen/DDEngine/Source/aeng.cpp)
extern SLONG dfacets_drawn_this_gameturn;
// uc_orig: allow_debug_keys (fallen/DDEngine/Source/aeng.cpp)
extern BOOL allow_debug_keys;

// uc_orig: move_clouds (fallen/DDEngine/Source/aeng.cpp)
void move_clouds(void)
{
    cloud_x += 10;
    cloud_z += 5;
}

// uc_orig: calc_global_cloud (fallen/DDEngine/Source/aeng.cpp)
// Bilinearly samples the 32x32 cloud texture at world position (x,y,z).
// Writes the result to global_b. Only active during daytime (NIGHT_FLAG_DAYTIME).
void calc_global_cloud(SLONG x, SLONG y, SLONG z)
{
    SLONG in, r, g, b;
    SLONG in1, in2, in3, in0;
    SLONG dx, dz, mx, mz;
    if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME))
        return;
    y >>= 1;

    x = (x + (cloud_x)-y);
    z = (z + (cloud_z)-y);

    mx = (x >> 8) & 0x1f;
    mz = (z >> 8) & 0x1f;

    in0 = cloud_data[mx][mz];
    in1 = cloud_data[(mx + 1) & 0x1f][mz];
    in2 = cloud_data[(mx) & 0x1f][(mz + 1) & 0x1f];
    in3 = cloud_data[(mx + 1) & 0x1f][(mz + 1) & 0x1f];

    dx = x & 0xff;
    dz = z & 0xff;

    if (dx + dz < 256) {
        in = in0;
        in += ((in1 - in0) * dx) >> 8;
        in += ((in2 - in0) * dz) >> 8;
    } else {
        in = in3;
        in += ((in2 - in3) * (256 - dx)) >> 8;
        in += ((in1 - in3) * (256 - dz)) >> 8;
    }
    if (in < 0)
        in = 0;
    global_b = in;
}

// uc_orig: use_global_cloud (fallen/DDEngine/Source/aeng.cpp)
// Applies the precomputed cloud shadow (global_b) to a vertex colour.
void use_global_cloud(ULONG* col)
{
    SLONG r, g, b;
    SLONG in = global_b;

    if (!aeng_draw_cloud_flag)
        return;

    if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME))
        return;
    r = ((*col) & 0xff0000) >> 16;

    in = (in * (r)) >> 8;

    {
        r -= in;
        if (r < 0)
            r = 0;
    }

    g = ((*col) & 0xff00) >> 8;
    {
        g -= in;
        if (g < 0)
            g = 0;
    }

    b = ((*col) & 0xff);

    {
        b -= in;
        if (b < 0)
            b = 0;
    }

    *col = (*col & 0xff000000) | (r << 16) | (g << 8) | (b);
}

// uc_orig: apply_cloud (fallen/DDEngine/Source/aeng.cpp)
// Full per-vertex cloud shadow: samples cloud texture, then modulates colour.
// NOTE: this function always returns immediately (the body is unreachable dead code preserved 1:1).
void apply_cloud(SLONG x, SLONG y, SLONG z, ULONG* col)
{
    return;

    if (!aeng_draw_cloud_flag)
        return;

    SLONG in, r, g, b;
    SLONG in1, in2, in3, in0;
    SLONG dx, dz, mx, mz;
    if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME))
        return;
    y >>= 1;

    x = (x + (cloud_x)-y);
    z = (z + (cloud_z)-y);

    mx = (x >> 8) & 0x1f;
    mz = (z >> 8) & 0x1f;

    in0 = cloud_data[mx][mz];
    in1 = cloud_data[(mx + 1) & 0x1f][mz];
    in2 = cloud_data[(mx) & 0x1f][(mz + 1) & 0x1f];
    in3 = cloud_data[(mx + 1) & 0x1f][(mz + 1) & 0x1f];

    dx = x & 0xff;
    dz = z & 0xff;

    if (dx + dz < 256) {
        in = in0;
        in += ((in1 - in0) * dx) >> 8;
        in += ((in2 - in0) * dz) >> 8;
    } else {
        in = in3;
        in += ((in2 - in3) * (256 - dx)) >> 8;
        in += ((in1 - in3) * (256 - dz)) >> 8;
    }
    if (in < 0)
        in = 0;

    r = ((*col) & 0xff0000) >> 16;

    in = (in * (r)) >> 8;

    {
        r -= in;
        if (r < 0)
            r = 0;
    }

    g = ((*col) & 0xff00) >> 8;
    {
        g -= in;
        if (g < 0)
            g = 0;
    }

    b = ((*col) & 0xff);

    {
        b -= in;
        if (b < 0)
            b = 0;
    }

    *col = (*col & 0xff000000) | (r << 16) | (g << 8) | (b);
}

// uc_orig: init_clouds (fallen/DDEngine/Source/aeng.cpp)
// Loads the 32x32 cloud shadow map from data\cloud.raw. Fills with zeros if missing.
static void init_clouds(void)
{
    MFFileHandle handle;
    handle = FileOpen("data\\cloud.raw");
    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, cloud_data, 1024);
        FileClose(handle);
    } else {
        memset(cloud_data, 0, 1024);
    }
}

// uc_orig: GetShadowPixelMapping (fallen/DDEngine/Source/aeng.cpp)
// Returns a 256-entry UBYTE->UWORD lookup table mapping shadow density to
// the correct 16-bit pixel format for the current display mode.
UWORD* GetShadowPixelMapping()
{
    static UWORD mapping[256];
    static int mapping_state = -1;

    if ((mapping_state == -1) || (mapping_state != (int)the_display.GetDeviceInfo()->DestInvSourceColourSupported())) {
        mapping_state = the_display.GetDeviceInfo()->DestInvSourceColourSupported();

        if (mapping_state) {
            for (int ii = 0; ii < 256; ii++) {
                int val = ii >> 1;

                mapping[ii] = ((val >> TEXTURE_shadow_mask_red) << TEXTURE_shadow_shift_red)
                    | ((val >> TEXTURE_shadow_mask_green) << TEXTURE_shadow_shift_green)
                    | ((val >> TEXTURE_shadow_mask_blue) << TEXTURE_shadow_shift_blue);
            }
        } else {
            for (int ii = 0; ii < 256; ii++) {
                int val = ii >> 1;

                mapping[ii] = (val >> TEXTURE_shadow_mask_alpha) << TEXTURE_shadow_shift_alpha;
            }
        }
    }

    return mapping;
}

// uc_orig: add_debug_line (fallen/DDEngine/Source/aeng.cpp)
// Queues a debug world-space line segment for drawing this frame.
static void add_debug_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG colour)
{
    SLONG line;

    line = next_line % MAX_LINES;

    Lines[line].x1 = x1;
    Lines[line].y1 = y1;
    Lines[line].z1 = z1;

    Lines[line].x2 = x2;
    Lines[line].y2 = y2;
    Lines[line].z2 = z2;
    Lines[line].col = colour;
    next_line++;
}

// uc_orig: AENG_movie_init (fallen/DDEngine/Source/aeng.cpp)
// Loads the pre-rendered movie file (movie\bond.mmv) into AENG_movie_data.
void AENG_movie_init()
{
    SLONG bytes_read;

    FILE* handle;

    handle = MF_Fopen("movie\\bond.mmv", "rb");

    if (!handle) {
        goto file_error;
    }

    bytes_read = fread(AENG_movie_data, sizeof(UBYTE), AENG_MAX_MOVIE_DATA, handle);
    ASSERT(bytes_read < AENG_MAX_MOVIE_DATA);

    AENG_frame_last = &AENG_frame_one;
    AENG_frame_next = &AENG_frame_two;
    AENG_frame_count = 0;
    AENG_frame_tick = 0;
    AENG_frame_number = 200;
    AENG_movie_upto = AENG_movie_data;

    return;

file_error:;

    AENG_frame_last = NULL;
    AENG_frame_next = NULL;
    AENG_frame_count = 0;
    AENG_frame_tick = 0;
    AENG_frame_number = 0;

    return;
}

// uc_orig: AENG_movie_update (fallen/DDEngine/Source/aeng.cpp)
// Decodes and uploads the next movie frame to the video texture page.
// NOTE: returns immediately at top (movie playback disabled in this build).
void AENG_movie_update()
{
    return;
    if (AENG_frame_number == 0) {
        return;
    }

    COMP_Delta* cd;

    AENG_frame_tick += TICK_RATIO >> 1;

    if (AENG_frame_tick >= (1 << TICK_SHIFT)) {
        AENG_frame_tick -= (1 << TICK_SHIFT);
        AENG_frame_tick &= (1 << TICK_SHIFT) - 1;

        AENG_frame_count += 1;

        if (AENG_frame_count >= AENG_frame_number) {
            if (AENG_frame_count < AENG_frame_number + 32) {
                return;
            }

            AENG_frame_count = 0;
            AENG_movie_upto = AENG_movie_data;
        }

        cd = (COMP_Delta*)AENG_movie_upto;

        COMP_decomp(
            AENG_frame_last,
            cd,
            AENG_frame_next);

        AENG_movie_upto += cd->size + 4;

        ASSERT(TEXTURE_VIDEO_SIZE == COMP_SIZE);

        if (TEXTURE_86_lock()) {
            SLONG x;
            SLONG y;

            UWORD* data;
            UWORD pixel;

            TGA_Pixel* tp;

            for (y = 0; y < TEXTURE_VIDEO_SIZE; y++) {
                tp = &AENG_frame_next->p[y][0];

                data = TEXTURE_shadow_bitmap;
                data += y * (TEXTURE_shadow_pitch >> 1);

                for (x = 0; x < TEXTURE_VIDEO_SIZE; x++) {
                    pixel = 0;
                    pixel |= (tp->red >> TEXTURE_shadow_mask_red) << TEXTURE_shadow_shift_red;
                    pixel |= (tp->green >> TEXTURE_shadow_mask_green) << TEXTURE_shadow_shift_green;
                    pixel |= (tp->blue >> TEXTURE_shadow_mask_blue) << TEXTURE_shadow_shift_blue;

                    *data = pixel;

                    tp += 1;
                    data += 1;
                }
            }

            TEXTURE_86_unlock();
            TEXTURE_86_update();
        }

        SWAP_FRAME(AENG_frame_last, AENG_frame_next);
    }
}

// uc_orig: AENG_get_draw_distance (fallen/DDEngine/Source/aeng.cpp)
SLONG AENG_get_draw_distance()
{
    return CurDrawDistance >> 8;
}

// uc_orig: AENG_set_draw_distance (fallen/DDEngine/Source/aeng.cpp)
// Currently a no-op stub (the draw distance is not runtime-adjustable in this build).
void AENG_set_draw_distance(SLONG dist)
{
}

// uc_orig: AENG_init (fallen/DDEngine/Source/aeng.cpp)
// One-time engine startup: inits meshes, cloud data, sky, polygon system, textures.
void AENG_init(void)
{
    MESH_init();
    init_clouds();
    SKY_init("stars\\poo");
    POLY_init();
    AENG_movie_init();
    TEXTURE_choose_set(1);
    INDOORS_INDEX_FADE = 255;

    init_flames();
}

// uc_orig: AENG_fini (fallen/DDEngine/Source/aeng.cpp)
// Engine shutdown: frees all loaded textures.
void AENG_fini()
{
    TEXTURE_free();
}

// uc_orig: AENG_create_dx_prim_points (fallen/DDEngine/Source/aeng.cpp)
// Converts the integer prim_points array to the float AENG_dx_prim_points array.
// Called after loading a level to prepare points for Direct3D rendering.
void AENG_create_dx_prim_points()
{
    SLONG i;

    for (i = 0; i < MAX_PRIM_POINTS; i++) {
        AENG_dx_prim_points[i].X = float(prim_points[i].X);
        AENG_dx_prim_points[i].Y = float(prim_points[i].Y);
        AENG_dx_prim_points[i].Z = float(prim_points[i].Z);
    }
}

// uc_orig: AENG_world_line (fallen/DDEngine/Source/aeng.cpp)
// Draws a world-space line segment between two 3D points with per-endpoint width and colour.
void AENG_world_line(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front)
{
    POLY_Point p1;
    POLY_Point p2;

    POLY_transform(float(x1), float(y1), float(z1), &p1);
    POLY_transform(float(x2), float(y2), float(z2), &p2);

    if (POLY_valid_line(&p1, &p2)) {
        p1.colour = colour1;
        p1.specular = 0xff000000;

        p2.colour = colour2;
        p2.specular = 0xff000000;

        POLY_add_line(&p1, &p2, float(width1), float(width2), POLY_PAGE_COLOUR, sort_to_front);
    }
}

// uc_orig: AENG_world_line_nondebug (fallen/DDEngine/Source/aeng.cpp)
// Same as AENG_world_line but for non-debug (always-on) world lines.
void AENG_world_line_nondebug(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front)
{
    POLY_Point p1;
    POLY_Point p2;

    POLY_transform(float(x1), float(y1), float(z1), &p1);
    POLY_transform(float(x2), float(y2), float(z2), &p2);

    if (POLY_valid_line(&p1, &p2)) {
        p1.colour = colour1;
        p1.specular = 0xff000000;

        p2.colour = colour2;
        p2.specular = 0xff000000;

        POLY_add_line(&p1, &p2, float(width1), float(width2), POLY_PAGE_COLOUR, sort_to_front);
    }
}

// uc_orig: AENG_world_line_infinite (fallen/DDEngine/Source/aeng.cpp)
// Draws a long line by splitting it into 1024-unit segments and calling AENG_world_line.
void AENG_world_line_infinite(
    SLONG ix1, SLONG iy1, SLONG iz1, SLONG iwidth1, ULONG colour1,
    SLONG ix2, SLONG iy2, SLONG iz2, SLONG iwidth2, ULONG colour2,
    SLONG sort_to_front)
{
    float x1 = float(ix1);
    float y1 = float(iy1);
    float z1 = float(iz1);
    float w1 = float(iwidth1);
    float r1 = float((colour1 >> 16) & 0xff);
    float g1 = float((colour1 >> 8) & 0xff);
    float b1 = float((colour1 >> 0) & 0xff);

    float x2 = float(ix2);
    float y2 = float(iy2);
    float z2 = float(iz2);
    float w2 = float(iwidth2);
    float r2 = float((colour2 >> 16) & 0xff);
    float g2 = float((colour2 >> 8) & 0xff);
    float b2 = float((colour2 >> 0) & 0xff);

    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    float dr = r2 - r1;
    float dg = g2 - g1;
    float db = b2 - b1;
    float dw = w2 - w1;

    float dist = sqrt(dx * dx + dy * dy + dz * dz);
    float steps = (float)floor(dist * (1.0F / 1024.0F)) + 1.0F;
    float oversteps = 1.0F / steps;

    float x = x1;
    float y = y1;
    float z = z1;
    float w = w1;
    float r = r1;
    float g = g1;
    float b = b1;

    dx *= oversteps;
    dy *= oversteps;
    dz *= oversteps;
    dw *= oversteps;
    dr *= oversteps;
    dg *= oversteps;
    db *= oversteps;

    float f;

    for (f = 0.0F; f < steps; f += 1.0F) {
        colour1 = (SLONG(r) << 16) | (SLONG(g) << 8) | (SLONG(b) << 0);
        colour2 = (SLONG(r + dr) << 16) | (SLONG(g + dg) << 8) | (SLONG(b + db) << 0);

        AENG_world_line(
            SLONG(x),
            SLONG(y),
            SLONG(z),
            SLONG(w),
            colour1,
            SLONG(x + dx),
            SLONG(y + dy),
            SLONG(z + dz),
            SLONG(w + dw),
            colour2,
            sort_to_front);

        x += dx;
        y += dy;
        z += dz;
        w += dw;
        r += dr;
        g += dg;
        b += db;
    }
}

// uc_orig: AENG_calc_gamut (fallen/DDEngine/Source/aeng.cpp)
// Computes the full view-frustum gamut for camera-based map-square culling.
// Builds a 5-point pyramid (camera + 4 far corners) and feeds the 8 edges
// into NGAMUT to produce per-row xmin/xmax visible ranges.
void AENG_calc_gamut(
    float x,
    float y,
    float z,
    float yaw,
    float pitch,
    float roll,
    float draw_dist,
    float lens)
{
    float width;
    float height;
    float depth;
    float aspect;
    float matrix[9];

    MATRIX_calc(
        matrix,
        yaw,
        pitch,
        roll);

    width = draw_dist;
    height = draw_dist;
    depth = draw_dist;

    width *= POLY_screen_width;
    width /= POLY_screen_height;

    if (FC_cam[1].focus) {
        width *= 2.0F;
    }

    width /= lens;
    height /= lens;

    AENG_cone[3].x = AENG_cone[4].x = x / 256.0f;
    AENG_cone[3].y = AENG_cone[4].y = y / 256.0f;
    AENG_cone[3].z = AENG_cone[4].z = z / 256.0f;

    AENG_cone[3].x += depth * matrix[6];
    AENG_cone[3].y += depth * matrix[7];
    AENG_cone[3].z += depth * matrix[8];

    // Top-left corner.
    AENG_cone[0].x = AENG_cone[3].x + height * matrix[3];
    AENG_cone[0].y = AENG_cone[3].y + height * matrix[4];
    AENG_cone[0].z = AENG_cone[3].z + height * matrix[5];

    AENG_cone[0].x = AENG_cone[0].x - width * matrix[0];
    AENG_cone[0].y = AENG_cone[0].y - width * matrix[1];
    AENG_cone[0].z = AENG_cone[0].z - width * matrix[2];

    // Top-right corner.
    AENG_cone[1].x = AENG_cone[3].x + height * matrix[3];
    AENG_cone[1].y = AENG_cone[3].y + height * matrix[4];
    AENG_cone[1].z = AENG_cone[3].z + height * matrix[5];

    AENG_cone[1].x = AENG_cone[1].x + width * matrix[0];
    AENG_cone[1].y = AENG_cone[1].y + width * matrix[1];
    AENG_cone[1].z = AENG_cone[1].z + width * matrix[2];

    // Bottom-right corner.
    AENG_cone[2].x = AENG_cone[3].x - height * matrix[3];
    AENG_cone[2].y = AENG_cone[3].y - height * matrix[4];
    AENG_cone[2].z = AENG_cone[3].z - height * matrix[5];

    AENG_cone[2].x = AENG_cone[2].x + width * matrix[0];
    AENG_cone[2].y = AENG_cone[2].y + width * matrix[1];
    AENG_cone[2].z = AENG_cone[2].z + width * matrix[2];

    // Bottom-left corner.
    AENG_cone[3].x = AENG_cone[3].x - height * matrix[3];
    AENG_cone[3].y = AENG_cone[3].y - height * matrix[4];
    AENG_cone[3].z = AENG_cone[3].z - height * matrix[5];

    AENG_cone[3].x = AENG_cone[3].x - width * matrix[0];
    AENG_cone[3].y = AENG_cone[3].y - width * matrix[1];
    AENG_cone[3].z = AENG_cone[3].z - width * matrix[2];

    NGAMUT_init();

    NGAMUT_add_line(AENG_cone[4].x, AENG_cone[4].z, AENG_cone[0].x, AENG_cone[0].z);
    NGAMUT_add_line(AENG_cone[4].x, AENG_cone[4].z, AENG_cone[1].x, AENG_cone[1].z);
    NGAMUT_add_line(AENG_cone[4].x, AENG_cone[4].z, AENG_cone[2].x, AENG_cone[2].z);
    NGAMUT_add_line(AENG_cone[4].x, AENG_cone[4].z, AENG_cone[3].x, AENG_cone[3].z);

    NGAMUT_add_line(AENG_cone[0].x, AENG_cone[0].z, AENG_cone[1].x, AENG_cone[1].z);
    NGAMUT_add_line(AENG_cone[1].x, AENG_cone[1].z, AENG_cone[2].x, AENG_cone[2].z);
    NGAMUT_add_line(AENG_cone[2].x, AENG_cone[2].z, AENG_cone[3].x, AENG_cone[3].z);
    NGAMUT_add_line(AENG_cone[3].x, AENG_cone[3].z, AENG_cone[0].x, AENG_cone[0].z);

    NGAMUT_calculate_point_gamut();
    NGAMUT_calculate_out_gamut();
    NGAMUT_calculate_lo_gamut();
}

// uc_orig: AENG_calc_gamut_lo_only (fallen/DDEngine/Source/aeng.cpp)
// Lightweight gamut for the 32x32 lo-res skyline map: clips the frustum edges
// to map bounds and computes a simple AABB (no per-row xmin/xmax table).
void AENG_calc_gamut_lo_only(
    float x,
    float y,
    float z,
    float yaw,
    float pitch,
    float roll,
    float draw_dist,
    float lens)
{
    float width;
    float height;
    float depth;
    float aspect;
    float matrix[9];

    MATRIX_calc(
        matrix,
        yaw,
        pitch,
        roll);

    width = draw_dist;
    height = draw_dist;
    depth = draw_dist;

    width *= POLY_screen_width;
    width /= POLY_screen_height;

    if (FC_cam[1].focus) {
        width *= 2.0F;
    }

    width /= lens;
    height /= lens;

    AENG_cone[3].x = AENG_cone[4].x = x / 256.0f;
    AENG_cone[3].z = AENG_cone[4].z = z / 256.0f;

    AENG_cone[3].x += depth * matrix[6];
    AENG_cone[3].z += depth * matrix[8];

    AENG_cone[0].x = (AENG_cone[3].x + height * matrix[3]);
    AENG_cone[0].z = (AENG_cone[3].z + height * matrix[5]);

    AENG_cone[0].x = (AENG_cone[0].x - width * matrix[0]);
    AENG_cone[0].z = (AENG_cone[0].z - width * matrix[2]);

    AENG_cone[1].x = (AENG_cone[3].x + height * matrix[3]);
    AENG_cone[1].z = (AENG_cone[3].z + height * matrix[5]);

    AENG_cone[1].x = (AENG_cone[1].x + width * matrix[0]);
    AENG_cone[1].z = (AENG_cone[1].z + width * matrix[2]);

    AENG_cone[2].x = (AENG_cone[3].x - height * matrix[3]);
    AENG_cone[2].z = (AENG_cone[3].z - height * matrix[5]);

    AENG_cone[2].x = (AENG_cone[2].x + width * matrix[0]);
    AENG_cone[2].z = (AENG_cone[2].z + width * matrix[2]);

    AENG_cone[3].x = (AENG_cone[3].x - height * matrix[3]);
    AENG_cone[3].z = (AENG_cone[3].z - height * matrix[5]);

    AENG_cone[3].x = (AENG_cone[3].x - width * matrix[0]);
    AENG_cone[3].z = (AENG_cone[3].z - width * matrix[2]);

    // Clip the eight frustum edges to map bounds, then expand the bounding box.
    float gamut_lo_xmax = AENG_cone[4].x * 0.25f;
    float gamut_lo_xmin = AENG_cone[4].x * 0.25f;
    float gamut_lo_zmax = AENG_cone[4].z * 0.25f;
    float gamut_lo_zmin = AENG_cone[4].z * 0.25f;
    ASSERT((gamut_lo_xmax < PAP_SIZE_LO) && (gamut_lo_xmin > 0));
    ASSERT((gamut_lo_xmax < PAP_SIZE_LO) && (gamut_lo_xmin > 0));

    for (int i = 0; i < 8; i++) {
        int iPt1, iPt2;
        switch (i) {
        case 0:
            iPt1 = 4;
            iPt2 = 0;
            break;
        case 1:
            iPt1 = 4;
            iPt2 = 1;
            break;
        case 2:
            iPt1 = 4;
            iPt2 = 2;
            break;
        case 3:
            iPt1 = 4;
            iPt2 = 3;
            break;
        case 4:
            iPt1 = 0;
            iPt2 = 1;
            break;
        case 5:
            iPt1 = 1;
            iPt2 = 2;
            break;
        case 6:
            iPt1 = 2;
            iPt2 = 3;
            break;
        case 7:
            iPt1 = 3;
            iPt2 = 4;
            break;
        }

        float fX1 = AENG_cone[iPt1].x * 0.25f;
        float fZ1 = AENG_cone[iPt1].z * 0.25f;
        float fX2 = AENG_cone[iPt2].x * 0.25f;
        float fZ2 = AENG_cone[iPt2].z * 0.25f;

        // Clip to +ve X
        if (fX1 >= PAP_SIZE_LO) {
            if (fX2 >= PAP_SIZE_LO) {
                gamut_lo_xmax = PAP_SIZE_LO;
                continue;
            } else {
                float lambda = ((float)PAP_SIZE_LO - fX1) / (fX2 - fX1);
                fZ1 = fZ1 + lambda * (fZ2 - fZ1);
                fX1 = PAP_SIZE_LO;
            }
        } else {
            if (fX2 >= PAP_SIZE_LO) {
                float lambda = ((float)PAP_SIZE_LO - fX1) / (fX2 - fX1);
                fZ2 = fZ1 + lambda * (fZ2 - fZ1);
                fX2 = PAP_SIZE_LO;
            }
        }

        // Clip to 0 X
        if (fX1 <= 0.0f) {
            if (fX2 <= 0.0f) {
                gamut_lo_xmin = 0;
                continue;
            } else {
                float lambda = (0.0f - fX1) / (fX2 - fX1);
                fZ1 = fZ1 + lambda * (fZ2 - fZ1);
                fX1 = 0.0f;
            }
        } else {
            if (fX2 <= 0.0f) {
                float lambda = (0.0f - fX1) / (fX2 - fX1);
                fZ2 = fZ1 + lambda * (fZ2 - fZ1);
                fX2 = 0.0f;
            }
        }

        // Clip to +ve Z
        if (fZ1 >= PAP_SIZE_LO) {
            if (fZ2 >= PAP_SIZE_LO) {
                gamut_lo_zmax = PAP_SIZE_LO;
                continue;
            } else {
                float lambda = ((float)PAP_SIZE_LO - fZ1) / (fZ2 - fZ1);
                fX1 = fX1 + lambda * (fX2 - fX1);
                fZ1 = PAP_SIZE_LO;
            }
        } else {
            if (fZ2 >= PAP_SIZE_LO) {
                float lambda = ((float)PAP_SIZE_LO - fZ1) / (fZ2 - fZ1);
                fX2 = fX1 + lambda * (fX2 - fX1);
                fZ2 = PAP_SIZE_LO;
            }
        }

        // Clip to 0 Z
        if (fZ1 <= 0.0f) {
            if (fZ2 <= 0.0f) {
                gamut_lo_zmin = 0;
                continue;
            } else {
                float lambda = (0.0f - fZ1) / (fZ2 - fZ1);
                fX1 = fX1 + lambda * (fX2 - fX1);
                fZ1 = 0.0f;
            }
        } else {
            if (fZ2 <= 0.0f) {
                float lambda = (0.0f - fZ1) / (fZ2 - fZ1);
                fX2 = fX1 + lambda * (fX2 - fX1);
                fZ2 = 0.0f;
            }
        }

        ASSERT((fX1 <= PAP_SIZE_LO) && (fX1 >= 0.0f));
        ASSERT((fX2 <= PAP_SIZE_LO) && (fX2 >= 0.0f));
        ASSERT((fZ1 <= PAP_SIZE_LO) && (fZ1 >= 0.0f));
        ASSERT((fZ2 <= PAP_SIZE_LO) && (fZ2 >= 0.0f));

        if (gamut_lo_xmax < fX1) {
            gamut_lo_xmax = fX1;
        } else if (gamut_lo_xmin > fX1) {
            gamut_lo_xmin = fX1;
        }
        if (gamut_lo_zmax < fZ1) {
            gamut_lo_zmax = fZ1;
        } else if (gamut_lo_zmin > fZ1) {
            gamut_lo_zmin = fZ1;
        }

        if (gamut_lo_xmax < fX2) {
            gamut_lo_xmax = fX2;
        } else if (gamut_lo_xmin > fX2) {
            gamut_lo_xmin = fX2;
        }
        if (gamut_lo_zmax < fZ2) {
            gamut_lo_zmax = fZ2;
        } else if (gamut_lo_zmin > fZ2) {
            gamut_lo_zmin = fZ2;
        }
    }

    AENG_gamut_lo_xmin = (SLONG)(gamut_lo_xmin);
    AENG_gamut_lo_xmax = (SLONG)(gamut_lo_xmax);
    AENG_gamut_lo_zmin = (SLONG)(gamut_lo_zmin);
    AENG_gamut_lo_zmax = (SLONG)(gamut_lo_zmax);

    // Clamp to valid range (avoid out-of-bounds on the exact map edge).
    if (AENG_gamut_lo_xmax == PAP_SIZE_LO) {
        AENG_gamut_lo_xmax = PAP_SIZE_LO - 1;
    }
    if (AENG_gamut_lo_xmin == PAP_SIZE_LO) {
        AENG_gamut_lo_xmin = PAP_SIZE_LO - 1;
    }
    if (AENG_gamut_lo_zmax == PAP_SIZE_LO) {
        AENG_gamut_lo_zmax = PAP_SIZE_LO - 1;
    }
    if (AENG_gamut_lo_zmin == PAP_SIZE_LO) {
        AENG_gamut_lo_zmin = PAP_SIZE_LO - 1;
    }
}

// uc_orig: AENG_set_camera_radians (fallen/DDEngine/Source/aeng.cpp)
// Sets the camera from world position + euler angles (radians), no splitscreen.
void AENG_set_camera_radians(
    SLONG wx,
    SLONG wy,
    SLONG wz,
    float y,
    float p,
    float r)
{
    AENG_cam_x = float(wx);
    AENG_cam_y = float(wy);
    AENG_cam_z = float(wz);

    AENG_cam_yaw = y;
    AENG_cam_pitch = p;
    AENG_cam_roll = r;

    MATRIX_calc(
        AENG_cam_matrix,
        y, p, r);

    POLY_camera_set(
        AENG_cam_x,
        AENG_cam_y,
        AENG_cam_z,
        AENG_cam_yaw,
        AENG_cam_pitch,
        AENG_cam_roll,
        float(AENG_DRAW_DIST) * 256.0F,
        AENG_LENS,
        POLY_SPLITSCREEN_NONE);

    FMATRIX_vector(AENG_cam_vec, (y * 2048) / (2 * PI), (p * 2048) / (2 * PI));

    AENG_calc_gamut(
        AENG_cam_x,
        AENG_cam_y,
        AENG_cam_z,
        AENG_cam_yaw,
        AENG_cam_pitch,
        AENG_cam_roll,
        AENG_DRAW_DIST,
        AENG_LENS);
}

// uc_orig: AENG_set_camera_radians (fallen/DDEngine/Source/aeng.cpp)
// Sets the camera from world position + euler angles (radians), with splitscreen mode.
void AENG_set_camera_radians(
    SLONG wx,
    SLONG wy,
    SLONG wz,
    float y,
    float p,
    float r,
    SLONG splitscreen)
{
    AENG_cam_x = float(wx);
    AENG_cam_y = float(wy);
    AENG_cam_z = float(wz);

    AENG_cam_yaw = y;
    AENG_cam_pitch = p;
    AENG_cam_roll = r;

    MATRIX_calc(
        AENG_cam_matrix,
        y, p, r);

    POLY_camera_set(
        AENG_cam_x,
        AENG_cam_y,
        AENG_cam_z,
        AENG_cam_yaw,
        AENG_cam_pitch,
        AENG_cam_roll,
        float(AENG_DRAW_DIST) * 256.0F,
        AENG_LENS,
        splitscreen);

    FMATRIX_vector(AENG_cam_vec, (y * 2048) / (2 * PI), (p * 2048) / (2 * PI));

    AENG_calc_gamut(
        AENG_cam_x,
        AENG_cam_y,
        AENG_cam_z,
        AENG_cam_yaw,
        AENG_cam_pitch,
        AENG_cam_roll,
        AENG_DRAW_DIST,
        AENG_LENS);
}

// uc_orig: AENG_set_camera (fallen/DDEngine/Source/aeng.cpp)
// Sets the camera from world position + euler angles (integer 2048-units per turn).
// Also updates FC_cam[0] position/angle for the rest of the game.
void AENG_set_camera(
    SLONG wx,
    SLONG wy,
    SLONG wz,
    SLONG y,
    SLONG p,
    SLONG r)
{
    float radians_yaw = float(y) * (2.0F * PI / 2048.0F);
    float radians_pitch = float(p) * (2.0F * PI / 2048.0F);
    float radians_roll = float(r) * (2.0F * PI / 2048.0F);

    FC_cam[0].x = wx << 8;
    FC_cam[0].y = wy << 8;
    FC_cam[0].z = wz << 8;

    FC_cam[0].yaw = y << 8;
    FC_cam[0].pitch = p << 8;
    FC_cam[0].roll = r << 8;

    AENG_set_camera_radians(
        wx,
        wy,
        wz,
        radians_yaw,
        radians_pitch,
        radians_roll,
        POLY_SPLITSCREEN_NONE);
}

// uc_orig: AENG_do_cached_lighting_old (fallen/DDEngine/Source/aeng.cpp)
// Manages the per-map-square lighting cache: destroys out-of-gamut entries,
// creates missing entries for squares in the current view gamut.
void AENG_do_cached_lighting_old(void)
{
    SLONG i;
    SLONG x;
    SLONG z;
    SLONG kept = 0, new_squares = 0;

    NIGHT_Square* nq;

    extern SLONG HEAP_max_free(void);

    if (HEAP_max_free() < 4000 || Keys[KB_Q]) {
        CBYTE str[100];
        NIGHT_destroy_all_cached_info();
    } else {
        for (i = 1; i < NIGHT_MAX_SQUARES; i++) {
            nq = &NIGHT_square[i];

            if (nq->flag & NIGHT_SQUARE_FLAG_USED) {
                if (WITHIN(nq->lo_map_z, NGAMUT_lo_zmin, NGAMUT_lo_zmax) && WITHIN(nq->lo_map_x, NGAMUT_lo_gamut[nq->lo_map_z].xmin, NGAMUT_lo_gamut[nq->lo_map_z].xmax)) {
                    kept++;
                } else {
                    NIGHT_cache_destroy(i);
                }
            }
        }
    }

    if (INDOORS_INDEX) {
        if (light_inside != INDOORS_INDEX) {
            for (i = 1; i < NIGHT_MAX_SQUARES; i++) {
                nq = &NIGHT_square[i];
                if (nq->flag & NIGHT_SQUARE_FLAG_USED)
                    NIGHT_cache_destroy(i);
            }
        }
        light_inside = INDOORS_INDEX;

        {
            SLONG x, z, floor_y;
            struct InsideStorey* p_inside;
            SLONG in_width;
            UBYTE* in_block;
            SLONG min_z, max_z;
            SLONG min_x, max_x;

            p_inside = &inside_storeys[INDOORS_INDEX];

            floor_y = p_inside->StoreyY;

            min_z = MAX(NGAMUT_lo_zmin, p_inside->MinZ >> 2);
            max_z = MIN(NGAMUT_lo_zmax, p_inside->MaxZ >> 2);

            in_width = p_inside->MaxX - p_inside->MinX;

            for (z = min_z; z <= max_z + 1; z++) {
                min_x = MAX(NGAMUT_lo_gamut[z].xmin, p_inside->MinX >> 2);
                max_x = MIN(NGAMUT_lo_gamut[z].xmax, p_inside->MaxX >> 2);

                if (z > min_z) {
                    min_x = MIN(NGAMUT_lo_gamut[z - 1].xmin, min_x);
                    max_x = MAX(NGAMUT_lo_gamut[z - 1].xmax, max_x);
                }

                for (x = min_x; x <= max_x + 1; x++) {
                    if (NIGHT_cache[x][z] == NULL) {
                        NIGHT_cache_create_inside(x, z, floor_y);
                        new_squares++;
                    }
                }
            }
        }

        return;
    } else if (light_inside) {
        light_inside = 0;
        for (i = 1; i < NIGHT_MAX_SQUARES; i++) {
            nq = &NIGHT_square[i];

            if (nq->flag & NIGHT_SQUARE_FLAG_USED) {
                NIGHT_cache_destroy(i);
            }
        }
    }

    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
        for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
            ASSERT(WITHIN(x, 0, PAP_SIZE_LO - 1));
            ASSERT(WITHIN(z, 0, PAP_SIZE_LO - 1));

            if (NIGHT_cache[x][z] == NULL) {
                NIGHT_cache_create(x, z);
            }
        }
    }
}

// uc_orig: AENG_mark_night_squares_as_deleteme (fallen/DDEngine/Source/aeng.cpp)
// Marks all cached lighting squares with DELETEME so unused ones can be freed.
void AENG_mark_night_squares_as_deleteme(void)
{
    SLONG i;

    NIGHT_Square* nq;

    for (i = 1; i < NIGHT_MAX_SQUARES; i++) {
        nq = &NIGHT_square[i];
        nq->flag |= NIGHT_SQUARE_FLAG_DELETEME;
    }
}

// uc_orig: AENG_ensure_appropriate_caching (fallen/DDEngine/Source/aeng.cpp)
// Ensures all squares in the lo gamut have lighting cached with the correct type
// (outdoor vs warehouse). Recreates any square that has the wrong type.
void AENG_ensure_appropriate_caching(SLONG ware)
{
    SLONG x;
    SLONG z;
    SLONG ok;

    NIGHT_Square* nq;

    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
        for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
            ASSERT(WITHIN(x, 0, PAP_SIZE_LO - 1));
            ASSERT(WITHIN(z, 0, PAP_SIZE_LO - 1));

            if (NIGHT_cache[x][z] == NULL) {
                NIGHT_cache_create(x, z, ware);
            } else {
                nq = &NIGHT_square[NIGHT_cache[x][z]];

                if (nq->flag & NIGHT_SQUARE_FLAG_WARE) {
                    ok = ware;
                } else {
                    ok = !ware;
                }

                if (!ok) {
                    NIGHT_cache_destroy(NIGHT_cache[x][z]);
                    NIGHT_cache_create(x, z, ware);
                }
            }
        }
    }
}

// uc_orig: AENG_get_rid_of_deleteme_squares (fallen/DDEngine/Source/aeng.cpp)
// Destroys all cached lighting squares that still have DELETEME set (not visited this frame).
void AENG_get_rid_of_deleteme_squares()
{
    SLONG i;

    NIGHT_Square* nq;

    for (i = 1; i < NIGHT_MAX_SQUARES; i++) {
        nq = &NIGHT_square[i];

        if (nq->flag & NIGHT_SQUARE_FLAG_USED) {
            if (nq->flag & NIGHT_SQUARE_FLAG_DELETEME) {
                NIGHT_cache_destroy(i);
            }
        }
    }
}

// uc_orig: SHADOW_Z_BIAS_BODGE (fallen/DDEngine/Source/aeng.cpp)
// Small Z offset to prevent shadow-on-floor Z-fighting.
#define SHADOW_Z_BIAS_BODGE 0.0001f

// ---------------------------------------------------------------------------
// Chunk 2: shadow projection polys, weather, hook, colour multiply, dirt, pows
// ---------------------------------------------------------------------------

// uc_orig: AENG_add_projected_shadow_poly (fallen/DDEngine/Source/aeng.cpp)
// Transforms and submits a projected shadow polygon (flat, white, no fade-out).
void AENG_add_projected_shadow_poly(SMAP_Link* sl)
{
    SLONG i;

    POLY_Point* pp;

    POLY_buffer_upto = 0;

    while (sl) {
        ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

        pp = &POLY_buffer[POLY_buffer_upto++];

        POLY_transform(
            sl->wx,
            sl->wy,
            sl->wz,
            pp);

        if (pp->MaybeValid()) {
            pp->u = AENG_project_offset_u + sl->u * AENG_project_mul_u;
            pp->v = AENG_project_offset_v + sl->v * AENG_project_mul_v;

            pp->colour = 0xffffffff;
            pp->specular = 0xff000000;

        } else {
            return;
        }

        sl = sl->next;
    }

    POLY_Point* tri[3];

    tri[0] = &POLY_buffer[0];

    for (i = 1; i < POLY_buffer_upto - 1; i++) {
        tri[1] = &POLY_buffer[i + 0];
        tri[2] = &POLY_buffer[i + 1];

        if (POLY_valid_triangle(tri)) {
            POLY_add_triangle(tri, POLY_PAGE_SHADOW, UC_TRUE);
        }
    }
}

// uc_orig: AENG_add_projected_fadeout_shadow_poly (fallen/DDEngine/Source/aeng.cpp)
// Like AENG_add_projected_shadow_poly but alpha fades out beyond 64 units from the
// light origin (fully transparent at 256 units).
void AENG_add_projected_fadeout_shadow_poly(SMAP_Link* sl)
{
    float dx;
    float dz;
    float dist;

    SLONG i;
    SLONG alpha;

    POLY_Point* pp;

    POLY_buffer_upto = 0;

    while (sl) {
        ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

        pp = &POLY_buffer[POLY_buffer_upto++];

        POLY_transform(
            sl->wx,
            sl->wy,
            sl->wz,
            pp);

        if (pp->MaybeValid()) {
            dx = sl->wx - AENG_project_fadeout_x;
            dz = sl->wz - AENG_project_fadeout_z;

            dist = fabs(dx) + fabs(dz);

            if (dist < 64.0F) {
                alpha = 0xff;
            } else {
                if (dist > 256.0F) {
                    alpha = 0;
                } else {
                    alpha = 0xff - SLONG((dist - 64.0F) * (255.0F / 192.0F));
                }
            }

            pp->u = AENG_project_offset_u + sl->u * AENG_project_mul_u;
            pp->v = AENG_project_offset_v + sl->v * AENG_project_mul_v;

            alpha |= alpha << 8;
            alpha |= alpha << 16;

            pp->colour = alpha;
            pp->specular = 0xff000000;

        } else {
            return;
        }

        sl = sl->next;
    }

    POLY_Point* tri[3];

    tri[0] = &POLY_buffer[0];

    for (i = 1; i < POLY_buffer_upto - 1; i++) {
        tri[1] = &POLY_buffer[i + 0];
        tri[2] = &POLY_buffer[i + 1];

        if (POLY_valid_triangle(tri)) {
            POLY_add_triangle(tri, POLY_PAGE_SHADOW, UC_TRUE);
        }
    }
}

// uc_orig: AENG_add_projected_lit_shadow_poly (fallen/DDEngine/Source/aeng.cpp)
// Like the shadow poly but colour is computed from distance to a point light source
// stored in AENG_project_lit_light_*.
void AENG_add_projected_lit_shadow_poly(SMAP_Link* sl)
{
    SLONG i;

    float dx;
    float dy;
    float dz;
    float dist;
    float bright;

    POLY_Point* pp;

    POLY_buffer_upto = 0;

    while (sl) {
        ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

        pp = &POLY_buffer[POLY_buffer_upto++];

        POLY_transform(
            sl->wx,
            sl->wy,
            sl->wz,
            pp);

        if (pp->MaybeValid()) {
            pp->u = AENG_project_offset_u + sl->u * AENG_project_mul_u;
            pp->v = AENG_project_offset_v + sl->v * AENG_project_mul_v;

            dx = sl->wx - AENG_project_lit_light_x;
            dy = sl->wy - AENG_project_lit_light_y;
            dz = sl->wz - AENG_project_lit_light_z;

            dist = fabs(dx) + fabs(dy) + fabs(dz);
            bright = dist / AENG_project_lit_light_range;
            bright = 1.0F - bright;
            bright *= 512.0F;

            if (bright > 0.0F) {
                SLONG alpha = SLONG(bright);

                if (alpha > 255) {
                    alpha = 255;
                }

                alpha |= alpha << 8;
                alpha |= alpha << 16;

                pp->colour = alpha;
                pp->specular = 0xff000000;
            } else {
                pp->colour = 0x00000000;
                pp->specular = 0xff000000;
            }

        } else {
            return;
        }

        sl = sl->next;
    }

    POLY_Point* tri[3];

    tri[0] = &POLY_buffer[0];

    for (i = 1; i < POLY_buffer_upto - 1; i++) {
        tri[1] = &POLY_buffer[i + 0];
        tri[2] = &POLY_buffer[i + 1];

        if (POLY_valid_triangle(tri)) {
            POLY_add_triangle(tri, POLY_PAGE_SHADOW, UC_TRUE);
        }
    }
}

// uc_orig: AENG_draw_rain_old (fallen/DDEngine/Source/aeng.cpp)
// Legacy rain: draws 2-D screen-space rain streaks (replaced by AENG_draw_rain).
void AENG_draw_rain_old(float angle)
{
    SLONG i;

    float vec1x;
    float vec1y;
    float vec2x;
    float vec2y;

    float z;
    float X;
    float Y;
    float Z;

    POLY_Point pp[3];
    POLY_Point* tri[3];

    tri[0] = &pp[0];
    tri[1] = &pp[1];
    tri[2] = &pp[2];

    pp[0].colour = 0x00000000;
    pp[1].colour = 0x88333344;
    pp[2].colour = 0x88555577;

    pp[0].specular = 0x00000000;
    pp[1].specular = 0x00000000;
    pp[2].specular = 0x00000000;

    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[1].u = 0.0F;
    pp[1].v = 0.0F;
    pp[2].u = 0.0F;
    pp[2].v = 0.0F;

#define AENG_RAIN_SIZE (4.0F)

    vec1x = (float)sin(angle) * (32.0F * AENG_RAIN_SIZE);
    vec1y = -(float)cos(angle) * (32.0F * AENG_RAIN_SIZE);

    vec2x = (float)cos(angle) * AENG_RAIN_SIZE;
    vec2y = (float)sin(angle) * AENG_RAIN_SIZE;

#define AENG_NUM_RAINDROPS 128

    for (i = 0; i < AENG_NUM_RAINDROPS; i++) {
        z = float(rand() & 0xff) * (0.5F / 256.0F) + POLY_ZCLIP_PLANE;
        X = float(rand() % DisplayWidth);
        Y = float(rand() % DisplayHeight);
        Z = POLY_ZCLIP_PLANE / z;

        pp[0].X = X;
        pp[0].Y = Y;
        pp[0].Z = Z;
        pp[0].z = z;

        pp[1].X = X + vec1x * Z;
        pp[1].Y = Y + vec1y * Z;
        pp[1].Z = Z;
        pp[1].z = z;

        pp[2].X = X + vec2x * Z;
        pp[2].Y = Y + vec2y * Z;
        pp[2].Z = Z;
        pp[2].z = z;

        POLY_add_triangle(tri, POLY_PAGE_ALPHA, UC_FALSE, UC_TRUE);
    }
}

// uc_orig: AENG_draw_rain (fallen/DDEngine/Source/aeng.cpp)
// Draws rain as 3-D world-space droplets placed in front of the camera.
// Droplets are lit using the cached night-lighting colour at their grid position.
void AENG_draw_rain()
{
    SLONG i;

    float x1;
    float y1;
    float z1;

    float x2;
    float y2;
    float z2;

    float matrix[9];

    float fade;
    SLONG bright;
    SLONG r;
    SLONG g;
    SLONG b;
    ULONG colour;

    MATRIX_calc(
        matrix,
        AENG_cam_yaw,
        AENG_cam_pitch,
        AENG_cam_roll);

    // Scale columns to map a [-1,1] unit cube in camera space to a volume
    // that covers the visible area (aspect-corrected horizontal, lens-scaled).
    matrix[0] *= 640.0F / 480.0F;
    matrix[1] *= 640.0F / 480.0F;
    matrix[2] *= 640.0F / 480.0F;

    matrix[0] /= AENG_LENS;
    matrix[1] /= AENG_LENS;
    matrix[2] /= AENG_LENS;

    matrix[3] /= AENG_LENS;
    matrix[4] /= AENG_LENS;
    matrix[5] /= AENG_LENS;

    // Rain lasts 8 squares into the distance.

    // #undef AENG_NUM_RAINDROPS
    // #define AENG_NUM_RAINDROPS	500

    matrix[0] *= 256.0F * 8.0F;
    matrix[1] *= 256.0F * 8.0F;
    matrix[2] *= 256.0F * 8.0F;

    matrix[3] *= 256.0F * 8.0F;
    matrix[4] *= 256.0F * 8.0F;
    matrix[5] *= 256.0F * 8.0F;

    matrix[6] *= 256.0F * 8.0F;
    matrix[7] *= 256.0F * 8.0F;
    matrix[8] *= 256.0F * 8.0F;

    for (i = 0; i < AENG_NUM_RAINDROPS; i++) {
        // Pick a random place in world space in front of the camera.

        x1 = float(rand()) * (1.0F / float(RAND_MAX >> 1)) - 1.0F;
        y1 = float(rand()) * (1.0F / float(RAND_MAX >> 1)) - 0.5F;
        z1 = float(rand()) * (1.0F / float(RAND_MAX)) + 0.1F;

        fade = 1.0F - z1 * 0.8F;
        bright = SLONG(fade * 256.0F);

        colour = (bright << 24) | ((69 << 16) | (74 << 8) | (98 << 0));

        MATRIX_MUL_BY_TRANSPOSE(
            matrix,
            x1,
            y1,
            z1);

        x1 += AENG_cam_x;
        y1 += AENG_cam_y;
        z1 += AENG_cam_z;

        SLONG px = SLONG(x1) >> 10;
        SLONG pz = SLONG(z1) >> 10;
        SLONG dx = (SLONG(x1) >> 8) & 3;
        SLONG dz = (SLONG(z1) >> 8) & 3;

        if ((px < 0) || (px >= PAP_SIZE_LO))
            continue;
        if ((pz < 0) || (pz >= PAP_SIZE_LO))
            continue;

        SLONG square = NIGHT_cache[px][pz];

        if (!square)
            continue;

        ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
        ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);

        NIGHT_Square* nq = &NIGHT_square[square];
        ULONG col, spec;

        NIGHT_get_d3d_colour(nq->colour[dx + dz * PAP_BLOCKS], &col, &spec);

        colour = col;

        SHAPE_droplet(
            SLONG(x1),
            SLONG(y1),
            SLONG(z1),
            8,
            -64,
            8,
            colour,
            POLY_PAGE_RAINDROP);
    }
}

// uc_orig: AENG_draw_drips (fallen/DDEngine/Source/aeng.cpp)
// Draws drip puddles and drip splashes. puddles_only selects which sub-set to render.
void AENG_draw_drips(UBYTE puddles_only)
{
    SLONG i;

    float midx;
    float midy;
    float midz;

    float px;
    float pz;

    float radius;
    ULONG colour;

    DRIP_Info* di;

    POLY_Point pp[4];
    POLY_Point* quad[4];

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[1].u = 1.0F;
    pp[1].v = 0.0F;
    pp[2].u = 0.0F;
    pp[2].v = 1.0F;
    pp[3].u = 1.0F;
    pp[3].v = 1.0F;

    pp[0].specular = 0xff000000;
    pp[1].specular = 0xff000000;
    pp[2].specular = 0xff000000;
    pp[3].specular = 0xff000000;

    DRIP_get_start();

    while (di = DRIP_get_next()) {

        if (puddles_only != (di->flags & DRIP_FLAG_PUDDLES_ONLY))
            continue;

        midx = float(di->x);
        midy = float(di->y);
        midz = float(di->z);

        midy += 8.0F;

        radius = float(di->size);

        for (i = 0; i < 4; i++) {
            px = midx + ((i & 0x1) ? +radius : -radius);
            pz = midz + ((i & 0x2) ? +radius : -radius);

            POLY_transform(px, midy, pz, &pp[i]);

            if (!pp[i].IsValid())
                continue;
        }

        if (POLY_valid_quad(quad)) {
            colour = (di->fade << 16) | (di->fade << 8) | (di->fade << 0);

            pp[0].colour = colour;
            pp[1].colour = colour;
            pp[2].colour = colour;
            pp[3].colour = colour;

            POLY_add_quad(quad, POLY_PAGE_DRIP, UC_FALSE);
        }
    }
}

// uc_orig: AENG_draw_bangs (fallen/DDEngine/Source/aeng.cpp)
// Stub — bang rendering was not implemented in this build.
void AENG_draw_bangs()
{
    float u_mid;
    float v_mid;
}

// uc_orig: AENG_draw_cloth (fallen/DDEngine/Source/aeng.cpp)
// Stub — cloth rendering was not implemented in this build.
void AENG_draw_cloth(void)
{
}

// uc_orig: AENG_draw_fire (fallen/DDEngine/Source/aeng.cpp)
// Iterates fire entries in the current gamut and kicks off rendering for each z-slice.
void AENG_draw_fire()
{
    SLONG z;

    FIRE_Info* fi;
    FIRE_Point* fp;

    for (z = NGAMUT_point_zmin; z <= NGAMUT_point_zmax; z++) {
        FIRE_get_start(
            NGAMUT_point_gamut[z].xmin,
            NGAMUT_point_gamut[z].xmax,
            z);
    }
}

// uc_orig: AENG_draw_sparks (fallen/DDEngine/Source/aeng.cpp)
// Draws sparks (SPARK) and glitter (GLITTER) for each z-slice in the gamut.
void AENG_draw_sparks()
{
    SLONG z;

    SPARK_Info* si;
    GLITTER_Info* gi;

    POLY_flush_local_rot();

    for (z = NGAMUT_point_zmin; z <= NGAMUT_point_zmax; z++) {
        SPARK_get_start(
            NGAMUT_point_gamut[z].xmin,
            NGAMUT_point_gamut[z].xmax,
            z);

        while (si = SPARK_get_next()) {
            SHAPE_sparky_line(
                si->num_points,
                si->x,
                si->y,
                si->z,
                si->colour,
                float(si->size));
        }

        GLITTER_get_start(
            NGAMUT_point_gamut[z].xmin,
            NGAMUT_point_gamut[z].xmax,
            z);

        while (gi = GLITTER_get_next()) {
            SHAPE_glitter(
                gi->x1,
                gi->y1,
                gi->z1,
                gi->x2,
                gi->y2,
                gi->z2,
                gi->colour);
        }
    }
}

// uc_orig: AENG_draw_hook (fallen/DDEngine/Source/aeng.cpp)
// Draws the grapple hook head mesh and the chain (as coloured world lines).
void AENG_draw_hook(void)
{
    SLONG i;

    SLONG x;
    SLONG y;
    SLONG z;
    SLONG yaw;
    SLONG pitch;
    SLONG roll;

    SLONG x1;
    SLONG y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG red = 0x80;
    SLONG green = 0x20;
    SLONG blue = 0x00;

    ULONG colour1;
    ULONG colour2;

    HOOK_pos_grapple(
        &x,
        &y,
        &z,
        &yaw,
        &pitch,
        &roll);

    x >>= 8;
    y >>= 8;
    z >>= 8;

    MESH_draw_poly(
        PRIM_OBJ_HOOK,
        x, y, z,
        yaw,
        pitch,
        roll,
        NULL, 0xff, 0);

    for (i = HOOK_NUM_POINTS - 1; i >= 1; i--) {
        HOOK_pos_point(i + 0, &x1, &y1, &z1);
        HOOK_pos_point(i - 1, &x2, &y2, &z2);

        x1 >>= 8;
        y1 >>= 8;
        z1 >>= 8;

        x2 >>= 8;
        y2 >>= 8;
        z2 >>= 8;

        if (red < 250) {
            red += 2;
        } else {
            if (green < 250) {
                green += 2;
            } else {
                if (blue < 250) {
                    blue += 3;
                }
            }
        }

        colour1 = (red << 16) | (green << 8) | (blue << 0);
        colour2 = (red << 17) | (green << 8) | (blue >> 1);

        AENG_world_line(
            x1, y1, z1, 0x8, colour1,
            x2, y2, z2, 0x6, colour2,
            UC_FALSE);
    }
}

// uc_orig: AENG_colour_mult (fallen/DDEngine/Source/aeng.cpp)
// Component-wise RGB multiply: result channel = (c1_ch * c2_ch) >> 8.
ULONG AENG_colour_mult(ULONG c1, ULONG c2)
{
    SLONG r1 = (c1 >> 16) & 0xff;
    SLONG g1 = (c1 >> 8) & 0xff;
    SLONG b1 = (c1 >> 0) & 0xff;

    SLONG r2 = (c2 >> 16) & 0xff;
    SLONG g2 = (c2 >> 8) & 0xff;
    SLONG b2 = (c2 >> 0) & 0xff;

    SLONG ar = r1 * r2 >> 8;
    SLONG ag = g1 * g2 >> 8;
    SLONG ab = b1 * b2 >> 8;

    ULONG ans = (ar << 16) | (ag << 8) | (ab << 0);

    return ans;
}

// uc_orig: estate (fallen/DDEngine/Source/aeng.cpp)
// Forward reference — declared extern in aeng.cpp body (original pattern preserved).
extern UBYTE estate;

// uc_orig: AENG_draw_dirt (fallen/DDEngine/Source/aeng.cpp)
// Renders all active debris (leaves, snowflakes, cans, ammo cases, water/blood
// droplets) using indexed D3D primitives for batched leaf/snow rendering.
void AENG_draw_dirt()
{
    if (GAME_FLAGS & GF_NO_FLOOR) {
        return;
    }

    SLONG i;

#define LEAF_PAGE (POLY_PAGE_LEAF)
#define LEAF_CENTRE_U (0.5F)
#define LEAF_CENTRE_V (0.5F)
#define LEAF_RADIUS (0.5F)
#define LEAF_U(a) (LEAF_CENTRE_U + LEAF_RADIUS * (float)sin(a))
#define LEAF_V(a) (LEAF_CENTRE_V + LEAF_RADIUS * (float)cos(a))

#define SNOW_CENTRE_U (0.5F)
#define SNOW_CENTRE_V (0.5F)
#define SNOW_RADIUS (1.0F)

#define LEAF_UP 8
#define LEAF_SIZE (20.0F + (float)(i & 15))

    SLONG j;

    float fyaw;
    float fpitch;
    float froll;
    float ubase;
    float vbase;

    float matrix[9];
    float angle;
    SVector_F temp[4];
    PolyPage* pp;
    D3DLVERTEX* lv;
    ULONG rubbish_colour;

    ULONG leaf_colour_choice_rgb[4] = {
        0x332d1d,
        0x243224,
        0x123320,
        0x332f07
    };

    ULONG leaf_colour_choice_grey[4] = {
        0x333333,
        0x444444,
        0x222222,
        0x383838
    };

    if (AENG_dirt_uvlookup_valid && AENG_dirt_uvlookup_world_type == world_type) {
        // Valid lookup table.
    } else {
        // Calculate the uvlookup table.

        for (i = 0; i < AENG_MAX_DIRT_UVLOOKUP; i++) {
            float angle = float(i) * (2.0F * PI / AENG_MAX_DIRT_UVLOOKUP);

            float cangle;
            float sangle;

            sangle = sinf(angle);
            cangle = cosf(angle);

            if (world_type == WORLD_TYPE_SNOW) {
                pp = &POLY_Page[POLY_PAGE_SNOWFLAKE];
                AENG_dirt_uvlookup[i].u = SNOW_CENTRE_U + sangle * SNOW_RADIUS;
                AENG_dirt_uvlookup[i].v = SNOW_CENTRE_V + cangle * SNOW_RADIUS;
            } else {
                pp = &POLY_Page[POLY_PAGE_LEAF];
                AENG_dirt_uvlookup[i].u = LEAF_CENTRE_U + sangle * LEAF_RADIUS;
                AENG_dirt_uvlookup[i].v = LEAF_CENTRE_V + cangle * LEAF_RADIUS;
            }

            AENG_dirt_uvlookup[i].u = AENG_dirt_uvlookup[i].u * pp->m_UScale + pp->m_UOffset;
            AENG_dirt_uvlookup[i].v = AENG_dirt_uvlookup[i].v * pp->m_VScale + pp->m_VOffset;
        }

        AENG_dirt_uvlookup_valid = UC_TRUE;
        AENG_dirt_uvlookup_world_type = world_type;
    }

    for (i = 0; i < 4; i++) {
        leaf_colour_choice_rgb[i] = AENG_colour_mult(leaf_colour_choice_rgb[i], NIGHT_amb_d3d_colour);
    }

    ULONG flag[4];
    ULONG leaf_colour;
    ULONG leaf_specular;

    POLY_set_local_rotation_none();
    POLY_flush_local_rot();

    AENG_dirt_lvert_upto = 0;
    AENG_dirt_index_upto = 0;

    AENG_dirt_lvert = (D3DLVERTEX*)((SLONG(AENG_dirt_lvert_buffer) + 31) & ~0x1f);
    AENG_dirt_matrix = (D3DMATRIX*)((SLONG(AENG_dirt_matrix_buffer) + 31) & ~0x1f);

    DIRT_Dirt* dd;

    for (i = 0; i < DIRT_MAX_DIRT; i++) {
        dd = &DIRT_dirt[i];

        if (dd->type == DIRT_TYPE_UNUSED) {
            continue;
        }

        dd->flag &= ~DIRT_FLAG_DELETE_OK;

        {
            float dx;
            float dy;
            float dz;

            dx = float(dd->x) - AENG_cam_x;
            dy = float(dd->y) - AENG_cam_y;
            dz = float(dd->z) - AENG_cam_z;

            float dprod;

            dprod = dx * AENG_cam_matrix[6] + dy * AENG_cam_matrix[7] + dz * AENG_cam_matrix[8];

            if (dprod < 64.0F) {
                DIRT_MARK_AS_OFFSCREEN_QUICK(i);

                goto do_next_dirt;
            }
        }

        switch (dd->type) {
        case DIRT_TYPE_LEAF:
        case DIRT_TYPE_SNOW:

        {
            if (AENG_dirt_lvert_upto + 4 > AENG_MAX_DIRT_LVERTS) {
                // Flush current batch.
                POLY_set_local_rotation_none();

                if (world_type == WORLD_TYPE_SNOW) {
                    POLY_Page[POLY_PAGE_SNOWFLAKE].RS.SetChanged();
                } else {
                    POLY_Page[POLY_PAGE_LEAF].RS.SetChanged();
                }

                the_display.lp_D3D_Device->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST,
                    D3DFVF_LVERTEX,
                    AENG_dirt_lvert,
                    AENG_dirt_lvert_upto,
                    AENG_dirt_index,
                    AENG_dirt_index_upto,
                    0);

                AENG_dirt_lvert_upto = 0;
                AENG_dirt_index_upto = 0;

                lv = AENG_dirt_lvert;
            } else {
                lv = &AENG_dirt_lvert[AENG_dirt_lvert_upto];
            }

            if ((i & 0xf) == 0 && !estate && world_type != WORLD_TYPE_SNOW) {
                // This is some rubbish (litter: paper, money, etc.)

                fpitch = float(dd->pitch) * (PI / 1024.0F);
                froll = float(dd->roll) * (PI / 1024.0F);

                // Rotation matrix (yaw = 0 optimisation from MATRIX_calc):
                float cy, cp, cr;
                float sy, sp, sr;

                sy = 0.0F;
                cy = 1.0F;

                sp = sin(fpitch);
                sr = sin(froll);

                cp = cos(fpitch);
                cr = cos(froll);

                // Note: matrix[3..5] intentionally left undefined.

                matrix[0] = cy * cr + sy * sp * sr;
                matrix[6] = sy * cp;
                matrix[1] = -cp * sr;
                matrix[7] = sp;
                matrix[2] = -sy * cr + cy * sp * sr;
                matrix[8] = cy * cp;

                matrix[0] *= 24.0F;
                matrix[1] *= 24.0F;
                matrix[2] *= 24.0F;

                matrix[6] *= 24.0F;
                matrix[7] *= 24.0F;
                matrix[8] *= 24.0F;

                float base_x = float(dd->x);
                float base_y = float(dd->y + LEAF_UP);
                float base_z = float(dd->z);

                lv[0].x = base_x + matrix[6] + matrix[0];
                lv[0].y = base_y + matrix[7] + matrix[1];
                lv[0].z = base_z + matrix[8] + matrix[2];

                lv[1].x = base_x + matrix[6] - matrix[0];
                lv[1].y = base_y + matrix[7] - matrix[1];
                lv[1].z = base_z + matrix[8] - matrix[2];

                lv[2].x = base_x - matrix[6] + matrix[0];
                lv[2].y = base_y - matrix[7] + matrix[1];
                lv[2].z = base_z - matrix[8] + matrix[2];

                lv[3].x = base_x - matrix[6] - matrix[0];
                lv[3].y = base_y - matrix[7] - matrix[1];
                lv[3].z = base_z - matrix[8] - matrix[2];

                rubbish_colour = NIGHT_amb_d3d_colour;

                if (i & 32) {
                    ubase = 0.0F;
                    vbase = 0.0F;
                } else {
                    ubase = 0.5F;
                    vbase = 0.0F;
                }

                if (i == 64) {
                    // Only one bit of money!
                    ubase = 0.0F;
                    vbase = 0.5F;
                } else {
                    if (!(i & 32)) {
                        if (i & 64) {
                            rubbish_colour &= 0xffffff00;
                        }
                    }
                }

                lv[0].tu = ubase;
                lv[0].tv = vbase;
                lv[0].color = rubbish_colour;
                lv[0].specular = 0xff000000;

                lv[1].tu = ubase + 0.5F;
                lv[1].tv = vbase;
                lv[1].color = rubbish_colour;
                lv[1].specular = 0xff000000;

                lv[2].tu = ubase;
                lv[2].tv = vbase + 0.5F;
                lv[2].color = rubbish_colour;
                lv[2].specular = 0xff000000;

                lv[3].tu = ubase + 0.5F;
                lv[3].tv = vbase + 0.5F;
                lv[3].color = rubbish_colour;
                lv[3].specular = 0xff000000;

                pp = &POLY_Page[POLY_PAGE_RUBBISH];

                lv[0].tu = lv[0].tu * pp->m_UScale + pp->m_UOffset;
                lv[0].tv = lv[0].tv * pp->m_VScale + pp->m_VOffset;

                lv[1].tu = lv[1].tu * pp->m_UScale + pp->m_UOffset;
                lv[1].tv = lv[1].tv * pp->m_VScale + pp->m_VOffset;

                lv[2].tu = lv[2].tu * pp->m_UScale + pp->m_UOffset;
                lv[2].tv = lv[2].tv * pp->m_VScale + pp->m_VOffset;

                lv[3].tu = lv[3].tu * pp->m_UScale + pp->m_UOffset;
                lv[3].tv = lv[3].tv * pp->m_VScale + pp->m_VOffset;

                ASSERT(AENG_dirt_index_upto + 6 <= AENG_MAX_DIRT_INDICES);

                AENG_dirt_index[AENG_dirt_index_upto + 0] = AENG_dirt_lvert_upto + 0;
                AENG_dirt_index[AENG_dirt_index_upto + 1] = AENG_dirt_lvert_upto + 1;
                AENG_dirt_index[AENG_dirt_index_upto + 2] = AENG_dirt_lvert_upto + 2;

                AENG_dirt_index[AENG_dirt_index_upto + 3] = AENG_dirt_lvert_upto + 3;
                AENG_dirt_index[AENG_dirt_index_upto + 4] = AENG_dirt_lvert_upto + 2;
                AENG_dirt_index[AENG_dirt_index_upto + 5] = AENG_dirt_lvert_upto + 1;

                AENG_dirt_index_upto += 6;
                AENG_dirt_lvert_upto += 4;
            } else {
                // Leaf or snowflake

                float leaf_size = LEAF_SIZE;

                if ((dd->pitch | dd->roll) == 0) {
                    // Common case — no rotation.
                    lv[0].x = float(dd->x);
                    lv[0].y = float(dd->y + LEAF_UP);
                    lv[0].z = float(dd->z + leaf_size);

                    lv[1].x = float(dd->x + leaf_size);
                    lv[1].y = float(dd->y + LEAF_UP);
                    lv[1].z = float(dd->z - leaf_size);

                    lv[2].x = float(dd->x - leaf_size);
                    lv[2].y = float(dd->y + LEAF_UP);
                    lv[2].z = float(dd->z - leaf_size);
                } else {
                    fpitch = float(dd->pitch) * (PI / 1024.0F);
                    froll = float(dd->roll) * (PI / 1024.0F);

                    // Rotation matrix (yaw = 0 optimisation from MATRIX_calc):
                    float cy, cp, cr;
                    float sy, sp, sr;

                    sy = 0.0F;
                    cy = 1.0F;

                    sp = sin(fpitch);
                    sr = sin(froll);

                    cp = cos(fpitch);
                    cr = cos(froll);

                    // Note: matrix[3..5] intentionally left undefined.

                    matrix[0] = cy * cr + sy * sp * sr;
                    matrix[6] = sy * cp;
                    matrix[1] = -cp * sr;
                    matrix[7] = sp;
                    matrix[2] = -sy * cr + cy * sp * sr;
                    matrix[8] = cy * cp;

                    matrix[0] *= leaf_size;
                    matrix[1] *= leaf_size;
                    matrix[2] *= leaf_size;

                    matrix[6] *= leaf_size;
                    matrix[7] *= leaf_size;
                    matrix[8] *= leaf_size;

                    lv[0].x = float(dd->x);
                    lv[0].y = float(dd->y + LEAF_UP);
                    lv[0].z = float(dd->z);

                    lv[1].x = lv[0].x - matrix[6] + matrix[0];
                    lv[1].y = lv[0].y - matrix[7] + matrix[1];
                    lv[1].z = lv[0].z - matrix[8] + matrix[2];

                    lv[2].x = lv[0].x - matrix[6] - matrix[0];
                    lv[2].y = lv[0].y - matrix[7] - matrix[1];
                    lv[2].z = lv[0].z - matrix[8] - matrix[2];

                    lv[0].x += matrix[6];
                    lv[0].y += matrix[7];
                    lv[0].z += matrix[8];
                }

                if (world_type == WORLD_TYPE_SNOW) {
                    DWORD dwColour = ((i & 0x0f) << 2) + 0xc0;
                    dwColour *= 0x010101;
                    dwColour |= 0xff000000;

                    lv[0].color = dwColour;
                    lv[0].specular = 0xff000000;

                    lv[1].color = dwColour;
                    lv[1].specular = 0xff000000;

                    lv[2].color = dwColour;
                    lv[2].specular = 0xff000000;
                } else {
                    leaf_colour = leaf_colour_choice_rgb[i & 0x3];

                    lv[0].color = (leaf_colour * 3) | 0xff000000;
                    lv[0].specular = 0xff000000;

                    lv[1].color = (leaf_colour * 4) | 0xff000000;
                    lv[1].specular = 0xff000000;

                    lv[2].color = (leaf_colour * 5) | 0xff000000;
                    lv[2].specular = 0xff000000;
                }

                lv[0].tu = AENG_dirt_uvlookup[(i + (AENG_MAX_DIRT_UVLOOKUP * 0 / 3)) & (AENG_MAX_DIRT_UVLOOKUP - 1)].u;
                lv[0].tv = AENG_dirt_uvlookup[(i + (AENG_MAX_DIRT_UVLOOKUP * 0 / 3)) & (AENG_MAX_DIRT_UVLOOKUP - 1)].v;

                lv[1].tu = AENG_dirt_uvlookup[(i + (AENG_MAX_DIRT_UVLOOKUP * 1 / 3)) & (AENG_MAX_DIRT_UVLOOKUP - 1)].u;
                lv[1].tv = AENG_dirt_uvlookup[(i + (AENG_MAX_DIRT_UVLOOKUP * 1 / 3)) & (AENG_MAX_DIRT_UVLOOKUP - 1)].v;

                lv[2].tu = AENG_dirt_uvlookup[(i + (AENG_MAX_DIRT_UVLOOKUP * 2 / 3)) & (AENG_MAX_DIRT_UVLOOKUP - 1)].u;
                lv[2].tv = AENG_dirt_uvlookup[(i + (AENG_MAX_DIRT_UVLOOKUP * 2 / 3)) & (AENG_MAX_DIRT_UVLOOKUP - 1)].v;

                ASSERT(AENG_dirt_index_upto + 3 <= AENG_MAX_DIRT_INDICES);

                AENG_dirt_index[AENG_dirt_index_upto + 0] = AENG_dirt_lvert_upto + 0;
                AENG_dirt_index[AENG_dirt_index_upto + 1] = AENG_dirt_lvert_upto + 1;
                AENG_dirt_index[AENG_dirt_index_upto + 2] = AENG_dirt_lvert_upto + 2;

                AENG_dirt_index_upto += 3;
                AENG_dirt_lvert_upto += 3;
            }
        }

        break;

        case DIRT_TYPE_HELDCAN:

            // Don't draw inside the car.
            {
                Thing* p_person = TO_THING(dd->droll); // droll => owner

                if (p_person->Genus.Person->InCar) {
                    continue;
                }
            }

            // FALLTHROUGH!

        case DIRT_TYPE_CAN:
        case DIRT_TYPE_THROWCAN:

            MESH_draw_poly(
                PRIM_OBJ_CAN,
                dd->x,
                dd->y,
                dd->z,
                dd->yaw,
                dd->pitch,
                dd->roll,
                NULL, 0, 0);

            break;

        case DIRT_TYPE_BRASS:

            extern UBYTE kludge_shrink;

            kludge_shrink = UC_TRUE;

            MESH_draw_poly(
                PRIM_OBJ_ITEM_AMMO_SHOTGUN,
                dd->x,
                dd->y,
                dd->z,
                dd->yaw,
                dd->pitch,
                dd->roll,
                NULL, 0, 0);

            kludge_shrink = UC_FALSE;

            break;

        case DIRT_TYPE_WATER:

            SHAPE_droplet(
                dd->x,
                dd->y,
                dd->z,
                dd->dx >> 2,
                dd->dy >> TICK_SHIFT,
                dd->dz >> 2,
                0x00224455,
                POLY_PAGE_DROPLET);
            break;

        case DIRT_TYPE_SPARKS:

            SHAPE_droplet(
                dd->x,
                dd->y,
                dd->z,
                dd->dx >> 2,
                dd->dy >> TICK_SHIFT,
                dd->dz >> 2,
                0x7f997744,
                POLY_PAGE_BLOOM1);
            break;

        case DIRT_TYPE_URINE:
            SHAPE_droplet(
                dd->x,
                dd->y,
                dd->z,
                dd->dx >> 2,
                dd->dy >> TICK_SHIFT,
                dd->dz >> 2,
                0x00775533,
                POLY_PAGE_DROPLET);
            break;

        case DIRT_TYPE_BLOOD:
            SHAPE_droplet(
                dd->x,
                dd->y,
                dd->z,
                dd->dx >> 2,
                dd->dy >> TICK_SHIFT,
                dd->dz >> 2,
                0x9fFFFFFF,
                POLY_PAGE_BLOODSPLAT);
            break;

        default:
            ASSERT(0);
            break;
        }

    do_next_dirt:;
    }

    // Draw remaining leaves/snowflakes.
    if (AENG_dirt_lvert_upto) {
        POLY_set_local_rotation_none();

        if (world_type == WORLD_TYPE_SNOW) {
            POLY_Page[POLY_PAGE_SNOWFLAKE].RS.SetChanged();
        } else {
            POLY_Page[POLY_PAGE_LEAF].RS.SetChanged();
        }

        the_display.lp_D3D_Device->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            D3DFVF_LVERTEX,
            AENG_dirt_lvert,
            AENG_dirt_lvert_upto,
            AENG_dirt_index,
            AENG_dirt_index_upto,
            0);
    }
}

// uc_orig: AENG_draw_pows (fallen/DDEngine/Source/aeng.cpp)
// Collects all active POW explosion sprites, depth-sorts them into buckets, and
// renders them as screen-aligned quads using the 4x4-frame explosion atlas.
void AENG_draw_pows(void)
{
    SLONG pow;
    SLONG sprite;
    SLONG bucket;

    AENG_Pow* ap;
    POW_Pow* pp;
    POW_Sprite* ps;

    POLY_Point pt;

    memset(AENG_pow_bucket, 0, sizeof(AENG_pow_bucket));

    AENG_pow_upto = 0;

    for (pow = POW_pow_used; pow; pow = pp->next) {
        ASSERT(WITHIN(pow, 1, POW_MAX_POWS - 1));

        pp = &POW_pow[pow];

        for (sprite = pp->sprite; sprite; sprite = ps->next) {
            ASSERT(WITHIN(sprite, 1, POW_MAX_SPRITES - 1));

            ps = &POW_sprite[sprite];

            POLY_transform(
                float(ps->x) * (1.0F / 256.0F),
                float(ps->y) * (1.0F / 256.0F),
                float(ps->z) * (1.0F / 256.0F),
                &pt);

            if (pt.clip & POLY_CLIP_TRANSFORMED) {
                ASSERT(WITHIN(AENG_pow_upto, 0, AENG_MAX_POWS - 1));

                ap = &AENG_pow[AENG_pow_upto++];

                ap->frame = ps->frame;
                ap->sx = pt.X;
                ap->sy = pt.Y;
                ap->sz = pt.z;
                ap->Z = pt.Z;
                ap->next = NULL;

                bucket = ftol(pt.z * float(AENG_POW_NUM_BUCKETS));

                SATURATE(bucket, 0, AENG_POW_NUM_BUCKETS - 1);

                ap->next = AENG_pow_bucket[bucket];
                AENG_pow_bucket[bucket] = ap;
            }
        }
    }

    // Draw the depth-sorted buckets front-to-back.
    {
        POLY_Point ppt[4];
        POLY_Point* quad[4];

        float size;
        float u;
        float v;

        ppt[0].colour = 0xffffffff;
        ppt[1].colour = 0xffffffff;
        ppt[2].colour = 0xffffffff;
        ppt[3].colour = 0xffffffff;

        ppt[0].specular = 0xff000000;
        ppt[1].specular = 0xff000000;
        ppt[2].specular = 0xff000000;
        ppt[3].specular = 0xff000000;

        quad[0] = &ppt[0];
        quad[1] = &ppt[1];
        quad[2] = &ppt[2];
        quad[3] = &ppt[3];

        for (bucket = 0; bucket < AENG_POW_NUM_BUCKETS; bucket++) {
            for (ap = AENG_pow_bucket[bucket]; ap; ap = ap->next) {
                // Push slightly forward in the z-buffer to avoid z-fighting.
                ap->sz -= 0.025F; // Half a mapsquare!

                if (ap->sz < POLY_ZCLIP_PLANE) {
                    ap->sz = POLY_ZCLIP_PLANE;
                }

                ap->Z = POLY_ZCLIP_PLANE / ap->sz;

                // Select frame from 4x4 atlas layout.
                u = float(ap->frame & 0x3) * (1.0F / 4.0F);
                v = float(ap->frame >> 2) * (1.0F / 4.0F);

                size = 650.0F * ap->Z;

                ppt[0].X = ap->sx - size;
                ppt[0].Y = ap->sy - size;
                ppt[1].X = ap->sx + size;
                ppt[1].Y = ap->sy - size;
                ppt[2].X = ap->sx - size;
                ppt[2].Y = ap->sy + size;
                ppt[3].X = ap->sx + size;
                ppt[3].Y = ap->sy + size;

                ppt[0].u = u + (0.0F / 4.0F);
                ppt[0].v = v + (0.0F / 4.0F);
                ppt[1].u = u + (1.0F / 4.0F);
                ppt[1].v = v + (0.0F / 4.0F);
                ppt[2].u = u + (0.0F / 4.0F);
                ppt[2].v = v + (1.0F / 4.0F);
                ppt[3].u = u + (1.0F / 4.0F);
                ppt[3].v = v + (1.0F / 4.0F);

                ppt[0].Z = ap->Z;
                ppt[1].Z = ap->Z;
                ppt[2].Z = ap->Z;
                ppt[3].Z = ap->Z;

                ppt[0].z = ap->sz;
                ppt[1].z = ap->sz;
                ppt[2].z = ap->sz;
                ppt[3].z = ap->sz;

                POLY_add_quad(quad, POLY_PAGE_EXPLODE1, UC_FALSE, UC_TRUE);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Chunk 3: balloons, sky, 2D rects/tris, people messages, bike wheel,
//          detail level management, floor tile infrastructure
// ---------------------------------------------------------------------------

// uc_orig: AENG_draw_released_balloons (fallen/DDEngine/Source/aeng.cpp)
// Draws any balloons that have been released (no longer held by a thing).
void AENG_draw_released_balloons(void)
{
    SLONG i;

    BALLOON_Balloon* bb;

    for (i = 1; i < BALLOON_balloon_upto; i++) {
        bb = &BALLOON_balloon[i];

        if (bb->type && !bb->thing) {
            SHAPE_draw_balloon(i);
        }
    }
}

// uc_orig: AENG_set_sky_nighttime (fallen/DDEngine/Source/aeng.cpp)
void AENG_set_sky_nighttime()
{
    AENG_sky_type = AENG_SKY_TYPE_NIGHT;
}

// uc_orig: AENG_set_sky_daytime (fallen/DDEngine/Source/aeng.cpp)
void AENG_set_sky_daytime(ULONG bottom_colour, ULONG top_colour)
{
    AENG_sky_type = AENG_SKY_TYPE_DAY;
    AENG_sky_colour_bot = bottom_colour;
    AENG_sky_colour_top = top_colour;
}

// uc_orig: AENG_draw_rectr (fallen/DDEngine/Source/aeng.cpp)
// Queues a screen-space rectangle for deferred rendering (flushed by draw_all_boxes).
void AENG_draw_rectr(SLONG x, SLONG y, SLONG w, SLONG h, SLONG col, SLONG layer, SLONG page)
{
    ASSERT(next_rrect < 2000);
    rrect[next_rrect].x = x;
    rrect[next_rrect].y = y;
    rrect[next_rrect].w = w;
    rrect[next_rrect].h = h;
    rrect[next_rrect].col = col;
    rrect[next_rrect].layer = layer;
    rrect[next_rrect].page = page;
    next_rrect++;
}

// uc_orig: AENG_draw_rect (fallen/DDEngine/Source/aeng.cpp)
// Forward declaration needed by draw_all_boxes (defined after it).
void AENG_draw_rect(SLONG x, SLONG y, SLONG w, SLONG h, SLONG col, SLONG layer, SLONG page);

// uc_orig: draw_all_boxes (fallen/DDEngine/Source/aeng.cpp)
// Flushes all queued rectangles to the screen and resets the queue.
void draw_all_boxes(void)
{
    SLONG x, y, w, h, col, layer, page;
    SLONG c0;

    for (c0 = 1; c0 < next_rrect; c0++) {
        x = rrect[c0].x;
        y = rrect[c0].y;
        w = rrect[c0].w;
        h = rrect[c0].h;
        col = rrect[c0].col;
        layer = rrect[c0].layer;
        page = rrect[c0].page;

        AENG_draw_rect(x, y, w, h, col, layer, page);
    }
    next_rrect = 0;
}

// uc_orig: AENG_BACKGROUND_COLOUR (fallen/DDEngine/Source/aeng.cpp)
#define AENG_BACKGROUND_COLOUR 0x55888800

// uc_orig: AENG_draw_rect (fallen/DDEngine/Source/aeng.cpp)
// Draws a solid-colour 2D screen-space rectangle as a quad polygon.
void AENG_draw_rect(SLONG x, SLONG y, SLONG w, SLONG h, SLONG col, SLONG layer, SLONG page)
{
    float offset = 0.0;
    POLY_Point pp[4];
    POLY_Point* quad[4];
    float top, bottom, left, right;

    offset = ((float)layer) * 0.0001f;

    top = (float)y;
    bottom = (float)(y + h);
    left = (float)x;
    right = (float)(x + w);

    pp[0].X = left;
    pp[0].Y = top;
    pp[0].z = 0.0F + offset;
    pp[0].Z = 1.0F - offset;
    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[0].colour = col;
    pp[0].specular = 0;

    pp[1].X = right;
    pp[1].Y = top;
    pp[1].z = 0.0F + offset;
    pp[1].Z = 1.0F - offset;
    pp[1].u = 0.0F;
    pp[1].v = 0.0F;
    pp[1].colour = col;
    pp[1].specular = 0;

    pp[2].X = left;
    pp[2].Y = bottom;
    pp[2].z = 0.0F + offset;
    pp[2].Z = 1.0F - offset;
    pp[2].u = 0.0F;
    pp[2].v = 0.0F;
    pp[2].colour = col;
    pp[2].specular = 0;

    pp[3].X = right;
    pp[3].Y = bottom;
    pp[3].z = 0.0F + offset;
    pp[3].Z = 1.0F - offset;
    pp[3].u = 0.0F;
    pp[3].v = 0.0F;
    pp[3].colour = col;
    pp[3].specular = 0;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
}

// uc_orig: AENG_draw_col_tri (fallen/DDEngine/Source/aeng.cpp)
// Draws a solid-colour 2D screen-space triangle as a poly triangle.
void AENG_draw_col_tri(SLONG x0, SLONG y0, SLONG col0, SLONG x1, SLONG y1, SLONG col1, SLONG x2, SLONG y2, SLONG col2, SLONG layer)
{
    float offset = 0.0;
    POLY_Point pp[4];
    POLY_Point* tri[4];
    POLY_Point* quad[4];

    float left, right, top, bottom;

    left = 100.0;
    right = 200.0;
    top = 100.0;
    bottom = 200.0;

    offset = ((float)layer) * 0.0001f;

    /*
            #define AENG_BACKGROUND_COLOUR 0x55888800

            pp[0].X        = left;
            pp[0].Y        = top;
            pp[0].z        = 0.0F;
            pp[0].Z        = 1.0F;
            pp[0].u        = 0.0F;
            pp[0].v        = 0.0F;
            pp[0].colour   = AENG_BACKGROUND_COLOUR;
            pp[0].specular = 0;

            pp[1].X        = right;
            pp[1].Y        = top;
            pp[1].z        = 0.0F;
            pp[1].Z        = 1.0F;
            pp[1].u        = 0.0F;
            pp[1].v        = 0.0F;
            pp[1].colour   = AENG_BACKGROUND_COLOUR;
            pp[1].specular = 0;

            pp[2].X        = left;
            pp[2].Y        = bottom;
            pp[2].z        = 0.0F;
            pp[2].Z        = 1.0F;
            pp[2].u        = 0.0F;
            pp[2].v        = 0.0F;
            pp[2].colour   = AENG_BACKGROUND_COLOUR;
            pp[2].specular = 0;

            pp[3].X        = right;
            pp[3].Y        = bottom;
            pp[3].z        = 0.0F;
            pp[3].Z        = 1.0F;
            pp[3].u        = 0.0F;
            pp[3].v        = 0.0F;
            pp[3].colour   = AENG_BACKGROUND_COLOUR;
            pp[3].specular = 0;

            quad[0] = &pp[0];
            quad[1] = &pp[1];
            quad[2] = &pp[2];
            quad[3] = &pp[3];

            POLY_add_quad(quad, POLY_PAGE_COLOUR, UC_FALSE, UC_TRUE);
    */

    pp[0].X = (float)x0;
    pp[0].Y = (float)y0;
    pp[0].z = 0.0F + offset;
    pp[0].Z = 1.0F - offset;
    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[0].colour = col0;
    pp[0].specular = 0;

    pp[1].X = (float)x1;
    pp[1].Y = (float)y1;
    pp[1].z = 0.0F + offset;
    pp[1].Z = 1.0F - offset;
    pp[1].u = 0.0F;
    pp[1].v = 0.0F;
    pp[1].colour = col1;
    pp[1].specular = 0;

    pp[2].X = (float)x2;
    pp[2].Y = (float)y2;
    pp[2].z = 0.0F + offset;
    pp[2].Z = 1.0F - offset;
    pp[2].u = 0.0F;
    pp[2].v = 0.0F;
    pp[2].colour = col2;
    pp[2].specular = 0;

    tri[0] = &pp[0];
    tri[1] = &pp[1];
    tri[2] = &pp[2];

    POLY_add_triangle(tri, POLY_PAGE_COLOUR, UC_FALSE, UC_TRUE);
}

// uc_orig: show_gamut_lo (fallen/DDEngine/Source/aeng.cpp)
// Debug stub: was intended to visualise lo-res gamut cells as coloured rectangles.
void show_gamut_lo(SLONG x, SLONG z)
{
    return;
}

// uc_orig: show_gamut_hi (fallen/DDEngine/Source/aeng.cpp)
// Debug stub: was intended to visualise hi-res gamut cells as coloured pixels.
void show_gamut_hi(SLONG x, SLONG z)
{
    return;
}

// show_facet is defined in aeng_globals.h

// uc_orig: AENG_draw_people_messages (fallen/DDEngine/Source/aeng.cpp)
// Debug stub: iterates visible things and overlays state text. Returns immediately (disabled).
void AENG_draw_people_messages()
{
    return;

    SLONG x;
    SLONG z;

    SLONG t_index;
    Thing* p_thing;

    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
        for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
            t_index = PAP_2LO(x, z).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                if (p_thing->Flags & FLAGS_IN_BUILDING) {
                    // Don't draw things inside buildings when outdoors.
                } else {
                    switch (p_thing->DrawType) {
                    case DT_ROT_MULTI:

                        if (POLY_sphere_visible(
                                float(p_thing->WorldPos.X >> 8),
                                float(p_thing->WorldPos.Y >> 8) + KERB_HEIGHT,
                                float(p_thing->WorldPos.Z >> 8),
                                256.0F / (AENG_DRAW_DIST * 256.0F))) {
                            CBYTE str[100];

                            sprintf(str, "%d %d", p_thing->State, p_thing->SubState);
                            AENG_world_text(
                                (p_thing->WorldPos.X >> 8),
                                (p_thing->WorldPos.Y >> 8) + 0x60,
                                (p_thing->WorldPos.Z >> 8),
                                200,
                                180,
                                50,
                                UC_TRUE,
                                str);
                        }

                        break;
                    }
                }

                t_index = p_thing->Child;
            }
        }
    }
}

// uc_orig: AENG_set_bike_wheel_rotation (fallen/DDEngine/Source/aeng.cpp)
// Updates UV offsets on the two bike wheel prim faces (faces 6+7) to animate rolling.
void AENG_set_bike_wheel_rotation(UWORD rot, UBYTE prim)
{
    SLONG i;

    PrimObject* po;
    PrimFace4* f4;

    po = &prim_objects[prim];

    SLONG du1 = SIN(+rot & 2047) * 15 >> 16;
    SLONG dv1 = COS(+rot & 2047) * 15 >> 16;

    SLONG du2 = SIN(-rot & 2047) * 15 >> 16;
    SLONG dv2 = COS(-rot & 2047) * 15 >> 16;

    SLONG u;
    SLONG v;

    static SLONG order[4] = { 2, 1, 3, 0 };

    f4 = &prim_faces4[po->StartFace4 + 6];

    for (i = 0; i < 4; i++) {
        switch (order[i]) {
        case 0:
            u = 16 + du1;
            v = 16 + dv1;
            break;
        case 1:
            u = 16 + dv1;
            v = 16 - du1;
            break;
        case 2:
            u = 16 - du1;
            v = 16 - dv1;
            break;
        case 3:
            u = 16 - dv1;
            v = 16 + du1;
            break;
        }

        f4[0].UV[i][0] &= ~0x3f;
        f4[0].UV[i][1] &= ~0x3f;

        f4[0].UV[i][0] |= u;
        f4[0].UV[i][1] |= v;

        switch (order[i]) {
        case 0:
            u = 16 + du2;
            v = 16 + dv2;
            break;
        case 1:
            u = 16 + dv2;
            v = 16 - du2;
            break;
        case 2:
            u = 16 - du2;
            v = 16 - dv2;
            break;
        case 3:
            u = 16 - dv2;
            v = 16 + du2;
            break;
        }

        f4[1].UV[i][0] &= ~0x3f;
        f4[1].UV[i][1] &= ~0x3f;

        f4[1].UV[i][0] |= u;
        f4[1].UV[i][1] |= v;
    }
}

/*
uc_orig: AENG_draw_warehouse_floor_near_door (fallen/DDEngine/Source/aeng.cpp)
This function was commented out in the original source (#if 0 / block comment).
Dead code — not migrated.
*/

// uc_orig: AENG_set_detail_levels (fallen/DDEngine/Source/aeng.cpp)
// Writes new detail level settings to the config (INI) and re-reads them.
void AENG_set_detail_levels(int stars,
    int shadows,
    int moon_reflection,
    int people_reflection,
    int puddles,
    int dirt,
    int mist,
    int rain,
    int skyline,
    int filter,
    int perspective,
    int crinkles)
{
    ENV_set_value_number("detail_shadows", shadows, "Render");
    ENV_set_value_number("detail_puddles", puddles, "Render");
    ENV_set_value_number("detail_dirt", dirt, "Render");
    ENV_set_value_number("detail_mist", mist, "Render");
    ENV_set_value_number("detail_rain", rain, "Render");
    ENV_set_value_number("detail_skyline", skyline, "Render");
    ENV_set_value_number("detail_crinkles", crinkles, "Render");

    AENG_read_detail_levels();
}

// uc_orig: AENG_draw_some_polys (fallen/DDEngine/Source/aeng.cpp)
// Benchmark helper: draws N triangles and returns elapsed time in seconds.
float AENG_draw_some_polys(bool large, bool blend)
{
    PolyPoint2D *vert, *vp;
    WORD *ind, *ip;

    vert = new PolyPoint2D[large ? 300 : 30000];
    vp = vert;

    ind = new WORD[large ? 300 : 30000];
    ip = ind;

    float u = 0;
    float v = 0;

    if (large) {
        for (int ii = 0; ii < 100; ii++) {
            vp->SetSC(0, 0);
            vp->SetColour(0x80FFFFFF);
            vp->SetSpecular(0);
            vp->SetUV(u, v);
            vp++;

            vp->SetSC(640, 0);
            vp->SetColour(0x80FFFFFF);
            vp->SetSpecular(0);
            vp->SetUV(u, v);
            vp++;

            vp->SetSC(0, 480);
            vp->SetColour(0x80FFFFFF);
            vp->SetSpecular(0);
            vp->SetUV(u, v);
            vp++;

            *ip++ = ii * 3;
            *ip++ = ii * 3 + 1;
            *ip++ = ii * 3 + 2;
        }
    } else {
        for (int ii = 0; ii < 10000; ii++) {
            int x = ii % 20;
            int y = (ii / 20) % 15;

            vp->SetSC(x * 32, y * 32);
            vp->SetColour(0x80FFFFFF);
            vp->SetSpecular(0);
            vp->SetUV(u, v);
            vp++;

            vp->SetSC(x * 32 + 32, y * 32);
            vp->SetColour(0x80FFFFFF);
            vp->SetSpecular(0);
            vp->SetUV(u, v);
            vp++;

            vp->SetSC(x * 32, y * 32 + 32);
            vp->SetColour(0x80FFFFFF);
            vp->SetSpecular(0);
            vp->SetUV(u, v);
            vp++;

            *ip++ = ii * 3;
            *ip++ = ii * 3 + 1;
            *ip++ = ii * 3 + 2;
        }
    }

    StartStopwatch();

    BEGIN_SCENE;

    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZENABLE, UC_FALSE);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZWRITEENABLE, UC_FALSE);
    if (blend) {
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, UC_TRUE);
    } else {
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, UC_FALSE);
    }

    if (large) {
        HRESULT res = DRAW_INDEXED_PRIMITIVE(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, (D3DTLVERTEX*)vert, 300, ind, 300, D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT);
        ASSERT(!FAILED(res));
    } else {
        HRESULT res = DRAW_INDEXED_PRIMITIVE(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, (D3DTLVERTEX*)vert, 30000, ind, 30000, D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT);
        ASSERT(!FAILED(res));
    }

    END_SCENE;

    the_display.screen_lock();
    the_display.screen_unlock();

    float time = StopStopwatch();

    delete[] vert;
    delete[] ind;

    return time;
}

// uc_orig: GENVAR (fallen/DDEngine/Source/aeng.cpp)
// Returns 1 if the GPU generation is >= G, 0 otherwise. Used in AENG_guess_detail_levels.
#define GENVAR(G) ((generation >= G) ? 1 : 0)

// uc_orig: AENG_guess_detail_levels (fallen/DDEngine/Source/aeng.cpp)
// Benchmarks the GPU and sets detail levels based on measured fill rate.
void AENG_guess_detail_levels()
{
    if (!AENG_estimate_detail_levels)
        return;

    ENV_set_value_number("estimate_detail_levels", 0, "Render");
    AENG_estimate_detail_levels = 0;

    int generation;

    D3DDeviceInfo* dev = the_display.GetDeviceInfo();

    if (!dev->IsHardware()) {
        generation = 0;
    } else {
        float tso = 10000 / AENG_draw_some_polys(false, false);

        if (tso < 10000) {
            generation = 0;
        } else if (tso < 40000) {
            generation = 1;
        } else {
            generation = 2;
            if (dev->ModulateAlphaSupported() && dev->DestInvSourceColourSupported()) {
                generation = 3;
            }
        }
    }

    int stars = GENVAR(0);
    int shadows = GENVAR(2);
    int moon_reflection = GENVAR(2);
    int people_reflection = GENVAR(3);
    int puddles = GENVAR(2);
    int dirt = GENVAR(1);
    int mist = GENVAR(2);
    int rain = GENVAR(0);
    int skyline = GENVAR(2);
    int filter = GENVAR(1);
    int perspective = GENVAR(1);
    int crinkles = GENVAR(3);

    AENG_set_detail_levels(stars, shadows, moon_reflection, people_reflection, puddles, dirt, mist, rain, skyline, filter, perspective, crinkles);
}

// uc_orig: AENG_get_detail_levels (fallen/DDEngine/Source/aeng.cpp)
// Reads current detail level settings into output parameters.
void AENG_get_detail_levels(int* stars,
    int* shadows,
    int* moon_reflection,
    int* people_reflection,
    int* puddles,
    int* dirt,
    int* mist,
    int* rain,
    int* skyline,
    int* filter,
    int* perspective,
    int* crinkles)
{
    *shadows = AENG_detail_shadows;
    *puddles = AENG_detail_puddles;
    *dirt = AENG_detail_dirt;
    *mist = AENG_detail_mist;
    *rain = AENG_detail_rain;
    *skyline = AENG_detail_skyline;
    *crinkles = AENG_detail_crinkles;
}

// ---------------------------------------------------------------------------
// Floor tile rendering infrastructure: cache_a_row, add_kerb, draw_i_prim, general_steam
// These are used by draw_quick_floor (chunk 4).
// ---------------------------------------------------------------------------

// uc_orig: cache_a_row (fallen/DDEngine/Source/aeng.cpp)
// Pre-fetches lighting and PAP data for a row of map tile corners into a FloorStore array.
void cache_a_row(SLONG x, SLONG z, struct FloorStore* p2, SLONG endx)
{
    SLONG px, pz, dx, dz;
    SLONG square;
    SLONG mapz;
    NIGHT_Square* nq;
    PAP_Hi* ph;
    ULONG spec;
    SLONG y;

    for (ph = &PAP_2HI(x, z); x <= endx; x++, ph += PAP_SIZE_HI) {
        float dist;

        y = ph->Alt << ALT_SHIFT;

        p2->Alt = float(y);

        px = x >> 2;
        pz = z >> 2;

        dx = x & 0x3;
        dz = z & 0x3;

        square = NIGHT_cache[px][pz];

        ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
        ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);

        nq = &NIGHT_square[square];

        /*

        {
                SLONG	cdx,cdz;
                cdx=abs(AENG_cam_x-(x<<8));
                cdz=abs(AENG_cam_z-(z<<8));
                mapz = QDIST2(cdx,cdz);
                dist=((float)mapz)/(float)(AENG_DRAW_DIST<<8);
                if(dist>1.0f)
                        dist=1.0f;
        }

        NIGHT_get_d3d_colour_and_fade(
                nq->colour[dx + dz * PAP_BLOCKS],
           &p2->Colour,
           &spec,dist);

        */

        {
            NIGHT_Colour* col = &nq->colour[dx + dz * PAP_BLOCKS];

            SLONG r = col->red << 2;
            SLONG g = col->green << 2;
            SLONG b = col->blue << 2;

            if (r > 255) {
                r = 255;
            }
            if (g > 255) {
                g = 255;
            }
            if (b > 255) {
                b = 255;
            }

            p2->Colour = (r << 16) | (g << 8) | b;
        }

        p2->Flags = ph->Flags;
        p2->Texture = ph->Texture;

        p2++;
    }
}

// uc_orig: add_kerb (fallen/DDEngine/Source/aeng.cpp)
// Emits four vertices for one kerb strip quad into the D3DLVERTEX buffer, with
// camera-distance Z-rejection. Returns UC_TRUE if the quad was added.
SLONG add_kerb(float alt1, float alt2, SLONG x, SLONG z, SLONG dx, SLONG dz, D3DLVERTEX* pv, UWORD* p_indicies, SLONG count, ULONG c1, ULONG c2, SLONG flip)
{
    pv->x = x * 256.0F;
    pv->z = z * 256.0F;
    pv->y = alt1 - KERB_HEIGHT;

    pv->tu = 0.0f;
    pv->tv = 1.0f;

    pv->tu = pv->tu * kerb_scaleu + kerb_du;
    pv->tv = pv->tv * kerb_scalev + kerb_dv;

    pv->color = c1;
    pv->specular = 0xff000000;
    SET_MM_INDEX(*pv, 0);
    pv++;

    pv->x = (x + dx) * 256.0F;
    pv->z = (z + dz) * 256.0F;
    pv->y = alt2 - KERB_HEIGHT;

    pv->tu = 1.0f;
    pv->tv = 1.0f;

    pv->tu = pv->tu * kerb_scaleu + kerb_du;
    pv->tv = pv->tv * kerb_scalev + kerb_dv;

    pv->color = c2;
    pv->specular = 0xff000000;
    SET_MM_INDEX(*pv, 0);
    pv++;

    pv->x = (x + dx) * 256.0F;
    pv->z = (z + dz) * 256.0F;
    pv->y = alt2;

    pv->tu = 1.0f;
    pv->tv = 0.0f;

    pv->tu = pv->tu * kerb_scaleu + kerb_du;
    pv->tv = pv->tv * kerb_scalev + kerb_dv;

    pv->color = c2;
    pv->specular = 0xff000000;
    SET_MM_INDEX(*pv, 0);
    pv++;

    pv->x = (x) * 256.0F;
    pv->z = (z) * 256.0F;
    pv->y = alt1;

    pv->tu = 0.0f;
    pv->tv = 0.0f;

    pv->tu = pv->tu * kerb_scaleu + kerb_du;
    pv->tv = pv->tv * kerb_scalev + kerb_dv;

    pv->color = c1;
    pv->specular = 0xff000000;
    SET_MM_INDEX(*pv, 0);
    pv++;

    pv -= 4;

    {
        SLONG i;

        float dx;
        float dy;
        float dz;

        float dprod;

        dx = pv[0].x - AENG_cam_x;
        dy = pv[0].y - AENG_cam_y;
        dz = pv[0].z - AENG_cam_z;

        float dist;

        float adx;
        float ady;
        float adz;

        adx = fabsf(dx);
        ady = fabsf(dy);
        adz = fabsf(dz);

        dist = adx + ady + adz;

        if (dist > 768.0F) {
            // No need to zclip!
        } else {
            for (i = 0; i < 4; i++) {
                dx = pv[i].x - AENG_cam_x;
                dy = pv[i].y - AENG_cam_y;
                dz = pv[i].z - AENG_cam_z;

                dprod = dx * AENG_cam_matrix[6] + dy * AENG_cam_matrix[7] + dz * AENG_cam_matrix[8];

                if (dprod < 8.0F) {
                    return UC_FALSE;
                }
            }
        }
    }

    if (flip) {
        *p_indicies++ = count + 0;
        *p_indicies++ = count + 3;
        *p_indicies++ = count + 1;

        *p_indicies++ = count + 2;
        *p_indicies++ = 0xffff;
    } else {
        *p_indicies++ = count;
        *p_indicies++ = count + 1;
        *p_indicies++ = count + 3;

        *p_indicies++ = count + 2;
        *p_indicies++ = 0xffff;
    }

    return UC_TRUE;
}

// uc_orig: draw_i_prim (fallen/DDEngine/Source/aeng.cpp)
// Flushes one indexed primitive strip group to the GPU using DrawIndPrimMM.
void draw_i_prim(LPDIRECT3DTEXTURE2 page, D3DLVERTEX* verts, UWORD* indicies, SLONG* vert_count, SLONG* index_count, D3DMULTIMATRIX* mm_draw_floor)
{
    HRESULT res;

    mm_draw_floor->lpvVertices = verts;

    indicies[*index_count] = 0x1234;

    REALLY_SET_TEXTURE(page);

    res = DrawIndPrimMM(the_display.lp_D3D_Device, D3DFVF_LVERTEX, mm_draw_floor, *vert_count, indicies, *index_count);

    ASSERT(res == DD_OK);

    *index_count = 0;
    *vert_count = 0;
}

// HALF_COL is defined in aeng_globals.h

// uc_orig: general_steam (fallen/DDEngine/Source/aeng.cpp)
// Accumulates steam source positions during floor tile rendering (mode=1),
// renders all of them when flushed (mode=2), or resets (mode=0).
void general_steam(SLONG x, SLONG z, UWORD texture, SLONG mode)
{
    static SLONG stx[MAX_STEAM], sty[MAX_STEAM], stz[MAX_STEAM], lod[MAX_STEAM];
    static SLONG count_steam = 0;

    if (mode == 0) {
        count_steam = 0;
        return;
    } else if (mode == 2) {
        for (SLONG c0 = 0; c0 < count_steam; c0++) {
            extern void draw_steam(SLONG x, SLONG y, SLONG z, SLONG lod);
            draw_steam(stx[c0], sty[c0], stz[c0], lod[c0]);
        }
        count_steam = 0;
        return;
    }
    if (count_steam >= MAX_STEAM)
        return;

    if (AENG_detail_shadows) {
        SLONG dx, dz, dist, sx, sy, sz;

        dx = abs((((SLONG)AENG_cam_x) >> 8) - (x));
        dz = abs((((SLONG)AENG_cam_z) >> 8) - (z));

        dist = QDIST2(dx, dz);

        if (dist < 15) {
            SLONG sx, sy, sz;
            switch ((texture >> 0xa) & 0x3) {
            case 0:
                sx = 190;
                sz = 128;
                break;
            case 1:
                sx = 128;
                sz = 66;
                break;
            case 2:
                sx = 66;
                sz = 128;
                break;
            case 3:
                sx = 128;
                sz = 190;
                break;
            default:
                ASSERT(0);
                break;
            }
            sx += x << 8;
            sz += z << 8;
            sy = PAP_calc_height_at(sx, sz);

            stx[count_steam] = sx;
            sty[count_steam] = sy;
            stz[count_steam] = sz;
            lod[count_steam] = 10 + (15 - dist) * 3;
            count_steam++;
        }
    }
}

// uc_orig: draw_quick_floor (fallen/DDEngine/Source/aeng.cpp)
// Renders all visible ground tiles as indexed triangle strips, batched by texture page.
// Handles kerb geometry between sunken (road) and normal squares.
// Also processes shadow-lit quads which require an extra split vertex.
// `warehouse` non-zero: only render hidden (PAP_FLAG_HIDDEN) tiles for warehouse floor-over-city.
void draw_quick_floor(SLONG warehouse)
{
    PAP_Hi* ph;

    SLONG c0;
    SLONG x, z;
    float dy, y;
    UWORD index;

    SLONG page, page2, prev_page = -10000, apage = 0;

    SLONG current_set = 0;

    struct GroupInfo group[IPRIM_COUNT];

    PolyPage* pp;
    LPDIRECT3DTEXTURE2 tex_handle;

    UWORD kerb_indicies[KERB_INDICIES];

    UWORD* p_indicies;

    D3DMATRIX* m_view;
    D3DLVERTEX *p_verts[IPRIM_COUNT], *pv, *kerb_verts;

    UBYTE some_data[sizeof(D3DMATRIX) + 32]; // 32-byte alignment staging buffer
    UBYTE* ptr32;

    SLONG index_count[IPRIM_COUNT], vert_count[IPRIM_COUNT], age[IPRIM_COUNT], kerb_counti = 0, kerb_countv = 0;

    SLONG bin_set;

    D3DMULTIMATRIX mm_draw_floor;

    static int init_stats = 1;

    static SLONG biggest = 0;

    struct FloorStore row[MAX_DRAW_WIDTH * 2 + 2];
    struct FloorStore *p1, *p2;
    SLONG startx, endx, offsetx;
    SLONG no_floor = 0;
    SLONG is_shadow;

    pp = &POLY_Page[0];

    kerb_du = pp->m_UOffset;
    kerb_dv = pp->m_VOffset;
    kerb_scaleu = pp->m_UScale;
    kerb_scalev = pp->m_VScale;

    if (GAME_FLAGS & GF_NO_FLOOR)
        no_floor = 1;

    general_steam(0, 0, 0, 0); // init it

    memset(group, 0, sizeof(struct GroupInfo) * IPRIM_COUNT);

    if (init_stats) {
        init_stats = 0;
    }

    ptr32 = (UBYTE*)(((ULONG)(some_data + 32)) & 0xffffffe0);
    m_view = (D3DMATRIX*)ptr32;

    mm_draw_floor.lpd3dMatrices = m_view;
    mm_draw_floor.lpvLightDirs = NULL;
    mm_draw_floor.lpLightTable = NULL;

    ptr32 = (UBYTE*)(((ULONG)(m_vert_mem_block32 + 32)) & 0xffffffe0);

    kerb_verts = (D3DLVERTEX*)ptr32;
    ptr32 += sizeof(D3DLVERTEX) * KERB_VERTS;

    for (c0 = 0; c0 < IPRIM_COUNT; c0++) {
        p_verts[c0] = (D3DLVERTEX*)ptr32;
        ptr32 += sizeof(D3DLVERTEX) * MAX_VERTS_FOR_STRIPS;
        index_count[c0] = 0;
        vert_count[c0] = 0;
        age[c0] = 0x7fff;
    }

    BEGIN_SCENE;

    RenderState default_renderstate;

    default_renderstate.SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
    default_renderstate.SetChanged();

    GenerateMMMatrixFromStandardD3DOnes(m_view, &g_matProjection, NULL, &g_viewData);

    z = NGAMUT_zmin;

    startx = NGAMUT_point_gamut[z].xmin;
    endx = NGAMUT_point_gamut[z].xmax;

    extern SLONG NGAMUT_xmin;

    offsetx = startx - (NGAMUT_xmin);

    ASSERT(offsetx < MAX_DRAW_WIDTH);
    ASSERT(offsetx + endx - startx < MAX_DRAW_WIDTH);
    if (z & 1) {
        p1 = &row[0 + offsetx];
    } else {
        p1 = &row[MAX_DRAW_WIDTH + offsetx];
    }
    cache_a_row(startx, z, p1, endx);

    if (!INDOORS_INDEX)
        for (z = NGAMUT_zmin; z <= NGAMUT_zmax; z++) {

            startx = NGAMUT_point_gamut[z + 1].xmin;
            endx = NGAMUT_point_gamut[z + 1].xmax;
            offsetx = startx - (NGAMUT_xmin);

            ASSERT(offsetx < MAX_DRAW_WIDTH);
            ASSERT(offsetx + endx - startx < MAX_DRAW_WIDTH);

            if (z & 1) {
                p2 = &row[MAX_DRAW_WIDTH + offsetx];
                memset(&row[MAX_DRAW_WIDTH], 0, MAX_DRAW_WIDTH * sizeof(struct FloorStore));
            } else {
                p2 = &row[0 + offsetx];
                memset(&row[0], 0, MAX_DRAW_WIDTH * sizeof(struct FloorStore));
            }

            cache_a_row(startx, z + 1, p2, endx);

            offsetx = NGAMUT_gamut[z].xmin - (NGAMUT_xmin);

            ASSERT(offsetx < MAX_DRAW_WIDTH);
            ASSERT(offsetx + endx - startx < MAX_DRAW_WIDTH);

            if (z & 1) {
                p1 = &row[0 + offsetx];
                p2 = &row[MAX_DRAW_WIDTH + offsetx];
            } else {
                p1 = &row[MAX_DRAW_WIDTH + offsetx];
                p2 = &row[0 + offsetx];
            }

            for (x = NGAMUT_gamut[z].xmin; x <= NGAMUT_gamut[z].xmax; x++, p1++, p2++) {
                ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
                ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

                ASSERT(p1 >= &row[0]);
                ASSERT(p2 >= &row[0]);
                ASSERT(p1 < &row[MAX_DRAW_WIDTH * 2 + 2]);
                ASSERT(p2 < &row[MAX_DRAW_WIDTH * 2 + 2]);

                ph = &PAP_2HI(x, z);

                if (warehouse) {
                    if (!(p1->Flags & PAP_FLAG_HIDDEN)) {
                        continue;
                    }

                } else {
                    SLONG s1, s2;

                    s1 = p1->Flags & PAP_FLAG_SINK_SQUARE;
                    s2 = (p1 + 1)->Flags & PAP_FLAG_SINK_SQUARE;

                    if (s1 != s2) {
                        // change of sink status along x — draw a kerb edge
                        if (kerb_countv >= KERB_VERTS - 4) {
                            draw_i_prim(POLY_Page[0].RS.GetTexture(), kerb_verts, kerb_indicies, &kerb_countv, &kerb_counti, &mm_draw_floor);
                        }

                        if (add_kerb((p1 + 1)->Alt, (p2 + 1)->Alt, x + 1, z, 0, 1, &kerb_verts[kerb_countv], &kerb_indicies[kerb_counti], kerb_countv, (p1 + 1)->Colour, (p2 + 1)->Colour, s1)) {
                            kerb_countv += 4;
                            kerb_counti += 5;
                        }
                    }

                    s1 = p1->Flags & PAP_FLAG_SINK_SQUARE;
                    s2 = (p2)->Flags & PAP_FLAG_SINK_SQUARE;

                    if (s1 != s2) {
                        // change of sink status along z — draw a kerb edge
                        if (kerb_countv >= KERB_VERTS - 4) {
                            draw_i_prim(POLY_Page[0].RS.GetTexture(), kerb_verts, kerb_indicies, &kerb_countv, &kerb_counti, &mm_draw_floor);
                        }

                        if (add_kerb((p2)->Alt, (p2 + 1)->Alt, x, z + 1, 1, 0, &kerb_verts[kerb_countv], &kerb_indicies[kerb_counti], kerb_countv, (p2)->Colour, (p2 + 1)->Colour, s2)) {
                            kerb_countv += 4;
                            kerb_counti += 5;
                        }
                    }

                    // MikeD: must be inside the else block.
                    if ((p1->Flags & (PAP_FLAG_HIDDEN | PAP_FLAG_ROOF_EXISTS)) == PAP_FLAG_HIDDEN) {
                        continue;
                    }
                }

                if (warehouse == 0 && (p1->Flags & (PAP_FLAG_ROOF_EXISTS))) {
                    y = MAVHEIGHT(x, z) << 6;
                    if (y > AENG_cam_y)
                        continue;
                } else {
                    if (no_floor) {
                        // Don't draw the floor if there isn't any (like the final level).
                        continue;
                    }
                }

                if (warehouse)
                    is_shadow = 0;
                else
                    is_shadow = p1->Flags & (PAP_FLAG_SHADOW_1 | PAP_FLAG_SHADOW_2 | PAP_FLAG_SHADOW_3);

                page = p1->Texture & 0x3ff;

                if (page == 4 * 64 + 53)
                    general_steam(x, z, p1->Texture, 1); // store it

                pp = &POLY_Page[page];

                tex_handle = pp->RS.GetTexture();

                current_set = -1;

                // age the iprims
                for (c0 = 0; c0 < IPRIM_COUNT; c0++)
                    age[c0]++;

                for (c0 = 0; c0 < IPRIM_COUNT; c0++) {
                    if (group[c0].page == tex_handle) {
                        age[c0] = 0;
                        current_set = c0;
                        break;
                    }
                }

                bin_set = -1;
                if (current_set == -1) {
                    SLONG oldest = -1;
                    // no group currently supports this new page, find empty or evict oldest
                    for (c0 = 0; c0 < IPRIM_COUNT; c0++) {
                        if (vert_count[c0] == 0) {
                            bin_set = -1; // no need to bin an empty one
                            current_set = c0;

                            group[current_set].page = tex_handle;

                            break;
                        }

                        if (age[c0] > oldest) {
                            bin_set = c0;
                            current_set = c0;
                            oldest = age[c0];
                        }
                    }

                } else {
                    SLONG cv = 4, ci = 5;
                    if (is_shadow) {
                        cv = 5; // shadow squares require more verts and indicies as quads are drawn as two separate tri's
                        ci = 8;
                    }
                    // do we have to bin (draw) a prim because it is too full?
                    if (vert_count[current_set] >= MAX_VERTS_FOR_STRIPS - cv || index_count[current_set] >= MAX_INDICES_FOR_STRIPS - ci)
                        bin_set = current_set;
                }
                age[current_set] = 0;

                ASSERT(current_set >= 0 && current_set < IPRIM_COUNT);

                // Draw this prim because its buffer is full, or its buffer is required for new texture page.
                if (bin_set >= 0) {
                    if (vert_count[bin_set]) {
                        draw_i_prim(group[bin_set].page, p_verts[bin_set], &m_indicies[bin_set][0], &vert_count[bin_set], &index_count[bin_set], &mm_draw_floor);

                        ASSERT(bin_set == current_set);

                        group[current_set].page = tex_handle;
                    }
                }

                // build the floor quad
                pv = &p_verts[current_set][vert_count[current_set]];
                p_indicies = &m_indicies[current_set][index_count[current_set]];

                ASSERT(vert_count[current_set] < MAX_VERTS_FOR_STRIPS);
                ASSERT(index_count[current_set] < MAX_INDICES_FOR_STRIPS);

                if ((p1->Flags & PAP_FLAG_SINK_SQUARE) && warehouse == 0) {
                    dy = -KERB_HEIGHT;
                } else {
                    dy = 0.0f;
                }

                //   0   1        0    1          1
                //
                //   3   2        3          3   2

                pv->x = x * 256.0F;
                pv->z = z * 256.0F;
                pv->color = p1->Colour;
                pv->specular = 0xff000000;
                SET_MM_INDEX(*pv, 0);
                pv++;

                pv->x = (x + 1) * 256.0F;
                pv->z = z * 256.0F;
                pv->color = (p1 + 1)->Colour;
                pv->specular = 0xff000000;
                SET_MM_INDEX(*pv, 0);
                pv++;

                pv->x = (x + 1) * 256.0F;
                pv->z = (z + 1) * 256.0F;
                pv->color = (p2 + 1)->Colour;
                pv->specular = 0xff000000;
                SET_MM_INDEX(*pv, 0);
                pv++;

                pv->x = x * 256.0F;
                pv->z = (z + 1) * 256.0F;
                pv->color = p2->Colour;
                pv->specular = 0xff000000;
                SET_MM_INDEX(*pv, 0);

                pv -= 3;

                TEXTURE_get_minitexturebits_uvs(
                    p1->Texture,
                    &page2,
                    &pv[0].tu,
                    &pv[0].tv,
                    &pv[1].tu,
                    &pv[1].tv,
                    &pv[3].tu,
                    &pv[3].tv,
                    &pv[2].tu,
                    &pv[2].tv);

                pv[0].tu = pv[0].tu * pp->m_UScale + pp->m_UOffset;
                pv[1].tu = pv[1].tu * pp->m_UScale + pp->m_UOffset;
                pv[2].tu = pv[2].tu * pp->m_UScale + pp->m_UOffset;
                pv[3].tu = pv[3].tu * pp->m_UScale + pp->m_UOffset;

                pv[0].tv = pv[0].tv * pp->m_VScale + pp->m_VOffset;
                pv[1].tv = pv[1].tv * pp->m_VScale + pp->m_VOffset;
                pv[2].tv = pv[2].tv * pp->m_VScale + pp->m_VOffset;
                pv[3].tv = pv[3].tv * pp->m_VScale + pp->m_VOffset;

                if ((p1->Flags & (PAP_FLAG_ROOF_EXISTS)) && warehouse == 0) {
                    y = MAVHEIGHT(x, z) << 6;

                    pv[0].y = y;
                    pv[1].y = y;
                    pv[2].y = y;
                    pv[3].y = y;
                } else {

                    pv[0].y = p1->Alt + dy;
                    pv[1].y = (p1 + 1)->Alt + dy;
                    pv[2].y = (p2 + 1)->Alt + dy;
                    pv[3].y = (p2)->Alt + dy;
                }

                {
                    SLONG i;

                    float dx;
                    float dy;
                    float dz;

                    float dprod;

                    dx = (pv[0].x + pv[2].x) * 0.5F - AENG_cam_x;
                    dy = (pv[0].y + pv[2].y) * 0.5F - AENG_cam_y;
                    dz = (pv[0].z + pv[2].z) * 0.5F - AENG_cam_z;

                    float dist;

                    float adx;
                    float ady;
                    float adz;

                    adx = fabsf(dx);
                    ady = fabsf(dy);
                    adz = fabsf(dz);

                    dist = 0;
                    dist += adx;
                    dist += ady;
                    dist += adz;

                    if (dist > 512.0F) {
                        // No need to zclip — tile is far from camera.
                    } else {
                        ULONG zclip = 0;
                        float along[4];

                        for (i = 0; i < 4; i++) {
                            dx = pv[i].x - AENG_cam_x;
                            dy = pv[i].y - AENG_cam_y;
                            dz = pv[i].z - AENG_cam_z;

                            dprod = dx * AENG_cam_matrix[6] + dy * AENG_cam_matrix[7] + dz * AENG_cam_matrix[8];

                            if (dprod < 8.0F) {
                                // Too close to camera — zclip.
                                along[i] = 8.0F - dprod;

                                zclip |= 1 << i;
                            }
                        }

                        if (zclip) {
                            if (zclip == 0xf) {
                                // Reject whole quad.
                                goto abandon_quad;
                            }

                            for (i = 0; i < 4; i++) {
                                if (zclip & (1 << i)) {
                                    pv[i].x += along[i] * AENG_cam_matrix[6];
                                    pv[i].y += along[i] * AENG_cam_matrix[7];
                                    pv[i].z += along[i] * AENG_cam_matrix[8];
                                }
                            }
                        }
                    }
                }

                pv += 4;

                // shadows
                if (is_shadow) {
                    // For shadows we need to duplicate the diagonal verts.
                    // Shadow quads are drawn as two separate triangles, not a strip.

                    pv -= 4;
                    pv[4] = pv[3];

                    // to be compatible with shadow.h we have to rotate quad by 180 degrees
                    //     3          2   4
                    //
                    // 1   0          1

                    switch (is_shadow) {
                    case 0:
                        ASSERT(0); // We shouldn't be doing any of this in this case.
                        break;

                    case 1:
                        HALF_COL(pv[0].color);
                        HALF_COL(pv[3].color);

                        break;

                    case 2:
                    case 6:
                        HALF_COL(pv[4].color);
                        HALF_COL(pv[3].color);
                        HALF_COL(pv[0].color);

                        break;

                    case 3:
                        HALF_COL(pv[4].color);
                        HALF_COL(pv[3].color);

                        break;

                    case 4:
                        HALF_COL(pv[2].color);
                        HALF_COL(pv[3].color);
                        HALF_COL(pv[4].color);

                        break;

                    case 5:
                        HALF_COL(pv[2].color);
                        HALF_COL(pv[0].color);
                        HALF_COL(pv[4].color);
                        HALF_COL(pv[3].color);
                        break;

                    case 7:
                        HALF_COL(pv[2].color);
                        HALF_COL(pv[4].color);
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }
                    pv += 5;

                    // make the indicies for the 5-vert shadow split quad
                    SLONG count = vert_count[current_set];

                    *p_indicies++ = count + 3;
                    *p_indicies++ = count + 0;
                    *p_indicies++ = count + 1;
                    *p_indicies++ = 0xffff;

                    *p_indicies++ = count + 2;
                    *p_indicies++ = count + 4;
                    *p_indicies++ = count + 1;
                    *p_indicies++ = 0xffff;

                    vert_count[current_set] += 5;
                    index_count[current_set] += 8; // 8 per quad: two separate tris

                } else {
                    // make the indicies for the normal 4-vert quad
                    SLONG count = vert_count[current_set];

                    *p_indicies++ = count + 0;
                    *p_indicies++ = count + 1;
                    *p_indicies++ = count + 3;
                    *p_indicies++ = count + 2;
                    *p_indicies++ = 0xffff;

                    vert_count[current_set] += 4;
                    index_count[current_set] += 5; // 5 per quad: terminates with -1
                }

            abandon_quad:;
            }
        }

    // Draw any prims left with data in them.
    for (c0 = 0; c0 < IPRIM_COUNT; c0++) {
        if (vert_count[c0]) {
            draw_i_prim(group[c0].page, p_verts[c0], &m_indicies[c0][0], &vert_count[c0], &index_count[c0], &mm_draw_floor);
        }
    }

    if (kerb_countv) {
        draw_i_prim(POLY_Page[0].RS.GetTexture(), kerb_verts, kerb_indicies, &kerb_countv, &kerb_counti, &mm_draw_floor);
    }
    general_steam(0, 0, 0, 2); // draw it

    POLY_set_local_rotation_none();
    general_steam(0, 0, 0, 2); // draw it

    END_SCENE;
}

