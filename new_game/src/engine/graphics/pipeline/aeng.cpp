// Chunk 1: globals init/fini, cloud system, world lines, view frustum gamut, camera, lighting cache.
// aeng.cpp = "Another Engine" — the main 3D scene renderer.
// All Direct3D rendering will be replaced in Stage 7; for now this is a 1:1 port.

#include "engine/graphics/pipeline/aeng_globals.h"
#include "engine/console/message.h"  // MSG_draw
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/pipeline/polypoint.h"
#include "engine/graphics/geometry/mesh.h"
#include "engine/graphics/lighting/ngamut.h"
#include "engine/graphics/lighting/ngamut_globals.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "engine/graphics/geometry/sky.h"
#include "engine/compression/compression.h"
#include "assets/formats/tga.h"
#include "assets/texture.h"
#include "assets/texture_globals.h"
#include "engine/core/matrix.h"

#include "engine/core/fmatrix.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "game/game_types.h"
#include "buildings/prim_types.h"    // PrimObject, PrimFace3/4, FACE_FLAG_*, etc.
#include "buildings/building_types.h" // STOREY_TYPE_*, FACET_FLAG_*, FBuilding, etc.
#include "navigation/inside2.h"
#include "navigation/inside2_globals.h"
#include "map/pap.h"
#include "map/pap_globals.h"
#include "ai/mav.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "map/level_pools.h"

#include "engine/platform/uc_common.h"
#include "config.h"                        // OC_FOV_MULTIPLIER
#include "engine/graphics/aspect_clamp.h"  // FOV_MIN_ASPECT / FOV_CAP_ASPECT
#include <math.h>
#include <stdio.h>

// Real-framebuffer dimensions — defined in the graphics backend, used here
// to compute the pillarbox regions when the horizontal FOV is capped.
extern SLONG ScreenWidth;
extern SLONG ScreenHeight;

#include "engine/graphics/geometry/figure.h"
#include "engine/graphics/geometry/figure_globals.h"  // kludge_shrink
#include "engine/graphics/geometry/shape.h"
#include "engine/graphics/lighting/smap.h"
#include "engine/graphics/lighting/smap_globals.h"
#include "engine/audio/sound.h"   // WORLD_TYPE_SNOW
#include "engine/audio/sound_globals.h" // world_type
#include "assets/formats/anim_globals.h"  // estate
#include "effects/weather/drip.h"
#include "effects/weather/drip_globals.h"
#include "effects/environment/fire.h"
#include "effects/environment/fire_globals.h"
#include "effects/combat/spark.h"
#include "effects/combat/spark_globals.h"
#include "effects/combat/glitter.h"
#include "effects/combat/glitter_globals.h"
#include "world_objects/dirt.h"
#include "world_objects/dirt_globals.h"
#include "effects/combat/pow.h"
#include "things/items/hook.h"
// PRIM_OBJ_CAN, PRIM_OBJ_HOOK, PRIM_OBJ_ITEM_AMMO_SHOTGUN are in world/environment/prim_types.h (included above)
#include "things/items/balloon_globals.h"
#include "engine/core/timer.h"
#include "engine/io/env.h"

// Additional includes for chunks 4b and 5a
#include "engine/graphics/text/font2d.h"

// Additional includes for chunk 5b
#include "navigation/inside2.h"
#include "map/sewers.h"
#include "missions/eway.h"
#include "things/core/thing_globals.h"
#include "map/supermap_globals.h"

// Additional includes for AENG_draw_city() (chunk 4b)
#include "world_objects/puddle.h"
#include "world_objects/puddle_globals.h"
#include "effects/weather/mist.h"
#include "effects/weather/mist_globals.h"
#include "engine/graphics/postprocess/wibble.h"
#include "engine/graphics/postprocess/bloom.h"
#include "engine/graphics/postprocess/bloom_globals.h"
#include "engine/graphics/geometry/cone.h"
#include "engine/graphics/geometry/cone_globals.h"
#include "world_objects/tripwire.h"
#include "world_objects/tripwire_globals.h"
#include "engine/physics/sm.h"
#include "engine/physics/sm_globals.h"
#include "engine/effects/psystem.h"
#include "engine/effects/psystem_globals.h"
#include "effects/environment/ribbon.h"
#include "effects/environment/ribbon_globals.h"
#include "effects/combat/pyro.h"
#include "effects/combat/pyro_globals.h"
#include "map/ob.h"
#include "map/ob_globals.h"
#include "map/supermap.h"
#include "engine/graphics/geometry/facet.h"
#include "engine/graphics/geometry/facet_globals.h"
#include "engine/graphics/geometry/farfacet.h"
#include "things/core/interact.h"
#include "things/animals/bat.h"
#include "things/characters/person.h"
#include "things/items/special.h"
#include "things/vehicles/vehicle.h"
#include "things/vehicles/chopper.h"
#include "things/items/grenade.h"
#include "ai/pcom.h"
#include "effects/combat/glow.h"
#include "effects/environment/tracks.h"
#include "engine/graphics/geometry/oval.h"
#include "game/input_actions_globals.h"
#include "game/game_tick_globals.h"
#include "things/characters/anim_ids.h"
#include "assets/formats/anim_tmap.h"
#include "things/core/statedef.h"

// uc_orig: POLY_set_local_rotation_none (fallen/DDEngine/Source/aeng.cpp)
// Pre-release: was an empty macro `#define POLY_set_local_rotation_none() {}`.
// This caused MESH_draw_poly calls (cans, brass) inside AENG_draw_dirt to leave the D3D
// world transform set to the object's position, so subsequent DrawIndexedPrimitive for
// leaves used the wrong transform — making leaves flicker in groups depending on camera angle.
// Fix: use the real function from poly.cpp which resets D3D world transform to camera-only.

// uc_orig: AENG_MAX_BBOXES (fallen/DDEngine/Source/aeng.cpp)
#define AENG_MAX_BBOXES 8

// uc_orig: AENG_BBOX_PUSH_IN (fallen/DDEngine/Source/aeng.cpp)
#define AENG_BBOX_PUSH_IN 16

// uc_orig: AENG_BBOX_PUSH_OUT (fallen/DDEngine/Source/aeng.cpp)
#define AENG_BBOX_PUSH_OUT 4

// uc_orig: ALT_SHIFT (fallen/DDEngine/Source/aeng.cpp)
#define ALT_SHIFT (3)

// Forward declarations for private helpers defined later in the file.
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

// uc_orig: apply_cloud (fallen/DDEngine/Source/aeng.cpp)
// Full per-vertex cloud shadow: samples cloud texture, then modulates colour.
// NOTE: this function always returns immediately (the body is unreachable dead code preserved 1:1).
void apply_cloud(SLONG x, SLONG y, SLONG z, ULONG* col)
{
    return;
}

