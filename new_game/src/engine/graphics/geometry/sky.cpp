// Must come before gd_display.h: game.h -> MFStdLib.h declares extern DisplayWidth/Height,
// while gd_display.h redefines them as macros (#define 640/480). Wrong order causes syntax errors.
#include "game/game_types.h"

#include "engine/platform/uc_common.h"
#include "engine/graphics/geometry/sky.h"
#include "engine/graphics/geometry/sky_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/graphics_api/gd_display.h"
#include "engine/core/matrix.h"
#include <math.h>

// uc_orig: SKY_STAR_T_DIM (fallen/DDEngine/Source/sky.cpp)
#define SKY_STAR_T_DIM 1
// uc_orig: SKY_STAR_T_MEDIUM (fallen/DDEngine/Source/sky.cpp)
#define SKY_STAR_T_MEDIUM 2
// uc_orig: SKY_STAR_T_BRIGHT (fallen/DDEngine/Source/sky.cpp)
#define SKY_STAR_T_BRIGHT 3
// uc_orig: SKY_STAR_T_PLANET (fallen/DDEngine/Source/sky.cpp)
#define SKY_STAR_T_PLANET 4

// Wibble constants for moon reflection distortion (lookup table indices).
// uc_orig: SKY_wibble_y1 (fallen/DDEngine/Source/sky.cpp)
#define SKY_wibble_y1 62
// uc_orig: SKY_wibble_y2 (fallen/DDEngine/Source/sky.cpp)
#define SKY_wibble_y2 137
// uc_orig: SKY_wibble_g1 (fallen/DDEngine/Source/sky.cpp)
#define SKY_wibble_g1 17
// uc_orig: SKY_wibble_g2 (fallen/DDEngine/Source/sky.cpp)
#define SKY_wibble_g2 78
// uc_orig: SKY_wibble_s1 (fallen/DDEngine/Source/sky.cpp)
#define SKY_wibble_s1 40
// uc_orig: SKY_wibble_s2 (fallen/DDEngine/Source/sky.cpp)
#define SKY_wibble_s2 45

// uc_orig: SKY_init (fallen/DDEngine/Source/sky.cpp)
void SKY_init(CBYTE* star_file)
{
    SLONG i;

    float twidth;
    float theight;

    SLONG yaw;
    SLONG pitch;
    SLONG bright;
    SLONG match;

    FILE* handle;

    CBYTE line[128];

    SKY_Cloud* sc;
    SKY_Texture* st;

    // Create all the clouds.
    for (i = 0; i < SKY_NUM_CLOUDS; i++) {
        sc = &SKY_cloud[i];

        sc->texture = rand() % SKY_NUM_TEXTURES;
        sc->flip = rand() & 0x8;
        sc->yaw = (float(rand()) / float(RAND_MAX)) * (2.0F * PI);
        sc->pitch = (float(rand()) / float(RAND_MAX)) * (PI / 3.0F) + (PI / 64.0F);
        sc->dyaw = (float(rand()) / float(RAND_MAX)) * 0.0005F + 0.0001F;

        ASSERT(WITHIN(sc->texture, 0, SKY_NUM_TEXTURES - 1));

        st = &SKY_texture[sc->texture];

        twidth = (st->u2 - st->u1) * 256.0F;
        theight = (st->v2 - st->v1) * 256.0F;

        // Randomise the height and width of the cloud, but always make the
        // texels more than one pixel so we get the benefit of filtering.
        twidth *= 0.3F + (float(rand()) * 0.5F / float(RAND_MAX));
        theight *= 0.3F + (float(rand()) * 0.5F / float(RAND_MAX));

        sc->width = UBYTE(twidth);
        sc->height = UBYTE(theight);
    }

    // Place the stars.
    if (star_file == NULL) {
        handle = NULL;
    } else {
        handle = MF_Fopen(star_file, "rb");
    }

    if (handle == NULL) {
        // Randomly generate the stars.
        for (i = 0; i < SKY_MAX_STARS; i++) {
            yaw = rand() % 360;
            pitch = rand() % 60;
            pitch += 10;

            bright = rand() % (pitch + 0x3f);
            bright += 0x1f;

            SKY_star[i].colour = bright;
            SKY_star[i].spread = (bright < 100) ? 0 : bright >> 2;
            SKY_star[i].yaw = float(yaw) * 2.0F * PI / 360.0F;
            SKY_star[i].pitch = float(pitch) * 2.0F * PI / 360.0F;
        }

        SKY_star_upto = SKY_MAX_STARS;
    } else {
        SKY_star_upto = 0;

        while (fgets(line, 128, handle)) {
            if (SKY_star_upto >= SKY_MAX_STARS) {
                // Can't read in any more stars.
                break;
            }

            match = sscanf(line, "Star: %d, %d, %d", &yaw, &pitch, &bright);

            if (match == 3) {
                // Make sure that the brightness isn't out of range.
                SATURATE(bright, 0, 255);

                SKY_star[SKY_star_upto].colour = bright;
                SKY_star[SKY_star_upto].spread = (bright < 100) ? 0 : bright >> 2;
                SKY_star[SKY_star_upto].yaw = float(yaw) * 2.0F * PI / 2048.0F;
                SKY_star[SKY_star_upto].pitch = float(pitch) * 2.0F * PI / 2048.0F;

                SKY_star_upto += 1;
            }
        }

        MF_Fclose(handle);
    }

    // London, England: apply a latitude tilt to star positions.
    SKY_Star* ss;

    float dpitch = 39.0F * 2.0F * PI / 360.0F;

    for (i = 0; i < SKY_star_upto; i++) {
        ss = &SKY_star[i];

        MATRIX_vector(
            ss->vector,
            ss->yaw,
            ss->pitch + dpitch);
    }
}

