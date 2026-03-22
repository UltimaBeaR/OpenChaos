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

// --- chunk 5: set_person_idle_uncroutch..set_person_carry ---

// Transitions from crouching-idle back to standing idle with uncrouch animation.
// uc_orig: set_person_idle_uncroutch (fallen/Source/Person.cpp)
void set_person_idle_uncroutch(Thing* p_person);

// Starts the turn-to-face-wall animation (entering hug-wall mode from front).
// uc_orig: set_person_turn_to_hug_wall (fallen/Source/Person.cpp)
void set_person_turn_to_hug_wall(Thing* p_person);

// Starts sidling along wall in the given direction while hugging wall (1=left, 0=right).
// uc_orig: set_person_hug_wall_dir (fallen/Source/Person.cpp)
void set_person_hug_wall_dir(Thing* p_person, SLONG dir);

// Starts the peek-around-wall animation in the given direction while hugging wall.
// uc_orig: set_person_hug_wall_look (fallen/Source/Person.cpp)
void set_person_hug_wall_look(Thing* p_person, SLONG dir);

// Transitions person to standing idle based on mode, weapon, and combat state.
// uc_orig: set_person_idle (fallen/Source/Person.cpp)
void set_person_idle(Thing* p_person);

// Sets person into a locked idle ready stance (respects gun/weapon state).
// uc_orig: set_person_locked_idle_ready (fallen/Source/Person.cpp)
void set_person_locked_idle_ready(Thing* p_person);

// Checks if person's back is against a wall and sets sidle sub-state. Returns 1 if active.
// uc_orig: set_person_sidle (fallen/Source/Person.cpp)
SLONG set_person_sidle(struct Thing* p_person);

// Starts a flip animation (0=left, 1=right).
// uc_orig: set_person_flip (fallen/Source/Person.cpp)
void set_person_flip(Thing* p_person, SLONG dir);

// Sets person into running/walking/sneaking/sprint based on current Mode.
// uc_orig: set_person_running (fallen/Source/Person.cpp)
void set_person_running(Thing* p_person);

// Sets person running starting from a specific animation frame.
// uc_orig: set_person_running_frame (fallen/Source/Person.cpp)
void set_person_running_frame(Thing* p_person, SLONG frame);

// Cycles to the next carried special item and plays the draw sound.
// uc_orig: set_person_draw_special (fallen/Source/Person.cpp)
void set_person_draw_special(Thing* p_person);

// Starts gun-draw animation and alerts nearby NPCs.
// uc_orig: set_person_draw_gun (fallen/Source/Person.cpp)
void set_person_draw_gun(Thing* p_person);

// Holsters the gun and transitions to idle.
// uc_orig: set_person_gun_away (fallen/Source/Person.cpp)
void set_person_gun_away(Thing* p_person);

// Positions person at the correct entry point for the given vehicle door (0=driver, 1=passenger).
// uc_orig: position_person_for_vehicle (fallen/Source/Person.cpp)
void position_person_for_vehicle(Thing* p_person, Thing* p_vehicle, SLONG door);

// Starts person entering a vehicle (key check, occupancy check, plays enter animation).
// uc_orig: set_person_enter_vehicle (fallen/Source/Person.cpp)
void set_person_enter_vehicle(Thing* p_person, Thing* p_vehicle, SLONG door);

// Appends person to the head of the vehicle's passenger linked list.
// uc_orig: add_person_to_passenger_list (fallen/Source/Person.cpp)
void add_person_to_passenger_list(Thing* p_person, Thing* p_vehicle);

// Removes person from the vehicle's passenger linked list.
// uc_orig: remove_person_from_passenger_list (fallen/Source/Person.cpp)
void remove_person_from_passenger_list(Thing* p_person, Thing* p_vehicle);

// Instantly places person inside vehicle as a passenger (no entry animation).
// uc_orig: set_person_passenger_in_vehicle (fallen/Source/Person.cpp)
void set_person_passenger_in_vehicle(Thing* p_person, Thing* p_vehicle, SLONG door);

