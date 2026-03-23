#include <platform.h>
#include "world/map/map.h"   // MAP_WIDTH, MAP_HEIGHT, ELE_SHIFT, MapElement
#include "core/macros.h"     // WITHIN, SWAP
#include "ai/mav.h"
#include "world/environment/puddle.h"
#include "world/environment/puddle_globals.h"
#include "world/map/pap.h"
#include "world/map/pap_globals.h"

// uc_orig: PUDDLE_Texture (fallen/Source/puddle.cpp)
// UV coordinates for each puddle type.
typedef struct
{
    SLONG u1;
    SLONG v1;
    SLONG u2;
    SLONG v2;

} PUDDLE_Texture;

// uc_orig: PUDDLE_texture (fallen/Source/puddle.cpp)
static PUDDLE_Texture PUDDLE_texture[PUDDLE_TYPE_NUMBER] = {
    { 0, 0, 256, 62 },
    { 0, 64, 256, 126 },
    { 0, 128, 128, 256 },
    { 128, 128, 256, 256 }
};

// uc_orig: PUDDLE_NUM_RIPPLES (fallen/Source/puddle.cpp)
#define PUDDLE_NUM_RIPPLES 1
// uc_orig: PUDDLE_ripple (fallen/Source/puddle.cpp)
// Colour values applied to a puddle when it is splashed.
static struct
{
    UBYTE y1;
    UBYTE y2;
    UBYTE g1;
    UBYTE g2;
    UBYTE s1;
    UBYTE s2;

} PUDDLE_ripple[PUDDLE_NUM_RIPPLES] = {
    { 62, 137, 17, 178, 40, 45 },
};

// uc_orig: PUDDLE_create_do (fallen/Source/puddle.cpp)
// Internal helper: inserts a puddle quad into the sorted spatial index.
static void PUDDLE_create_do(
    UWORD x1,
    UWORD z1,
    UWORD x2,
    UWORD z2,
    SWORD y,
    UBYTE type,
    UBYTE rotate_uvs)
{
    SLONG x;
    SLONG z;
    PUDDLE_Puddle* pp;
    UBYTE next;
    UBYTE* prev;

    UBYTE map_x = x1 + x2 >> 9;
    UBYTE map_z = z1 + z2 >> 9;

    if (!WITHIN(map_z, 0, PUDDLE_MAPWHO_SIZE - 1)) {
        return;
    }

    if (!WITHIN(PUDDLE_puddle_upto, 1, PUDDLE_MAX_PUDDLES - 1)) {
        return;
    }

    if (!PAP_is_flattish(x1, z1, x2, z2)) {
        return;
    }

    pp = &PUDDLE_puddle[PUDDLE_puddle_upto];
    pp->x1 = x1;
    pp->z1 = z1;
    pp->x2 = x2;
    pp->z2 = z2;
    pp->y = y;
    pp->map_x = map_x;
    pp->rotate_uvs = rotate_uvs;
    pp->type = type;

    // Insert into the z-line linked list sorted by map_x.
    prev = &PUDDLE_mapwho[map_z];
    next = PUDDLE_mapwho[map_z];

    while (1) {
        ASSERT(WITHIN(next, 0, PUDDLE_puddle_upto - 1));

        if (next == NULL || PUDDLE_puddle[next].map_x >= map_x) {
            *prev = PUDDLE_puddle_upto;
            pp->next = next;
            PUDDLE_puddle_upto += 1;
            break;
        }

        prev = &PUDDLE_puddle[next].next;
        next = PUDDLE_puddle[next].next;
    }

    // Mark map squares around this puddle as reflective.
    {
        SLONG mx1, mz1, mx2, mz2;

#define PUDDLE_EXTEND_REFLECTION 128

        mx1 = pp->x1;
        mz1 = pp->z1;
        mx2 = pp->x2;
        mz2 = pp->z2;

        if (mx1 > mx2) { SWAP(mx1, mx2); }
        if (mz1 > mz2) { SWAP(mz1, mz2); }

        mx1 = mx1 - PUDDLE_EXTEND_REFLECTION >> ELE_SHIFT;
        mz1 = mz1 - PUDDLE_EXTEND_REFLECTION >> ELE_SHIFT;
        mx2 = mx2 + PUDDLE_EXTEND_REFLECTION >> ELE_SHIFT;
        mz2 = mz2 + PUDDLE_EXTEND_REFLECTION >> ELE_SHIFT;

        SATURATE(mx1, 0, MAP_WIDTH - 1);
        SATURATE(mx2, 0, MAP_WIDTH - 1);
        SATURATE(mz1, 0, MAP_HEIGHT - 1);
        SATURATE(mz2, 0, MAP_HEIGHT - 1);

        for (x = mx1; x <= mx2; x++)
            for (z = mz1; z <= mz2; z++) {
                PAP_2HI(x, z).Flags |= PAP_FLAG_REFLECTIVE;
            }
    }
}

