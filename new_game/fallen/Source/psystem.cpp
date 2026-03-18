//
// psystem.cpp
//
// Proper particle system for handling a wide variety of effects:
// smoke, dust, mud, sparks, blood...
//

// claude-ai: PARTICLE SYSTEM OVERVIEW
// claude-ai: Fixed-size pool of PSYSTEM_MAX_PARTICLES Particle structs managed as two doubly-linked lists:
// claude-ai:   - "used" list: active particles, iterated backwards from next_used → head sentinel (index 0)
// claude-ai:   - "free" list: available slots, singly-linked via Particle::next, head = next_free
// claude-ai: Particle index 0 is a permanent sentinel ("sacrificed to the readability gods") — never used as real particle.
// claude-ai:
// claude-ai: Per-tick update (PARTICLE_Run):
// claude-ai:   1. Compute local_ratio = elapsed_ms / NORMAL_TICK_TOCK scaled by TICK_SHIFT — used for dt-independent movement.
// claude-ai:   2. Integrate position: new_pos = pos + vel * TICK_RATIO (TICK_RATIO is fixed; not local_ratio — gives smooth motion).
// claude-ai:   3. Apply gravity if PFLAG_GRAVITY: dy -= 0xff * TICK_RATIO >> TICK_SHIFT each tick.
// claude-ai:   4. Per-logical-tick (local_ratio > 255): apply wander/damping, fade alpha, bounce/collide, resize, hurt-people damage.
// claude-ai:   5. Update life counter; recycle dead particles.
// claude-ai:
// claude-ai: Particle flags (PFLAG_*) control per-particle behaviour — see psystem.h.
// claude-ai: Colour encoding: top byte (bits 31..24) = alpha; lower 3 bytes = RGB.
// claude-ai:   PFLAG_INVALPHA: inverted alpha (0x00 = opaque, 0xFF = transparent — used for fade-in).
// claude-ai:
// claude-ai: Port notes: pool concept is correct — use instanced rendering in OpenGL (one draw call for all particles).
// claude-ai:   Replace the linked-list traversal with a flat active array + free index stack for better cache performance.
// claude-ai:   The PSX codepath (local_ratio = constant TICK_RATIO) can be dropped — PC only.

#include "game.h"
#include "psystem.h"
#include "mav.h"
#include "FMatrix.h"
#include "..\DDEngine\Headers\poly.h"
#include "animate.h"
#include "interact.h"
#include "sound.h"
#include "pcom.h"
#include "pow.h"
#include "fc.h"

// pvt globals...

// claude-ai: The entire particle pool — statically allocated, zero-initialised by PARTICLE_Reset.
// claude-ai: next_free  = head of free list (index into particles[])
// claude-ai: next_used  = tail of used list (most recently added particle; iteration walks backwards via prev)
// claude-ai: particle_count = number of currently active particles
Particle particles[PSYSTEM_MAX_PARTICLES];
SLONG next_free, next_used, particle_count;

// nick this bit

// claude-ai: fire_pal is a 256-entry RGB palette (768 bytes) used to colour PFLAG_FIRE particles.
// claude-ai: Alpha value indexes into the palette to give a fire colour gradient (white-hot → dark red → black).
// claude-ai: Defined elsewhere (not in psystem.cpp); PSX build doesn't use it (fire colour baked differently).
extern UBYTE fire_pal[768];

static SLONG prev_tick; // claude-ai: timestamp of last PARTICLE_Run call (GetTickCount units)
static BOOL first_pass; // claude-ai: skip dt calculation on first frame to avoid huge initial delta

// claude-ai: PARTICLE_Reset — call at level load / restart to wipe all active particles.
// claude-ai: Builds initial free-list: particles[0..N-1].next = i+1 (forward chain), .prev = i-1 (backward chain).
// claude-ai: particles[N-1].next = 0 terminates the free-list.
// claude-ai: Particles[0] is the sentinel: next_used starts at 0 so the used list is initially empty.
// claude-ai: next_free = 1 means the first allocation will grab particles[1].
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
    // sacrifice particles[0] to the readability gods...
    next_free = 1;
    next_used = 0;
    prev_tick = GetTickCount();
    first_pass = 1;
}

