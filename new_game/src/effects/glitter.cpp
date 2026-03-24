#include "engine/platform/uc_common.h"
#include "effects/glitter.h"
#include "effects/glitter_globals.h"

// uc_orig: GLITTER_init (fallen/Source/glitter.cpp)
// Resets the mapwho buckets, marks all glitters unused, and builds the spark free-list.
void GLITTER_init(void)
{
    SLONG i;

    for (i = 0; i < GLITTER_MAPWHO; i++) {
        GLITTER_mapwho[i] = NULL;
    }

    for (i = 0; i < GLITTER_MAX_GLITTER; i++) {
        GLITTER_glitter[i].flag &= ~GLITTER_FLAG_USED;
    }

    GLITTER_glitter_last = 1;

    GLITTER_spark_free = 1;

    for (i = 1; i < GLITTER_MAX_SPARKS - 1; i++) {
        GLITTER_spark[i].next = i + 1;
    }

    GLITTER_spark[GLITTER_MAX_SPARKS - 1].next = 0;
}

// uc_orig: GLITTER_create (fallen/Source/glitter.cpp)
// Allocates a glitter slot and links it into the mapwho z-bucket.
// Returns its index, or NULL if map_z is out of range or no slots are free.
UBYTE GLITTER_create(
    UBYTE flag,
    UBYTE map_x,
    UBYTE map_z,
    ULONG colour)
{
    SLONG i;
    GLITTER_Glitter* gg;

    if (!WITHIN(map_z, 0, GLITTER_MAPWHO - 1)) {
        return NULL;
    }

    // Round-robin search for a free slot
    for (i = 0; i < GLITTER_MAX_GLITTER; i++) {
        GLITTER_glitter_last += 1;

        if (GLITTER_glitter_last >= GLITTER_MAX_GLITTER) {
            GLITTER_glitter_last = 1;
        }

        gg = &GLITTER_glitter[GLITTER_glitter_last];

        if (!(gg->flag & GLITTER_FLAG_USED)) {
            goto found_unused_glitter;
        }
    }

    return NULL;

found_unused_glitter:;

    gg->flag = flag;
    gg->flag |= GLITTER_FLAG_USED;
    gg->flag &= ~GLITTER_FLAG_DESTROY;
    gg->map_x = map_x;
    gg->map_z = map_z;
    gg->red = (colour >> 16) & 0xff;
    gg->green = (colour >> 8) & 0xff;
    gg->blue = (colour >> 0) & 0xff;
    gg->spark = NULL;

    // Prepend to the mapwho z-bucket list
    gg->next = GLITTER_mapwho[map_z];
    GLITTER_mapwho[map_z] = GLITTER_glitter_last;

    return GLITTER_glitter_last;
}

// uc_orig: GLITTER_add (fallen/Source/glitter.cpp)
// Allocates a spark from the free-list and prepends it to the glitter's spark list.
// Velocity is random ±31, with optional directional clamps from glitter flags.
void GLITTER_add(
    UBYTE glitter,
    SLONG x,
    SLONG y,
    SLONG z)
{
    UBYTE spark;

    if (glitter == NULL) {
        return;
    }

    if (GLITTER_spark_free == NULL) {
        return;
    }

    GLITTER_Glitter* gg;

    ASSERT(WITHIN(glitter, 1, GLITTER_MAX_GLITTER - 1));

    gg = &GLITTER_glitter[glitter];

    GLITTER_Spark* gs;

    ASSERT(WITHIN(GLITTER_spark_free, 1, GLITTER_MAX_SPARKS - 1));

    spark = GLITTER_spark_free;
    gs = &GLITTER_spark[spark];
    GLITTER_spark_free = gs->next;

    gs->x = x;
    gs->y = y;
    gs->z = z;

    SLONG dx = (rand() & 0x3f) - 0x1f;
    SLONG dy = (rand() & 0x3f) - 0x1f;
    SLONG dz = (rand() & 0x3f) - 0x1f;

    if (gg->flag & GLITTER_FLAG_DXPOS) {
        dx = +abs(dx);
    }
    if (gg->flag & GLITTER_FLAG_DXNEG) {
        dx = -abs(dx);
    }
    if (gg->flag & GLITTER_FLAG_DYPOS) {
        dy = +abs(dy);
    }
    if (gg->flag & GLITTER_FLAG_DYNEG) {
        dy = -abs(dy);
    }
    if (gg->flag & GLITTER_FLAG_DZPOS) {
        dz = +abs(dz);
    }
    if (gg->flag & GLITTER_FLAG_DZNEG) {
        dz = -abs(dz);
    }

    gs->dx = dx;
    gs->dy = dy;
    gs->dz = dz;

    gs->die = GLITTER_SPARK_LIFE;

    gs->next = gg->spark;
    gg->spark = spark;

    return;
}

// uc_orig: GLITTER_destroy (fallen/Source/glitter.cpp)
// Marks the glitter for deferred destruction; freed in GLITTER_process when all sparks die.
void GLITTER_destroy(UBYTE glitter)
{
    if (glitter == NULL) {
        return;
    }

    ASSERT(WITHIN(glitter, 1, GLITTER_MAX_GLITTER));

    GLITTER_glitter[glitter].flag |= GLITTER_FLAG_DESTROY;
}

