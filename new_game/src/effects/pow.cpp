#include "engine/platform/platform.h"
#include "missions/game_types.h"
#include "effects/pow.h"
#include "effects/pow_globals.h"
#include "engine/core/fmatrix.h"

// Debug check stub — originally scanned for list corruption; empty in pre-release.
// uc_orig: check_pows (fallen/Source/pow.cpp)
void check_pows(void)
{
    SLONG sprite; // unused — pre-release debug stub
    (void)sprite;
}

// Inserts a new sprite into the linked list of the given POW.
// dx/dy/dz should NOT be pre-shifted by POW_DELTA_SHIFT — this function does the shift.
// uc_orig: POW_insert_sprite (fallen/Source/pow.cpp)
static void POW_insert_sprite(
    POW_Pow* pp,
    SLONG x, SLONG y, SLONG z,
    SLONG dx, SLONG dy, SLONG dz,
    SLONG frame_speed,
    SLONG damp)
{
    SLONG sprite_index;
    POW_Sprite* ps;

    if (POW_sprite_free == NULL) {
        return;
    }

    ASSERT(WITHIN(POW_sprite_free, 1, POW_MAX_SPRITES - 1));

    // Debug: verify the free slot is not already in this POW's list.
    {
        SLONG sprite;
        sprite = pp->sprite;
        while (sprite) {
            ASSERT(sprite != POW_sprite_free);
            sprite = POW_sprite[sprite].next;
        }
    }

    ps = &POW_sprite[POW_sprite_free];
    sprite_index = POW_sprite_free;
    POW_sprite_free = ps->next;

    ps->x = x;
    ps->y = y;
    ps->z = z;
    ps->dx = dx >> POW_DELTA_SHIFT;
    ps->dy = dy >> POW_DELTA_SHIFT;
    ps->dz = dz >> POW_DELTA_SHIFT;
    ps->frame = 0;
    ps->frame_counter = POW_MAX_FRAME_SPEED;
    ps->frame_speed = frame_speed;
    ps->damp = damp;

    ps->next = pp->sprite;
    pp->sprite = sprite_index;
}

// uc_orig: POW_init (fallen/Source/pow.cpp)
void POW_init()
{
    SLONG i;
    // uc_orig: check_pows forward declaration inside POW_init (fallen/Source/pow.cpp)
    void check_pows(void);
    check_pows();

    memset(POW_sprite, 0, sizeof(POW_sprite));
    memset(POW_pow, 0, sizeof(POW_pow));
    memset(POW_mapwho, 0, PAP_SIZE_LO * sizeof(POW_mapwho[0]));

    for (i = 1; i < POW_MAX_SPRITES - 1; i++) {
        POW_sprite[i].next = i + 1;
    }

    POW_sprite[POW_MAX_SPRITES - 1].next = NULL;

    for (i = 1; i < POW_MAX_POWS - 1; i++) {
        POW_pow[i].next = i + 1;
    }

    POW_pow[POW_MAX_POWS - 1].next = NULL;

    POW_sprite_free = 1;
    POW_pow_free = 1;
}

