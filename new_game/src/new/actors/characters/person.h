#ifndef ACTORS_CHARACTERS_PERSON_H
#define ACTORS_CHARACTERS_PERSON_H

// Person subsystem: public API for the person (character) system.
// The Person struct, type constants, flags, and macros come from the legacy header
// (fallen/Headers/Person.h) which is still being migrated. This header adds the
// additional function declarations from Person.cpp that were not declared there,
// and re-exports everything via the old header include.

#include "fallen/Headers/Game.h"            // Temporary: pulls in Person struct/macros + all game types
#include "actors/characters/person_globals.h"

// Internal helpers — defined in person.cpp, declared here because
// old/Person.cpp uses them after the MIGRATED block.

// Plays a slide sound if the person is sliding on a surface (force=1 bypasses cooldown).
// uc_orig: SlideSoundCheck (fallen/Source/Person.cpp)
void SlideSoundCheck(Thing* p_person, BOOL force = 0);

// Sets a person into the dying state with the given substate.
// uc_orig: set_person_dying (fallen/Source/Person.cpp)
void set_person_dying(Thing* p_person, UBYTE substate);

// Finds a position near person that is not occupied by another person.
// uc_orig: find_nice_place_near_person (fallen/Source/Person.cpp)
void find_nice_place_near_person(Thing* p_person, SLONG* nice_x, SLONG* nice_y, SLONG* nice_z);

// Sets animation using game_chunk[type] rather than global_anim_array.
// uc_orig: set_anim_of_type (fallen/Source/Person.cpp)
void set_anim_of_type(Thing* p_person, SLONG anim, SLONG type);

// Sets a locked animation (sub_object = which limb/part to apply to).
// uc_orig: set_locked_anim (fallen/Source/Person.cpp)
void set_locked_anim(Thing* p_person, SLONG anim, SLONG sub_object);

// Variant with explicit angular offset (dangle).
// uc_orig: set_locked_anim_angle (fallen/Source/Person.cpp)
void set_locked_anim_angle(Thing* p_person, SLONG anim, SLONG sub_object, SLONG dangle);

// Variant using game_chunk[AnimType] for animation lookup.
// uc_orig: set_locked_anim_of_type (fallen/Source/Person.cpp)
void set_locked_anim_of_type(Thing* p_person, SLONG anim, SLONG sub_object);

// Returns true if the current animation frame index >= frameindex (for syncing effects).
// uc_orig: MagicFrameCheck (fallen/Source/Person.cpp)
BOOL MagicFrameCheck(Thing* p_person, UBYTE frameindex);

// Releases any item a person was searching (e.g. looting a body).
// uc_orig: release_searched_item (fallen/Source/Person.cpp)
void release_searched_item(Thing* p_person);

// Returns the bottom Y of a fence facet at (x,z) with collision id col.
// uc_orig: get_fence_bottom (fallen/Source/Person.cpp)
SLONG get_fence_bottom(SLONG x, SLONG z, SLONG col);

// Returns the top Y of a fence facet at (x,z) with collision id col.
// uc_orig: get_fence_top (fallen/Source/Person.cpp)
SLONG get_fence_top(SLONG x, SLONG z, SLONG col);

// Plays a splash effect on the given limb when a person hits water (limb=-1 for centre).
// uc_orig: person_splash (fallen/Source/Person.cpp)
void person_splash(Thing* p_person, SLONG limb);

// Moves a person along a death-slide cable toward the end.
// uc_orig: person_death_slide (fallen/Source/Person.cpp)
void person_death_slide(Thing* p_person);

// Statistics helpers (for end-of-mission stats).
// uc_orig: set_stats (fallen/Source/Person.cpp)
void set_stats(void);
// uc_orig: init_stats (fallen/Source/Person.cpp)
void init_stats(void);

// Returns true if the person is an MIB character.
// uc_orig: PersonIsMIB (fallen/Source/Person.cpp)
BOOL PersonIsMIB(Thing* p_person);

// Helper: gets/sets PersonID on the DrawTween based on currently equipped item.
// uc_orig: set_persons_personid (fallen/Source/Person.cpp)
void set_persons_personid(Thing* p_person);

