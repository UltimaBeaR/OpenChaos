#include "fallen/Headers/Game.h"     // Temporary: Thing, GAME_TURN, TICK_RATIO, TICK_SHIFT, PAP_calc_map_height_at, etc.
#include "fallen/Headers/sound.h"    // Temporary: S_EXPLODE_START, S_KICK_CAN
#include "fallen/Headers/fmatrix.h"  // Temporary: FMATRIX_vector
#include "fallen/Headers/pcom.h"     // Temporary: PCOM_oscillate_tympanum, PCOM_person_wants_to_kill
#include "actors/characters/anim_ids.h"
#include "fallen/Headers/dirt.h"     // Temporary: DIRT_new_sparks, DIRT_gust
#include "engine/graphics/pipeline/poly.h"
#include "mesh.h"                    // Temporary: MESH_draw_poly
#include "fallen/Headers/statedef.h" // Temporary: STATE_IDLE
#include "DrawXtra.h"                // Temporary: AENG_world_line_nondebug
#include "engine/graphics/geometry/bloom.h"
#include "actors/items/grenade.h"
#include "actors/items/grenade_globals.h"

// Forward declarations for functions not yet migrated.
// uc_orig: PANEL_draw_gun_sight (fallen/Source/gamemenu.cpp or frontend.cpp)
void PANEL_draw_gun_sight(SLONG mx, SLONG my, SLONG mz, SLONG accuracy, SLONG scale);
// uc_orig: GAMEMENU_is_paused (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_is_paused(void);
// uc_orig: GAMEMENU_slowdown_mul (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_slowdown_mul();

// Simulate one physics tick for a grenade.
// If explode=0, explosion is suppressed (used for trajectory preview).
// uc_orig: ProcessGrenade (fallen/Source/grenade.cpp)
static void ProcessGrenade(Grenade* gp, SLONG explode, SLONG ii)
{
    if (!gp->owner)
        return;

    SLONG ticks = 16 * TICK_RATIO >> TICK_SHIFT;

    if (gp->timer < ticks) {
        if (explode)
            CreateGrenadeExplosion(gp->x, gp->y, gp->z, gp->owner);
        gp->owner = NULL;
        return;
    } else {
        gp->timer -= ticks;
    }

    // Periodically alert nearby NPCs to the grenade's presence.
    if (explode)
        if (!(GAME_TURN & 0x0F)) {
            PCOM_oscillate_tympanum(PCOM_SOUND_GRENADE_FLY, gp->owner, gp->x >> 8, gp->y >> 8, gp->z >> 8);
        }

    // Apply gravity.
    gp->dy -= TICK_RATIO * 2;
    if (gp->dy < -0x2000)
        gp->dy = -0x2000;

    SLONG oldx = gp->x;
    SLONG oldy = gp->y;
    SLONG oldz = gp->z;

    gp->x += (gp->dx * TICK_RATIO) >> TICK_SHIFT;
    gp->y += (gp->dy * TICK_RATIO) >> TICK_SHIFT;
    gp->z += (gp->dz * TICK_RATIO) >> TICK_SHIFT;

    gp->yaw += gp->dyaw;
    gp->pitch += gp->dpitch;

    SLONG floor = (PAP_calc_map_height_at(gp->x >> 8, gp->z >> 8) + 11) << 8;

    if (gp->y < floor) {
        SLONG under = floor - gp->y;

        if (explode)
            if (abs(gp->dy) > 0x80000) {
                MFX_play_xyz(ii, S_KICK_CAN, MFX_REPLACE, gp->x, gp->y, gp->z);
                PCOM_oscillate_tympanum(PCOM_SOUND_GRENADE_HIT, gp->owner, oldx >> 8, oldy >> 8, oldz >> 8);
            }

        if ((under > 0x4000) || (gp->dy > 0)) {
            // Hit a building wall; reflect the axis that crossed a tile boundary.
            if ((oldx >> 16) != (gp->x >> 16)) {
                gp->x = oldx;
                gp->dx = -(gp->dx >> 1);
            }

            if ((oldz >> 16) != (gp->z >> 16)) {
                gp->z = oldz;
                gp->dz = -(gp->dz >> 1);
            }
        } else {
            // Bounce off the floor; stop completely when vertical speed is negligible.
            gp->dy = abs(gp->dy) >> 1;
            gp->y = oldy;

            if (gp->dy <= 0x400) {
                gp->dy = 0;
                gp->dx = 0;
                gp->dz = 0;
                gp->dyaw = 0;
                gp->dpitch = 0;
            }
        }
    }
}

// uc_orig: InitGrenades (fallen/Source/grenade.cpp)
void InitGrenades()
{
    for (int ii = 0; ii < MAX_GRENADES; ii++) {
        GrenadeArray[ii].owner = NULL;
    }
}