// claude-ai: PARTICLE_Run — main particle update loop, called every game tick.
// claude-ai:
// claude-ai: TIMING MODEL:
// claude-ai:   local_ratio = (elapsed_ms << TICK_SHIFT) / NORMAL_TICK_TOCK
// claude-ai:   This is a fixed-point fraction: local_ratio == 1<<TICK_SHIFT means exactly one normal tick elapsed.
// claude-ai:   local_ratio < 256 → frame was too short; skip logical updates to avoid micro-step accumulation.
// claude-ai:   Position integration always uses the constant TICK_RATIO (not local_ratio) for smooth sub-tick rendering.
// claude-ai:
// claude-ai: isWare flag: if the camera is following a person carrying a weapon (Ware != NULL),
// claude-ai:   particles that hit the ground are NOT killed — prevents bullet-trail particles from disappearing
// claude-ai:   prematurely when close to the ground.
void PARTICLE_Run()
{
    SLONG ctr, tx, ty, tz;
    Particle* p;
    SLONG trans;
    UBYTE* palptr;
    SLONG palndx;
    UBYTE isWare;

    // claude-ai: isWare — special case to keep near-ground particles alive when player is armed
    isWare = (FC_cam->focus->Class == CLASS_PERSON && FC_cam->focus->Genus.Person->Ware);

    // let's try something cunning with tick_shift ...
    // I'm sure this shouldn't SLONG
    // SLONG local_ratio, local_shift;
    // claude-ai: ULONG because the signed version caused negative ratio on fast frames (see original comment)
    ULONG local_ratio, local_shift;
    SLONG cur_tick, tick_diff;

    cur_tick = GetTickCount();
    tick_diff = cur_tick - prev_tick;

    if (first_pass) {
        tick_diff = NORMAL_TICK_TOCK; // claude-ai: pretend exactly one tick elapsed on first frame
        first_pass = 0;
    }
    local_shift = TICK_SHIFT;
    local_ratio = (tick_diff << local_shift) / (NORMAL_TICK_TOCK); // claude-ai: fixed-point dt ratio

    if (local_ratio < 256) // we'll get big problems
    {
        local_ratio = 0; // so ignore it for now, and let it catch up later
        // claude-ai: Frame too short — skip logical-tick updates; position still integrates via constant TICK_RATIO
    } else
        prev_tick = cur_tick;

    if (!particle_count)
        return;

    // claude-ai: Walk backwards through the used list: next_used → ... → 0 (sentinel).
    // claude-ai: Backwards traversal lets us safely remove the current particle (p->prev is next to visit).
    //	p=particles;
    //	for (ctr=0;ctr<PSYSTEM_MAX_PARTICLES;ctr++,p++)
    for (p = particles + next_used; p != particles;)
    //		if (p->priority)
    {
        // these are nice n smooth always (not local_ratio'd)
        // claude-ai: POSITION INTEGRATION — uses fixed TICK_RATIO, not local_ratio.
        // claude-ai: This runs every frame regardless of frame rate → smooth motion at high fps.
        // claude-ai: Result stored in tx/ty/tz; only committed to p->x/y/z at end of the block.
        tx = p->x + ((p->dx * TICK_RATIO) >> TICK_SHIFT);
        ty = p->y + ((p->dy * TICK_RATIO) >> TICK_SHIFT);
        tz = p->z + ((p->dz * TICK_RATIO) >> TICK_SHIFT);

        // claude-ai: GRAVITY — applies constant downward acceleration to dy each tick.
        // claude-ai: Gravity magnitude: 0xFF * TICK_RATIO >> TICK_SHIFT units/tick² (world-space fixed-point).
        // claude-ai: No terminal velocity cap — particles in free fall accelerate without bound.
        if (p->flags & PFLAG_GRAVITY) {
            p->dy -= (0xff * TICK_RATIO) >> TICK_SHIFT;
        }

        if (p->flags & PFLAG_SPRITEANI) {
            p->sprite += 4;
        }

        // claude-ai: LOGICAL-TICK GATE — only run simulation updates when a full tick's worth of time passed.
        // claude-ai: local_ratio <= 255 means the frame was too short; skip these updates entirely.
        // claude-ai: This separates "smooth rendering position" (always updated above) from
        // claude-ai:   "game logic updates" (run at a controlled rate tied to real time).
        if (local_ratio > 255) {
            /*				if (p->flags & PFLAG_SPRITEANI) {
                                              p->sprite+=(TICK_RATIO>>TICK_SHIFT)<<2;
                                            }*/

            // claude-ai: PFLAG_WANDER — random velocity perturbation each tick; used for smoke.
            // claude-ai: Adds ±(0..30)*4 to each velocity component — total range ≈ ±120 units/tick.
            if (p->flags & PFLAG_WANDER) {
                p->dx += ((rand() & 0x1f) - 0xf) * 4;
                p->dy += ((rand() & 0x1f) - 0xf) * 4;
                p->dz += ((rand() & 0x1f) - 0xf) * 4;
            }

            // claude-ai: PFLAG_DRIFT — sinusoidal drift (intended for slow billowing effects).
            // claude-ai: UNIMPLEMENTED — the code inside is commented out and was never finished.
            if (p->flags & PFLAG_DRIFT) {
                /*				static SLONG tick=0;
                                                tick+=8;
                                          p->dx+=abs(SIN((p->y+tick)&2047))/2048;
                                          p->dz+=abs(SIN((p->y+tick)&2047))/2048;
                                          p->dy+=abs(COS((p->y+tick)&2047))/2048;*/
            }

            // claude-ai: PFLAG_DAMPING — velocity damping (friction) in X/Z only; Y (vertical) not damped.
            // claude-ai: Factor 245/256 ≈ 0.957 per tick. Applied only to horizontal components.
            // claude-ai: Used for sliding debris, sparks skidding along floor.
            if (p->flags & PFLAG_DAMPING) {
                p->dx = (p->dx * 245) >> 8;
                p->dz = (p->dz * 245) >> 8;
            }

            /*				if (p->flags & PFLAG_BOUYANT) {
                                                    // do this after we actually HAVE water in the game again...
                                            }*/

            // claude-ai: PFLAG_FADE2 — same alpha-fade logic as PFLAG_FADE but only runs on odd life values.
            // claude-ai: Combined with PFLAG_FADE on the same particle, this effectively halves the fade rate.
            if ((p->flags & PFLAG_FADE2) && (p->life & 1)) {
                // I'm sure this shouldn't a SLONG
                // SLONG diff=0x01000000*((p->fade*local_ratio)>>local_shift);
                // claude-ai: ALPHA FADE — diff is the per-tick alpha change amount (shifted into top byte scale).
                // claude-ai: p->fade * local_ratio >> local_shift = fade rate scaled by actual elapsed time.
                // claude-ai: 0x01000000 = 1 in the top-byte (alpha) component of the ARGB colour word.
                ULONG diff = 0x01000000 * ((p->fade * local_ratio) >> local_shift);
                if (p->flags & PFLAG_INVALPHA) {
                    // claude-ai: PFLAG_INVALPHA: alpha=0x00 means opaque → fade IN by incrementing alpha toward 0xFF
                    if ((p->colour & 0xFF000000) < 0xFF000000 - diff)
                        p->colour += diff;
                    else
                        p->life = 1; // killlll
                } else {
                    // claude-ai: Normal: alpha=0xFF means opaque → fade OUT by decrementing alpha toward 0x00
                    if ((p->colour & 0xFF000000) > diff)
                        p->colour -= diff;
                    else
                        p->life = 1; // killlll (kill by zeroing life when fully transparent)
                }
            }

            // claude-ai: PFLAG_FADE — standard per-tick alpha fade; runs every logical tick (no odd-life gate).
            // claude-ai: Identical fade calculation to PFLAG_FADE2 above.
            // claude-ai: When alpha reaches 0 (or 0xFF for inverted), particle is immediately killed (life=1).
            if (p->flags & PFLAG_FADE) {
                // I'm sure this shouldn't a SLONG
                // SLONG diff=0x01000000*((p->fade*local_ratio)>>local_shift);
                ULONG diff = 0x01000000 * ((p->fade * local_ratio) >> local_shift);
                if (p->flags & PFLAG_INVALPHA) {
                    if ((p->colour & 0xFF000000) < 0xFF000000 - diff)
                        p->colour += diff;
                    else
                        p->life = 1; // killlll
                } else {
                    if ((p->colour & 0xFF000000) > diff)
                        p->colour -= diff;
                    else
                        p->life = 1; // killlll
                }
            }
            // claude-ai: PFLAG_FIRE — palette-based colour cycling for fire particles.
            // claude-ai: Uses the alpha value (trans = top byte) as an index into fire_pal (768-byte RGB palette).
            // claude-ai: Simultaneously looks up the INVERSE index (256-trans)*3 for the RGB colour.
            // claude-ai: Effect: as alpha decreases (particle fades), colour shifts through the fire palette
            // claude-ai:   from white-hot (low alpha index) → orange/red → dark (high alpha index).
            // claude-ai: Port note: implement as a shader LUT lookup or bake into texture atlas frames.
            if (p->flags & PFLAG_FIRE) {
                trans = (p->colour & 0xFF000000) >> 24;
                palptr = (trans * 3) + fire_pal;
                palndx = (256 - trans) * 3;
                trans <<= 24;
                p->colour = trans + (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
            }

            // claude-ai: PFLAG_BOUNCE — particle bounces off the terrain surface.
            // claude-ai: Reflects Y velocity: dy = -(dy * 180 >> 8) ≈ -0.703 * dy (not elastic; energy lost).
            // claude-ai: When |dy| < 256 (sliding along surface), apply horizontal friction too:
            // claude-ai:   dx/dz *= 180/256 ≈ 0.703 — particle slows to a stop.
            // claude-ai: Used for sparks and debris that should skitter across the ground.
            if (p->flags & PFLAG_BOUNCE) {
                SLONG tmpy = PAP_calc_map_height_at(tx >> 8, tz >> 8) << 8;
                if (ty < tmpy) {
                    ty -= tmpy;
                    ty = tmpy - ty;
                    p->dy = -(p->dy * 180) >> 8; // dy*=-0.9;
                    if (abs(p->dy) < 256) { // friction applies
                        p->dx = (p->dx * 180) >> 8;
                        p->dz = (p->dz * 180) >> 8;
                    }
                }
            } else
                // claude-ai: Non-bouncing ground collision: kill the particle when it goes below terrain height.
                // claude-ai: isWare exception: when player is armed, DON'T kill on ground collision (bullet trails).
                //				if (MAV_inside(tx>>8,ty>>8,tz>>8))
                if ((ty >> 8) < PAP_calc_map_height_at(tx >> 8, tz >> 8)) {
                    if (p->flags & PFLAG_COLLIDE) {
                        // twiddle... // claude-ai: PFLAG_COLLIDE — stub, not implemented; reserved for future use
                    } else {
                        if (!isWare)

                            p->life = 1; // the -- at end will remove
                        //					TRACE("psystem: collide-removed\n");
                    }

                    // claude-ai: PFLAG_EXPLODE_ON_IMPACT — spawns a PYRO_WHOOMPH explosion + shockwave on ground hit.
                    // claude-ai: Used for explosive projectile trails (e.g., grenade bouncing particles).
                    // claude-ai: Shockwave radius 0x140 (320 world units), pushback strength 80.
                    if (p->flags & PFLAG_EXPLODE_ON_IMPACT) {
                        /*POW_create(
                                POW_CREATE_MEDIUM,
                                tx,
                                ty,
                                tz,0,0,0);*/
                        GameCoord pos = { tx, 0, tz };
                        pos.Y = PAP_calc_map_height_at(tx >> 8, tz >> 8) << 8;
                        pos.Y += 1024;
                        Thing* pyro = PYRO_create(pos, PYRO_WHOOMPH);
                        if (pyro)
                            pyro->Genus.Pyro->counter = 0; // heh

                        create_shockwave(
                            tx >> 8,
                            ty >> 8,
                            tz >> 8,
                            0x140,
                            80,
                            NULL);
                    }
                }

            // claude-ai: PFLAG_LEAVE_TRAIL — spawns two child fire particles every logical tick at the current position.
            // claude-ai: Child 1: stationary fire sprite (dx/dy/dz = 0), random jitter ±0x4fff world units.
            // claude-ai: Child 2: inherits 1/8 of parent's velocity + random jitter (smaller jitter range).
            // claude-ai: Both children use the PFLAG_FIRE palette cycling + PFLAG_FADE + PFLAG_RESIZE.
            // claude-ai: Used for flaming projectile trails (e.g., Molotov in flight).
            //				if ((p->flags & PFLAG_LEAVE_TRAIL)&&(GAME_TURN&3))
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
                /*					PARTICLE_Add(
                                                                tx + (Random() & 0x3fff) - 0x1fff,
                                                                ty + (Random() & 0x3fff) - 0x1fff,
                                                                tz + (Random() & 0x3fff) - 0x1fff,
                                                                p->dx + (Random() & 0xfff) - 0x7ff          >> 3,
                                                                p->dy + (Random() & 0xfff) - 0x7ff + 0x4000 >> 3,
                                                                p->dz + (Random() & 0xfff) - 0x7ff          >> 3,
                                                                POLY_PAGE_SMOKECLOUD,
                                                                2 + ((Random() & 0x3) << 2),
                                                                0x7fffffff,
                                                                PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE,
                                                                100,
                                                                100,
                                                                1,
                                                                1,
                                                                1);*/
            }

            // claude-ai: PFLAG_RESIZE — grow/shrink particle size each tick by (resize * local_ratio >> local_shift).
            // claude-ai: p->resize is signed (SBYTE): positive = grow, negative = shrink.
            // claude-ai: Killed when size drops to 0 or less; size capped at 255. Used for smoke puffs, sparks.
            if (p->flags & PFLAG_RESIZE) {
                SLONG temp = p->size;

                temp += (p->resize * local_ratio) >> local_shift;
                if (temp < 1) {
                    temp = 1;
                    p->life = 1;
                } // clear 0-size or less particles
                if (temp > 255)
                    temp = 255;
                p->size = temp;
            }
            // claude-ai: PFLAG_RESIZE2 — same as PFLAG_RESIZE but size is 16-bit (pre-shifted <<8 at spawn time).
            // claude-ai: Allows finer sub-unit shrink rates. Size capped at 65535 (UWORD).
            if (p->flags & PFLAG_RESIZE2) {
                SLONG temp = p->size;

                temp += (p->resize * local_ratio) >> local_shift;
                if (temp < 1)
                    p->life = 1;
                SATURATE(temp, 1, 65535);
                p->size = temp;
            }

            // claude-ai: PFLAG_HURTPEOPLE — particle deals fire/explosion damage to nearby persons every other tick.
            // claude-ai: Sphere query radius: 0xa0 (160 units). Actual hit radius: 0x40 (64 units).
            // claude-ai: Damage: 1..5 HP per hit, distance-dependent. Respects invulnerability flags.
            // claude-ai: Port note: keep the sphere query + height check; route damage through a central DamageSystem.
            if (p->flags & PFLAG_HURTPEOPLE) {
                if (GAME_TURN & 0x1) // claude-ai: throttle to every other tick to reduce per-frame CPU cost
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

                        extern SLONG is_person_dead(Thing * p_person);

                        if (is_person_dead(p_hurt)) {
                            //
                            // Don't hurt dead people!
                            //
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
                                    PainSound(p_hurt); // scream yer lungs out!

                                    if ((p_hurt->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) || (p_hurt->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL)) {
                                        //
                                        // Nothing hurts this person.
                                        //
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
                            } // (abs(dist) < 0x40)
                        } // is_person_dead() ... else
                    } // (i_hurt)
                } // (GAME_TURN & 0x1)
            } // (p->flags & PFLAG_HURTPEOPLE)

        } // (local_ratio>255)

        p->x = tx;
        p->y = ty;
        p->z = tz;

        // claude-ai: POSITION COMMIT — write integrated position to particle after all updates done
        // ending too soon is preferable to ending too late
        // ending too late is Real Bad
        // claude-ai: LIFETIME DECREMENT — subtract time-scaled delta from life counter each frame.
        // claude-ai: PC: life -= local_ratio >> local_shift  (~1 per normal tick, more on slow frames).
        // claude-ai: PSX: fixed decrement of 2 per tick (constant frame rate assumption).
        // claude-ai: "ending too late is Real Bad" — leaked particle stays in used list indefinitely.
        SLONG temp = local_ratio >> local_shift;
        //			TRACE("old life: %d   subtracting: %d\n",p->life,temp);
        p->life -= local_ratio >> local_shift;
        // claude-ai: PARTICLE RECYCLING — unlink dead particle from used list, prepend to free list.
        // claude-ai: Save p->prev in temp BEFORE unlinking so the iterator can advance to previous element.
        if (p->life <= 0) {
            SWORD idx = p - particles, temp = p->prev;
            p->priority = 0;
            particle_count--;

            // pull it off the used list
            if (p->next)
                particles[p->next].prev = p->prev;
            if (p->prev)
                particles[p->prev].next = p->next;
            if (next_used == idx) {
                next_used = p->prev;
                particles[next_used].next = 0;
            }
            p->prev = 0;

            // join it onto the free list
            // claude-ai: Prepend idx to free list; fix up ex-head's prev pointer too
            particles[next_free].prev = idx;
            p->next = next_free;
            next_free = idx;

            p = particles + temp; // claude-ai: advance backwards; temp was saved before unlink

            /*				// experimental psystem speeder-upper
                                            next_free=p-particles;*/
        } else
            p = particles + p->prev; // claude-ai: particle alive: step backwards through used list
    }
}

