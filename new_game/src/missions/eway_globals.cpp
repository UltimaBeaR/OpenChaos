#include "missions/eway_globals.h"

// uc_orig: EWAY_cond_upto (fallen/Source/eway.cpp)
SLONG EWAY_cond_upto;

// uc_orig: EWAY_cond (fallen/Source/eway.cpp)
EWAY_Cond* EWAY_cond;

// uc_orig: EWAY_way (fallen/Source/eway.cpp)
EWAY_Way* EWAY_way;

// uc_orig: EWAY_edef (fallen/Source/eway.cpp)
EWAY_Edef* EWAY_edef;

// uc_orig: EWAY_mess (fallen/Source/eway.cpp)
CBYTE** EWAY_mess;

// uc_orig: EWAY_mess_buffer (fallen/Source/eway.cpp)
CBYTE* EWAY_mess_buffer;

// uc_orig: EWAY_way_upto (fallen/Source/eway.cpp)
SLONG EWAY_way_upto;

// uc_orig: EWAY_edef_upto (fallen/Source/eway.cpp)
SLONG EWAY_edef_upto;

// uc_orig: EWAY_magic_radius_flag (fallen/Source/eway.cpp)
EWAY_Way* EWAY_magic_radius_flag;

// uc_orig: EWAY_mess_buffer_upto (fallen/Source/eway.cpp)
SLONG EWAY_mess_buffer_upto;

// uc_orig: EWAY_mess_upto (fallen/Source/eway.cpp)
SLONG EWAY_mess_upto;

// Accurate time counter: increments at 80 units/tick (1600/sec at 20Hz).
// EWAY_time = EWAY_time_accurate >> 4 (100 units/sec at 20Hz).
// uc_orig: EWAY_time_accurate (fallen/Source/eway.cpp)
SLONG EWAY_time_accurate;

// uc_orig: EWAY_time (fallen/Source/eway.cpp)
SLONG EWAY_time;

// Amount of time since the last process waypoints call (100 ticks per sec).
// uc_orig: EWAY_tick (fallen/Source/eway.cpp)
SLONG EWAY_tick;

// Visible count-up timer for racing/driving missions.
// uc_orig: EWAY_count_up (fallen/Source/eway.cpp)
SLONG EWAY_count_up;

// uc_orig: EWAY_count_up_visible (fallen/Source/eway.cpp)
UBYTE EWAY_count_up_visible;

// uc_orig: EWAY_count_up_add_penalties (fallen/Source/eway.cpp)
UBYTE EWAY_count_up_add_penalties;

// uc_orig: EWAY_count_up_num_penalties (fallen/Source/eway.cpp)
SWORD EWAY_count_up_num_penalties;

// uc_orig: EWAY_count_up_penalty_timer (fallen/Source/eway.cpp)
UWORD EWAY_count_up_penalty_timer;

// uc_orig: EWAY_cam_jumped (fallen/Source/eway.cpp)
SLONG EWAY_cam_jumped = 0;

// Timers indexed by low byte of COUNTDOWN condition arg.
// uc_orig: EWAY_timer (fallen/Source/eway.cpp)
UWORD* EWAY_timer;

// uc_orig: EWAY_counter (fallen/Source/eway.cpp)
UBYTE* EWAY_counter;

// uc_orig: EWAY_fake_wander_text_normal_index (fallen/Source/eway.cpp)
UWORD EWAY_fake_wander_text_normal_index;

// uc_orig: EWAY_fake_wander_text_normal_number (fallen/Source/eway.cpp)
UWORD EWAY_fake_wander_text_normal_number;

// uc_orig: EWAY_fake_wander_text_guilty_index (fallen/Source/eway.cpp)
UWORD EWAY_fake_wander_text_guilty_index;

// uc_orig: EWAY_fake_wander_text_guilty_number (fallen/Source/eway.cpp)
UWORD EWAY_fake_wander_text_guilty_number;

// uc_orig: EWAY_fake_wander_text_annoyed_index (fallen/Source/eway.cpp)
UWORD EWAY_fake_wander_text_annoyed_index;