// uc_orig: SKY_draw_stars (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_stars(
    float mid_x,
    float mid_y,
    float mid_z,
    float max_dist)
{
    SLONG i;

    SVector_F temp;
    POLY_Point pp;

    SKY_Star* ss;

    float xmul = float(RealDisplayWidth) / float(DisplayWidth);
    float ymul = float(RealDisplayHeight) / float(DisplayHeight);

    for (i = 0; i < SKY_star_upto; i++) {
        ss = &SKY_star[i];

// Place the star at a distance just inside the far clip plane.
// uc_orig: SKY_STAR_DIST (fallen/DDEngine/Source/sky.cpp)
#define SKY_STAR_DIST (max_dist - 256.0F)

        temp.X = ss->vector[0] * SKY_STAR_DIST + mid_x;
        temp.Y = ss->vector[1] * SKY_STAR_DIST + (mid_y * 0.5F);
        temp.Z = ss->vector[2] * SKY_STAR_DIST + mid_z;

        POLY_transform(
            temp.X,
            temp.Y,
            temp.Z,
            &pp);

        if (!(pp.clip & (POLY_CLIP_LEFT | POLY_CLIP_RIGHT | POLY_CLIP_TOP | POLY_CLIP_BOTTOM | POLY_CLIP_NEAR | POLY_CLIP_FAR))) {
            SLONG px = SLONG(pp.X * xmul);
            SLONG py = SLONG(pp.Y * ymul);

            if ((rand() & 0x7f) == (i & 0x7f)) {
                // Make the star twinkle — skip drawing this frame.
            } else {
                the_display.PlotPixel(
                    px, py,
                    ss->colour,
                    ss->colour,
                    ss->colour);

                if (ss->spread) {
                    ULONG col = the_display.GetFormattedPixel(ss->spread, ss->spread, ss->spread);

                    the_display.PlotFormattedPixel(px - 1, py, col);
                    the_display.PlotFormattedPixel(px + 1, py, col);
                    the_display.PlotFormattedPixel(px, py - 1, col);
                    the_display.PlotFormattedPixel(px, py + 1, col);
                }
            }
        }
    }
}