// uc_orig: init_clouds (fallen/DDEngine/Source/aeng.cpp)
// Loads the 32x32 cloud shadow map from data\cloud.raw. Fills with zeros if missing.
static void init_clouds(void)
{
    MFFileHandle handle;
    handle = FileOpen("data/cloud.raw");
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

    if ((mapping_state == -1) || (mapping_state != (int)ge_supports_dest_inv_src_color())) {
        mapping_state = ge_supports_dest_inv_src_color();

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

// uc_orig: AENG_movie_init (fallen/DDEngine/Source/aeng.cpp)
// Loads the pre-rendered movie file (movie\bond.mmv) into AENG_movie_data.
void AENG_movie_init()
{
    SLONG bytes_read;

    FILE* handle;

    handle = MF_Fopen("movie/bond.mmv", "rb");

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
    SKY_init("stars/poo");
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
// Flag: true when inside AENG_draw (after POLY_frame_init). Geometry added
// outside the render pass gets cleared by POLY_frame_init and causes VB pool
// corruption (shadow artifacts). Gate AENG_world_line on this flag.
static bool s_in_render_pass = false;
void AENG_set_render_pass(bool active) { s_in_render_pass = active; }

void AENG_world_line(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front)
{
    // Drop geometry added outside render pass — it would be cleared by
    // POLY_frame_init and the freed VB slot causes shadow corruption.
    if (!s_in_render_pass) return;

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

// uc_orig: AENG_NUM_RAINDROPS (fallen/DDEngine/Source/aeng.cpp)
// Number of rain droplets drawn per frame.
#define AENG_NUM_RAINDROPS 128

// uc_orig: AENG_draw_rain (fallen/DDEngine/Source/aeng.cpp)
// Draws rain as 3-D world-space droplets placed in front of the camera.
// Droplets are lit using the cached night-lighting colour at their grid position.
void AENG_draw_rain()
{
    SLONG i;

    float x1;
    float y1;
    float z1;

    float matrix[9];

    float fade;
    SLONG bright;
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

        NIGHT_get_colour(nq->colour[dx + dz * PAP_BLOCKS], &col, &spec);

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
}

// uc_orig: AENG_draw_fire (fallen/DDEngine/Source/aeng.cpp)
// Iterates fire entries in the current gamut and kicks off rendering for each z-slice.
void AENG_draw_fire()
{
    SLONG z;

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

    float fpitch;
    float froll;
    float ubase;
    float vbase;

    float matrix[9];
    ULONG rubbish_colour;

    ULONG leaf_colour_choice_rgb[4] = {
        0x332d1d,
        0x243224,
        0x123320,
        0x332f07
    };

    for (i = 0; i < 4; i++) {
        leaf_colour_choice_rgb[i] = AENG_colour_mult(leaf_colour_choice_rgb[i], NIGHT_amb_colour);
    }

    ULONG leaf_colour;

    POLY_set_local_rotation_none();
    POLY_flush_local_rot();

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
            if ((i & 0xf) == 0 && !estate && world_type != WORLD_TYPE_SNOW) {
                // Rubbish (litter: paper, money, etc.) — drawn via POLY_add_quad
                // with its own texture page, NOT batched with leaves.

                fpitch = float(dd->pitch) * (PI / 1024.0F);
                froll = float(dd->roll) * (PI / 1024.0F);
                float fyaw = float(i);

                MATRIX_calc(matrix, fyaw, fpitch, froll);

                matrix[0] *= 24.0F;
                matrix[1] *= 24.0F;
                matrix[2] *= 24.0F;

                matrix[6] *= 24.0F;
                matrix[7] *= 24.0F;
                matrix[8] *= 24.0F;

                POLY_Point temp[4];
                POLY_Point* quad[4];

                temp[0].X = float(dd->x) + matrix[6] + matrix[0];
                temp[0].Y = float(dd->y + LEAF_UP) + matrix[7] + matrix[1];
                temp[0].Z = float(dd->z) + matrix[8] + matrix[2];

                temp[1].X = float(dd->x) + matrix[6] - matrix[0];
                temp[1].Y = float(dd->y + LEAF_UP) + matrix[7] - matrix[1];
                temp[1].Z = float(dd->z) + matrix[8] - matrix[2];

                temp[2].X = float(dd->x) - matrix[6] + matrix[0];
                temp[2].Y = float(dd->y + LEAF_UP) - matrix[7] + matrix[1];
                temp[2].Z = float(dd->z) - matrix[8] + matrix[2];

                temp[3].X = float(dd->x) - matrix[6] - matrix[0];
                temp[3].Y = float(dd->y + LEAF_UP) - matrix[7] - matrix[1];
                temp[3].Z = float(dd->z) - matrix[8] - matrix[2];

                rubbish_colour = NIGHT_amb_colour;
                ULONG colour_and = 0xffffffff;

                if (i & 32) {
                    ubase = 0.0F;
                    vbase = 0.0F;
                } else {
                    ubase = 0.5F;
                    vbase = 0.0F;
                }

                if (i == 64) {
                    ubase = 0.0F;
                    vbase = 0.5F;
                } else {
                    if (!(i & 32)) {
                        if (i & 64) {
                            colour_and = 0xffffff00;
                        }
                    }
                }

                SLONG j;
                SLONG all_valid = UC_TRUE;
                for (j = 0; j < 4; j++) {
                    POLY_transform(temp[j].X, temp[j].Y, temp[j].Z, &temp[j]);

                    if (!temp[j].MaybeValid()) {
                        all_valid = UC_FALSE;
                        break;
                    }

                    temp[j].u = ubase;
                    temp[j].v = vbase;
                    if (j & 1) { temp[j].u += 0.5F; }
                    if (j & 2) { temp[j].v += 0.5F; }

                    temp[j].colour = (rubbish_colour & colour_and) | 0xff000000;
                    temp[j].specular = 0xff000000;

                    quad[j] = &temp[j];
                }

                if (all_valid) {
                    POLY_add_quad(quad, POLY_PAGE_RUBBISH, UC_FALSE);
                } else {
                    DIRT_mark_as_offscreen(i);
                }
            } else {
                // Leaf or snowflake — deferred path via POLY_add_triangle
                // for correct alpha sort with additive effects (bloom, fire, smoke).

                POLY_Point leaf_pp[3];
                POLY_Point* tri[3];
                tri[0] = &leaf_pp[0];
                tri[1] = &leaf_pp[1];
                tri[2] = &leaf_pp[2];

                float leaf_size = LEAF_SIZE;
                float wx[3], wy[3], wz[3];

                if ((dd->pitch | dd->roll) == 0) {
                    // Common case — no rotation.
                    wx[0] = float(dd->x);
                    wy[0] = float(dd->y + LEAF_UP);
                    wz[0] = float(dd->z + leaf_size);

                    wx[1] = float(dd->x + leaf_size);
                    wy[1] = float(dd->y + LEAF_UP);
                    wz[1] = float(dd->z - leaf_size);

                    wx[2] = float(dd->x - leaf_size);
                    wy[2] = float(dd->y + LEAF_UP);
                    wz[2] = float(dd->z - leaf_size);
                } else {
                    fpitch = float(dd->pitch) * (PI / 1024.0F);
                    froll = float(dd->roll) * (PI / 1024.0F);

                    float cy = 1.0F, sy = 0.0F;
                    float sp = sinf(fpitch), cp = cosf(fpitch);
                    float sr = sinf(froll), cr = cosf(froll);

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

                    wx[0] = float(dd->x) + matrix[6];
                    wy[0] = float(dd->y + LEAF_UP) + matrix[7];
                    wz[0] = float(dd->z) + matrix[8];

                    wx[1] = float(dd->x) - matrix[6] + matrix[0];
                    wy[1] = float(dd->y + LEAF_UP) - matrix[7] + matrix[1];
                    wz[1] = float(dd->z) - matrix[8] + matrix[2];

                    wx[2] = float(dd->x) - matrix[6] - matrix[0];
                    wy[2] = float(dd->y + LEAF_UP) - matrix[7] - matrix[1];
                    wz[2] = float(dd->z) - matrix[8] - matrix[2];
                }

                SLONG leaf_page = (world_type == WORLD_TYPE_SNOW) ? POLY_PAGE_SNOWFLAKE : POLY_PAGE_LEAF;

                SLONG all_valid = UC_TRUE;
                float angle = float(i);
                for (SLONG j = 0; j < 3; j++) {
                    POLY_transform(wx[j], wy[j], wz[j], &leaf_pp[j]);

                    if (!leaf_pp[j].MaybeValid()) {
                        all_valid = UC_FALSE;
                        break;
                    }

                    if (world_type == WORLD_TYPE_SNOW) {
                        leaf_pp[j].u = SNOW_CENTRE_U + SNOW_RADIUS * sinf(angle);
                        leaf_pp[j].v = SNOW_CENTRE_V + SNOW_RADIUS * cosf(angle);
                    } else {
                        leaf_pp[j].u = LEAF_U(angle);
                        leaf_pp[j].v = LEAF_V(angle);
                    }
                    angle += 2.0F * PI / 3.0F;

                    if (world_type == WORLD_TYPE_SNOW) {
                        DWORD dwColour = ((i & 0x0f) << 2) + 0xc0;
                        dwColour *= 0x010101;
                        dwColour |= 0xff000000;
                        leaf_pp[j].colour = dwColour;
                    } else {
                        leaf_colour = leaf_colour_choice_rgb[i & 0x3];
                        leaf_pp[j].colour = (leaf_colour * (j + 3)) | 0xff000000;
                    }
                    leaf_pp[j].specular = 0xff000000;
                }

                if (all_valid) {
                    POLY_add_triangle(tri, leaf_page, UC_FALSE);
                } else {
                    DIRT_mark_as_offscreen(i);
                }
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
    pp[0].specular = 0xff000000;

    pp[1].X = right;
    pp[1].Y = top;
    pp[1].z = 0.0F + offset;
    pp[1].Z = 1.0F - offset;
    pp[1].u = 0.0F;
    pp[1].v = 0.0F;
    pp[1].colour = col;
    pp[1].specular = 0xff000000;

    pp[2].X = left;
    pp[2].Y = bottom;
    pp[2].z = 0.0F + offset;
    pp[2].Z = 1.0F - offset;
    pp[2].u = 0.0F;
    pp[2].v = 0.0F;
    pp[2].colour = col;
    pp[2].specular = 0xff000000;

    pp[3].X = right;
    pp[3].Y = bottom;
    pp[3].z = 0.0F + offset;
    pp[3].Z = 1.0F - offset;
    pp[3].u = 0.0F;
    pp[3].v = 0.0F;
    pp[3].colour = col;
    pp[3].specular = 0xff000000;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
}

// uc_orig: show_gamut_lo (fallen/DDEngine/Source/aeng.cpp)
// Debug stub: was intended to visualise lo-res gamut cells as coloured rectangles.
void show_gamut_lo(SLONG x, SLONG z)
{
    return;
}

// uc_orig: AENG_draw_people_messages (fallen/DDEngine/Source/aeng.cpp)
// Debug stub: iterates visible things and overlays state text. Returns immediately (disabled).
void AENG_draw_people_messages()
{
    return;
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

// AENG_draw_some_polys / AENG_guess_detail_levels — removed. These were a
// 1999-era GPU generation benchmark (draw 10k untextured tris → tris/sec
// thresholds pick Voodoo1..GeForce 2 equivalents → seed default detail
// levels). On any post-2001 hardware the benchmark always lands in the top
// tier and unconditionally enables all details, which is exactly what
// env.cpp's default .env block already provides. The benchmark also ran a
// pointless ge_lock_screen()/ge_unlock_screen() pair that triggered the
// very startup-hang pattern we're eliminating.

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

// uc_orig: MAX_FPM_ALPHA (fallen/DDEngine/Source/aeng.cpp)
// Alpha threshold used in first-person mode blending.
#define MAX_FPM_ALPHA 160

// uc_orig: AENG_draw_city (fallen/DDEngine/Source/aeng.cpp)
// Main outdoor city scene renderer. Draws sky, terrain, buildings, Things, effects, and HUD.
// This is the primary render path for exterior gameplay (not used for indoor/sewer sections).
void AENG_draw_city()
{

    // LOG_ENTER ( AENG_Draw_City )

    // TRACE ( "AengIn" );

    // DumpTracies();

    SLONG i;

    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;
    SLONG dist;

    SLONG nx;
    SLONG nz;

    SLONG page;
    SLONG square;

    float world_x;
    float world_y;
    float world_z;

    POLY_Point* pp;
    POLY_Point* ppl;

    PAP_Lo* pl;
    PAP_Hi* ph;

    THING_INDEX t_index;
    Thing* p_thing;

    POLY_Point* tri[3];
    POLY_Point* quad[4];

    NIGHT_Square* nq;

    SLONG red;
    SLONG green;
    SLONG blue;

    SLONG px;
    SLONG pz;

    SLONG mx;
    SLONG mz;

    SLONG worked_out_colour;
    ULONG colour;
    ULONG specular;
    static int outside = 1;
    static int sea_offset = 0;

    AENG_total_polys_drawn = 0;
    void draw_all_boxes(void);
    draw_all_boxes();

    extern SLONG tick_tock_unclipped;
    sea_offset += (tick_tock_unclipped);

    //
    // Work out which things are in view.
    //

    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
        for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
            t_index = PAP_2LO(x, z).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                if (p_thing->Flags & FLAGS_IN_BUILDING) {
                    //
                    // Dont draw things inside buildings when we are outdoors.
                    //
                } else {
                    switch (p_thing->Class) {
                    case CLASS_PERSON:

                        //
                        // We only have a rejection test for people now.
                        //

                        if (p_thing->Genus.Person->PlayerID && !p_thing->Genus.Person->Ware)
                            p_thing->Flags |= FLAGS_IN_VIEW;
                        else if (!p_thing->Genus.Person->Ware && FC_can_see_person(AENG_cur_fc_cam, p_thing)) {
                            if (POLY_sphere_visible(
                                    float(p_thing->WorldPos.X >> 8),
                                    float(p_thing->WorldPos.Y >> 8) + 0x80,
                                    float(p_thing->WorldPos.Z >> 8),
                                    256.0F / (AENG_DRAW_DIST * 256.0F))) {
                                p_thing->Flags |= FLAGS_IN_VIEW;
                            }
                        }

                        break;

                    default:
                        p_thing->Flags |= FLAGS_IN_VIEW;
                        break;
                    }
                }

                t_index = p_thing->Child;
            }

            NIGHT_square[NIGHT_cache[x][z]].flag &= ~NIGHT_SQUARE_FLAG_DELETEME;
        }
    }

    BreakTime("Worked out things in view");

    //
    // Points out of the ambient light.
    //

    if (Keys[KB_L] && ControlFlag && allow_debug_keys) {
        outside ^= 1;
    }

    if (INDOORS_INDEX) {
        POLY_frame_draw(UC_TRUE, UC_TRUE);
        POLY_frame_init(UC_TRUE, UC_TRUE);
        if (INDOORS_INDEX_NEXT)
            AENG_draw_inside_floor(INDOORS_INDEX_NEXT, INDOORS_ROOM_NEXT, 0);

        //		POLY_frame_draw(UC_TRUE,UC_TRUE);
        if (INDOORS_INDEX)
            AENG_draw_inside_floor(INDOORS_INDEX, INDOORS_ROOM, INDOORS_INDEX_FADE);
        POLY_frame_draw(UC_TRUE, UC_TRUE);
        POLY_frame_init(UC_TRUE, UC_TRUE);
        //		return;
    }

    //
    // Rotate all the points.   //draw_floor
    //

    colour = 0x00888888;
    specular = 0xff000000;

    if (!INDOORS_INDEX || outside)
        for (z = NGAMUT_point_zmin; z <= NGAMUT_point_zmax; z++) {
            for (x = NGAMUT_point_gamut[z].xmin; x <= NGAMUT_point_gamut[z].xmax; x++) {
                ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
                ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

                ph = &PAP_2HI(x, z);
                // show_gamut_hi(x,z);

                //
                // The upper point.
                //

                world_x = x * 256.0F;
                world_y = ph->Alt * float(1 << ALT_SHIFT);
                world_z = z * 256.0F;

                worked_out_colour = UC_FALSE;

                if (!(ph->Flags & PAP_FLAG_NOUPPER)) {
                    pp = &AENG_upper[x & 63][z & 63];

                    POLY_transform(world_x, world_y, world_z, pp);

                    if (pp->MaybeValid()) {
                        //
                        // Work out the colour of this point... what a palaver!
                        //

                        if (INDOORS_INDEX) {
                            /*
                                                                            NIGHT_get_colour_dim(
                                                                                    nq->colour[dx + dz * PAP_BLOCKS],
                                                                               &pp->colour,
                                                                               &pp->specular);
                            */
                            pp->colour = 0x80808080; // 202020;
                            pp->specular = 0x80000000;
                        } else {
                            px = x >> 2;
                            pz = z >> 2;

                            dx = x & 0x3;
                            dz = z & 0x3;

                            ASSERT(WITHIN(px, 0, PAP_SIZE_LO - 1));
                            ASSERT(WITHIN(pz, 0, PAP_SIZE_LO - 1));

                            square = NIGHT_cache[px][pz];

                            ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
                            ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);

                            nq = &NIGHT_square[square];

                            NIGHT_get_colour(
                                nq->colour[dx + dz * PAP_BLOCKS],
                                &pp->colour,
                                &pp->specular);
                        }

                        apply_cloud((SLONG)world_x, (SLONG)world_y, (SLONG)world_z, &pp->colour);

                        POLY_fadeout_point(pp);

                        colour = pp->colour;
                        specular = pp->specular;

                        worked_out_colour = UC_TRUE;
                    }
                }

                //
                // The lower point.
                //

                if (ph->Flags & PAP_FLAG_SINK_POINT || (MAV_SPARE(x, z) & MAV_SPARE_FLAG_WATER)) {

                    if (ph->Flags & PAP_FLAG_SINK_POINT) {
                        world_y -= KERB_HEIGHT;
                    } else {
                        world_y += (COS(((x << 8) + (sea_offset >> 1)) & 2047) + SIN(((z << 8) + (sea_offset >> 1) + 700) & 2047)) >> 13;
                    }

                    ppl = &AENG_lower[x & 63][z & 63];

                    POLY_transform(world_x, world_y, world_z, ppl);

                    if (ppl->MaybeValid()) {
                        if (worked_out_colour) {
                            //
                            // Use the colour of the upper point...
                            //

                            ppl->colour = colour;
                            ppl->specular = specular;
                        } else {
                            //
                            // Work out the colour of this point... what a palaver!
                            //

                            if (INDOORS_INDEX) {
                                /*
                                                                                        NIGHT_get_colour_dim(
                                                                                                nq->colour[dx + dz * PAP_BLOCKS],
                                                                                           &ppl->colour,
                                                                                           &ppl->specular);
                                */
                                ppl->colour = 0x202020;
                                ppl->specular = 0xff000000;

                            } else {
                                px = x >> 2;
                                pz = z >> 2;

                                dx = x & 0x3;
                                dz = z & 0x3;

                                ASSERT(WITHIN(px, 0, PAP_SIZE_LO - 1));
                                ASSERT(WITHIN(pz, 0, PAP_SIZE_LO - 1));

                                square = NIGHT_cache[px][pz];

                                ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
                                ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);

                                nq = &NIGHT_square[square];

                                NIGHT_get_colour(
                                    nq->colour[dx + dz * PAP_BLOCKS],
                                    &ppl->colour,
                                    &ppl->specular);
                            }

                            POLY_fadeout_point(ppl);
                        }
                        apply_cloud((SLONG)world_x, (SLONG)world_y, (SLONG)world_z, &ppl->colour);
                    }
                }
            }
        }
    BreakTime("Rotated points");

    //
    // No reflection and shadow stuff second time round during 3d mode.
    //

    if (AENG_detail_stars && !(NIGHT_flag & NIGHT_FLAG_DAYTIME)) {
        //
        // Draw the stars. SKY_draw_stars now batches the per-star pixels into
        // 1×1 quads through the TL pipeline — no backbuffer lock/unlock.
        //
        SKY_draw_stars(
            AENG_cam_x,
            AENG_cam_y,
            AENG_cam_z,
            AENG_DRAW_DIST * 256.0F);

        BreakTime("Drawn stars");
    }

    BreakTime("Done stars");

    //
    // Shadows.
    //

    // Max number of simultaneous detailed character shadows. Can't be bumped
    // on its own — the shadow bitmap is TEXTURE_SHADOW_SIZE (64) and each
    // shadow occupies a 32x32 quadrant, so only 4 slots fit. To raise this:
    // grow TEXTURE_SHADOW_SIZE (and the shadow texture in the renderer),
    // fix the offset_x/offset_y mapping (currently hardcoded 2x2 via i&1 / i&2),
    // update the ASSERTs on TEXTURE_SHADOW_SIZE / AENG_AA_BUF_SIZE, then bump
    // this value. Both shadow code paths use the same layout.
#define AENG_NUM_SHADOWS 4

    // Max distance (|dx|+|dz|, raw world units) from the camera at which a
    // character gets a detailed shadow. Original: 0x60000. Bumped for 1.0 so
    // characters stop popping into detailed shadows right in front of the cam.
    // Both shadow paths (detail_shadows and shadows_on) use this.
    // uc_orig: was inline 0x60000 in fallen/DDEngine/Source/aeng.cpp
#define AENG_SHADOW_MAX_DIST 0xC0000

    struct
    {
        Thing* p_person;
        SLONG dist;

    } shadow_person[AENG_NUM_SHADOWS];
    SLONG shadow_person_upto = 0;

    if (AENG_detail_shadows) {
        Thing* darci = FC_cam[AENG_cur_fc_cam].focus;

        // Reference point for shadow LOD selection: position of the actual
        // render camera, not Darci. In gameplay the camera follows Darci so
        // this is essentially the same, but in cutscenes (EWAY scripted cams
        // or PLAYCUTS cine packets) the camera is far from Darci — using her
        // position drops NPCs shown in close-up to blob shadows. FC_cam[].x/z
        // are in raw world units (same scale as Thing::WorldPos).
        SLONG ref_x = FC_cam[AENG_cur_fc_cam].x;
        SLONG ref_z = FC_cam[AENG_cur_fc_cam].z;

        //
        // How many people do we generate shadows for?
        //

        SLONG shadow_person_worst_dist = -UC_INFINITY;
        SLONG shadow_person_worst_person;

        for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
                t_index = PAP_2LO(x, z).MapWho;

                while (t_index) {
                    p_thing = TO_THING(t_index);

                    if (p_thing->Class == CLASS_PERSON && (p_thing->Flags & FLAGS_IN_VIEW)) {
                        //
                        // Distance from the reference point (camera position —
                        // see above).
                        //

                        dx = p_thing->WorldPos.X - ref_x;
                        dz = p_thing->WorldPos.Z - ref_z;

                        dist = abs(dx) + abs(dz);

                        if (dist < AENG_SHADOW_MAX_DIST) {
                            if (shadow_person_upto < AENG_NUM_SHADOWS) {
                                //
                                // Put this person in the shadow array.
                                //

                                shadow_person[shadow_person_upto].p_person = p_thing;
                                shadow_person[shadow_person_upto].dist = dist;

                                //
                                // Keep track of the furthest person away.
                                //

                                if (dist > shadow_person_worst_dist) {
                                    shadow_person_worst_dist = dist;
                                    shadow_person_worst_person = shadow_person_upto;
                                }

                                shadow_person_upto += 1;
                            } else {
                                if (dist < shadow_person_worst_dist) {
                                    //
                                    // Replace the worst person.
                                    //

                                    ASSERT(WITHIN(shadow_person_worst_person, 0, AENG_NUM_SHADOWS - 1));

                                    shadow_person[shadow_person_worst_person].p_person = p_thing;
                                    shadow_person[shadow_person_worst_person].dist = dist;

                                    //
                                    // Find the worst person
                                    //

                                    shadow_person_worst_dist = -UC_INFINITY;

                                    for (i = 0; i < AENG_NUM_SHADOWS; i++) {
                                        if (shadow_person[i].dist > shadow_person_worst_dist) {
                                            shadow_person_worst_dist = shadow_person[i].dist;
                                            shadow_person_worst_person = i;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    t_index = p_thing->Child;
                }
            }
        }

        //
        // Draw the people's shadow maps.
        //

        SLONG offset_x;
        SLONG offset_y;

        POLY_flush_local_rot();

        for (i = 0; i < shadow_person_upto; i++) {
            darci = shadow_person[i].p_person;

            memset(AENG_aa_buffer, 0, sizeof(AENG_aa_buffer));

            SMAP_person(
                darci,
                (UBYTE*)AENG_aa_buffer,
                AENG_AA_BUF_SIZE,
                AENG_AA_BUF_SIZE,
                147,
                -148,
                -147);

            //
            // Where do we put it in the shadow texture page? Hard code everything!
            //

            ASSERT(AENG_AA_BUF_SIZE == 32);
            ASSERT(TEXTURE_SHADOW_SIZE == 64);

            offset_x = (i & 1) << 5;
            offset_y = (i & 2) << 4;

            //
            // Plonk it into the shadow texture page.
            //

            if (TEXTURE_shadow_lock()) {
                SLONG x;
                SLONG y;
                UWORD* line;
                UBYTE* buf = (UBYTE*)AENG_aa_buffer;
                UWORD* mapping = GetShadowPixelMapping();

                for (y = 0; y < AENG_AA_BUF_SIZE; y++) {
                    line = &TEXTURE_shadow_bitmap[((y + offset_y) * TEXTURE_shadow_pitch >> 1) + offset_x];

                    for (x = AENG_AA_BUF_SIZE - 1; x >= 0; x--) {
                        *line++ = mapping[*buf++];
                    }
                }

                TEXTURE_shadow_unlock();
            }

            //
            // How we map floating points coordinates from 0 to 1 onto
            // where we plonked the shadow map in the texture page.
            //

            AENG_project_offset_u = float(offset_x) / float(TEXTURE_SHADOW_SIZE);
            AENG_project_offset_v = float(offset_y) / float(TEXTURE_SHADOW_SIZE);
            AENG_project_mul_u = float(AENG_AA_BUF_SIZE) / float(TEXTURE_SHADOW_SIZE);
            AENG_project_mul_v = float(AENG_AA_BUF_SIZE) / float(TEXTURE_SHADOW_SIZE);

            //
            // The position from which the shadow fades out.
            //

            AENG_project_fadeout_x = float(darci->WorldPos.X >> 8);
            AENG_project_fadeout_z = float(darci->WorldPos.Z >> 8);

            {
                //
                // Map this poly onto the mapsquares surrounding darci.
                //

                SLONG i;

                SLONG mx;
                SLONG mz;
                SLONG dx;
                SLONG dz;

                SLONG mx1;
                SLONG mz1;
                SLONG mx2;
                SLONG mz2;
                SLONG exit = UC_FALSE;

                SLONG mx_lo;
                SLONG mz_lo;

                MapElement* me[4];
                PAP_Hi* ph[4];

                SVector_F poly[4];
                SMAP_Link* projected;

                SLONG v_list;
                SLONG i_vect;

                DFacet* df;

                SLONG w_face;

                PrimFace4* p_f4;
                PrimPoint* pp;

                SLONG face_height;
                UBYTE face_order[4] = { 0, 1, 3, 2 };

                //
                // Colvects we have already done.
                //

#define AENG_MAX_DONE 8

                SLONG done[AENG_MAX_DONE];
                SLONG done_upto = 0;

                for (dx = -1; dx <= 1; dx++)
                    for (dz = -1; dz <= 1; dz++) {
                        mx = (darci->WorldPos.X >> 16) + dx;
                        mz = (darci->WorldPos.Z >> 16) + dz;

                        if (WITHIN(mx, 0, MAP_WIDTH - 2) && WITHIN(mz, 0, MAP_HEIGHT - 2)) {
                            me[0] = &MAP[MAP_INDEX(mx + 0, mz + 0)];
                            me[1] = &MAP[MAP_INDEX(mx + 1, mz + 0)];
                            me[2] = &MAP[MAP_INDEX(mx + 1, mz + 1)];
                            me[3] = &MAP[MAP_INDEX(mx + 0, mz + 1)];

                            ph[0] = &PAP_2HI(mx + 0, mz + 0);
                            ph[1] = &PAP_2HI(mx + 1, mz + 0);
                            ph[2] = &PAP_2HI(mx + 1, mz + 1);
                            ph[3] = &PAP_2HI(mx + 0, mz + 1);

                            if ((!(PAP_2HI(mx, mz).Flags & (PAP_FLAG_HIDDEN | PAP_FLAG_WATER))) || (PAP_2HI(mx, mz).Flags & (PAP_FLAG_ROOF_EXISTS))) {
                                poly[3].X = float(mx << 8);
                                poly[3].Z = float(mz << 8);

                                poly[0].X = poly[3].X + 256.0F;
                                poly[0].Z = poly[3].Z;

                                poly[1].X = poly[3].X + 256.0F;
                                poly[1].Z = poly[3].Z + 256.0F;

                                poly[2].X = poly[3].X;
                                poly[2].Z = poly[3].Z + 256.0F;

                                if (PAP_2HI(mx, mz).Flags & (PAP_FLAG_ROOF_EXISTS)) {
                                    poly[0].Y = poly[1].Y = poly[2].Y = poly[3].Y = MAVHEIGHT(mx, mz) << 6;
                                } else {
                                    poly[3].Y = float(ph[0]->Alt << ALT_SHIFT);
                                    poly[2].Y = float(ph[3]->Alt << ALT_SHIFT);
                                    poly[0].Y = float(ph[1]->Alt << ALT_SHIFT);
                                    poly[1].Y = float(ph[2]->Alt << ALT_SHIFT);
                                }

                                if (PAP_2HI(mx, mz).Flags & PAP_FLAG_SINK_SQUARE) {
                                    poly[0].Y -= KERB_HEIGHT;
                                    poly[1].Y -= KERB_HEIGHT;
                                    poly[2].Y -= KERB_HEIGHT;
                                    poly[3].Y -= KERB_HEIGHT;
                                }

                                if (ph[0]->Alt == ph[1]->Alt && ph[0]->Alt == ph[2]->Alt && ph[0]->Alt == ph[3]->Alt) {
                                    //
                                    // This quad is coplanar.
                                    //

                                    projected = SMAP_project_onto_poly(poly, 4);

                                    if (projected) {
                                        AENG_add_projected_fadeout_shadow_poly(projected);
                                    }
                                } else {
                                    //
                                    // Do each triangle separatly.
                                    //

                                    projected = SMAP_project_onto_poly(poly, 3);

                                    if (projected) {
                                        AENG_add_projected_fadeout_shadow_poly(projected);
                                    }

                                    //
                                    // The triangles are 0,1,2 and 0,2,3.
                                    //

                                    poly[1] = poly[0];

                                    projected = SMAP_project_onto_poly(poly + 1, 3);

                                    if (projected) {
                                        AENG_add_projected_fadeout_shadow_poly(projected);
                                    }
                                }
                            }
                        }
                    }

                mx1 = (darci->WorldPos.X >> 8) - 0x100 >> PAP_SHIFT_LO;
                mz1 = (darci->WorldPos.Z >> 8) - 0x100 >> PAP_SHIFT_LO;

                mx2 = (darci->WorldPos.X >> 8) + 0x100 >> PAP_SHIFT_LO;
                mz2 = (darci->WorldPos.Z >> 8) + 0x100 >> PAP_SHIFT_LO;

                SATURATE(mx1, 0, PAP_SIZE_LO - 1);
                SATURATE(mz1, 0, PAP_SIZE_LO - 1);
                SATURATE(mx2, 0, PAP_SIZE_LO - 1);
                SATURATE(mz2, 0, PAP_SIZE_LO - 1);

                for (mx_lo = mx1; mx_lo <= mx2; mx_lo++)
                    for (mz_lo = mz1; mz_lo <= mz2; mz_lo++) {
                        //
                        // Project onto nearby colvects...
                        //

                        v_list = PAP_2LO(mx_lo, mz_lo).ColVectHead;

                        if (v_list) {
                            exit = UC_FALSE;

                            while (!exit) {
                                i_vect = facet_links[v_list];

                                if (i_vect < 0) {
                                    i_vect = -i_vect;

                                    exit = UC_TRUE;
                                }

                                df = &dfacets[i_vect];

                                if (df->FacetType == STOREY_TYPE_NORMAL) {
                                    for (i = 0; i < done_upto; i++) {
                                        if (done[i] == i_vect) {
                                            //
                                            // Dont do this facet again
                                            //

                                            goto ignore_this_facet;
                                        }
                                    }

                                    float facet_height = float((df->BlockHeight << 4) * (df->Height >> 2));

                                    poly[0].X = float(df->x[1] << 8);
                                    poly[0].Y = float(df->Y[1]);
                                    poly[0].Z = float(df->z[1] << 8);

                                    poly[1].X = float(df->x[1] << 8);
                                    poly[1].Y = float(df->Y[1]) + facet_height;
                                    poly[1].Z = float(df->z[1] << 8);

                                    poly[2].X = float(df->x[0] << 8);
                                    poly[2].Y = float(df->Y[0]) + facet_height;
                                    poly[2].Z = float(df->z[0] << 8);

                                    poly[3].X = float(df->x[0] << 8);
                                    poly[3].Y = float(df->Y[0]);
                                    poly[3].Z = float(df->z[0] << 8);

                                    if (df->FHeight) {
                                        //
                                        // Foundations go down deep into the ground...
                                        //

                                        poly[0].Y -= 512.0F;
                                        poly[3].Y -= 512.0F;
                                    }

                                    projected = SMAP_project_onto_poly(poly, 4);

                                    if (projected) {
                                        AENG_add_projected_fadeout_shadow_poly(projected);
                                    }

                                    //
                                    // Remember We've already done this facet.
                                    //

                                    if (done_upto < AENG_MAX_DONE) {
                                        done[done_upto++] = i_vect;
                                    }
                                }

                            ignore_this_facet:;

                                v_list++;
                            }
                        }

                        // if (darci->OnFace) Always do this.
                        {
                            //
                            // Cast shadow on walkable faces.
                            //

                            w_face = PAP_2LO(mx_lo, mz_lo).Walkable;

                            while (w_face) {
                                if (w_face > 0) {
                                    p_f4 = &prim_faces4[w_face];
                                    face_height = prim_points[p_f4->Points[0]].Y;

                                    if (face_height > ((darci->WorldPos.Y + 0x11000) >> 8)) {
                                        //
                                        // This face is above Darci, so don't put her shadow
                                        // on it.
                                        //
                                    } else {
                                        for (i = 0; i < 4; i++) {
                                            pp = &prim_points[p_f4->Points[face_order[i]]];

                                            poly[i].X = float(pp->X);
                                            poly[i].Y = float(pp->Y);
                                            poly[i].Z = float(pp->Z);
                                        }

                                        projected = SMAP_project_onto_poly(poly, 4);

                                        if (projected) {
                                            AENG_add_projected_fadeout_shadow_poly(projected);
                                        }
                                    }

                                    w_face = p_f4->Col2;
                                } else {
                                    struct RoofFace4* rf;
                                    rf = &roof_faces4[-w_face];
                                    face_height = rf->Y;

                                    if (face_height > ((darci->WorldPos.Y + 0x11000) >> 8)) {
                                        //
                                        // This face is above Darci, so don't put her shadow
                                        // on it.
                                        //
                                    } else {
                                        SLONG mx, mz;
                                        mx = (rf->RX & 127) << 8;
                                        mz = (rf->RZ & 127) << 8;

                                        poly[0].X = (float)(mx);
                                        poly[0].Y = (float)(rf->Y);
                                        poly[0].Z = (float)(mz);

                                        poly[1].X = (float)(mx + 256);
                                        poly[1].Y = (float)(rf->Y + (rf->DY[0] << ROOF_SHIFT));
                                        poly[1].Z = (float)(mz);

                                        poly[2].X = (float)(mx + 256);
                                        poly[2].Y = (float)(rf->Y + (rf->DY[1] << ROOF_SHIFT));
                                        poly[2].Z = (float)(mz + 256);

                                        poly[3].X = (float)(mx);
                                        poly[3].Y = (float)(rf->Y + (rf->DY[2] << ROOF_SHIFT));
                                        poly[3].Z = (float)(mz + 256);

                                        projected = SMAP_project_onto_poly(poly, 4);

                                        if (projected) {
                                            AENG_add_projected_fadeout_shadow_poly(projected);
                                        }
                                    }

                                    w_face = rf->Next;
                                }
                            }
                        }
                    }
            }

            TEXTURE_shadow_update();
        }
    }

    BreakTime("Done shadows");

    // No reflections on DC.

    //
    // Where we remember the bounding boxes of reflections.
    //

    struct
    {
        SLONG x1;
        SLONG y1;
        SLONG x2;
        SLONG y2;
        SLONG water_box;

    } bbox[AENG_MAX_BBOXES];
    SLONG bbox_upto = 0;

    if (AENG_detail_moon_reflection && !(NIGHT_flag & NIGHT_FLAG_DAYTIME) && !(GAME_FLAGS & GF_NO_FLOOR)) {
        //
        // The moon upside down...
        //

        float moon_x1;
        float moon_y1;
        float moon_x2;
        float moon_y2;

        if (SKY_draw_moon_reflection(
                AENG_cam_x,
                AENG_cam_y,
                AENG_cam_z,
                AENG_DRAW_DIST * 256.0F,
                &moon_x1,
                &moon_y1,
                &moon_x2,
                &moon_y2)) {
            /*

            //
            // The moon is wibbled with polys now.
            //

            bbox[0].x1 = MAX((SLONG)moon_x1 - AENG_BBOX_PUSH_OUT, AENG_BBOX_PUSH_IN);
            bbox[0].y1 = MAX((SLONG)moon_y1, 0);
            bbox[0].x2 = MIN((SLONG)moon_x2 + AENG_BBOX_PUSH_OUT, DisplayWidth  - AENG_BBOX_PUSH_IN);
            bbox[0].y2 = MIN((SLONG)moon_y2, DisplayHeight);

            bbox[0].water_box = UC_FALSE;

            bbox_upto = 1;

            */
        }
    }

    //
    // Draw the reflections of people.
    //

    if (AENG_detail_people_reflection)
        for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
                t_index = PAP_2LO(x, z).MapWho;

                while (t_index) {
                    p_thing = TO_THING(t_index);

                    if (p_thing->Class == CLASS_PERSON && (p_thing->Flags & FLAGS_IN_VIEW)) {
                        //
                        // This is a person... Is she standing near a puddle or some water?
                        //

                        mx = p_thing->WorldPos.X >> 16;
                        mz = p_thing->WorldPos.Z >> 16;

                        if (!PAP_on_map_hi(mx, mz)) {
                            //
                            // Off the map.
                            //
                        } else {
                            ph = &PAP_2HI(mx, mz);

                            if (ph->Flags & (PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER)) {
                                //
                                // Not too far away?
                                //

                                dx = abs(p_thing->WorldPos.X - FC_cam[AENG_cur_fc_cam].x >> 8);
                                dz = abs(p_thing->WorldPos.Z - FC_cam[AENG_cur_fc_cam].z >> 8);

                                if (dx + dz < 0x600) {
                                    SLONG reflect_height;

                                    //
                                    // Puddles are always on the floor nowadays...
                                    //

                                    if (ph->Flags & PAP_FLAG_REFLECTIVE) {
                                        reflect_height = PAP_calc_height_at(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Z >> 8);
                                    } else {
                                        //
                                        // The height of the water is given by the lo-res mapsquare corresponding
                                        // to the hi-res mapsquare that Darci is standing on.
                                        //

                                        pl = &PAP_2LO(
                                            p_thing->WorldPos.X >> (8 + PAP_SHIFT_LO),
                                            p_thing->WorldPos.Z >> (8 + PAP_SHIFT_LO));

                                        reflect_height = pl->water << ALT_SHIFT;
                                    }

                                    //
                                    // Draw the reflection!
                                    //

                                    FIGURE_draw_reflection(p_thing, reflect_height);

                                    if (WITHIN(bbox_upto, 0, AENG_MAX_BBOXES - 1)) {
                                        //
                                        // Create a new bounding box
                                        //

                                        bbox[bbox_upto].x1 = MAX(FIGURE_reflect_x1 - AENG_BBOX_PUSH_OUT, AENG_BBOX_PUSH_IN);
                                        bbox[bbox_upto].y1 = MAX(FIGURE_reflect_y1, 0);
                                        bbox[bbox_upto].x2 = MIN(FIGURE_reflect_x2 + AENG_BBOX_PUSH_OUT, DisplayWidth - AENG_BBOX_PUSH_IN);
                                        bbox[bbox_upto].y2 = MIN(FIGURE_reflect_y2, DisplayHeight);

                                        bbox[bbox_upto].water_box = ph->Flags & PAP_FLAG_WATER;

                                        bbox_upto += 1;
                                    }
                                }
                            }
                        }
                    }

                    t_index = p_thing->Child;
                }
            }
        }

    /*

    //
    // Draw the reflections of the OBs.
    //

    if(DETAIL_LEVEL&DETAIL_REFLECTIONS)
    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++)
    {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++)
            {
                    for (oi = OB_find(x,z); oi->prim; oi += 1)
                    {
                            //
                            // On map?
                            //

                            mx = oi->x >> 8;
                            mz = oi->z >> 8;

                            if (WITHIN(mx, 0, PAP_SIZE_HI - 1) &&
                                    WITHIN(mz, 0, PAP_SIZE_HI - 1))
                            {
                                    //
                                    // On a reflective square?
                                    //

                                    if (PAP_2HI(mx,mz).Flags & (PAP_FLAG_WATER|PAP_FLAG_REFLECTIVE))
                                    {
                                            //
                                            // Not too far away?
                                            //

                                            dx = abs(oi->x - (FC_cam[AENG_cur_fc_cam].x >> 8));
                                            dz = abs(oi->z - (FC_cam[AENG_cur_fc_cam].z >> 8));

                                            if (dx + dz < 0x00)
                                            {
                                                    MESH_draw_reflection(
                                                            oi->prim,
                                                            oi->x,
                                                            oi->y,
                                                            oi->z,
                                                            oi->yaw,
                                                            NULL);
                                            }
                                    }
                            }
                    }
            }
    }

    */

    BreakTime("Drawn reflections");

    {
        //
        // Drips inside puddles only...
        //

        AENG_draw_drips(1);

        //
        // Draw the reflections and drips.  Clear the poly lists.
        //

        POLY_frame_draw(UC_FALSE, UC_FALSE);
        POLY_frame_init(UC_TRUE, UC_TRUE);
        BreakTime("Done first poly flush");
    }

    //
    // Draw the puddles.
    //

    if (AENG_detail_puddles && !(GAME_FLAGS & GF_NO_FLOOR)) {
        SLONG i;

        PUDDLE_Info* pi;

        float px1;
        float pz1;
        float px2;
        float pz2;

        float world_x;
        float world_y;
        float world_z;

        POLY_Point pp[4];
        POLY_Point* quad[4];

        quad[0] = &pp[0];
        quad[1] = &pp[1];
        quad[2] = &pp[2];
        quad[3] = &pp[3];

        // Iterate all map cells instead of gamut range — puddles are flat quads
        // visible at grazing angles well beyond the wall/floor gamut. Empty cells
        // are nearly free (just a linked-list head check), and puddles are sparse.
        for (z = 0; z < NGAMUT_SIZE; z++) {
            PUDDLE_get_start(z, 0, NGAMUT_SIZE - 1);

            while (pi = PUDDLE_get_next()) {
                px1 = float(pi->x1);
                pz1 = float(pi->z1);
                px2 = float(pi->x2);
                pz2 = float(pi->z2);

                world_y = float(pi->y);

                for (i = 0; i < 4; i++) {
                    world_x = (i & 0x1) ? px1 : px2;
                    world_z = (i & 0x2) ? pz1 : pz2;

                    POLY_transform(
                        world_x,
                        world_y,
                        world_z,
                        &pp[i]);

                    if (pp[i].MaybeValid()) {
                        pp[i].u = ((i & 0x1) ? float(pi->u1) : float(pi->u2)) * (1.0F / 256.0F);
                        pp[i].v = ((i & 0x2) ? float(pi->v1) : float(pi->v2)) * (1.0F / 256.0F);
                        pp[i].colour = 0xffffffff;

                        if (ControlFlag) {
                            pp[i].colour = 0x44ffffff;
                        }
                        if (ShiftFlag) {
                            pp[i].colour = 0x88ffffff;
                        }

                        pp[i].specular = 0xff000000;
                    } else {
                        goto ignore_this_puddle;
                    }
                }

                if (POLY_valid_quad(quad)) {
                    if ((GAME_FLAGS & GF_RAINING) && (rand() & 0x100)) {
                        float drip_along_x;
                        float drip_along_z;

                        //
                        // Choose somewhere in the puddle to put a drip.
                        //

                        drip_along_x = float(rand() & 0xff) * (1.0F / 256.0F);
                        drip_along_z = float(rand() & 0xff) * (1.0F / 256.0F);

                        world_x = px1 + (px2 - px1) * drip_along_x;
                        world_z = pz1 + (pz2 - pz1) * drip_along_z;

                        DRIP_create(
                            UWORD(world_x),
                            SWORD(world_y),
                            UWORD(world_z),
                            1);

                        if (rand() & 0x11) {
                            //
                            // Don't splash.
                            //
                        } else {
                            //
                            // Splash the puddle.
                            //

                            PUDDLE_splash(
                                SLONG(world_x),
                                SLONG(world_y),
                                SLONG(world_z));
                        }
                    }

                    if (pi->rotate_uvs) {
                        SWAP_FL(pp[0].u, pp[1].u);
                        SWAP_FL(pp[1].u, pp[3].u);
                        SWAP_FL(pp[3].u, pp[2].u);

                        SWAP_FL(pp[0].v, pp[1].v);
                        SWAP_FL(pp[1].v, pp[3].v);
                        SWAP_FL(pp[3].v, pp[2].v);
                    }

                    POLY_add_quad(quad, POLY_PAGE_PUDDLE, UC_FALSE);

                    if (pi->puddle_s1 | pi->puddle_s2) {
                        //
                        // Find the bounding box of this puddle quad on screen.
                        //

                        SLONG px;
                        SLONG py;
                        SLONG px1 = +UC_INFINITY;
                        SLONG py1 = +UC_INFINITY;
                        SLONG px2 = -UC_INFINITY;
                        SLONG py2 = -UC_INFINITY;

                        for (i = 0; i < 4; i++) {
                            px = SLONG(pp[i].X);
                            py = SLONG(pp[i].Y);

                            if (px < px1) {
                                px1 = px;
                            }
                            if (py < py1) {
                                py1 = py;
                            }
                            if (px > px2) {
                                px2 = px;
                            }
                            if (py > py2) {
                                py2 = py;
                            }
                        }

                        //
                        // Wibble the intersection of this bounding box with the bounding
                        // box of each reflection we have drawn.
                        //

                        SLONG ix1;
                        SLONG iy1;
                        SLONG ix2;
                        SLONG iy2;

                        for (i = 0; i < bbox_upto; i++) {
                            if (bbox[i].water_box) {
                                //
                                // This box always gets wibbled anyway.
                                //

                                continue;
                            }

                            ix1 = MAX(px1, bbox[i].x1);
                            iy1 = MAX(py1, bbox[i].y1);
                            ix2 = MIN(px2, bbox[i].x2);
                            iy2 = MIN(py2, bbox[i].y2);

                            if (ix1 < ix2 && iy1 < iy2) {
                                //
                                // Wibble away! Runs as a GPU post-process
                                // (copy-texture + fragment shader) — no lock.
                                //

                                WIBBLE_simple(
                                    ix1, iy1,
                                    ix2, iy2,
                                    pi->puddle_y1,
                                    pi->puddle_y2,
                                    pi->puddle_g1,
                                    pi->puddle_g2,
                                    pi->puddle_s1,
                                    pi->puddle_s2);
                            }
                        }
                    }
                }

            ignore_this_puddle:;
            }
        }
    }

    BreakTime("Drawn puddles");

    if (AENG_detail_people_reflection) {
        SLONG oldcolour[4];
        SLONG oldspecular[4];

        //
        // Draw all the floor-reflective squares
        //

        for (z = NGAMUT_zmin; z <= NGAMUT_zmax; z++) {
            for (x = NGAMUT_gamut[z].xmin; x <= NGAMUT_gamut[z].xmax; x++) {
                ph = &PAP_2HI(x, z);

                if (ph->Flags & PAP_FLAG_HIDDEN) {
                    continue;
                }

                if (!(ph->Flags & PAP_FLAG_REFLECTIVE)) {
                    continue;
                }

                //
                // The four points of the quad.
                //

                if (ph->Flags & PAP_FLAG_SINK_SQUARE) {
                    quad[0] = &AENG_lower[(x + 0) & 63][(z + 0) & 63];
                    quad[1] = &AENG_lower[(x + 1) & 63][(z + 0) & 63];
                    quad[2] = &AENG_lower[(x + 0) & 63][(z + 1) & 63];
                    quad[3] = &AENG_lower[(x + 1) & 63][(z + 1) & 63];
                } else {
                    quad[0] = &AENG_upper[(x + 0) & 63][(z + 0) & 63];
                    quad[1] = &AENG_upper[(x + 1) & 63][(z + 0) & 63];
                    quad[2] = &AENG_upper[(x + 0) & 63][(z + 1) & 63];
                    quad[3] = &AENG_upper[(x + 1) & 63][(z + 1) & 63];
                }

                if (POLY_valid_quad(quad)) {
                    //
                    // Darken the quad.
                    //

                    for (i = 0; i < 4; i++) {
                        oldcolour[i] = quad[i]->colour;
                        oldspecular[i] = quad[i]->specular;

                        quad[i]->colour >>= 1;
                        quad[i]->colour &= 0x007f7f7f;
                        quad[i]->specular = 0xff000000;
                    }

                    //
                    // Texture the quad.
                    //

                    if (ph->Flags & PAP_FLAG_ANIM_TMAP) {
                        struct AnimTmap* p_a;
                        SLONG cur;
                        p_a = &anim_tmaps[ph->Texture];
                        cur = p_a->Current;

                        quad[0]->u = float(p_a->UV[cur][0][0] & 0x3f) * (1.0F / 32.0F);
                        quad[0]->v = float(p_a->UV[cur][0][1]) * (1.0F / 32.0F);

                        quad[1]->u = float(p_a->UV[cur][1][0]) * (1.0F / 32.0F);
                        quad[1]->v = float(p_a->UV[cur][1][1]) * (1.0F / 32.0F);

                        quad[2]->u = float(p_a->UV[cur][2][0]) * (1.0F / 32.0F);
                        quad[2]->v = float(p_a->UV[cur][2][1]) * (1.0F / 32.0F);

                        quad[3]->u = float(p_a->UV[cur][3][0]) * (1.0F / 32.0F);
                        quad[3]->v = float(p_a->UV[cur][3][1]) * (1.0F / 32.0F);

                        page = p_a->UV[cur][0][0] & 0xc0;
                        page <<= 2;
                        page |= p_a->Page[cur];
                    } else {
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

                    POLY_add_quad(quad, page, UC_FALSE);

                    //
                    // Restore old colour info.
                    //

                    for (i = 0; i < 4; i++) {
                        quad[i]->colour = oldcolour[i];
                        quad[i]->specular = oldspecular[i];
                    }
                }
            }
        }
    }
    BreakTime("Drawn reflective squares");

    /*

    //
    // Draw the water in the city.
    //

    {
            SLONG dmx;
            SLONG dmz;

            SLONG dx;
            SLONG dz;

            SLONG px;
            SLONG py;
            SLONG pz;

            SLONG ix;
            SLONG iz;

            POLY_Point  water_pp[5][5];
            POLY_Point *quad[4];
            POLY_Point *pp;

            SLONG user_upto = 1;

            for (i = 0, pp = &water_pp[0][0]; i < 25; i++, pp++)
            {
                    pp->user = 0;
            }

            for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++)
            {
                    for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++)
                    {
                            pl = &PAP_2LO(x,z);

                            if (pl->water == PAP_LO_NO_WATER)
                            {
                                    //
                                    // No water in this square.
                                    //
                            }
                            else
                            {
                                    //
                                    // The height of water in this lo-res mapsquare.
                                    //

                                    py = pl->water << ALT_SHIFT;

                                    //
                                    // Look for water in the hi-res mapsquare enclosed by this
                                    // lo-res mapsqure.
                                    //

                                    for (dmx = 0; dmx < 4; dmx++)
                                    for (dmz = 0; dmz < 4; dmz++)
                                    {
                                            mx = (x << 2) + dmx;
                                            mz = (z << 2) + dmz;

                                            ph = &PAP_2HI(mx,mz);

                                            if (ph->Flags & PAP_FLAG_WATER)
                                            {
                                                    //
                                                    // Transform all the points.
                                                    //

                                                    i = 0;

                                                    for (i = 0; i < 4; i++)
                                                    {
                                                            dx = i &  1;
                                                            dz = i >> 1;

                                                            ix = dmx + dx;
                                                            iz = dmz + dz;

                                                            if (water_pp[ix][iz].user == user_upto)
                                                            {
                                                                    //
                                                                    // Already transformed.
                                                                    //
                                                            }
                                                            else
                                                            {
                                                                    //
                                                                    // Transform this point.
                                                                    //

                                                                    water_pp[ix][iz].user = user_upto;

                                                                    px = mx + dx << 8;
                                                                    pz = mz + dz << 8;

                                                                    POLY_transform(
                                                                            float(px),
                                                                            float(py),
                                                                            float(pz),
                                                                       &water_pp[ix][iz]);

                                                                    water_pp[ix][iz].colour   = 0x44608564;
                                                                    water_pp[ix][iz].specular = 0xff000000;

                                                                    POLY_fadeout_point(&water_pp[ix][iz]);

                                                                    //
                                                                    // This point's (u,v) coordinates are a function of its
                                                                    // position and the time.
                                                                    //

                                                                    {
                                                                            float angle_u = float(((((mx + dx) * 5 + (mz + dz) * 6) & 0x1f) + 0x1f) * GAME_TURN) * (1.0F / 1754.0F);
                                                                            float angle_v = float(((((mx + dx) * 4 + (mz + dz) * 7) & 0x1f) + 0x1f) * GAME_TURN) * (1.0F / 1816.0F);

                                                                            water_pp[ix][iz].u = px * (1.0F / 256.0F) + sin(angle_u) * 0.15F;
                                                                            water_pp[ix][iz].v = pz * (1.0F / 256.0F) + cos(angle_v) * 0.15F;
                                                                    }
                                                            }

                                                            if (!water_pp[ix][iz].MaybeValid())
                                                            {
                                                                    goto abandon_poly;
                                                            }

                                                            quad[i] = &water_pp[ix][iz];
                                                    }

                                                    if (POLY_valid_quad(quad))
                                                    {
                                                            POLY_add_quad(quad, POLY_PAGE_SEWATER, UC_FALSE);
                                                    }
                                            }

                                      abandon_poly:;
                                    }

                                    user_upto += 1;
                            }
                    }
            }
    }

    */

    // End of reflection stuff.

    //
    // The sky.
    //

    extern void SKY_draw_poly_sky_old(float world_camera_x, float world_camera_y, float world_camera_z, float world_camera_yaw, float max_dist, ULONG bot_colour, ULONG top_colour);

    if (AENG_detail_skyline)
        SKY_draw_poly_sky_old(AENG_cam_x, AENG_cam_y, AENG_cam_z, AENG_cam_yaw, AENG_DRAW_DIST * 256.0F, 0xffffff, 0xffffff);
    /*
            SKY_draw_poly_clouds(
                    AENG_cam_x,
                    AENG_cam_y,
                    AENG_cam_z,
                    AENG_DRAW_DIST * 256.0F);
      */
    if (!INDOORS_INDEX || outside)
        if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME)) {
            SKY_draw_poly_moon(
                AENG_cam_x,
                AENG_cam_y,
                AENG_cam_z,
                /*AENG_DRAW_DIST * 256.0F*/
                256.0f * 22.0f);
        }

    //
    // Draw the puddles and clear the poly lists.
    //

    {
        // C5 fix: on the first frame after loading, the back buffer contains residual
        // loading screen pixels that AENG_clear_viewport's Viewport::Clear does not
        // fully remove.  The exact porting error was not found (all D3D calls are 1:1
        // identical — verified by full review + direct D3D call substitution), but the
        // clear only takes effect after the main render scene's EndScene.
        // Re-clear here (color only, preserve depth) before the additive sky/moon pass.
        // Details: new_game_devlog/stage7_c5_investigation.md
        {
            static bool s_first_frame = true;
            if (s_first_frame) {
                s_first_frame = false;
                ge_clear(true, false);
            }
        }

        POLY_frame_draw_odd();
        POLY_frame_draw_puddles();

        POLY_frame_init(UC_TRUE, UC_TRUE);
    }
    BreakTime("Done second polygon flush");

    //
    // FAR FACETS!!!!!!!!!!!!!
    //

    FARFACET_draw(
        AENG_cam_x,
        AENG_cam_y,
        AENG_cam_z,
        AENG_cam_yaw,
        AENG_cam_pitch,
        AENG_cam_roll,
        // Draw dist changes dynamically, so do it to a fixed distance.
        // AENG_DRAW_DIST * 4,
        90.0F * 256.0F,

        AENG_LENS);

    // F10 debug mode (after bangunsnotgames): skip the rest of the level
    // geometry pass so the far-facet silhouette layer can be inspected
    // in isolation. See game.cpp for the toggle and stage12 devlog.
    {
        extern BOOL g_farfacet_debug;
        if (g_farfacet_debug) return;
    }

    // #if defined(NDEBUG) || defined(TARGET_DC)
    //	if (AENG_detail_skyline)
    //		AENG_draw_far_facets();
    // #endif

    //
    // Create all the squares.
    //

    //
    // draw floor draw_floor  //things to search for
    //


    {
        if (!INDOORS_INDEX || outside)
            for (z = NGAMUT_zmin; z <= NGAMUT_zmax; z++) {
                for (x = NGAMUT_gamut[z].xmin; x <= NGAMUT_gamut[z].xmax; x++) {
                    ASSERT(WITHIN(x, 0, PAP_SIZE_HI - 2));
                    ASSERT(WITHIN(z, 0, PAP_SIZE_HI - 2));

                    ph = &PAP_2HI(x, z);

                    if ((ph->Flags & (PAP_FLAG_HIDDEN | PAP_FLAG_ROOF_EXISTS)) == PAP_FLAG_HIDDEN) {
                        continue;
                    }

                    /*

                    if (ph->Flags & PAP_FLAG_WATER)
                    {
                            //
                            // We don't draw the ground because it is covered by water.
                            //
                    }
                    else

                    */
                    {
                        POLY_Point fake_roof[4];

                        //
                        // The four points of the quad.
                        //
                        if ((ph->Flags & (PAP_FLAG_ROOF_EXISTS))) {
                            SLONG c0, light_lookup;
                            float y;

                            y = MAVHEIGHT(x, z) << 6; //(PAP_hi[x][z].Height<<6);
                            // 0   1
                            //
                            // 2   3

                            POLY_transform((x) << 8, y, (z) << 8, &fake_roof[0]);
                            POLY_transform((x + 1) << 8, y, (z) << 8, &fake_roof[1]);
                            POLY_transform((x) << 8, y, (z + 1) << 8, &fake_roof[3]);
                            POLY_transform((x + 1) << 8, y, (z + 1) << 8, &fake_roof[2]);

                            light_lookup = hidden_roof_index[x][z];

                            ASSERT(light_lookup);

                            for (c0 = 0; c0 < 4; c0++) {
                                SLONG c0_bodge;
                                c0_bodge = index_lookup[c0];
                                pp = &fake_roof[c0];
                                //						pp->colour=0x808080;

                                if (pp->MaybeValid()) {
                                    NIGHT_get_colour(NIGHT_ROOF_WALKABLE_POINT(light_lookup, c0), &pp->colour, &pp->specular);

                                    apply_cloud((x + (c0_bodge & 1)) << 8, y, (z + ((c0_bodge & 2) >> 1)) << 8, &pp->colour);

                                    POLY_fadeout_point(pp);
                                }
                                //						pp->specular=0xff000000;
                                //						pp->colour=0xff000000;
                            }

                            quad[0] = &fake_roof[0];
                            quad[1] = &fake_roof[1];
                            quad[2] = &fake_roof[3];
                            quad[3] = &fake_roof[2];

                        } else {
                            if (GAME_FLAGS & GF_NO_FLOOR) {
                                //
                                // Don't draw the floor if there isn't any!
                                //

                                continue;
                            }

                            if (ph->Flags & PAP_FLAG_SINK_SQUARE) {
                                quad[0] = &AENG_lower[(x + 0) & 63][(z + 0) & 63];
                                quad[1] = &AENG_lower[(x + 1) & 63][(z + 0) & 63];
                                quad[2] = &AENG_lower[(x + 0) & 63][(z + 1) & 63];
                                quad[3] = &AENG_lower[(x + 1) & 63][(z + 1) & 63];
                            } else {

                                if ((MAV_SPARE(x, z) & MAV_SPARE_FLAG_WATER))
                                    quad[0] = &AENG_lower[(x + 0) & 63][(z + 0) & 63];
                                else
                                    quad[0] = &AENG_upper[(x + 0) & 63][(z + 0) & 63];

                                if ((MAV_SPARE(x + 1, z) & MAV_SPARE_FLAG_WATER))
                                    quad[1] = &AENG_lower[(x + 1) & 63][(z + 0) & 63];
                                else
                                    quad[1] = &AENG_upper[(x + 1) & 63][(z + 0) & 63];

                                if ((MAV_SPARE(x, z + 1) & MAV_SPARE_FLAG_WATER))
                                    quad[2] = &AENG_lower[(x + 0) & 63][(z + 1) & 63];
                                else
                                    quad[2] = &AENG_upper[(x + 0) & 63][(z + 1) & 63];

                                if ((MAV_SPARE(x + 1, z + 1) & MAV_SPARE_FLAG_WATER))
                                    quad[3] = &AENG_lower[(x + 1) & 63][(z + 1) & 63];
                                else
                                    quad[3] = &AENG_upper[(x + 1) & 63][(z + 1) & 63];
                            }
                        }

                        if (POLY_valid_quad(quad)) {
                            {
                                //
                                // Texture the quad.
                                //
                                /*
                                                                                if (ph->Flags & PAP_FLAG_ANIM_TMAP)
                                                                                {
                                                                                        struct	AnimTmap	*p_a;
                                                                                        SLONG	cur;
                                                                                        p_a=&anim_tmaps[ph->Texture];
                                                                                        cur=p_a->Current;

                                                                                        quad[0]->u = float(p_a->UV[cur][0][0] & 0x3f) * (1.0F / 32.0F);
                                                                                        quad[0]->v = float(p_a->UV[cur][0][1]       ) * (1.0F / 32.0F);

                                                                                        quad[1]->u = float(p_a->UV[cur][1][0]       ) * (1.0F / 32.0F);
                                                                                        quad[1]->v = float(p_a->UV[cur][1][1]       ) * (1.0F / 32.0F);

                                                                                        quad[2]->u = float(p_a->UV[cur][2][0]       ) * (1.0F / 32.0F);
                                                                                        quad[2]->v = float(p_a->UV[cur][2][1]       ) * (1.0F / 32.0F);

                                                                                        quad[3]->u = float(p_a->UV[cur][3][0]       ) * (1.0F / 32.0F);
                                                                                        quad[3]->v = float(p_a->UV[cur][3][1]       ) * (1.0F / 32.0F);

                                                                                        page   = p_a->UV[cur][0][0] & 0xc0;
                                                                                        page <<= 2;
                                                                                        page  |= p_a->Page[cur];
                                                                                }
                                                                                else
                                */
                                {
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

                                    // page = 86;
                                }

                                if (AENG_detail_shadows)
                                    if (page == 4 * 64 + 53) {
                                        SLONG dx, dz, dist;

                                        dx = abs((((SLONG)AENG_cam_x) >> 8) - (x));
                                        dz = abs((((SLONG)AENG_cam_z) >> 8) - (z));

                                        dist = QDIST2(dx, dz);

                                        if (dist < 15) {
                                            SLONG sx, sy, sz;
                                            void draw_steam(SLONG x, SLONG y, SLONG z, SLONG lod);
                                            switch ((ph->Texture >> 0xa) & 0x3) {
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

                                            draw_steam(sx, sy, sz, 10 + (15 - dist) * 3);
                                        }
                                    }

                                if (ph->Flags & (PAP_FLAG_SHADOW_1 | PAP_FLAG_SHADOW_2 | PAP_FLAG_SHADOW_3)) {
                                    POLY_Point ps[4];

                                    //
                                    // Create four darkened points.
                                    //

                                    ps[0] = *(quad[0]);
                                    ps[1] = *(quad[1]);
                                    ps[2] = *(quad[2]);
                                    ps[3] = *(quad[3]);

                                    //
                                    // Darken the points.
                                    //

                                    for (i = 0; i < 4; i++) {
                                        red = (ps[i].colour >> 16) & 0xff;
                                        green = (ps[i].colour >> 8) & 0xff;
                                        blue = (ps[i].colour >> 0) & 0xff;

                                        red -= 120;
                                        green -= 120;
                                        blue -= 120;

                                        if (red < 0) {
                                            red = 0;
                                        }
                                        if (green < 0) {
                                            green = 0;
                                        }
                                        if (blue < 0) {
                                            blue = 0;
                                        }

                                        ps[i].colour = (red << 16) | (green << 8) | (blue << 0) | (0xff000000);
                                    }

                                    ASSERT(PAP_FLAG_SHADOW_1 == 1);

                                    switch (ph->Flags & (PAP_FLAG_SHADOW_1 | PAP_FLAG_SHADOW_2 | PAP_FLAG_SHADOW_3)) {
                                    case 0:
                                        ASSERT(0); // We shouldn't be doing any of this in this case.
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

                                        // ps[2].colour += 0x00101010;

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

                                    {
                                        POLY_add_quad(quad, page, UC_FALSE);
                                    }
                                }
                            }
                        }
                    }

                    if (ph->Flags & (PAP_FLAG_SINK_SQUARE | PAP_FLAG_WATER)) {
                        //
                        // Do the curbs now.
                        //

                        static struct
                        {
                            SLONG dpx1;
                            SLONG dpz1;
                            SLONG dpx2;
                            SLONG dpz2;

                            SLONG dsx;
                            SLONG dsz;

                        } curb[4] = {
                            { 0, 0, 0, 1, -1, 0 },
                            { 0, 1, 1, 1, 0, +1 },
                            { 1, 1, 1, 0, +1, 0 },
                            { 1, 0, 0, 0, 0, -1 }
                        };

                        for (i = 0; i < 4; i++) {
                            nx = x + curb[i].dsx;
                            nz = z + curb[i].dsz;

                            if (PAP_on_map_hi(nx, nz)) {
                                if (PAP_2HI(nx, nz).Flags & (PAP_FLAG_SINK_SQUARE | PAP_FLAG_WATER)) {
                                    //
                                    // No need for a curb here.
                                    //
                                } else {
                                    quad[0] = &AENG_lower[(x + curb[i].dpx1) & 63][(z + curb[i].dpz1) & 63];
                                    quad[1] = &AENG_lower[(x + curb[i].dpx2) & 63][(z + curb[i].dpz2) & 63];
                                    quad[2] = &AENG_upper[(x + curb[i].dpx1) & 63][(z + curb[i].dpz1) & 63];
                                    quad[3] = &AENG_upper[(x + curb[i].dpx2) & 63][(z + curb[i].dpz2) & 63];

                                    if (POLY_valid_quad(quad)) {
                                        TEXTURE_get_minitexturebits_uvs(
                                            0,
                                            &page,
                                            &quad[0]->u,
                                            &quad[0]->v,
                                            &quad[1]->u,
                                            &quad[1]->v,
                                            &quad[2]->u,
                                            &quad[2]->v,
                                            &quad[3]->u,
                                            &quad[3]->v);

                                        //
                                        // Add the poly.
                                        //

                                        POLY_add_quad(quad, page, UC_TRUE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
    }
    BreakTime("Drawn floors");

    //	POLY_frame_draw(UC_FALSE,UC_FALSE);
    //	POLY_frame_init(UC_TRUE,UC_TRUE);
    //	BreakTime("Done another flush");

    //
    // Draw the objects and the things.
    //

    dfacets_drawn_this_gameturn = 0;
    {

        for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
                OB_Info* oi;
                NIGHT_Colour* col;

                //
                // The cached lighting for this low-res mapsquare.
                //

                ASSERT(WITHIN(x, 0, PAP_SIZE_LO - 1));
                ASSERT(WITHIN(z, 0, PAP_SIZE_LO - 1));

                // ASSERT(WITHIN(NIGHT_cache[x][z], 1, NIGHT_MAX_SQUARES - 1));

                col = NIGHT_square[NIGHT_cache[x][z]].colour;

                //
                // Skip over the mapsquares.
                //

                col += PAP_BLOCKS * PAP_BLOCKS;

                //
                // The objects on this mapsquare.
                //

                oi = OB_find(x, z);

                while (oi->prim) {
                    if (!(oi->flags & OB_FLAG_WAREHOUSE)) {
                        if (oi->prim == 133 || oi->prim == 235) {
                            //
                            // This prim slowly rotates...
                            //

                            oi->yaw = (GAME_TURN << 1) & 2047;
                        }

                        UBYTE fade = 0xff;

                        col = MESH_draw_poly(
                            oi->prim,
                            oi->x,
                            oi->y,
                            oi->z,
                            oi->yaw,
                            oi->pitch,
                            oi->roll,
                            col,
                            fade,
                            oi->crumple);
                        SHAPE_prim_shadow(oi);

                        if ((prim_objects[oi->prim].flag & PRIM_FLAG_GLARE)) {
                            if (oi->prim == 230) {
                                BLOOM_draw(oi->x, oi->y + 48, oi->z, 0, 0, 0, 0x808080, BLOOM_RAISE_LOS);
                            } else {
                                BLOOM_draw(oi->x, oi->y, oi->z, 0, 0, 0, 0x808080, 0);
                            }
                        } else if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME)) {

                            switch (oi->prim) {
                            case 2:
                                /*
                                BLOOM_draw(oi->x+270,oi->y+350,oi->z, 0,-255,0,0x7f6500,BLOOM_BEAM);
                                BLOOM_draw(oi->x-270,oi->y+350,oi->z, 0,-255,0,0x7f6500,BLOOM_BEAM);
                                BLOOM_draw(oi->x,oi->y+350,oi->z+270, 0,-255,0,0x7f6500,BLOOM_BEAM);
                                BLOOM_draw(oi->x,oi->y+350,oi->z-270, 0,-255,0,0x7f6500,BLOOM_BEAM);
                                */
                                break;
                            case 190:
                                BLOOM_draw(oi->x, oi->y, oi->z, 0, 0, 0, 0x808080, 0);
                                break;
                            }
                        }

                        //
                        // As good a place as any to put this!
                        //

                        if (prim_objects[oi->prim].flag & PRIM_FLAG_ITEM) {
                            OB_ob[oi->index].yaw += 1;
                        }
                    }

                    oi += 1;
                }
            }
        }

        BreakTime("Drawn prims");
        //		POLY_frame_draw(UC_FALSE,UC_FALSE);
        //		POLY_frame_init(UC_TRUE,UC_TRUE);
        //		BreakTime("Flushed prims");

        POLY_set_local_rotation_none();

        for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
                //
                // The cached lighting for this low-res mapsquare.
                //

                ASSERT(WITHIN(x, 0, PAP_SIZE_LO - 1));
                ASSERT(WITHIN(z, 0, PAP_SIZE_LO - 1));

                //
                // Look at the colvects on this square.
                //

                {
                    SLONG f_list;
                    SLONG facet;
                    SLONG build;
                    SLONG exit = UC_FALSE;

                    f_list = PAP_2LO(x, z).ColVectHead;

                    show_gamut_lo(x, z);

                    if (f_list) {
                        while (!exit) {
                            facet = facet_links[f_list];

                            /*
                                                                                    AENG_world_text(x<<10,(PAP_2HI(x<<2,z<<2).Alt<<3)+50,z<<10,128,128,128,1,"x %d z %d %d type %d",x,z,facet,dfacets[facet].FacetType);
                                                                                    if(x==13 && z==5)
                                                                                    AENG_world_line_infinite(dfacets[facet].X[0],dfacets[facet].Y[0]+10,dfacets[facet].Z[0],2,0xffffff,
                                                                                      dfacets[facet].X[1],dfacets[facet].Y[1]+10,dfacets[facet].Z[1],2,0xffffff,0);
                                                                                      */

                            ASSERT(facet);

                            if (facet < 0) {
                                //
                                // The last facet in the list for each square
                                // is negative.
                                //

                                facet = -facet;
                                exit = UC_TRUE;
                            }

                            if (dfacets[facet].Counter[AENG_cur_fc_cam] != SUPERMAP_counter[AENG_cur_fc_cam]) {
                                //								ASSERT(facet!=676);
                                //
                                // Mark this facet as drawn this gameturn already.
                                //

                                dfacets[facet].Counter[AENG_cur_fc_cam] = SUPERMAP_counter[AENG_cur_fc_cam];

                                if (dfacets[facet].FacetType == STOREY_TYPE_NORMAL)
                                    build = dfacets[facet].Building;
                                else
                                    build = 0;

                                //
                                // Don't draw inside facets
                                //
                                if (build && dbuildings[build].Type == BUILDING_TYPE_CRATE_IN || (dfacets[facet].FacetFlags & FACET_FLAG_INSIDE)) {
                                    //
                                    // Don't draw inside buildings outside.
                                    //
                                } else if (dfacets[facet].FacetType == STOREY_TYPE_DOOR) {
                                    //
                                    // Draw the warehouse ground around this facet but don't draw
                                    // the facet.
                                    //

                                    AENG_draw_box_around_recessed_door(&dfacets[facet], UC_FALSE);
                                } else {
                                    //
                                    // Draw the facet.
                                    //

                                    show_facet(facet);
                                    FACET_draw(facet, 0);

                                    //
                                    // Has this facet's building been processed this
                                    // gameturn yet?
                                    //

                                    switch (dfacets[facet].FacetType) {
                                    case STOREY_TYPE_NORMAL:

                                        if (build) {
                                            if (dbuildings[build].Counter[AENG_cur_fc_cam] != SUPERMAP_counter[AENG_cur_fc_cam]) {
                                                //
                                                // Draw all the walkable faces for this building.
                                                //

                                                FACET_draw_walkable(build);

                                                //
                                                // Mark the buiding as procesed this gameturn.
                                                //

                                                dbuildings[build].Counter[AENG_cur_fc_cam] = SUPERMAP_counter[AENG_cur_fc_cam];
                                            }
                                        }
                                        break;
                                    }
                                }
                            }

                            f_list++;
                        }
                    }
                }
            }
        }

        BreakFacets(dfacets_drawn_this_gameturn);
        BreakTime("Drawn facets");
        //		POLY_frame_draw(UC_FALSE,UC_FALSE);
        //		POLY_frame_init(UC_TRUE,UC_TRUE);
        //		BreakTime("Flushed facets");

        for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
                //
                // The cached lighting for this low-res mapsquare.
                //

                ASSERT(WITHIN(x, 0, PAP_SIZE_LO - 1));
                ASSERT(WITHIN(z, 0, PAP_SIZE_LO - 1));

                t_index = PAP_2LO(x, z).MapWho;

                while (t_index) {
                    p_thing = TO_THING(t_index);

                    if (p_thing->Flags & FLAGS_IN_VIEW) {
                        //						p_thing->Flags &=~FLAGS_IN_VIEW;
                        switch (p_thing->DrawType) {
                        case DT_NONE:
                            break;

                        case DT_BUILDING:
                            break;

                        case DT_PRIM:
                            break;
                        case DT_ANIM_PRIM:
                            extern void ANIM_obj_draw(Thing * p_thing, DrawTween * dt);
                            ANIM_obj_draw(p_thing, p_thing->Draw.Tweened);

                            if (p_thing->Class == CLASS_BAT && p_thing->Genus.Bat->type == BAT_TYPE_BANE) {
                                DRAWXTRA_final_glow(
                                    p_thing->WorldPos.X >> 8,
                                    p_thing->WorldPos.Y + 0x8000 >> 8,
                                    p_thing->WorldPos.Z >> 8,
                                    p_thing->Genus.Bat->glow >> 8);
                            }
                            break;

                        case DT_ROT_MULTI:

                            /*
                            if (ControlFlag)
                            if (p_thing->Genus.Person->PlayerID)
                            {
                                    //
                                    // Draw some wheels above Darci's head!
                                    //

                                    AENG_set_bike_wheel_rotation((GAME_TURN << 3) & 2047, PRIM_OBJ_BIKE_BWHEEL);

                                    MESH_draw_poly(
                                                    PRIM_OBJ_BIKE_BWHEEL,
                                                    p_thing->WorldPos.X          >> 8,
                                                    p_thing->WorldPos.Y + 0xa000 >> 8,
                                                    p_thing->WorldPos.Z          >> 8,
                                                    p_thing->Draw.Tweened->Angle,
                                                    0,0,
                                                    NULL,0);

                                    AENG_set_bike_wheel_rotation((GAME_TURN << 3) & 2047, PRIM_OBJ_VAN_WHEEL);

                                    MESH_draw_poly(
                                                    PRIM_OBJ_VAN_WHEEL,
                                                    p_thing->WorldPos.X           >> 8,
                                                    p_thing->WorldPos.Y + 0x10000 >> 8,
                                                    p_thing->WorldPos.Z           >> 8,
                                                    p_thing->Draw.Tweened->Angle,
                                                    0,0,
                                                    NULL,0);

                                    AENG_set_bike_wheel_rotation((GAME_TURN << 3) & 2047, PRIM_OBJ_CAR_WHEEL);

                                    MESH_draw_poly(
                                                    PRIM_OBJ_CAR_WHEEL,
                                                    p_thing->WorldPos.X           >> 8,
                                                    p_thing->WorldPos.Y + 0x16000 >> 8,
                                                    p_thing->WorldPos.Z           >> 8,
                                                    p_thing->Draw.Tweened->Angle,
                                                    0,0,
                                                    NULL,0);
                            }
                            */

                            {
                                ASSERT(p_thing->Class == CLASS_PERSON);

                                {
                                    if (p_thing->Genus.Person->PlayerID) {
                                        if (FirstPersonMode) {
                                            FirstPersonAlpha -= (TICK_RATIO * 16) >> TICK_SHIFT;
                                            if (FirstPersonAlpha < MAX_FPM_ALPHA)
                                                FirstPersonAlpha = MAX_FPM_ALPHA;
                                        } else {
                                            FirstPersonAlpha += (TICK_RATIO * 16) >> TICK_SHIFT;
                                            if (FirstPersonAlpha > 255)
                                                FirstPersonAlpha = 255;
                                        }

                                        // FIGURE_alpha = FirstPersonAlpha;
                                        FIGURE_draw(p_thing);
                                        // FIGURE_alpha = 255;
                                    } else {
                                        SLONG dx, dy, dz, dist;

                                        dx = abs((p_thing->WorldPos.X >> 8) - AENG_cam_x);
                                        dy = abs((p_thing->WorldPos.Y >> 8) - AENG_cam_y);
                                        dz = abs((p_thing->WorldPos.Z >> 8) - AENG_cam_z);

                                        dist = QDIST3(dx, dy, dz);

                                        if (dist < AENG_DRAW_PEOPLE_DIST) {

                                            FIGURE_draw(p_thing);
                                        }
                                    }
                                }

                                p_thing->Draw.Tweened->Drawn = SUPERMAP_counter[AENG_cur_fc_cam];

                                if (ControlFlag && allow_debug_keys) {
                                    AENG_world_text(
                                        (p_thing->WorldPos.X >> 8),
                                        (p_thing->WorldPos.Y >> 8) + 0x60,
                                        (p_thing->WorldPos.Z >> 8),
                                        200,
                                        180,
                                        50,
                                        UC_TRUE,
                                        PCOM_person_state_debug(p_thing));
                                }
                            }

                            if (p_thing->State == STATE_DEAD) {
                                if (p_thing->Genus.Person->Timer1 > 10) {
                                    if (p_thing->Genus.Person->PersonType == PERSON_MIB1 || p_thing->Genus.Person->PersonType == PERSON_MIB2 || p_thing->Genus.Person->PersonType == PERSON_MIB3) {
                                        //
                                        // Dead MIB self destruct!
                                        //
                                        DRAWXTRA_MIB_destruct(p_thing);
                                        /*
                                                                                                                                SLONG px;
                                                                                                                                SLONG py;
                                                                                                                                SLONG pz;

                                                                                                                                calc_sub_objects_position(
                                                                                                                                        p_thing,
                                                                                                                                        p_thing->Draw.Tweened->AnimTween,
                                                                                                                                        SUB_OBJECT_PELVIS,
                                                                                                                                   &px,
                                                                                                                                   &py,
                                                                                                                                   &pz);

                                                                                                                                px += p_thing->WorldPos.X >> 8;
                                                                                                                                py += p_thing->WorldPos.Y >> 8;
                                                                                                                                pz += p_thing->WorldPos.Z >> 8;

                                                                                                                                //
                                                                                                                                // Ripped from the DRAWXTRA_special!
                                                                                                                                //

                                                                                                                                // (So why didn't you put it there?!)

                                                                                                                                {
                                                                                                                                        SLONG c0;
                                                                                                                                        SLONG dx;
                                                                                                                                        SLONG dz;

                                                                                                                                  c0=3+(THING_NUMBER(p_thing)&7);
                                                                                                                                  c0=(((GAME_TURN*c0)+(THING_NUMBER(p_thing)*9))<<4)&2047;
                                                                                                                                  dx=SIN(c0)>>8;
                                                                                                                                  dz=COS(c0)>>8;
                                                                                                                                  BLOOM_draw(
                                                                                                                                        px, py+15, pz,
                                                                                                                                        dx,0,dz,0x9F2040,0);
                                                                                                                                }*/
                                    }
                                }
                            }

                            if (p_thing->Genus.Person->pcom_ai == PCOM_AI_BANE) {
                                DRAWXTRA_final_glow(
                                    p_thing->WorldPos.X >> 8,
                                    p_thing->WorldPos.Y + 0x8000 >> 8,
                                    p_thing->WorldPos.Z >> 8,
                                    -p_thing->Draw.Tweened->Tilt);
                            }

                            break;

                        case DT_EFFECT:
                            break;

                        case DT_MESH:

                        {
                            // Weapons & other powerups.
                            if (p_thing->Class == CLASS_SPECIAL)
                                DRAWXTRA_Special(p_thing);

                            MESH_draw_poly(
                                p_thing->Draw.Mesh->ObjectId,
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                p_thing->Draw.Mesh->Angle,
                                p_thing->Draw.Mesh->Tilt,
                                p_thing->Draw.Mesh->Roll,
                                NULL, 0xff, 0);
                        }

                        break;

                        case DT_VEHICLE:

                            if (p_thing->Class == CLASS_VEHICLE) {
                                if (p_thing->Genus.Vehicle->Driver) {
                                    TO_THING(p_thing->Genus.Vehicle->Driver)->Flags |= FLAGS_IN_VIEW;
                                }
                            }
                            //
                            // Set the tinted colour of this van.
                            //

                            {
                                ULONG car_colours[6] = {
                                    0xffffff00,
                                    0xffff00ff,
                                    0xff00ffff,
                                    0xffff0000,
                                    0xff00ff00,
                                    0xf00000ff
                                };

                                MESH_colour_and = car_colours[THING_NUMBER(p_thing) % 6];
                            }

                            extern void draw_car(Thing * p_car);

                            draw_car(p_thing);

                            if (ControlFlag && allow_debug_keys) {
                                //
                                // Draw a line towards where you have to be
                                // to get into the van.
                                //

                                SLONG dx = -SIN(p_thing->Genus.Vehicle->Draw.Angle);
                                SLONG dz = -COS(p_thing->Genus.Vehicle->Draw.Angle);

                                SLONG ix = p_thing->WorldPos.X >> 8;
                                SLONG iz = p_thing->WorldPos.Z >> 8;

                                ix += dx >> 9;
                                iz += dz >> 9;

                                ix -= dz >> 9;
                                iz += dx >> 9;

                                AENG_world_line(
                                    p_thing->WorldPos.X >> 8, p_thing->WorldPos.Y >> 8, p_thing->WorldPos.Z >> 8, 64, 0x00ffffff,
                                    ix, p_thing->WorldPos.Y >> 8, iz, 0, 0x0000ff00, UC_TRUE);
                            }

                            break;

                        case DT_CHOPPER:
                            CHOPPER_draw_chopper(p_thing);
                            break;

                        case DT_PYRO:
                            PYRO_draw_pyro(p_thing);
                            break;
                        case DT_ANIMAL_PRIM:

                            break;

                        case DT_TRACK:
                            if (!INDOORS_INDEX)
                                TRACKS_DrawTrack(p_thing);
                            break;

                        default:
                            ASSERT(0);
                            break;
                        }
                    }
                    t_index = p_thing->Child;
                }
            }
        }
    }

    BreakTime("Drawn things");

    //	POLY_frame_draw(UC_FALSE,UC_FALSE);
    //	POLY_frame_init(UC_TRUE,UC_TRUE);
    //	BreakTime("Flushed things");

    //
    // debug info for the car movement
    //
    // void	draw_car_tracks(void);
    //	if(!INDOORS_INDEX||outside)
    //		draw_car_tracks();

    //
    // Do oval shadows.
    //

    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
        for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
            t_index = PAP_2LO(x, z).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                if (p_thing->Flags & FLAGS_IN_VIEW) {
                    bool draw = true;

                    for (int ii = 0; ii < shadow_person_upto; ii++) {
                        if (shadow_person[ii].p_person == p_thing) {
                            draw = false;
                            break;
                        }
                    }

                    if (draw) {
                        switch (p_thing->Class) {
                        case CLASS_PERSON:

                        {

                            SLONG px;
                            SLONG py;
                            SLONG pz;

                            calc_sub_objects_position(
                                p_thing,
                                p_thing->Draw.Tweened->AnimTween,
                                SUB_OBJECT_PELVIS,
                                &px,
                                &py,
                                &pz);

                            px += p_thing->WorldPos.X >> 8;
                            py += p_thing->WorldPos.Y >> 8;
                            pz += p_thing->WorldPos.Z >> 8;

                            OVAL_add(
                                px,
                                py,
                                pz,
                                130);
                        }

                        break;

                        case CLASS_SPECIAL:

                            OVAL_add(
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                100);

                            break;

                        case CLASS_BARREL:

                            OVAL_add(
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                128);

                            break;

                        default:
                            break;
                        }
                    }
                }

                t_index = p_thing->Child;
            }
        }
    }

    // Grenades should be drawn here.
    DrawGrenades();

    //
    // The balloons that nobody is holding.
    //

    if (!INDOORS_INDEX || outside)
        AENG_draw_released_balloons();

    //
    // The POWs!
    //

    AENG_draw_pows();

    //
    // Draw the mist.
    //

    if (!INDOORS_INDEX || outside)
        if (AENG_detail_mist) {
            SLONG sx;
            SLONG sz;

            SLONG px;
            SLONG pz;
            SLONG detail;

            SLONG wx;
            SLONG wy;
            SLONG wz;

            // Internal gubbins.
            POLY_flush_local_rot();

            POLY_Point* pp;
            POLY_Point* quad[4];

#define MAX_MIST_DETAIL 32

            POLY_Point mist_pp[MAX_MIST_DETAIL][MAX_MIST_DETAIL];

            MIST_get_start();

            while (detail = MIST_get_detail()) {
                //
                // Only draw as much as we can!
                //

                if (detail > MAX_MIST_DETAIL) {
                    detail = MAX_MIST_DETAIL;
                }

                for (px = 0; px < detail; px++)
                    for (pz = 0; pz < detail; pz++) {
                        pp = &mist_pp[px][pz];

                        MIST_get_point(px, pz,
                            &wx,
                            &wy,
                            &wz);

                        POLY_transform(
                            float(wx),
                            float(wy),
                            float(wz),
                            pp);

                        if (pp->MaybeValid()) {
                            pp->colour = 0x88666666;
                            pp->specular = 0xff000000;

                            MIST_get_texture(
                                px,
                                pz,
                                &pp->u,
                                &pp->v);

                            POLY_fadeout_point(pp);
                        }
                    }

                for (sx = 0; sx < detail - 1; sx++)
                    for (sz = 0; sz < detail - 1; sz++) {
                        quad[0] = &mist_pp[sx + 0][sz + 0];
                        quad[1] = &mist_pp[sx + 1][sz + 0];
                        quad[2] = &mist_pp[sx + 0][sz + 1];
                        quad[3] = &mist_pp[sx + 1][sz + 1];

                        if (POLY_valid_quad(quad)) {
                            POLY_add_quad(quad, POLY_PAGE_FOG, UC_FALSE);
                        }
                    }
            }
        }

    //
    // Rain.
    //

    /*
    #ifdef _DEBUG	// about time we removed this kind of crap
            if (Keys[KB_R] && !ShiftFlag)
            {
                    Keys[KB_R] = 0;

                    GAME_FLAGS ^= GF_RAINING;
            }
    #endif
    */

    if (!INDOORS_INDEX || outside) {
        if (GAME_FLAGS & GF_RAINING) {
            if (AENG_detail_rain)
                AENG_draw_rain();
        }
    }

    //
    // Drawing the steam.
    //

    //	void draw_steam(SLONG x,SLONG y,SLONG z,SLONG lod);
    //	void draw_flames(SLONG x,SLONG y,SLONG z,SLONG lod);

    //
    // Pyrotechincs.
    //

    if (!INDOORS_INDEX || outside)
        AENG_draw_bangs();
    if (!INDOORS_INDEX || outside)
        AENG_draw_fire();
    if (!INDOORS_INDEX || outside)
        AENG_draw_sparks();
    //	ANEG_draw_messages();

    /*

          for (z = NGAMUT_zmin; z <= NGAMUT_zmax; z++)
          {
                  PUDDLE_Info *pi;
                          SLONG	sx,sy,sz;
                          SLONG	dx,dy,dz,dist,lod;
                          PUDDLE_get_start(z, NGAMUT_gamut[z].xmin, NGAMUT_gamut[z].xmax);

                          while(pi = PUDDLE_get_next())
                          {
                                  sx = (pi->x1+pi->x2)>>1;
                                  sz = (pi->z1+pi->z2)>>1;
                                  sy = pi->y;

                                  dx=abs( ((SLONG)AENG_cam_x>>0)-sx);
  //				dy=abs( ((SLONG)AENG_cam_y>>0)-SLONG(world_y))*4;
                                  dy=abs( ((SLONG)AENG_cam_y>>0)-sy);
                                  dz=abs( ((SLONG)AENG_cam_z>>0)-sz);

                                  dist=QDIST3(dx,dy,dz);

                                  lod = 90-(dist/(32*2));
                                  if(lod>0)
                                          draw_steam(sx,sy,sz,lod);
  //                    draw_flames(sx,sy,sz,lod);
                          }
                  }
  */

    //	if (Keys[KB_RBRACE] && !ShiftFlag) {Keys[KB_RBRACE] = 0; AENG_torch_on ^= UC_TRUE;}

    //
    // Draw a torch out of darci...
    //

    if (AENG_torch_on) {
        Thing* darci = NET_PERSON(0);

        float angle;

        float dx;
        float dy;
        float dz;

        SLONG x;
        SLONG y;
        SLONG z;

        calc_sub_objects_position(
            darci,
            darci->Draw.Tweened->AnimTween,
            0, // Torso?
            &x,
            &y,
            &z);

        x += darci->WorldPos.X >> 8;
        y += darci->WorldPos.Y >> 8;
        z += darci->WorldPos.Z >> 8;

        angle = float(darci->Draw.Tweened->Angle);
        angle *= 2.0F * PI / 2048.0F;

        float dyaw = ((float)sin(float(GAME_TURN) * 0.025F)) * 0.25F;
        float dpitch = ((float)cos(float(GAME_TURN) * 0.020F) - 0.5F) * 0.25F;

        float matrix[3];

        dyaw += angle;
        dyaw += PI;

        MATRIX_vector(
            matrix,
            dyaw,
            dpitch);

        dx = matrix[0];
        dy = matrix[1];
        dz = matrix[2];

        // experimental light bloom
        //		BLOOM_draw(x,y,z,dx*256,dy*256,dz*256,0x00666600);
        BLOOM_draw(x, y, z, dx * 256, dy * 256, dz * 256, 0x00ffffa0);

        CONE_create(
            float(x),
            float(y),
            float(z),
            dx,
            dy,
            dz,
            256.0F * 4.0F,
            256.0F,
            0x00666600,
            0x00000000,
            50);

        CONE_intersect_with_map();

        //		CONE_draw();
    }

    //
    // Draw the tripwires.
    //

    {
        SLONG map_x1;
        SLONG map_z1;

        SLONG map_x2;
        SLONG map_z2;

        TRIP_Info* ti;

        // Deal with internal bollocks.
        POLY_flush_local_rot();

        TRIP_get_start();

        while (ti = TRIP_get_next()) {
            //
            // Check whether this tripwire is on the map.
            //

            map_x1 = ti->x1 >> 8;
            map_z1 = ti->z1 >> 8;

            map_x2 = ti->x2 >> 8;
            map_z2 = ti->z2 >> 8;

            if ((WITHIN(map_z1, NGAMUT_zmin, NGAMUT_zmax) && WITHIN(map_x1, NGAMUT_gamut[map_z1].xmin, NGAMUT_gamut[map_z1].xmax)) || (WITHIN(map_z2, NGAMUT_zmin, NGAMUT_zmax) && WITHIN(map_x2, NGAMUT_gamut[map_z2].xmin, NGAMUT_gamut[map_z2].xmax))) {
                //
                // Draw the bugger.
                //

#define AENG_TRIPWIRE_WIDTH 0x3

                SHAPE_tripwire(
                    ti->x1,
                    ti->y,
                    ti->z1,
                    ti->x2,
                    ti->y,
                    ti->z2,
                    AENG_TRIPWIRE_WIDTH,
                    0x00661122,
                    ti->counter,
                    ti->along);
            }
        }
    }

    //
    // Draw the hook.
    //

    if (!INDOORS_INDEX || outside)
        AENG_draw_hook();

    //
    // Draw the sphere-matter.
    //

    if (!INDOORS_INDEX || outside) {
        SM_Info* si;

        SM_get_start();

        while (si = SM_get_next()) {
            SHAPE_sphere(
                si->x,
                si->y,
                si->z,
                si->radius,
                si->colour);
        }
    }

    //
    // Draw the drips... again!
    //

    if (!INDOORS_INDEX || outside)
        AENG_draw_drips(0);

    //
    // Draw the particle system
    //

    if (!INDOORS_INDEX || outside)
        PARTICLE_Draw();

    //
    // Draw the ribbon system
    //

    if (!INDOORS_INDEX || outside)
        RIBBON_draw();

    //
    // Draw the tyre tracks (changed -- now turned into things)
    //
    /*
            if (ControlFlag) {
                    TRACKS_Draw();
            }*/

    //
    // Draw the cloth.
    //

    //	if(!INDOORS_INDEX||outside)
    //	AENG_draw_cloth();

    //
    // Draw.
    //
    //
    // Draw the people messages.
    //

    AENG_draw_people_messages();

    //	MSG_add(" draw insides %d and %d \n",INDOORS_INDEX,INDOORS_INDEX_NEXT);

    /*

    POLY_frame_draw(UC_TRUE,UC_TRUE);
    if(INDOORS_INDEX_NEXT)
            AENG_draw_inside_floor(INDOORS_INDEX_NEXT,INDOORS_ROOM_NEXT,255); //downtairs non transparent

    POLY_frame_draw(UC_TRUE,UC_TRUE);
    if(INDOORS_INDEX)
    {
            AENG_draw_inside_floor(INDOORS_INDEX,INDOORS_ROOM,INDOORS_INDEX_FADE);

    }

    */

    BreakTime("Drawn other crap");

    //
    // The dirt.
    // Drawn BEFORE the final flush so that deferred polys from dirt (leaves, droplets,
    // mesh objects like cans/brass) participate in alpha sort together with additive
    // effects (bloom, fire, smoke). Fixes pre-release overdraw bug where leaves/debris
    // rendered over additive effects.
    //

    if (!INDOORS_INDEX || outside)
        if (AENG_detail_dirt)
            AENG_draw_dirt();

    POLY_frame_draw(UC_TRUE, UC_TRUE);

    BreakTime("Done final polygon flush");
    // Cope with some wacky internals.
    POLY_set_local_rotation_none();
    POLY_flush_local_rot();

    // Tell the pyros we've done a frame.
    Pyros_EndOfFrameMarker();

    //	ANEG_draw_messages();

    /*

    //
    // This really _is_ shite!
    //

    if (ShiftFlag)
    {
            static float focus = 0.95F;

            if (Keys[KB_P7]) {focus += 0.001F;}
            if (Keys[KB_P4]) {focus -= 0.001F;}

            POLY_frame_draw_focused(focus);
    }

    */

    //	TRACE("Total polys = %d\n", AENG_total_polys_drawn);

    // LOG_EXIT ( AENG_Draw_City )

    // TRACE ( "AengOut" );
}

// uc_orig: AENG_draw_warehouse (fallen/DDEngine/Source/aeng.cpp)
// Renders interior warehouse/building scenes.
// Used when the player is inside a warehouse (Person->Ware is set).
// Interior uses separate ambient lighting and different culling from the city renderer.
void AENG_draw_warehouse()
{
    SLONG i;

    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG px;
    SLONG pz;

    SLONG dist;
    SLONG t_index;
    SLONG square;
    SLONG build;
    SLONG page;
    SLONG facet;
    SLONG f_list;
    SLONG exit;
    float world_x;
    float world_y;
    float world_z;

    SLONG old_aeng_draw_cloud_flag = aeng_draw_cloud_flag;
    UC_FALSE;

    aeng_draw_cloud_flag = UC_FALSE;

    Thing* p_thing;
    OB_Info* oi;
    PAP_Hi* ph;
    NIGHT_Square* nq;
    NIGHT_Colour* col;
    POLY_Point* pp;
    DFacet* df;
    // BALLOON_Balloon *bb;
    POLY_Point* quad[4];

    // POLY_frame_init(UC_FALSE,UC_FALSE);

    POLY_frame_init(UC_FALSE, UC_FALSE);

    // Work out which things are in view.
    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
        for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
            t_index = PAP_2LO(x, z).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                switch (p_thing->Class) {
                case CLASS_PERSON:

                    // Only draw people who are in warehouses.
                    if (p_thing->Genus.Person->PlayerID && (p_thing->Genus.Person->Flags & FLAG_PERSON_WAREHOUSE))
                        p_thing->Flags |= FLAGS_IN_VIEW;
                    else if (p_thing->Genus.Person->Flags & FLAG_PERSON_WAREHOUSE) {
                        if (POLY_sphere_visible(
                                float(p_thing->WorldPos.X >> 8),
                                float(p_thing->WorldPos.Y >> 8) + KERB_HEIGHT,
                                float(p_thing->WorldPos.Z >> 8),
                                256.0F / (AENG_DRAW_DIST * 256.0F))) {
                            p_thing->Flags |= FLAGS_IN_VIEW;
                        }
                    }

                    break;

                default:

                    // Draw everything else that is on a HIDDEN square (i.e. is inside the
                    // floorplan of the warehouse).
                    if (PAP_2HI(p_thing->WorldPos.X >> 16, p_thing->WorldPos.Z >> 16).Flags & PAP_FLAG_HIDDEN) {
                        p_thing->Flags |= FLAGS_IN_VIEW;
                    }

                    break;
                }

                t_index = p_thing->Child;
            }

            NIGHT_square[NIGHT_cache[x][z]].flag &= ~NIGHT_SQUARE_FLAG_DELETEME;
        }
    }

    // Rotate all the points.
    for (z = NGAMUT_point_zmin; z <= NGAMUT_point_zmax; z++) {
        for (x = NGAMUT_point_gamut[z].xmin; x <= NGAMUT_point_gamut[z].xmax; x++) {
            ASSERT(WITHIN(x, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(z, 0, PAP_SIZE_HI - 1));

            ph = &PAP_2HI(x, z);

            // We only have upper points inside a warehouse.
            world_x = x * 256.0F;
            world_y = ph->Alt * float(1 << ALT_SHIFT);
            world_z = z * 256.0F;

            pp = &AENG_upper[x & 63][z & 63];

            POLY_transform(
                world_x,
                world_y,
                world_z,
                pp);

            if (pp->MaybeValid()) {
                // Work out the colour of this point.
                px = x >> 2;
                pz = z >> 2;

                dx = x & 0x3;
                dz = z & 0x3;

                ASSERT(WITHIN(px, 0, PAP_SIZE_LO - 1));
                ASSERT(WITHIN(pz, 0, PAP_SIZE_LO - 1));

                square = NIGHT_cache[px][pz];

                ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
                ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);

                nq = &NIGHT_square[square];

                NIGHT_get_colour(
                    nq->colour[dx + dz * PAP_BLOCKS],
                    &pp->colour,
                    &pp->specular);

                apply_cloud((SLONG)world_x, (SLONG)world_y, (SLONG)world_z, &pp->colour);

                POLY_fadeout_point(pp);
            }
        }
    }

    // Who shall we generate shadows for?
    if (AENG_shadows_on) {
        Thing* darci = NET_PERSON(0);

        // Reference point for shadow LOD selection. Original used Darci's
        // position — fine in gameplay (camera follows her) but wrong in
        // cutscenes (EWAY or PLAYCUTS) where the cine camera can be far
        // from Darci, dropping NPCs shown in close-up to blob shadows.
        // Use the actual render camera position, which covers every case:
        // gameplay (follow-cam next to Darci), EWAY cutscenes (EWAY_grab_camera
        // already wrote fc->x/z), PLAYCUTS (AENG_set_camera wrote FC_cam[0]).
        // FC_cam[].x/z are in raw world units (same scale as Thing::WorldPos).
        SLONG ref_x = FC_cam[AENG_cur_fc_cam].x;
        SLONG ref_z = FC_cam[AENG_cur_fc_cam].z;

        // How many people do we generate shadows for?
        // See AENG_draw_city for the full comment on why this can't be bumped
        // on its own.
        // uc_orig: AENG_NUM_SHADOWS (fallen/DDEngine/Source/aeng.cpp)
#define AENG_NUM_SHADOWS 4

        // See AENG_draw_city for rationale; redefined here for self-containment.
#define AENG_SHADOW_MAX_DIST 0xC0000

        struct
        {
            Thing* p_person;
            SLONG dist;

        } shadow_person[AENG_NUM_SHADOWS];
        SLONG shadow_person_upto = 0;
        SLONG shadow_person_worst_dist = -UC_INFINITY;
        SLONG shadow_person_worst_person;

        for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
                t_index = PAP_2LO(x, z).MapWho;

                while (t_index) {
                    p_thing = TO_THING(t_index);

                    if (p_thing->Class == CLASS_PERSON && (p_thing->Flags & FLAGS_IN_VIEW)) {
                        // Distance from the reference point (Darci in gameplay,
                        // cine camera in cutscenes — see above).
                        dx = p_thing->WorldPos.X - ref_x;
                        dz = p_thing->WorldPos.Z - ref_z;

                        dist = abs(dx) + abs(dz);

                        if (dist < AENG_SHADOW_MAX_DIST) {
                            if (shadow_person_upto < AENG_NUM_SHADOWS) {
                                // Put this person in the shadow array.
                                shadow_person[shadow_person_upto].p_person = p_thing;
                                shadow_person[shadow_person_upto].dist = dist;

                                // Keep track of the furthest person away.
                                if (dist > shadow_person_worst_dist) {
                                    shadow_person_worst_dist = dist;
                                    shadow_person_worst_person = shadow_person_upto;
                                }

                                shadow_person_upto += 1;
                            } else {
                                if (dist < shadow_person_worst_dist) {
                                    // Replace the worst person.
                                    ASSERT(WITHIN(shadow_person_worst_person, 0, AENG_NUM_SHADOWS - 1));

                                    shadow_person[shadow_person_worst_person].p_person = p_thing;
                                    shadow_person[shadow_person_worst_person].dist = dist;

                                    // Find the worst person
                                    shadow_person_worst_dist = -UC_INFINITY;

                                    for (i = 0; i < AENG_NUM_SHADOWS; i++) {
                                        if (shadow_person[i].dist > shadow_person_worst_dist) {
                                            shadow_person_worst_dist = shadow_person[i].dist;
                                            shadow_person_worst_person = i;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    t_index = p_thing->Child;
                }
            }
        }

        // Draw the people's shadow maps.
        SLONG offset_x;
        SLONG offset_y;

        for (i = 0; i < shadow_person_upto; i++) {
            darci = shadow_person[i].p_person;

            memset(AENG_aa_buffer, 0, sizeof(AENG_aa_buffer));

            SMAP_person(
                darci,
                (UBYTE*)AENG_aa_buffer,
                AENG_AA_BUF_SIZE,
                AENG_AA_BUF_SIZE,
                0,
                -255,
                -0);

            ASSERT(AENG_AA_BUF_SIZE == 32);
            ASSERT(TEXTURE_SHADOW_SIZE == 64);

            offset_x = (i & 1) << 5;
            offset_y = (i & 2) << 4;

            // Plonk it into the shadow texture page.
            if (TEXTURE_shadow_lock()) {
                SLONG x;
                SLONG y;
                UWORD* line;
                UBYTE* buf = (UBYTE*)AENG_aa_buffer;
                UWORD* mapping = GetShadowPixelMapping();

                for (y = 0; y < AENG_AA_BUF_SIZE; y++) {
                    line = &TEXTURE_shadow_bitmap[((y + offset_y) * TEXTURE_shadow_pitch >> 1) + offset_x];

                    for (x = AENG_AA_BUF_SIZE - 1; x >= 0; x--) {
                        *line++ = mapping[*buf++];
                    }
                }

                TEXTURE_shadow_unlock();
            }

            // How we map floating points coordinates from 0 to 1 onto
            // where we plonked the shadow map in the texture page.
            AENG_project_offset_u = float(offset_x) / float(TEXTURE_SHADOW_SIZE);
            AENG_project_offset_v = float(offset_y) / float(TEXTURE_SHADOW_SIZE);
            AENG_project_mul_u = float(AENG_AA_BUF_SIZE) / float(TEXTURE_SHADOW_SIZE);
            AENG_project_mul_v = float(AENG_AA_BUF_SIZE) / float(TEXTURE_SHADOW_SIZE);

            {
                // Map this poly onto the mapsquares surrounding darci.
                SLONG i;

                SLONG mx;
                SLONG mz;
                SLONG dx;
                SLONG dz;

                SLONG mx1;
                SLONG mz1;
                SLONG mx2;
                SLONG mz2;

                SLONG mx_lo;
                SLONG mz_lo;

                MapElement* me[4];
                PAP_Hi* ph[4];

                SVector_F poly[4];
                SMAP_Link* projected;

                SLONG w_face;

                PrimFace4* p_f4;
                PrimPoint* pp;

                SLONG face_height;
                UBYTE face_order[4] = { 0, 1, 3, 2 };

                for (dx = -1; dx <= 1; dx++)
                    for (dz = -1; dz <= 1; dz++) {
                        mx = (darci->WorldPos.X >> 16) + dx;
                        mz = (darci->WorldPos.Z >> 16) + dz;

                        if (WITHIN(mx, 0, MAP_WIDTH - 2) && WITHIN(mz, 0, MAP_HEIGHT - 2)) {
                            me[0] = &MAP[MAP_INDEX(mx + 0, mz + 0)];
                            me[1] = &MAP[MAP_INDEX(mx + 1, mz + 0)];
                            me[2] = &MAP[MAP_INDEX(mx + 1, mz + 1)];
                            me[3] = &MAP[MAP_INDEX(mx + 0, mz + 1)];

                            ph[0] = &PAP_2HI(mx + 0, mz + 0);
                            ph[1] = &PAP_2HI(mx + 1, mz + 0);
                            ph[2] = &PAP_2HI(mx + 1, mz + 1);
                            ph[3] = &PAP_2HI(mx + 0, mz + 1);

                            if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
                                poly[3].X = float(mx << 8);
                                poly[3].Y = float(ph[0]->Alt << ALT_SHIFT);
                                poly[3].Z = float(mz << 8);

                                poly[0].X = poly[3].X + 256.0F;
                                poly[0].Y = float(ph[1]->Alt << ALT_SHIFT);
                                poly[0].Z = poly[3].Z;

                                poly[1].X = poly[3].X + 256.0F;
                                poly[1].Y = float(ph[2]->Alt << ALT_SHIFT);
                                poly[1].Z = poly[3].Z + 256.0F;

                                poly[2].X = poly[3].X;
                                poly[2].Y = float(ph[3]->Alt << ALT_SHIFT);
                                poly[2].Z = poly[3].Z + 256.0F;

                                if (ph[0]->Alt == ph[1]->Alt && ph[0]->Alt == ph[2]->Alt && ph[0]->Alt == ph[3]->Alt) {
                                    // This quad is coplanar.
                                    projected = SMAP_project_onto_poly(poly, 4);

                                    if (projected) {
                                        AENG_add_projected_shadow_poly(projected);
                                    }
                                } else {
                                    // Do each triangle separately.
                                    projected = SMAP_project_onto_poly(poly, 3);

                                    if (projected) {
                                        AENG_add_projected_fadeout_shadow_poly(projected);
                                    }

                                    // The triangles are 0,1,2 and 0,2,3.
                                    poly[1] = poly[0];

                                    projected = SMAP_project_onto_poly(poly + 1, 3);

                                    if (projected) {
                                        AENG_add_projected_fadeout_shadow_poly(projected);
                                    }
                                }
                            }
                        }
                    }

                mx1 = (darci->WorldPos.X >> 8) - 0x100 >> PAP_SHIFT_LO;
                mz1 = (darci->WorldPos.Z >> 8) - 0x100 >> PAP_SHIFT_LO;

                mx2 = (darci->WorldPos.X >> 8) + 0x100 >> PAP_SHIFT_LO;
                mz2 = (darci->WorldPos.Z >> 8) + 0x100 >> PAP_SHIFT_LO;

                SATURATE(mx1, 0, PAP_SIZE_LO - 1);
                SATURATE(mz1, 0, PAP_SIZE_LO - 1);
                SATURATE(mx2, 0, PAP_SIZE_LO - 1);
                SATURATE(mz2, 0, PAP_SIZE_LO - 1);

                for (mx_lo = mx1; mx_lo <= mx2; mx_lo++)
                    for (mz_lo = mz1; mz_lo <= mz2; mz_lo++) {
                        // Cast shadow on walkable faces.
                        w_face = PAP_2LO(mx_lo, mz_lo).Walkable;

                        while (w_face) {
                            if (w_face > 0) {
                                p_f4 = &prim_faces4[w_face];
                                face_height = prim_points[p_f4->Points[0]].Y;

                                if (face_height > ((darci->WorldPos.Y + 0x11000) >> 8)) {
                                    // This face is above Darci, so don't put her shadow on it.
                                } else {
                                    for (i = 0; i < 4; i++) {
                                        pp = &prim_points[p_f4->Points[face_order[i]]];

                                        poly[i].X = float(pp->X);
                                        poly[i].Y = float(pp->Y);
                                        poly[i].Z = float(pp->Z);
                                    }

                                    projected = SMAP_project_onto_poly(poly, 4);

                                    if (projected) {
                                        AENG_add_projected_shadow_poly(projected);
                                    }
                                }

                                w_face = p_f4->Col2;
                            } else {
                                struct RoofFace4* rf;
                                rf = &roof_faces4[-w_face];
                                face_height = rf->Y;

                                if (face_height > ((darci->WorldPos.Y + 0x11000) >> 8)) {
                                    // This face is above Darci, so don't put her shadow on it.
                                } else {
                                    SLONG mx, mz;

                                    mx = (rf->RX & 127) << 8;
                                    mz = (rf->RZ & 127) << 8;

                                    poly[0].X = (float)(mx);
                                    poly[0].Y = (float)(rf->Y);
                                    poly[0].Z = (float)(mz);

                                    poly[1].X = (float)((mx) + 256);
                                    poly[1].Y = (float)(rf->Y + (rf->DY[0] << ROOF_SHIFT));
                                    poly[1].Z = (float)((mz));

                                    poly[2].X = (float)((mx) + 256);
                                    poly[2].Y = (float)(rf->Y + (rf->DY[1] << ROOF_SHIFT));
                                    poly[2].Z = (float)((mz) + 256);

                                    poly[3].X = (float)((mx));
                                    poly[3].Y = (float)(rf->Y + (rf->DY[2] << ROOF_SHIFT));
                                    poly[3].Z = (float)((mz) + 256);

                                    // Assuming its coplanar!
                                    projected = SMAP_project_onto_poly(poly, 4);

                                    if (projected) {
                                        AENG_add_projected_shadow_poly(projected);
                                    }
                                }

                                w_face = rf->Next;
                            }
                        }
                    }
            }

            TEXTURE_shadow_update();
        }
    } else {
        // Do oval shadows instead!
        for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
            for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
                t_index = PAP_2LO(x, z).MapWho;

                while (t_index) {
                    p_thing = TO_THING(t_index);

                    if (p_thing->Flags & FLAGS_IN_VIEW) {
                        switch (p_thing->Class) {
                        case CLASS_PERSON:

                            OVAL_add(
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                32);

                            break;

                        case CLASS_SPECIAL:

                            OVAL_add(
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                16);

                            break;

                        case CLASS_BARREL:

                            OVAL_add(
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                32);

                            break;

                        default:
                            break;
                        }
                    }

                    t_index = p_thing->Child;
                }
            }
        }
    }

    // Create all the squares.
    for (z = NGAMUT_zmin; z <= NGAMUT_zmax; z++) {
        for (x = NGAMUT_gamut[z].xmin; x <= NGAMUT_gamut[z].xmax; x++) {
            ASSERT(WITHIN(x, 0, PAP_SIZE_HI - 2));
            ASSERT(WITHIN(z, 0, PAP_SIZE_HI - 2));

            ph = &PAP_2HI(x, z);

            if (!(ph->Flags & PAP_FLAG_HIDDEN)) {
                // We only draw hidden squares!
                continue;
            }

            // The four points of the quad.
            quad[0] = &AENG_upper[(x + 0) & 63][(z + 0) & 63];
            quad[1] = &AENG_upper[(x + 1) & 63][(z + 0) & 63];
            quad[2] = &AENG_upper[(x + 0) & 63][(z + 1) & 63];
            quad[3] = &AENG_upper[(x + 1) & 63][(z + 1) & 63];

            if (POLY_valid_quad(quad)) {
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

                POLY_add_quad(quad, page, UC_TRUE);
            }
        }
    }

    // Draw the objects and the things.
    for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
        for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
            /*

            {
                    CBYTE str[20];

                    sprintf(str, "%d,%d", x, z);

                    FONT2D_DrawString(str, (pos & 7) * 60 + 20, (pos >> 3) * 20 + 200, 0xff00ff);
            }

            pos += 1;

            */

            ASSERT(WITHIN(x, 0, PAP_SIZE_LO - 1));
            ASSERT(WITHIN(z, 0, PAP_SIZE_LO - 1));
            ASSERT(WITHIN(NIGHT_cache[x][z], 1, NIGHT_MAX_SQUARES - 1));

            col = NIGHT_square[NIGHT_cache[x][z]].colour;

            // Skip over the mapsquares.
            col += PAP_BLOCKS * PAP_BLOCKS;

            // The objects on this mapsquare.
            oi = OB_find(x, z);

            while (oi->prim) {
                // Only draw objects that are in buildings (assume that means our warehouse!).
                if (oi->flags & OB_FLAG_WAREHOUSE) {
                    if (oi->prim == 235) {
                        oi->yaw = GAME_TURN;
                    }

                    col = MESH_draw_poly(
                        oi->prim,
                        oi->x,
                        oi->y,
                        oi->z,
                        oi->yaw,
                        oi->pitch,
                        oi->roll,
                        col, 0xff, 0);

                    if (prim_objects[oi->prim].flag & PRIM_FLAG_ITEM) {
                        OB_ob[oi->index].yaw += 1;
                    }
                }

                oi += 1;
            }

            // Draw the facets on this square.
            f_list = PAP_2LO(x, z).ColVectHead;

            if (f_list) {
                exit = UC_FALSE;

                while (!exit) {
                    facet = facet_links[f_list];

                    ASSERT(facet);

                    if (facet < 0) {
                        // The last facet in the list for each square is negative.
                        facet = -facet;
                        exit = UC_TRUE;
                    }

                    df = &dfacets[facet];

                    if ((df->FacetType == STOREY_TYPE_NORMAL) || (df->FacetType == STOREY_TYPE_DOOR) || (df->FacetType == STOREY_TYPE_FENCE) || (df->FacetType == STOREY_TYPE_FENCE_FLAT)) {
                        build = df->Building;

                        // Has this facet's building been processed this gameturn yet?
                        if (df->Counter[AENG_cur_fc_cam] != SUPERMAP_counter[AENG_cur_fc_cam]) {
                            // warehouse walls only drawn if they are inside walls
                            if ((dbuildings[build].Type == BUILDING_TYPE_WAREHOUSE && (df->FacetFlags & FACET_FLAG_INSIDE)) || dbuildings[build].Type == BUILDING_TYPE_CRATE_IN || df->FacetType == STOREY_TYPE_DOOR) {
                                // Draw the facet.
                                if (df->FacetType == STOREY_TYPE_DOOR) {
                                    // Draw the outside around this facet but don't draw
                                    AENG_draw_box_around_recessed_door(&dfacets[facet], UC_TRUE);
                                } else
                                // if (df->FacetType == STOREY_TYPE_NORMAL) Why only draw normal facets?
                                {
                                    FACET_draw(facet, 0);

                                    build = df->Building;

                                    if (df->FacetType == STOREY_TYPE_NORMAL) // Why only draw normal facets?
                                        if (build) {
                                            if (dbuildings[build].Counter[AENG_cur_fc_cam] != SUPERMAP_counter[AENG_cur_fc_cam]) {
                                                // Draw all the walkable faces for this building.
                                                FACET_draw_walkable(build);

                                                // Mark the building as processed this gameturn.
                                                dbuildings[build].Counter[AENG_cur_fc_cam] = SUPERMAP_counter[AENG_cur_fc_cam];
                                            }
                                        }
                                }
                            }

                            // Mark this facet as drawn this gameturn already.
                            dfacets[facet].Counter[AENG_cur_fc_cam] = SUPERMAP_counter[AENG_cur_fc_cam];
                        }
                    }

                    f_list++;
                }
            }

            // Draw the things.
            t_index = PAP_2LO(x, z).MapWho;

            while (t_index) {
                p_thing = TO_THING(t_index);

                if (p_thing->Flags & FLAGS_IN_VIEW) {
                    switch (p_thing->DrawType) {
                    case DT_NONE:
                        break;

                    case DT_BUILDING:
                        break;

                    case DT_PRIM:
                        break;

                    case DT_ANIM_PRIM:
                        ANIM_obj_draw(p_thing, p_thing->Draw.Tweened);
                        break;

                    case DT_ROT_MULTI:

                        ASSERT(p_thing->Class == CLASS_PERSON);

                        if (p_thing->Genus.Person->PlayerID) {
                            if (FirstPersonMode) {
                                FirstPersonAlpha -= (TICK_RATIO * 16) >> TICK_SHIFT;
                                if (FirstPersonAlpha < MAX_FPM_ALPHA)
                                    FirstPersonAlpha = MAX_FPM_ALPHA;
                            } else {
                                FirstPersonAlpha += (TICK_RATIO * 16) >> TICK_SHIFT;
                                if (FirstPersonAlpha > 255)
                                    FirstPersonAlpha = 255;
                            }

                            // FIGURE_alpha = FirstPersonAlpha;
                            FIGURE_draw(p_thing);
                            // FIGURE_alpha = 255;
                        } else {
                            SLONG dx, dy, dz, dist;

                            dx = abs((p_thing->WorldPos.X >> 8) - AENG_cam_x);
                            dy = abs((p_thing->WorldPos.Y >> 8) - AENG_cam_y);
                            dz = abs((p_thing->WorldPos.Z >> 8) - AENG_cam_z);

                            dist = QDIST3(dx, dy, dz);

                            if (dist < AENG_DRAW_PEOPLE_DIST) {

                                FIGURE_draw(p_thing);
                            }
                        }

                        if (ControlFlag && allow_debug_keys) {
                            AENG_world_text(
                                (p_thing->WorldPos.X >> 8),
                                (p_thing->WorldPos.Y >> 8) + 0x60,
                                (p_thing->WorldPos.Z >> 8),
                                200,
                                180,
                                50,
                                UC_TRUE,
                                PCOM_person_state_debug(p_thing));
                        }

                        if ((p_thing->State == STATE_DEAD) && (p_thing->Genus.Person->Timer1 > 10)) {
                            if (p_thing->Genus.Person->PersonType == PERSON_MIB1 || p_thing->Genus.Person->PersonType == PERSON_MIB2 || p_thing->Genus.Person->PersonType == PERSON_MIB3) {
                                // Dead MIB self destruct!
                                DRAWXTRA_MIB_destruct(p_thing);
                            }
                        }

                        break;

                    case DT_EFFECT:
                        break;

                    case DT_MESH:

                        if (p_thing->Class == CLASS_SPECIAL) {
                            DRAWXTRA_Special(p_thing);
                        }

                        if (p_thing->Class == CLASS_BIKE) {
                            // There shouldn't be any bikes indoors.
                            ASSERT(0);
                        } else {
                            MESH_draw_poly(
                                p_thing->Draw.Mesh->ObjectId,
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                p_thing->Draw.Mesh->Angle,
                                p_thing->Draw.Mesh->Tilt,
                                p_thing->Draw.Mesh->Roll,
                                NULL, 0xff, 0);
                        }

                        break;

                    case DT_VEHICLE:
                    case DT_CHOPPER:

                        // There shouldn't be any vehicles or helicopters indoors.
                        break;

                    case DT_PYRO:
                        PYRO_draw_pyro(p_thing);
                        break;

                    case DT_ANIMAL_PRIM:

                        break;

                    case DT_TRACK:
                        TRACKS_DrawTrack(p_thing);
                        break;

                    default:
                        break;
                    }
                }

                t_index = p_thing->Child;
            }
        }
    }

    // Now draw the special fx that somebody left out before
    POLY_set_local_rotation_none();
    PARTICLE_Draw();
    RIBBON_draw();
    AENG_draw_sparks();

    // Now actually draw everything!
    POLY_frame_draw(UC_TRUE, UC_TRUE);

    // Restore cloud to default value.
    aeng_draw_cloud_flag = old_aeng_draw_cloud_flag;
}