// Helper: sets player_visited[x][z] bit to mark the map tile as visited.
// uc_orig: set_player_visited (fallen/Source/Person.cpp)
void set_player_visited(UBYTE x, UBYTE z);

// Distance from person_a's pelvis to person_b's pelvis.
// uc_orig: dist_to_target_pelvis (fallen/Source/Person.cpp)
SLONG dist_to_target_pelvis(Thing* p_person_a, Thing* p_person_b);

// Finds the nearest available arrestee for the given officer.
// uc_orig: find_arrestee (fallen/Source/Person.cpp)
UWORD find_arrestee(Thing* p_person);

// Sets the fight idle stance animation.
// uc_orig: set_person_fight_idle (fallen/Source/Person.cpp)
void set_person_fight_idle(Thing* p_person);

// Returns the scalar distance from person_a to person_b.
// uc_orig: dist_to_target (fallen/Source/Person.cpp)
SLONG dist_to_target(Thing* p_person_a, Thing* p_person_b);

// Returns non-zero if the person is currently crouching.
// uc_orig: is_person_crouching (fallen/Source/Person.cpp)
SLONG is_person_crouching(Thing* p_person);

// Returns non-zero if the person is dead (STATE_DEAD or health <= 0).
// uc_orig: is_person_dead (fallen/Source/Person.cpp)
SLONG is_person_dead(Thing* p_person);

// Returns non-zero if the person is knocked out (lying on ground, alive).
// uc_orig: is_person_ko (fallen/Source/Person.cpp)
SLONG is_person_ko(Thing* p_person);
// uc_orig: is_person_ko_and_lay_down (fallen/Source/Person.cpp)
SLONG is_person_ko_and_lay_down(Thing* p_person);

// Returns non-zero if the person is guilty (wanted by police).
// uc_orig: is_person_guilty (fallen/Source/Person.cpp)
SLONG is_person_guilty(Thing* p_person);

// Returns non-zero if person is on the floor (not a building face).
// uc_orig: person_on_floor (fallen/Source/Person.cpp)
SLONG person_on_floor(Thing* p_person);
// uc_orig: really_on_floor (fallen/Source/Person.cpp)
SLONG really_on_floor(Thing* p_person);

// Returns the footstep sound sample index for the surface the person is on.
// uc_orig: footstep_wave (fallen/Source/Person.cpp)
SLONG footstep_wave(Thing* p_person);

// Returns true if there is room behind the person to die in the given direction.
// hit_from_behind=1 means the attacker was behind them.
// uc_orig: is_there_room_behind_person (fallen/Source/Person.cpp)
SLONG is_there_room_behind_person(Thing* p_person, SLONG hit_from_behind);

// Returns the fractional position along a collision facet at (x,z).
// uc_orig: get_along_facet (fallen/Source/Person.cpp)
SLONG get_along_facet(SLONG x, SLONG z, SLONG colvect);

// Sets person into a death/knockdown state appropriate for death_type and aggressor.
// uc_orig: set_person_dead (fallen/Source/Person.cpp)
void set_person_dead(Thing* p_thing, Thing* p_aggressor, SLONG death_type, SLONG behind, SLONG height);

// Deals hitpoints damage and knocks the person to the ground.
// uc_orig: knock_person_down (fallen/Source/Person.cpp)
void knock_person_down(Thing* p_person, SLONG hitpoints, SLONG origin_x, SLONG origin_z, Thing* p_aggressor);

// Moves person forward by dist units along their facing angle (for punch-push effects).
// uc_orig: person_bodge_forward (fallen/Source/Person.cpp)
void person_bodge_forward(Thing* p_person, SLONG dist);

// Returns true if there is a clear line-of-sight between the two persons' heads.
// uc_orig: los_between_heads (fallen/Source/Person.cpp)
SLONG los_between_heads(Thing* person_1, Thing* person_2);

// Plays a tin-pan sound and turns nearby idle NPCs to look at (x,y,z).
// uc_orig: oscilate_tinpanum (fallen/Source/Person.cpp)
void oscilate_tinpanum(SLONG x, SLONG y, SLONG z, Thing* p_thing, SLONG vol);