// uc_orig: SKY_draw_poly_clouds (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_clouds(
    float mid_x,
    float mid_y,
    float mid_z,
    float max_dist)
{
    SLONG i;
    SLONG j;

    float yaw;
    float pitch;
    float vector[3];

    SVector_F temp;

    float screen_width = float(DisplayWidth);
    float screen_height = float(DisplayHeight);

    float width;
    float height;

    SKY_Cloud* sc;

    POLY_Point mid;
    POLY_Point pp[4];
    POLY_Point* quad[4];

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

// Distance for cloud placement: just inside the far clip plane.
// uc_orig: SKY_CLOUD_DIST (fallen/DDEngine/Source/sky.cpp)
#define SKY_CLOUD_DIST (max_dist - 512.0F)

    for (i = 0; i < SKY_NUM_CLOUDS; i++) {
        sc = &SKY_cloud[i];

        // Animate: rotate cloud around the sky sphere.
        sc->yaw += sc->dyaw;

        yaw = sc->yaw;
        pitch = sc->pitch;

        MATRIX_vector(
            vector,
            yaw,
            pitch);

        temp.X = vector[0] * SKY_CLOUD_DIST + mid_x;
        temp.Y = vector[1] * SKY_CLOUD_DIST + mid_y * 0.5F;
        temp.Z = vector[2] * SKY_CLOUD_DIST + mid_z;

        POLY_transform(
            temp.X,
            temp.Y,
            temp.Z,
            &mid);

        if (!mid.IsValid()) {
            continue;
        }

        width = float(sc->width);
        height = float(sc->height);

        if (mid.X + width < 0 || mid.X - width > screen_width || mid.Y + height < 0 || mid.Y - width > screen_height) {
            continue;
        }

// Place clouds at the very back of the z-buffer (behind everything).
// uc_orig: SKY_VERY_FAR_AWAY (fallen/DDEngine/Source/sky.cpp)
#define SKY_VERY_FAR_AWAY (1.0F / 65536.0F)

        mid.Z = SKY_VERY_FAR_AWAY;
        mid.colour = 0xff333333 + 0x00010101 * SLONG(sc->dyaw * (128.0F / 0.0005F));
        ;
        mid.specular = 0x000000;

        for (j = 0; j < 4; j++) {
            pp[j] = mid;

            pp[j].X += (j & 1) ? width : -width;
            pp[j].Y += (j >> 1) ? height : -height;
        }

        // The sky texture.
        SKY_Texture* st;

        ASSERT(WITHIN(sc->texture, 0, SKY_NUM_TEXTURES - 1));

        st = &SKY_texture[sc->texture];

        if (sc->flip) {
            pp[0].u = st->u1;
            pp[0].v = st->v1;
            pp[1].u = st->u2;
            pp[1].v = st->v1;
            pp[2].u = st->u1;
            pp[2].v = st->v2;
            pp[3].u = st->u2;
            pp[3].v = st->v2;
        } else {
            pp[0].u = st->u2;
            pp[0].v = st->v1;
            pp[1].u = st->u1;
            pp[1].v = st->v1;
            pp[2].u = st->u2;
            pp[2].v = st->v2;
            pp[3].u = st->u1;
            pp[3].v = st->v2;
        }

        POLY_add_quad(quad, POLY_PAGE_CLOUDS, UC_FALSE, UC_TRUE);
    }

    return;
}

// uc_orig: SKY_draw_poly_moon (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_moon(
    float mid_x,
    float mid_y,
    float mid_z,
    float max_dist)
{
    SLONG j;

    float yaw;
    float pitch;
    float vector[3];

    SVector_F temp;

    float screen_width = float(DisplayWidth);
    float screen_height = float(DisplayHeight);

    POLY_Point mid;
    POLY_Point pp[4];
    POLY_Point* quad[4];

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

// Fixed moon position in the sky.
// uc_orig: SKY_MOON_YAW (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_YAW (0)
// uc_orig: SKY_MOON_PITCH (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_PITCH (PI / 8.0F)
// uc_orig: SKY_MOON_DIST (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_DIST (max_dist - 128.0F)
// Moon appears large on screen.
// uc_orig: SKY_MOON_RADIUS (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_RADIUS (screen_width * 0.15F)
// uc_orig: SKY_MOON_UV_IN (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_UV_IN (0.02F)

    const struct {
        float u;
        float v;
    } moon_uv[4] = {
        { SKY_MOON_UV_IN, SKY_MOON_UV_IN },
        { 1.0F - SKY_MOON_UV_IN, SKY_MOON_UV_IN },
        { SKY_MOON_UV_IN, 1.0F - SKY_MOON_UV_IN },
        { 1.0F - SKY_MOON_UV_IN, 1.0F - SKY_MOON_UV_IN }
    };

    yaw = SKY_MOON_YAW;
    pitch = SKY_MOON_PITCH;

    MATRIX_vector(
        vector,
        yaw,
        pitch);

    temp.X = vector[0] * SKY_MOON_DIST + mid_x;
    temp.Y = vector[1] * SKY_MOON_DIST + mid_y * 0.5f;
    temp.Z = vector[2] * SKY_MOON_DIST + mid_z;

    POLY_transform(
        temp.X,
        temp.Y,
        temp.Z,
        &mid);

    if (mid.IsValid()) {
        if (!(mid.X + SKY_MOON_RADIUS < 0 || mid.X - SKY_MOON_RADIUS > screen_width || mid.Y + SKY_MOON_RADIUS < 0 || mid.Y - SKY_MOON_RADIUS > screen_height)) {
            mid.Z = SKY_VERY_FAR_AWAY;
            mid.colour = 0xffaaaa88;
            mid.specular = 0x00000000;

            for (j = 0; j < 4; j++) {
                pp[j] = mid;

                pp[j].X += (j & 1) ? SKY_MOON_RADIUS : -SKY_MOON_RADIUS;
                pp[j].Y += (j >> 1) ? SKY_MOON_RADIUS : -SKY_MOON_RADIUS;

                pp[j].u = moon_uv[j].u;
                pp[j].v = moon_uv[j].v;
            }

            POLY_add_quad(quad, POLY_PAGE_MOON, UC_FALSE, UC_TRUE);
        }
    }

    return;
}

