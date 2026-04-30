#include "effects/weather/drip.h"
#include "effects/weather/drip_globals.h"
#include "engine/effects/psystem.h"
#include "engine/graphics/pipeline/poly.h"
#include "world_objects/puddle.h"
#include "ai/mav.h"
#include "game/game_types.h" // UC_VISUAL_CADENCE_TICK_MS — 30 Hz visual cadence

// Starting fade and size for a newly created drip.
#define DRIP_SFADE (255)
#define DRIP_SSIZE (rand() & 0x7)

// Per-tick changes applied in DRIP_process. One tick = UC_VISUAL_CADENCE_TICK_MS
// of wall-clock time (= 33.33 ms = 30 Hz) — see the accumulator in DRIP_process.
// Decrement values are calibrated against the original 30 Hz visual cadence
// (PS1 hardware lock / PC config default — see game_types.h).
#define DRIP_DFADE 16
#define DRIP_DSIZE 4

// Cap for the accumulator after a long stall — prevents catching up too
// many ticks at once. 200 ms matches the spiral-of-death cap in the main
// render loop (game.cpp).
#define DRIP_ACC_MAX_MS 200.0f

static float DRIP_tick_acc_ms = 0.0f;

// uc_orig: DRIP_init (fallen/Source/drip.cpp)
void DRIP_init()
{
    SLONG i;

    for (i = 0; i < DRIP_MAX_DRIPS; i++) {
        DRIP_drip[i].fade = 0;
    }

    DRIP_tick_acc_ms = 0.0f;
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
void DRIP_process(float dt_ms)
{
    if (dt_ms <= 0.0f)
        return;

    DRIP_tick_acc_ms += dt_ms;
    if (DRIP_tick_acc_ms > DRIP_ACC_MAX_MS)
        DRIP_tick_acc_ms = DRIP_ACC_MAX_MS;

    SLONG ticks = SLONG(DRIP_tick_acc_ms / UC_VISUAL_CADENCE_TICK_MS);
    if (ticks <= 0)
        return;
    DRIP_tick_acc_ms -= float(ticks) * UC_VISUAL_CADENCE_TICK_MS;

    SLONG i;
    SLONG fade;
    SLONG size;

    DRIP_Drip* dd;

    for (i = 0; i < DRIP_MAX_DRIPS; i++) {
        dd = &DRIP_drip[i];

        if (dd->fade) {
            fade = dd->fade;
            size = dd->size;

            // Apply N ticks of decay/growth at once. Cap size to fit UBYTE
            // (255) — reaching the cap means the drip is invisible anyway
            // (fade hits 0 first at DRIP_DFADE=16 over ~16 ticks, while
            // size needs 64 ticks to reach 255).
            fade -= DRIP_DFADE * ticks;
            size += DRIP_DSIZE * ticks;

            if (fade < 0) {
                fade = 0;
            }
            if (size > 255) {
                size = 255;
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