// uc_orig: AENG_screen_shot (fallen/DDEngine/Source/aeng.cpp)
// Dumps the screen to a TGA file when KB_S is pressed (debug only).
void AENG_screen_shot(void)
{
    if (allow_debug_keys)
        if (Keys[KB_S] || record_video) {
            Keys[KB_S] = 0;
            if (ShiftFlag) {
                record_video ^= 1;
            }

            extern void tga_dump(void);
            tga_dump();
        }
}

// uc_orig: AENG_draw_messages (fallen/DDEngine/Source/aeng.cpp)
// Legacy FPS display using direct screen lock (mostly superseded by AENG_draw_FPS).
void AENG_draw_messages()
{
    static SLONG fps = 0;
    static SLONG last_game_turn = 0;
    static clock_t last_time = 0;
    clock_t this_time = 0;

    this_time = clock() / CLOCKS_PER_SEC;

    if (this_time != last_time) {
        if (this_time - last_time > 1) {
            // Only work out the new frames per second if there hasn't been a major delay.
        } else {
            fps = GAME_TURN - last_game_turn;
        }

        last_time = this_time;
        last_game_turn = GAME_TURN;
    }

    // Text now emits textured quads through the TL pipeline — no backbuffer
    // lock/unlock. Wrapping the draws in ge_lock_screen/ge_unlock_screen would
    // unconditionally overwrite them with the pre-draw framebuffer contents.
    FONT_draw(DisplayWidth >> 1, 10, "FPS: %d", fps);
    MSG_draw();
}