// uc_orig: EWAY_fake_wander_text_annoyed_number (fallen/Source/eway.cpp)
UWORD EWAY_fake_wander_text_annoyed_number;

// uc_orig: EWAY_tutorial_string (fallen/Source/eway.cpp)
CBYTE* EWAY_tutorial_string;

// uc_orig: EWAY_tutorial_counter (fallen/Source/eway.cpp)
SLONG EWAY_tutorial_counter;

// Bitmask of Darci's combat moves performed this frame (EWAY_DARCI_MOVE_* bits).
// uc_orig: EWAY_darci_move (fallen/Source/eway.cpp)
UBYTE EWAY_darci_move;

// Thing index of the person who says any message (overrides waypoint's person arg).
// uc_orig: EWAY_used_thing (fallen/Source/eway.cpp)
UWORD EWAY_used_thing;

// uc_orig: EWAY_cam_active (fallen/Source/eway.cpp)
SLONG EWAY_cam_active;

// uc_orig: EWAY_cam_goinactive (fallen/Source/eway.cpp)
SLONG EWAY_cam_goinactive;

// uc_orig: EWAY_cam_x (fallen/Source/eway.cpp)
SLONG EWAY_cam_x;

// uc_orig: EWAY_cam_y (fallen/Source/eway.cpp)
SLONG EWAY_cam_y;

// uc_orig: EWAY_cam_z (fallen/Source/eway.cpp)
SLONG EWAY_cam_z;

// uc_orig: EWAY_cam_dx (fallen/Source/eway.cpp)
SLONG EWAY_cam_dx;

// uc_orig: EWAY_cam_dy (fallen/Source/eway.cpp)
SLONG EWAY_cam_dy;

// uc_orig: EWAY_cam_dz (fallen/Source/eway.cpp)
SLONG EWAY_cam_dz;

// uc_orig: EWAY_cam_yaw (fallen/Source/eway.cpp)
SLONG EWAY_cam_yaw;

// uc_orig: EWAY_cam_pitch (fallen/Source/eway.cpp)
SLONG EWAY_cam_pitch;

// uc_orig: EWAY_cam_want_yaw (fallen/Source/eway.cpp)
SLONG EWAY_cam_want_yaw;

// uc_orig: EWAY_cam_want_pitch (fallen/Source/eway.cpp)
SLONG EWAY_cam_want_pitch;

// uc_orig: EWAY_cam_waypoint (fallen/Source/eway.cpp)
SLONG EWAY_cam_waypoint;

// uc_orig: EWAY_cam_target (fallen/Source/eway.cpp)
SLONG EWAY_cam_target;

// uc_orig: EWAY_cam_delay (fallen/Source/eway.cpp)
SLONG EWAY_cam_delay;

// uc_orig: EWAY_cam_speed (fallen/Source/eway.cpp)
SLONG EWAY_cam_speed;

// uc_orig: EWAY_cam_freeze (fallen/Source/eway.cpp)
SLONG EWAY_cam_freeze;

// uc_orig: EWAY_cam_cant_interrupt (fallen/Source/eway.cpp)
SLONG EWAY_cam_cant_interrupt;

// uc_orig: EWAY_cam_thing (fallen/Source/eway.cpp)
UWORD EWAY_cam_thing;

// uc_orig: EWAY_cam_targx (fallen/Source/eway.cpp)
SLONG EWAY_cam_targx;

// uc_orig: EWAY_cam_targy (fallen/Source/eway.cpp)
SLONG EWAY_cam_targy;

// uc_orig: EWAY_cam_targz (fallen/Source/eway.cpp)
SLONG EWAY_cam_targz;

// 16-bit fixed-point lens value for the cutscene camera.
// uc_orig: EWAY_cam_lens (fallen/Source/eway.cpp)
SLONG EWAY_cam_lens;

// uc_orig: EWAY_cam_warehouse (fallen/Source/eway.cpp)
SLONG EWAY_cam_warehouse;

// uc_orig: EWAY_cam_lock (fallen/Source/eway.cpp)
SLONG EWAY_cam_lock;

