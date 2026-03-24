#include "engine/platform/uc_common.h"
#include <string.h>

#include "things/core/statedef.h"
#include "missions/eway.h"
#include "ai/pcom.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "effects/dirt.h"
#include "things/characters/anim_ids.h"
#include "things/vehicles/vehicle.h"
#include "things/vehicles/vehicle_globals.h"

#include "things/items/special.h"
#include "things/items/special_globals.h"
#include "things/items/grenade.h"
#include "things/core/interact.h"
#include "things/core/thing.h"
#include "missions/game_types.h"
#include "things/core/thing_globals.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "assets/sound_id_globals.h"
#include "effects/pyro.h"
#include "effects/pow.h"
#include "ui/hud/overlay.h"
#include "assets/xlat_str.h"
#include "world/map/pap.h"
#include "engine/core/memory.h"
#include "engine/physics/collide.h"
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "ui/menus/cnet.h"
#include "ui/menus/cnet_globals.h"
#include "engine/audio/sound.h"
#include "engine/graphics/postprocess/bloom.h"

// Prototype: free_special is used before its definition in this file.
// uc_orig: free_special (fallen/Source/Special.cpp)
void free_special(Thing* s_thing);

// uc_orig: find_nice_place_near_person (fallen/Source/Person.cpp)
extern void find_nice_place_near_person(Thing* p_person, SLONG* nice_x, SLONG* nice_y, SLONG* nice_z);

// Prototype: find_empty_special used before its definition in this file.
// uc_orig: find_empty_special (fallen/Source/Special.cpp)
SLONG find_empty_special(void);

// health table (indexed by PersonType), defined in Person.cpp.
// uc_orig: health (fallen/Source/Person.cpp)
extern SWORD health[];

// Global stat bonus counter for badge pickups, defined in Game.cpp.
// uc_orig: stat_count_bonus (fallen/Source/Game.cpp)
extern SLONG stat_count_bonus;

// ---------------------------------------------------------------------------
// Inventory management
// ---------------------------------------------------------------------------

// uc_orig: special_pickup (fallen/Source/Special.cpp)
void special_pickup(Thing* p_special, Thing* p_person)
{
    ASSERT(p_special->Class == CLASS_SPECIAL);
    ASSERT(p_person->Class == CLASS_PERSON);
    ASSERT(p_special->Genus.Special->SpecialType != SPECIAL_MINE);

    p_special->Genus.Special->OwnerThing = THING_NUMBER(p_person);
    p_special->Genus.Special->NextSpecial = p_person->Genus.Person->SpecialList;
    p_person->Genus.Person->SpecialList = THING_NUMBER(p_special);

    if (p_special->SubState == SPECIAL_SUBSTATE_PROJECTILE) {
        p_special->SubState = SPECIAL_SUBSTATE_NONE;
    }
}

// uc_orig: special_drop (fallen/Source/Special.cpp)
void special_drop(Thing* p_special, Thing* p_person)
{
    ASSERT(p_special->Class == CLASS_SPECIAL);
    ASSERT(p_person->Class == CLASS_PERSON);
    ASSERT(p_special->Genus.Special->SpecialType != SPECIAL_MINE);

    UWORD next = p_person->Genus.Person->SpecialList;
    UWORD* prev = &p_person->Genus.Person->SpecialList;

    while (1) {
        if (next == NULL) {
            return;
        }

        Thing* p_next = TO_THING(next);
        ASSERT(p_next->Class == CLASS_SPECIAL);

        if (next == THING_NUMBER(p_special)) {
            *prev = p_special->Genus.Special->NextSpecial;
            p_special->Genus.Special->NextSpecial = NULL;
            p_special->Genus.Special->OwnerThing = NULL;

            {
                SLONG gx;
                SLONG gy;
                SLONG gz;

                find_nice_place_near_person(p_person, &gx, &gy, &gz);

                p_special->WorldPos.X = gx << 8;
                p_special->WorldPos.Y = gy << 8;
                p_special->WorldPos.Z = gz << 8;
            }

            add_thing_to_map(p_special);

            // Player-dropped weapons get reduced ammo to discourage drop/pickup cycling.
            if (p_person->Genus.Person->PlayerID == 0) {
                switch (p_special->Genus.Special->SpecialType) {
                case SPECIAL_SHOTGUN:
                    p_special->Genus.Special->ammo = (Random() & 0x1) + 2;
                    break;

                case SPECIAL_AK47:
                    p_special->Genus.Special->ammo = (Random() & 0x7) + 6;
                    break;
                }
            }

            p_special->Velocity = 5;
            p_special->Genus.Special->counter = 0;
            p_special->SubState = SPECIAL_SUBSTATE_NONE;

            if (next == p_person->Genus.Person->SpecialUse) {
                p_person->Genus.Person->SpecialUse = NULL;
            }

            if (next == p_person->Genus.Person->SpecialDraw) {
                p_person->Genus.Person->SpecialDraw = NULL;
            }

            return;
        }

        prev = &p_next->Genus.Special->NextSpecial;
        next = p_next->Genus.Special->NextSpecial;
    }
}