// uc_orig: AENG_fade_out (fallen/DDEngine/Source/aeng.cpp)
// Draws the MuckyFoot logo and a full-screen white overlay with alpha = amount,
// used for the intro screen fade-out effect.
void AENG_fade_out(UBYTE amount)
{
    SLONG logo_fade_top = amount;
    SLONG back_fade_top = amount;

    ULONG logo_colour_top = (logo_fade_top << 24) | 0x00ffffff;
    ULONG back_colour_top = (back_fade_top << 24) | 0x00ffffff;

    SLONG logo_fade_bot = amount;
    SLONG back_fade_bot = amount;

    ULONG logo_colour_bot = (logo_fade_bot << 24) | 0x00ffffff;
    ULONG back_colour_bot = (back_fade_bot << 24) | 0x00ffffff;

    // Draw the logo.
    POLY_Point pp[4];
    POLY_Point* quad[4];

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

// uc_orig: AENG_LOGO_MID_X (fallen/DDEngine/Source/aeng.cpp)
#define AENG_LOGO_MID_X (320.0F)
// uc_orig: AENG_LOGO_MID_Y (fallen/DDEngine/Source/aeng.cpp)
#define AENG_LOGO_MID_Y (240.0F)
// uc_orig: AENG_LOGO_SIZE (fallen/DDEngine/Source/aeng.cpp)
#define AENG_LOGO_SIZE (128.0F)

    POLY_frame_init(UC_FALSE, UC_FALSE);

    pp[0].X = AENG_LOGO_MID_X - AENG_LOGO_SIZE;
    pp[0].Y = AENG_LOGO_MID_Y - AENG_LOGO_SIZE;
    pp[0].z = 1.0F / 65536.0F;
    pp[0].Z = 1.0F;
    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[0].colour = logo_colour_top;
    pp[0].specular = 0x00000000;

    pp[1].X = AENG_LOGO_MID_X + AENG_LOGO_SIZE;
    pp[1].Y = AENG_LOGO_MID_Y - AENG_LOGO_SIZE;
    pp[1].z = 1.0F / 65536.0F;
    pp[1].Z = 1.0F;
    pp[1].u = 1.0F;
    pp[1].v = 0.0F;
    pp[1].colour = logo_colour_top;
    pp[1].specular = 0x00000000;

    pp[2].X = AENG_LOGO_MID_X - AENG_LOGO_SIZE;
    pp[2].Y = AENG_LOGO_MID_Y + AENG_LOGO_SIZE;
    pp[2].z = 1.0F / 65536.0F;
    pp[2].Z = 1.0F;
    pp[2].u = 0.0F;
    pp[2].v = 1.0F;
    pp[2].colour = logo_colour_bot;
    pp[2].specular = 0x00000000;

    pp[3].X = AENG_LOGO_MID_X + AENG_LOGO_SIZE;
    pp[3].Y = AENG_LOGO_MID_Y + AENG_LOGO_SIZE;
    pp[3].z = 1.0F / 65536.0F;
    pp[3].Z = 1.0F;
    pp[3].u = 1.0F;
    pp[3].v = 1.0F;
    pp[3].colour = logo_colour_bot;
    pp[3].specular = 0x00000000;

    POLY_add_quad(quad, POLY_PAGE_LOGO, UC_FALSE, UC_TRUE);

    POLY_frame_draw(UC_TRUE, UC_TRUE);
    POLY_frame_init(UC_FALSE, UC_FALSE);

    pp[0].X = 0.0F;
    pp[0].Y = 0.0F;
    pp[0].z = 2.0F / 65536.0F;
    pp[0].Z = 65535.0F / 65536.0F;
    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[0].colour = back_colour_top;
    pp[0].specular = 0x00000000;

    pp[1].X = 640.0F;
    pp[1].Y = 0.0F;
    pp[1].z = 2.0F / 65536.0F;
    pp[1].Z = 65535.0F / 65536.0F;
    pp[1].u = 1.0F;
    pp[1].v = 0.0F;
    pp[1].colour = back_colour_top;
    pp[1].specular = 0x00000000;

    pp[2].X = 0.0F;
    pp[2].Y = 480.0F;
    pp[2].z = 2.0F / 65536.0F;
    pp[2].Z = 65535.0F / 65536.0F;
    pp[2].u = 0.0F;
    pp[2].v = 1.0F;
    pp[2].colour = back_colour_bot;
    pp[2].specular = 0x00000000;

    pp[3].X = 640.0F;
    pp[3].Y = 480.0F;
    pp[3].z = 2.0F / 65536.0F;
    pp[3].Z = 65535.0F / 65536.0F;
    pp[3].u = 1.0F;
    pp[3].v = 1.0F;
    pp[3].colour = back_colour_bot;
    pp[3].specular = 0x00000000;

    POLY_add_quad(quad, POLY_PAGE_ALPHA, UC_FALSE, UC_TRUE);

    POLY_frame_draw(UC_TRUE, UC_TRUE);
}

