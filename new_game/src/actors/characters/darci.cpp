// uc_orig: Darci.cpp (fallen/Source/Darci.cpp)
// Darci Mason — the player-controlled character.
// Physics, movement, and state initialisation for Darci.

#include "fallen/Headers/Game.h"
#include "fallen/Headers/statedef.h"
#include "actors/characters/anim_ids.h"
#include "fallen/Headers/pap.h"
#include "fallen/Headers/pcom.h"
#include "fallen/Headers/ns.h"
#include "fallen/Headers/memory.h"
#include "fallen/Headers/Sound.h"
#include "fallen/Headers/mav.h"
#include "engine/physics/collide.h"
#include "engine/physics/collide_globals.h"
#include "fallen/Headers/building.h"
#include "fallen/Headers/barrel.h"
#include "fallen/Headers/Person.h"
#include "actors/characters/darci.h"
#include "actors/characters/darci_globals.h"

// Forward declarations for functions not in any header (declared inline in originals).
extern SLONG set_person_kick_off_wall(Thing* p_person, SLONG col, SLONG set_pos);
extern void add_damage_value_thing(Thing* p_thing, SLONG value);
extern void locked_anim_change(Thing* p_person, UWORD locked_object, UWORD anim, SLONG dangle = 0);
extern void fn_person_moveing(Thing* p_person);
extern void fn_person_idle(Thing* p_person);
extern SLONG find_face_near_y(MAPCO16 x, MAPCO16 y, MAPCO16 z, SLONG ignore_faces_of_this_building, Thing* p_person, SLONG neg_dy, SLONG pos_dy, SLONG* ret_y);
extern SLONG person_slide_inside(SLONG inside_index, SLONG x1, SLONG y1, SLONG z1, SLONG* x2, SLONG* y2, SLONG* z2);

// Gravity applied to Darci's DY each tick: 4<<8 = 1024 units/tick.
// uc_orig: GRAVITY (fallen/Source/Darci.cpp)
#define GRAVITY ((4 << 8))


// uc_orig: fn_darci_init (fallen/Source/Darci.cpp)
void fn_darci_init(Thing* t_thing)
{
    t_thing->DrawType = DT_ROT_MULTI;
    t_thing->Draw.Tweened->Roll = 0;
    t_thing->Draw.Tweened->Tilt = 0;
    t_thing->Draw.Tweened->AnimTween = 0;
    t_thing->Draw.Tweened->TweenStage = 0;
    t_thing->Draw.Tweened->NextFrame = NULL;
    t_thing->Draw.Tweened->QueuedFrame = NULL;
    t_thing->Draw.Tweened->TheChunk = &game_chunk[0];
    t_thing->Draw.Tweened->FrameIndex = 0;
    t_thing->Draw.Tweened->Flags = FLAGS_DRAW_SHADOW;

    set_anim(t_thing, ANIM_STAND_READY);
    set_person_idle(t_thing);
    add_thing_to_map(t_thing);
}

// uc_orig: fn_darci_normal (fallen/Headers/Darci.h)
void fn_darci_normal(Thing* t_thing)
{
    // Intentionally empty — Darci has no AI normal mode; driven by player input.
}

// Forward declaration for already-migrated matrix function (core/fmatrix.h).
void matrix_transformZMY(Matrix31* result, Matrix33* trans, Matrix31* mat2);

// uc_orig: advance_keyframe (fallen/Source/Darci.cpp)
KeyFrame* advance_keyframe(KeyFrame* frame, SLONG count)
{
    while (count && frame->NextFrame) {
        frame = frame->NextFrame;
        count--;
    }
    return (frame);
}

