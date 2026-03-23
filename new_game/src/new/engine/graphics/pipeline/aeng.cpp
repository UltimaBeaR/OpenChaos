// Chunk 1: globals init/fini, cloud system, world lines, view frustum gamut, camera, lighting cache.
// aeng.cpp = "Another Engine" — the main 3D scene renderer.
// All Direct3D rendering will be replaced in Stage 7; for now this is a 1:1 port.

#include "engine/graphics/pipeline/aeng_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
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
