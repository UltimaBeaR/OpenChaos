// Chunk 2+: remaining sewer geometry functions (wall strips, curves, cache, queries).
// Chunk 1 (init, floors, wallstrip helpers) is in new/world/environment/ns.cpp.

// Temporary: game.h provides ASSERT, WITHIN, MIN, NULL, rand, memset, and base types.
#include "game.h"
#include "world/environment/ns.h"
#include "world/environment/ns_globals.h"
#include "core/heap.h"
#include "world/map/pap.h"

void NS_cache_create_wallstrip(
    SLONG px1, SLONG pz1,
    SLONG px2, SLONG pz2,
    SLONG bot, // On a mapsquare boundary.
    SLONG ty1, SLONG ty2,
    SLONG shared,
    SLONG norm)
{
    SLONG last1;
    SLONG last2;

    SLONG now1;
    SLONG now2;

    SLONG py1;
    SLONG py2;

    SLONG p[4];

    SLONG darken_bottom;
    SLONG usenorm;

    //
    // Make sure bot is on a mapsquare boundary.
    //

    ASSERT((bot & 0x7) == 0);

    if (bot == 0) {
        bot = MIN(ty1, ty2);
        bot -= 24;
        bot &= ~7;

        darken_bottom = TRUE;
    } else {
        darken_bottom = FALSE;
    }

    //
    // The inclusive region searched to avoid creating identical points.
    //

    NS_search_start = shared;
    NS_search_end = NS_scratch_point_upto;

    //
    // Create the two bottom points.
    //

    last1 = NS_create_wallstrip_point(px1, bot, pz1, (darken_bottom) ? NS_NORM_BLACK : norm);
    last2 = NS_create_wallstrip_point(px2, bot, pz2, (darken_bottom) ? NS_NORM_BLACK : norm);

    py1 = bot + 8;
    py2 = bot + 8;

    while (1) {
        if (py1 >= ty1 || py2 >= ty2) {
            py1 = ty1;
            py2 = ty2;
        }

        //
        // Create two new points.
        //

        if (darken_bottom) {
            usenorm = NS_NORM_GREY;
            darken_bottom = FALSE;
        } else {
            usenorm = norm;
        }

        now1 = NS_create_wallstrip_point(px1, py1, pz1, usenorm);
        now2 = NS_create_wallstrip_point(px2, py2, pz2, usenorm);

        //
        // Create the face quad.
        //

        p[0] = last1;
        p[1] = last2;
        p[2] = now1;
        p[3] = now2;

        NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_FULL);

        //
        // Reached the end of the strip.
        //

        if (py1 >= ty1 || py2 >= ty2) {
            return;
        }

        py1 += 8;
        py2 += 8;

        if (py1 > 255) {
            py1 = 255;
        }
        if (py2 > 255) {
            py2 = 255;
        }

        last1 = now1;
        last2 = now2;
    }
}

//
// The extra bits of wall at the top of a sewer.
//

void NS_cache_create_extra_bit_left(
    SLONG px1, SLONG pz1,
    SLONG px2, SLONG pz2,
    SLONG bottom,
    SLONG ty1, SLONG ty2,
    SLONG norm)
{
    SLONG dx = px2 - px1 >> 8;
    SLONG dz = pz2 - pz1 >> 8;

    SLONG pindex[6];
    SLONG p[4];

    //
    // Create all the points.
    //

    pindex[0] = NS_create_wallstrip_point(px1, bottom, pz1, norm);
    pindex[1] = NS_create_wallstrip_point(px1, ty1, pz1, norm);
    pindex[2] = NS_create_wallstrip_point(px1 + dx * 128, bottom + 8, pz1 + dz * 128, norm);
    pindex[3] = NS_create_wallstrip_point(px1 + dx * 192, bottom + 3, pz1 + dz * 192, norm);
    pindex[4] = NS_create_wallstrip_point(px1 + dx * 256, bottom + 2, pz1 + dz * 256, norm);
    pindex[5] = NS_create_wallstrip_point(px1 + dx * 256, bottom, pz1 + dz * 256, norm);

    //
    // Create the two quads.
    //

    p[0] = pindex[2];
    p[1] = pindex[1];
    p[2] = pindex[3];
    p[3] = pindex[0];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_EL1);

    p[0] = pindex[4];
    p[1] = pindex[3];
    p[2] = pindex[5];
    p[3] = pindex[0];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_EL2);
}

void NS_cache_create_extra_bit_right(
    SLONG px1, SLONG pz1,
    SLONG px2, SLONG pz2,
    SLONG bottom,
    SLONG ty1, SLONG ty2,
    SLONG norm)
{
    SLONG dx = px2 - px1 >> 8;
    SLONG dz = pz2 - pz1 >> 8;

    SLONG pindex[6];
    SLONG p[4];

    //
    // Create all the points.
    //

    pindex[0] = NS_create_wallstrip_point(px2, bottom, pz2, norm);
    pindex[1] = NS_create_wallstrip_point(px2, ty2, pz2, norm);
    pindex[2] = NS_create_wallstrip_point(px2 - dx * 128, bottom + 8, pz2 - dz * 128, norm);
    pindex[3] = NS_create_wallstrip_point(px2 - dx * 192, bottom + 3, pz2 - dz * 192, norm);
    pindex[4] = NS_create_wallstrip_point(px2 - dx * 256, bottom + 2, pz2 - dz * 256, norm);
    pindex[5] = NS_create_wallstrip_point(px2 - dx * 256, bottom, pz2 - dz * 256, norm);

    //
    // Create the two quads.
    //

    p[0] = pindex[1];
    p[1] = pindex[2];
    p[2] = pindex[0];
    p[3] = pindex[3];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_ER1);

    p[0] = pindex[3];
    p[1] = pindex[4];
    p[2] = pindex[0];
    p[3] = pindex[5];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_ER2);
}