// uc_orig: do_floor_collide (fallen/Source/Darci.cpp)
SLONG do_floor_collide(Thing* p_thing, SWORD pelvis, SLONG* new_y, SLONG* foot_y, SLONG max_range)
{
    SLONG x, y, z;
    SLONG floor_y;

    calc_sub_objects_position(p_thing, p_thing->Draw.Tweened->AnimTween, pelvis ? 0 : 3, &x, &y, &z);
    *foot_y = y;

    // Prevent feet from poking through walls.
    x >>= 2;
    z >>= 2;

    x += (p_thing->WorldPos.X) >> 8;
    y += (p_thing->WorldPos.Y) >> 8;
    z += (p_thing->WorldPos.Z) >> 8;
    if (p_thing->Flags & FLAGS_IN_SEWERS) {
        floor_y = NS_calc_height_at(x, z);
    } else {
        floor_y = PAP_calc_height_at_thing(p_thing, x, z);
    }
    *new_y = floor_y;

    if (y <= (floor_y + 9) && (floor_y - y) < max_range) {
        MSG_add(" feet collide  y %d floory %d\n", y, floor_y);
        return (1);
    } else
        return (0);
}

// uc_orig: predict_collision_with_floor (fallen/Source/Darci.cpp)
SLONG predict_collision_with_floor(Thing* p_thing, SWORD pelvis, SLONG* new_y, SLONG* foot_y)
{
    SLONG ret;
    GameCoord temp_pos;
    SLONG temp_velocity, temp_dy;
    SLONG c0;
    SLONG dx, dy, dz;

    temp_pos = p_thing->WorldPos;
    temp_velocity = p_thing->Velocity;
    temp_dy = p_thing->DY;
    ret = do_floor_collide(p_thing, pelvis, new_y, foot_y, 10000);
    if (ret)
        return (ret);

    {
        SLONG dx, dy, dz;
        dx = (SIN(p_thing->Draw.Tweened->Angle) * p_thing->Velocity) >> 8;
        dz = (COS(p_thing->Draw.Tweened->Angle) * p_thing->Velocity) >> 8;
        dy = p_thing->DY;

        dx = (dx * TICK_RATIO) >> (TICK_SHIFT);
        dy = (dy * TICK_RATIO) >> (TICK_SHIFT);
        dz = (dz * TICK_RATIO) >> (TICK_SHIFT);

        p_thing->Velocity += p_thing->DeltaVelocity;
        p_thing->DY -= (GRAVITY * TICK_RATIO) >> TICK_SHIFT; // gravity
        if (p_thing->DY < -30000)
            p_thing->DY = -30000;

        p_thing->WorldPos.X -= dx;
        p_thing->WorldPos.Z -= dz;
        p_thing->WorldPos.Y += dy;
    }

    ret = do_floor_collide(p_thing, pelvis, new_y, foot_y, 128);

    p_thing->Velocity = temp_velocity;
    p_thing->DY = temp_dy;
    p_thing->WorldPos = temp_pos;
    return (ret);
}

// uc_orig: predict_collision_with_face (fallen/Source/Darci.cpp)
SLONG predict_collision_with_face(Thing* p_thing, SLONG wx, SLONG wy, SLONG wz, SWORD pelvis, SLONG* new_y, SLONG* foot_y)
{
    SLONG ret;
    GameCoord temp_pos;
    SLONG temp_velocity, temp_dy;
    SLONG c0;

    SLONG dx, dy, dz, fx, fy, fz;

    SLONG ignore_building;
    if (p_thing->DY > 0)
        return (0);

    // Ignore the building we're currently inside when detecting face collisions.
    if (p_thing->Flags & FLAGS_IN_BUILDING) {
        ignore_building = INDOORS_DBUILDING;
    } else {
        ignore_building = NULL;
    }

    calc_sub_objects_position(
        p_thing,
        255,
        pelvis ? 0 : SUB_OBJECT_LEFT_FOOT,
        &fx,
        &fy,
        &fz);

    if (!pelvis) {
        // Clear XZ offset of the foot to prevent falling-through-ground bug
        // when jumping and sliding against a wall.
        fx = 0;
        fz = 0;
    }

    *foot_y = fy;

    {
        SLONG min_y, max_y;
        max_y = abs(p_thing->DY >> 9);
        min_y = -max_y;
        if (max_y < 5)
            max_y = 5;
        if (min_y > -100)
            min_y = -100;

        MSG_add(" vel>>1 %d min_y %d max-y %d \n", abs(p_thing->DY >> 9), min_y, max_y);

        ret = find_face_near_y(wx + fx, wy + fy, wz + fz, ignore_building, p_thing, min_y, max_y, new_y);
    }

    if (ret == GRAB_FLOOR)
        ret = 0;

    return (ret);
}