// claude-ai: PARTICLE_AddParticle — low-level allocation; takes a fully-configured Particle struct by value.
// claude-ai: Returns new particle_count on success, 0 on failure (pool full or game paused).
// claude-ai: Pops from free list, pushes onto tail of used list (next_used updated to new particle).
// claude-ai: PFLAG_RESIZE2 special case: pre-shifts size <<8 at add time and auto-computes resize rate if unset.
UWORD PARTICLE_AddParticle(Particle& p)
{
    //	UBYTE priority=0;
    //	UWORD ctr=0;
    UWORD new_particle;

    extern SLONG GAMEMENU_menu_type;
    // claude-ai: No new particles during pause/menu — prevents effects accumulating while frozen
    if (GAMEMENU_menu_type != 0 /*GAMEMENU_MENU_TYPE_NONE*/) {
        // Some sort of pause mode is up - don't make any more particles.
        return 0;
    }

    if (particle_count == PSYSTEM_MAX_PARTICLES)
        return 0; // claude-ai: pool exhausted — silently drop

    /*	while ((particles[next_free].priority>priority)&&(ctr<1000)) {
                    next_free++;
                    if (next_free==PSYSTEM_MAX_PARTICLES) next_free=0;
                    ctr++;
                    if ((ctr>200)&&(priority<p.priority)) priority++;
            }
            if (ctr<1000) {
                    if (!particles[next_free].priority) particle_count++;
                    particles[next_free]=p;
                    return particle_count;
            }
            return 0;*/

    // list version...

    if (!next_free)
        return 0; // claude-ai: free list empty — shouldn't happen if particle_count check above passed

    // claude-ai: PFLAG_RESIZE2 init: pre-shift size into 16-bit fixed-point, auto-compute shrink rate if not set
    if (p.flags & PFLAG_RESIZE2) {
        p.size <<= 8; // claude-ai: upper 8 bits = whole, lower 8 = fractional size
        if (!p.resize) {
            p.resize = -(p.size / p.life); // claude-ai: auto shrink-rate so particle hits size 0 exactly at end of life
        }
    }

    // claude-ai: POP from free list: next_free advances to next free slot, clear its prev back-pointer
    // pull the particle off the list
    new_particle = next_free;
    next_free = particles[next_free].next;
    particles[next_free].prev = 0;

    // set its contents
    particles[new_particle] = p;
    particles[new_particle].next = 0; // part of pulling off the list, but JIC...
    particles[new_particle].priority = 1;

    // claude-ai: APPEND to tail of used list: new particle becomes the new next_used tail
    // join it onto the used list
    particles[next_used].next = new_particle;
    particles[new_particle].prev = next_used;
    next_used = new_particle;

    /*	CBYTE msg[10];
            itoa(particle_count,msg,10);
            CONSOLE_text(msg,1000);*/

    return ++particle_count;
}