// Exits person from vehicle: tries both doors, repositions, removes from driver/passenger lists.
// uc_orig: set_person_exit_vehicle (fallen/Source/Person.cpp)
void set_person_exit_vehicle(Thing* p_person);

// Selects the correct walk animation for person type and equipped weapon.
// uc_orig: set_anim_walking (fallen/Source/Person.cpp)
void set_anim_walking(Thing* p_person);

// Selects the correct run animation for person type.
// uc_orig: set_anim_running (fallen/Source/Person.cpp)
void set_anim_running(Thing* p_person);

// Selects the correct idle animation based on person type, mode, and weapon.
// uc_orig: set_anim_idle (fallen/Source/Person.cpp)
void set_anim_idle(Thing* p_person);

// Starts walking at velocity 5 with the appropriate walk animation.
// uc_orig: set_person_walking (fallen/Source/Person.cpp)
void set_person_walking(Thing* p_person);

// Starts walking backwards at velocity -5.
// uc_orig: set_person_walk_backwards (fallen/Source/Person.cpp)
void set_person_walk_backwards(Thing* p_person);

// Starts sneaking (slow walk with sneak animation).
// uc_orig: set_person_sneaking (fallen/Source/Person.cpp)
void set_person_sneaking(Thing* p_person);

// Starts a short backward hop (used in fight responses).
// uc_orig: set_person_hop_back (fallen/Source/Person.cpp)
void set_person_hop_back(Thing* p_person);

// Returns the best fight-idle animation for the person given their current weapon.
// uc_orig: find_idle_fight_stance (fallen/Source/Person.cpp)
SLONG find_idle_fight_stance(Thing* p_person);

// set_person_fight_idle already declared above (chunk 4 section, line ~96)

// Starts a fight step in the given cardinal direction (0=N, 1=E, 2=S, 3=W).
// uc_orig: set_person_fight_step (fallen/Source/Person.cpp)
void set_person_fight_step(Thing* p_person, SLONG dir);

// Starts a fight step forward if not already in that animation.
// uc_orig: set_person_fight_step_forward (fallen/Source/Person.cpp)
void set_person_fight_step_forward(Thing* p_person);

// Plays the block animation for the current weapon.
// uc_orig: set_person_block (fallen/Source/Person.cpp)
void set_person_block(Thing* p_person);

// Transitions to crouching idle state.
// uc_orig: set_person_idle_croutch (fallen/Source/Person.cpp)
void set_person_idle_croutch(Thing* p_person);

// Forcibly releases the person being carried when carrier is hit.
// uc_orig: emergency_uncarry (fallen/Source/Person.cpp)
void emergency_uncarry(Thing* p_person);

// Updates carried person's world position each frame during carry-running.
// uc_orig: carry_running (fallen/Source/Person.cpp)
void carry_running(Thing* p_person);

// Begins carrying another person: sets carry state, pickup animations, positions victim.
// uc_orig: set_person_carry (fallen/Source/Person.cpp)
void set_person_carry(Thing* p_person, SLONG s_index);

// Stub for setting a vehicle animation (always asserts — disabled in original).
// uc_orig: set_vehicle_anim (fallen/Source/Person.cpp)
void set_vehicle_anim(Thing* p_vehicle, SLONG anim);

// --- chunk 6: set_person_uncarry..set_person_pos_for_fence_vault ---

// Starts the put-down carry animation for both carrier and victim.
// uc_orig: set_person_uncarry (fallen/Source/Person.cpp)
void set_person_uncarry(Thing* p_person);

// Switches to standing-carry state (stand-carry anims for both persons).
// uc_orig: set_person_stand_carry (fallen/Source/Person.cpp)
void set_person_stand_carry(Thing* p_person);

// State handler for the carry state.
// uc_orig: fn_person_carry (fallen/Source/Person.cpp)
void fn_person_carry(Thing* p_person);

