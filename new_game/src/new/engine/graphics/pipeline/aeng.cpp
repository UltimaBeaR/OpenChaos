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