// uc_orig: SKY_draw_moon_reflection (fallen/DDEngine/Source/sky.cpp)
SLONG SKY_draw_moon_reflection(
    float mid_x,
    float mid_y,
    float mid_z,
    float max_dist,
    float* moon_x1,
    float* moon_y1,
    float* moon_x2,
    float* moon_y2)
{
    SLONG i;
    SLONG j;

    float y;
    float v;

    float yaw;
    float pitch;
    float vector[3];

    SVector_F temp;

    float screen_width = float(DisplayWidth);
    float screen_height = float(DisplayHeight);

    POLY_Point mid;
    POLY_Point pp[4];
    POLY_Point* quad[4];

    SLONG angle1;
    SLONG angle2;
    SLONG offset1;
    SLONG offset2;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    const struct {
        float u;
        float v;
    } moon_uv[4] = {
        { SKY_MOON_UV_IN, SKY_MOON_UV_IN },
        { 1.0F - SKY_MOON_UV_IN, SKY_MOON_UV_IN },
        { SKY_MOON_UV_IN, 1.0F - SKY_MOON_UV_IN },
        { 1.0F - SKY_MOON_UV_IN, 1.0F - SKY_MOON_UV_IN }
    };

    yaw = SKY_MOON_YAW;
    pitch = SKY_MOON_PITCH;

    MATRIX_vector(
        vector,
        yaw,
        pitch);

    temp.X = vector[0] * SKY_MOON_DIST + mid_x;
    // Reflection: negate Y to flip the moon below the waterline.
    temp.Y = -vector[1] * SKY_MOON_DIST + mid_y * 0.5f;
    temp.Z = vector[2] * SKY_MOON_DIST + mid_z;

    POLY_transform(
        temp.X,
        temp.Y,
        temp.Z,
        &mid);

    if (!mid.IsValid()) {
        return UC_FALSE;
    } else {
        if (mid.X + SKY_MOON_RADIUS < 8 || mid.X - SKY_MOON_RADIUS > screen_width + 8 || mid.Y + SKY_MOON_RADIUS < 8 || mid.Y - SKY_MOON_RADIUS > screen_height + 8) {
            return UC_FALSE;
        } else {
            mid.Z = SKY_VERY_FAR_AWAY;
            mid.colour = 0xffaaaa88;
            mid.specular = 0x00000000;

// Number of horizontal slices the moon reflection is divided into (for the wibble effect).
// uc_orig: SKY_MOON_SEGMENTS (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_SEGMENTS 16

            y = mid.Y - SKY_MOON_RADIUS;
            v = moon_uv[0].v;

            // Calculate the first line's wibble offset.
            angle1 = SLONG(y) * SKY_wibble_y1;
            angle2 = SLONG(y) * SKY_wibble_y2;
            angle1 += GAME_TURN * SKY_wibble_g1;
            angle2 += GAME_TURN * SKY_wibble_g2;

            angle1 &= 2047;
            angle2 &= 2047;

            offset2 = SIN(angle1) * SKY_wibble_s1 >> 19;
            offset2 += COS(angle2) * SKY_wibble_s2 >> 19;

// Y and V step per segment.
// uc_orig: SKY_MOON_SEG_DY (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_SEG_DY (float(SKY_MOON_RADIUS) / float(SKY_MOON_SEGMENTS))
// uc_orig: SKY_MOON_SEG_DV (fallen/DDEngine/Source/sky.cpp)
#define SKY_MOON_SEG_DV ((moon_uv[2].v - moon_uv[0].v) / float(SKY_MOON_SEGMENTS))

            for (i = 0; i < SKY_MOON_SEGMENTS; i++) {
                offset1 = offset2;

                // Calculate the next line's wibble offset.
                angle1 = SLONG(y + SKY_MOON_SEG_DY) * SKY_wibble_y1;
                angle2 = SLONG(y + SKY_MOON_SEG_DY) * SKY_wibble_y2;
                angle1 += GAME_TURN * SKY_wibble_g1;
                angle2 += GAME_TURN * SKY_wibble_g2;

                angle1 &= 2047;
                angle2 &= 2047;

                offset2 = SIN(angle1) * SKY_wibble_s1 >> 19;
                offset2 += COS(angle2) * SKY_wibble_s2 >> 19;

                for (j = 0; j < 4; j++) {
                    pp[j] = mid;

                    pp[j].X += (j & 1) ? SKY_MOON_RADIUS : -SKY_MOON_RADIUS;
                    pp[j].u = moon_uv[j].u;

                    if (j & 2) {
                        pp[j].Y = y + SKY_MOON_SEG_DY;
                        pp[j].v = v + SKY_MOON_SEG_DV;
                        pp[j].X += offset2;
                    } else {
                        pp[j].Y = y;
                        pp[j].v = v;
                        pp[j].X += offset1;
                    }
                }

                POLY_add_quad(quad, POLY_PAGE_MOON, UC_FALSE, UC_TRUE);

                if (i == 0) {
                    *moon_x1 = pp[0].X;
                    *moon_y1 = pp[2].Y;
                }

                if (i == SKY_MOON_SEGMENTS - 1) {
                    *moon_x2 = pp[1].X;
                    *moon_y2 = pp[0].Y;
                }

                y += SKY_MOON_SEG_DY;
                v += SKY_MOON_SEG_DV;
            }
        }
    }

    return UC_TRUE;
}

