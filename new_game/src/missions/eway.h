#ifndef MISSIONS_EWAY_H
#define MISSIONS_EWAY_H

#include "missions/eway_globals.h"

// Extracts the level filename word (e.g. "Rescue1" from "path\Rescue1.ucm").
// Used for building speech filenames.
// uc_orig: get_level_word (fallen/Source/eway.cpp)
void get_level_word(CBYTE* str);

// Returns UC_TRUE if the currently loaded mission is a combat tutorial level.
// uc_orig: playing_combat_tutorial (fallen/Source/eway.cpp)
SLONG playing_combat_tutorial(void);

// Returns UC_TRUE if the currently loaded level filename matches the given name.
// uc_orig: playing_level (fallen/Source/eway.cpp)
SLONG playing_level(const CBYTE* name);

// Returns UC_TRUE if the current level counts as a real mission (not a tutorial/fight/drive test).
// uc_orig: playing_real_mission (fallen/Source/eway.cpp)
SLONG playing_real_mission(void);

// Plays the voiced speech WAV for the given waypoint number.
// File path: <SpeechPath>/talk2/<levelname>.ucm<waypoint>.wav
// uc_orig: EWAY_talk (fallen/Source/eway.cpp)
void EWAY_talk(ULONG waypoint);

// Checks whether the currently-playing speech should be stopped
// (speaker died, got hit, or stop flag is set).
// uc_orig: check_eway_talk (fallen/Source/eway.cpp)
void check_eway_talk(SLONG stop);

// Plays a conversation speech WAV for the given waypoint + sub-part index (A/B/C...).
// uc_orig: EWAY_talk_conv (fallen/Source/eway.cpp)
void EWAY_talk_conv(ULONG waypoint, SLONG conversation);

// Returns the message string for the given index (asserts it's in range).
// uc_orig: EWAY_get_mess (fallen/Source/eway.cpp)
CBYTE* EWAY_get_mess(SLONG index);

// Resets all EWAY state (waypoints, messages, conditions, timers) for a fresh level.
// Does NOT allocate memory — arrays are allocated before this is called.
// uc_orig: EWAY_init (fallen/Source/eway.cpp)
void EWAY_init(void);

// Creates the player character at the given position.
// uc_orig: EWAY_create_player (fallen/Source/eway.cpp)
UWORD EWAY_create_player(
    UBYTE subtype,
    UBYTE yaw,
    SLONG has,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z);

// Creates an NPC enemy. Zone byte encodes spawn zone (low 4 bits) + flags in bits 4-6.
// uc_orig: EWAY_create_enemy (fallen/Source/eway.cpp)
UWORD EWAY_create_enemy(
    UBYTE subtype,
    UBYTE yaw,
    UBYTE colour,
    UBYTE group,
    UBYTE pcom_ai,
    UBYTE pcom_bent,
    UBYTE pcom_move,
    UBYTE drop,
    UBYTE zone,
    SLONG skill,
    UWORD follow,
    UWORD other,
    SLONG has,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG random);

// Creates an animal (bat, gargoyle, balrog, bane) at the given position.
// uc_orig: EWAY_create_animal (fallen/Source/eway.cpp)
UWORD EWAY_create_animal(
    UBYTE subtype,
    UBYTE yaw,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z);

// Creates a pickup special at a waypoint position. If stashed, hides it inside a nearby OB.
// uc_orig: EWAY_create_item (fallen/Source/eway.cpp)
UWORD EWAY_create_item(
    UBYTE subtype,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    EWAY_Way* ew);

// Creates a vehicle. Wheeled vehicles fall to ground from 1 square above the waypoint.
// uc_orig: EWAY_create_vehicle (fallen/Source/eway.cpp)
UWORD EWAY_create_vehicle(
    UBYTE subtype,
    UBYTE key,
    UWORD arg,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG yaw);

// Resolves an EWAY_Conddef (raw .ucm condition data) into a runtime EWAY_Cond.
// Handles tripwire/switch/prim OB lookups and recursive boolean sub-conditions.
// uc_orig: EWAY_create_cond (fallen/Source/eway.cpp)
EWAY_Cond EWAY_create_cond(
    EWAY_Way* ew,
    EWAY_Conddef* ecd);

// Creates and registers a waypoint from .ucm file data.
// Must be called in loading order; EWAY_created_last_waypoint() finalises them.
// uc_orig: EWAY_create (fallen/Source/eway.cpp)
void EWAY_create(
    SLONG identifier,
    SLONG colour,
    SLONG group,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG yaw,
    EWAY_Conddef* ecd,
    EWAY_Do* ed,
    EWAY_Stay* es,
    EWAY_Edef* ee,
    SLONG unreferenced,
    SLONG kludge_index,
    UWORD magic_index);

// Stores the message string for message index 'number' into the EWAY message buffer.
// Returns UC_TRUE on success, UC_FALSE if buffer is full or index out of range.
// uc_orig: EWAY_set_message (fallen/Source/eway.cpp)
SLONG EWAY_set_message(
    UBYTE number,
    CBYTE* message);

