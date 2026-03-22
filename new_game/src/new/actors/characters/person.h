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

#endif // ACTORS_CHARACTERS_PERSON_H