// uc_orig: person_has_twohanded_weapon (fallen/Source/Special.cpp)
SLONG person_has_twohanded_weapon(Thing* p_person)
{
    return (person_has_special(p_person, SPECIAL_SHOTGUN) || person_has_special(p_person, SPECIAL_AK47) || person_has_special(p_person, SPECIAL_BASEBALLBAT));
}

// ---------------------------------------------------------------------------
// Pickup eligibility
// ---------------------------------------------------------------------------

// uc_orig: should_person_get_item (fallen/Source/Special.cpp)
SLONG should_person_get_item(Thing* p_person, Thing* p_special)
{
    Thing* p_grenade;
    Thing* p_explosives;
    Thing* p_has;

    if (p_person->State == STATE_MOVEING && p_person->SubState == SUB_STATE_RUNNING_SKID_STOP) {
        return UC_FALSE;
    }

    switch (p_special->Genus.Special->SpecialType) {
    case SPECIAL_GUN:

        if (p_person->Flags & FLAGS_HAS_GUN) {
            return (p_person->Genus.Person->ammo_packs_pistol < (255 - SPECIAL_AMMO_IN_A_PISTOL));
        } else {
            return UC_TRUE;
        }

    case SPECIAL_HEALTH:

        return (p_person->Genus.Person->Health < health[p_person->Genus.Person->PersonType]);

    case SPECIAL_BOMB:

        return UC_FALSE;

    case SPECIAL_SHOTGUN:

        p_has = person_has_special(p_person, SPECIAL_SHOTGUN);

        if (p_has) {
            return (p_person->Genus.Person->ammo_packs_shotgun < (255 - SPECIAL_AMMO_IN_A_SHOTGUN));
        } else {
            return UC_TRUE;
        }

    case SPECIAL_AK47:

        p_has = person_has_special(p_person, SPECIAL_AK47);

        if (p_has) {
            return (p_person->Genus.Person->ammo_packs_ak47 < (255 - SPECIAL_AMMO_IN_A_AK47));
        } else {
            return UC_TRUE;
        }

    case SPECIAL_KNIFE:

        return !person_has_special(p_person, SPECIAL_KNIFE);

    case SPECIAL_BASEBALLBAT:

        return !person_has_special(p_person, SPECIAL_BASEBALLBAT);

    case SPECIAL_CROWBAR:

        return UC_TRUE;

    case SPECIAL_EXPLOSIVES:

        p_explosives = person_has_special(p_person, SPECIAL_EXPLOSIVES);

        if (p_explosives) {
            return (p_explosives->Genus.Special->ammo < 4);
        } else {
            return UC_TRUE;
        }

    case SPECIAL_GRENADE:

        p_grenade = person_has_special(p_person, SPECIAL_GRENADE);

        if (p_grenade) {
            return (p_grenade->Genus.Special->ammo < 8);
        } else {
            return UC_TRUE;
        }

    case SPECIAL_AMMO_SHOTGUN:

        return (p_person->Genus.Person->ammo_packs_shotgun < (255 - SPECIAL_AMMO_IN_A_SHOTGUN));

    case SPECIAL_AMMO_AK47:

        return (p_person->Genus.Person->ammo_packs_ak47 < (255 - SPECIAL_AMMO_IN_A_AK47));

    case SPECIAL_AMMO_PISTOL:

        return (p_person->Genus.Person->ammo_packs_pistol < (255 - SPECIAL_AMMO_IN_A_PISTOL));

    case SPECIAL_MINE:

        return UC_FALSE;

    default:

        return UC_TRUE;
    }
}

// ---------------------------------------------------------------------------
// Item pickup execution
// ---------------------------------------------------------------------------