// Returns true if person_a can see person_b.
// range=0 uses default (8<<8); range<0 ignores view conditions; range>0 clips view.
// no_los=1 skips geometric LOS check (FOV only).
// NOTE: Also declared in fallen/Headers/Person.h (accessible via Game.h) with default args.
// uc_orig: can_a_see_b (fallen/Source/Person.cpp)
// (declaration omitted here — provided by fallen/Headers/Person.h via Game.h)

// Returns true if person can see the world-space point (x,y,z).
// NOTE: Also declared in fallen/Headers/Person.h (accessible via Game.h).
// uc_orig: can_i_see_place (fallen/Source/Person.cpp)
// (declaration omitted here — provided by fallen/Headers/Person.h via Game.h)

// Enters the sliding-tackle state toward p_target if not already sliding.
// uc_orig: set_person_sliding_tackle (fallen/Source/Person.cpp)
void set_person_sliding_tackle(Thing* p_person, Thing* p_target);

// Attempts a fence vault over facet; returns true if vault was initiated.
// uc_orig: set_person_vault (fallen/Source/Person.cpp)
SLONG set_person_vault(Thing* p_person, SLONG facet);

// Attempts a half-block step up over facet; returns true if climb was initiated.
// uc_orig: set_person_climb_half (fallen/Source/Person.cpp)
SLONG set_person_climb_half(Thing* p_person, SLONG facet);

// Returns true if person can see player 0.
// uc_orig: can_i_see_player (fallen/Source/Person.cpp)
SLONG can_i_see_player(Thing* p_person);

// Checks if person can see any enemies and starts pursuit if so.
// uc_orig: do_look_for_enemies (fallen/Source/Person.cpp)
void do_look_for_enemies(Thing* p_person);

// Per-frame player-specific updates: fight mode, boredom timer, camera roll.
// uc_orig: general_process_player (fallen/Source/Person.cpp)
void general_process_player(Thing* p_person);

// Cycles person's current target through nearby attackers (dir=1 forward, dir=-1 back).
// uc_orig: person_pick_best_target (fallen/Source/Person.cpp)
void person_pick_best_target(Thing* p_person, SLONG dir);

// Per-frame person processing: moving platforms, stamina, burning, bleeding, hooks.
// uc_orig: general_process_person (fallen/Source/Person.cpp)
void general_process_person(Thing* p_person);

// Sweeps person's feet, potentially knocking them down or dead.
// uc_orig: sweep_feet (fallen/Source/Person.cpp)
void sweep_feet(Thing* p_person, Thing* p_aggressor, SLONG death_type);

// Returns non-zero if the person is on a slope steep enough to cause sliding.
// uc_orig: check_on_slippy_slope (fallen/Source/Person.cpp)
SLONG check_on_slippy_slope(Thing* p_person);

// Temporarily moves person dist units forward and checks if slope ahead would cause sliding.
// uc_orig: slope_ahead (fallen/Source/Person.cpp)
SLONG slope_ahead(Thing* p_person, SLONG dist);

// Moves person by (dx, dz) with height-tracking, tick scaling, and collision.
// uc_orig: person_normal_move_dxdz (fallen/Source/Person.cpp)
void person_normal_move_dxdz(Thing* p_person, SLONG dx, SLONG dz);

// Moves person forward along their facing angle by their Velocity.
// uc_orig: person_normal_move (fallen/Source/Person.cpp)
void person_normal_move(Thing* p_person);

// Like person_normal_move, but skips if target position would be outside map bounds.
// uc_orig: person_normal_move_check (fallen/Source/Person.cpp)
void person_normal_move_check(Thing* p_person);

// Advances one animation keyframe, returns 1=ended, 2=queued anim loaded.
// uc_orig: advance_keyframe (fallen/Source/Person.cpp)
SLONG advance_keyframe(DrawTween* draw_info);

// Retreats one animation keyframe for backwards playback. Returns 1 when start reached.
// uc_orig: retreat_keyframe (fallen/Source/Person.cpp)
SLONG retreat_keyframe(DrawTween* draw_info);

// Adjusts world position to keep a locked limb stationary between two tween values.
// uc_orig: move_locked_tween (fallen/Source/Person.cpp)
void move_locked_tween(Thing* p_person, DrawTween* dt, SLONG t1, SLONG t2);

// Animates person forward at given speed. Handles fight frames (apply_violence), barrel hits.
// uc_orig: person_normal_animate_speed (fallen/Source/Person.cpp)
SLONG person_normal_animate_speed(Thing* p_person, SLONG speed);