//
// Creates all the vertical walls.
//

void NS_cache_create_walls(UBYTE mx, UBYTE mz)
{
    SLONG x;
    SLONG z;

    SLONG sx;
    SLONG sz;

    SLONG px1, pz1;
    SLONG px2, pz2;
    SLONG ty1;
    SLONG ty2;
    SLONG bot;

    SLONG shared_last;
    SLONG shared_now;

    NS_Hi* nh;
    NS_Hi* nh2;

    //
    // Walls parallel to the z-axis first.
    //

    for (x = 0; x < 4; x++) {
        //
        // The strip from x to (x - 1)
        //

        shared_last = NS_scratch_point_upto - 1;

        if (shared_last < 0) {
            shared_last = 0;
        }

        for (z = 0; z < 4; z++) {
            shared_now = NS_scratch_point_upto - 1;

            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx - 1][sz];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx << 8;
                pz1 = sz << 8;

                px2 = sx << 8;
                pz2 = sz + 1 << 8;

                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx][sz + 0].top;
                    ty2 = NS_hi[sx][sz + 1].top;

                    NS_cache_create_wallstrip(
                        px1, pz1,
                        px2, pz2,
                        bot,
                        ty1, ty2,
                        shared_last,
                        NS_NORM_XL);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;

                        NS_cache_create_wallstrip(
                            px1, pz1,
                            px2, pz2,
                            bot,
                            ty1, ty2,
                            shared_last,
                            NS_NORM_XL);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            //
                            // Create the extra bits at the side of the sewer.
                            //

                            ty1 = NS_hi[sx][sz + 0].top;
                            ty2 = NS_hi[sx][sz + 1].top;

                            if (NS_HI_TYPE(&NS_hi[sx - 1][sz - 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_XL);
                            } else {
                                NS_cache_create_extra_bit_right(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_XL);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }

        //
        // The strip from x to (x + 1)
        //

        shared_last = NS_scratch_point_upto - 1;

        if (shared_last < 0) {
            shared_last = 0;
        }

        for (z = 0; z < 4; z++) {
            shared_now = NS_scratch_point_upto - 1;

            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx + 1][sz];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx + 1 << 8;
                pz1 = sz + 1 << 8;

                px2 = sx + 1 << 8;
                pz2 = sz << 8;

                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx + 1][sz + 1].top;
                    ty2 = NS_hi[sx + 1][sz + 0].top;

                    NS_cache_create_wallstrip(
                        px1, pz1,
                        px2, pz2,
                        bot,
                        ty1, ty2,
                        shared_last,
                        NS_NORM_XS);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;

                        NS_cache_create_wallstrip(
                            px1, pz1,
                            px2, pz2,
                            bot,
                            ty1, ty2,
                            shared_last,
                            NS_NORM_XS);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            //
                            // Create the extra bits at the side of the sewer.
                            //

                            ty1 = NS_hi[sx + 1][sz + 1].top;
                            ty2 = NS_hi[sx + 1][sz + 0].top;

                            if (NS_HI_TYPE(&NS_hi[sx + 1][sz + 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_XS);
                            } else {
                                NS_cache_create_extra_bit_right(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_XS);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }
    }

    //
    // Walls parallel to the x-axis now.
    //

    for (z = 0; z < 4; z++) {
        //
        // The strip from z to (z - 1)
        //

        shared_last = NS_scratch_point_upto - 1;

        if (shared_last < 0) {
            shared_last = 0;
        }

        for (x = 0; x < 4; x++) {
            shared_now = NS_scratch_point_upto - 1;

            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx][sz - 1];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx + 1 << 8;
                pz1 = sz << 8;

                px2 = sx << 8;
                pz2 = sz << 8;

                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx + 1][sz].top;
                    ty2 = NS_hi[sx + 0][sz].top;

                    NS_cache_create_wallstrip(
                        px1, pz1,
                        px2, pz2,
                        bot,
                        ty1, ty2,
                        shared_last,
                        NS_NORM_ZL);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;

                        NS_cache_create_wallstrip(
                            px1, pz1,
                            px2, pz2,
                            bot,
                            ty1, ty2,
                            shared_last,
                            NS_NORM_ZL);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            //
                            // Create the extra bits at the side of the sewer.
                            //

                            ty1 = NS_hi[sx + 1][sz].top;
                            ty2 = NS_hi[sx + 0][sz].top;

                            if (NS_HI_TYPE(&NS_hi[sx + 1][sz - 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_ZL);
                            } else {
                                NS_cache_create_extra_bit_right(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_ZL);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }

        //
        // The strip from z to (z + 1)
        //

        shared_last = NS_scratch_point_upto - 1;

        if (shared_last < 0) {
            shared_last = 0;
        }

        for (x = 0; x < 4; x++) {
            shared_now = NS_scratch_point_upto - 1;

            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx][sz + 1];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx << 8;
                pz1 = sz + 1 << 8;

                px2 = sx + 1 << 8;
                pz2 = sz + 1 << 8;

                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx + 0][sz + 1].top;
                    ty2 = NS_hi[sx + 1][sz + 1].top;

                    NS_cache_create_wallstrip(
                        px1, pz1,
                        px2, pz2,
                        bot,
                        ty1, ty2,
                        shared_last,
                        NS_NORM_ZS);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;

                        NS_cache_create_wallstrip(
                            px1, pz1,
                            px2, pz2,
                            bot,
                            ty1, ty2,
                            shared_last,
                            NS_NORM_ZS);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            //
                            // Create the extra bits at the side of the sewer.
                            //

                            ty1 = NS_hi[sx + 0][sz + 1].top;
                            ty2 = NS_hi[sx + 1][sz + 1].top;

                            if (NS_HI_TYPE(&NS_hi[sx - 1][sz + 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_ZS);
                            } else {
                                NS_cache_create_extra_bit_right(
                                    px1, pz1,
                                    px2, pz2,
                                    nh2->bot,
                                    ty1, ty2,
                                    NS_NORM_ZS);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }
    }
}

