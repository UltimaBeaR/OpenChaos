#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/core/statedef.h"
#include "missions/eway.h"
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "things/core/interact.h"
#include "things/core/interact_globals.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "ui/hud/overlay.h"
#include "ui/hud/overlay_globals.h"

// uc_orig: draw_view_line (fallen/Source/pcom.cpp)
extern void draw_view_line(Thing* p_person, Thing* p_target);

// Deferred view line rendering: draw_view_line is called during game tick (pcom),
// but POLY_frame_init in AENG_draw clears all poly pages before rendering.
// We store shooter/target pairs during tick and draw them during overlay pass.
#define MAX_DEFERRED_VIEW_LINES 8
static struct {
    Thing* shooter;
    Thing* target;
} s_deferred_view_lines[MAX_DEFERRED_VIEW_LINES];
static int s_deferred_view_line_count = 0;

void OVERLAY_queue_view_line(Thing* shooter, Thing* target)
{
    if (s_deferred_view_line_count < MAX_DEFERRED_VIEW_LINES) {
        s_deferred_view_lines[s_deferred_view_line_count].shooter = shooter;
        s_deferred_view_lines[s_deferred_view_line_count].target = target;
        s_deferred_view_line_count++;
    }
}

static void OVERLAY_draw_deferred_view_lines()
{
    extern BOOL allow_debug_keys;
    extern volatile UBYTE ControlFlag;

    if (allow_debug_keys && ControlFlag) {
        for (int i = 0; i < s_deferred_view_line_count; i++) {
            draw_view_line(s_deferred_view_lines[i].shooter, s_deferred_view_lines[i].target);
        }
    }
    s_deferred_view_line_count = 0;
}

// Forward declarations for functions not yet migrated to new/.
// uc_orig: PANEL_draw_face (fallen/DDEngine/Source/panel.cpp)
extern void PANEL_draw_face(SLONG x, SLONG y, SLONG face, SLONG size);
// uc_orig: PANEL_draw_local_health (fallen/DDEngine/Source/panel.cpp)
extern void PANEL_draw_local_health(SLONG mx, SLONG my, SLONG mz, SLONG percentage, SLONG radius);
// uc_orig: person_holding_special (fallen/Source/interfac.cpp)
extern SLONG person_holding_special(Thing* p_person, UBYTE special);
// uc_orig: show_grenade_path (fallen/Source/guns.cpp)
extern void show_grenade_path(Thing* p_person);
// uc_orig: PersonIsMIB (fallen/Source/Person.cpp)
extern BOOL PersonIsMIB(Thing* p_person);
// uc_orig: health (fallen/Source/Person.cpp)
extern SWORD health[];
// uc_orig: draw_map_screen (fallen/Source/Game.cpp)
extern UBYTE draw_map_screen;
// uc_orig: cheat (fallen/Source/Controls.cpp)
extern UBYTE cheat;
// uc_orig: tick_tock_unclipped (fallen/Source/Game.cpp)
extern SLONG tick_tock_unclipped;
// uc_orig: RealDisplayWidth (fallen/DDLibrary/Source/GDisplay.cpp)
extern SLONG RealDisplayWidth;
// uc_orig: RealDisplayHeight (fallen/DDLibrary/Source/GDisplay.cpp)
extern SLONG RealDisplayHeight;

// uc_orig: MAX_TRACK (fallen/Source/overlay.cpp)
#define MAX_TRACK 4

// uc_orig: STATE_TRACKING (fallen/Source/overlay.cpp)
#define STATE_TRACKING 1
// uc_orig: STATE_UNUSED (fallen/Source/overlay.cpp)
#define STATE_UNUSED 0

// uc_orig: INFO_NUMBER (fallen/Source/overlay.cpp)
#define INFO_NUMBER 1
// uc_orig: INFO_TEXT (fallen/Source/overlay.cpp)
#define INFO_TEXT 2

// Interaction hint types for arrow_object / arrow_pos.
// uc_orig: USE_CAR (fallen/Source/overlay.cpp)
#define USE_CAR (1)
// uc_orig: USE_CABLE (fallen/Source/overlay.cpp)
#define USE_CABLE (2)
// uc_orig: USE_LADDER (fallen/Source/overlay.cpp)
#define USE_LADDER (3)
// uc_orig: PICKUP_ITEM (fallen/Source/overlay.cpp)
#define PICKUP_ITEM (4)
// uc_orig: USE_MOTORBIKE (fallen/Source/overlay.cpp)
#define USE_MOTORBIKE (5)
// uc_orig: TALK (fallen/Source/overlay.cpp)
#define TALK (6)