// uc_orig: AENG_clear_screen (fallen/DDEngine/Source/aeng.cpp)
// Clears the entire back buffer to black and reclaims vertex pool buffers.
void AENG_clear_screen()
{
    ge_set_background(GEBackground::Black);
    ge_clear(true, true);
    ge_reclaim_vertex_buffers();
}

// AENG_lock / AENG_unlock — removed. These were thin wrappers around
// ge_lock_screen / ge_unlock_screen with no in-tree callers left. The direct
// pixel access pattern they fronted is gone along with the screen-buffer
// path in the GL backend (see Stage 1-3 of the startup-hang fix).

// =============================================================================
// Chunk 5b: flip/blit, editor 3D debug lines, raycast, light/waypoint/
//           trigger draw, viewport clear, top-level AENG_draw, detail level I/O,
//           recessed door box, dfcache pruning, indoor floor renderer.
// =============================================================================

// uc_orig: AENG_flip (fallen/DDEngine/Source/aeng.cpp)
// Flips the back buffer to the primary surface. Comment notes PerMedia2 requires
// DDFLIP_WAIT; dropping it causes corruption on that GPU.
void AENG_flip()
{
    ge_flip();
}

// uc_orig: AENG_blit (fallen/DDEngine/Source/aeng.cpp)
// Blits the back buffer to the primary surface (non-flipping path).
void AENG_blit()
{
    ge_blit_back_buffer();
}