//
// Creates a segment of curved sewer wall.
//

void NS_cache_create_curve_sewer(
    SLONG sx,
    SLONG sz)
{
    SLONG px1;
    SLONG pz1;

    SLONG px2;
    SLONG pz2;

    SLONG dx;
    SLONG dz;

    SLONG dx1;
    SLONG dz1;

    SLONG dx2;
    SLONG dz2;

    SLONG px;
    SLONG py;
    SLONG pz;

    SLONG p[4];

    SLONG curve;

    NS_Hi* nh;

    UBYTE pindex[16];

    ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

    nh = &NS_hi[sx][sz];

    //
    // What type of sewer wall curve?
    //

    curve = nh->water;

    if (curve < 4) {
        //
        // Normal flat types. Set the paramters we use to create
        // the flat curve at different places and rotations.
        //

        switch (curve) {
        case NS_HI_CURVE_XS:

            px1 = sx + 1 << 8;
            pz1 = sz + 0 << 8;

            px2 = sx + 1 << 8;
            pz2 = sz + 1 << 8;

            dx = -256;
            dz = 0;

            break;

        case NS_HI_CURVE_XL:

            px1 = sx + 0 << 8;
            pz1 = sz + 1 << 8;

            px2 = sx + 0 << 8;
            pz2 = sz + 0 << 8;

            dx = +256;
            dz = 0;

            break;

        case NS_HI_CURVE_ZS:

            px1 = sx + 1 << 8;
            pz1 = sz + 1 << 8;

            px2 = sx + 0 << 8;
            pz2 = sz + 1 << 8;

            dx = 0;
            dz = -256;

            break;

        case NS_HI_CURVE_ZL:

            px1 = sx + 0 << 8;
            pz1 = sz + 0 << 8;

            px2 = sx + 1 << 8;
            pz2 = sz + 0 << 8;

            dx = 0;
            dz = +256;

            break;

        default:
            ASSERT(0);
            break;
        }

        //
        // Create the points and faces.
        //

        pindex[0] = NS_create_wallstrip_point(px1, nh->bot + 0, pz1);
        pindex[1] = NS_create_wallstrip_point(px2, nh->bot + 0, pz2);

        pindex[2] = NS_create_wallstrip_point(px1, nh->bot + 2, pz1);
        pindex[3] = NS_create_wallstrip_point(px2, nh->bot + 2, pz2);

        px1 += dx >> 2;
        pz1 += dz >> 2;

        px2 += dx >> 2;
        pz2 += dz >> 2;

        pindex[4] = NS_create_wallstrip_point(px1, nh->bot + 3, pz1);
        pindex[5] = NS_create_wallstrip_point(px2, nh->bot + 3, pz2);

        px1 += dx >> 2;
        pz1 += dz >> 2;

        px2 += dx >> 2;
        pz2 += dz >> 2;

        pindex[6] = NS_create_wallstrip_point(px1, nh->bot + 8, pz1);
        pindex[7] = NS_create_wallstrip_point(px2, nh->bot + 8, pz2);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP1);

        p[0] = pindex[2];
        p[1] = pindex[3];
        p[2] = pindex[4];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP2);

        p[0] = pindex[4];
        p[1] = pindex[5];
        p[2] = pindex[6];
        p[3] = pindex[7];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP3);
    } else if (curve < 8) {
        //
        // Inside curvey bits.
        //

        switch (curve) {
        case NS_HI_CURVE_ASS:

            px = sx + 1 << 8;
            pz = sz + 1 << 8;

            dx1 = 0;
            dz1 = -1;

            dx2 = -1;
            dz2 = 0;

            break;

        case NS_HI_CURVE_ALS:

            px = sx + 0 << 8;
            pz = sz + 1 << 8;

            dx1 = +1;
            dz1 = 0;

            dx2 = 0;
            dz2 = -1;

            break;

        case NS_HI_CURVE_ASL:

            px = sx + 1 << 8;
            pz = sz + 0 << 8;

            dx1 = -1;
            dz1 = 0;

            dx2 = 0;
            dz2 = +1;

            break;

        case NS_HI_CURVE_ALL:

            px = sx + 0 << 8;
            pz = sz + 0 << 8;

            dx1 = 0;
            dz1 = +1;

            dx2 = +1;
            dz2 = 0;

            break;

        default:
            ASSERT(0);
            break;
        }

        //
        // Create the points and faces.
        //

        pindex[0] = NS_create_wallstrip_point(px + dx1 * 0 + dx2 * 0, nh->bot + 0, pz + dz1 * 0 + dz2 * 0);
        pindex[1] = NS_create_wallstrip_point(px + dx1 * 0 + dx2 * 0, nh->bot + 2, pz + dz1 * 0 + dz2 * 0);
        pindex[2] = NS_create_wallstrip_point(px + dx1 * 64 + dx2 * 64, nh->bot + 3, pz + dz1 * 64 + dz2 * 64);
        pindex[3] = NS_create_wallstrip_point(px + dx1 * 160 + dx2 * 160, nh->bot + 8, pz + dz1 * 160 + dz2 * 160);

        pindex[4] = NS_create_wallstrip_point(px + dx1 * 0 + dx2 * 256, nh->bot + 0, pz + dz1 * 0 + dz2 * 256);
        pindex[5] = NS_create_wallstrip_point(px + dx1 * 0 + dx2 * 256, nh->bot + 2, pz + dz1 * 0 + dz2 * 256);
        pindex[6] = NS_create_wallstrip_point(px + dx1 * 64 + dx2 * 256, nh->bot + 3, pz + dz1 * 64 + dz2 * 256);
        pindex[7] = NS_create_wallstrip_point(px + dx1 * 128 + dx2 * 256, nh->bot + 8, pz + dz1 * 128 + dz2 * 256);

        pindex[8] = NS_create_wallstrip_point(px + dx1 * 256 + dx2 * 0, nh->bot + 0, pz + dz1 * 256 + dz2 * 0);
        pindex[9] = NS_create_wallstrip_point(px + dx1 * 256 + dx2 * 0, nh->bot + 2, pz + dz1 * 256 + dz2 * 0);
        pindex[10] = NS_create_wallstrip_point(px + dx1 * 256 + dx2 * 64, nh->bot + 3, pz + dz1 * 256 + dz2 * 64);
        pindex[11] = NS_create_wallstrip_point(px + dx1 * 256 + dx2 * 128, nh->bot + 8, pz + dz1 * 256 + dz2 * 128);

        p[0] = pindex[8];
        p[1] = pindex[0];
        p[2] = pindex[9];
        p[3] = pindex[1];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP1);

        p[0] = pindex[0];
        p[1] = pindex[4];
        p[2] = pindex[1];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP1);

        p[0] = pindex[9];
        p[1] = pindex[1];
        p[2] = pindex[10];
        p[3] = pindex[2];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP4);

        p[0] = pindex[1];
        p[1] = pindex[5];
        p[2] = pindex[2];
        p[3] = pindex[6];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP5);

        p[0] = pindex[10];
        p[1] = pindex[2];
        p[2] = pindex[11];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP6);

        p[0] = pindex[2];
        p[1] = pindex[6];
        p[2] = pindex[3];
        p[3] = pindex[7];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP7);
    } else {
        //
        // Outside curvey bits.
        //

        switch (curve) {
        case NS_HI_CURVE_OSS:

            px = sx + 1 << 8;
            pz = sz + 1 << 8;

            dx1 = 0;
            dz1 = -1;

            dx2 = -1;
            dz2 = 0;

            break;

        case NS_HI_CURVE_OLS:

            px = sx + 0 << 8;
            pz = sz + 1 << 8;

            dx1 = +1;
            dz1 = 0;

            dx2 = 0;
            dz2 = -1;

            break;

        case NS_HI_CURVE_OSL:

            px = sx + 1 << 8;
            pz = sz + 0 << 8;

            dx1 = -1;
            dz1 = 0;

            dx2 = 0;
            dz2 = +1;

            break;

        case NS_HI_CURVE_OLL:

            px = sx + 0 << 8;
            pz = sz + 0 << 8;

            dx1 = 0;
            dz1 = +1;

            dx2 = +1;
            dz2 = 0;

            break;

        default:
            ASSERT(0);
            break;
        }

        pindex[0] = NS_create_wallstrip_point(px + dx1 * 0 + dx2 * 0, nh->bot + 2, pz + dz1 * 0 + dz2 * 0);
        pindex[1] = NS_create_wallstrip_point(px + dx1 * 64 + dx2 * 64, nh->bot + 3, pz + dz1 * 64 + dz2 * 64);
        pindex[2] = NS_create_wallstrip_point(px + dx1 * 96 + dx2 * 96, nh->bot + 8, pz + dz1 * 96 + dz2 * 96);
        pindex[3] = NS_create_wallstrip_point(px + dx1 * 64 + dx2 * 0, nh->bot + 3, pz + dz1 * 64 + dz2 * 0);
        pindex[4] = NS_create_wallstrip_point(px + dx1 * 128 + dx2 * 0, nh->bot + 8, pz + dz1 * 128 + dz2 * 0);
        pindex[5] = NS_create_wallstrip_point(px + dx1 * 0 + dx2 * 64, nh->bot + 3, pz + dz1 * 0 + dz2 * 64);
        pindex[6] = NS_create_wallstrip_point(px + dx1 * 0 + dx2 * 128, nh->bot + 8, pz + dz1 * 0 + dz2 * 128);

        p[0] = pindex[3];
        p[1] = pindex[0];
        p[2] = pindex[1];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP8);

        p[0] = pindex[3];
        p[1] = pindex[1];
        p[2] = pindex[4];
        p[3] = pindex[2];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP9);

        p[0] = pindex[1];
        p[1] = pindex[5];
        p[2] = pindex[2];
        p[3] = pindex[6];

        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP10);
    }
}