// uc_orig: HELP_MAX_COL (fallen/Source/overlay.cpp)
#define HELP_MAX_COL 16
// uc_orig: HELP_GRAB_CABLE (fallen/Source/overlay.cpp)
#define HELP_GRAB_CABLE 0
// uc_orig: HELP_PICKUP_ITEM (fallen/Source/overlay.cpp)
#define HELP_PICKUP_ITEM 1
// uc_orig: HELP_USE_CAR (fallen/Source/overlay.cpp)
#define HELP_USE_CAR 2
// uc_orig: HELP_USE_BIKE (fallen/Source/overlay.cpp)
#define HELP_USE_BIKE 3

// Returns true if an in-world hint message of this type can be shown this frame.
// Throttles messages to once per 100 game turns to avoid spamming.
// uc_orig: should_i_add_message (fallen/Source/overlay.cpp)
SLONG should_i_add_message(SLONG type)
{
    static SLONG last_message[4] = { 0, 0, 0, 0 };

    ASSERT(WITHIN(type, 0, 3));

    if (last_message[type] > GAME_TURN || last_message[type] <= GAME_TURN - 100) {
        last_message[type] = GAME_TURN;
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// Empty stub — help text display was disabled pre-release.
// uc_orig: show_help_text (fallen/Source/overlay.cpp)
void show_help_text(SLONG index)
{
    // Stub — help text display was disabled pre-release.
}

// uc_orig: track_enemy (fallen/Source/overlay.cpp)
void track_enemy(Thing* p_thing)
{
    // Stub — enemy tracking overlay was cut.
}

// Registers a Thing as a gun sight target for this frame.
// If the same Thing is tracked twice, keeps the lower accuracy value.
// uc_orig: track_gun_sight (fallen/Source/overlay.cpp)
void track_gun_sight(Thing* p_thing, SLONG accuracy)
{
    SLONG c0;
    for (c0 = 0; c0 < track_count; c0++) {
        if (panel_gun_sight[c0].PThing == p_thing) {
            if (accuracy < panel_gun_sight[c0].Timer)
                panel_gun_sight[c0].Timer = accuracy;
            return;
        }
    }
    if (track_count < MAX_TRACK) {
        panel_gun_sight[track_count].Timer = accuracy;
        panel_gun_sight[track_count].PThing = p_thing;
        track_count++;
    }
}

// Draws tracked enemy faces and health bars (disabled in original — loop body has unconditional STATE_UNUSED).
// uc_orig: OVERLAY_draw_tracked_enemies (fallen/Source/overlay.cpp)
void OVERLAY_draw_tracked_enemies(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_TRACK; c0++) {
        if (panel_enemy[c0].State == STATE_TRACKING) {
            SLONG h;

            h = panel_enemy[c0].PThing->Genus.Person->Health;
            if (h < 0)
                h = 0;
            PANEL_draw_face(c0 * 150 + 5, 450 - 14, panel_enemy[c0].Face, 32);
            PANEL_draw_health_bar(40 + c0 * 150, 450, h >> 1);
            PANEL_draw_health_bar(40 + c0 * 150, 460, (GET_SKILL(panel_enemy[c0].PThing) * 100) / 15);

            panel_enemy[c0].Timer -= TICK_TOCK;
            {
                panel_enemy[c0].State = STATE_UNUSED;
                panel_enemy[c0].PThing = 0;
            }
        }
    }
}

// Draws 3D crosshair sights on all targets registered this frame.
// Also draws grenade path preview when the player holds a grenade.
// Resets track_count to 0 after drawing.
// uc_orig: OVERLAY_draw_gun_sights (fallen/Source/overlay.cpp)
void OVERLAY_draw_gun_sights(void)
{
    SLONG c0;
    SLONG hx, hy, hz;
    Thing* p_thing;

    POLY_flush_local_rot();

    for (c0 = 0; c0 < track_count; c0++) {
        p_thing = panel_gun_sight[c0].PThing;
        switch (p_thing->Class) {
        case CLASS_PERSON:
            // Bone 11 is the head.
            calc_sub_objects_position(p_thing, p_thing->Draw.Tweened->AnimTween, 11, &hx, &hy, &hz);
            hx += p_thing->WorldPos.X >> 8;
            hy += p_thing->WorldPos.Y >> 8;
            hz += p_thing->WorldPos.Z >> 8;
            PANEL_draw_gun_sight(hx, hy, hz, panel_gun_sight[c0].Timer, 256);
            break;
        case CLASS_SPECIAL:
            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 30, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, 128);
            break;
        case CLASS_BAT:
        {
            SLONG scale;
            if (p_thing->Genus.Bat->type == BAT_TYPE_BALROG) {
                scale = 450;
            } else {
                scale = 128;
            }
            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 30, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, scale);
        }
        break;
        case CLASS_BARREL:
            p_thing = panel_gun_sight[c0].PThing;
            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 80, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, 200);
            break;
        case CLASS_VEHICLE:
            p_thing = panel_gun_sight[c0].PThing;
            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 80, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, 450);
            break;
        }
    }
    track_count = 0;

    Thing* p_player = NET_PERSON(0);

    if (person_holding_special(p_player, SPECIAL_GRENADE)) {
        if (!p_player->Genus.Person->Ware) {
            show_grenade_path(p_player);
        }
    }
}

