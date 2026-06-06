#include "effects/weather/drip.h"
#include "effects/weather/drip_globals.h"
#include "engine/effects/psystem.h"
#include "engine/graphics/pipeline/poly.h"
#include "world_objects/puddle.h"
#include "ai/mav.h"
#include "game/game_types.h" // UC_VISUAL_CADENCE_TICK_MS — 30 Hz visual cadence

#include <cmath> // sqrtf — splash falloff curve

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

                // Splash billboard Y depends on how close the foot is
                // to the puddle's rect center: at center → lifted above
                // water (visible), at rect edge → sunk below ground
                // (invisible). Why: puddle rects are larger than the
                // actual water area — STRIP/CORNER puddle types are
                // alpha-cut from a full rect, with dry asphalt in the
                // cut-away corners. Splashes added when the foot lands
                // in a cut-away corner would otherwise pop visibly on
                // dry ground. Distance-from-center fade approximates
                // "stay inside the wet shape" without per-puddle alpha
                // sampling. For infinite-water tiles (rivers/sewers)
                // there is no PUDDLE_Puddle rect — use full lift.
                //
                // Lift-at-center / sink-at-edge tuned by eye:
                //   * Sprite half-extent = 40 (PARTICLE_Add size arg).
                //   * +10  → top of billboard ~50 above water, bottom
                //           ~30 below → most of ripple visible.
                //   * -40 → top at water surface, bottom 80 below →
                //           billboard fully submerged, invisible.
                // Pre-release sources placed the center at ground Y
                // unconditionally → half-submerged everywhere → from an
                // oblique camera only a 1-pixel sliver survived z-test.
                // PieroZ D3D build of the same pre-release sources
                // reproduces our invisibility — confirms it's a
                // pre-release defect MuckyFoot fixed for retail.
                const SLONG PUDDLE_SPLASH_LIFT_Y_CENTER = 10;
                const SLONG PUDDLE_SPLASH_LIFT_Y_EDGE = -40;

                SLONG lift_y = PUDDLE_SPLASH_LIFT_Y_CENTER;
                SLONG px1, pz1, px2, pz2;
                if (PUDDLE_get_rect_at(x, z, &px1, &pz1, &px2, &pz2, nullptr)) {
                    SLONG cx = (px1 + px2) >> 1;
                    SLONG cz = (pz1 + pz2) >> 1;
                    SLONG halfw = (px2 - px1) >> 1;
                    SLONG halfh = (pz2 - pz1) >> 1;
                    if (halfw < 1)
                        halfw = 1;
                    if (halfh < 1)
                        halfh = 1;
                    SLONG dx = SLONG(x) - cx;
                    if (dx < 0)
                        dx = -dx;
                    SLONG dz = SLONG(z) - cz;
                    if (dz < 0)
                        dz = -dz;
                    // Euclidean L2 distance from rect center, normalised
                    // so that axis-aligned rect edge = 1.0 and rect
                    // corners ≈ √2. The puddle's wet area is roughly a
                    // CIRCLE inscribed in the rect (alpha-cut shape) —
                    // corner quadrants are dry asphalt. r ≥ 1 → outside
                    // the inscribed circle, definitely dry, sink fully.
                    float dxn = float(dx) / float(halfw);
                    float dzn = float(dz) / float(halfh);
                    float r = sqrtf(dxn * dxn + dzn * dzn);

                    // Plateau (no sink) up to PLATEAU_R, then smoothstep
                    // sink to full at r=1. Plateau covers the central
                    // ~PLATEAU_R² of the inscribed circle's area —
                    // splash visible across most of the wet shape, only
                    // the rim and corners get sunk. PLATEAU_R = 0.7
                    // ≈ 50% of the disc area always-visible; raising it
                    // gives more visible area but pushes the sink
                    // closer to the dry-corner border.
                    const float PLATEAU_R = 0.7f;

                    float t;
                    if (r <= PLATEAU_R) {
                        t = 0.0f;
                    } else if (r >= 1.0f) {
                        t = 1.0f;
                    } else {
                        float u = (r - PLATEAU_R) / (1.0f - PLATEAU_R);
                        t = u * u * (3.0f - 2.0f * u); // smoothstep
                    }
                    lift_y = SLONG(float(PUDDLE_SPLASH_LIFT_Y_CENTER)
                        + (float(PUDDLE_SPLASH_LIFT_Y_EDGE)
                              - float(PUDDLE_SPLASH_LIFT_Y_CENTER))
                            * t);
                }

                PARTICLE_Add(
                    x << 8, (my + lift_y) << 8, z << 8,
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
