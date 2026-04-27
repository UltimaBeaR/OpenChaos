// uc_orig: drawxtra.cpp (fallen/DDEngine/Source/drawxtra.cpp)
// Light bloom and lens flare rendering: BLOOM_flare_draw + BLOOM_draw.

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/physics/collide.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "missions/eway.h"

#include "engine/graphics/postprocess/bloom.h"
#include "engine/graphics/postprocess/bloom_globals.h"
#include "engine/graphics/geometry/sprite.h" // SPRITE_draw_rotated

// AENG_cur_fc_cam and AENG_cam_vec are defined in aeng.cpp without a public header declaration.
extern SLONG AENG_cur_fc_cam;
extern SLONG AENG_cam_vec[3];

// uc_orig: BLOOM_flare_draw (fallen/DDEngine/Source/drawxtra.cpp)
void BLOOM_flare_draw(SLONG x, SLONG y, SLONG z, SLONG str)
{
    POLY_Point pt1, pt2, pt3, pt4;
    SLONG dx, dy, fx, fy, cx, cy;
    SLONG i, sz, j;
    SLONG scale;
    POLY_Point* quad[4];
    SLONG fc_x, fc_y, fc_z, dummy;

    if (!EWAY_grab_camera(
            &fc_x, &fc_y, &fc_z,
            &dummy, &dummy, &dummy, &dummy)) {
        fc_x = FC_cam[AENG_cur_fc_cam].x;
        fc_y = FC_cam[AENG_cur_fc_cam].y;
        fc_z = FC_cam[AENG_cur_fc_cam].z;
    }

    if (!there_is_a_los(x, y, z,
            fc_x >> 8, fc_y >> 8, fc_z >> 8,
            LOS_FLAG_IGNORE_PRIMS))
        return;

    POLY_transform(x, y, z, &pt1);

    if ((pt1.X < 0) || (pt1.X > POLY_screen_width) || (pt1.Y < 0) || (pt1.Y > POLY_screen_height))
        return;

    cx = ge_get_screen_width() >> 1;
    cy = ge_get_screen_height() >> 1;
    dx = pt1.X - cx;
    dy = pt1.Y - cy;

    dx <<= 4;
    dy <<= 4;

    pt1.z = 0.01F;
    pt1.Z = 1.00F;

    pt2 = pt3 = pt4 = pt1;
    pt1.u = 0;
    pt1.v = 0;
    pt2.u = 1;
    pt2.v = 0;
    pt3.u = 0;
    pt3.v = 1;
    pt4.u = 1;
    pt4.v = 1;

    quad[0] = &pt1;
    quad[1] = &pt2;
    quad[2] = &pt3;
    quad[3] = &pt4;

    // Darken the flare as the light source gets further from the camera.
    {
        SLONG ddx = abs(x - (fc_x >> 8));
        SLONG ddy = abs(y - (fc_y >> 8));
        SLONG ddz = abs(z - (fc_z >> 8));

        SLONG dist = QDIST3(ddx, ddy, ddz);

        scale = 256 - (dist / 16);

        if (scale < 0)
            return;

        SATURATE(scale, 0, 256);

        scale *= str;
        scale >>= 8;
    }

    if (!scale)
        return;

    for (i = -12; i < 15; i++)
        if (i) {
            j = abs(i >> 2);
            j *= i;
            fx = cx + ((j * dx) >> 8);
            fy = cy + ((j * dy) >> 8);
            sz = abs(abs(i) - 3) * 8;
            if (abs(i) > 7)
                sz >>= 1;
            if (abs(i) > 11)
                sz >>= 1;
            pt1.X = fx - sz;
            pt1.Y = fy - sz;
            pt2.X = fx + sz;
            pt2.Y = fy - sz;
            pt3.X = fx - sz;
            pt3.Y = fy + sz;
            pt4.X = fx + sz;
            pt4.Y = fy + sz;
            pt1.specular = pt2.specular = pt3.specular = pt4.specular = 0xff000000;

            SLONG r, g, b;

            r = flare_table[(i >> 2) + 3][0];
            g = flare_table[(i >> 2) + 3][1];
            b = flare_table[(i >> 2) + 3][2];

            r *= scale;
            r >>= 8;
            g *= scale;
            g >>= 8;
            b *= scale;
            b >>= 8;

            pt1.colour = pt2.colour = pt3.colour = pt4.colour = r | (g << 8) | (b << 16);

            if (POLY_valid_quad(quad))
                POLY_add_quad(quad, POLY_PAGE_LENSFLARE, UC_FALSE, true);
        }
}

// uc_orig: BLOOM_draw (fallen/DDEngine/Source/drawxtra.cpp)
void BLOOM_draw(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, SLONG col, UBYTE opts)
{
    SLONG a, b, c, dot;
    SLONG rgba, sz;
    POLY_Point pt1, pt2;
    SLONG fc_x, fc_y, fc_z, dummy;

    POLY_flush_local_rot();

    if (!EWAY_grab_camera(
            &fc_x, &fc_y, &fc_z,
            &dummy, &dummy, &dummy, &dummy)) {
        fc_x = FC_cam[AENG_cur_fc_cam].x;
        fc_y = FC_cam[AENG_cur_fc_cam].y;
        fc_z = FC_cam[AENG_cur_fc_cam].z;
    }

    // Check line-of-sight against walls; optionally raise the test point by 16 units.
    SLONG losy = (opts & BLOOM_RAISE_LOS) ? y + 16 : y;
    if (!there_is_a_los(x, losy, z,
            fc_x >> 8, fc_y >> 8, fc_z >> 8,
            LOS_FLAG_IGNORE_PRIMS))
        return;

    if ((!dx) && (!dy) && (!dz))
        dot = -255;
    else {
        // Dot product of the light direction and the camera's forward vector.
        a = (dx * AENG_cam_vec[0]) / 65536;
        b = (dy * AENG_cam_vec[1]) / 65536;
        c = (dz * AENG_cam_vec[2]) / 65536;
        dot = a + b + c;
    }

    sz = ((col & 0xff) + ((col & 0xff00) >> 8) + ((col & 0xff0000) >> 16)) >> 2;

    // Draw the glow bloom if the light is facing towards the camera (or GLOW_ALWAYS is set).
    if ((dot < 0) || (opts & BLOOM_GLOW_ALWAYS)) {
        rgba = abs(dot);
        rgba <<= 24;
        rgba |= col;
        SPRITE_draw_rotated((float)x, (float)y, (float)z, sz << 2, rgba, POLY_PAGE_BLOOM1, SPRITE_SORT_NORMAL, 0xffffff);

        if (opts & BLOOM_LENSFLARE) {
            rgba = abs(dot);
            rgba >>= 1;
            if (opts & BLOOM_FLENSFLARE)
                rgba >>= 1;
            BLOOM_flare_draw(x, y, z, rgba);
        }
    }

    if (opts & BLOOM_BEAM) {
        // Scale the flare beam direction by sprite size.
        dx *= sz;
        dy *= sz;
        dz *= sz;
        dx >>= 5;
        dy >>= 5;
        dz >>= 5;

        POLY_transform(x, y, z, &pt1);
        POLY_transform(x + dx, y + dy, z + dz, &pt2);

        rgba = 255 - abs(dot);
        rgba <<= 24;
        pt1.colour = col | rgba;
        pt2.colour = col;
        pt1.specular = pt2.specular = 0xFF000000;

        sz *= 3;

        if (POLY_valid_line(&pt1, &pt2))
            POLY_add_line_tex(&pt1, &pt2, sz >> 2, sz, POLY_PAGE_BLOOM2, 0);
    }
}