// uc_orig: GLITTER_process (fallen/Source/glitter.cpp)
// Per-tick update: reap dead sparks, free destroyed-and-empty glitters,
// then integrate gravity and positions for surviving sparks.
void GLITTER_process()
{
    SLONG i;

    SLONG dy;

    UBYTE spark;
    UBYTE next;
    UBYTE* prev;

    GLITTER_Glitter* gg;
    GLITTER_Spark* gs;

    for (i = 1; i < GLITTER_MAX_GLITTER; i++) {
        gg = &GLITTER_glitter[i];

        if (!(gg->flag & GLITTER_FLAG_USED)) {
            continue;
        }

        // Remove dead sparks from the glitter's list
        next = gg->spark;
        prev = &gg->spark;

        while (1) {
            if (next == 0) {
                break;
            }

            ASSERT(WITHIN(next, 1, GLITTER_MAX_SPARKS - 1));

            gs = &GLITTER_spark[next];

            if (gs->die == 0) {
                spark = next;

                *prev = gs->next;
                next = gs->next;

                gs->next = GLITTER_spark_free;
                GLITTER_spark_free = spark;
            } else {
                prev = &gs->next;
                next = gs->next;
            }
        }

        if (gg->flag & GLITTER_FLAG_DESTROY) {
            if (gg->spark == NULL) {
                // All sparks dead: free this glitter slot
                gg->flag &= ~GLITTER_FLAG_USED;

                // Remove from mapwho z-bucket list
                ASSERT(WITHIN(gg->map_z, 0, GLITTER_MAPWHO - 1));

                prev = &GLITTER_mapwho[gg->map_z];
                next = GLITTER_mapwho[gg->map_z];

                while (1) {
                    if (next == i) {
                        *prev = gg->next;
                        break;
                    } else {
                        ASSERT(WITHIN(next, 1, GLITTER_MAX_GLITTER - 1));

                        prev = &GLITTER_glitter[next].next;
                        next = GLITTER_glitter[next].next;
                    }
                }

                continue;
            }
        }

        // Integrate all surviving sparks: apply gravity, move, decrement lifetime
        for (spark = gg->spark; spark; spark = gs->next) {
            ASSERT(WITHIN(spark, 1, GLITTER_MAX_SPARKS - 1));

            gs = &GLITTER_spark[spark];

            dy = gs->dy;
            dy += GLITTER_SPARK_GRAVITY;

            if (dy < -127) {
                dy = -127;
            }

            gs->dy = dy;

            gs->x += gs->dx >> 3;
            gs->y += gs->dy >> 3;
            gs->z += gs->dz >> 3;

            gs->die -= 1;
        }
    }
}

// uc_orig: GLITTER_get_start (fallen/Source/glitter.cpp)
// Initialises the renderer iterator for the given screen-space z-slice and x range.
void GLITTER_get_start(UBYTE xmin, UBYTE xmax, UBYTE z)
{
    GLITTER_get_z = z;
    GLITTER_get_xmin = xmin;
    GLITTER_get_xmax = xmax;

    if (WITHIN(GLITTER_get_z, 0, GLITTER_MAPWHO - 1)) {
        GLITTER_get_glitter = GLITTER_mapwho[GLITTER_get_z];

        if (GLITTER_get_glitter != NULL) {
            ASSERT(WITHIN(GLITTER_get_glitter, 1, GLITTER_MAX_GLITTER - 1));

            GLITTER_get_spark = GLITTER_glitter[GLITTER_get_glitter].spark;
        }
    } else {
        GLITTER_get_glitter = NULL;
        GLITTER_get_spark = NULL;
    }
}

// uc_orig: GLITTER_get_next (fallen/Source/glitter.cpp)
// Returns the next GLITTER_Info to draw, or NULL when all glitters in the z-slice are exhausted.
// Advances through glitters in the mapwho bucket and sparks within each glitter.
// Spark colour fades linearly from full colour at birth to black at death.
GLITTER_Info* GLITTER_get_next()
{
    GLITTER_Glitter* gg;
    GLITTER_Spark* gs;

tail_recurse:;

    if (GLITTER_get_glitter == NULL) {
        return NULL;
    }

    ASSERT(WITHIN(GLITTER_get_glitter, 1, GLITTER_MAX_GLITTER - 1));

    gg = &GLITTER_glitter[GLITTER_get_glitter];

    if (GLITTER_get_spark == NULL) {
        // Move to next glitter in the bucket
        ASSERT(WITHIN(gg->next, 0, GLITTER_MAX_GLITTER - 1));

        GLITTER_get_glitter = gg->next;
        GLITTER_get_spark = GLITTER_glitter[gg->next].spark;

        goto tail_recurse;
    }

    ASSERT(WITHIN(GLITTER_get_spark, 1, GLITTER_MAX_SPARKS - 1));

    gs = &GLITTER_spark[GLITTER_get_spark];

    // Build line segment endpoints: tail is 1/4 velocity behind current position
    GLITTER_get_info.x1 = gs->x;
    GLITTER_get_info.y1 = gs->y;
    GLITTER_get_info.z1 = gs->z;
    GLITTER_get_info.x2 = gs->x + (gs->dx >> 2);
    GLITTER_get_info.y2 = gs->y + (gs->dy >> 2);
    GLITTER_get_info.z2 = gs->z + (gs->dz >> 2);

    // Fade colour linearly from birth to death
    SLONG red = (gg->red * gs->die) / GLITTER_SPARK_LIFE;
    SLONG green = (gg->green * gs->die) / GLITTER_SPARK_LIFE;
    SLONG blue = (gg->blue * gs->die) / GLITTER_SPARK_LIFE;

    GLITTER_get_info.colour = (red << 16) | (green << 8) | (blue << 0);

    GLITTER_get_spark = gs->next;

    return &GLITTER_get_info;
}
