#ifndef MISSIONS_EWAY_H
#define MISSIONS_EWAY_H

#include "missions/eway_globals.h"

// Conversation sub-indices (used internally in EWAY_process_conversation).
// uc_orig: EWAY_CONV_TALK_A (fallen/Source/eway.cpp)
#define EWAY_CONV_TALK_A 0

// uc_orig: EWAY_CONV_TALK_B (fallen/Source/eway.cpp)
#define EWAY_CONV_TALK_B 1

// Extracts the level filename word (e.g. "Rescue1" from "path\Rescue1.ucm").
// Used for building speech filenames.
// uc_orig: get_level_word (fallen/Source/eway.cpp)
void get_level_word(CBYTE* str);

// Returns TRUE if the currently loaded mission is a combat tutorial level.
// uc_orig: playing_combat_tutorial (fallen/Source/eway.cpp)
SLONG playing_combat_tutorial(void);

// Returns TRUE if the currently loaded level filename matches the given name.
// uc_orig: playing_level (fallen/Source/eway.cpp)
SLONG playing_level(const CBYTE* name);

// Returns TRUE if the current level counts as a real mission (not a tutorial/fight/drive test).
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
// Returns TRUE on success, FALSE if buffer is full or index out of range.
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
// Returns TRUE if the file was found and loaded.
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

// ====================================================
// Not-yet-migrated function declarations (still in old/fallen/Source/eway.cpp)
// ====================================================

// uc_orig: EWAY_evaluate_condition (fallen/Source/eway.cpp)
// Default argument (= FALSE) is declared in the definition in old/eway.cpp.
SLONG EWAY_evaluate_condition(EWAY_Way* ew, EWAY_Cond* ec, SLONG EWAY_sub_condition_of_a_boolean);

// uc_orig: EWAY_create_camera (fallen/Source/eway.cpp)
void EWAY_create_camera(SLONG waypoint);

// uc_orig: EWAY_process_camera (fallen/Source/eway.cpp)
void EWAY_process_camera(void);

// uc_orig: EWAY_finish_conversation (fallen/Source/eway.cpp)
void EWAY_finish_conversation(void);

// uc_orig: EWAY_process_conversation (fallen/Source/eway.cpp)
void EWAY_process_conversation(void);

// uc_orig: EWAY_process_emit_steam (fallen/Source/eway.cpp)
void EWAY_process_emit_steam(EWAY_Way* ew);

// uc_orig: EWAY_set_active (fallen/Source/eway.cpp)
void EWAY_set_active(EWAY_Way* ew);

// uc_orig: EWAY_set_inactive (fallen/Source/eway.cpp)
void EWAY_set_inactive(EWAY_Way* ew);

// uc_orig: EWAY_stop_player_moving (fallen/Source/eway.cpp)
SLONG EWAY_stop_player_moving(void);

// uc_orig: EWAY_process_penalties (fallen/Source/eway.cpp)
void EWAY_process_penalties(void);

// uc_orig: count_people_types (fallen/Source/eway.cpp)
void count_people_types(void);

// uc_orig: EWAY_process (fallen/Source/eway.cpp)
void EWAY_process(void);

// uc_orig: EWAY_get_position (fallen/Source/eway.cpp)
void EWAY_get_position(
    SLONG waypoint,
    SLONG* world_x,
    SLONG* world_y,
    SLONG* world_z);

// uc_orig: EWAY_get_angle (fallen/Source/eway.cpp)
UWORD EWAY_get_angle(SLONG waypoint);

// uc_orig: EWAY_get_person (fallen/Source/eway.cpp)
UWORD EWAY_get_person(SLONG waypoint);

// uc_orig: EWAY_get_warehouse (fallen/Source/eway.cpp)
UBYTE EWAY_get_warehouse(SLONG waypoint);

// uc_orig: EWAY_find_waypoint (fallen/Source/eway.cpp)
SLONG EWAY_find_waypoint(
    SLONG index,
    SLONG whatdo,
    SLONG colour,
    SLONG group,
    UBYTE only_active);

// uc_orig: EWAY_find_waypoint_rand (fallen/Source/eway.cpp)
SLONG EWAY_find_waypoint_rand(
    SLONG not_this_index,
    SLONG colour,
    SLONG group,
    UBYTE only_active);

// uc_orig: EWAY_find_nearest_waypoint (fallen/Source/eway.cpp)
SLONG EWAY_find_nearest_waypoint(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG colour,
    SLONG group);

// uc_orig: EWAY_grab_camera (fallen/Source/eway.cpp)
SLONG EWAY_grab_camera(
    SLONG* cam_x,
    SLONG* cam_y,
    SLONG* cam_z,
    SLONG* cam_yaw,
    SLONG* cam_pitch,
    SLONG* cam_roll,
    SLONG* cam_lens);

// uc_orig: EWAY_camera_warehouse (fallen/Source/eway.cpp)
UBYTE EWAY_camera_warehouse(void);

// uc_orig: EWAY_item_pickedup (fallen/Source/eway.cpp)
void EWAY_item_pickedup(SLONG waypoint);

// uc_orig: EWAY_get_delay (fallen/Source/eway.cpp)
SLONG EWAY_get_delay(SLONG waypoint, SLONG default_delay);

// uc_orig: EWAY_is_active (fallen/Source/eway.cpp)
SLONG EWAY_is_active(SLONG waypoint);

// uc_orig: EWAY_used_person (fallen/Source/eway.cpp)
SLONG EWAY_used_person(UWORD t_index);

// uc_orig: EWAY_work_out_which_ones_are_in_warehouses (fallen/Source/eway.cpp)
void EWAY_work_out_which_ones_are_in_warehouses(void);

// uc_orig: EWAY_cam_get_position_for_angle (fallen/Source/eway.cpp)
void EWAY_cam_get_position_for_angle(
    Thing* p_thing,
    SLONG angle,
    SLONG* vx,
    SLONG* vy,
    SLONG* vz);

// uc_orig: EWAY_cam_converse (fallen/Source/eway.cpp)
void EWAY_cam_converse(Thing* p_thing, Thing* p_listener);

// uc_orig: EWAY_cam_look_at (fallen/Source/eway.cpp)
void EWAY_cam_look_at(Thing* p_thing);

// uc_orig: EWAY_cam_relinquish (fallen/Source/eway.cpp)
void EWAY_cam_relinquish(void);

// uc_orig: EWAY_find_or_create_waypoint_that_created_person (fallen/Source/eway.cpp)
SLONG EWAY_find_or_create_waypoint_that_created_person(Thing* p_person);

// uc_orig: EWAY_conversation_happening (fallen/Source/eway.cpp)
SLONG EWAY_conversation_happening(
    THING_INDEX* person_a,
    THING_INDEX* person_b);

// uc_orig: EWAY_prim_activated (fallen/Source/eway.cpp)
void EWAY_prim_activated(SLONG ob_index);

// uc_orig: EWAY_deduct_time_penalty (fallen/Source/eway.cpp)
void EWAY_deduct_time_penalty(SLONG time_to_deduct_in_hundreths_of_a_second);

#endif // MISSIONS_EWAY_H