// uc_orig: AENG_raytraced_position (fallen/DDEngine/Source/aeng.cpp)
// Casts a ray from the camera through screen pixel (sx,sy) and finds where it
// intersects the ground surface (or indoor floor). Returns UC_TRUE on hit; fills
// world_x/y/z with the intersection point.
SLONG AENG_raytraced_position(
    SLONG sx,
    SLONG sy,
    SLONG* world_x,
    SLONG* world_y,
    SLONG* world_z,
    SLONG indoors)
{
    SLONG i;

    float ax;
    float ay;

    float ex;
    float ey;
    float ez;

    float dx;
    float dy;
    float dz;
    float len;

    SLONG wx;
    SLONG wy;
    SLONG wz;

    float rx = AENG_cam_x;
    float ry = AENG_cam_y;
    float rz = AENG_cam_z;

    // Use the view frustum cone to find the direction of the ray through (sx, sy).
    ax = float(sx) * (1.0F / 640.0F);
    ay = float(sy) * (1.0F / 480.0F);

    ex = AENG_cone[1].x;
    ey = AENG_cone[1].y;
    ez = AENG_cone[1].z;

    ex += ax * (AENG_cone[0].x - AENG_cone[1].x);
    ey += ax * (AENG_cone[0].y - AENG_cone[1].y);
    ez += ax * (AENG_cone[0].z - AENG_cone[1].z);

    ex += ay * (AENG_cone[2].x - AENG_cone[1].x);
    ey += ay * (AENG_cone[2].y - AENG_cone[1].y);
    ez += ay * (AENG_cone[2].z - AENG_cone[1].z);

    // All in mapsquare coordinates, not world coordinates.
    ex *= 256.0F;
    ey *= 256.0F;
    ez *= 256.0F;

    // Direction of the ray.
    dx = ex - rx;
    dy = ey - ry;
    dz = ez - rz;

    len = sqrt(dx * dx + dy * dy + dz * dz);

// uc_orig: AENG_RAYTRACE_ACCURACY (fallen/DDEngine/Source/aeng.cpp)
#define AENG_RAYTRACE_ACCURACY 16

    dx *= (256.0F / AENG_RAYTRACE_ACCURACY) / len;
    dy *= (256.0F / AENG_RAYTRACE_ACCURACY) / len;
    dz *= (256.0F / AENG_RAYTRACE_ACCURACY) / len;

    if (GAME_FLAGS & GF_SEWERS) {
        // Use the sewer LOS function.
        SLONG x1 = SLONG(rx);
        SLONG y1 = SLONG(ry);
        SLONG z1 = SLONG(rz);

        SLONG x2 = SLONG(rx + dx * (AENG_RAYTRACE_ACCURACY * 16));
        SLONG y2 = SLONG(ry + dy * (AENG_RAYTRACE_ACCURACY * 16));
        SLONG z2 = SLONG(rz + dz * (AENG_RAYTRACE_ACCURACY * 16));

        if (NS_there_is_a_los(
                x1, y1, z1,
                x2, y2, z2)) {
            return UC_FALSE;
        }

        *world_x = NS_los_fail_x;
        *world_y = NS_los_fail_y;
        *world_z = NS_los_fail_z;

        if (!WITHIN(*world_x, 0, (PAP_SIZE_HI << 8) - 1) || !WITHIN(*world_z, 0, (PAP_SIZE_HI << 8) - 1)) {
            return UC_FALSE;
        } else {
            return UC_TRUE;
        }
    }

    // Intersect the ray with the world by stepping along it.
    rx += dx * AENG_RAYTRACE_ACCURACY;
    ry += dy * AENG_RAYTRACE_ACCURACY;
    rz += dz * AENG_RAYTRACE_ACCURACY;

    for (i = 0; i < AENG_RAYTRACE_ACCURACY * (AENG_DRAW_DIST - 1); i++) {
        wx = (SLONG)rx;
        wy = (SLONG)ry;
        wz = (SLONG)rz;

        if (
            (indoors && (wy < get_inside_alt(indoors))) || ((!indoors) && (wy < PAP_calc_map_height_at(wx, wz)))) {
            if (indoors)
                wy = get_inside_alt(indoors);
            else
                wy = PAP_calc_map_height_at(wx, wz);

            *world_x = wx;
            *world_y = wy;
            *world_z = wz;

            if (!WITHIN(*world_x, 0, (PAP_SIZE_HI << 8) - 1) || !WITHIN(*world_z, 0, (PAP_SIZE_HI << 8) - 1)) {
                return UC_FALSE;
            } else {
                return UC_TRUE;
            }
        }

        rx += dx;
        ry += dy;
        rz += dz;
    }

    return UC_FALSE;
}