void NS_cache_create_curve_top(SLONG sx, SLONG sz)
{
    UBYTE pindex[16];
    UBYTE curve;

    SLONG ox = sx << 8;
    SLONG oz = sz << 8;

    SLONG p[4];

    SLONG px;
    SLONG py;
    SLONG pz;

    NS_Hi* nh;

    ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 2));
    ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 2));

    nh = &NS_hi[sx][sz];

    ASSERT(NS_HI_TYPE(nh) == NS_HI_TYPE_CURVE);

    curve = nh->water; // We use the water field to identify the type of curve.

    switch (curve) {
    case NS_HI_CURVE_XS:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT1);

        break;

    case NS_HI_CURVE_XL:

        px = ox + 128;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT2);

        break;

    case NS_HI_CURVE_ZS:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 128;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 128;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT3);

        break;

    case NS_HI_CURVE_ZL:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz + 128;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz + 128;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT4);

        break;

    case NS_HI_CURVE_ASS:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 128;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 96;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 96;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT5);

        break;

    case NS_HI_CURVE_ALS:

        px = ox + 128;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 160;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 96;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 128;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT6);

        break;

    case NS_HI_CURVE_ASL:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz + 128;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 96;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz + 160;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT7);

        break;

    case NS_HI_CURVE_ALL:

        px = ox + 160;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz + 160;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz + 128;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT8);

        break;

    case NS_HI_CURVE_OSS:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 160;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 160;

        pindex[4] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 128;

        pindex[5] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[4];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT9);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[4];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT10);

        break;

    case NS_HI_CURVE_OLS:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 128;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 96;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 160;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[4] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[5] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[2];
        p[3] = pindex[3];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT11);

        p[0] = pindex[3];
        p[1] = pindex[1];
        p[2] = pindex[4];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT12);

        break;

    case NS_HI_CURVE_OSL:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 160;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz + 96;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz + 128;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[4] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[5] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[4];
        p[3] = pindex[2];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT13);

        p[0] = pindex[2];
        p[1] = pindex[3];
        p[2] = pindex[4];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT14);

        break;

    case NS_HI_CURVE_OLL:

        px = ox;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz + 128;

        pindex[0] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 96;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz + 96;

        pindex[1] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 128;
        py = NS_hi[sx + 0][sz + 0].top;
        pz = oz + 0;

        pindex[2] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 0].top;
        pz = oz + 0;

        pindex[3] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox;
        py = NS_hi[sx + 0][sz + 1].top;
        pz = oz + 256;

        pindex[4] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        px = ox + 256;
        py = NS_hi[sx + 1][sz + 1].top;
        pz = oz + 256;

        pindex[5] = NS_create_wallstrip_point(px, py, pz, NS_NORM_YL);

        p[0] = pindex[0];
        p[1] = pindex[1];
        p[2] = pindex[4];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT15);

        p[0] = pindex[2];
        p[1] = pindex[3];
        p[2] = pindex[1];
        p[3] = pindex[5];

        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT16);

        break;
    }
}