// Linear search for a waypoint by designer-assigned id. Returns array index or NULL.
// uc_orig: EWAY_find_id (fallen/Source/eway.cpp)
SLONG EWAY_find_id(SLONG id);

// Post-load ID-to-index resolution for a condition struct.
// uc_orig: EWAY_fix_cond (fallen/Source/eway.cpp)
void EWAY_fix_cond(EWAY_Cond* ec);

// Post-load ID-to-index resolution for an action struct.
// uc_orig: EWAY_fix_do (fallen/Source/eway.cpp)
void EWAY_fix_do(EWAY_Do* ed, EWAY_Way* ew);

// Post-load ID-to-index resolution for an enemy definition.
// uc_orig: EWAY_fix_edef (fallen/Source/eway.cpp)
void EWAY_fix_edef(EWAY_Edef* ee);

// Loads a numbered text file of mission messages into the EWAY message buffer.
// Returns UC_TRUE if the file was found and loaded.
// uc_orig: EWAY_load_message_file (fallen/Source/eway.cpp)
SLONG EWAY_load_message_file(CBYTE* fname, UWORD* index, UWORD* number);

// Loads ambient NPC dialogue pools: normal/guilty/annoyed lines from text files.
// Handles localisation (Italian/French/German/Spanish).
// uc_orig: EWAY_load_fake_wander_text (fallen/Source/eway.cpp)
void EWAY_load_fake_wander_text(CBYTE* fname);

// Returns a random message from the appropriate ambient-NPC dialogue pool.
// type: EWAY_FAKE_MESSAGE_NORMAL / ANNOYED / GUILTY
// uc_orig: EWAY_get_fake_wander_message (fallen/Source/eway.cpp)
CBYTE* EWAY_get_fake_wander_message(SLONG type);

// Post-load fixup pass: resolves all ID references to indices, calculates CRIME_RATE_SCORE_MUL,
// tags mission-fail messages with EWAY_FLAG_WHY_LOST.
// Call once after all EWAY_create() calls.
// uc_orig: EWAY_created_last_waypoint (fallen/Source/eway.cpp)
void EWAY_created_last_waypoint(void);

// Evaluates a single condition struct. Returns UC_TRUE if satisfied. EWAY_sub_condition_of_a_boolean=UC_TRUE
// skips post-evaluation guards (called from BOOL_AND/OR recursion).
// uc_orig: EWAY_evaluate_condition (fallen/Source/eway.cpp)
SLONG EWAY_evaluate_condition(EWAY_Way* ew, EWAY_Cond* ec, SLONG EWAY_sub_condition_of_a_boolean = UC_FALSE);

// Initialises the scripted cut-scene camera from a CAMERA_CREATE waypoint.
// uc_orig: EWAY_create_camera (fallen/Source/eway.cpp)
void EWAY_create_camera(SLONG waypoint);

// Advances the scripted cut-scene camera one tick.
// uc_orig: EWAY_process_camera (fallen/Source/eway.cpp)
void EWAY_process_camera(void);

// Ends the current two-person scripted conversation.
// uc_orig: EWAY_finish_conversation (fallen/Source/eway.cpp)
void EWAY_finish_conversation(void);

// Advances the active scripted conversation one tick.
// uc_orig: EWAY_process_conversation (fallen/Source/eway.cpp)
void EWAY_process_conversation(void);

// Emits steam particles from an active EWAY_DO_EMIT_STEAM waypoint.
// uc_orig: EWAY_process_emit_steam (fallen/Source/eway.cpp)
void EWAY_process_emit_steam(EWAY_Way* ew);

// Fires a waypoint's action when its condition becomes true.
// uc_orig: EWAY_set_active (fallen/Source/eway.cpp)
void EWAY_set_active(EWAY_Way* ew);

// Deactivates a waypoint and runs its teardown actions (close door, fence off, beacon remove).
// uc_orig: EWAY_set_inactive (fallen/Source/eway.cpp)
void EWAY_set_inactive(EWAY_Way* ew);

// Returns non-zero if player movement should be frozen (scripted camera active + freeze, or cut scene).
// uc_orig: EWAY_stop_player_moving (fallen/Source/eway.cpp)
SLONG EWAY_stop_player_moving(void);

// Animates the cone-penalty count-up display after a driving mission timer expires.
// uc_orig: EWAY_process_penalties (fallen/Source/eway.cpp)
void EWAY_process_penalties(void);

// Main mission update: evaluates all waypoints each frame, ticks conversation/penalties/camera.
// uc_orig: EWAY_process (fallen/Source/eway.cpp)
void EWAY_process(void);

// Returns world-space position of a waypoint.
// uc_orig: EWAY_get_position (fallen/Source/eway.cpp)
void EWAY_get_position(
    SLONG waypoint,
    SLONG* world_x,
    SLONG* world_y,
    SLONG* world_z);

// Returns waypoint facing angle (yaw << 3).
// uc_orig: EWAY_get_angle (fallen/Source/eway.cpp)
UWORD EWAY_get_angle(SLONG waypoint);