// uc_orig: person_get_item (fallen/Source/Special.cpp)
void person_get_item(Thing* p_person, Thing* p_special)
{
    SLONG keep = UC_FALSE;
    SLONG x_message = 0;
    SLONG overflow = 0;

    switch (p_special->Genus.Special->SpecialType) {
    case SPECIAL_GUN:

        if (p_person->Flags & FLAGS_HAS_GUN) {
            x_message = X_AMMO;

            overflow = (SWORD)p_person->Genus.Person->Ammo + p_special->Genus.Special->ammo;
            while (overflow > 15) {
                p_person->Genus.Person->ammo_packs_pistol += 15;
                overflow -= 15;
            }
            p_person->Genus.Person->Ammo = overflow;
        } else {
            x_message = X_GUN;

            p_person->Genus.Person->Ammo = p_special->Genus.Special->ammo;
            p_person->Flags |= FLAGS_HAS_GUN;
        }

        break;

    case SPECIAL_HEALTH:

        x_message = X_HEALTH;

        p_person->Genus.Person->Health += 100;

        if (p_person->Genus.Person->Health > health[p_person->Genus.Person->PersonType]) {
            p_person->Genus.Person->Health = health[p_person->Genus.Person->PersonType];
        }

        break;

    case SPECIAL_AMMO_SHOTGUN:
        p_person->Genus.Person->ammo_packs_shotgun += SPECIAL_AMMO_IN_A_SHOTGUN;

        x_message = X_AMMO;

        break;

    case SPECIAL_AMMO_AK47:

        p_person->Genus.Person->ammo_packs_ak47 += SPECIAL_AMMO_IN_A_AK47;

        x_message = X_AMMO;

        break;

    case SPECIAL_AMMO_PISTOL:

        p_person->Genus.Person->ammo_packs_pistol += SPECIAL_AMMO_IN_A_PISTOL;

        x_message = X_AMMO;

        break;

    case SPECIAL_MINE:

        // Mines cannot be picked up; this code path must never be reached.
        ASSERT(0);

        break;

    case SPECIAL_SHOTGUN:

    {
        Thing* p_gun = person_has_special(p_person, SPECIAL_SHOTGUN);

        if (p_gun) {
            overflow = (SWORD)p_gun->Genus.Special->ammo + p_special->Genus.Special->ammo;
            while (overflow > SPECIAL_AMMO_IN_A_SHOTGUN) {
                p_person->Genus.Person->ammo_packs_shotgun += SPECIAL_AMMO_IN_A_SHOTGUN;
                overflow -= SPECIAL_AMMO_IN_A_SHOTGUN;
            }
            p_gun->Genus.Special->ammo = overflow;

            x_message = X_AMMO;

            break;
        } else {
            special_pickup(p_special, p_person);

            remove_thing_from_map(p_special);
            keep = UC_TRUE;

            x_message = X_SHOTGUN;

            break;
        }
    }

    case SPECIAL_AK47:

    {
        Thing* p_ak47 = person_has_special(p_person, SPECIAL_AK47);

        if (p_ak47) {
            overflow = (SWORD)p_ak47->Genus.Special->ammo + p_special->Genus.Special->ammo;
            while (overflow > SPECIAL_AMMO_IN_A_AK47) {
                p_person->Genus.Person->ammo_packs_ak47 += SPECIAL_AMMO_IN_A_AK47;
                overflow -= SPECIAL_AMMO_IN_A_AK47;
            }
            p_ak47->Genus.Special->ammo = overflow;

            x_message = X_AMMO;

            break;
        } else {
            special_pickup(p_special, p_person);

            remove_thing_from_map(p_special);
            keep = UC_TRUE;

            x_message = X_AK;

            break;
        }
    }

    case SPECIAL_EXPLOSIVES:

    {
        Thing* p_explosives = person_has_special(p_person, SPECIAL_EXPLOSIVES);

        if (p_explosives) {
            p_explosives->Genus.Special->ammo += 1;
        } else {
            special_pickup(p_special, p_person);

            remove_thing_from_map(p_special);
            keep = UC_TRUE;
        }

        if (p_special->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            // Defuse automatically if picked up while armed.
            p_special->SubState = SPECIAL_SUBSTATE_NONE;
        }

        x_message = X_EXPLOSIVES;

        break;
    }

    case SPECIAL_GRENADE:

    {
        Thing* p_grenade = person_has_special(p_person, SPECIAL_GRENADE);

        if (p_grenade) {
            if (p_grenade->Genus.Special->ammo < 100) {
                p_grenade->Genus.Special->ammo += SPECIAL_AMMO_IN_A_GRENADE;
            }

            x_message = X_AMMO;

            break;
        } else {
            special_pickup(p_special, p_person);

            remove_thing_from_map(p_special);
            keep = UC_TRUE;

            x_message = X_GRENADE;

            break;
        }
    }

    default:

        // Mission-critical items: keys, keycards, files, disks, melee weapons, etc.
        // Melee weapons and mission items all land here; only specific types get a HUD message.
        if (p_special->Genus.Special->SpecialType == SPECIAL_KNIFE)       { x_message = X_KNIFE; }
        if (p_special->Genus.Special->SpecialType == SPECIAL_BASEBALLBAT) { x_message = X_BASEBALL_BAT; }
        if (p_special->Genus.Special->SpecialType == SPECIAL_KEY)         { x_message = X_KEYCARD; }
        if (p_special->Genus.Special->SpecialType == SPECIAL_VIDEO)       { x_message = X_VIDEO; }

        special_pickup(p_special, p_person);

        remove_thing_from_map(p_special);
        keep = UC_TRUE;

        break;
    }

    // Show HUD message only for the human player (PlayerID != 0); NPCs don't get HUD feedback.
    if (x_message && p_person->Genus.Person->PlayerID) {
        PANEL_new_info_message(XLAT_str(x_message));
    }

    // Notify the waypoint that spawned this item.
    if (p_special->Genus.Special->waypoint) {
        EWAY_item_pickedup(p_special->Genus.Special->waypoint);
    }

    if (!keep) {
        free_special(p_special);
    }
}

// ---------------------------------------------------------------------------
// Mine detonation
// ---------------------------------------------------------------------------