// uc_orig: EWAY_cam_last_yaw (fallen/Source/eway.cpp)
SLONG EWAY_cam_last_yaw;

// uc_orig: EWAY_cam_last_x (fallen/Source/eway.cpp)
SLONG EWAY_cam_last_x;

// uc_orig: EWAY_cam_last_y (fallen/Source/eway.cpp)
SLONG EWAY_cam_last_y;

// uc_orig: EWAY_cam_last_z (fallen/Source/eway.cpp)
SLONG EWAY_cam_last_z;

// uc_orig: EWAY_cam_skip (fallen/Source/eway.cpp)
SLONG EWAY_cam_skip;

// uc_orig: EWAY_cam_last_dyaw (fallen/Source/eway.cpp)
SLONG EWAY_cam_last_dyaw;

// Set to non-zero while a cutscene is playing; prevents player movement.
// Also declared extern in pcom.cpp.
// uc_orig: GAME_cut_scene (fallen/Source/eway.cpp)
UBYTE GAME_cut_scene = 0;

// Scratch buffer for formatting console/debug messages about waypoints.
// uc_orig: EWAY_message (fallen/Source/eway.cpp)
CBYTE EWAY_message[128];

// uc_orig: EWAY_conv_active (fallen/Source/eway.cpp)
UBYTE EWAY_conv_active;

// uc_orig: EWAY_conv_waypoint (fallen/Source/eway.cpp)
UWORD EWAY_conv_waypoint;

// uc_orig: EWAY_conv_person_a (fallen/Source/eway.cpp)
THING_INDEX EWAY_conv_person_a;

// uc_orig: EWAY_conv_person_b (fallen/Source/eway.cpp)
THING_INDEX EWAY_conv_person_b;

// Index into EWAY_mess_buffer for the current conversation line.
// uc_orig: EWAY_conv_str (fallen/Source/eway.cpp)
UWORD EWAY_conv_str;

// Line index within the current conversation (0 = first line).
// uc_orig: EWAY_conv_str_count (fallen/Source/eway.cpp)
UWORD EWAY_conv_str_count;

// uc_orig: EWAY_conv_timer (fallen/Source/eway.cpp)
SLONG EWAY_conv_timer;

// How long until the player is allowed to skip the conversation.
// uc_orig: EWAY_conv_skip (fallen/Source/eway.cpp)
SLONG EWAY_conv_skip;

// UC_TRUE => ambient conversation: no camera takeover, no widescreen.
// uc_orig: EWAY_conv_ambient (fallen/Source/eway.cpp)
SLONG EWAY_conv_ambient;

// uc_orig: EWAY_conv_talk (fallen/Source/eway.cpp)
SLONG EWAY_conv_talk = 0;

// Thing currently doing voiced speech (checked to stop speech on death/hit).
// uc_orig: talk_thing (fallen/Source/eway.cpp)
Thing* talk_thing;

// List of non-real-mission level names used by playing_real_mission() to exclude
// training/fight/drive missions from the score system.
// uc_orig: crap_levels (fallen/Source/eway.cpp)
CBYTE* crap_levels[] = {
    "FTutor1.ucm",
    "Assault1.ucm",
    "testdrive1a.ucm",
    "fight1.ucm",
    "testdrive2.ucm",
    "fight2.ucm",
    "testdrive3.ucm",
    0
};

// Scratch variables for OB_find_type results, reused across EWAY_create_cond calls.
// uc_orig: ob_x (fallen/Source/eway.cpp)
SLONG ob_x;

// uc_orig: ob_y (fallen/Source/eway.cpp)
SLONG ob_y;

// uc_orig: ob_z (fallen/Source/eway.cpp)
SLONG ob_z;

// uc_orig: ob_yaw (fallen/Source/eway.cpp)
SLONG ob_yaw;

// uc_orig: ob_prim (fallen/Source/eway.cpp)
SLONG ob_prim;

// uc_orig: ob_index (fallen/Source/eway.cpp)
SLONG ob_index;