//
// Creates the curvey bits in a lo-res mapsquare
//

void NS_cache_create_curves(UBYTE mx, UBYTE mz)
{
    SLONG x;
    SLONG z;

    SLONG sx;
    SLONG sz;

    NS_Hi* nh;

    ASSERT(WITHIN(mx, 1, PAP_SIZE_LO - 2));
    ASSERT(WITHIN(mz, 1, PAP_SIZE_LO - 2));

    //
    // The sewer points and faces of the curve only.
    //

    NS_search_start = NS_scratch_point_upto;
    NS_search_end = NS_scratch_point_upto;

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];

            if (NS_HI_TYPE(nh) == NS_HI_TYPE_CURVE) {
                NS_cache_create_curve_sewer(sx, sz);

                //
                // So we can use the points we've just created again.
                //

                NS_search_end = NS_scratch_point_upto;
            }
        }

    //
    // The top of the walls of the sewer system.
    //

    NS_search_start = NS_scratch_point_upto;
    NS_search_end = NS_scratch_point_upto;

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];

            if (NS_HI_TYPE(nh) == NS_HI_TYPE_CURVE) {
                NS_cache_create_curve_top(sx, sz);

                //
                // So we can use the points we've just created again.
                //

                NS_search_end = NS_scratch_point_upto;
            }
        }
}

void NS_cache_create_falls(UBYTE mx, UBYTE mz, NS_Cache* nc)
{
    SLONG i;

    SLONG x;
    SLONG z;

    SLONG sx;
    SLONG sz;

    SLONG dx;
    SLONG dz;

    SLONG nx;
    SLONG nz;

    SLONG fall;

    NS_Hi* nh;
    NS_Hi* nh2;
    NS_Fall* nf;

    ASSERT(WITHIN(mx, 1, PAP_SIZE_LO - 2));
    ASSERT(WITHIN(mz, 1, PAP_SIZE_LO - 2));

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE && NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK) {
                //
                // Look for water on a higher level.
                //

                const struct
                {
                    SBYTE dx;
                    SBYTE dz;

                } dir[4] = {
                    { +1, 0 },
                    { -1, 0 },
                    { 0, +1 },
                    { 0, -1 }
                };

                for (i = 0; i < 4; i++) {
                    dx = dir[i].dx;
                    dz = dir[i].dz;

                    nx = sx + dx;
                    nz = sz + dz;

                    ASSERT(WITHIN(nx, 0, PAP_SIZE_HI - 1));
                    ASSERT(WITHIN(nz, 0, PAP_SIZE_HI - 1));

                    nh2 = &NS_hi[nx][nz];

                    if (nh2->water && (NS_HI_TYPE(nh2) != NS_HI_TYPE_CURVE) && (NS_HI_TYPE(nh2) != NS_HI_TYPE_ROCK)) {
                        //
                        // Found a neighbouring square with water.
                        //

                        if ((nh->water && nh2->water > nh->water) || (!nh->water && nh2->water > nh->bot)) {
                            //
                            // Found a waterfall!
                            //

                            if (NS_fall_free) {
                                //
                                // Take a waterfall out of the linked list.
                                //

                                ASSERT(WITHIN(NS_fall_free, 1, NS_MAX_FALLS - 1));

                                fall = NS_fall_free;
                                nf = &NS_fall[fall];
                                NS_fall_free = nf->next;

                                //
                                // Create the waterfall structure.
                                //

                                nf->x = sx;
                                nf->z = sz;
                                nf->dx = dx;
                                nf->dz = dz;
                                nf->top = nh2->water;
                                nf->bot = (nh->water) ? nh->water : nh->bot;
                                nf->counter = 0;

                                //
                                // Add it to the linked list for this cache element.
                                //

                                nf->next = nc->fall;
                                nc->fall = fall;
                            }
                        }
                    }
                }
            }
        }
}