// Initiates an arrest sequence on s_index target.
// uc_orig: set_person_arrest (fallen/Source/Person.cpp)
void set_person_arrest(Thing* p_person, SLONG s_index);

// Transitions person to crouching idle; if Darci with a nearby arrestee, arrests them.
// uc_orig: set_person_croutch (fallen/Source/Person.cpp)
void set_person_croutch(Thing* p_person);

// Transitions person into crawling state.
// uc_orig: set_person_crawling (fallen/Source/Person.cpp)
void set_person_crawling(Thing* p_person);

// Initiates a leg-sweep kick attack.
// uc_orig: set_person_leg_sweep (fallen/Source/Person.cpp)
SLONG set_person_leg_sweep(Thing* p_person);

// Initiates a punch attack (handles knife, bat, shotgun/AK variants).
// uc_orig: set_person_punch (fallen/Source/Person.cpp)
SLONG set_person_punch(Thing* p_person);

// Initiates a directional kick (dir: 0=N, 1=E, 2=S, 3=W).
// uc_orig: set_person_kick_dir (fallen/Source/Person.cpp)
SLONG set_person_kick_dir(Thing* p_person, SLONG dir);

// Initiates a fight animation by anim type, entering fight state.
// uc_orig: set_person_fight_anim (fallen/Source/Person.cpp)
void set_person_fight_anim(Thing* p_person, SLONG anim);

// Initiates a standard front kick.
// uc_orig: set_person_kick (fallen/Source/Person.cpp)
SLONG set_person_kick(Thing* p_person);

// Initiates a near kick (ANIM_KICK_NAD if close, else ANIM_KICK_NEAR).
// uc_orig: set_person_kick_near (fallen/Source/Person.cpp)
SLONG set_person_kick_near(Thing* p_person, SLONG dist);

// Initiates a stomp attack on a downed opponent.
// uc_orig: set_person_stomp (fallen/Source/Person.cpp)
SLONG set_person_stomp(Thing* p_person);

// Positions person at the base of a ladder facet at the correct angle.
// uc_orig: set_person_position_for_ladder (fallen/Source/Person.cpp)
void set_person_position_for_ladder(Thing* p_person, UWORD facet);

// Starts a person climbing a ladder.
// uc_orig: set_person_climb_ladder (fallen/Source/Person.cpp)
void set_person_climb_ladder(Thing* p_person, UWORD storey);

// Switches to the looping on-ladder animation.
// uc_orig: set_person_on_ladder (fallen/Source/Person.cpp)
void set_person_on_ladder(Thing* p_person);

// Starts the climb-up-fence animation.
// uc_orig: set_person_on_fence (fallen/Source/Person.cpp)
void set_person_on_fence(Thing* p_person);

// Initiates a standing jump. Checks for climbable fence first.
// uc_orig: set_person_standing_jump (fallen/Source/Person.cpp)
void set_person_standing_jump(Thing* p_person);

// Initiates a forward standing jump (delegates to running jump).
// uc_orig: set_person_standing_jump_forwards (fallen/Source/Person.cpp)
void set_person_standing_jump_forwards(Thing* p_person);

// Initiates a backwards standing jump (back-flip).
// uc_orig: set_person_standing_jump_backwards (fallen/Source/Person.cpp)
void set_person_standing_jump_backwards(Thing* p_person);

// Initiates a running jump with STATE_JUMPING + run-jump-left anim.
// uc_orig: set_person_running_jump (fallen/Source/Person.cpp)
void set_person_running_jump(Thing* p_person);

// Stub for left/right running jump variant (body removed before shipping in original).
// uc_orig: set_person_running_jump_lr (fallen/Source/Person.cpp)
void set_person_running_jump_lr(Thing* p_person, SLONG dir);

