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

#endif // ACTORS_CHARACTERS_PERSON_H