// Returns Thing index spawned by a CREATE_ENEMY/PLAYER/VEHICLE/ANIMAL waypoint, or NULL.
// uc_orig: EWAY_get_person (fallen/Source/eway.cpp)
UWORD EWAY_get_person(SLONG waypoint);

// Returns warehouse index for a waypoint (0 if not inside a hidden building).
// uc_orig: EWAY_get_warehouse (fallen/Source/eway.cpp)
UBYTE EWAY_get_warehouse(SLONG waypoint);

// Circular search for a waypoint matching type/colour/group. Returns index or EWAY_NO_MATCH.
// uc_orig: EWAY_find_waypoint (fallen/Source/eway.cpp)
SLONG EWAY_find_waypoint(
    SLONG index,
    SLONG whatdo,
    SLONG colour,
    SLONG group,
    UBYTE only_active);

// Picks a random waypoint matching colour/group (excluding not_this_index). Returns index or EWAY_NO_MATCH.
// uc_orig: EWAY_find_waypoint_rand (fallen/Source/eway.cpp)
SLONG EWAY_find_waypoint_rand(
    SLONG not_this_index,
    SLONG colour,
    SLONG group,
    UBYTE only_active);

// Returns index of nearest active waypoint matching colour/group to (x,y,z), or EWAY_NO_MATCH.
// uc_orig: EWAY_find_nearest_waypoint (fallen/Source/eway.cpp)
SLONG EWAY_find_nearest_waypoint(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG colour,
    SLONG group);

// Fills camera pose from the active scripted camera. Returns UC_FALSE if suppressed by analogue controls.
// uc_orig: EWAY_grab_camera (fallen/Source/eway.cpp)
SLONG EWAY_grab_camera(
    SLONG* cam_x,
    SLONG* cam_y,
    SLONG* cam_z,
    SLONG* cam_yaw,
    SLONG* cam_pitch,
    SLONG* cam_roll,
    SLONG* cam_lens);

// Returns warehouse index for the active scripted camera (or speaker's warehouse during conversation).
// uc_orig: EWAY_camera_warehouse (fallen/Source/eway.cpp)
UBYTE EWAY_camera_warehouse(void);

// Flags a CREATE_ITEM waypoint as collected (player picked up the spawned item).
// uc_orig: EWAY_item_pickedup (fallen/Source/eway.cpp)
void EWAY_item_pickedup(SLONG waypoint);

// Returns delay in ms from a EWAY_DO_NOTHING waypoint, or default_delay if not applicable.
// uc_orig: EWAY_get_delay (fallen/Source/eway.cpp)
SLONG EWAY_get_delay(SLONG waypoint, SLONG default_delay);

// Returns UC_TRUE if the given waypoint is currently active.
// uc_orig: EWAY_is_active (fallen/Source/eway.cpp)
SLONG EWAY_is_active(SLONG waypoint);

// Triggers COND_PERSON_USED waypoints for the given Thing; clears FLAG_PERSON_USEABLE.
// uc_orig: EWAY_used_person (fallen/Source/eway.cpp)
SLONG EWAY_used_person(UWORD t_index);

// Tags all waypoints inside hidden buildings with the containing warehouse index.
// uc_orig: EWAY_work_out_which_ones_are_in_warehouses (fallen/Source/eway.cpp)
void EWAY_work_out_which_ones_are_in_warehouses(void);

// Positions the scripted camera for a two-person conversation using scored LOS sampling.
// uc_orig: EWAY_cam_converse (fallen/Source/eway.cpp)
void EWAY_cam_converse(Thing* p_thing, Thing* p_listener);

// Begins the fade-out back from scripted camera to player camera (sets goinactive=2).
// uc_orig: EWAY_cam_relinquish (fallen/Source/eway.cpp)
void EWAY_cam_relinquish(void);

// Returns waypoint index that spawned p_person; creates a dummy waypoint if not found.
// uc_orig: EWAY_find_or_create_waypoint_that_created_person (fallen/Source/eway.cpp)
SLONG EWAY_find_or_create_waypoint_that_created_person(Thing* p_person);

// Returns UC_TRUE if a scripted conversation is active; fills person_a/person_b with speaker indices.
// uc_orig: EWAY_conversation_happening (fallen/Source/eway.cpp)
SLONG EWAY_conversation_happening(
    THING_INDEX* person_a,
    THING_INDEX* person_b);

// Sets EWAY_FLAG_TRIGGERED on any waypoint waiting for ob_index's prim activation.
// uc_orig: EWAY_prim_activated (fallen/Source/eway.cpp)
void EWAY_prim_activated(SLONG ob_index);

// Deducts time_to_deduct (hundredths/sec) from all active COUNTDOWN_SEE timers (driving penalty).
// uc_orig: EWAY_deduct_time_penalty (fallen/Source/eway.cpp)
void EWAY_deduct_time_penalty(SLONG time_to_deduct_in_hundreths_of_a_second);

#endif // MISSIONS_EWAY_H