// uc_orig: CreateGrenade (fallen/Source/grenade.cpp)
bool CreateGrenade(Thing* owner, SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, SLONG timer)
{
    int ix;
    for (ix = 0; ix < MAX_GRENADES; ix++) {
        if (!GrenadeArray[ix].owner)
            break;
    }

    if (ix == MAX_GRENADES)
        return false;

    Grenade* gp = &GrenadeArray[ix];

    global_g = gp;

    gp->owner = owner;

    gp->x = x;
    gp->y = y;
    gp->z = z;
    gp->yaw = 0;
    gp->pitch = 0;

    gp->dx = dx;
    gp->dy = dy;
    gp->dz = dz;

    gp->dyaw = 0;
    gp->dpitch = 50;

    gp->timer = timer;

    return true;
}

// uc_orig: CreateGrenadeFromPerson (fallen/Source/grenade.cpp)
bool CreateGrenadeFromPerson(Thing* p_person, SLONG timer)
{
    SLONG vector[3];

    FMATRIX_vector(vector, p_person->Draw.Tweened->Angle, 0);

    SLONG dx = -vector[0] * 181 >> 11;
    SLONG dz = -vector[2] * 181 >> 11;
    SLONG dy = 181 << 6;

    // NPCs attacking a nearby target throw with half speed for shorter arc.
    if (p_person->Genus.Person->PlayerID) {
    } else {
        SLONG i_target = PCOM_person_wants_to_kill(p_person);

        if (i_target) {
            Thing* p_target = TO_THING(i_target);

            SLONG dpx = abs(p_target->WorldPos.X - p_person->WorldPos.X >> 8);
            SLONG dpz = abs(p_target->WorldPos.Z - p_person->WorldPos.Z >> 8);

            SLONG pdist = QDIST2(dpx, dpz);

            if (pdist < 0x500) {
                dx >>= 1;
                dz >>= 1;
                dy >>= 1;
            }
        }
    }

    SLONG px, py, pz;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_HAND, &px, &py, &pz);

    px = (px << 8) + p_person->WorldPos.X;
    py = (py << 8) + p_person->WorldPos.Y;
    pz = (pz << 8) + p_person->WorldPos.Z;

    return CreateGrenade(p_person, px, py, pz, dx, dy, dz, timer);
}

// uc_orig: DrawGrenades (fallen/Source/grenade.cpp)
void DrawGrenades()
{
    SLONG angle;

    for (int ii = 0; ii < MAX_GRENADES; ii++) {
        Grenade* gp = &GrenadeArray[ii];

        if (gp->owner) {
            MESH_draw_poly(PRIM_OBJ_ITEM_GRENADE, gp->x >> 8, gp->y >> 8, gp->z >> 8, gp->yaw, gp->pitch, 0, NULL, 0xff);

            angle = GAME_TURN;
            angle <<= 6;
            angle &= 2047;

            SLONG dx = SIN(angle) >> 8;
            SLONG dz = COS(angle) >> 8;

            BLOOM_draw(
                (gp->x >> 8),
                (gp->y >> 8) + 8,
                (gp->z >> 8),
                dx,
                0, dz,
                0x007f5d,
                0);
        }
    }
}

// uc_orig: ProcessGrenades (fallen/Source/grenade.cpp)
void ProcessGrenades(void)
{
    for (int ii = 0; ii < MAX_GRENADES; ii++) {
        Grenade* gp = &GrenadeArray[ii];

        ProcessGrenade(gp, 1, ii);
    }
}