// Checks traverse feasibility in given direction and repositions person on ledge.
// Returns 1 if traverse is possible, 0 if blocked.
// uc_orig: traverse_pos (fallen/Source/Person.cpp)
SLONG traverse_pos(Thing* p_person, SLONG right);

// Starts a sideways ledge traverse if possible (right=1 / left=0).
// uc_orig: set_person_traverse (fallen/Source/Person.cpp)
void set_person_traverse(Thing* p_person, SLONG right);

// Initiates a pull-up from dangling, checking destination for nogo.
// uc_orig: set_person_pulling_up (fallen/Source/Person.cpp)
void set_person_pulling_up(Thing* p_person);

// Transitions person to a falling/dropping state.
// flag: PERSON_DROP_DOWN_KEEP_VEL, PERSON_DROP_DOWN_KEEP_DY,
//       PERSON_DROP_DOWN_QUEUED, PERSON_DROP_DOWN_OFF_FACE.
// uc_orig: set_person_drop_down (fallen/Source/Person.cpp)
void set_person_drop_down(Thing* p_person, SLONG flag);

// Locked-anim variant of set_person_drop_down with explicit DY.
// uc_orig: set_person_locked_drop_down (fallen/Source/Person.cpp)
void set_person_locked_drop_down(Thing* p_person, SLONG vely);

// Returns true if the col wall is suitable for a bump-and-turn maneuver.
// uc_orig: is_wall_good_for_bump_and_turn (fallen/Source/Person.cpp)
SLONG is_wall_good_for_bump_and_turn(Thing* p_person, SLONG col);

// Returns true if person is facing within vault_da degrees of wall col's normal.
// Sets *wall_angle. Default vault_da=128 (~22.5 degrees).
// uc_orig: am_i_facing_wall (fallen/Source/Person.cpp)
SLONG am_i_facing_wall(Thing* p_person, SLONG col, SLONG* wall_angle, SLONG vault_da = 128);

// Returns true if person is positioned along the middle segment of facet col.
// uc_orig: along_middle_of_facet (fallen/Source/Person.cpp)
SLONG along_middle_of_facet(Thing* p_person, SLONG col);

// Positions person to vault a fence facet; returns 1 on success, 0 if not possible.
// uc_orig: set_person_pos_for_fence_vault (fallen/Source/Person.cpp)
SLONG set_person_pos_for_fence_vault(Thing* p_person, SLONG col);

// --- chunk 7 declarations ---

// Positions person relative to a fence facet at standoff req_dist.
// Returns 0 on success, 1 if not possible.
// uc_orig: set_person_pos_for_fence (fallen/Source/Person.cpp)
SLONG set_person_pos_for_fence(Thing* p_person, SLONG col, SLONG set_pos, SLONG req_dist);

// Positions person against a half-step facet; returns 1 on success, 0 otherwise.
// uc_orig: set_person_pos_for_half_step (fallen/Source/Person.cpp)
SLONG set_person_pos_for_half_step(Thing* p_person, SLONG col);

// Returns true if facet is a short (height==2, blockheight==16) vaultable fence.
// uc_orig: is_facet_vaultable (fallen/Source/Person.cpp)
SLONG is_facet_vaultable(SLONG facet);

// Returns true if facet is a short normal-type step (height*blockheight*4 == 128).
// uc_orig: is_facet_half_step (fallen/Source/Person.cpp)
SLONG is_facet_half_step(SLONG facet);

// Attempts to land person on a fence facet (vault or grab).
// uc_orig: set_person_land_on_fence (fallen/Source/Person.cpp)
SLONG set_person_land_on_fence(Thing* p_person, SLONG col, SLONG set_pos, SLONG while_walking);

// Positions person against wall for a wall-kick attack.
// uc_orig: set_person_kick_off_wall (fallen/Source/Person.cpp)
SLONG set_person_kick_off_wall(Thing* p_person, SLONG col, SLONG set_pos);

// Switches person to fight mode against their gang attacker if any.
// uc_orig: fight_any_gang_attacker (fallen/Source/Person.cpp)
SLONG fight_any_gang_attacker(Thing* p_person);