//
// Creates the walls around the grates.
//

void NS_cache_create_grates(UBYTE mx, UBYTE mz)
{
    SLONG i;

    SLONG x;
    SLONG z;

    SLONG x1;
    SLONG y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG sx;
    SLONG sz;

    SLONG fall;

    NS_Hi* nh;

    SLONG p[4];

    static struct
    {
        SBYTE dx;
        SBYTE dz;
        UWORD norm;

    } order[4] = {
        { +1, 0, NS_NORM_XS },
        { -1, 0, NS_NORM_XL },
        { 0, +1, NS_NORM_ZS },
        { 0, -1, NS_NORM_ZL }
    };

    ASSERT(WITHIN(mx, 1, PAP_SIZE_LO - 2));
    ASSERT(WITHIN(mz, 1, PAP_SIZE_LO - 2));

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];

            if (nh->packed & NS_HI_FLAG_GRATE) {
                //
                // Create each side of the wall under the grate.
                //

                y1 = nh->bot;
                y2 = nh->bot - 8;

                for (i = 0; i < 4; i++) {
                    x1 = (sx << 8) + 0x80 + (order[i].dx + (-order[i].dz) << 7);
                    z1 = (sz << 8) + 0x80 + (order[i].dz + (+order[i].dx) << 7);

                    x2 = (sx << 8) + 0x80 + (order[i].dx - (-order[i].dz) << 7);
                    z2 = (sz << 8) + 0x80 + (order[i].dz - (+order[i].dx) << 7);

                    p[0] = NS_scratch_point_upto + 0;
                    p[1] = NS_scratch_point_upto + 1;
                    p[2] = NS_scratch_point_upto + 2;
                    p[3] = NS_scratch_point_upto + 3;

                    NS_add_point(x1, y2, z1, order[i].norm);
                    NS_add_point(x2, y2, z2, order[i].norm);
                    NS_add_point(x1, y1, z1, NS_NORM_BLACK);
                    NS_add_point(x2, y1, z2, NS_NORM_BLACK);

                    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_FULL);
                }
            }
        }
}

SLONG NS_cache_create(UBYTE mx, UBYTE mz)
{
    SLONG dmx;
    SLONG dmz;

    NS_Cache* nc;
    NS_Lo* nl;

    SLONG c_index;
    SLONG memory_in_bytes;

    SLONG memory_points;
    SLONG memory_faces;

    //
    // Make sure this square is uncached and there is a
    // free cache entry for it.
    //

    ASSERT(WITHIN(mx, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_LO - 1));

    ASSERT(NS_lo[mx][mz].cache == NULL);
    ASSERT(NS_cache_free != NULL);

    if (mx == PAP_SIZE_LO - 1 || mz == PAP_SIZE_LO - 1 || mx == 0 || mz == 0) {
        //
        // We can't generate the edge of the map.
        //

        return FALSE;
    }

    //
    // We create points and faces in the scratch buffer first
    // and then (once we know how much memory we need) we allocated
    // the memory off the heap and copy the data across.
    //

    NS_scratch_point_upto = 0;
    NS_scratch_face_upto = 0;

    //
    // The origin of this square.
    //

    NS_scratch_origin_x = mx << PAP_SHIFT_LO;
    NS_scratch_origin_z = mz << PAP_SHIFT_LO;

    //
    // Get our cache element from the free list.
    //

    c_index = NS_cache_free;

    ASSERT(WITHIN(c_index, 1, NS_MAX_CACHES - 1));

    nc = &NS_cache[c_index];

    NS_cache_free = nc->next;

    //
    // All the lights that could effect this square.
    //

    NS_slight_upto = 0;

    for (dmx = -1; dmx <= +1; dmx++)
        for (dmz = -1; dmz <= +1; dmz++) {
            ASSERT(WITHIN(mx + dmx, 0, PAP_SIZE_LO - 1));
            ASSERT(WITHIN(mz + dmz, 0, PAP_SIZE_LO - 1));

            nl = &NS_lo[mx + dmx][mz + dmz];

            if (nl->light_y) {
                ASSERT(WITHIN(NS_slight_upto, 0, NS_MAX_SLIGHTS - 1));

                NS_slight[NS_slight_upto].x = nl->light_x + dmx * 128;
                NS_slight[NS_slight_upto].z = nl->light_z + dmz * 128;
                NS_slight[NS_slight_upto].y = nl->light_y;

                NS_slight_upto += 1;
            }
        }

    //
    // Create the horizontal squares into the scratch buffers.
    //

    NS_cache_create_floors(mx, mz);
    NS_cache_create_walls(mx, mz);
    NS_cache_create_curves(mx, mz);
    NS_cache_create_grates(mx, mz);

    //
    // Setup the cache element.
    //

    nc->next = NULL;
    nc->used = TRUE;
    nc->map_x = mx;
    nc->map_z = mz;
    nc->num_points = NS_scratch_point_upto;
    nc->num_faces = NS_scratch_face_upto;
    nc->fall = NULL;

    memory_points = nc->num_points * sizeof(NS_Point);
    memory_faces = nc->num_faces * sizeof(NS_Face);

    memory_in_bytes = memory_points + memory_faces;

    //
    // Allocate memory and copy data over from the scratch buffers.
    //

    nc->memory = (UBYTE*)HEAP_get(memory_in_bytes);

    ASSERT(nc->memory != NULL);

    memcpy(nc->memory, NS_scratch_point, memory_points);
    memcpy(nc->memory + memory_points, NS_scratch_face, memory_faces);

    //
    // Add the waterfalls.
    //

    NS_cache_create_falls(mx, mz, nc);

    //
    // Link the mapsquare to this cache element
    //

    NS_lo[mx][mz].cache = c_index;

    return TRUE;
}

