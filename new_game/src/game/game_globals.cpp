// uc_orig: Game.cpp (fallen/Source/Game.cpp)
// Global variables from the top-level game orchestrator.

#include "game/game_globals.h"

// uc_orig: the_game (fallen/Source/Game.cpp)
Game the_game;

// Violence flag: always 1 in standard builds.
// Set to 0 only for German/French PC censored versions (not needed in new game).
// uc_orig: VIOLENCE (fallen/Source/Game.cpp)
UBYTE VIOLENCE = 1;

// uc_orig: CAM_cur_x (fallen/Source/Game.cpp)
SLONG CAM_cur_x = 0;

// uc_orig: CAM_cur_y (fallen/Source/Game.cpp)
SLONG CAM_cur_y = 0;

// uc_orig: CAM_cur_z (fallen/Source/Game.cpp)
SLONG CAM_cur_z = 0;

// uc_orig: CAM_cur_yaw (fallen/Source/Game.cpp)
SLONG CAM_cur_yaw = 0;

// uc_orig: CAM_cur_pitch (fallen/Source/Game.cpp)
SLONG CAM_cur_pitch = 0;

// uc_orig: CAM_cur_roll (fallen/Source/Game.cpp)
SLONG CAM_cur_roll = 0;

// uc_orig: pause_menu (fallen/Source/Game.cpp)
CBYTE* pause_menu[PAUSE_MENU_SIZE] = {
    "CONTINUE GAME",
    "EXIT",
};

// uc_orig: game_paused_key (fallen/Source/Game.cpp)
UBYTE game_paused_key = 0;

// uc_orig: game_paused_highlight (fallen/Source/Game.cpp)
SBYTE game_paused_highlight = 0;

// uc_orig: bullet_point (fallen/Source/Game.cpp)
CBYTE* bullet_point[NUM_BULLETS] = {
    "A - Punch : B - Kick : C - Jump\n",
    "Try running over a coke can...\n",
    "Press JUMP and push back to do a backwards summersault!\n",
    "Stand in a puddle and see your reflection!\n",
    "Z is your action button. Press it to climb ladders\nand to take out and put away your gun\n",
    "You can swirl the mist by running through it.\n",
    "Your shadow goes up walls!\n",
    "Press JUMP and push left or right to do a cartwheel\n",
    "Jump on fences to clamber over them.\nWatch out! Some of them are electrified.\n",
    "You can climb onto the rooftops.\nSee how high you can go!\n",
    "Hold down Y to look around in first person.\n",
    "Sneak up behind enemies and press punch.\nIf you're in the right place you'll break their necks!\n",
    "The shoulder buttons rotate the camera.\n",
    "Press action to climb up ladders.\n",
    "X is your mode change button.\nChange through RUN, WALK and SNEAK modes\n",
};

// uc_orig: bullet_upto (fallen/Source/Game.cpp)
SLONG bullet_upto = 0;

// uc_orig: bullet_counter (fallen/Source/Game.cpp)
SLONG bullet_counter = 0;

// uc_orig: screen_mem (fallen/Source/Game.cpp)
UBYTE screen_mem[640 * 3][480];

// uc_orig: already_warned_about_leaving_map (fallen/Source/Game.cpp)
// claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
DWORD already_warned_about_leaving_map = 0;

// uc_orig: draw_map_screen (fallen/Source/Game.cpp)
UBYTE draw_map_screen = 0;

// uc_orig: single_step (fallen/Source/Game.cpp)
UBYTE single_step = 0;

// uc_orig: form_leave_map (fallen/Source/Game.cpp)
Form* form_leave_map = NULL;

// uc_orig: form_left_map (fallen/Source/Game.cpp)
SLONG form_left_map = 0;

// uc_orig: last_fudge_message (fallen/Source/Game.cpp)
UWORD last_fudge_message = 0;

// uc_orig: last_fudge_camera (fallen/Source/Game.cpp)
UWORD last_fudge_camera = 0;

// uc_orig: the_end (fallen/Source/Game.cpp)
UBYTE the_end = 0;

// uc_orig: env_frame_rate (fallen/Source/Game.cpp)
UWORD env_frame_rate = 30;

// uc_orig: playback_name (fallen/Source/Game.cpp)
// uc-abs-path: was "C:\Windows\Desktop\UrbanChaosRecordedGame.pkt"
CBYTE* playback_name = "UrbanChaosRecordedGame.pkt";

// uc_orig: verifier_name (fallen/Source/Game.cpp)
// uc-abs-path: was "C:\Windows\Desktop\UrbanChaosRecordedGame.tst"
CBYTE* verifier_name = "UrbanChaosRecordedGame.tst";

// player_pos is defined in actors/core/player_globals.cpp (from Player.cpp).

// uc_orig: tick_ratios (fallen/Source/Game.cpp)
SLONG tick_ratios[4];

// uc_orig: wptr (fallen/Source/Game.cpp)
SLONG wptr = 0;

// uc_orig: number (fallen/Source/Game.cpp)
SLONG number = 0;

// uc_orig: sum (fallen/Source/Game.cpp)
SLONG sum = 0;