// uc_orig: CreateGrenadeExplosion (fallen/Source/grenade.cpp)
void CreateGrenadeExplosion(SLONG x, SLONG y, SLONG z, Thing* owner)
{
    SLONG ytest = PAP_calc_map_height_at(x >> 8, z >> 8) << 8;

    if (y <= ytest)
        y = ytest + 1;

    MFX_play_xyz(0, S_EXPLODE_START, MFX_OVERLAP, x, y, z);

    PARTICLE_Add(x, y, z, 0, 0, 0, (Random() & 1) ? POLY_PAGE_EXPLODE1_ADDITIVE : POLY_PAGE_EXPLODE2_ADDITIVE, 2,
        0x00FFFFFF, PFLAG_SPRITEANI | PFLAG_RESIZE | PFLAG_BOUNCE, 120, 190 + (Random() & 0x3f), 1, 0, 20);
    PARTICLE_Add(x + (((Random() & 0xff) - 0x7f) << 4), y + (((Random() & 0xff) - 0x7f) << 4), z + (((Random() & 0xff) - 0x7f) << 4),
        0, 0, 0, (Random() & 1) ? POLY_PAGE_EXPLODE1_ADDITIVE : POLY_PAGE_EXPLODE2_ADDITIVE, 2,
        0x7fFFFFFF, PFLAG_SPRITEANI | PFLAG_RESIZE, 70, 120 + (Random() & 0x7f), 1, 0, 40);

    int iNumParticles = IWouldLikeSomePyroSpritesHowManyCanIHave(20 * 4);
    iNumParticles /= 4;

    for (int ii = 0; ii < iNumParticles; ii++) {
        PARTICLE_Add(x + (((Random() & 0x7f) - 0x3f) << 9), y + (((Random() & 0x3f) - 0x1f) << 6), z + (((Random() & 0x7f) - 0x3f) << 9),
            ((Random() & 0x1f) - 0xf) << 1, (1 + (Random() & 0xf)) << 6, ((Random() & 0x1f) - 0xf) << 1,
            POLY_PAGE_SMOKECLOUD, 2, 0xFFFFFFFF, PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE | PFLAG_RESIZE,
            150, 20 + (Random() & 0x7f), 1, 2 + (Random() & 3), 3);
        PARTICLE_Add(x, y, z, ((Random() & 0x1f) - 0xf) << 6, (Random() & 0x1f) << 6, ((Random() & 0x1f) - 0xf) << 6,
            POLY_PAGE_EXPLODE1 - (Random() & 1), 2 + ((Random() & 3) << 2), 0xFFFFFF, PFLAG_GRAVITY | PFLAG_RESIZE2 | PFLAG_FADE | PFLAG_INVALPHA,
            240, 20 + (Random() & 0x1f), 1, 3 + (Random() & 3), 0);

        if (Random() & 3)
            PARTICLE_Add(x, y, z, ((Random() & 0x1f) - 0xf) << 8, (Random() & 0x1f) << 8, ((Random() & 0x1f) - 0xf) << 8,
                POLY_PAGE_EXPLODE1 - (Random() & 1), 2 + ((Random() & 1) << 2), 0xFFFFFF, PFLAG_GRAVITY | PFLAG_RESIZE2 | PFLAG_FADE | PFLAG_INVALPHA | PFLAG_BOUNCE,
                240, 5, 1, 2 + (Random() & 3), 0);
        else
            PARTICLE_Add(x, y, z, ((Random() & 0x1f) - 0xf) << 12, (Random() & 0x1f) << 8, ((Random() & 0x1f) - 0xf) << 12,
                POLY_PAGE_EXPLODE1 - (Random() & 1), 2 + ((Random() & 3) << 2), 0xFFFFFF, PFLAG_GRAVITY | PFLAG_RESIZE2 | PFLAG_FADE | PFLAG_INVALPHA,
                240, 5, 1, 3, 0);
    }

    SLONG sx = x >> 8;
    SLONG sy = y >> 8;
    SLONG sz = z >> 8;

    DIRT_new_sparks(sx, sy, sz, 2);
    DIRT_new_sparks(sx, sy, sz, 2 | 64);
    DIRT_new_sparks(sx, sy, sz, 2 | 128);

    DIRT_gust(0, sx, sz, sx + 200, sz);
    DIRT_gust(0, sx, sz, sx - 200, sz);
    DIRT_gust(0, sx, sz, sx, sz + 200);
    DIRT_gust(0, sx, sz, sx, sz - 200);

    create_shockwave(sx, sy, sz, 0x300, 500, owner);
}

// uc_orig: show_grenade_path (fallen/Source/grenade.cpp)
void show_grenade_path(Thing* p_person)
{
    Grenade* gp;
    SLONG x, y, z, x1, y1, z1;
    SLONG count = (-GAME_TURN);

    if (GAMEMENU_is_paused())
        return;

    if (GAMEMENU_slowdown_mul() == 256)
        if (p_person->State == STATE_IDLE)
            if (CreateGrenadeFromPerson(p_person, 6000)) {
                gp = global_g;

                x = gp->x >> 8;
                y = gp->y >> 8;
                z = gp->z >> 8;

                while (gp->owner) {
                    ProcessGrenade(gp, 0, 0);
                    x1 = gp->x >> 8;
                    y1 = gp->y >> 8;
                    z1 = gp->z >> 8;

                    if (((count & 7) == 0) || ((count & 7) == 1)) {
                        AENG_world_line_nondebug(
                            x, y, z,
                            3,
                            0x8000af00,
                            x1, y1, z1,
                            3,
                            0x8000af00,
                            UC_TRUE);
                    }

                    count++;
                    x = x1;
                    y = y1;
                    z = z1;
                }
                PANEL_draw_gun_sight(x, y, z, 1000, 400);
            }
}