// uc_orig: SKY_draw_poly_sky (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_sky(
    float world_camera_x,
    float world_camera_y,
    float world_camera_z,
    float world_camera_yaw,
    float max_dist,
    ULONG bot_colour,
    ULONG top_colour)
{
    float px;
    float py;
    float pz;

    float screen_x;
    float screen_y;

    float vector[3];

    MATRIX_vector(
        vector,
        world_camera_yaw,
        0.0F);

    px = world_camera_x + vector[0] * max_dist;
    py = -256.0F;
    pz = world_camera_z + vector[2] * max_dist;

    if (!POLY_get_screen_pos(
            px, py, pz,
            &screen_x,
            &screen_y)) {
        // Can't see the horizon.
        return;
    }

    POLY_Point pp[4];
    POLY_Point* quad[4];

    // Bottom gradient band (bot_colour).
    pp[0].X = 0.0F;
    pp[0].Y = screen_y - 256.0F;
    pp[0].Z = 0.5F;
    pp[0].z = 0.5F;
    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[0].colour = bot_colour;
    pp[0].specular = 0xff000000;

    pp[1].X = 640.0F;
    pp[1].Y = screen_y - 256.0F;
    pp[1].Z = 0.5F;
    pp[1].z = 0.5F;
    pp[1].u = 0.0F;
    pp[1].v = 0.0F;
    pp[1].colour = bot_colour;
    pp[1].specular = 0xff000000;

    pp[2].X = 0.0F;
    pp[2].Y = screen_y;
    pp[2].Z = 0.5F;
    pp[2].z = 0.5F;
    pp[2].u = 0.0F;
    pp[2].v = 0.0F;
    pp[2].colour = bot_colour;
    pp[2].specular = 0xff000000;

    pp[3].X = 640.0F;
    pp[3].Y = screen_y;
    pp[3].Z = 0.5F;
    pp[3].z = 0.5F;
    pp[3].u = 0.0F;
    pp[3].v = 0.0F;
    pp[3].colour = bot_colour;
    pp[3].specular = 0xff000000;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    POLY_add_quad(quad, POLY_PAGE_SKY, UC_FALSE, UC_TRUE);

    // Top gradient band (fades from bot_colour to top_colour).
    pp[0].X = 0.0F;
    pp[0].Y = screen_y - 1024.0F;
    pp[0].Z = 0.5F;
    pp[0].z = 0.5F;
    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[0].colour = top_colour;
    pp[0].specular = 0xff000000;

    pp[1].X = 640.0F;
    pp[1].Y = screen_y - 1024.0F;
    pp[1].Z = 0.5F;
    pp[1].z = 0.5F;
    pp[1].u = 0.0F;
    pp[1].v = 0.0F;
    pp[1].colour = top_colour;
    pp[1].specular = 0xff000000;

    pp[2].X = 0.0F;
    pp[2].Y = screen_y - 256.0F;
    pp[2].Z = 0.5F;
    pp[2].z = 0.5F;
    pp[2].u = 0.0F;
    pp[2].v = 0.0F;
    pp[2].colour = bot_colour;
    pp[2].specular = 0xff000000;

    pp[3].X = 640.0F;
    pp[3].Y = screen_y - 256.0F;
    pp[3].Z = 0.5F;
    pp[3].z = 0.5F;
    pp[3].u = 0.0F;
    pp[3].v = 0.0F;
    pp[3].colour = bot_colour;
    pp[3].specular = 0xff000000;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    POLY_add_quad(quad, POLY_PAGE_SKY, UC_FALSE, UC_TRUE);
}