// Draws a health indicator above the player's current fight/aim target.
// uc_orig: OVERLAY_draw_enemy_health (fallen/Source/overlay.cpp)
void OVERLAY_draw_enemy_health(void)
{
    Thing* p_person;

    p_person = NET_PERSON(0);

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT || p_person->SubState == SUB_STATE_AIM_GUN) {
        if (p_person->Genus.Person->Target) {
            Thing* p_target;
            p_target = TO_THING(p_person->Genus.Person->Target);

            switch (p_target->Class) {
            case CLASS_BAT:
                if (p_target->Genus.Bat->type == BAT_TYPE_BALROG) {
                    PANEL_draw_local_health(p_target->WorldPos.X >> 8, p_target->WorldPos.Y >> 8, p_target->WorldPos.Z >> 8, (100 * p_target->Genus.Bat->health) >> 8, 300);
                }
                break;
            case CLASS_PERSON:
            {
                SLONG percent;

                if (PersonIsMIB(p_target)) {
                    percent = p_target->Genus.Person->Health * 100 / 700;
                } else {
                    percent = p_target->Genus.Person->Health * 100 / health[p_target->Genus.Person->PersonType];
                }

                PANEL_draw_local_health(p_target->WorldPos.X >> 8, p_target->WorldPos.Y >> 8, p_target->WorldPos.Z >> 8, percent, 60);
            }
            break;
            }
        }
    }
}

// Draws the full HUD for one game frame: panel, gun sights, enemy health, debug info.
// uc_orig: OVERLAY_handle (fallen/Source/overlay.cpp)
void OVERLAY_handle(void)
{
    Thing* darci = NET_PERSON(0);
    Thing* player = NET_PLAYER(0);
    SLONG panel = 1;

    POLY_flush_local_rot();

    // Reset viewport to full screen (required for letterbox mode so HUD draws edge-to-edge).
    ge_set_viewport(0, 0, RealDisplayWidth, RealDisplayHeight);

    PANEL_start();

    if (!EWAY_stop_player_moving()) {
        if (panel) {
            PANEL_draw_buffered();
            OVERLAY_draw_gun_sights();
            OVERLAY_draw_deferred_view_lines();
            OVERLAY_draw_enemy_health();
        }
    }

    if (panel) {
        if (!(GAME_STATE & GS_LEVEL_WON)) {
            PANEL_last();
        }
    }

    if (!draw_map_screen) {
        if (darci) {
            if (darci->State == STATE_SEARCH) {
                // PC search progress display was commented out in original.
            }
        }
    }

    PANEL_inventory(darci, player);

    if (cheat == 2) {
        CBYTE str[50];

        extern SLONG tick_tock_unclipped;
        if (tick_tock_unclipped == 0)
            tick_tock_unclipped = 1;

        extern SLONG SW_tick1;
        extern SLONG SW_tick2;
        extern ULONG debug_input;

        extern SLONG geom;
        extern SLONG EWAY_cam_jumped;
        extern SLONG look_pitch;

        sprintf(str, "(%d,%d,%d) fps %d", darci->WorldPos.X >> 16, darci->WorldPos.Y >> 16, darci->WorldPos.Z >> 16, ((1000) / tick_tock_unclipped) + 1);

        FONT2D_DrawString(str, 2, 2, 0xffffff, 256);
    }

    // GS_LEVEL_LOST / GS_LEVEL_WON text is handled by GAMEMENU — display code here was commented out in original.

    PANEL_finish();
}

// Empty — beacon system was fully commented out in original.
// uc_orig: overlay_beacons (fallen/Source/overlay.cpp)
void overlay_beacons(void)
{
    // Stub — beacon overlay was cut.
}

// Empty stubs — DAMAGE_TEXT system was behind #ifdef DAMAGE_TEXT (never defined).
// uc_orig: add_damage_value (fallen/Source/overlay.cpp)
void add_damage_value(SWORD x, SWORD y, SWORD z, SLONG value)
{
}

// uc_orig: add_damage_text (fallen/Source/overlay.cpp)
void add_damage_text(SWORD x, SWORD y, SWORD z, CBYTE* text)
{
}

// uc_orig: add_damage_value_thing (fallen/Source/overlay.cpp)
void add_damage_value_thing(Thing* p_thing, SLONG value)
{
}

// Clears the tracked enemy array.
// uc_orig: init_overlay (fallen/Source/overlay.cpp)
void init_overlay(void)
{
    memset((UBYTE*)panel_enemy, 0, sizeof(struct TrackEnemy) * MAX_TRACK);
}