// uc_orig: PUDDLE_init (fallen/Source/puddle.cpp)
void PUDDLE_init()
{
    SLONG x;
    SLONG z;
    MapElement* me;

    memset(PUDDLE_puddle, 0, sizeof(PUDDLE_puddle));
    memset(PUDDLE_mapwho, 0, sizeof(PUDDLE_mapwho));

    PUDDLE_puddle_upto = 1;

    // Clear the reflective flag on all map squares.
    for (x = 0; x < MAP_WIDTH; x++)
        for (z = 0; z < MAP_HEIGHT; z++) {
            PAP_2HI(x, z).Flags &= ~PAP_FLAG_REFLECTIVE;
        }
}

// uc_orig: PUDDLE_create (fallen/Source/puddle.cpp)
void PUDDLE_create(UWORD x, SWORD y, UWORD z)
{
    SLONG x1, z1, x2, z2;

#define PUDDLE_WHOLE_SIZE 0x140

    x1 = x - PUDDLE_WHOLE_SIZE;
    z1 = z - PUDDLE_WHOLE_SIZE;
    x2 = x + PUDDLE_WHOLE_SIZE;
    z2 = z + PUDDLE_WHOLE_SIZE;

    // Randomise the orientation of the puddle quad.
    if (rand() & 0x1) { SWAP(x1, x2); }
    if (rand() & 0x1) { SWAP(z1, z2); }

    PUDDLE_create_do(x1, z1, x2, z2, y, PUDDLE_TYPE_WHOLE, UC_FALSE);
}

