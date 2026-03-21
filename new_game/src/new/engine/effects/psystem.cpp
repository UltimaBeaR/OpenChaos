// Particle system — fixed-size pool of 2048 Particle structs, used for smoke, sparks, blood, etc.
// Pool is split into a doubly-linked "used" list (iterated backwards) and a singly-linked "free" list.
// particles[0] is a permanent sentinel so that the used-list always has a non-null head.

#include <MFStdLib.h>
#include "fallen/Headers/Game.h"   // Temporary: CLASS_PERSON, GAME_TURN, Thing, GameCoord, THING_find_sphere, TO_THING, TICK_RATIO, TICK_SHIFT, NORMAL_TICK_TOCK, Random, QDIST2, WITHIN
#include "fallen/Headers/mav.h"    // Temporary: MAV_inside (referenced in comments only; not actually called)
#include "core/fmatrix.h"          // FMATRIX_calc, FMATRIX_TRANSPOSE, FMATRIX_MUL
#include "engine/graphics/pipeline/poly.h"  // POLY_PAGE_FLAMES2, POLY_PAGE_STEAM, POLY_PAGE_SMOKECLOUD2
#include "fallen/Headers/Sound.h"  // Temporary: PainSound
#include "fallen/Headers/pcom.h"   // Temporary: PCOM_BENT_PLAYERKILL
#include "fallen/Headers/pow.h"    // Temporary: PYRO_create, PYRO_WHOOMPH, create_shockwave
#include "fallen/Headers/fc.h"     // Temporary: FC_cam
#include "fallen/Headers/animate.h" // Temporary: DT_MESH, DT_VEHICLE, DT_TWEEN, DT_ROT_MULTI, CLASS_BIKE
#include "fallen/Headers/interact.h" // Temporary: calc_sub_objects_position, SUB_OBJECT_LEFT_FOOT, SUB_OBJECT_HEAD

#include "engine/effects/psystem_globals.h"
#include "world/map/pap.h"  // PAP_calc_map_height_at

// fire_pal is a 256-entry RGB palette (768 bytes) loaded from data\flames1.pal by figure.cpp.
// Alpha value of a PFLAG_FIRE particle indexes into the palette to give a fire colour gradient.
// uc_orig: fire_pal (fallen/DDEngine/Source/figure.cpp)
extern UBYTE fire_pal[768];

// uc_orig: GAMEMENU_menu_type (fallen/Source/gamemenu.cpp)
extern SLONG GAMEMENU_menu_type;

// uc_orig: is_person_dead (fallen/Source/Person.cpp)
extern SLONG is_person_dead(Thing* p_person);

// uc_orig: set_person_dead (fallen/Source/Person.cpp)
extern void set_person_dead(Thing* p, Thing* killer, SLONG death_type, SLONG behind, SLONG height);

// uc_orig: PARTICLE_Reset (fallen/Source/psystem.cpp)
void PARTICLE_Reset()
{
    SLONG c0;

    memset((UBYTE*)particles, 0, sizeof(particles));
    particle_count = 0;

    for (c0 = 0; c0 < PSYSTEM_MAX_PARTICLES; c0++) {
        particles[c0].next = c0 + 1;
        particles[c0].prev = c0 - 1;
    }
    particles[PSYSTEM_MAX_PARTICLES - 1].next = 0;
    // particles[0] is sacrificed as sentinel; real allocations start at index 1
    next_free = 1;
    next_used = 0;
    prev_tick = GetTickCount();
    first_pass = 1;
}

