#include "game/game_types.h"
#include "effects/weather/drip.h"
#include "effects/weather/drip_globals.h"
#include "map/pap_globals.h"
#include "engine/effects/psystem.h"
#include "engine/graphics/pipeline/poly.h"
#include "world_objects/puddle.h"
#include "ai/mav.h"
#include "missions/memory.h"

// Starting fade and size for a newly created drip.
#define DRIP_SFADE (255)
#define DRIP_SSIZE (rand() & 0x7)

// Per-tick changes applied in DRIP_process.
#define DRIP_DFADE 16
#define DRIP_DSIZE 4

// uc_orig: DRIP_init (fallen/Source/drip.cpp)
void DRIP_init()
{
    SLONG i;

    for (i = 0; i < DRIP_MAX_DRIPS; i++) {
        DRIP_drip[i].fade = 0;
    }
}

// uc_orig: DRIP_create (fallen/Source/drip.cpp)
void DRIP_create(
    UWORD x,
    SWORD y,
    UWORD z,
    UBYTE flags)
{
    DRIP_last += 1;
    DRIP_last &= DRIP_MAX_DRIPS - 1;

    DRIP_drip[DRIP_last].x = x;
    DRIP_drip[DRIP_last].y = y;
    DRIP_drip[DRIP_last].z = z;
    DRIP_drip[DRIP_last].size = DRIP_SSIZE;
    DRIP_drip[DRIP_last].fade = DRIP_SFADE;
    DRIP_drip[DRIP_last].flags = flags;
}

// uc_orig: DRIP_create_if_in_puddle (fallen/Source/drip.cpp)
void DRIP_create_if_in_puddle(
    UWORD x,
    SWORD y,
    UWORD z)
{
    SLONG mx = x >> 8;
    SLONG mz = z >> 8;
    SLONG my;

    if (WITHIN(mx, 0, MAP_WIDTH - 1) && WITHIN(mz, 0, MAP_HEIGHT - 1)) {
        if (((PAP_2HI(mx, mz).Flags & PAP_FLAG_REFLECTIVE)
                && !(PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN))
            || (MAV_SPARE(mx, mz) & MAV_SPARE_FLAG_WATER)) {
            if (PUDDLE_in(x, z)) {
                DRIP_last += 1;
                DRIP_last &= DRIP_MAX_DRIPS - 1;

                DRIP_drip[DRIP_last].x = x;
                DRIP_drip[DRIP_last].y = y;
                DRIP_drip[DRIP_last].z = z;
                DRIP_drip[DRIP_last].size = DRIP_SSIZE;
                DRIP_drip[DRIP_last].fade = DRIP_SFADE;
                DRIP_drip[DRIP_last].flags = DRIP_FLAG_PUDDLES_ONLY;

                my = PAP_calc_height_at(x, z);
                //			if (y<my) y=my+10;

                PARTICLE_Add(
                    x << 8, my << 8, z << 8,
                    0, 0, 0,
                    POLY_PAGE_SPLASH, 2, 0x7Fffffff, PFLAG_SPRITEANI,
                    10, 40, 1, 0, 0);
            }
        }
    }
}

// uc_orig: DRIP_process (fallen/Source/drip.cpp)
void DRIP_process()
{
    SLONG i;
    SLONG fade;
    SLONG size;

    DRIP_Drip* dd;

    for (i = 0; i < DRIP_MAX_DRIPS; i++) {
        dd = &DRIP_drip[i];

        if (dd->fade) {
            fade = dd->fade;
            size = dd->size;

            fade -= DRIP_DFADE;
            size += DRIP_DSIZE;

            if (fade < 0) {
                fade = 0;
            }

            dd->fade = fade;
            dd->size = size;
        }
    }
}

// uc_orig: DRIP_get_start (fallen/Source/drip.cpp)
void DRIP_get_start()
{
    DRIP_get_upto = 0;
}

// uc_orig: DRIP_get_next (fallen/Source/drip.cpp)
DRIP_Info* DRIP_get_next()
{
    DRIP_Info* di;
    DRIP_Drip* dd;

    while (DRIP_get_upto < DRIP_MAX_DRIPS) {
        dd = &DRIP_drip[DRIP_get_upto++];

        if (dd->fade) {
            di = (DRIP_Info*)dd;

            return di;
        }
    }

    return NULL;
}