// uc_orig: special_activate_mine (fallen/Source/Special.cpp)
void special_activate_mine(Thing* p_mine)
{
    ASSERT(p_mine->Class == CLASS_SPECIAL);
    ASSERT(p_mine->Genus.Special->SpecialType == SPECIAL_MINE);

    if (NET_PERSON(0)->Genus.Person->Target == THING_NUMBER(p_mine)) {
        NET_PERSON(0)->Genus.Person->Target = NULL;
    }

    PYRO_create(p_mine->WorldPos, PYRO_FIREBOMB);
    PYRO_create(p_mine->WorldPos, PYRO_DUSTWAVE);

    MFX_play_xyz(THING_NUMBER(p_mine), SOUND_Range(S_EXPLODE_START, S_EXPLODE_END), 0, p_mine->WorldPos.X, p_mine->WorldPos.Y, p_mine->WorldPos.Z);

    create_shockwave(
        p_mine->WorldPos.X >> 8,
        p_mine->WorldPos.Y >> 8,
        p_mine->WorldPos.Z >> 8,
        0x200,
        250,
        NULL);

    free_special(p_mine);
}

// Dead global present in the original but never referenced. Preserved 1:1.
// uc_orig: special_di (fallen/Source/Special.cpp)
// Note: this is file-scope but unused; kept in _globals would require exposing DIRT_Info
// in the header chain — left as a comment here since it is unreferenced dead data.
// The actual definition lives in special_globals.cpp for completeness.

// ---------------------------------------------------------------------------
// Per-tick state function
// ---------------------------------------------------------------------------