void NS_cache_destroy(UBYTE cache)
{
    SLONG memory_in_bytes;
    SLONG fall;
    SLONG next;

    NS_Fall* nf;
    NS_Cache* nc;

    ASSERT(WITHIN(cache, 1, NS_MAX_CACHES - 1));

    nc = &NS_cache[cache];

    ASSERT(nc->used);
    ASSERT(WITHIN(nc->map_x, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(nc->map_z, 0, PAP_SIZE_LO - 1));

    //
    // Returns the memory to the heap.
    //

    memory_in_bytes = nc->num_points * sizeof(NS_Point);
    memory_in_bytes += nc->num_faces * sizeof(NS_Face);

    HEAP_give(nc->memory, memory_in_bytes);

    //
    // Put the waterfalls back into the free list.
    //

    fall = nc->fall;

    while (fall) {
        ASSERT(WITHIN(fall, 1, NS_MAX_FALLS - 1));

        nf = &NS_fall[fall];

        next = nf->next;

        //
        // Add to the free list.
        //

        nf->next = NS_fall_free;
        NS_fall_free = fall;

        fall = next;
    }

    //
    // Mark as unused and return to the free list.
    //

    nc->used = FALSE;
    nc->next = NS_cache_free;
    NS_cache_free = cache;

    //
    // Mark the mapsquare as uncached.
    //

    NS_lo[nc->map_x][nc->map_z].cache = NULL;
}

void NS_cache_fini()
{
    //
    // It does slightly more than necessary!
    //

    NS_cache_init();
}

SLONG NS_calc_height_at(SLONG x, SLONG z)
{
    NS_Hi* nh;

    SLONG ans;

    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
        return 0;
    }

    nh = &NS_hi[mx][mz];

    if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
        return -0x200;
    } else {
        ans = (nh->bot << 5) + (-32 * 0x100);
    }

    return ans;
}

SLONG NS_calc_splash_height_at(SLONG x, SLONG z)
{
    NS_Hi* nh;

    SLONG ans;

    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
        return 0;
    }

    nh = &NS_hi[mx][mz];

    if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
        return -0x200;
    } else {
        if (nh->water) {
            ans = (nh->water << 5) + (-32 * 0x100);
        } else {
            ans = (nh->bot << 5) + (-32 * 0x100);
        }
    }

    return ans;
}

void NS_slide_along(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG radius)
{
    SLONG i;
    SLONG height;
    SLONG collided = FALSE;

    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dz;

    SLONG vx1;
    SLONG vz1;
    SLONG vx2;
    SLONG vz2;

    SLONG sx1;
    SLONG sz1;
    SLONG sx2;
    SLONG sz2;
    SLONG sradius;

    NS_Hi* nh;

    const struct {
        SBYTE dx;
        SBYTE dz;
    } order[4] = {
        { +1, 0 },
        { -1, 0 },
        { 0, +1 },
        { 0, -1 }
    };

    SLONG collide;

    //
    // Keep on the map!
    //

#define NS_SLIDE_MIN ((1 << PAP_SHIFT_LO) << 8)
#define NS_SLIDE_MAX (((PAP_SIZE_LO - 1) << PAP_SHIFT_LO) << 8)

    if (*x2 < NS_SLIDE_MIN) {
        *x2 = NS_SLIDE_MIN;
    }
    if (*x2 > NS_SLIDE_MAX) {
        *x2 = NS_SLIDE_MAX;
    }
    if (*z2 < NS_SLIDE_MIN) {
        *z2 = NS_SLIDE_MIN;
    }
    if (*z2 > NS_SLIDE_MAX) {
        *z2 = NS_SLIDE_MAX;
    }

    //
    // Put the radius in the same coordinate system as (x,y,z)
    //

    radius <<= 8;

    //
    // Collide with the map.
    //

    for (i = 0; i < 4; i++) {
        mx = (x1 >> 16) + order[i].dx;
        mz = (z1 >> 16) + order[i].dz;

        ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
        ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

        nh = &NS_hi[mx][mz];

        switch (NS_HI_TYPE(nh)) {
        case NS_HI_TYPE_ROCK:
        case NS_HI_TYPE_CURVE:
        case NS_HI_TYPE_NOTHING:
            collide = TRUE;
            break;

        default:

            //
            // What is the height of the top of this square?
            //

            height = (nh->bot << (5 + 8)) + (-32 * 0x100 * 0x100);

            //
            // Can step up a quarter of a block.
            //

            if (*y2 + 0x4000 < height) {
                collide = TRUE;
            } else {
                collide = FALSE;
            }

            break;
        }

        if (collide) {
            //
            // Collide with the edge of this square.
            //

            dx = order[i].dx << 7;
            dz = order[i].dz << 7;

            mx = x1 >> 16;
            mz = z1 >> 16;

            sx1 = (mx << 8) + 0x80 + dx - dz;
            sz1 = (mz << 8) + 0x80 + dz + dx;

            sx2 = (mx << 8) + 0x80 + dx + dz;
            sz2 = (mz << 8) + 0x80 + dz - dx;

            sradius = radius >> 8;

            vx1 = x1 >> 8;
            vz1 = z1 >> 8;
            vx2 = *x2 >> 8;
            vz2 = *z2 >> 8;

            if (slide_around_sausage(
                    sx1, sz1,
                    sx2, sz2,
                    sradius,
                    vx1, vz1,
                    &vx2, &vz2)) {
                *x2 = vx2 << 8;
                *z2 = vz2 << 8;
            }
        }
    }
}