// uc_orig: PUDDLE_precalculate (fallen/Source/puddle.cpp)
void PUDDLE_precalculate()
{
    SLONG i, j;
    SLONG mx, mz;
    SLONG dx, dz;
    SLONG cx, cz;
    SLONG type;
    SLONG vec1x, vec1z, vec2x, vec2z;
    SLONG midx, midz, size;
    SLONG px1, px2, pz1, pz2, py;

    struct { SLONG dx; SLONG dz; } order[4] = {
        { -1, 0 }, { +1, 0 }, { 0, -1 }, { 0, +1 }
    };

    for (mx = 1; mx < MAP_WIDTH - 2; mx++) {
        for (mz = 1; mz < MAP_HEIGHT - 2; mz++) {

            // Corners of buildings.
            if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
                for (i = 0; i < 4; i++) {
                    dx = (i & 0x1) ? +1 : -1;
                    dz = (i & 0x2) ? +1 : -1;

                    for (j = 1; j < 4; j++) {
                        cx = mx + ((j & 0x1) ? dx : 0);
                        cz = mz + ((j & 0x2) ? dz : 0);

                        if (PAP_2HI(cx, cz).Flags & (PAP_FLAG_HIDDEN | PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER))
                            goto not_a_corner_of_a_building;
                        if (MAVHEIGHT(cx, cz) > MAVHEIGHT(mx, mz))
                            goto not_a_corner_of_a_building;
                        if (MAV_SPARE(cx, cz) & MAV_SPARE_FLAG_WATER)
                            goto not_a_corner_of_a_building;
                    }

                    if ((rand() & 0x3) == ((mx + mz) & 0x3)) {
                        midx = (mx << 8) + 0x80 + dx * 0x80;
                        midz = (mz << 8) + 0x80 + dz * 0x80;
                        size = 0xa0 + (rand() & 0x3f);
                        px1 = midx - dx * size;
                        pz1 = midz - dz * size;
                        px2 = midx + dx * size;
                        pz2 = midz + dz * size;
                        py = PAP_calc_height_at(px2, pz2) + 0x8;
                        PUDDLE_create_do(px1, pz1, px2, pz2, py, PUDDLE_TYPE_CORNER, UC_FALSE);
                    }

                not_a_corner_of_a_building:;
                }
            }

            // Corners of roads.
            if (!(PAP_2HI(mx, mz).Flags & PAP_FLAG_SINK_SQUARE)) {
                for (i = 0; i < 4; i++) {
                    dx = (i & 0x1) ? +1 : -1;
                    dz = (i & 0x2) ? +1 : -1;

                    for (j = 1; j < 4; j++) {
                        cx = mx + ((j & 0x1) ? dx : 0);
                        cz = mz + ((j & 0x2) ? dz : 0);

                        if (PAP_2HI(cx, cz).Flags & (PAP_FLAG_HIDDEN | PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER))
                            goto not_a_corner_of_a_road;
                        if (!(PAP_2HI(cx, cz).Flags & PAP_FLAG_SINK_SQUARE))
                            goto not_a_corner_of_a_road;
                    }
                    if (MAV_SPARE(cx, cz) & MAV_SPARE_FLAG_WATER)
                        goto not_a_corner_of_a_road;

                    if ((rand() & 0x3) == ((mx + mz) & 0x3)) {
                        midx = (mx << 8) + 0x80 + dx * 0x80;
                        midz = (mz << 8) + 0x80 + dz * 0x80;
                        size = 0xa0 + (rand() & 0x3f);
                        px1 = midx - dx * size;
                        pz1 = midz - dz * size;
                        px2 = midx + dx * size;
                        pz2 = midz + dz * size;
                        py = PAP_calc_height_at(px2, pz2) + 0x2;
                        PUDDLE_create_do(px1, pz1, px2, pz2, py, PUDDLE_TYPE_CORNER, UC_FALSE);
                    }

                not_a_corner_of_a_road:;
                }
            }

            // Edges of buildings.
            if (!(PAP_2HI(mx, mz).Flags & (PAP_FLAG_HIDDEN | PAP_FLAG_REFLECTIVE))) {
                for (i = 0; i < 4; i++) {
                    dx = order[i].dx;
                    dz = order[i].dz;
                    vec1x = dx; vec1z = dz;
                    vec2x = dz; vec2z = -dx;
                    cx = mx + vec1x; cz = mz + vec1z;

                    if (!(PAP_2HI(cx, cz).Flags & PAP_FLAG_HIDDEN) || (PAP_2HI(cx, cz).Flags & (PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER)))
                        goto not_a_building_edge;
                    if (MAVHEIGHT(cx, cz) < MAVHEIGHT(mx, mz))
                        goto not_a_building_edge;
                    if (MAV_SPARE(cx, cz) & MAV_SPARE_FLAG_WATER)
                        goto not_a_building_edge;

                    cx = mx + vec1x + vec2x; cz = mz + vec1z + vec2z;
                    if (!(PAP_2HI(cx, cz).Flags & PAP_FLAG_HIDDEN) || (PAP_2HI(cx, cz).Flags & (PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER)))
                        goto not_a_building_edge;
                    if (MAVHEIGHT(cx, cz) < MAVHEIGHT(mx, mz))
                        goto not_a_building_edge;

                    cx = mx + vec2x; cz = mz + vec2z;
                    if (PAP_2HI(cx, cz).Flags & (PAP_FLAG_HIDDEN | PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER))
                        goto not_a_building_edge;

                    if ((rand() & 0x7) == ((mx + mz) & 0x7)) {
                        size = 0xa0 + (rand() & 0x3f);
                        px1 = (mx << 8) + 0x80 + vec1x * 0x80 - vec2x * 0x80;
                        pz1 = (mz << 8) + 0x80 + vec1z * 0x80 - vec2z * 0x80;
                        px2 = px1 - vec1x * size + vec2x * 0x200;
                        pz2 = pz1 - vec1z * size + vec2z * 0x200;
                        py = PAP_calc_height_at(px2, pz2) + 0x4;
                        type = PUDDLE_TYPE_STRIP1 + (rand() & 0x1);
                        PUDDLE_create_do(px1, pz1, px2, pz2, py, type, (i == 0 || i == 1));
                    }

                not_a_building_edge:;
                }
            }

            // Edges of roads.
            if ((PAP_2HI(mx, mz).Flags & PAP_FLAG_SINK_SQUARE) && !(PAP_2HI(mx, mz).Flags & (PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER))) {
                for (i = 0; i < 4; i++) {
                    dx = order[i].dx; dz = order[i].dz;
                    vec1x = dx; vec1z = dz;
                    vec2x = dz; vec2z = -dx;
                    cx = mx + vec1x; cz = mz + vec1z;

                    if (PAP_2HI(cx, cz).Flags & (PAP_FLAG_SINK_SQUARE | PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER))
                        goto not_a_road_edge;
                    if (MAV_SPARE(cx, cz) & MAV_SPARE_FLAG_WATER)
                        goto not_a_road_edge;

                    cx = mx + vec1x + vec2x; cz = mz + vec1z + vec2z;
                    if (PAP_2HI(cx, cz).Flags & (PAP_FLAG_SINK_SQUARE | PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER))
                        goto not_a_road_edge;

                    cx = mx + vec2x; cz = mz + vec2z;
                    if (!(PAP_2HI(cx, cz).Flags & PAP_FLAG_SINK_SQUARE) || (PAP_2HI(cx, cz).Flags & (PAP_FLAG_REFLECTIVE | PAP_FLAG_WATER)))
                        goto not_a_road_edge;

                    if ((rand() & 0x3) == ((mx + mz) & 0x3)) {
                        size = 0xa0 + (rand() & 0x3f);
                        px1 = (mx << 8) + 0x80 + vec1x * 0x80 - vec2x * 0x80;
                        pz1 = (mz << 8) + 0x80 + vec1z * 0x80 - vec2z * 0x80;
                        px2 = px1 - vec1x * size + vec2x * 0x200;
                        pz2 = pz1 - vec1z * size + vec2z * 0x200;
                        py = PAP_calc_height_at(px2, pz2) + 0x2;
                        type = PUDDLE_TYPE_STRIP1 + (rand() & 0x1);
                        PUDDLE_create_do(px1, pz1, px2, pz2, py, type, (i == 0 || i == 1));
                    }

                not_a_road_edge:;
                }
            }
        }
    }
}

