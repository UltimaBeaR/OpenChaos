// Temporary includes — these modules are not yet migrated.
// game.h pulls in the core game types (Thing, GameCoord, TICK_RATIO, etc.)
#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/characters/person.h"  // set_face_thing
#include "missions/eway.h"
#include "map/ob.h"
#include "things/core/statedef.h"
#include "navigation/wmove.h"
#include "engine/effects/psystem.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/postprocess/bloom.h" // BLOOM_draw, BLOOM_BEAM, BLOOM_LENSFLARE

#include "buildings/prim.h"    // get_prim_info, slide_along_prim
#include "world_objects/plat.h"
#include "world_objects/plat_globals.h"

// Private state values for Plat.state.
// uc_orig: PLAT_STATE_NONE (fallen/Source/plat.cpp)
#define PLAT_STATE_NONE  0
// uc_orig: PLAT_STATE_GOTO (fallen/Source/plat.cpp)
#define PLAT_STATE_GOTO  1  // Moving toward a waypoint
// uc_orig: PLAT_STATE_PAUSE (fallen/Source/plat.cpp)
#define PLAT_STATE_PAUSE 2  // Waiting at a waypoint
// uc_orig: PLAT_STATE_STOP (fallen/Source/plat.cpp)
#define PLAT_STATE_STOP  3  // Permanently stopped

// Private axis-lock flags stored in Plat.flag (bits 5-7, not exposed in public header).
// uc_orig: PLAT_FLAG_LOCK_X (fallen/Source/plat.cpp)
#define PLAT_FLAG_LOCK_X (1 << 5)
// uc_orig: PLAT_FLAG_LOCK_Y (fallen/Source/plat.cpp)
#define PLAT_FLAG_LOCK_Y (1 << 6)
// uc_orig: PLAT_FLAG_LOCK_Z (fallen/Source/plat.cpp)
#define PLAT_FLAG_LOCK_Z (1 << 7)

// uc_orig: PLAT_init (fallen/Source/plat.cpp)
void PLAT_init()
{
    memset(PLAT_plat, 0, sizeof(Plat) * PLAT_MAX_PLATS);

    PLAT_plat_upto = 1;
}

