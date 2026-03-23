#ifndef ACTORS_CHARACTERS_DARCI_H
#define ACTORS_CHARACTERS_DARCI_H

#include "core/types.h"
#include "actors/core/state.h"

struct Thing;

// uc_orig: fn_darci_init (fallen/Source/Darci.cpp)
// Initialises a Darci thing: sets draw type, animation, and adds it to the map.
void fn_darci_init(Thing* t_thing);

// uc_orig: fn_darci_normal (fallen/Headers/Darci.h)
// Darci has no normal AI mode — declared for completeness but never invoked.
void fn_darci_normal(Thing* t_thing);

// uc_orig: advance_keyframe (fallen/Source/Darci.cpp)
// Advances a KeyFrame pointer by `count` steps following NextFrame links.
KeyFrame* advance_keyframe(KeyFrame* frame, SLONG count);

// uc_orig: do_floor_collide (fallen/Source/Darci.cpp)
// Checks whether the given person's foot sub-object intersects the floor at their current position.
// Returns 1 on collision; sets *new_y and *foot_y.
SLONG do_floor_collide(Thing* p_thing, SWORD pelvis, SLONG* new_y, SLONG* foot_y, SLONG max_range);

// uc_orig: predict_collision_with_floor (fallen/Source/Darci.cpp)
// Simulates one step of movement and checks for floor collision.
SLONG predict_collision_with_floor(Thing* p_thing, SWORD pelvis, SLONG* new_y, SLONG* foot_y);

// uc_orig: predict_collision_with_face (fallen/Source/Darci.cpp)
// Checks for collision with a specific face near the given world position.
SLONG predict_collision_with_face(Thing* p_thing, SLONG wx, SLONG wy, SLONG wz, SWORD pelvis, SLONG* new_y, SLONG* foot_y);

// uc_orig: col_is_fence (fallen/Source/Darci.cpp)
// Returns 1 if the collision vector corresponds to a fence facet type.
SLONG col_is_fence(SLONG col);

// uc_orig: set_person_in_building_through_roof (fallen/Source/Darci.cpp)
// Stub: would place person inside a building when entering through the roof.
void set_person_in_building_through_roof(Thing* p_person, SLONG face);

// uc_orig: damage_person_on_land (fallen/Source/Darci.cpp)
// Applies fall damage to the person based on their DY velocity at landing.
// Returns 1 if the person died, 0 otherwise.
SLONG damage_person_on_land(Thing* p_thing);

// uc_orig: projectile_move_thing (fallen/Source/Darci.cpp)
// Moves a person through the world while they are in projectile (falling/jumping) state.
// Handles wall sliding, floor/face collision, barrels, and death-fall detection.
SLONG projectile_move_thing(Thing* p_thing, SLONG flag);

// uc_orig: change_velocity_to (fallen/Source/Darci.cpp)
// Moves p_thing->Velocity towards the target velocity, stepping by half the difference.
void change_velocity_to(Thing* p_thing, SWORD velocity);

// uc_orig: change_velocity_to_slow (fallen/Source/Darci.cpp)
// Like change_velocity_to but steps by 1 when close to target.
void change_velocity_to_slow(Thing* p_thing, SWORD velocity);

// uc_orig: trickle_velocity_to (fallen/Source/Darci.cpp)
// Moves velocity towards target by 1 unit per tick.
void trickle_velocity_to(Thing* p_thing, SWORD velocity);

// uc_orig: set_thing_velocity (fallen/Source/Darci.cpp)
// Sets the thing's velocity (scaled to the target FPS).
void set_thing_velocity(Thing* t_thing, SLONG vel);

// uc_orig: show_walkable (fallen/Source/Darci.cpp)
// Debug stub: was intended to visualise the walkable grid for a map cell. No-op in current build.
void show_walkable(SLONG mx, SLONG mz);

#endif // ACTORS_CHARACTERS_DARCI_H