// uc_orig: AENG_world_text (fallen/DDEngine/Source/aeng.cpp)
// Projects a world-space point and queues a formatted text string at the result.
// Used by editor and debug HUD to label objects in 3D.
void AENG_world_text(
    SLONG x,
    SLONG y,
    SLONG z,
    UBYTE red,
    UBYTE blue,
    UBYTE green,
    UBYTE shadowed_or_not,
    CBYTE* fmt, ...)
{
    POLY_Point pp;

    POLY_transform(
        float(x),
        float(y),
        float(z),
        &pp);

    if (pp.IsValid()) {
        CBYTE message[FONT_MAX_LENGTH];
        va_list ap;

        va_start(ap, fmt);
        vsprintf(message, fmt, ap);
        va_end(ap);

        // pp.X / pp.Y come out of POLY_perspective in virtual screen pixels
        // (0..POLY_screen_width × 0..POLY_screen_height = 0..480*aspect ×
        // 0..480). FONT_buffer_add_virtual scales them to live framebuffer
        // pixels at flush time using PolyPage::s_XScale / s_YScale — which
        // tracks the post-composition UI viewport, so 3D-anchored labels
        // land on the character at any aspect / render-scale instead of
        // sitting in the upper-left because pp.X was treated as raw px.
        FONT_buffer_add_virtual(
            (SLONG)pp.X,
            (SLONG)pp.Y,
            red,
            green,
            blue,
            shadowed_or_not,
            message);
    }
}

// uc_orig: AENG_clear_viewport (fallen/DDEngine/Source/aeng.cpp)
// Clears the viewport each frame. Indoors/sewers always use black. Outdoors
// uses the sky colour (or grey when draw_3d stereo mode is active, or black
// during fade_black transitions).
void AENG_clear_viewport()
{
    if (INDOORS_INDEX || (GAME_FLAGS & GF_SEWERS) || (GAME_FLAGS & GF_INDOORS)) {
        ge_set_background(GEBackground::Black);
        ge_clear(true, true);
    } else {
        if (draw_3d) {
            SLONG white = NIGHT_sky_colour.red + NIGHT_sky_colour.green + NIGHT_sky_colour.blue;

            white /= 3;

            ge_set_background_color(
                white,
                white,
                white);
        } else {
            if (fade_black) {
                ge_set_background_color(0, 0, 0);
            } else {
                ge_set_background_color(
                    NIGHT_sky_colour.red,
                    NIGHT_sky_colour.green,
                    NIGHT_sky_colour.blue);
            }
        }

        ge_set_background(GEBackground::User);
        ge_clear(true, true);
    }

    // Aspect bars are no longer painted here. The scene FBO is the game's
    // "screen" and already fills its own aspect range edge-to-edge (aspect
    // clamp applied at FBO creation in OpenDisplay). If the real window
    // aspect lies outside [FOV_MIN_ASPECT, FOV_CAP_ASPECT], the
    // composition layer paints outer pillar/letterbox bars over the real
    // backbuffer when it blits the FBO — no game-side involvement needed.

    BreakTime("Cleared Viewport");
}