// Finds the best KO'd arrest candidate within 0x280 units.
// uc_orig: find_arrestee (fallen/Source/Person.cpp)
UWORD find_arrestee(Thing* p_person);

// Finds the nearest dead person within 256 units.
// uc_orig: find_corpse (fallen/Source/Person.cpp)
UWORD find_corpse(Thing* p_person);

// Arrests s_index and shows the HUD message.
// uc_orig: perform_arrest (fallen/Source/Person.cpp)
UWORD perform_arrest(Thing* p_person, UWORD s_index);

// STATE_SEARCH state machine (prim search / corpse looting).
// uc_orig: fn_person_search (fallen/Source/Person.cpp)
void fn_person_search(Thing* p_person);

// Picks a random idle animation appropriate for the person's type.
// uc_orig: set_person_random_idle (fallen/Source/Person.cpp)
void set_person_random_idle(Thing* p_person);

// STATE_IDLE state machine.
// uc_orig: fn_person_idle (fallen/Source/Person.cpp)
void fn_person_idle(Thing* p_person);

// Puts person in the driver's seat of a car.
// uc_orig: set_person_in_vehicle (fallen/Source/Person.cpp)
void set_person_in_vehicle(Thing* p_person, Thing* p_car);

// Removes person from their current vehicle.
// uc_orig: set_person_out_of_vehicle (fallen/Source/Person.cpp)
void set_person_out_of_vehicle(Thing* p_person);

// Switches animation keeping a limb locked in world-space position.
// uc_orig: locked_anim_change (fallen/Source/Person.cpp)
void locked_anim_change(Thing* p_person, UWORD locked_object, UWORD anim, SLONG dangle = 0);

// Variant of locked_anim_change using a specific animation type bank.
// uc_orig: locked_anim_change_of_type (fallen/Source/Person.cpp)
void locked_anim_change_of_type(Thing* p_person, UWORD locked_object, UWORD anim, SLONG type);

// Variant of locked_anim_change_of_type that only adjusts Y (height).
// uc_orig: locked_anim_change_height_type (fallen/Source/Person.cpp)
void locked_anim_change_height_type(Thing* p_person, UWORD locked_object, UWORD anim, SLONG type);

// Adjusts person Y so that limb obj is at world height y.
// uc_orig: set_limb_to_y (fallen/Source/Person.cpp)
SLONG set_limb_to_y(Thing* p_person, SLONG obj, SLONG y);

// Switches to queued_frame while locking a limb in world-space.
// uc_orig: locked_next_anim_change (fallen/Source/Person.cpp)
void locked_next_anim_change(Thing* p_person, UWORD locked_object, GameKeyFrame* queued_frame);

// Jumps to the last frame of an animation (typed bank) while locking a limb.
// uc_orig: locked_anim_change_end_type (fallen/Source/Person.cpp)
void locked_anim_change_end_type(Thing* p_person, UWORD locked_object, UWORD anim, SLONG type);

// Jumps to the last frame of an animation (own AnimType) while locking a limb.
// uc_orig: locked_anim_change_end (fallen/Source/Person.cpp)
void locked_anim_change_end(Thing* p_person, UWORD locked_object, UWORD anim);

// Returns the downhill slope ratio of a cable facet (dy/len * 256).
// uc_orig: steep_cable (fallen/Source/Person.cpp)
SLONG steep_cable(SLONG facet);

// Sets person's angle to face downhill along a cable.
// uc_orig: face_down_cable (fallen/Source/Person.cpp)
void face_down_cable(Thing* p_person, SLONG facet);

// Returns the cable's angle closest to person's current facing (0-2047).
// uc_orig: find_best_cable_angle (fallen/Source/Person.cpp)
SLONG find_best_cable_angle(Thing* p_person, SLONG facet);

#endif // ACTORS_CHARACTERS_PERSON_H