// uc_orig: col_is_fence (fallen/Source/Darci.cpp)
SLONG col_is_fence(SLONG col)
{
    if (dfacets[col].FacetType == STOREY_TYPE_FENCE || dfacets[col].FacetType == STOREY_TYPE_FENCE_FLAT || dfacets[col].FacetType == STOREY_TYPE_FENCE_BRICK) {
        return (1);
    } else
        return (0);
}

// uc_orig: MagicFrameCheck (fallen/Source/Darci.cpp)
// Returns UC_TRUE once when FrameIndex first reaches frameindex; resets when it drops back.
static inline BOOL MagicFrameCheck(Thing* p_person, UBYTE frameindex)
{
    if (p_person->Draw.Tweened->FrameIndex >= frameindex) {
        if (!(p_person->Genus.Person->Flags2 & FLAG2_SYNC_SOUNDFX)) {
            p_person->Genus.Person->Flags2 |= FLAG2_SYNC_SOUNDFX;
            return UC_TRUE;
        }
    } else {
        p_person->Genus.Person->Flags2 &= ~FLAG2_SYNC_SOUNDFX;
    }
    return UC_FALSE;
}

// uc_orig: set_person_in_building_through_roof (fallen/Source/Darci.cpp)
void set_person_in_building_through_roof(Thing* p_person, SLONG face)
{
    SLONG building, storey, wall, best_storey = 0;
    // Stub — body intentionally empty in the pre-release source.
}

// uc_orig: damage_person_on_land (fallen/Source/Darci.cpp)
// DY <= -30000 → instant death. DY < -20000 → damage = (-DY - 20000) / 100.
SLONG damage_person_on_land(Thing* p_thing)
{
    SLONG sound;
    SLONG damage;

    StopScreamFallSound(p_thing);

    if (p_thing->DY < -20000) {
        sound = PCOM_SOUND_DROP_BIG;
    } else if (p_thing->DY < -10000) {
        sound = PCOM_SOUND_DROP_MED;
    } else if (p_thing->DY < -5000) {
        sound = PCOM_SOUND_DROP;
    } else {
        sound = NULL;
    }

    if (sound) {
        PCOM_oscillate_tympanum(
            sound,
            p_thing,
            p_thing->WorldPos.X >> 8,
            p_thing->WorldPos.Y >> 8,
            p_thing->WorldPos.Z >> 8);
    }

    if (p_thing->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL) {
        // Only the player can hurt this person.
    } else if (p_thing->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
        // Nothing hurts this person.
    } else if (p_thing->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING) {
    } else {
        if (p_thing->DY <= -30000) {
            damage = 250; // Kill the person
            StopScreamFallSound(p_thing);
        } else {
            damage = -p_thing->DY - 20000;

            if (damage < 0) {
                return (0);
            }

            damage = damage / 100;
            PainSound(p_thing);
        }

        p_thing->Genus.Person->Health -= damage;

        if (p_thing->Genus.Person->Health <= 0) {
            set_person_dead(
                p_thing,
                NULL,
                PERSON_DEATH_TYPE_LAND,
                UC_FALSE,
                0);

            return (1);
        }
    }

    return (0);
}