// uc_orig: PUDDLE_in (fallen/Source/puddle.cpp)
SLONG PUDDLE_in(SLONG x, SLONG z)
{
    SLONG lz, pz;
    SLONG px1, px2, pz1, pz2;
    UBYTE next;
    PUDDLE_Puddle* pp;

    SLONG mx = x >> 8;
    SLONG mz = z >> 8;

    if (!WITHIN(mx, 0, MAP_WIDTH - 1) || !WITHIN(mz, 0, MAP_HEIGHT - 1)) {
        return UC_FALSE;
    }

    if (MAV_SPARE(mx, mz) & MAV_SPARE_FLAG_WATER) {
        return UC_TRUE;
    }

    if (PAP_2HI(mx, mz).Flags & PAP_FLAG_REFLECTIVE) {
        for (lz = -1; lz <= 1; lz++) {
            pz = mz + lz;

            if (WITHIN(pz, 0, PUDDLE_MAPWHO_SIZE - 1)) {
                for (next = PUDDLE_mapwho[pz]; next; next = PUDDLE_puddle[next].next) {
                    ASSERT(WITHIN(next, 1, PUDDLE_puddle_upto - 1));
                    pp = &PUDDLE_puddle[next];

                    px1 = pp->x1; pz1 = pp->z1;
                    px2 = pp->x2; pz2 = pp->z2;

                    if (px1 > px2) { SWAP(px1, px2); }
                    if (pz1 > pz2) { SWAP(pz1, pz2); }

                    if (WITHIN(x, px1, px2) && WITHIN(z, pz1, pz2)) {
                        return UC_TRUE;
                    }
                }
            }
        }
    }

    return UC_FALSE;
}

// uc_orig: PUDDLE_splash (fallen/Source/puddle.cpp)
void PUDDLE_splash(SLONG x, SLONG y, SLONG z)
{
    SLONG mx, mz, lz, pz;
    SLONG dy;
    UBYTE next;
    PUDDLE_Puddle* pp;

    mx = x >> 8;
    mz = z >> 8;

    if (!WITHIN(mx, 0, MAP_WIDTH - 1) || !WITHIN(mz, 0, MAP_HEIGHT - 1)) {
        return;
    }

    if ((PAP_2HI(mx, mz).Flags & PAP_FLAG_REFLECTIVE) || (MAV_SPARE(mx, mz) & MAV_SPARE_FLAG_WATER)) {
        for (lz = -1; lz <= 1; lz++) {
            pz = mz + lz;

            if (WITHIN(pz, 0, PUDDLE_MAPWHO_SIZE - 1)) {
                for (next = PUDDLE_mapwho[pz]; next; next = PUDDLE_puddle[next].next) {
                    ASSERT(WITHIN(next, 1, PUDDLE_puddle_upto - 1));
                    pp = &PUDDLE_puddle[next];

                    dy = pp->y - y;

#define PUDDLE_SPLASH_Y_RANGE 32

                    if (WITHIN(dy, -PUDDLE_SPLASH_Y_RANGE, +PUDDLE_SPLASH_Y_RANGE)) {
                        SLONG px1 = pp->x1, pz1 = pp->z1;
                        SLONG px2 = pp->x2, pz2 = pp->z2;

                        if (px1 > px2) { SWAP(px1, px2); }
                        if (pz1 > pz2) { SWAP(pz1, pz2); }

                        if (WITHIN(x, px1, px2) && WITHIN(z, pz1, pz2)) {
                            SLONG ripple = rand() & (PUDDLE_NUM_RIPPLES - 1);
                            pp->y1 = PUDDLE_ripple[ripple].y1;
                            pp->y2 = PUDDLE_ripple[ripple].y2;
                            pp->g1 = PUDDLE_ripple[ripple].g1;
                            pp->g2 = PUDDLE_ripple[ripple].g2;
                            pp->s1 = PUDDLE_ripple[ripple].s1;
                            pp->s2 = PUDDLE_ripple[ripple].s2;
                        }
                    }
                }
            }
        }
    }
}