// uc_orig: POW_new (fallen/Source/pow.cpp)
void POW_new(SLONG type, SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz)
{
    SLONG i;

    SLONG yaw;
    SLONG pitch;

    SLONG around;
    SLONG upndown;
    SLONG ring;

    SLONG dyaw;
    SLONG dpitch;

    SLONG pow_index;
    SLONG framespeed;

    SLONG vector[3];

    POW_Type* pt;
    POW_Pow* pp;

    ASSERT(WITHIN(type, 0, POW_TYPE_NUMBER - 1));

    pt = &POW_type[type];

    if (POW_pow_free == NULL) {
        return;
    }

    ASSERT(WITHIN(POW_pow_free, 1, POW_MAX_POWS - 1));

    pp = &POW_pow[POW_pow_free];
    pow_index = POW_pow_free;
    POW_pow_free = pp->next;

    pp->type = type;
    pp->timer = pt->life;
    pp->x = x;
    pp->y = y;
    pp->z = z;
    pp->dx = dx >> POW_DELTA_SHIFT;
    pp->dy = dy >> POW_DELTA_SHIFT;
    pp->dz = dz >> POW_DELTA_SHIFT;
    pp->mapwho = NULL;
    pp->next = NULL;
    pp->sprite = NULL;
    pp->flag = 0;
    pp->time_warp = Random();

    around = 5 + pt->density * 3;
    upndown = 4 + pt->density * 1;

    framespeed = 96 + pt->framespeed * 32;

    if (pt->arrange == POW_ARRANGE_CIRCLE) {
        // Circle arrangement — no sprites generated in pre-release.
    } else {
        // Sphere or semisphere arrangement.
        if (pt->arrange == POW_ARRANGE_SPHERE) {
            dpitch = 1024 / upndown;
            pitch = -512 + dpitch / 2;
        } else {
            dpitch = 512 / ((upndown + 1) / 2);
            pitch = dpitch / 2;
        }

        while (pitch < 512) {
            // Number of sprites in this latitude ring.
            ring = COS(pitch & 2047);
            ring *= around;
            ring >>= 16;

            dyaw = 2048 / ring;
            yaw = 0;

            for (i = 0; i < ring; i++) {
                FMATRIX_vector(vector, yaw, pitch);

                POW_insert_sprite(
                    pp,
                    x, y, z,
                    (vector[0] + (Random() & 0x3fff) >> (pt->speed + 1)) + dx,
                    (vector[1] + (Random() & 0x3fff) >> (pt->speed + 1)) + dy,
                    (vector[2] + (Random() & 0x3fff) >> (pt->speed + 1)) + dz,
                    framespeed + (Random() & 0x3f),
                    pt->damp + 1);

                yaw += dyaw;
            }

            pitch += dpitch;
        }
    }

    pp->next = POW_pow_used;
    POW_pow_used = pow_index;
}