//  0  1
//
//  2  3

// uc_orig: SKY_draw_poly_sky_old (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_sky_old(float world_camera_x, float world_camera_y, float world_camera_z, float world_camera_yaw, float max_dist, ULONG bot_colour, ULONG top_colour)
{
    SLONG i;

    SLONG p1;
    SLONG p2;

    float x;
    float z;

    float angle;

// Number of steps in the horizon circle.
// uc_orig: SKY_CIRCLE_STEPS (fallen/DDEngine/Source/sky.cpp)
#define SKY_CIRCLE_STEPS 30

// uc_orig: SKY_HORIZON (fallen/DDEngine/Source/sky.cpp)
#define SKY_HORIZON 0.0F
// uc_orig: SKY_MAXUP (fallen/DDEngine/Source/sky.cpp)
#define SKY_MAXUP (world_camera_y * 0.0f + 12072.0F)

    POLY_Point pp_bot[SKY_CIRCLE_STEPS];
    POLY_Point pp_top[SKY_CIRCLE_STEPS];

    POLY_Point* quad[4];

    angle = 0.0F;
    max_dist = 66.0F * 256.0F;

    for (i = 0; i < SKY_CIRCLE_STEPS; i++) {
        x = world_camera_x + (float)sin(angle) * (max_dist);
        z = world_camera_z + (float)cos(angle) * (max_dist);

        POLY_transform_c_saturate_z(x, SKY_HORIZON, z, &pp_bot[i]);

        // Only bother transforming the higher point if the lower point
        // wasn't behind you.
        if (pp_bot[i].IsValid()) {
            pp_bot[i].colour = bot_colour;
            pp_bot[i].specular = 0xff000000;

            x = world_camera_x + (float)sin(angle) * (max_dist);
            z = world_camera_z + (float)cos(angle) * (max_dist);

            POLY_transform_c_saturate_z(
                x,
                SKY_MAXUP,
                z,
                &pp_top[i]);

            pp_top[i].colour = top_colour;
            pp_top[i].specular = 0xff000000;
        } else
            pp_top[i].clip = 0;

        angle += 2.0F * PI / SKY_CIRCLE_STEPS;
    }

    // Draw the sky quads.
    for (i = 0; i < SKY_CIRCLE_STEPS; i++) {
        p1 = i + 0;
        p2 = i + 1;

        if (p2 == SKY_CIRCLE_STEPS) {
            p2 = 0;
        }

        quad[0] = &pp_top[p1];
        quad[1] = &pp_top[p2];
        quad[2] = &pp_bot[p1];
        quad[3] = &pp_bot[p2];
        if ((quad[0]->clip & quad[1]->clip & quad[1]->clip & quad[2]->clip) & POLY_CLIP_TRANSFORMED)
            if (POLY_valid_quad(quad)) {
                float pos;

                switch (i % 5) {
                case 0:
                    pos = 0.0F;
                    break;
                case 1:
                    pos = 0.2F;
                    break;
                case 2:
                    pos = 0.4F;
                    break;
                case 3:
                    pos = 0.6F;
                    break;
                case 4:
                    pos = 0.8F;
                    break;
                }

                quad[0]->u = pos;
                quad[1]->u = pos + 0.2F;
                quad[2]->u = pos;
                quad[3]->u = pos + 0.2F;

                quad[0]->v = 0.0F;
                quad[1]->v = 0.0F;
                quad[2]->v = 1.0F;
                quad[3]->v = 1.0F;

                POLY_add_quad(quad, POLY_PAGE_SKY, UC_FALSE, 1);
            }
    }
}