// uc_orig: PUDDLE_process (fallen/Source/puddle.cpp)
void PUDDLE_process()
{
    SLONG i, s1, s2;
    PUDDLE_Puddle* pp;

    for (i = 1; i < PUDDLE_puddle_upto; i++) {
        pp = &PUDDLE_puddle[i];

        if (pp->s1 | pp->s2) {
            s1 = pp->s1;
            s2 = pp->s2;
            if (s1 > 0) { s1 -= 1; }
            if (s2 > 0) { s2 -= 1; }
            pp->s1 = s1;
            pp->s2 = s2;
        }
    }
}

// uc_orig: PUDDLE_get_start (fallen/Source/puddle.cpp)
void PUDDLE_get_start(UBYTE z_map, UBYTE x_map_min, UBYTE x_map_max)
{
    PUDDLE_get_z = z_map;
    PUDDLE_get_x_min = x_map_min - 1;
    PUDDLE_get_x_max = x_map_max + 1;

    if (!WITHIN(z_map, 0, PUDDLE_MAPWHO_SIZE - 1)) {
        PUDDLE_get_upto = 0;
    } else {
        PUDDLE_get_upto = PUDDLE_mapwho[z_map];

        // Advance past puddles before the x_map_min boundary.
        while (1) {
            if (PUDDLE_get_upto == NULL) {
                return;
            }
            ASSERT(WITHIN(PUDDLE_get_upto, 1, PUDDLE_puddle_upto - 1));
            if (PUDDLE_puddle[PUDDLE_get_upto].map_x >= x_map_min) {
                return;
            }
            PUDDLE_get_upto = PUDDLE_puddle[PUDDLE_get_upto].next;
        }
    }
}

// uc_orig: PUDDLE_get_next (fallen/Source/puddle.cpp)
PUDDLE_Info* PUDDLE_get_next()
{
    PUDDLE_Puddle* pp;

    if (PUDDLE_get_upto == NULL) {
        return NULL;
    }

    ASSERT(WITHIN(PUDDLE_get_upto, 1, PUDDLE_puddle_upto - 1));

    pp = &PUDDLE_puddle[PUDDLE_get_upto];

    if (pp->map_x > PUDDLE_get_x_max) {
        return NULL;
    }

    PUDDLE_get_info.x1 = pp->x1;
    PUDDLE_get_info.z1 = pp->z1;
    PUDDLE_get_info.x2 = pp->x2;
    PUDDLE_get_info.z2 = pp->z2;
    PUDDLE_get_info.y = pp->y;
    PUDDLE_get_info.puddle_y1 = pp->y1;
    PUDDLE_get_info.puddle_y2 = pp->y2;
    PUDDLE_get_info.puddle_g1 = pp->g1;
    PUDDLE_get_info.puddle_g2 = pp->g2;
    PUDDLE_get_info.puddle_s1 = pp->s1;
    PUDDLE_get_info.puddle_s2 = pp->s2;

    ASSERT(WITHIN(pp->type, 0, PUDDLE_TYPE_NUMBER - 1));

    PUDDLE_get_info.u1 = PUDDLE_texture[pp->type].u1;
    PUDDLE_get_info.v1 = PUDDLE_texture[pp->type].v1;
    PUDDLE_get_info.u2 = PUDDLE_texture[pp->type].u2;
    PUDDLE_get_info.v2 = PUDDLE_texture[pp->type].v2;

    PUDDLE_get_info.rotate_uvs = pp->rotate_uvs;
    PUDDLE_get_upto = pp->next;

    return &PUDDLE_get_info;
}