SLONG NS_inside(SLONG x, SLONG y, SLONG z)
{
    return y < NS_calc_height_at(x, z);
}

SLONG NS_there_is_a_los(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG i;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dx = x2 - x1;
    SLONG dy = y2 - y1;
    SLONG dz = z2 - z1;

    SLONG len = QDIST3(abs(dx), abs(dy), abs(dz));
    SLONG steps = len >> 5;

    if (len == 0) {
        //
        // there is a los for this small distance
        //
        return (TRUE);
    }
    dx = (dx << 5) / len;
    dy = (dy << 5) / len;
    dz = (dz << 5) / len;

    x = x1 + dx;
    y = y1 + dy;
    z = z1 + dz;

    for (i = 1; i < steps; i++) {
        if (NS_inside(x, y, z)) {
            NS_los_fail_x = x - dx;
            NS_los_fail_y = y - dy;
            NS_los_fail_z = z - dz;

            return FALSE;
        }
    }

    return TRUE;
}

//
// Returns the index of an unused sewer thing or NULL if there
// are no spare sewer things.
//

SLONG NS_get_unused_st(void)
{
    SLONG i;
    SLONG pick;

    NS_St* nst;

    //
    // Look for an unused object.
    //

    pick = rand() % (NS_MAX_STS - 1);
    pick += 1;

    for (i = 0; i < NS_MAX_STS; i++) {
        ASSERT(WITHIN(pick, 1, NS_MAX_STS - 1));

        nst = &NS_st[pick];

        if (nst->type == NS_ST_TYPE_UNUSED) {
            return pick;
        }

        pick += 1;

        if (pick >= NS_MAX_STS) {
            pick = 1;
        }
    }

    //
    // No more sewer things left.
    //

    return NULL;
}

void NS_add_ladder(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG height)
{
    SLONG mx = (x1 + x2 << 7) + ((z2 - z1) << 2) >> PAP_SHIFT_LO;
    SLONG mz = (z1 + z2 << 7) - ((x2 - x1) << 2) >> PAP_SHIFT_LO;

    if (!WITHIN(mx, 1, PAP_SIZE_LO - 2) || !WITHIN(mz, 1, PAP_SIZE_LO - 2)) {
        //
        // The ladder is off the map!
        //

        return;
    }

    NS_Lo* nl = &NS_lo[mx][mz];

    SLONG index = NS_get_unused_st();

    if (index == NULL) {
        //
        // No spare sewer things.
        //

        return;
    }

    //
    // Create the sewer thing.
    //

    ASSERT(WITHIN(index, 1, NS_MAX_STS - 1));

    NS_St* nst = &NS_st[index];

    nst->type = NS_ST_TYPE_LADDER;
    nst->ladder.x1 = x1;
    nst->ladder.z1 = z1;
    nst->ladder.x2 = x2;
    nst->ladder.z2 = z2;
    nst->ladder.height = height;

    //
    // Add to the mapwho.
    //

    nst->next = nl->st;
    nl->st = index;
}

void NS_add_prim(
    SLONG prim,
    SLONG yaw,
    SLONG x,
    SLONG y,
    SLONG z)
{
    SLONG mx = x >> PAP_SHIFT_LO;
    SLONG mz = z >> PAP_SHIFT_LO;

    if (!WITHIN(mx, 1, PAP_SIZE_LO - 2) || !WITHIN(mz, 1, PAP_SIZE_LO - 2)) {
        //
        // The prim is off the map!
        //

        return;
    }

    NS_Lo* nl = &NS_lo[mx][mz];

    SLONG index = NS_get_unused_st();

    if (index == NULL) {
        //
        // No spare sewer things.
        //

        return;
    }

    ASSERT(WITHIN(index, 1, NS_MAX_STS - 1));

    NS_St* nst = &NS_st[index];

    //
    // Room on this square for another
    //

    nst->type = NS_ST_TYPE_PRIM;
    nst->prim.prim = prim;
    nst->prim.yaw = yaw >> 3;
    nst->prim.x = (x - (mx << PAP_SHIFT_LO)) >> 3;
    nst->prim.z = (z - (mz << PAP_SHIFT_LO)) >> 3;
    nst->prim.y = y;

    //
    // Add to the mapwho.
    //

    nst->next = nl->st;
    nl->st = index;
}