// claude-ai: PARTICLE_Add — convenience wrapper that constructs a Particle on the stack and calls AddParticle.
// claude-ai: Parameters match the Particle struct fields directly.
// claude-ai: colour: top byte = alpha (0xFF = opaque for normal, 0x00 = opaque for PFLAG_INVALPHA).
// claude-ai: fade: signed rate at which alpha changes per tick (positive = fade out, negative = fade in).
// claude-ai: resize: signed size delta per tick (positive = grow, negative = shrink).
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

//
// -------------------------------------------------------------------------------------
// Shortcuts to some of the more commonly-used effects:
//
// claude-ai: These helper functions are PC-only (#ifndef PSX) and pre-configure Particle structs for
// claude-ai: common game effects: exhaust smoke, steam jets, smoke grenades.
// claude-ai: Port note: keep these as convenience factories; adapt to new particle API.
// claude-ai: PARTICLE_Exhaust — emits 'density' smoke particles at a fixed world position.
// claude-ai: Used for stationary exhaust vents. PFLAG_WANDER makes smoke billow randomly.
// claude-ai: disperse controls fade rate (higher = shorter-lived cloud).
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

// claude-ai: PARTICLE_Exhaust2 — direction-aware exhaust for moving vehicles/bikes.
// claude-ai: Computes exhaust direction from object orientation matrix (Angle/Tilt/Roll).
// claude-ai: vel = 1024 - (object->Velocity * 128): faster vehicles produce shorter exhaust blasts.
// claude-ai: Bike special case: BIKE_get_speed() doesn't account for direction so vel forced to 0.
// claude-ai: Particles inherit half the object's exhaust velocity + random jitter.
UWORD PARTICLE_Exhaust2(Thing* object, UBYTE density, UBYTE disperse)
{
    UBYTE i;
    UWORD res;
    Particle p;
    SLONG x, y, z, dx, dy, dz, ox, oy, oz;
    //	float matrix[9], vector[3];
    SLONG matrix[9], vector[3];
    /*	float yaw;
            float pitch;
            float roll;
            */
    //	SLONG matrix[9], vector[3], yaw, pitch, roll;
    SLONG vel;

    vel = 1024 - (object->Velocity * 128);
    switch (object->DrawType) {
    case DT_MESH:
    case DT_VEHICLE:
        /*		yaw   = -float(object->Draw.Mesh->Angle)   * (2.0F * PI / 2048.0F);
                        pitch = -float(object->Draw.Mesh->Tilt) * (2.0F * PI / 2048.0F);
                        roll  = -float(object->Draw.Mesh->Roll)  * (2.0F * PI / 2048.0F);*/
        //		MATRIX_calc(matrix, yaw, pitch, roll);
        if (object->Class == CLASS_BIKE) {
            //			vel=BIKE_get_speed(object);
            //			TRACE("bikevel %d\n",vel);
            //			vel=4096+vel;
            vel = 0; // bike_get_speed does not take account of direction :-(
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
        /*		if (object->Class==CLASS_BIKE) {
                          ox=oy=oz=0;
                        } else {*/
        ox = dx;
        oy = dy;
        oz = dz;
        //		}
        break;
    case DT_TWEEN:
    case DT_ROT_MULTI:
        /*		yaw   = -float(object->Draw.Tweened->Angle)   * (2.0F * PI / 2048.0F);
                        pitch = -float(object->Draw.Tweened->Tilt) * (2.0F * PI / 2048.0F);
                        roll  = -float(object->Draw.Tweened->Roll)  * (2.0F * PI / 2048.0F);*/
        //		MATRIX_calc(matrix, yaw, pitch, roll);
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
    //	p.dx=0; p.dy=0; p.dz=0;
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

// claude-ai: PARTICLE_Steam — directional steam jet along one of three world axes (0=X, 1=Y, 2=Z).
// claude-ai: vel = main velocity along axis; range = spread perpendicular to axis.
// claude-ai: time is packed into the colour alpha byte to control initial opacity.
// claude-ai: Emits 8 particles per call with PFLAG_WANDER for turbulent billowing.
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

// claude-ai: PARTICLE_SGrenade — smoke grenade effect: 3 large animated smoke cloud particles per call.
// claude-ai: Particles use POLY_PAGE_SMOKECLOUD2 sprite sheet with random frame + PFLAG_SPRITEANI loop.
// claude-ai: PFLAG_DRIFT flag set but the drift implementation is unfinished (no-op in PARTICLE_Run).
// claude-ai: Called repeatedly each tick while the grenade is active to build up a thick cloud.
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