// uc_orig: special_normal (fallen/Source/Special.cpp)
void special_normal(Thing* s_thing)
{
    SLONG i;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;

    // Advance the pickup cooldown counter (dual-use: also holds containing-object ID for hidden items).
    if (s_thing->Flags & FLAG_SPECIAL_HIDDEN) {
        // Counter is the ob this special is inside.
    } else {
        s_thing->Genus.Special->counter += 16 * TICK_RATIO >> TICK_SHIFT;
    }

    // PROJECTILE substate: physics for an item that was spawned or dropped mid-air.
    // timer encodes signed velocity as a UWORD biased by 0x8000 (zero = 0x8000).
    if (s_thing->SubState == SPECIAL_SUBSTATE_PROJECTILE) {
        SpecialPtr ss;
        SLONG velocity;
        SLONG ground;

        ss = s_thing->Genus.Special;
        velocity = ss->timer;
        velocity -= 0x8000;
        velocity -= 256; // gravity

        ground = PAP_calc_map_height_at(s_thing->WorldPos.X >> 8, s_thing->WorldPos.Z >> 8) + 0x30;

        s_thing->WorldPos.Y += velocity;

        if (s_thing->WorldPos.Y >> 8 < ground) {
            s_thing->WorldPos.Y = ground << 8;
            velocity = -(velocity >> 1); // 50% bounce

            if (velocity < 0x100) {
                // Came to rest.
                s_thing->SubState = SPECIAL_SUBSTATE_NONE;
                ss->timer = 0;
                return;
            }
        }

        ss->timer = (UWORD)(velocity + 0x8000);
        return;
    }

    if (s_thing->Genus.Special->OwnerThing) {
        // Carried item: track owner's position.
        Thing* p_person = TO_THING(s_thing->Genus.Special->OwnerThing);

        if (s_thing->Genus.Special->SpecialType != SPECIAL_EXPLOSIVES) {
            s_thing->WorldPos = p_person->WorldPos;
        }

        // Grenade fuse countdown while carried.
        if (s_thing->Genus.Special->SpecialType == SPECIAL_GRENADE && s_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            if (s_thing->Genus.Special->timer <= (UWORD)(16 * TICK_RATIO >> TICK_SHIFT)) {
                // Grenade cooked off in hand — detonate.
                if (s_thing->Genus.Special->ammo > 1) {
                    s_thing->Genus.Special->ammo -= 1;
                    s_thing->SubState = SPECIAL_SUBSTATE_NONE;
                } else {
                    CreateGrenadeExplosion(p_person->WorldPos.X, p_person->WorldPos.Y, p_person->WorldPos.Z, p_person);

                    special_drop(s_thing, p_person);
                    free_special(s_thing);
                    p_person->Genus.Person->SpecialUse = NULL;
                }
                return;
            } else {
                s_thing->Genus.Special->timer -= 16 * TICK_RATIO >> TICK_SHIFT;
            }
        }

        return;
    }

    // Item is on the ground.

    // Rotate most items to make them visible.
    if (s_thing->DrawType == DT_MESH) {
        if (s_thing->Genus.Special->SpecialType != SPECIAL_EXPLOSIVES || s_thing->SubState != SPECIAL_SUBSTATE_ACTIVATED)
            s_thing->Draw.Mesh->Angle = (s_thing->Draw.Mesh->Angle + 32) & 2047;
    }

    switch (s_thing->Genus.Special->SpecialType) {
    case SPECIAL_BOMB:

        if (s_thing->Genus.Special->waypoint) {
            if (s_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
                if (EWAY_is_active(s_thing->Genus.Special->waypoint)) {
                    PYRO_construct(s_thing->WorldPos, -1, 256);
                    free_special(s_thing);
                }
            }
        }

        break;

    case SPECIAL_EXPLOSIVES:

        if (s_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            SLONG tickdown = 0x10 * TICK_RATIO >> TICK_SHIFT;

            if (s_thing->Genus.Special->timer <= tickdown) {
                // Explode.
                PYRO_construct(s_thing->WorldPos, 1 | 4 | 8 | 64, 0xa0);
                MFX_play_xyz(THING_NUMBER(s_thing), SOUND_Range(S_EXPLODE_MEDIUM, S_EXPLODE_BIG), 0, s_thing->WorldPos.X, s_thing->WorldPos.Y, s_thing->WorldPos.Z);

                {
                    SLONG cx = s_thing->WorldPos.X >> 8;
                    SLONG cz = s_thing->WorldPos.Z >> 8;
                    DIRT_gust(s_thing, cx, cz, cx + 0xa0, cz);
                    DIRT_gust(s_thing, cx + 0xa0, cz, cx, cz);
                }

                create_shockwave(
                    s_thing->WorldPos.X >> 8,
                    s_thing->WorldPos.Y >> 8,
                    s_thing->WorldPos.Z >> 8,
                    0x500,
                    500,
                    TO_THING(s_thing->Genus.Special->OwnerThing));

                free_special(s_thing);
            } else {
                s_thing->Genus.Special->timer -= tickdown;
            }
        } else {
            if (!s_thing->Genus.Special->OwnerThing) {
                goto try_pickup;
            }
        }

        break;

    case SPECIAL_MINE:

        if (s_thing->Genus.Special->ammo > 0) {
            if (s_thing->Genus.Special->ammo == 1) {
                special_activate_mine(s_thing);
            } else {
                s_thing->Genus.Special->ammo -= 1;
            }
        } else if ((GAME_TURN & 1) == (THING_NUMBER(s_thing) & 1)) {
            SLONG j;
            SLONG wx;
            SLONG wy;
            SLONG wz;

            SLONG num_found = THING_find_sphere(
                s_thing->WorldPos.X >> 8,
                s_thing->WorldPos.Y >> 8,
                s_thing->WorldPos.Z >> 8,
                0x300,
                THING_array,
                THING_ARRAY_SIZE,
                (1 << CLASS_PERSON) | (1 << CLASS_VEHICLE) | (1 << CLASS_BAT));

            Thing* p_found;

            for (i = 0; i < num_found; i++) {
                p_found = TO_THING(THING_array[i]);

                switch (p_found->Class) {
                case CLASS_BAT:

                    if (p_found->Genus.Bat->type == BAT_TYPE_BALROG) {
                        dx = abs(p_found->WorldPos.X - s_thing->WorldPos.X);
                        dz = abs(p_found->WorldPos.Z - s_thing->WorldPos.Z);
                        dist = QDIST2(dx, dz);

                        if (dist < 0x6000) {
                            special_activate_mine(s_thing);
                            return;
                        }
                    }

                    break;

                case CLASS_PERSON:

                {
                    Thing* p_person = p_found;

                    dx = abs(p_person->WorldPos.X - s_thing->WorldPos.X);
                    dz = abs(p_person->WorldPos.Z - s_thing->WorldPos.Z);
                    dist = QDIST2(dx, dz);

                    if (dist < 0x3000) {
                        SLONG fx;
                        SLONG fy;
                        SLONG fz;

                        calc_sub_objects_position(
                            p_person,
                            p_person->Draw.Tweened->AnimTween,
                            SUB_OBJECT_LEFT_FOOT,
                            &fx,
                            &fy,
                            &fz);

                        fx += p_person->WorldPos.X >> 8;
                        fy += p_person->WorldPos.Y >> 8;
                        fz += p_person->WorldPos.Z >> 8;

                        if (fy > (s_thing->WorldPos.Y + 0x2000 >> 8)) {
                            // Person jumped over the mine.
                        } else {
                            // Bodyguards of the player do not trigger mines.
                            if (p_person->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD && TO_THING(EWAY_get_person(p_person->Genus.Person->pcom_ai_other))->Genus.Person->PlayerID) {
                                // skip
                            } else {
                                special_activate_mine(s_thing);
                                return;
                            }
                        }
                    }
                }

                break;

                case CLASS_VEHICLE:

                {
                    Thing* p_vehicle = p_found;

                    vehicle_wheel_pos_init(p_vehicle);

                    for (j = 0; j < 4; j++) {
                        vehicle_wheel_pos_get(j, &wx, &wy, &wz);

                        dx = abs(wx - (s_thing->WorldPos.X >> 8));
                        dy = abs(wy - (s_thing->WorldPos.Y >> 8));
                        dz = abs(wz - (s_thing->WorldPos.Z >> 8));
                        dist = QDIST3(dx, dy, dz);

                        if (dist < 0x50) {
                            special_activate_mine(s_thing);
                            return;
                        }
                    }
                }

                break;

                default:
                    ASSERT(0);
                    break;
                }
            }
        }

        break;

    case SPECIAL_THERMODROID:
        // No ground behaviour.
        break;

    case SPECIAL_TREASURE:

        for (i = 0; i < NO_PLAYERS; i++) {
            Thing* darci = NET_PERSON(i);

            if (!(darci->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
                dx = darci->WorldPos.X - s_thing->WorldPos.X >> 8;
                dy = darci->WorldPos.Y - s_thing->WorldPos.Y >> 8;
                dz = darci->WorldPos.Z - s_thing->WorldPos.Z >> 8;

                dist = abs(dx) + abs(dy) + abs(dz);

                if (dist < 0xa0) {
                    SLONG x_message;

                    // Badge type is encoded in the mesh ObjectId.
                    switch (s_thing->Draw.Mesh->ObjectId) {
                    case 81:
                        x_message = X_STA_INCREASED;
                        add_damage_text(
                            s_thing->WorldPos.X >> 8,
                            s_thing->WorldPos.Y + 0x6000 >> 8,
                            s_thing->WorldPos.Z >> 8,
                            XLAT_str(X_STA_INCREASED));
                        NET_PLAYER(0)->Genus.Player->Stamina++;
                        break;

                    case 94:
                        x_message = X_STR_INCREASED;
                        add_damage_text(
                            s_thing->WorldPos.X >> 8,
                            s_thing->WorldPos.Y + 0x6000 >> 8,
                            s_thing->WorldPos.Z >> 8,
                            XLAT_str(X_STR_INCREASED));
                        NET_PLAYER(0)->Genus.Player->Strength++;
                        break;

                    case 71:
                        x_message = X_REF_INCREASED;
                        add_damage_text(
                            s_thing->WorldPos.X >> 8,
                            s_thing->WorldPos.Y + 0x6000 >> 8,
                            s_thing->WorldPos.Z >> 8,
                            XLAT_str(X_REF_INCREASED));
                        NET_PLAYER(0)->Genus.Player->Skill++;
                        break;

                    case 39:
                        x_message = X_CON_INCREASED;
                        add_damage_text(
                            s_thing->WorldPos.X >> 8,
                            s_thing->WorldPos.Y + 0x6000 >> 8,
                            s_thing->WorldPos.Z >> 8,
                            XLAT_str(X_CON_INCREASED));
                        NET_PLAYER(0)->Genus.Player->Constitution++;
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }

                    PANEL_new_info_message(XLAT_str(x_message));
                    free_special(s_thing);
                    stat_count_bonus++;
                }
            }
        }

        break;

    case SPECIAL_HEALTH:

        extern SWORD health[];
        if (NET_PERSON(0)->Genus.Person->Health > health[NET_PERSON(0)->Genus.Person->PersonType] - 100) {
            break;
        }
        // fall through to try_pickup

    case SPECIAL_GRENADE:

        if (s_thing->Genus.Special->SpecialType == SPECIAL_GRENADE && s_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            break;
        }
        // fall through to try_pickup

    case SPECIAL_AK47:
    case SPECIAL_SHOTGUN:
    case SPECIAL_GUN:
    case SPECIAL_KNIFE:
    case SPECIAL_BASEBALLBAT:
    case SPECIAL_AMMO_AK47:
    case SPECIAL_AMMO_SHOTGUN:
    case SPECIAL_AMMO_PISTOL:
    default:

    try_pickup:;
        if (s_thing->Genus.Special->counter > 16 * 20) {
            if (s_thing->Flags & FLAGS_ON_MAPWHO) {
                Thing* darci = NET_PERSON(0);

                if (!(darci->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
                    dx = darci->WorldPos.X - s_thing->WorldPos.X >> 8;
                    dy = darci->WorldPos.Y - s_thing->WorldPos.Y >> 8;
                    dz = darci->WorldPos.Z - s_thing->WorldPos.Z >> 8;

                    dist = abs(dx) + abs(dy) + abs(dz);

                    if (dist < 0xa0) {
                        if (should_person_get_item(darci, s_thing)) {
                            person_get_item(darci, s_thing);
                        }
                    }
                }
            }
        }

        break;
    }
}

// ---------------------------------------------------------------------------
// Pool management
// ---------------------------------------------------------------------------

// uc_orig: init_specials (fallen/Source/Special.cpp)
void init_specials(void)
{
    memset((UBYTE*)SPECIALS, 0, sizeof(Special) * MAX_SPECIALS);
    SPECIAL_COUNT = 0;
}

// uc_orig: find_empty_special (fallen/Source/Special.cpp)
SLONG find_empty_special(void)
{
    SLONG c0;
    for (c0 = 1; c0 < MAX_SPECIALS; c0++) {
        if (SPECIALS[c0].SpecialType == SPECIAL_NONE) {
            return (c0);
        }
    }
    return NULL;
}

// uc_orig: alloc_special (fallen/Source/Special.cpp)
Thing* alloc_special(
    UBYTE type,
    UBYTE substate,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    UWORD waypoint)
{
    DrawMesh* dm;
    Special* new_special;
    Thing* special_thing = NULL;
    SLONG special_index;

    ASSERT(WITHIN(type, 1, SPECIAL_NUM_TYPES - 1));

    special_index = find_empty_special();
    dm = alloc_draw_mesh();
    special_thing = alloc_thing(CLASS_SPECIAL);

    if (!special_index || !dm || !special_thing) {
        if (special_index) {
            TO_SPECIAL(special_index)->SpecialType = SPECIAL_NONE;
        }
        if (dm) {
            free_draw_mesh(dm);
        }
        if (special_thing) {
            free_thing(special_thing);
        }

        // Pool full: hijack a low-priority on-map item far from the player.
        SLONG score;
        SLONG best_score = -1;
        Thing* best_thing = NULL;

        SLONG list = thing_class_head[CLASS_SPECIAL];

        while (list) {
            Thing* p_hijack = TO_THING(list);
            ASSERT(p_hijack->Class == CLASS_SPECIAL);
            list = p_hijack->NextLink;

            score = -1;

            if (!(p_hijack->Flags & FLAGS_ON_MAPWHO)) {
                continue;
            }
            if (p_hijack->Genus.Special->OwnerThing) {
                continue;
            }

            // Score by replaceability (knife/bat = most replaceable, guns = least).
            switch (p_hijack->Genus.Special->SpecialType) {
            case SPECIAL_KNIFE:
            case SPECIAL_BASEBALLBAT:
                score += 4000;
                break;
            case SPECIAL_AMMO_PISTOL:
            case SPECIAL_AMMO_SHOTGUN:
            case SPECIAL_AMMO_AK47:
                score += 3000;
                break;
            case SPECIAL_GUN:
            case SPECIAL_GRENADE:
            case SPECIAL_HEALTH:
            case SPECIAL_SHOTGUN:
            case SPECIAL_AK47:
                score += 2000;
                break;
            default:
                // Mission-critical types: never hijack.
                continue;
            }

            SLONG dx = abs(p_hijack->WorldPos.X - NET_PERSON(0)->WorldPos.X) >> 16;
            SLONG dz = abs(p_hijack->WorldPos.Z - NET_PERSON(0)->WorldPos.Z) >> 16;

            if (dx + dz > 12) {
                score += dx;
                score += dz;

                if (score > best_score) {
                    best_score = score;
                    best_thing = p_hijack;
                }
            }
        }

        if (best_thing) {
            remove_thing_from_map(best_thing);
            special_index = SPECIAL_NUMBER(best_thing->Genus.Special);
            special_thing = best_thing;
            dm = special_thing->Draw.Mesh;
        } else {
            return NULL;
        }
    }

    new_special = TO_SPECIAL(special_index);
    new_special->SpecialType = type;
    new_special->Thing = THING_NUMBER(special_thing);
    new_special->waypoint = waypoint;
    new_special->counter = 0;
    new_special->OwnerThing = NULL;

    special_thing->Genus.Special = new_special;
    special_thing->State = STATE_NORMAL;
    special_thing->SubState = substate;
    special_thing->StateFn = special_normal;

    special_thing->Draw.Mesh = dm;
    special_thing->DrawType = DT_MESH;
    dm->Angle = 0;
    dm->Tilt = 0;
    dm->Roll = 0;

    // Default ammo per weapon type.
    switch (type) {
    case SPECIAL_GUN:        new_special->ammo = SPECIAL_AMMO_IN_A_PISTOL;  break;
    case SPECIAL_SHOTGUN:    new_special->ammo = SPECIAL_AMMO_IN_A_SHOTGUN; break;
    case SPECIAL_AK47:       new_special->ammo = SPECIAL_AMMO_IN_A_AK47;    break;
    case SPECIAL_GRENADE:    new_special->ammo = SPECIAL_AMMO_IN_A_GRENADE; break;
    case SPECIAL_EXPLOSIVES: new_special->ammo = 1;                         break;
    default:                 new_special->ammo = 0;                         break;
    }

    dm->ObjectId = SPECIAL_info[type].prim;

    // Treasure badges: pick a random variant (Skill/Strength/Stamina/Constitution).
    if (dm->ObjectId == PRIM_OBJ_ITEM_TREASURE) {
        switch (Random() & 3) {
        case 0: dm->ObjectId = 71; break;
        case 1: dm->ObjectId = 94; break;
        case 2: dm->ObjectId = 81; break;
        case 3: dm->ObjectId = 39; break;
        }
    }

    special_thing->WorldPos.X = world_x << 8;
    special_thing->WorldPos.Y = world_y << 8;
    special_thing->WorldPos.Z = world_z << 8;

    add_thing_to_map(special_thing);

    // If spawned above terrain, start dropping.
    if (world_y > PAP_calc_map_height_at(world_x, world_z) + 0x50) {
        special_thing->SubState = SPECIAL_SUBSTATE_PROJECTILE;
        special_thing->Genus.Special->timer = 0x8000; // biased-zero velocity
    }

    return special_thing;
}

// uc_orig: free_special (fallen/Source/Special.cpp)
void free_special(Thing* special_thing)
{
    special_thing->Genus.Special->SpecialType = SPECIAL_NONE;
    free_draw_mesh(special_thing->Draw.Mesh);
    remove_thing_from_map(special_thing);
    free_thing(special_thing);
}

// uc_orig: person_has_special (fallen/Source/Special.cpp)
Thing* person_has_special(Thing* p_person, SLONG special_type)
{
    SLONG special;
    Thing* p_special;

    for (special = p_person->Genus.Person->SpecialList; special; special = p_special->Genus.Special->NextSpecial) {
        p_special = TO_THING(special);

        if (p_special->Genus.Special->SpecialType == special_type) {
            return p_special;
        }
    }

    return NULL;
}

// ---------------------------------------------------------------------------
// Grenade operations
// ---------------------------------------------------------------------------

// uc_orig: SPECIAL_throw_grenade (fallen/Source/Special.cpp)
void SPECIAL_throw_grenade(Thing* p_special)
{
    Thing* p_person = TO_THING(p_special->Genus.Special->OwnerThing);

    if (!CreateGrenadeFromPerson(p_person, p_special->Genus.Special->timer)) {
        return;
    }

    if (p_special->Genus.Special->ammo == 1) {
        special_drop(p_special, p_person);
        free_special(p_special);
        p_person->Genus.Person->SpecialUse = NULL;
    } else {
        p_special->Genus.Special->ammo -= 1;
        p_special->SubState = SPECIAL_SUBSTATE_NONE;
    }
}

// uc_orig: SPECIAL_prime_grenade (fallen/Source/Special.cpp)
void SPECIAL_prime_grenade(Thing* p_special)
{
    p_special->SubState = SPECIAL_SUBSTATE_ACTIVATED;
    p_special->Genus.Special->timer = 16 * 20 * 6; // 6-second fuse
}

/*
// SPECIAL_throw_mine is fully commented out in the original pre-release.
// In the final game, mines are map-placed only (via alloc_special from level scripts).
// uc_orig: SPECIAL_throw_mine (fallen/Source/Special.cpp)
void SPECIAL_throw_mine(Thing *p_special)
{
    ...
}
*/

// uc_orig: SPECIAL_set_explosives (fallen/Source/Special.cpp)
void SPECIAL_set_explosives(Thing* p_person)
{
    Thing* p_special;

    p_special = person_has_special(p_person, SPECIAL_EXPLOSIVES);

    ASSERT(p_special->Genus.Special->SpecialType == SPECIAL_EXPLOSIVES);

    if (p_special) {
        if (p_special->Genus.Special->ammo == 1) {
            p_special->WorldPos = p_person->WorldPos;

            special_drop(p_special, p_person);

            if (p_person->Genus.Person->SpecialUse == THING_NUMBER(p_special)) {
                p_person->Genus.Person->SpecialUse = NULL;
            }
        } else {
            p_special->Genus.Special->ammo -= 1;

            p_special = alloc_special(
                SPECIAL_EXPLOSIVES,
                SPECIAL_SUBSTATE_NONE,
                p_person->WorldPos.X >> 8,
                p_person->WorldPos.Y >> 8,
                p_person->WorldPos.Z >> 8,
                NULL);
        }

        p_special->SubState = SPECIAL_SUBSTATE_ACTIVATED;
        p_special->Genus.Special->timer = 16 * 20 * 5; // 5-second fuse
        p_special->Genus.Special->OwnerThing = THING_NUMBER(p_person);

        PANEL_new_info_message(XLAT_str(X_FUSE_SET));
    }
}

// Draws any extra visual effects for a Special Thing, based on its SpecialType.
// Called from the aeng render loop alongside the standard Thing draw pass.
// Currently handles: SPECIAL_MINE (rotating bloom) and SPECIAL_EXPLOSIVES (beam bloom on countdown).
// uc_orig: DRAWXTRA_Special (fallen/DDEngine/Source/drawxtra.cpp)
void DRAWXTRA_Special(Thing* p_thing)
{
    SLONG dx, dz, c0, flags = 0;

    switch (p_thing->Genus.Special->SpecialType) {

    case SPECIAL_MINE:
        if (p_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            c0 = (p_thing->Genus.Special->counter >> 1) & 2047;
            dx = SIN(c0) >> 8;
            dz = COS(c0) >> 8;
            BLOOM_draw(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 25, p_thing->WorldPos.Z >> 8,
                dx, 0, dz, 0x7F0000, BLOOM_BEAM);
        } else {
            c0 = 3 + (THING_NUMBER(p_thing) & 7);
            c0 = (((GAME_TURN * c0) + (THING_NUMBER(p_thing) * 9)) << 4) & 2047;
            dx = SIN(c0) >> 8;
            dz = COS(c0) >> 8;
            BLOOM_draw(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 15, p_thing->WorldPos.Z >> 8,
                dx, 0, dz, 0x7F0000, 0);
        }
        break;
    case SPECIAL_EXPLOSIVES:
        flags = BLOOM_BEAM;
        // FALL THRU
        // case SPECIAL_GRENADE:
        if (p_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            c0 = p_thing->Genus.Special->timer;
            c0 = (c0 << 3) & 2047;
            dx = SIN(c0) >> 8;
            dz = COS(c0) >> 8;
            BLOOM_draw(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 25, p_thing->WorldPos.Z >> 8,
                dx, 0, dz, 0x007F5D, flags);
        }
        break;

        //  no default -- not all specials have extra stuff.
    }
}