// uc_orig: PARTICLE_Run (fallen/Source/psystem.cpp)
void PARTICLE_Run()
{
    SLONG ctr, tx, ty, tz;
    Particle* p;
    SLONG trans;
    UBYTE* palptr;
    SLONG palndx;
    UBYTE isWare;

    // Special case: keep near-ground particles alive when the player is carrying a weapon,
    // so bullet-trail particles don't pop out near the ground.
    isWare = (FC_cam->focus->Class == CLASS_PERSON && FC_cam->focus->Genus.Person->Ware);

    // local_ratio: fixed-point elapsed-time ratio. 1<<TICK_SHIFT = one normal tick elapsed.
    // ULONG avoids a negative ratio on very fast frames (signed overflow in original).
    ULONG local_ratio, local_shift;
    SLONG cur_tick, tick_diff;

    cur_tick = GetTickCount();
    tick_diff = cur_tick - prev_tick;

    if (first_pass) {
        tick_diff = NORMAL_TICK_TOCK; // pretend exactly one tick elapsed on first call
        first_pass = 0;
    }
    local_shift = TICK_SHIFT;
    local_ratio = (tick_diff << local_shift) / (NORMAL_TICK_TOCK);

    if (local_ratio < 256) {
        // Frame was too short — skip logic updates this frame; position still integrates via constant TICK_RATIO.
        local_ratio = 0;
    } else
        prev_tick = cur_tick;

    if (!particle_count)
        return;

    // Walk backwards through the used list: next_used → ... → 0 (sentinel).
    // Backwards traversal lets us safely unlink the current particle during iteration.
    for (p = particles + next_used; p != particles;)
    {
        // POSITION INTEGRATION — uses fixed TICK_RATIO, runs every frame for smooth motion at high fps.
        tx = p->x + ((p->dx * TICK_RATIO) >> TICK_SHIFT);
        ty = p->y + ((p->dy * TICK_RATIO) >> TICK_SHIFT);
        tz = p->z + ((p->dz * TICK_RATIO) >> TICK_SHIFT);

        // Gravity: apply constant downward acceleration to dy each frame (no terminal velocity).
        if (p->flags & PFLAG_GRAVITY) {
            p->dy -= (0xff * TICK_RATIO) >> TICK_SHIFT;
        }

        if (p->flags & PFLAG_SPRITEANI) {
            p->sprite += 4;
        }

        // LOGICAL-TICK GATE — only run simulation updates when a full tick's worth of time has passed.
        if (local_ratio > 255) {
            // PFLAG_WANDER: random velocity perturbation each tick; used for smoke billowing.
            if (p->flags & PFLAG_WANDER) {
                p->dx += ((rand() & 0x1f) - 0xf) * 4;
                p->dy += ((rand() & 0x1f) - 0xf) * 4;
                p->dz += ((rand() & 0x1f) - 0xf) * 4;
            }

            // PFLAG_DRIFT: sinusoidal drift — unimplemented, code was commented out in original.
            if (p->flags & PFLAG_DRIFT) {
                // (unimplemented in original)
            }

            // PFLAG_DAMPING: horizontal friction only (Y not damped); factor ~0.957 per tick.
            if (p->flags & PFLAG_DAMPING) {
                p->dx = (p->dx * 245) >> 8;
                p->dz = (p->dz * 245) >> 8;
            }

            // PFLAG_FADE2: alpha fade that runs only on odd life values — effectively half the rate of PFLAG_FADE.
            if ((p->flags & PFLAG_FADE2) && (p->life & 1)) {
                ULONG diff = 0x01000000 * ((p->fade * local_ratio) >> local_shift);
                if (p->flags & PFLAG_INVALPHA) {
                    if ((p->colour & 0xFF000000) < 0xFF000000 - diff)
                        p->colour += diff;
                    else
                        p->life = 1;
                } else {
                    if ((p->colour & 0xFF000000) > diff)
                        p->colour -= diff;
                    else
                        p->life = 1;
                }
            }

            // PFLAG_FADE: standard per-tick alpha fade; kills particle when fully transparent.
            if (p->flags & PFLAG_FADE) {
                ULONG diff = 0x01000000 * ((p->fade * local_ratio) >> local_shift);
                if (p->flags & PFLAG_INVALPHA) {
                    if ((p->colour & 0xFF000000) < 0xFF000000 - diff)
                        p->colour += diff;
                    else
                        p->life = 1;
                } else {
                    if ((p->colour & 0xFF000000) > diff)
                        p->colour -= diff;
                    else
                        p->life = 1;
                }
            }

            // PFLAG_FIRE: colour cycles through the fire palette based on alpha value.
            // As alpha decreases (particle fades), colour shifts from white-hot to dark red.
            if (p->flags & PFLAG_FIRE) {
                trans = (p->colour & 0xFF000000) >> 24;
                palptr = (trans * 3) + fire_pal;
                palndx = (256 - trans) * 3;
                trans <<= 24;
                p->colour = trans + (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
            }

            // PFLAG_BOUNCE: reflects off terrain; energy loss factor ~0.703.
            // When nearly stopped (|dy| < 256), also applies horizontal friction.
            if (p->flags & PFLAG_BOUNCE) {
                SLONG tmpy = PAP_calc_map_height_at(tx >> 8, tz >> 8) << 8;
                if (ty < tmpy) {
                    ty -= tmpy;
                    ty = tmpy - ty;
                    p->dy = -(p->dy * 180) >> 8;
                    if (abs(p->dy) < 256) {
                        p->dx = (p->dx * 180) >> 8;
                        p->dz = (p->dz * 180) >> 8;
                    }
                }
            } else
                // Non-bouncing ground collision: kill particle when it goes below terrain height.
                // isWare exception: bullet-trail particles stay alive near the ground when player is armed.
                if ((ty >> 8) < PAP_calc_map_height_at(tx >> 8, tz >> 8)) {
                    if (p->flags & PFLAG_COLLIDE) {
                        // PFLAG_COLLIDE: stub, not implemented in original
                    } else {
                        if (!isWare)
                            p->life = 1;
                    }

                    // PFLAG_EXPLODE_ON_IMPACT: spawns a PYRO_WHOOMPH explosion on ground hit.
                    // Shockwave radius 0x140 (320 world units), strength 80.
                    if (p->flags & PFLAG_EXPLODE_ON_IMPACT) {
                        GameCoord pos = { tx, 0, tz };
                        pos.Y = PAP_calc_map_height_at(tx >> 8, tz >> 8) << 8;
                        pos.Y += 1024;
                        Thing* pyro = PYRO_create(pos, PYRO_WHOOMPH);
                        if (pyro)
                            pyro->Genus.Pyro->counter = 0;

                        create_shockwave(
                            tx >> 8,
                            ty >> 8,
                            tz >> 8,
                            0x140,
                            80,
                            NULL);
                    }
                }

            // PFLAG_LEAVE_TRAIL: spawns two child fire particles per tick at the current position.
            // Used for flaming projectile trails (e.g. Molotov in flight).
            if (p->flags & PFLAG_LEAVE_TRAIL) {
                PARTICLE_Add(
                    tx + (Random() & 0x9fff) - 0x4fff,
                    ty + (Random() & 0x9fff) - 0x4fff,
                    tz + (Random() & 0x9fff) - 0x4fff,
                    0, 0, 0,
                    POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE,
                    300, 40, 1, 3, -1);
                PARTICLE_Add(
                    tx + (Random() & 0x4fff) - 0x27ff,
                    ty + (Random() & 0x4fff) - 0x27ff,
                    tz + (Random() & 0x4fff) - 0x27ff,
                    p->dx + (Random() & 0xfff) - 0x7ff >> 3,
                    p->dy + (Random() & 0xfff) - 0x7ff >> 3,
                    p->dz + (Random() & 0xfff) - 0x7ff >> 3,
                    POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE,
                    300, 60, 1, 2, -1);
            }

            // PFLAG_RESIZE: grow/shrink size each tick. Kills particle if size drops to 0.
            // resize is signed: positive = grow, negative = shrink.
            if (p->flags & PFLAG_RESIZE) {
                SLONG temp = p->size;
                temp += (p->resize * local_ratio) >> local_shift;
                if (temp < 1) {
                    temp = 1;
                    p->life = 1;
                }
                if (temp > 255)
                    temp = 255;
                p->size = temp;
            }

            // PFLAG_RESIZE2: same as PFLAG_RESIZE but size is 16-bit fixed-point (pre-shifted <<8 at spawn).
            if (p->flags & PFLAG_RESIZE2) {
                SLONG temp = p->size;
                temp += (p->resize * local_ratio) >> local_shift;
                if (temp < 1)
                    p->life = 1;
                SATURATE(temp, 1, 65535);
                p->size = temp;
            }

            // PFLAG_HURTPEOPLE: deals fire damage to nearby persons every other tick.
            // Sphere query radius 0xa0, actual hit radius 0x40.
            if (p->flags & PFLAG_HURTPEOPLE) {
                if (GAME_TURN & 0x1) // throttle to every other tick
                {
                    SLONG i;
                    UWORD hurt[8];
                    SLONG num;
                    THING_INDEX i_hurt;

                    num = THING_find_sphere(
                        p->x >> 8,
                        p->y >> 8,
                        p->z >> 8,
                        0xa0,
                        hurt,
                        8,
                        1 << CLASS_PERSON);

                    for (i = 0; i < num; i++) {
                        Thing* p_hurt = TO_THING(hurt[i]);

                        if (is_person_dead(p_hurt)) {
                            // Don't hurt dead people.
                        } else {
                            SLONG dx = abs(p->x - p_hurt->WorldPos.X >> 8);
                            SLONG dz = abs(p->z - p_hurt->WorldPos.Z >> 8);
                            SLONG dist = QDIST2(dx, dz);

                            if (abs(dist) < 0x40) {
                                SLONG junkx;
                                SLONG junkz;
                                SLONG ytop;
                                SLONG ybot;

                                calc_sub_objects_position(
                                    p_hurt,
                                    0,
                                    SUB_OBJECT_LEFT_FOOT,
                                    &junkx,
                                    &ybot,
                                    &junkz);

                                calc_sub_objects_position(
                                    p_hurt,
                                    0,
                                    SUB_OBJECT_HEAD,
                                    &junkx,
                                    &ytop,
                                    &junkz);

                                ybot <<= 8;
                                ytop <<= 8;

                                ybot += p_hurt->WorldPos.Y;
                                ytop += p_hurt->WorldPos.Y;

                                if (WITHIN(p->y, ybot - 0x2000, ytop + 0x2000)) {
                                    PainSound(p_hurt);

                                    if ((p_hurt->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) || (p_hurt->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL)) {
                                        // Nothing hurts this person.
                                    } else {
                                        p_hurt->Genus.Person->Health -= (0x40 - dist >> 4) + 1;
                                    }

                                    if (p_hurt->Genus.Person->Health <= 0) {
                                        set_person_dead(
                                            p_hurt,
                                            NULL,
                                            PERSON_DEATH_TYPE_OTHER,
                                            FALSE,
                                            FALSE);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        } // local_ratio > 255

        p->x = tx;
        p->y = ty;
        p->z = tz;

        // Lifetime decrement: life -= elapsed ticks (fractional on fast frames).
        // "ending too late is Real Bad" — leaked particles stay in the used list indefinitely.
        p->life -= local_ratio >> local_shift;

        // Recycle dead particle: unlink from used list, prepend to free list.
        if (p->life <= 0) {
            SWORD idx = p - particles, temp = p->prev;
            p->priority = 0;
            particle_count--;

            // Unlink from used list.
            if (p->next)
                particles[p->next].prev = p->prev;
            if (p->prev)
                particles[p->prev].next = p->next;
            if (next_used == idx) {
                next_used = p->prev;
                particles[next_used].next = 0;
            }
            p->prev = 0;

            // Prepend to free list.
            particles[next_free].prev = idx;
            p->next = next_free;
            next_free = idx;

            p = particles + temp; // advance iterator: temp was saved before unlink
        } else
            p = particles + p->prev;
    }
}

// uc_orig: PARTICLE_AddParticle (fallen/Source/psystem.cpp)
UWORD PARTICLE_AddParticle(Particle& p)
{
    UWORD new_particle;

    // No new particles during pause/menu — prevents effects accumulating while frozen.
    if (GAMEMENU_menu_type != 0 /*GAMEMENU_MENU_TYPE_NONE*/) {
        return 0;
    }

    if (particle_count == PSYSTEM_MAX_PARTICLES)
        return 0; // pool exhausted — silently drop

    if (!next_free)
        return 0; // free list empty (shouldn't happen if count check passed)

    // PFLAG_RESIZE2 init: pre-shift size into 16-bit fixed-point; auto-compute shrink rate if not set.
    if (p.flags & PFLAG_RESIZE2) {
        p.size <<= 8;
        if (!p.resize) {
            p.resize = -(p.size / p.life);
        }
    }

    // Pop from free list.
    new_particle = next_free;
    next_free = particles[next_free].next;
    particles[next_free].prev = 0;

    particles[new_particle] = p;
    particles[new_particle].next = 0;
    particles[new_particle].priority = 1;

    // Append to tail of used list.
    particles[next_used].next = new_particle;
    particles[new_particle].prev = next_used;
    next_used = new_particle;

    return ++particle_count;
}

// uc_orig: PARTICLE_Add (fallen/Source/psystem.cpp)
UWORD PARTICLE_Add(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, UWORD page, UWORD sprite, SLONG colour, SLONG flags, SLONG life, UBYTE size, UBYTE priority, SBYTE fade, SBYTE resize)
{
    Particle p;
    p.x = x;
    p.y = y;
    p.z = z;
    p.dx = dx;
    p.dy = dy;
    p.dz = dz;
    p.page = page;
    p.sprite = sprite;
    p.colour = colour;
    p.flags = flags;
    p.life = life;
    p.size = size;
    p.priority = priority;
    p.resize = resize;
    p.fade = fade;
    return PARTICLE_AddParticle(p);
}

// uc_orig: PARTICLE_Exhaust (fallen/Source/psystem.cpp)
UWORD PARTICLE_Exhaust(SLONG x, SLONG y, SLONG z, UBYTE density, UBYTE disperse)
{
    UBYTE i;
    UWORD res;
    Particle p;

    res = 1;
    p.page = POLY_PAGE_STEAM;
    p.sprite = 1 + ((rand() & 3) << 2);
    p.colour = 0x7FFFFFFF;
    p.flags = PFLAGS_SMOKE | PFLAG_WANDER;
    p.life = 50;
    p.priority = 1;
    p.size = 4;
    p.resize = 6;
    p.fade = disperse;
    p.dx = 0;
    p.dy = 0;
    p.dz = 0;
    for (i = density; i && res; i--) {
        p.x = x + (rand() & 0xff) - 0x7f;
        p.y = y + (rand() & 0xff) - 0x7f;
        p.z = z + (rand() & 0xff) - 0x7f;
        res = PARTICLE_AddParticle(p);
    }
    return res;
}

// Direction-aware exhaust for moving vehicles and bikes.
// Computes exhaust direction from the object's orientation matrix.
// vel = 1024 - (object->Velocity * 128): faster objects produce shorter exhaust blasts.
// Bike special case: BIKE_get_speed() ignores direction, so vel is forced to 0.
// uc_orig: PARTICLE_Exhaust2 (fallen/Source/psystem.cpp)
UWORD PARTICLE_Exhaust2(Thing* object, UBYTE density, UBYTE disperse)
{
    UBYTE i;
    UWORD res;
    Particle p;
    SLONG x, y, z, dx, dy, dz, ox, oy, oz;
    SLONG matrix[9], vector[3];
    SLONG vel;

    vel = 1024 - (object->Velocity * 128);
    switch (object->DrawType) {
    case DT_MESH:
    case DT_VEHICLE:
        if (object->Class == CLASS_BIKE) {
            vel = 0; // BIKE_get_speed does not take account of direction
        }
        FMATRIX_calc(matrix, object->Draw.Mesh->Angle, object->Draw.Mesh->Tilt, object->Draw.Mesh->Roll);
        FMATRIX_TRANSPOSE(matrix);
        vector[2] = vel;
        vector[1] = 0;
        vector[0] = 0;
        FMATRIX_MUL(matrix, vector[0], vector[1], vector[2]);
        dx = vector[0];
        dy = vector[1];
        dz = vector[2];
        ox = dx;
        oy = dy;
        oz = dz;
        break;
    case DT_TWEEN:
    case DT_ROT_MULTI:
        FMATRIX_calc(matrix, object->Draw.Mesh->Angle, object->Draw.Mesh->Tilt, object->Draw.Mesh->Roll);
        vector[2] = vel;
        vector[1] = 0;
        vector[0] = 0;
        FMATRIX_MUL(matrix, vector[0], vector[1], vector[2]);
        dx = vector[0];
        dy = vector[1];
        dz = vector[2];
        ox = dx;
        oy = dy;
        oz = dz;
        break;
    default:
        dx = dy = dz = 0;
        ox = oy = oz = 0;
    }

    x = object->WorldPos.X + ox;
    y = object->WorldPos.Y + oy + 0x600;
    z = object->WorldPos.Z + oz;
    ox = oy = oz = 0;
    res = 1;
    p.page = POLY_PAGE_STEAM;
    p.sprite = 1 + ((rand() & 3) << 2);
    p.colour = 0x3FFFFFFF;
    p.flags = PFLAGS_SMOKE | PFLAG_WANDER;
    p.life = 50;
    p.priority = 1;
    p.size = 4;
    p.resize = 6;
    p.fade = disperse;
    p.dx = dx / 2;
    p.dy = dy / 2;
    p.dz = dz / 2;

    for (i = density; i && res; i--) {
        if (object->Class == CLASS_BIKE) {
            vector[2] = 18768 + (rand() & vel);
            vector[1] = 4048;
            vector[0] = 0;
            FMATRIX_MUL(matrix, vector[0], vector[1], vector[2]);
            ox = vector[0];
            oy = vector[1];
            oz = vector[2];
        }

        p.x = ox + x + (rand() & 0xff) - 0x7f;
        p.y = oy + y + (rand() & 0xff) - 0x7f;
        p.z = oz + z + (rand() & 0xff) - 0x7f;
        res = PARTICLE_AddParticle(p);
    }
    return res;
}

// Directional steam jet along one world axis (0=X, 1=Y, 2=Z).
// vel: main velocity along axis. range: perpendicular spread.
// time is packed into the colour alpha byte to control initial opacity.
// uc_orig: PARTICLE_Steam (fallen/Source/psystem.cpp)
UWORD PARTICLE_Steam(SLONG x, SLONG y, SLONG z, UBYTE axis, SLONG vel, SLONG range, UBYTE time)
{
    Particle p;
    UBYTE i, dir;
    SLONG res = 1;
    SLONG dx, dy, dz, rx, ry, rz;

    switch (axis) {
    case 0:
        dx = vel;
        dy = dz = 0;
        rx = 0;
        ry = rz = range;
        break;
    case 1:
        dx = dz = 0;
        dy = vel;
        rx = rz = range;
        ry = 0;
        break;
    case 2:
        dx = dy = 0;
        dz = vel;
        rx = ry = range;
        rz = 0;
    }

    p.x = x;
    p.y = y;
    p.z = z;
    p.page = POLY_PAGE_STEAM;
    p.sprite = 1 + ((rand() & 3) << 2);
    p.colour = 0x00FFFFFF + (time << 24);
    p.flags = PFLAGS_SMOKE | PFLAG_WANDER;
    p.life = 45;
    p.priority = 1;
    p.size = 4;
    p.resize = 3;
    p.fade = 2;
    for (i = 8; i && res; i--) {
        p.dx = dx;
        p.dy = dy;
        p.dz = dz;
        if (rx)
            p.dx += (rand() % rx);
        if (ry)
            p.dy += (rand() % ry);
        if (rz)
            p.dz += (rand() % rz);
        res = PARTICLE_AddParticle(p);
    }
    return res;
}

// Smoke grenade effect: emits 3 large animated smoke-cloud particles per call.
// PFLAG_DRIFT is set but its implementation was never finished in the original (no-op in PARTICLE_Run).
// uc_orig: PARTICLE_SGrenade (fallen/Source/psystem.cpp)
UWORD PARTICLE_SGrenade(Thing* object, UBYTE time)
{
    Particle p;
    UBYTE i;
    SLONG res = 1;

    p.page = POLY_PAGE_SMOKECLOUD2;
    p.sprite = 2 + ((rand() & 3) << 2);
    p.colour = 0x7FFFFFFF;
    p.flags = PFLAGS_SMOKE | PFLAG_DRIFT | PFLAG_SPRITEANI | PFLAG_SPRITELOOP;
    p.life = 80;
    p.priority = 1;
    p.size = 6;
    p.resize = 6;
    p.fade = 3;
    for (i = 3; i && res; i--) {
        p.x = object->WorldPos.X + (rand() & 0x3FFF) - 0x1FFF;
        p.y = object->WorldPos.Y + 0x600;
        p.z = object->WorldPos.Z + (rand() & 0x3FFF) - 0x1FFF;
        p.dx = (rand() & 0x7FF) - 0x3FF;
        p.dy = (rand() & 0x3FF) + 0x100;
        p.dz = (rand() & 0x7FF) - 0x3FF;
        res = PARTICLE_AddParticle(p);
    }
    return res;
}