// uc_orig: AENG_draw (fallen/DDEngine/Source/aeng.cpp)
// Top-level frame render entry point, called once per game tick.
// Handles: cloud movement, dynamic lighting, splitscreen vs single-camera,
// cutscene camera override, playback/record of camera data, warehouse vs city.
void AENG_draw(SLONG draw_3d)
{
    /*

    if (Keys[KB_PPOINT])
    {
            Keys[KB_PPOINT] = 0;

            NIGHT_amb_red   <<= 1;
            NIGHT_amb_green <<= 1;
            NIGHT_amb_blue  <<= 1;

            NIGHT_Colour amb_colour;

            amb_colour.red   = NIGHT_amb_red;
            amb_colour.green = NIGHT_amb_green;
            amb_colour.blue  = NIGHT_amb_blue;

            NIGHT_get_colour(
                    amb_colour,
               &NIGHT_amb_colour,
               &NIGHT_amb_specular);

            NIGHT_cache_recalc();
            NIGHT_dfcache_recalc();
            NIGHT_generate_walkable_lighting();
    }

    */

    SLONG i;
    SLONG warehouse;

    FC_Cam* fc;

    /*
            if (Keys[KB_RBRACE])
            {
                    Keys[KB_RBRACE] = 0;

                    AENG_transparent_warehouses ^= 1;
            }
    */

    AENG_drawing_a_warehouse = UC_FALSE;

    // Update stuff.
    // d	AENG_movie_update();
    move_clouds();
    POLY_set_wibble(62, 137, 17, 178, 20, 25);

    AENG_clear_viewport();

    // Reclaim vertex buffers.
    ge_reclaim_vertex_buffers();

    // Put in the dynamic lighting.
    NIGHT_dlight_squares_up();

    POLY_colour_restrict = 0;
    POLY_force_additive_alpha = UC_FALSE;

    // Mark all NIGHT cache squares for deletion.
    AENG_mark_night_squares_as_deleteme();

    if (FC_cam[1].focus) {
        // Splitscreen mode.
        SUPERMAP_counter_increase(0);
        SUPERMAP_counter_increase(1);

        for (i = 0; i < FC_MAX_CAMS; i++) {
            fc = &FC_cam[i];

            // OC_FOV_MULTIPLIER pre-scales the live camera lens so every
            // downstream consumer (POLY_camera_set → MATRIX_skew, the
            // gamut / map-square visibility cone in AENG_calc_gamut, 3D
            // rain positioning, etc.) sees the same widened FOV. Applying
            // it later (e.g. only inside POLY_camera_set) desyncs culling
            // from rendering → geometry that is visible after the widening
            // gets pre-culled by the un-widened gamut cone, producing
            // black cut-outs in the periphery.
            //
            // Auto zoom-out (aspect < 4:3): widens FOV by base/aspect so
            // horizontal extent stays at 4:3-equivalent and the character
            // doesn't pixel-scale up with extra vertical window height.
            // The extra height fills with more sky / ground instead. Zoom
            // is clamped at base/MIN so the letterbox branch in
            // POLY_camera_set can take over continuously (character size
            // stays 4:3-width-equivalent across the MIN boundary).
            {
                const float real_aspect = float(ScreenWidth) / float(ScreenHeight);
                const float base_aspect = float(DisplayWidth) / float(DisplayHeight);
                const float min_aspect  = float(FOV_MIN_ASPECT);
                float auto_zoom = 1.0F;
                if (real_aspect < base_aspect) {
                    const float zoom_aspect = (real_aspect < min_aspect) ? min_aspect : real_aspect;
                    auto_zoom = base_aspect / zoom_aspect;
                }
                AENG_lens = fc->lens * 1.5F * (1.0F / float(65536.0F))
                    / (float(OC_FOV_MULTIPLIER) * auto_zoom);
            }

            AENG_set_camera_radians(
                fc->x >> 8,
                fc->y >> 8,
                fc->z >> 8,
                float(fc->yaw) * (2.0F * PI / (2048.0F * 256.0F)),
                float(fc->pitch) * (2.0F * PI / (2048.0F * 256.0F)),
                float(fc->roll) * (2.0F * PI / (2048.0F * 256.0F)),
                (i == 0) ? POLY_SPLITSCREEN_TOP : POLY_SPLITSCREEN_BOTTOM);

            AENG_cur_fc_cam = i;

            if (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->Ware) {
                AENG_ensure_appropriate_caching(UC_TRUE);
                AENG_draw_warehouse();
            } else {
                AENG_ensure_appropriate_caching(UC_FALSE);
                AENG_draw_city();
            }
        }

        AENG_get_rid_of_deleteme_squares();
        AENG_get_rid_of_unused_dfcache_lighting(UC_TRUE);
    } else {
        fc = &FC_cam[0];

        SUPERMAP_counter_increase(0);

        // Not splitscreen. We might have to use the cutscene camera.
        SLONG old_cam_x = fc->x;
        SLONG old_cam_y = fc->y;
        SLONG old_cam_z = fc->z;
        SLONG old_cam_yaw = fc->yaw;
        SLONG old_cam_pitch = fc->pitch;
        SLONG old_cam_roll = fc->roll;
        SLONG old_cam_lens = fc->lens;

        // If there is a cut-scene camera, let EWAY override the FC_cam fields.
        if (EWAY_grab_camera(
                &fc->x,
                &fc->y,
                &fc->z,
                &fc->yaw,
                &fc->pitch,
                &fc->roll,
                &fc->lens)) {
            warehouse = EWAY_camera_warehouse();
        } else {
            warehouse = (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->Ware);
        }

        extern MFFileHandle playback_file;
        extern MFFileHandle verifier_file;

        if (GAME_STATE & GS_PLAYBACK) {
            FileRead(playback_file, &fc->x, sizeof(fc->x));
            FileRead(playback_file, &fc->y, sizeof(fc->y));
            FileRead(playback_file, &fc->z, sizeof(fc->z));
            FileRead(playback_file, &fc->yaw, sizeof(fc->yaw));
            FileRead(playback_file, &fc->pitch, sizeof(fc->pitch));
            FileRead(playback_file, &fc->roll, sizeof(fc->roll));
            FileRead(playback_file, &fc->lens, sizeof(fc->lens));

            if (verifier_file) {
                extern void check_thing_data();
                check_thing_data();
            }
        } else if (GAME_STATE & GS_RECORD) {
            FileWrite(playback_file, &fc->x, sizeof(fc->x));
            FileWrite(playback_file, &fc->y, sizeof(fc->y));
            FileWrite(playback_file, &fc->z, sizeof(fc->z));
            FileWrite(playback_file, &fc->yaw, sizeof(fc->yaw));
            FileWrite(playback_file, &fc->pitch, sizeof(fc->pitch));
            FileWrite(playback_file, &fc->roll, sizeof(fc->roll));
            FileWrite(playback_file, &fc->lens, sizeof(fc->lens));

            if (verifier_file) {
                extern void store_thing_data();
                store_thing_data();
            }
        }

        // See the note above the splitscreen branch for why the multiplier
        // AND auto-zoom are applied to AENG_lens itself rather than inside
        // POLY_camera_set.
        {
            const float real_aspect = float(ScreenWidth) / float(ScreenHeight);
            const float base_aspect = float(DisplayWidth) / float(DisplayHeight);
            const float min_aspect  = float(FOV_MIN_ASPECT);
            float auto_zoom = 1.0F;
            if (real_aspect < base_aspect) {
                const float zoom_aspect = (real_aspect < min_aspect) ? min_aspect : real_aspect;
                auto_zoom = base_aspect / zoom_aspect;
            }
            AENG_lens = fc->lens * (1.0F / float(65536.0F))
                / (float(OC_FOV_MULTIPLIER) * auto_zoom);
        }

        AENG_set_camera_radians(
            fc->x >> 8,
            fc->y >> 8,
            fc->z >> 8,
            float(fc->yaw) * (2.0F * PI / (2048.0F * 256.0F)),
            float(fc->pitch) * (2.0F * PI / (2048.0F * 256.0F)),
            float(fc->roll) * (2.0F * PI / (2048.0F * 256.0F)),
            POLY_SPLITSCREEN_NONE);

        AENG_cur_fc_cam = 0;

        if (warehouse) {
            AENG_drawing_a_warehouse = UC_TRUE;

            AENG_ensure_appropriate_caching(UC_TRUE);
            AENG_draw_warehouse();
        } else {
            AENG_ensure_appropriate_caching(UC_FALSE);
            AENG_draw_city();

            if (AENG_transparent_warehouses) {
                SUPERMAP_counter_increase(0);

                AENG_ensure_appropriate_caching(UC_TRUE);
                AENG_draw_warehouse();
            }
        }

        AENG_get_rid_of_deleteme_squares();
        AENG_get_rid_of_unused_dfcache_lighting(UC_FALSE);

        // Restore the camera to its pre-cutscene values.
        fc->x = old_cam_x;
        fc->y = old_cam_y;
        fc->z = old_cam_z;
        fc->yaw = old_cam_yaw;
        fc->pitch = old_cam_pitch;
        fc->roll = old_cam_roll;
        fc->lens = old_cam_lens;
    }

    /*

    if (draw_3d)
    {
            ...stereo/left-eye/right-eye block (commented out in original)...
    }
    else
    {
    ...
    }

    */

    // Take out the dynamic lighting.
    NIGHT_dlight_squares_down();

    POLY_colour_restrict = 0;
    POLY_force_additive_alpha = UC_FALSE;

    // SPONG
    // #if !USE_TOMS_ENGINE_PLEASE_BOB
    //
    //  Do it here so that AENG_world_line works during the game.
    //
    POLY_frame_init(UC_FALSE, UC_FALSE);

    // Render pass is now active — geometry added via AENG_world_line will not
    // be cleared by POLY_frame_init and can safely render this frame.
    AENG_set_render_pass(true);
    // #endif
}

// uc_orig: AENG_read_detail_levels (fallen/DDEngine/Source/aeng.cpp)
// Reads per-feature detail level settings from the environment (ini/registry).
void AENG_read_detail_levels()
{
    AENG_detail_stars = ENV_get_value_number("detail_stars", 1, "Render");
    AENG_detail_shadows = ENV_get_value_number("detail_shadows", 1, "Render");
    AENG_detail_moon_reflection = ENV_get_value_number("detail_moon_reflection", 1, "Render");
    AENG_detail_people_reflection = ENV_get_value_number("detail_people_reflection", 1, "Render");
    AENG_detail_puddles = ENV_get_value_number("detail_puddles", 1, "Render");
    AENG_detail_dirt = ENV_get_value_number("detail_dirt", 1, "Render");
    AENG_detail_mist = ENV_get_value_number("detail_mist", 1, "Render");
    AENG_detail_rain = ENV_get_value_number("detail_rain", 1, "Render");
    AENG_detail_skyline = ENV_get_value_number("detail_skyline", 1, "Render");
    AENG_detail_filter = ENV_get_value_number("detail_filter", 1, "Render");
    AENG_detail_perspective = ENV_get_value_number("detail_perspective", 1, "Render");
    AENG_detail_crinkles = ENV_get_value_number("detail_crinkles", 1, "Render");
}

// uc_orig: AENG_draw_box_around_recessed_door (fallen/DDEngine/Source/aeng.cpp)
// Draws a short tunnel segment connecting a recessed door to the street,
// using per-tile floor textures and sky-coloured ceiling/walls.
// inside_out flips the door direction (for rendering the inside-facing side).
void AENG_draw_box_around_recessed_door(DFacet* df, SLONG inside_out)
{
    SLONG i;

    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    SLONG page;
    SLONG upto;

    ULONG sky_colour;
    ULONG sky_specular;

    SLONG col_page;

    NIGHT_get_colour(
        NIGHT_sky_colour,
        &sky_colour,
        &sky_specular);

    sky_specular |= 0xff000000;

    PAP_Hi* ph;

    float wx;
    float wy;
    float wz;

// uc_orig: MAX_DOOR_LENGTH (fallen/DDEngine/Source/aeng.cpp)
#define MAX_DOOR_LENGTH 8

    POLY_Point lower[MAX_DOOR_LENGTH][2];
    POLY_Point upper[MAX_DOOR_LENGTH][2];

    POLY_Point* pp;
    POLY_Point* quad[4];

    if (inside_out) {
        SWAP(df->x[0], df->x[1]);
        SWAP(df->z[0], df->z[1]);
    }

    col_page = POLY_PAGE_COLOUR_WITH_FOG;

    // Create the points along the door span.
    x = df->x[0];
    z = df->z[0];

    dx = df->x[1] - df->x[0];
    dz = df->z[1] - df->z[0];

    dx = SIGN(dx);
    dz = SIGN(dz);

    upto = 0;

    while (1) {
        ASSERT(WITHIN(upto, 0, MAX_DOOR_LENGTH - 1));

        for (i = 0; i < 4; i++) {
            wx = float(x << 8);
            wz = float(z << 8);
            wy = float(df->Y[0]);

            if (i & 1) {
                // One block inside the warehouse.
                wx += float(-dz << 8);
                wz += float(+dx << 8);
            }

            if (i & 2) {
                // The upper level.
                pp = &upper[upto][i & 1];
                wy += 256.0F;
            } else {
                // The lower level.
                pp = &lower[upto][i & 1];
            }

            POLY_transform(
                wx,
                wy,
                wz,
                pp);

            if (i == 0) {
                if (inside_out) {
                    pp->colour = 0xffaaaaaa;
                    pp->specular = 0xff000000;
                } else {
                    pp->colour = 0xffaaaaaa;
                    pp->specular = 0xff000000;
                }
            } else {
                if (inside_out) {
                    pp->colour = 0xffffffff;
                    pp->specular = 0x01000000; // Completely fogged out.
                } else {
                    pp->colour = 0xff000000;
                    pp->specular = 0xff000000;
                }
            }

            pp->u = 0.0F;
            pp->v = 0.0F;
        }

        upto += 1;

        if (x == df->x[1] && z == df->z[1]) {
            break;
        }

        x += dx;
        z += dz;
    }

    ASSERT(upto >= 2);

    // Floor and ceiling faces along the span.
    x = df->x[0];
    z = df->z[0];

    for (i = 0; i < upto - 1; i++) {
        mx = (x << 8) + dx - dz >> 8;
        mz = (z << 8) + dz + dx >> 8;

        // The floor.
        ph = &PAP_2HI(mx, mz);

        quad[0] = &lower[i + 0][0];
        quad[1] = &lower[i + 0][1];
        quad[2] = &lower[i + 1][0];
        quad[3] = &lower[i + 1][1];

        if (POLY_valid_quad(quad)) {
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

            POLY_add_quad(quad, page, UC_FALSE);
        }

        // The ceiling.
        quad[0] = &upper[i + 0][0];
        quad[1] = &upper[i + 0][1];
        quad[2] = &upper[i + 1][0];
        quad[3] = &upper[i + 1][1];

        if (POLY_valid_quad(quad)) {
            POLY_add_quad(quad, col_page, UC_FALSE);
        }

        // The back wall.
        quad[0] = &lower[i + 0][1];
        quad[1] = &lower[i + 1][1];
        quad[2] = &upper[i + 0][1];
        quad[3] = &upper[i + 1][1];

        if (POLY_valid_quad(quad)) {
            POLY_add_quad(quad, col_page, UC_FALSE);
        }

        x += dx;
        z += dz;
    }

    // End cap faces.
    quad[0] = &upper[0][0];
    quad[2] = &upper[0][1];
    quad[1] = &lower[0][0];
    quad[3] = &lower[0][1];

    if (POLY_valid_quad(quad)) {
        if (!inside_out) {
            quad[0]->colour = 0xff000000;
            quad[0]->specular = 0xff000000;
            quad[1]->colour = 0xff000000;
            quad[1]->specular = 0xff000000;
        }

        POLY_add_quad(quad, col_page, UC_TRUE);
    }

    quad[0] = &upper[upto - 1][0];
    quad[1] = &upper[upto - 1][1];
    quad[2] = &lower[upto - 1][0];
    quad[3] = &lower[upto - 1][1];

    if (POLY_valid_quad(quad)) {
        if (!inside_out) {
            quad[0]->colour = 0xff000000;
            quad[0]->specular = 0xff000000;
            quad[2]->colour = 0xff000000;
            quad[2]->specular = 0xff000000;
        }

        POLY_add_quad(quad, col_page, UC_TRUE);
    }

    if (inside_out) {
        SWAP(df->x[0], df->x[1]);
        SWAP(df->z[0], df->z[1]);
    }
}

// uc_orig: AENG_get_rid_of_unused_dfcache_lighting (fallen/DDEngine/Source/aeng.cpp)
// Walks the NIGHT dfcache linked list and frees cached per-facet lighting for
// any facet that was not drawn this frame. In splitscreen mode, a facet is
// kept alive if it was drawn by either camera.
void AENG_get_rid_of_unused_dfcache_lighting(SLONG splitscreen) // Splitscreen = UC_TRUE or UC_FALSE
{
    SLONG dfcache;
    SLONG next;

    NIGHT_Dfcache* ndf;

    for (dfcache = NIGHT_dfcache_used; dfcache; dfcache = next) {
        ASSERT(WITHIN(dfcache, 1, NIGHT_MAX_DFCACHES - 1));

        ndf = &NIGHT_dfcache[dfcache];
        next = ndf->next;

        // Was this facet drawn this gameturn? If not, free its cached lighting.
        ASSERT(WITHIN(ndf->dfacet, 1, next_dfacet - 1));
        ASSERT(dfacets[ndf->dfacet].Dfcache == dfcache);

        if (dfacets[ndf->dfacet].Counter[0] != SUPERMAP_counter[0]) {
            if (splitscreen) {
                // Might have been drawn from the second camera.
                if (dfacets[ndf->dfacet].Counter[1] == SUPERMAP_counter[1]) {
                    // It was drawn from the second camera — don't free it.
                    continue;
                }
            }

            // Free the cached lighting for this facet.
            dfacets[ndf->dfacet].Dfcache = 0;

            NIGHT_dfcache_destroy(dfcache);
        }
    }
}

// uc_orig: AENG_draw_inside_floor (fallen/DDEngine/Source/aeng.cpp)
// Renders the floor tiles visible through the indoor overlay for the given
// indoor section and room. Each tile is textured with the indoor floor texture
// and optionally lit from the cached NIGHT lighting grid.
void AENG_draw_inside_floor(UWORD inside_index, UWORD inside_room, UBYTE fade)
{
    SLONG x, z;
    SLONG page;

    float world_x;
    float floor_y;
    float world_z;

    POLY_Point pp[4];

    POLY_Point* quad[4];

    struct InsideStorey* p_inside;
    SLONG in_width;
    UBYTE* in_block;
    SLONG min_z, max_z;
    SLONG c0;
    SLONG floor_type;
    SLONG do_light;

    if (inside_index == light_inside)
        do_light = 1;
    else
        do_light = 0;

    // Draw the internal walls.
    extern void draw_insides(SLONG indoor_index, SLONG room, UBYTE fade);
    draw_insides(inside_index, inside_room, fade);

    p_inside = &inside_storeys[inside_index];

    floor_type = p_inside->TexType;

    floor_y = (float)p_inside->StoreyY;

    min_z = MAX(NGAMUT_point_zmin, p_inside->MinZ);
    max_z = MIN(NGAMUT_point_zmax, p_inside->MaxZ);

    in_width = p_inside->MaxX - p_inside->MinX;

    in_block = &inside_block[p_inside->InsideBlock];

    for (c0 = 0; c0 < 4; c0++) {
        pp[c0].colour = 0xffffff;
        pp[c0].specular = 0xff000000;
    }

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    quad[0]->u = 0.0;
    quad[0]->v = 0.0;
    quad[1]->u = 1.0;
    quad[1]->v = 0.0;
    quad[2]->u = 0.0;
    quad[2]->v = 1.0;
    quad[3]->u = 1.0;
    quad[3]->v = 1.0;

    for (z = min_z; z < max_z; z++) {
        SLONG min_x, max_x;
        float face_y;
        SLONG col;
        min_x = MAX(NGAMUT_point_gamut[z].xmin, p_inside->MinX);
        max_x = MIN(NGAMUT_point_gamut[z].xmax, p_inside->MaxX);

        for (x = min_x; x < max_x; x++) {
            ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
            ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

            if ((PAP_2HI(x, z).Flags & (PAP_FLAG_HIDDEN))) {
                SLONG room_id;
                SLONG px, pz, square;
                NIGHT_Square* nq;

                room_id = in_block[(x - p_inside->MinX) + (z - p_inside->MinZ) * in_width] & (0xf | 0x80 | 0x40);
                if (!(room_id & 0xc0)) {
                    face_y = floor_y;
                    col = 0xffffff;

                    col = col | ((fade & 255) << 24);
                    if (room_id) {

                        world_x = x * 256.0F;
                        world_z = z * 256.0F;
                        POLY_transform(world_x, face_y, world_z, &pp[0]);

                        if (do_light) {
                            px = x >> 2;
                            pz = z >> 2;

                            ASSERT(WITHIN(px, 0, PAP_SIZE_LO - 1));
                            ASSERT(WITHIN(pz, 0, PAP_SIZE_LO - 1));

                            square = NIGHT_cache[px][pz];
                            ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
                            ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);
                            if (!square)
                                return;
                            nq = &NIGHT_square[square];

                            NIGHT_get_colour(
                                nq->colour[(x & 3) + (z & 3) * PAP_BLOCKS],
                                &pp->colour,
                                &pp->specular);
                            pp->colour |= fade << 24;
                        } else {
                            pp->colour = 0x7f7f7f | (fade << 24);
                            pp->specular = 0xff000000;
                        }

                        world_x = (x + 1) * 256.0F;
                        world_z = z * 256.0F;
                        POLY_transform(world_x, face_y, world_z, &pp[1]);

                        if (do_light) {
                            if ((x + 1) >> 2 != px) {
                                px = (x + 1) >> 2;

                                ASSERT(WITHIN(px, 0, PAP_SIZE_LO - 1));
                                ASSERT(WITHIN(pz, 0, PAP_SIZE_LO - 1));

                                square = NIGHT_cache[px][pz];
                                ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
                                ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);
                                if (!square)
                                    return;
                                nq = &NIGHT_square[square];
                            }
                            NIGHT_get_colour(
                                nq->colour[((x + 1) & 3) + (z & 3) * PAP_BLOCKS],
                                &(pp[1].colour),
                                &(pp[1].specular));

                            pp[1].colour |= fade << 24;
                        } else {
                            pp[1].colour = 0x7f7f7f | (fade << 24);
                            pp[1].specular = 0xff000000;
                        }

                        world_x = x * 256.0F;
                        world_z = (z + 1) * 256.0F;
                        POLY_transform(world_x, face_y, world_z, &pp[2]);

                        if (do_light) {
                            if ((z + 1) >> 2 != pz || (x >> 2) != px) {
                                px = x >> 2;
                                pz = (z + 1) >> 2;

                                ASSERT(WITHIN(px, 0, PAP_SIZE_LO - 1));
                                ASSERT(WITHIN(pz, 0, PAP_SIZE_LO - 1));

                                square = NIGHT_cache[px][pz];
                                ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
                                ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);
                                if (!square)
                                    return;
                                nq = &NIGHT_square[square];
                            }

                            NIGHT_get_colour(
                                nq->colour[(x & 3) + ((z + 1) & 3) * PAP_BLOCKS],
                                &(pp[2].colour),
                                &(pp[2].specular));
                            pp[2].colour |= fade << 24;
                        } else {
                            pp[2].colour = 0x7f7f7f | (fade << 24);
                            pp[2].specular = 0xff000000;
                        }

                        world_x = (x + 1) * 256.0F;
                        world_z = (z + 1) * 256.0F;
                        POLY_transform(world_x, face_y, world_z, &pp[3]);

                        if (do_light) {
                            if ((x + 1) >> 2 != px || (z + 1) >> 2 != pz) {
                                px = (x + 1) >> 2;
                                pz = (z + 1) >> 2;

                                ASSERT(WITHIN(px, 0, PAP_SIZE_LO - 1));
                                ASSERT(WITHIN(pz, 0, PAP_SIZE_LO - 1));

                                square = NIGHT_cache[px][pz];
                                if (!square)
                                    return;
                                ASSERT(WITHIN(square, 1, NIGHT_MAX_SQUARES - 1));
                                ASSERT(NIGHT_square[square].flag & NIGHT_SQUARE_FLAG_USED);
                                nq = &NIGHT_square[square];
                            }

                            NIGHT_get_colour(
                                nq->colour[((x + 1) & 3) + ((z + 1) & 3) * PAP_BLOCKS],
                                &(pp[3].colour),
                                &(pp[3].specular));
                            pp[3].colour |= fade << 24;
                        } else {
                            pp[3].colour = 0x7f7f7f | (fade << 24);
                            pp[3].specular = 0xff000000;
                        }

                        if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid() && pp[3].MaybeValid()) {
                            pp[0].colour = col;
                            pp[1].colour = col;
                            pp[2].colour = col;
                            pp[3].colour = col;
                            page = inside_tex[floor_type][room_id - 1] + START_PAGE_FOR_FLOOR * 64; // temp
                            POLY_add_quad(quad, page, UC_FALSE);
                        }
                    }
                }
            }
        }
    }
}