// uc_orig: projectile_move_thing (fallen/Source/Darci.cpp)
SLONG projectile_move_thing(Thing* p_thing, SLONG flag)
{
    GameCoord new_position;
    DrawTween* draw_info;
    SLONG face;
    SLONG ret = 0;
    SLONG col = 0;

    SLONG dx, dy, dz;

    draw_info = p_thing->Draw.Tweened;
    new_position = p_thing->WorldPos;

    dx = (SIN(p_thing->Draw.Tweened->Angle) * p_thing->Velocity) >> 8;
    dz = (COS(p_thing->Draw.Tweened->Angle) * p_thing->Velocity) >> 8;
    dy = p_thing->DY;

    dx = (dx * TICK_RATIO) >> TICK_SHIFT;
    dy = (dy * TICK_RATIO) >> TICK_SHIFT;
    dz = (dz * TICK_RATIO) >> TICK_SHIFT;

    if (p_thing->Genus.Person->Ware) {
        SLONG px, py, pz, wy;
        // Slide along warehouse roof.
        calc_sub_objects_position(p_thing, p_thing->Draw.Tweened->AnimTween, SUB_OBJECT_HEAD, &px, &py, &pz);
        px += (dx + p_thing->WorldPos.X) >> 8;
        py += (dy + p_thing->WorldPos.Y) >> 8;
        pz += (dz + p_thing->WorldPos.Z) >> 8;

        wy = MAVHEIGHT(px >> 8, pz >> 8) << 6;
        if ((py) + 80 > wy)
            dy -= ((py) + 80 - wy) << 8;
    }

    ASSERT(p_thing->Class == CLASS_PERSON);

    p_thing->Genus.Person->SlideOdd = 0;

    if (flag & (1 | 8)) // check for walls
    {
        SLONG x1, x2, y1, y2, z1, z2;
        {
            {
                // Get pelvis world position.
                calc_sub_objects_position(p_thing, p_thing->Draw.Tweened->AnimTween, 0, &x1, &y1, &z1);

                x1 <<= 8;
                y1 <<= 8;
                z1 <<= 8;

                x1 += p_thing->WorldPos.X;
                y1 += p_thing->WorldPos.Y;
                z1 += p_thing->WorldPos.Z;

                // End of movement vector.
                x2 = x1 - dx;
                y2 = y1 + dy;
                z2 = z1 - dz;

                if (x2 < 0 || (x2 >> 16) > 127 || z2 < 0 || (z2 >> 16) > 127)
                    return (0);

                if (p_thing->Flags & FLAGS_IN_SEWERS) {
                    // Slide along sewer walls when jumping.
                    NS_slide_along(x1, y1, z1, &x2, &y2, &z2, 50);
                    col = 0;
                } else {
                    if (!(flag & 8)) {
                        // Collide with things.
                        collide_against_things(
                            p_thing,
                            50,
                            x1, y1, z1,
                            &x2, &y2, &z2);

                        if (p_thing->State == STATE_DYING) {
                            // Darci died while colliding with things.
                            return 100;
                        }

                        // Collision with objects.
                        collide_against_objects(
                            p_thing,
                            30,
                            x1, y1, z1,
                            &x2, &y2, &z2);
                    }

                    // Collision with walls.
                    if (p_thing->Genus.Person->InsideIndex) {
                        person_slide_inside(
                            p_thing->Genus.Person->InsideIndex,
                            x1, y1, z1,
                            &x2,
                            &y2,
                            &z2);
                    } else {
                        SLONG extra_wall_height = 0;

                        SATURATE(extra_wall_height, 0, 128);

                        MSG_add(" extra wall height %d y1 %d y2 %d\n", extra_wall_height, y1, y2);

                        if (flag & 8) {
                            just_started_falling_off_backwards = UC_TRUE;
                        }

                        slide_along(
                            x1, y1, z1,
                            &x2, &y2, &z2,
                            extra_wall_height,
                            50, SLIDE_ALONG_FLAG_JUMPING);

                        just_started_falling_off_backwards = UC_FALSE;

                        if (slide_into_warehouse) {
                            p_thing->Genus.Person->Flags |= FLAG_PERSON_WAREHOUSE;
                            p_thing->Genus.Person->Ware = dbuildings[slide_into_warehouse].Ware;
                        }

                        if (slide_outof_warehouse) {
                            p_thing->Genus.Person->Flags &= ~FLAG_PERSON_WAREHOUSE;
                            p_thing->Genus.Person->Ware = 0;
                        }
                    }

                    if (actual_sliding) {
                        col = last_slide_colvect;
                    } else {
                        col = 0;
                    }
                }

                dx = -(x2 - x1);
                dz = -(z2 - z1);
            }

            if (col) {
                if (col_is_fence(col) || dfacets[col].FacetType == STOREY_TYPE_NORMAL) {
                    if (dfacets[col].FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                        // Can't land on this fence — just slide.
                    } else {
                        if (set_person_land_on_fence(p_thing, col, 1)) {
                            return (2);
                        }
                    }
                }
            }

            if (col) {
                MSG_add(" HIT wall !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            }
        }
    }

    new_position.X -= dx;
    new_position.Z -= dz;
    new_position.Y += dy;

    if (new_position.X < 0 || (new_position.X >> 16) > 127 || new_position.Z < 0 || (new_position.Z >> 16) > 127)
        return (0);

    SATURATE(new_position.X, 0, (PAP_SIZE_HI << 16) - 1);
    SATURATE(new_position.Z, 0, (PAP_SIZE_HI << 16) - 1);

    p_thing->Velocity += p_thing->DeltaVelocity;
    p_thing->DY -= (GRAVITY * TICK_RATIO) >> TICK_SHIFT; // gravity

    if (p_thing->DY < -30000) {
        p_thing->DY = -30000; // terminal velocity
    }

    if (flag & (2 | 4)) // under feet
    {
        SLONG new_y, on_face, foot_y;
        if (face = predict_collision_with_face(p_thing, new_position.X >> 8, new_position.Y >> 8, new_position.Z >> 8, flag & 4, &new_y, &foot_y)) {
            {
                if (!(GAME_FLAGS & GF_INDOORS))
                    if (face > 0 && prim_faces4[face].Type == FACE_TYPE_SKYLIGHT) {
                        ASSERT(0);
                        set_person_in_building_through_roof(p_thing, face);
                        return (0);
                    }
            }
            if (flag & 4)
                return (1);
            MSG_add(" projectile move hit face %d new y %d pelvis col %d\n", face, new_y, flag & 4);

            p_thing->OnFace = face;

            p_thing->Flags &= ~FLAGS_PROJECTILE_MOVEMENT;

            if ((flag & 4) == 0) {
                new_position.Y = (new_y - foot_y + 5) << 8;

                if (p_thing->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_START || p_thing->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_FORWARDS) {
                    new_position.Y = PAP_calc_map_height_at(new_position.X >> 8, new_position.Z >> 8);
                    new_position.Y -= 0x50;
                    new_position.Y <<= 8;
                }

                move_thing_on_map(p_thing, &new_position);
            }

            if (damage_person_on_land(p_thing))
                return (100);

            return (1);
        } else if (predict_collision_with_floor(p_thing, flag & 4, &new_y, &foot_y)) {
            if (flag & 4)
                return (1);
            MSG_add(" predict collision with FLOOR");

            ret = 1;

            if (p_thing->Flags & FLAGS_IN_SEWERS) {
                p_thing->WorldPos.Y = NS_calc_height_at(new_position.X >> 8, new_position.Z >> 8) << 8;
            } else {
                if ((flag & 4) == 0) {
                    new_position.Y = (new_y - foot_y + 5) << 8;

                    if (p_thing->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_START || p_thing->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_FORWARDS) {
                        new_position.Y = PAP_calc_height_at_thing(p_thing, new_position.X >> 8, new_position.Z >> 8);
                        new_position.Y -= 0x50;
                        new_position.Y <<= 8;
                    }

                } else {
                    new_position.Y = PAP_calc_height_at_thing(p_thing, new_position.X >> 8, new_position.Z >> 8) << 8;
                    new_position.X = p_thing->WorldPos.X;
                    new_position.Z = p_thing->WorldPos.Z;
                }

                move_thing_on_map(p_thing, &new_position);
            }
            p_thing->OnFace = 0;
            p_thing->Flags &= ~FLAGS_PROJECTILE_MOVEMENT;
            if (damage_person_on_land(p_thing))
                return (100);
            return (ret);
        }
    }

    // Actually move the person.
    move_thing_on_map(p_thing, &new_position);

    // Hit any nearby barrels.
    BARREL_hit_with_sphere(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Y >> 8,
        p_thing->WorldPos.Z >> 8,
        0x70);

    // Check whether this person is falling to their death.
    SLONG death_check;

    if (p_thing->Genus.Person->PlayerID) {
        death_check = -12000;
    } else {
        death_check = -6000;
    }

    if (GAME_FLAGS & GF_NO_FLOOR) {
        if (p_thing->WorldPos.Y < 0x0 && p_thing->DY < -6000) {
            p_thing->Velocity = 0;
        }
    }

    if (p_thing->DY < death_check) {
        if ((GAME_TURN & 0x3) == THING_NUMBER(p_thing)) {
            if (p_thing->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_START || p_thing->Draw.Tweened->CurrentAnim == ANIM_PLUNGE_FORWARDS) {
                if (MagicFrameCheck(p_thing, 2)) {
                    ScreamFallSound(p_thing);
                }

                p_thing->Velocity = 0;
            } else {
                SLONG i;

                SLONG mx;
                SLONG mz;

                SLONG height;

                // Is this person going to die from the fall?
                for (i = 1; i < 4; i++) {
                    mx = (p_thing->WorldPos.X >> 8) - (dx * i >> 5);
                    mz = (p_thing->WorldPos.Z >> 8) - (dz * i >> 5);

                    if (WITHIN(mx, 0, (PAP_SIZE_HI << 8) - 1) && WITHIN(mz, 0, (PAP_SIZE_HI << 8) - 1)) {
                        height = PAP_calc_map_height_at(mx, mz);

                        if (height >= (p_thing->WorldPos.Y >> 8) - 0x600) {
                            // Safe landing ahead — no death.
                            goto no_death;
                        }
                    }
                }

                // Person is going to die — start fall animation.
                locked_anim_change(p_thing, SUB_OBJECT_PELVIS, ANIM_PLUNGE_START);

            no_death:;
            }
        }
    }

    return (ret);
}

// uc_orig: change_velocity_to (fallen/Source/Darci.cpp)
void change_velocity_to(Thing* p_thing, SWORD velocity)
{
    SLONG dv;

    velocity = (velocity * 3) >> 2;

    dv = velocity - p_thing->Velocity;
    if (dv < 0) {
        if (dv > -2)
            p_thing->Velocity = velocity;
        else
            p_thing->Velocity += (dv >> 1);

    } else if (dv > 0) {
        if (dv < 2)
            p_thing->Velocity = velocity;
        else
            p_thing->Velocity += (dv >> 1);
    }
}

// uc_orig: change_velocity_to_slow (fallen/Source/Darci.cpp)
void change_velocity_to_slow(Thing* p_thing, SWORD velocity)
{
    SLONG dv;

    velocity = (velocity * 3) >> 2;

    dv = velocity - p_thing->Velocity;
    if (dv < 0) {
        if (dv > -2)
            p_thing->Velocity = velocity;
        else
            p_thing->Velocity -= 1;

    } else if (dv > 0) {
        if (dv < 2)
            p_thing->Velocity = velocity;
        else
            p_thing->Velocity += 1;
    }
}

// uc_orig: trickle_velocity_to (fallen/Source/Darci.cpp)
void trickle_velocity_to(Thing* p_thing, SWORD velocity)
{
    SLONG dv;

    velocity = (velocity * 3) >> 2;

    dv = velocity - p_thing->Velocity;
    if (dv < 0) {
        p_thing->Velocity--;

    } else if (dv > 0) {
        p_thing->Velocity++;
    }
}

// uc_orig: set_thing_velocity (fallen/Source/Darci.cpp)
void set_thing_velocity(Thing* t_thing, SLONG vel)
{
    vel = (vel * 3) >> 2;
    t_thing->Velocity = vel;
}

// uc_orig: show_walkable (fallen/Source/Darci.cpp)
// Debug stub — was intended to visualise the walkable map grid; returns immediately.
void show_walkable(SLONG mx, SLONG mz)
{
    return;
}