// Animates person forward at normal speed.
// uc_orig: person_normal_animate (fallen/Source/Person.cpp)
SLONG person_normal_animate(Thing* p_person);

// Animates person backward (retreating keyframes).
// uc_orig: person_backwards_animate (fallen/Source/Person.cpp)
SLONG person_backwards_animate(Thing* p_person);

// Camera helpers — bodies removed before shipping in the original.
// uc_orig: camera_shoot (fallen/Source/Person.cpp)
void camera_shoot(void);
// uc_orig: camera_fight (fallen/Source/Person.cpp)
void camera_fight(void);
// uc_orig: camera_normal (fallen/Source/Person.cpp)
void camera_normal(void);

// --- chunk 4: set_person_aim..drop_all_items ---

// Sets person into aiming stance using their current weapon.
// uc_orig: set_person_aim (fallen/Source/Person.cpp)
void set_person_aim(Thing* p_person, SLONG locked = 0);

// Checks if the vehicle is driven or occupied by a MIB agent.
// uc_orig: VehicleBelongsToMIB (fallen/Source/Person.cpp)
UBYTE VehicleBelongsToMIB(Thing* p_target);

// Computes damage dealt to p_target when p_person shoots.
// Returns 0 on miss. Sets *gun_type to one of HIT_TYPE_GUN_SHOT_*.
// uc_orig: get_shoot_damage (fallen/Source/Person.cpp)
SLONG get_shoot_damage(Thing* p_person, Thing* p_target, SLONG* gun_type);

// Determines ammo, sound, animation and cooldown time for a shot.
// Returns ammo count, NOT_A_GUN_YOU_SHOOT, or HAD_TO_CHANGE_CLIP.
// uc_orig: shoot_get_ammo_sound_anim_time (fallen/Source/Person.cpp)
SLONG shoot_get_ammo_sound_anim_time(Thing* p_person, SLONG* sound, SLONG* anim, SLONG* time);

// Fires the gun: dynamic light, brass eject, applies hit or ricochet.
// uc_orig: actually_fire_gun (fallen/Source/Person.cpp)
void actually_fire_gun(Thing* p_person);

// Fires the gun if person is running (respects cooldown timer).
// uc_orig: set_person_running_shoot (fallen/Source/Person.cpp)
void set_person_running_shoot(Thing* p_person);

// Returns the best special weapon type with ammo the person is carrying.
// Returns SPECIAL_NONE if no viable weapon found.
// uc_orig: get_persons_best_weapon_with_ammo (fallen/Source/Person.cpp)
SLONG get_persons_best_weapon_with_ammo(Thing* p_person);

// Returns true if NPC should hold fire due to an active cutscene.
// uc_orig: dont_hurt_target_during_cutscene (fallen/Source/Person.cpp)
SLONG dont_hurt_target_during_cutscene(Thing* p_person, Thing* p_target);

// Main shoot action: handles ammo, targeting, fire, weapon-switch on empty.
// uc_orig: set_person_shoot (fallen/Source/Person.cpp)
void set_person_shoot(Thing* p_person, UWORD shoot_target);

// Starts the grappling hook windup animation.
// uc_orig: set_person_grapple_windup (fallen/Source/Person.cpp)
void set_person_grapple_windup(Thing* p_person);

// Starts the grappling hook release animation.
// uc_orig: set_person_grappling_hook_release (fallen/Source/Person.cpp)
void set_person_grappling_hook_release(Thing* p_person);

// Returns SPECIAL_TYPE if person has a gun-type weapon drawn, else FALSE.
// uc_orig: person_has_gun_out (fallen/Source/Person.cpp)
SLONG person_has_gun_out(Thing* p_person);

// Drops the currently held gun or special weapon to the ground.
// uc_orig: drop_current_gun (fallen/Source/Person.cpp)
void drop_current_gun(Thing* p_person, SLONG change_anim);

// Drops all items (gun, specials, bounty) the person is carrying.
// uc_orig: drop_all_items (fallen/Source/Person.cpp)
void drop_all_items(Thing* p_person, UBYTE is_being_searched);

#endif // ACTORS_CHARACTERS_PERSON_H