// uc_orig: POW_process (fallen/Source/pow.cpp)
void POW_process()
{
    SLONG i;
    SLONG j;
    SLONG x;
    SLONG y;
    SLONG z;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG pow;
    SLONG yaw;
    SLONG pitch;
    SLONG sprite;
    SLONG vector[3];

    UBYTE next;
    UBYTE* prev;

    POW_Pow* pp;
    POW_Sprite* ps;
    POW_Type* pt;

    SLONG ticks;
    SLONG frame_ticks;
    SLONG frame_counter;
    SLONG pow_index;
    SLONG sprite_index;

    SLONG count2 = 0;

    ticks = (POW_TICKS_PER_SECOND / 20) * TICK_RATIO >> TICK_SHIFT;

    // Animate and move sprites for each active POW.
    for (pow = POW_pow_used; pow && count2++ < 50; pow = pp->next) {
        ASSERT(WITHIN(pow, 1, POW_MAX_POWS - 1));

        pp = &POW_pow[pow];

        if (pp->sprite) {
            SLONG count = 0;

            for (sprite = pp->sprite; sprite && count++ < 256; sprite = ps->next) {
                ASSERT(WITHIN(sprite, 1, POW_MAX_SPRITES - 1));

                ps = &POW_sprite[sprite];

                // Advance animation frame.
                frame_ticks = ticks * ps->frame_speed >> 8;
                frame_counter = ps->frame_counter;
                frame_counter -= frame_ticks;

                while (frame_counter <= 0) {
                    frame_counter += POW_MAX_FRAME_SPEED;
                    ps->frame += 1;
                }

                ps->frame_counter = frame_counter;

                // Move sprite.
                ps->x += ps->dx << POW_DELTA_SHIFT;
                ps->y += ps->dy << POW_DELTA_SHIFT;
                ps->z += ps->dz << POW_DELTA_SHIFT;

                // Damp velocity.
                ps->dx -= ps->dx >> ps->damp;
                ps->dy -= ps->dy >> ps->damp;
                ps->dz -= ps->dz >> ps->damp;
            }

            // Retire dead sprites (frame >= 16).
            next = pp->sprite;
            prev = &pp->sprite;

            count = 0;
            while (count++ < 256) {
                if (next == NULL) {
                    break;
                }

                ASSERT(WITHIN(next, 1, POW_MAX_SPRITES - 1));

                ps = &POW_sprite[next];

                if (ps->frame >= 16) {
                    sprite_index = next;
                    *prev = ps->next;
                    next = ps->next;

                    ps->next = POW_sprite_free;
                    POW_sprite_free = sprite_index;
                } else {
                    prev = &ps->next;
                    next = ps->next;
                }
            }

            // Safety: reset if we hit the guard — indicates list corruption.
            if (count >= 256) {
                POW_init();
                return;
            }
        }

        if (pp->timer <= ticks) {
            pp->timer = 0;
        } else {
            pp->timer -= ticks;
        }
    }

    if (count2 >= 50) {
        POW_init();
        return;
    }

    // Retire dead POWs (no sprites and timer expired).
    next = POW_pow_used;
    prev = &POW_pow_used;

    count2 = 0;
    while (count2++ < 50) {
        if (next == NULL) {
            break;
        }

        ASSERT(WITHIN(next, 1, POW_MAX_POWS - 1));

        pp = &POW_pow[next];

        if (pp->sprite == NULL && pp->timer == NULL) {
            pow_index = next;
            *prev = pp->next;
            next = pp->next;

            pp->next = POW_pow_free;
            POW_pow_free = pow_index;

            pp->type = POW_TYPE_UNUSED;
        } else {
            prev = &pp->next;
            next = pp->next;
        }
    }

    if (count2 >= 50) {
        POW_init();
        return;
    }

    // Trigger child POW spawning for each active POW that has passed a spawn threshold.
    for (i = 1; i < POW_MAX_POWS; i++) {
        pp = &POW_pow[i];

        if (pp->type == POW_TYPE_UNUSED) {
            continue;
        }

        ASSERT(WITHIN(pp->type, 1, POW_TYPE_NUMBER - 1));

        pt = &POW_type[pp->type];

        for (j = 0; j < 3; j++) {
            if (pt->spawn[j].when) {
                if (pt->spawn[j].when > pp->timer) {
                    if (!(pp->flag & (1 << j))) {
                        pp->flag |= 1 << j;

                        x = pp->x;
                        y = pp->y;
                        z = pp->z;

                        if (pt->spawn[j].flag & POW_SPAWN_FLAG_MIDDLE) {
                            dx = 0;
                            dy = 0;
                            dz = 0;
                        } else {
                            yaw = Random() & 2047;
                            pitch = Random() & 255;
                            pitch -= 128;

                            if (POW_type[pp->type].arrange == POW_ARRANGE_SEMISPHERE) {
                                pitch = abs(pitch);
                                pitch += 128;
                            }

                            pitch &= 2047;

                            FMATRIX_vector(vector, yaw, pitch);

                            dx = vector[0];
                            dy = vector[1];
                            dz = vector[2];

                            dx >>= 2;
                            dy >>= 2;
                            dz >>= 2;

                            if (pt->spawn[j].flag & POW_SPAWN_FLAG_FAR_OFF) {
                                dx += dx >> 1;
                                dy += dy >> 1;
                                dz += dz >> 1;
                            }

                            x += dx;
                            y += dy;
                            z += dz;
                        }

                        if (pt->spawn[j].flag & POW_SPAWN_FLAG_AWAY) {
                            dx >>= 2;
                            dy >>= 2;
                            dz >>= 2;
                        } else {
                            dx = 0;
                            dy = 0;
                            dz = 0;
                        }

                        dx += (Random() & 0xfff) - 0x7ff;
                        dy += (Random() & 0xfff) - 0x7ff;
                        dz += (Random() & 0xfff) - 0x7ff;

                        POW_new(pt->spawn[j].type, x, y, z, dx, dy, dz);
                    }
                }
            }
        }
    }
}
