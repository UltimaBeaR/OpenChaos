#include "engine/platform/platform.h"
#include "missions/game_types.h" // for GAME_TURN macro
#include "effects/fire.h"
#include "effects/fire_globals.h"

// uc_orig: FIRE_num_flames_for_size (fallen/Source/fire.cpp)
// Maps size to target flame count: size>>2 (0→0, 4→1, …, 255→63).
static UBYTE FIRE_num_flames_for_size(UBYTE size)
{
    return size >> 2;
}

// uc_orig: FIRE_add_flame (fallen/Source/fire.cpp)
// Allocates one flame from the global free-list and prepends it to the given fire's flame list.
// Flame placement: random dx/dz with center flames getting more control points (taller flame).
static void FIRE_add_flame(FIRE_Fire* ff)
{
    SLONG i;
    SLONG dx;
    SLONG dz;
    SLONG dist;
    UBYTE flame;

    ASSERT(WITHIN(ff, &FIRE_fire[0], &FIRE_fire[FIRE_MAX_FIRE - 1]));

    FIRE_Flame* fl;

    if (FIRE_flame_free == NULL) {
        return;
    }

    ASSERT(WITHIN(FIRE_flame_free, 1, FIRE_MAX_FLAMES - 1));

    flame = FIRE_flame_free;
    fl = &FIRE_flame[FIRE_flame_free];
    FIRE_flame_free = fl->next;

    dx = (Random() & 0xff) - 0x1f;
    dz = (Random() & 0xff) - 0x1f;
    dist = abs(dx) + abs(dz);

    fl->dx = dx;
    fl->dz = dz;
    fl->height = 255 - dist;
    fl->counter = 0;
    fl->die = 32 + (Random() & 0x1f);
    fl->points = (fl->height >> 6) + 1;

    SATURATE(fl->points, 2, FIRE_MAX_FLAME_POINTS);

    for (i = 0; i < fl->points; i++) {
        fl->angle[i] = Random();
        fl->offset[i] = Random();
    }

    fl->next = ff->next;
    ff->next = flame;
    ff->num += 1;
}

// uc_orig: FIRE_init (fallen/Source/fire.cpp)
// One-time initialisation: build flame free-list, mark all fire slots as unused.
void FIRE_init()
{
    SLONG i;

    FIRE_flame_free = 1;

    for (i = 1; i < FIRE_MAX_FLAMES; i++) {
        FIRE_flame[i].next = i + 1;
    }

    FIRE_flame[FIRE_MAX_FLAMES - 1].next = 0;

    FIRE_fire_last = 0;

    for (i = 0; i < FIRE_MAX_FIRE; i++) {
        FIRE_fire[i].num = 0;
    }
}

// uc_orig: FIRE_create (fallen/Source/fire.cpp)
// Spawns a new fire at a world position. size = initial intensity; life = burn-out rate.
// Uses round-robin allocation over 8 slots; silently drops if all 8 are in use.
void FIRE_create(
    UWORD x,
    SWORD y,
    UWORD z,
    UBYTE size,
    UBYTE life)
{
    SLONG i;
    FIRE_Fire* ff;

    for (i = 0; i < FIRE_MAX_FIRE; i++) {
        FIRE_fire_last += 1;

        if (FIRE_fire_last >= FIRE_MAX_FIRE) {
            FIRE_fire_last = 0;
        }

        if (!FIRE_fire[FIRE_fire_last].num) {
            goto found_spare_fire_structure;
        }
    }

    return;

found_spare_fire_structure:;

    ff = &FIRE_fire[FIRE_fire_last];

    ff->x = x;
    ff->y = y;
    ff->z = z;
    ff->size = size;
    ff->shrink = life;
    ff->num = 0;

    FIRE_add_flame(ff);
}

// uc_orig: FIRE_process (fallen/Source/fire.cpp)
// Per-tick update for all active fires.
// Phase 1: animate control points (odd/even points spin opposite directions).
// Phase 2: reap expired flames, return them to the free-list.
// Phase 3: every 4 ticks, reduce size by shrink; replenish one flame if below target density.
void FIRE_process()
{
    SLONG i;
    SLONG j;
    UBYTE flame;
    UBYTE next;
    UBYTE* prev;

    FIRE_Fire* ff;
    FIRE_Flame* fl;

    for (i = 0; i < FIRE_MAX_FIRE; i++) {
        ff = &FIRE_fire[i];

        if (ff->num) {
            // Phase 1: animate control points
            for (next = ff->next; next; next = fl->next) {
                ASSERT(WITHIN(next, 1, FIRE_MAX_FLAMES - 1));

                fl = &FIRE_flame[next];

                fl->counter += 1;

                // Skip point [0] (base anchor), animate [1..points-1]
                for (j = 1; j < fl->points; j++) {
                    if (j & 1) {
                        // Odd points: clockwise drift with contracting offset
                        fl->angle[j] += 31;
                        fl->offset[j] -= 17;
                    } else {
                        // Even points: counter-clockwise drift with expanding offset
                        fl->angle[j] -= 33;
                        fl->offset[j] += 21;
                    }
                }
            }

            // Phase 2: reap expired flames
            prev = &ff->next;
            next = ff->next;

            while (next) {
                ASSERT(WITHIN(next, 1, FIRE_MAX_FLAMES - 1));

                fl = &FIRE_flame[next];

                if (fl->counter >= fl->die) {
                    flame = next;

                    *prev = fl->next;
                    next = fl->next;

                    fl->next = FIRE_flame_free;
                    FIRE_flame_free = flame;

                    ff->num -= 1;
                } else {
                    prev = &fl->next;
                    next = fl->next;
                }
            }

            // Phase 3: burn down every 4 ticks; replenish one flame if under target
            if ((GAME_TURN & 0x3) == 0) {
                SLONG size;

                size = ff->size;
                size -= ff->shrink;

                if (size < 0) {
                    size = 0;
                }

                ff->size = size;
            }

            if (ff->num < FIRE_num_flames_for_size(ff->size)) {
                FIRE_add_flame(ff);
            }
        }
    }
}

// uc_orig: FIRE_get_start (fallen/Source/fire.cpp)
// Initialise renderer iterator with screen-space filter parameters.
void FIRE_get_start(UBYTE z, UBYTE xmin, UBYTE xmax)
{
    FIRE_get_fire_upto = 0;
    FIRE_get_flame = NULL;
    FIRE_get_z = z;
    FIRE_get_xmin = xmin;
    FIRE_get_xmax = xmax;
}

