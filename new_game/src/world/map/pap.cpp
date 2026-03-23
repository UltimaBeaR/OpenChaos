// Two-tier heightfield map (PAP).
// Provides terrain height queries and map bounds checks used throughout game, AI, and collision code.

#include <MFStdLib.h>
#include "world/map/pap.h"
#include "world/map/pap_globals.h"
// Temporary: game.h for GAME_FLAGS/GF_NO_FLOOR, Thing struct, WARE_calc_height_at, etc.
#include "fallen/Headers/Game.h"
#include "ai/mav.h"
#include "fallen/Headers/ware.h"

// uc_orig: PAP_on_map_lo (fallen/Source/pap.cpp)
SLONG PAP_on_map_lo(SLONG x, SLONG z)
{
    if (WITHIN(x, 0, PAP_SIZE_LO - 1) && WITHIN(z, 0, PAP_SIZE_LO - 1)) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: PAP_on_map_hi (fallen/Source/pap.cpp)
SLONG PAP_on_map_hi(SLONG x, SLONG z)
{
    if (WITHIN(x, 0, PAP_SIZE_HI - 1) && WITHIN(z, 0, PAP_SIZE_HI - 1)) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: PAP_assert_if_off_map_lo (fallen/Source/pap.cpp)
static void PAP_assert_if_off_map_lo(SLONG x, SLONG z)
{
    ASSERT(PAP_on_map_lo(x, z));
}

// uc_orig: PAP_assert_if_off_map_hi (fallen/Source/pap.cpp)
static void PAP_assert_if_off_map_hi(SLONG x, SLONG z)
{
    ASSERT(PAP_on_map_hi(x, z));
}

// uc_orig: PAP_clear (fallen/Source/pap.cpp)
void PAP_clear(void)
{
    memset((UBYTE*)&PAP_lo[0][0], 0, sizeof(PAP_Lo) * PAP_SIZE_LO * PAP_SIZE_LO);
    memset((UBYTE*)&PAP_hi[0][0], 0, sizeof(PAP_Hi) * PAP_SIZE_HI * PAP_SIZE_HI);
}

// uc_orig: PAP_calc_height_at_point (fallen/Source/pap.cpp)
SLONG PAP_calc_height_at_point(SLONG map_x, SLONG map_z)
{
    if (!WITHIN(map_x, 0, PAP_SIZE_HI - 1) || !WITHIN(map_z, 0, PAP_SIZE_HI - 1)) {
        return 0;
    } else {
        return PAP_2HI(map_x, map_z).Alt << PAP_ALT_SHIFT;
    }
}

// Bilinear interpolation over the hi-res grid. The grid uses two triangles per quad:
//   triangle 1 (xfrac+zfrac < 256): uses corners h0, h1, h2
//   triangle 2 (xfrac+zfrac >= 256): uses corner h3
// uc_orig: PAP_calc_height_at (fallen/Source/pap.cpp)
SLONG PAP_calc_height_at(SLONG x, SLONG z)
{
    SLONG h0;
    SLONG h1;
    SLONG h2;
    SLONG h3;

    SLONG xfrac;
    SLONG zfrac;

    SLONG answer;

    PAP_Hi* ph;

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return -32767;
    }

    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (mx < 0 || mx > PAP_SIZE_HI - 2 || mz < 0 || mz > PAP_SIZE_HI - 2) {
        return 0;
    }

    ph = &PAP_2HI(mx, mz);

    h0 = ph[0].Alt;
    h1 = ph[1].Alt;
    h2 = ph[PAP_SIZE_HI + 0].Alt;
    h3 = ph[PAP_SIZE_HI + 1].Alt;

    if (h0 == h1 && h1 == h2 && h2 == h3) {
        answer = h0 << PAP_ALT_SHIFT;
    } else {
        h0 <<= PAP_ALT_SHIFT;
        h1 <<= PAP_ALT_SHIFT;
        h2 <<= PAP_ALT_SHIFT;
        h3 <<= PAP_ALT_SHIFT;

        xfrac = x & 0xff;
        zfrac = z & 0xff;

        if (xfrac + zfrac < 0x100) {
            answer = h0;
            answer += (h2 - h0) * xfrac >> 8;
            answer += (h1 - h0) * zfrac >> 8;
        } else {
            answer = h3;
            answer += (h1 - h3) * (0x100 - xfrac) >> 8;
            answer += (h2 - h3) * (0x100 - zfrac) >> 8;
        }
    }

    if (ph->Flags & PAP_FLAG_SINK_SQUARE) {
        answer -= KERB_HEIGHTI;
    }

    return answer;
}

// uc_orig: PAP_calc_height_at_thing (fallen/Source/pap.cpp)
SLONG PAP_calc_height_at_thing(Thing* p_thing, SLONG x, SLONG z)
{
    switch (p_thing->Class) {
    case CLASS_PERSON:

        if (p_thing->Genus.Person->Flags & FLAG_PERSON_WAREHOUSE) {
            return WARE_calc_height_at(
                p_thing->Genus.Person->Ware,
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Z >> 8);
        }
        break;
    }
    return (PAP_calc_map_height_at(x, z));
}

// uc_orig: PAP_calc_map_height_at (fallen/Source/pap.cpp)
SLONG PAP_calc_map_height_at(SLONG x, SLONG z)
{
    SLONG h0;
    SLONG h1;
    SLONG h2;
    SLONG h3;

    SLONG xfrac;
    SLONG zfrac;

    SLONG answer;

    PAP_Hi* ph;

    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (mx < 0 || mx > PAP_SIZE_HI - 2 || mz < 0 || mz > PAP_SIZE_HI - 2) {
        if (GAME_FLAGS & GF_NO_FLOOR) {
            return -32767;
        } else {
            return 0;
        }
    }

    ph = &PAP_2HI(mx, mz);

    if (ph->Flags & PAP_FLAG_HIDDEN) {
        return MAVHEIGHT(mx, mz) << 6;
    }

    if (GAME_FLAGS & GF_NO_FLOOR) {
        return -32767;
    }

    h0 = ph[0].Alt;
    h1 = ph[1].Alt;
    h2 = ph[PAP_SIZE_HI + 0].Alt;
    h3 = ph[PAP_SIZE_HI + 1].Alt;

    if (h0 == h1 && h1 == h2 && h2 == h3) {
        answer = h0 << PAP_ALT_SHIFT;
    } else {
        h0 <<= PAP_ALT_SHIFT;
        h1 <<= PAP_ALT_SHIFT;
        h2 <<= PAP_ALT_SHIFT;
        h3 <<= PAP_ALT_SHIFT;

        xfrac = x & 0xff;
        zfrac = z & 0xff;

        if (xfrac + zfrac < 0x100) {
            answer = h0;
            answer += (h2 - h0) * xfrac >> 8;
            answer += (h1 - h0) * zfrac >> 8;
        } else {
            answer = h3;
            answer += (h1 - h3) * (0x100 - xfrac) >> 8;
            answer += (h2 - h3) * (0x100 - zfrac) >> 8;
        }
    }

    if (ph->Flags & PAP_FLAG_SINK_SQUARE) {
        answer -= KERB_HEIGHTI;
    }

    return answer;
}

// uc_orig: PAP_is_flattish (fallen/Source/pap.cpp)
SLONG PAP_is_flattish(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG i;

    SLONG max = -UC_INFINITY;
    SLONG min = +UC_INFINITY;

    SLONG alongx;
    SLONG alongz;

    SLONG dx = x2 - x1;
    SLONG dz = z2 - z1;

    SLONG x;
    SLONG z;

    SLONG height;

#define PAP_FLATTISH_SAMPLES 8

    for (i = 0; i < PAP_FLATTISH_SAMPLES; i++) {
        alongx = Random() & 0xff;
        alongz = Random() & 0xff;

        x = x1 + (dx * alongx >> 8);
        z = z1 + (dz * alongz >> 8);

        height = PAP_calc_height_at(x, z);

        if (height > max) {
            max = height;
        }
        if (height < min) {
            min = height;
        }

        if (abs(max - min) > 0x10) {
            return UC_FALSE;
        }
    }

    return UC_TRUE;
}

// uc_orig: PAP_calc_height_noroads (fallen/Source/pap.cpp)
SLONG PAP_calc_height_noroads(SLONG x, SLONG z)
{
    SLONG h0;
    SLONG h1;
    SLONG h2;
    SLONG h3;

    SLONG xfrac;
    SLONG zfrac;

    SLONG answer;

    PAP_Hi* ph;

    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (mx < 0 || mx > PAP_SIZE_HI - 2 || mz < 0 || mz > PAP_SIZE_HI - 2) {
        return 0;
    }

    ph = &PAP_2HI(mx, mz);

    h0 = ph[0].Alt;
    h1 = ph[1].Alt;
    h2 = ph[PAP_SIZE_HI + 0].Alt;
    h3 = ph[PAP_SIZE_HI + 1].Alt;

    if (h0 == h1 && h1 == h2 && h2 == h3) {
        answer = h0 << PAP_ALT_SHIFT;
    } else {
        h0 <<= PAP_ALT_SHIFT;
        h1 <<= PAP_ALT_SHIFT;
        h2 <<= PAP_ALT_SHIFT;
        h3 <<= PAP_ALT_SHIFT;

        xfrac = x & 0xff;
        zfrac = z & 0xff;

        if (xfrac + zfrac < 0x100) {
            answer = h0;
            answer += (h2 - h0) * xfrac >> 8;
            answer += (h1 - h0) * zfrac >> 8;
        } else {
            answer = h3;
            answer += (h1 - h3) * (0x100 - xfrac) >> 8;
            answer += (h2 - h3) * (0x100 - zfrac) >> 8;
        }
    }

    return answer;
}

// uc_orig: PAP_calc_map_height_near (fallen/Source/pap.cpp)
SLONG PAP_calc_map_height_near(SLONG x, SLONG z)
{
    SLONG i;

    SLONG dx;
    SLONG dz;

    SLONG height;
    SLONG max = -UC_INFINITY;

    for (i = 0; i < 4; i++) {
        dx = (i & 1) ? -8 : +8;
        dz = (i & 2) ? -8 : +8;

        height = PAP_calc_map_height_at(x + dx, z + dz);

        if (height > max) {
            max = height;
        }
    }

    return max;
}

// Returns slope steepness (0 = flat) and fills *angle with the downslope direction.
// Uses cross-product of two edge vectors in the triangle containing (x,z).
// uc_orig: PAP_on_slope (fallen/Source/pap.cpp)
SLONG PAP_on_slope(SLONG x, SLONG z, SLONG* angle)
{
    SLONG h0;
    SLONG h1;
    SLONG h2;
    SLONG h3;

    SLONG xfrac;
    SLONG zfrac;

    SLONG answer;

    PAP_Hi* ph;

    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (mx < 0 || mx > PAP_SIZE_HI - 2 || mz < 0 || mz > PAP_SIZE_HI - 2) {
        return 0;
    }

    ph = &PAP_2HI(mx, mz);

    if (ph->Flags & PAP_FLAG_HIDDEN) {
        return 0;
    }

    h0 = ph[0].Alt;
    h1 = ph[1].Alt;
    h2 = ph[PAP_SIZE_HI + 0].Alt;
    h3 = ph[PAP_SIZE_HI + 1].Alt;

    if (h0 == h1 && h1 == h2 && h2 == h3) {
        return (0);

    } else {

        //  h0   h2
        //
        //  h1   h3

        h0 <<= PAP_ALT_SHIFT;
        h1 <<= PAP_ALT_SHIFT;
        h2 <<= PAP_ALT_SHIFT;
        h3 <<= PAP_ALT_SHIFT;

        xfrac = x & 0xff;
        zfrac = z & 0xff;

        if (xfrac + zfrac < 0x100) {
            SLONG vx, vy, vz;
            SLONG wx, wy, wz;
            SLONG rx, ry, rz;
            SLONG len;

            vx = 256;
            vy = h2 - h0;
            vz = 0;

            wx = 0;
            wy = h1 - h0;
            wz = -256;

            rx = (vy * wz);
            ry = 65536;
            rz = (vx * wy);

            if (rx == 0 && rz == 0)
                return (0);

            *angle = (Arctan(-rx, -rz)) & 2047;

            rx = abs(rx);
            rz = abs(rz);
            len = QDIST3(rx, ry, rz);

            ry = (ry << 8) / (len);

            return (abs(256 - (len >> 8)));

        } else {
            SLONG vx, vy, vz;
            SLONG wx, wy, wz;
            SLONG rx, ry, rz;
            SLONG len;

            vx = -256;
            vy = h1 - h3;
            vz = 0;

            wx = 0;
            wy = h2 - h3;
            wz = -256;

            rx = (vy * wz);
            ry = 65536;
            rz = (vx * wy);

            if (rx == 0 && rz == 0)
                return (0);

            *angle = (Arctan(rx, -rz)) & 2047;

            rx = abs(rx);
            rz = abs(rz);
            len = QDIST3(rx, ry, rz);

            ry = (ry << 8) / (len);

            return (abs(256 - (len >> 8)));
        }
    }
}