// Per-frame update for a CLASS_PLAT thing.
// Handles acceleration/deceleration toward the current waypoint, person collision, and rocket fx.
// uc_orig: PLAT_process (fallen/Source/plat.cpp)
void PLAT_process(Thing* p_thing)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG way_x;
    SLONG way_y;
    SLONG way_z;

    SLONG waypoint;
    SLONG millisecs;
    SLONG ticks;

    SLONG wspeed;
    SLONG speed;
    SLONG move;
    SLONG len;
    SLONG overlen;

    GameCoord newpos;

    millisecs = 50 * TICK_RATIO >> TICK_SHIFT;
    ticks = 5 * TICK_RATIO >> TICK_SHIFT;

    ASSERT(p_thing->Class = CLASS_PLAT);

    Plat* plat = p_thing->Genus.Plat;
    DrawMesh* dm = p_thing->Draw.Mesh;

    switch (plat->state) {
    case PLAT_STATE_NONE:

        switch (plat->move) {
        case PLAT_MOVE_STILL:
            break;

        case PLAT_MOVE_PATROL:
        case PLAT_MOVE_PATROL_RAND:
            plat->waypoint = NULL;
            plat->state = PLAT_STATE_PAUSE;
            plat->counter = 1 * 1000;

            break;

        default:
            ASSERT(0);
            break;
        }

        break;

    case PLAT_STATE_GOTO:

        // Rocket exhaust particles (PC only).
        if (plat->flag & PLAT_FLAG_BODGE_ROCKET) {
            PARTICLE_Add(p_thing->WorldPos.X + (((Random() & 0xff) - 0x7f) << 7), p_thing->WorldPos.Y, p_thing->WorldPos.Z + (((Random() & 0xff) - 0x7f) << 7),
                ((Random() & 0xff) - 0x7f) << 2, 0, ((Random() & 0xff) - 0x7f) << 2,
                POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE,
                300, 70, 1, 1, 2);
            PARTICLE_Add(p_thing->WorldPos.X + (((Random() & 0xff) - 0x7f) << 5), p_thing->WorldPos.Y, p_thing->WorldPos.Z + (((Random() & 0xff) - 0x7f) << 5),
                ((Random() & 0xff) - 0x7f) << 2, 0, ((Random() & 0xff) - 0x7f) << 2,
                POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE,
                300, 70, 1, 1, -2);
            BLOOM_draw(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Y >> 8, p_thing->WorldPos.Z >> 8,
                0, -0xff, 0, 0x00ffffff, BLOOM_BEAM | BLOOM_LENSFLARE);
        }

        if (ControlFlag) {
            waypoint = 0;
        }

        EWAY_get_position(
            plat->waypoint,
            &way_x,
            &way_y,
            &way_z);

        dx = way_x - (p_thing->WorldPos.X >> 8);
        dy = way_y - (p_thing->WorldPos.Y >> 8);
        dz = way_z - (p_thing->WorldPos.Z >> 8);

        if (plat->flag & PLAT_FLAG_LOCK_X) {
            dy = 0;
            dz = 0;
        } else if (plat->flag & PLAT_FLAG_LOCK_Y) {
            dz = 0;
            dx = 0;
        } else if (plat->flag & PLAT_FLAG_LOCK_Z) {
            dx = 0;
            dy = 0;
        }

        len = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

        // Smooth deceleration near the waypoint.
        wspeed = plat->wspeed;
        speed = plat->speed;

        if (len < 0x100) { wspeed >>= 2; }
        if (len < 0x80)  { wspeed >>= 2; }
        if (len < 0x40)  { wspeed >>= 2; }

        if (wspeed < 3) { wspeed = 3; }

        if (abs(speed - wspeed) < ticks) {
            speed = wspeed;
        } else if (speed > wspeed) {
            speed -= ticks;
        } else {
            speed += ticks;
        }

        SATURATE(speed, 0, 255);

        plat->speed = speed;

        move = speed * TICK_RATIO >> TICK_SHIFT;

        if (len <= ((move >> 8) + 4)) {
            // Arrived at waypoint.
            plat->state = PLAT_STATE_PAUSE;
            plat->counter = EWAY_get_delay(plat->waypoint, 2 * 1000);

            if (plat->counter == 10000) {
                // 10 seconds means stop forever.
                plat->state = PLAT_STATE_STOP;
            }
        } else {
            overlen = (move << 8) / len;
            dx = dx * overlen;
            dy = dy * overlen;
            dz = dz * overlen;

            dx = dx * TICK_RATIO >> TICK_SHIFT;
            dy = dy * TICK_RATIO >> TICK_SHIFT;
            dz = dz * TICK_RATIO >> TICK_SHIFT;

            SATURATE(dy, -8 << 8, +8 << 8);

            newpos.X = p_thing->WorldPos.X + dx;
            newpos.Y = p_thing->WorldPos.Y + dy;
            newpos.Z = p_thing->WorldPos.Z + dz;

            move_thing_on_map(p_thing, &newpos);

            // Check for people in the platform's bounding box and push/crush them.
            {
// uc_orig: PLAT_MAX_FIND (fallen/Source/plat.cpp)
#define PLAT_MAX_FIND 8

                SLONG i;
                SLONG num;
                THING_INDEX found[PLAT_MAX_FIND];
                Thing* p_person;

                PrimInfo* pi = get_prim_info(dm->ObjectId);

                num = THING_find_sphere(
                    p_thing->WorldPos.X >> 8,
                    p_thing->WorldPos.Y >> 8,
                    p_thing->WorldPos.Z >> 8,
                    pi->radius + 0x100,
                    found,
                    PLAT_MAX_FIND,
                    THING_FIND_PEOPLE);

                SLONG y_bot = p_thing->WorldPos.Y >> 8;
                SLONG y_top = p_thing->WorldPos.Y >> 8;

                y_top += pi->maxy;
                y_bot += pi->miny;

                for (i = 0; i < num; i++) {
                    p_person = TO_THING(found[i]);

                    if (p_person->State == STATE_DEAD || p_person->State == STATE_DYING) {
                        continue;
                    }

                    if (WITHIN(p_person->WorldPos.Y >> 8, y_bot - 0x100, y_top + 0x100)) {
                        SLONG x1, y1, z1;
                        SLONG x2, y2, z2;

                        x1 = x2 = p_person->WorldPos.X;
                        y1 = y2 = p_person->WorldPos.Y;
                        z1 = z2 = p_person->WorldPos.Z;

                        if (slide_along_prim(
                                dm->ObjectId,
                                p_thing->WorldPos.X >> 8,
                                p_thing->WorldPos.Y >> 8,
                                p_thing->WorldPos.Z >> 8,
                                dm->Angle,
                                x1, y1, z1,
                                &x2,
                                &y2,
                                &z2,
                                10,
                                UC_FALSE,
                                UC_FALSE)) {

                            extern SLONG playing_level(const CBYTE* name); // eway.cpp
                            if (playing_level("botanicc.ucm") && (p_person->SubState == SUB_STATE_STANDING_JUMP_FORWARDS || p_person->SubState == SUB_STATE_STANDING_JUMP_BACKWARDS || p_person->SubState == SUB_STATE_STANDING_JUMP || p_person->SubState == SUB_STATE_RUNNING_JUMP)) {
                                if (WITHIN(p_person->WorldPos.Y >> 8, y_bot, y_top)) {
                                    p_person->WorldPos.Y = p_thing->WorldPos.Y + (pi->maxy << 8);
                                }
                            } else {
                                y_top = y_bot - 0x40;
                                y_bot = y_bot - 0x80;

                                if (WITHIN(p_person->WorldPos.Y >> 8, y_bot, y_top)) {
                                    set_face_thing(p_person, p_thing);

                                    p_person->Genus.Person->Health = 0;

                                    set_person_dead(
                                        p_person,
                                        NULL,
                                        PERSON_DEATH_TYPE_OTHER,
                                        UC_FALSE,
                                        0);
                                }
                            }
                        }
                    }
                }
            }
        }

        break;

    case PLAT_STATE_PAUSE:

        if (plat->counter <= millisecs) {
            plat->counter = 0;

            switch (plat->state) {
            case PLAT_MOVE_STILL:
                break;

            case PLAT_MOVE_PATROL:
            case PLAT_MOVE_PATROL_RAND:

                if (plat->waypoint == NULL) {
                    plat->waypoint = EWAY_find_nearest_waypoint(
                        p_thing->WorldPos.X >> 8,
                        p_thing->WorldPos.Y >> 8,
                        p_thing->WorldPos.Z >> 8,
                        plat->colour,
                        plat->group);
                }

                waypoint = EWAY_find_waypoint(
                    plat->waypoint + 1,
                    EWAY_DONT_CARE,
                    plat->colour,
                    plat->group,
                    UC_TRUE);

                if (waypoint == EWAY_NO_MATCH) {
                    plat->waypoint = NULL;
                    plat->state = PLAT_STATE_PAUSE;
                    plat->counter = 2 * 1000;
                } else {
                    plat->waypoint = waypoint;
                    plat->state = PLAT_STATE_GOTO;
                    plat->speed = 0;

                    EWAY_get_position(
                        waypoint,
                        &way_x,
                        &way_y,
                        &way_z);

                    plat->flag &= ~(PLAT_FLAG_LOCK_X | PLAT_FLAG_LOCK_Y | PLAT_FLAG_LOCK_Z);

                    if (plat->flag & PLAT_FLAG_LOCK_MOVE) {
                        dx = abs((p_thing->WorldPos.X >> 8) - way_x);
                        dy = abs((p_thing->WorldPos.Y >> 8) - way_y);
                        dz = abs((p_thing->WorldPos.Z >> 8) - way_z);

                        if (dx > dy && dx > dz) {
                            plat->flag |= PLAT_FLAG_LOCK_X;
                        } else if (dy > dz && dy > dx) {
                            plat->flag |= PLAT_FLAG_LOCK_Y;
                        } else {
                            plat->flag |= PLAT_FLAG_LOCK_Z;
                        }
                    }
                }

                break;

            default:
                ASSERT(0);
                break;
            }
        } else {
            plat->counter -= millisecs;
        }

        break;

    case PLAT_STATE_STOP:
        break;

    default:
        ASSERT(0);
        break;
    }
}

// Finds the nearest static ob to (world_x, world_y, world_z), removes it, and creates a moving
// platform Thing in its place. Returns the THING_NUMBER or NULL on failure.
// uc_orig: PLAT_create (fallen/Source/plat.cpp)
UWORD PLAT_create(
    UBYTE colour,
    UBYTE group,
    UBYTE move,
    UBYTE flag,
    UBYTE speed,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z)
{
    SLONG mx;
    SLONG mz;

    SLONG x1;
    SLONG z1;

    SLONG x2;
    SLONG z2;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG score;

    SLONG best_score;
    OB_Info best_oi;

    OB_Info* oi;
    Thing* p_thing;
    Plat* plat;
    DrawMesh* dm;

    if (!WITHIN(PLAT_plat_upto, 1, PLAT_MAX_PLATS - 1)) {
        return NULL;
    }

    // Search radius around the spawn position.
// uc_orig: PLAT_LOOK (fallen/Source/plat.cpp)
#define PLAT_LOOK (0x300)

    x1 = world_x - PLAT_LOOK >> PAP_SHIFT_LO;
    z1 = world_z - PLAT_LOOK >> PAP_SHIFT_LO;

    x2 = world_x + PLAT_LOOK >> PAP_SHIFT_LO;
    z2 = world_z + PLAT_LOOK >> PAP_SHIFT_LO;

    SATURATE(x1, 0, PAP_SIZE_LO - 1);
    SATURATE(z1, 0, PAP_SIZE_LO - 1);

    SATURATE(x2, 0, PAP_SIZE_LO - 1);
    SATURATE(z2, 0, PAP_SIZE_LO - 1);

    // Find the closest static ob (by Manhattan distance, with Y weighted less).
    best_score = UC_INFINITY;

    for (mx = x1; mx <= x2; mx++)
        for (mz = z1; mz <= z2; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                dx = oi->x - world_x;
                dy = oi->y - world_y;
                dz = oi->z - world_z;

                score = abs(dx) + abs(dz) + (abs(dy) >> 2);

                if (score < best_score) {
                    best_score = score;
                    best_oi = *oi;
                }
            }
        }

    if (best_score == UC_INFINITY) {
        return NULL;
    }

    dm = alloc_draw_mesh();

    if (dm == NULL) {
        ASSERT(0);
        return NULL;
    }

    p_thing = alloc_thing(CLASS_PLAT);

    if (p_thing == NULL) {
        return NULL;
    }

    plat = &PLAT_plat[PLAT_plat_upto++];

    p_thing->Class = CLASS_PLAT;
    p_thing->WorldPos.X = best_oi.x << 8;
    p_thing->WorldPos.Y = best_oi.y << 8;
    p_thing->WorldPos.Z = best_oi.z << 8;
    p_thing->Flags = 0;
    p_thing->State = STATE_NORMAL;
    p_thing->StateFn = PLAT_process;
    p_thing->DrawType = DT_MESH;
    p_thing->Draw.Mesh = dm;
    p_thing->Genus.Plat = plat;

    dm->Angle = best_oi.yaw;
    dm->Tilt = 0;
    dm->Roll = 0;
    dm->ObjectId = best_oi.prim;
    dm->Cache = 0;
    dm->Hm = 0;

    plat->used = UC_TRUE;
    plat->colour = colour;
    plat->group = group;
    plat->move = move;
    plat->state = PLAT_STATE_NONE;
    plat->counter = 0;
    plat->waypoint = 0;
    plat->speed = 0;
    plat->wspeed = speed;
    plat->flag = flag;

    add_thing_to_map(p_thing);

    OB_remove(&best_oi);

    WMOVE_create(p_thing);

    return THING_NUMBER(p_thing);
}
