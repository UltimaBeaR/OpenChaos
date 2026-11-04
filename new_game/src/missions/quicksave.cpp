// In-mission quick save / quick load (OpenChaos addition, feature flag ext_in_mission_saves).
// See docs/topics/in_mission_saves.md for the design; the on-disk format is in
// missions/quicksave_format.h.
//
// uc_orig: MEMORY_quick_save / MEMORY_quick_load (fallen/Source/memory.cpp) — the
// pool-dump idea; this version writes disk records with index-encoded pointers, a validated
// header, and copies into the live pools instead of re-seating the arena.
// uc_orig: save_whole_wad (fallen/Source/memory.cpp) — the per-pool record shape
// (id, count, struct size, byte size) reused here.
// uc_orig: SAVE_ingame / LOAD_ingame (fallen/Source/save.cpp) — the reload-then-overlay
// load model.
//
// Load model: the menu answers "load", game_loop restarts the mission through the existing
// GS_REPLAY path (the level is rebuilt from the same .ucm/.map), and QUICKSAVE_apply_pending()
// then overlays the saved state before the first tick. Save model: a pure read of the frozen
// game state while the pause menu is up.

#include "missions/quicksave.h"
#include "missions/quicksave_format.h"

#include <cstdio>
#include <cstring>
#include <vector>

#include "feature_flags.h"
#include "engine/io/file.h"
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks
#include "engine/console/console.h" // CONSOLE_text_en
#include "engine/graphics/text/menufont.h" // MENUFONT_AltScope, FontInfo
#include "engine/graphics/text/menufont_globals.h" // FontPage, MENUFONT_fadein_x
#include "engine/graphics/pipeline/aeng.h" // AENG_create_dx_prim_points
#include "engine/graphics/lighting/night.h" // NIGHT_generate_walkable_lighting
#include "engine/graphics/lighting/night_globals.h"
#include "engine/audio/mfx.h" // MFX_stop_attached
#include "assets/xlat_str.h" // X_SAVE_GAME, X_LOAD_GAME
#include "assets/formats/elev_globals.h" // ELEV_fname_level
#include "assets/formats/anim_globals.h" // game_chunk, anim_chunk
#include "ui/hud/panel.h" // PANEL_draw_quad, PANEL_new_text_init
#include "ui/frontend/frontend_globals.h" // complete_point, mission_hierarchy, best_found
#include "game/game_globals.h" // form_leave_map
#include "game/input_actions.h" // process_hardware_level_input_for_player, input_actions_clear_ak47_reload_gate
#include "missions/memory.h" // MEM_STATIC, MEM_DYNAMIC, uncache
#include "missions/memory_globals.h" // save_table, MAP_beacon
#include "missions/eway_globals.h"
#include "missions/playcuts_globals.h"
#include "things/core/statedef.h"
#include "things/core/state.h" // StateFunction, GenusFunctions
#include "things/core/thing_globals.h" // slow_mo
#include "things/core/player_globals.h" // player_functions
#include "things/characters/person_globals.h"
#include "things/characters/darci_globals.h" // just_started_falling_off_backwards
#include "things/characters/snipe_globals.h"
#include "things/items/barrel_globals.h"
#include "things/items/special.h" // special_normal
#include "things/vehicles/vehicle.h" // VEH_driving
#include "things/vehicles/chopper.h" // CHOPPER_fn_normal
#include "things/animals/bat.h" // BAT_normal
#include "world_objects/plat.h" // PLAT_process
#include "buildings/prim.h" // calc_prim_normals, fn_anim_prim_normal
#include "buildings/ware_globals.h" // WARE_in
#include "effects/combat/pyro_globals.h" // PYRO_functions
#include "effects/environment/tracks_globals.h"
#include "effects/environment/ribbon_globals.h"
#include "camera/fc_globals.h"
#include "map/supermap_globals.h"

// Defined in things/items/barrel.cpp; not declared in a header (missions/memory.cpp
// declares it locally the same way).
void BARREL_process_normal(Thing* p_barrel);

namespace {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// File name: saves/quicksave_<tag>.sav in the per-user folder.
constexpr const char* QS_FILE_PREFIX = "saves/quicksave_";
constexpr const char* QS_FILE_SUFFIX = ".sav";
constexpr const char* QS_LEVEL_EXTENSION = ".ucm";
constexpr size_t QS_FILE_NAME_BYTES = 128;
// The tag field holds at most this many characters plus a NUL.
constexpr size_t QS_LEVEL_TAG_MAX_CHARS = QS_LEVEL_TAG_BYTES - 1;

// Console message lifetime (ms); a little longer than the CONSOLE_text default (4000)
// because GAME LOADED appears while the screen fades in after the reload.
constexpr SLONG QS_MESSAGE_MS = 5000;

// Literal UI strings. Always English, drawn from the license-clean
// alt atlases (menu: MENUFONT_AltScope, console: CONSOLE_text_en), all caps because the
// replacement menu atlas is caps-only.
constexpr const char* QS_MSG_SAVED = "GAME SAVED";
constexpr const char* QS_MSG_LOADED = "GAME LOADED";
constexpr const char* QS_MSG_LOAD_FAILED = "QUICK SAVE CANNOT BE LOADED";
constexpr const char* QS_REASON_VEHICLE = "CANNOT SAVE WHILE IN A VEHICLE";
constexpr const char* QS_REASON_COMBAT = "CANNOT SAVE DURING A FIGHT";
constexpr const char* QS_REASON_ENEMY = "CANNOT SAVE WHILE AN ENEMY IS ENGAGED";
constexpr const char* QS_REASON_CUTSCENE = "CANNOT SAVE DURING A CUTSCENE";
constexpr const char* QS_REASON_OTHER = "CANNOT SAVE RIGHT NOW";
constexpr SLONG QS_WARN_LINES = 3;
constexpr const char* QS_WARN_OLDER[QS_WARN_LINES] = {
    "THE CAMPAIGN PROGRESS IN THIS QUICK SAVE IS OLDER",
    "THAN YOUR CURRENT PROGRESS. LOADING IT WILL",
    "DISCARD THE PROGRESS MADE SINCE.",
};
constexpr const char* QS_WARN_DIFF[QS_WARN_LINES] = {
    "THE CAMPAIGN PROGRESS IN THIS QUICK SAVE IS",
    "DIFFERENT FROM YOUR CURRENT PROGRESS. LOADING IT",
    "REPLACES YOUR CURRENT PROGRESS WITH THE SAVED ONE.",
};

// Menu body layout, in the virtual 640x480 frame GAMEMENU_draw uses.
constexpr float QS_MENU_FRAME_W = 640.0F;
constexpr float QS_MENU_FRAME_H = 480.0F;
constexpr float QS_MENU_CENTRE_X = QS_MENU_FRAME_W * 0.5F;
// Refusal line: below the deepest item list (6 items, the last at 155 + 6 * 40 = 395,
// ui/menus/gamemenu.cpp GAMEMENU_draw).
constexpr float QS_REASON_Y = 430.0F;
// Dialog body: under the SURE dialog's two items (the last at 155 + 2 * 40 = 235).
constexpr float QS_WARN_Y = 300.0F;
constexpr float QS_WARN_LINE_SPACING = 30.0F;
// Lines wider than this are scaled down to fit the frame (20 px margin each side).
constexpr float QS_TEXT_MAX_WIDTH = QS_MENU_FRAME_W - 2.0F * 20.0F;
// Same alpha as an unselected menu item (gamemenu.cpp) for the refusal line, full for
// the dialog body.
constexpr UBYTE QS_REASON_ALPHA = 128;
constexpr UBYTE QS_WARN_ALPHA = 255;

// GAMEMENU_Menu.word[8]: word[0] is the title, word[1..7] the items
// (ui/menus/gamemenu_globals.h). The hook inserts our items after item 1.
constexpr SLONG QS_MENU_ROW_WORDS = 8;
constexpr SLONG QS_MENU_FIRST_INSERT = 2;

// EWAY_time / EWAY_tick are derived from EWAY_time_accurate every tick
// (EWAY_process, missions/eway.cpp); the restore derives them the same way.
constexpr SLONG QS_EWAY_TIME_SHIFT = 4;
constexpr SLONG QS_EWAY_TICK_SCALE = 5;

// DFacet.Counter has two visit stamps (map/supermap.h), SUPERMAP_counter likewise.
constexpr SLONG QS_SUPERMAP_COUNTERS = 2;

// mission_hierarchy[] bit set when a mission is complete (set in FRONTEND_level_won, read
// in FRONTEND_draw_districts, ui/frontend/frontend.cpp).
constexpr UBYTE QS_MISSION_COMPLETE_BIT = 2;

// CRC32 (IEEE 802.3, reflected).
constexpr uint32_t QS_CRC_POLY = 0xEDB88320u;
constexpr uint32_t QS_CRC_INIT = 0xFFFFFFFFu;

// Saturation bound of the 32-bit wall-clock ages.
constexpr uint64_t QS_AGE_MAX_MS = 0xFFFFFFFFull;

// generic_people_functions[] (things/characters/person_globals.cpp): one entry per
// state STATE_INIT..STATE_FLOAT plus a trailing { 0, NULL }. The header declares the array
// without a bound, so its size has to be restated here and no static_assert can tie the two
// together: a table grown past STATE_FLOAT needs this constant updated, or the new states
// are refused on load.
constexpr uint32_t QS_GENERIC_PERSON_STATE_FNS = STATE_FLOAT + 2;
// Every non-NULL PYRO_functions[] table holds STATE_INIT and STATE_NORMAL only
// (effects/combat/pyro_globals.cpp); PYRO_functions[PYRO_NONE] is NULL.
constexpr uint32_t QS_PYRO_STATE_FNS = STATE_NORMAL + 1;

// Player.PlayerType values a level can hold: EWAY_create_player (missions/eway.cpp) creates
// PLAYER_DARCI..PLAYER_THUG (bankbomb1 starts the player as PLAYER_THUG), and a
// free slot is PLAYER_NONE. The load never indexes player_functions[] with it: a player
// Thing's handler is always process_hardware_level_input_for_player (qs_derive_statefn).
constexpr uint32_t QS_PLAYER_TYPES = PLAYER_THUG + 1;

// Ribbon handles of a PYRO_IMMOLATE: radii[0..1] always, radii[2..4] once it burns out
// (free_pyro, PYRO_fn_init and PYRO_fn_normal, effects/combat/pyro.cpp).
constexpr uint32_t QS_PYRO_IMMOLATE_RIBBONS = 5;
// A PYRO_GAMEOVER keeps one dynamic-light handle per radii[] entry (PYRO_fn_normal,
// effects/combat/pyro.cpp).
constexpr uint32_t QS_PYRO_GAMEOVER_DLIGHTS = 8;

// Body-part tables a DrawTween selects from through its TheChunk: MultiObject[MeshID] and
// PeopleTypes[PersonID & mask] or [PersonID >> 5] (FIGURE_draw,
// engine/graphics/geometry/figure.cpp). The shifted form of a UBYTE always fits.
constexpr uint32_t QS_CHUNK_MESH_SLOTS = sizeof(GameKeyFrameChunk::MultiObject) / sizeof(GameKeyFrameChunk::MultiObject[0]);
constexpr uint32_t QS_PERSON_ID_BODY_MASK = 0x1F;
constexpr uint32_t QS_PERSON_ID_BODY_SHIFT = 5;
static_assert((0xFFu >> QS_PERSON_ID_BODY_SHIFT) < MAX_PEOPLE_TYPES);

// CPChannel.type of a character channel (CT_CHAR, missions/playcuts.cpp). Its index is a
// PERSON_* type plus one: create_person(chan->index - 1, ...) (PLAYCUTS_Play).
constexpr uint8_t QS_CUTSCENE_CHANNEL_CHAR = 1;

// A MemTable.CountW cursor is a UWORD.
constexpr uint32_t QS_MAX_CURSOR_W = 0xFFFF;

// Slots of the pools the cross-record validation reads.
constexpr uint32_t QS_SLOT_THINGS = qs_pool_slot(QS_POOL_THINGS);
constexpr uint32_t QS_SLOT_PEOPLE = qs_pool_slot(QS_POOL_PEOPLE);
constexpr uint32_t QS_SLOT_PYRO = qs_pool_slot(QS_POOL_PYRO);
constexpr uint32_t QS_SLOT_EWAY_WAY = qs_pool_slot(QS_POOL_EWAY_WAY);
constexpr uint32_t QS_SLOT_PRIM_POINTS = qs_pool_slot(QS_POOL_PRIM_POINTS);
constexpr uint32_t QS_SLOT_PRIM_FACES4 = qs_pool_slot(QS_POOL_PRIM_FACES4);
constexpr uint32_t QS_SLOT_CHOPPERS = qs_pool_slot(QS_POOL_CHOPPERS);
constexpr uint32_t QS_SLOT_MAP_BEACON = qs_pool_slot(QS_POOL_MAP_BEACON);
constexpr uint32_t QS_SLOT_CUTSCENE_DATA = qs_pool_slot(QS_POOL_CUTSCENE_DATA);
constexpr uint32_t QS_SLOT_CUTSCENE_TRKS = qs_pool_slot(QS_POOL_CUTSCENE_TRKS);
constexpr uint32_t QS_SLOT_CUTSCENE_PKTS = qs_pool_slot(QS_POOL_CUTSCENE_PKTS);
static_assert(QS_SLOT_THINGS < QS_POOL_COUNT && QS_SLOT_PEOPLE < QS_POOL_COUNT && QS_SLOT_PYRO < QS_POOL_COUNT);
static_assert(QS_SLOT_EWAY_WAY < QS_POOL_COUNT && QS_SLOT_PRIM_POINTS < QS_POOL_COUNT && QS_SLOT_PRIM_FACES4 < QS_POOL_COUNT);
static_assert(QS_SLOT_CHOPPERS < QS_POOL_COUNT && QS_SLOT_MAP_BEACON < QS_POOL_COUNT);
static_assert(QS_SLOT_CUTSCENE_DATA < QS_POOL_COUNT && QS_SLOT_CUTSCENE_TRKS < QS_POOL_COUNT && QS_SLOT_CUTSCENE_PKTS < QS_POOL_COUNT);

// The pools a Thing.Genus pointer can point into.
constexpr uint32_t QS_GENUS_POOLS[] = {
    QS_POOL_VEHICLES, QS_POOL_PEOPLE, QS_POOL_ANIMALS, QS_POOL_CHOPPERS, QS_POOL_PYRO,
    QS_POOL_PLAYERS, QS_POOL_PROJECTILES, QS_POOL_SPECIALS, QS_POOL_SWITCHES, QS_POOL_BATS,
    QS_POOL_BARRELS, QS_POOL_PLATS, QS_POOL_TRACKS
};

// The EWAY camera scalars, in disk order (missions/eway_globals.h).
SLONG* const qs_eway_cam_fields[QS_EWAY_CAM_FIELDS] = {
    &EWAY_cam_active, &EWAY_cam_goinactive, &EWAY_cam_x, &EWAY_cam_y, &EWAY_cam_z,
    &EWAY_cam_dx, &EWAY_cam_dy, &EWAY_cam_dz, &EWAY_cam_yaw, &EWAY_cam_pitch,
    &EWAY_cam_want_yaw, &EWAY_cam_want_pitch, &EWAY_cam_waypoint, &EWAY_cam_target, &EWAY_cam_delay,
    &EWAY_cam_speed, &EWAY_cam_freeze, &EWAY_cam_cant_interrupt, &EWAY_cam_targx, &EWAY_cam_targy,
    &EWAY_cam_targz, &EWAY_cam_lens, &EWAY_cam_warehouse, &EWAY_cam_lock, &EWAY_cam_last_yaw,
    &EWAY_cam_last_x, &EWAY_cam_last_y, &EWAY_cam_last_z, &EWAY_cam_skip, &EWAY_cam_last_dyaw
};

// The SNIPE_* scope scalars, in disk order (things/characters/snipe_globals.h).
SLONG* const qs_snipe_fields[QS_SNIPE_FIELDS] = {
    &SNIPE_yaw, &SNIPE_pitch, &SNIPE_dyaw, &SNIPE_dpitch,
    &SNIPE_dlens, &SNIPE_cam_yaw, &SNIPE_cam_pitch, &SNIPE_cam_lens
};

// The EWAY_fake_wander_text_* values, in disk order (missions/eway_globals.h).
UWORD* const qs_fake_wander_fields[QS_FAKE_WANDER_FIELDS] = {
    &EWAY_fake_wander_text_normal_index, &EWAY_fake_wander_text_normal_number,
    &EWAY_fake_wander_text_guilty_index, &EWAY_fake_wander_text_guilty_number,
    &EWAY_fake_wander_text_annoyed_index, &EWAY_fake_wander_text_annoyed_number
};

using QS_StateFn = void (*)(Thing*);

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------

// A quick load was confirmed; QUICKSAVE_apply_pending() applies it after the reload.
bool s_pending_restore = false;
// The SURE dialog is open on behalf of a quick load (campaign mismatch).
bool s_warn_pending = false;
// What the menu body shows, and the refusal behind QS_BODY_SAVE_REFUSED.
QS_Body s_body = QS_BODY_NONE;
QS_Refusal s_refusal = QS_OK;
// The menu that opened last, as told by the hook.
QS_MenuKind s_menu_kind = QS_MENU_OTHER;
// The pristine pause / level-lost rows, captured the first time the hook sees them,
// so every rewrite starts from the original table content.
UBYTE s_pause_row[QS_MENU_ROW_WORDS];
bool s_pause_row_captured = false;
UBYTE s_lost_row[QS_MENU_ROW_WORDS];
bool s_lost_row_captured = false;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

bool qs_flag_on()
{
    return g_feature_flags.ext_in_mission_saves;
}

uint32_t qs_align_up(uint32_t v)
{
    return (v + QS_SECTION_ALIGN - 1) & ~(QS_SECTION_ALIGN - 1);
}

uint32_t qs_crc32(uint32_t crc, const void* data, size_t bytes)
{
    static uint32_t table[256];
    static bool table_ready = false;
    if (!table_ready) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? (QS_CRC_POLY ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        table_ready = true;
    }
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < bytes; i++) {
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

uint32_t qs_age_ms(uint64_t stamp, uint64_t now)
{
    if (stamp >= now) {
        return 0;
    }
    const uint64_t age = now - stamp;
    return age > QS_AGE_MAX_MS ? uint32_t(QS_AGE_MAX_MS) : uint32_t(age);
}

uint64_t qs_rebase(uint32_t age_ms, uint64_t now)
{
    return (uint64_t(age_ms) >= now) ? 0 : now - age_ms;
}

// Derives the save tag from ELEV_fname_level: the leaf after the last '/' or '\',
// without a trailing ".ucm", lowercase ASCII, every byte outside [a-z0-9_-] replaced
// by '_', truncated. Example: "levels\assault1.ucm" -> "assault1".
void qs_level_tag(char out[QS_LEVEL_TAG_BYTES])
{
    memset(out, 0, QS_LEVEL_TAG_BYTES);
    const char* leaf = ELEV_fname_level;
    for (const char* p = ELEV_fname_level; *p; p++) {
        if (*p == '/' || *p == '\\') {
            leaf = p + 1;
        }
    }
    size_t len = strlen(leaf);
    const size_t ext_len = strlen(QS_LEVEL_EXTENSION);
    if (len >= ext_len) {
        bool has_ext = true;
        for (size_t i = 0; i < ext_len; i++) {
            char c = leaf[len - ext_len + i];
            if (c >= 'A' && c <= 'Z') {
                c = char(c - 'A' + 'a');
            }
            if (c != QS_LEVEL_EXTENSION[i]) {
                has_ext = false;
                break;
            }
        }
        if (has_ext) {
            len -= ext_len;
        }
    }
    if (len > QS_LEVEL_TAG_MAX_CHARS) {
        len = QS_LEVEL_TAG_MAX_CHARS;
    }
    for (size_t i = 0; i < len; i++) {
        char c = leaf[i];
        if (c >= 'A' && c <= 'Z') {
            c = char(c - 'A' + 'a');
        }
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        out[i] = ok ? c : '_';
    }
}

// Builds saves/quicksave_<tag>.sav; false when no level is loaded.
bool qs_file_name(char out[QS_FILE_NAME_BYTES])
{
    char tag[QS_LEVEL_TAG_BYTES];
    qs_level_tag(tag);
    if (!tag[0]) {
        return false;
    }
    snprintf(out, QS_FILE_NAME_BYTES, "%s%s%s", QS_FILE_PREFIX, tag, QS_FILE_SUFFIX);
    return true;
}

bool qs_exists_for_level()
{
    char fn[QS_FILE_NAME_BYTES];
    return qs_file_name(fn) && FileExists(fn);
}

// save_table[] must have the shape this format was written for; a changed table also
// changes the fingerprint, but the literal pool ids would be wrong first.
bool qs_save_table_ok()
{
    uint32_t n = 0;
    while (save_table[n].Point) {
        n++;
    }
    if (n != QS_SAVE_TABLE_ENTRIES) {
        fprintf(stderr, "QUICKSAVE: save_table has %u entries, the format expects %u\n", n, QS_SAVE_TABLE_ENTRIES);
        return false;
    }
    return true;
}

// CRC32 over (Name, StructSize, Maximum) of every save_table[] entry, read after
// init_memory's Maximum overrides (missions/memory.cpp) — i.e. while a level is loaded.
uint32_t qs_table_fingerprint()
{
    uint32_t crc = QS_CRC_INIT;
    for (uint32_t i = 0; save_table[i].Point; i++) {
        const MemTable& t = save_table[i];
        const char* name = t.Name ? t.Name : "";
        crc = qs_crc32(crc, name, strlen(name));
        uint8_t sizes[6];
        qs_put_u16(sizes, uint16_t(t.StructSize));
        qs_put_u32(sizes + 2, uint32_t(t.Maximum));
        crc = qs_crc32(crc, sizes, sizeof(sizes));
    }
    return ~crc;
}

// Base address of a save_table[] pool. Entries 65-68 (cutscene_data/_trks/_pkts/_text)
// name static arrays, not pool pointers: the table stores the array's own address cast
// to void** (missions/memory_globals.cpp), so for them the "pool pointer" is the
// array itself and *Point is the array's first bytes. An existing defect, worked around here.
uint8_t* qs_pool_base(uint32_t id)
{
    void** point = save_table[id].Point;
    if (point == (void**)PLAYCUTS_cutscenes || point == (void**)PLAYCUTS_tracks
        || point == (void**)PLAYCUTS_packets || point == (void**)PLAYCUTS_text_data) {
        return (uint8_t*)point;
    }
    return (uint8_t*)*point;
}

bool qs_pool_has_cursor(const MemTable& t)
{
    return t.CountL != nullptr || t.CountW != nullptr;
}

SLONG qs_pool_cursor(const MemTable& t)
{
    if (t.CountL) {
        return *t.CountL;
    }
    if (t.CountW) {
        return *t.CountW;
    }
    return 0;
}

void qs_set_pool_cursor(MemTable& t, SLONG v)
{
    if (t.CountL) {
        *t.CountL = v;
    } else if (t.CountW) {
        *t.CountW = UWORD(v);
    }
}

// Element index of `p` inside pool `id`, if `p` points exactly at one of its elements.
bool qs_find_element(uint32_t id, const void* p, uint16_t* index)
{
    const MemTable& t = save_table[id];
    const uintptr_t base = (uintptr_t)qs_pool_base(id);
    const uintptr_t a = (uintptr_t)p;
    const uintptr_t size = uintptr_t(t.StructSize);
    if (a < base || size == 0) {
        return false;
    }
    const uintptr_t off = a - base;
    if (off % size) {
        return false;
    }
    const uintptr_t idx = off / size;
    if (idx >= uintptr_t(t.Maximum) || idx > 0xFFFF) {
        return false;
    }
    *index = uint16_t(idx);
    return true;
}

void* qs_element(uint32_t id, uint16_t index)
{
    return qs_pool_base(id) + size_t(index) * size_t(save_table[id].StructSize);
}

// Thing reference: THING_INDEX, 0 = NULL (slot 0 is never allocated: init_things, things/core/thing.cpp).
uint16_t qs_thing_ref(const Thing* p)
{
    if (!p) {
        return 0;
    }
    uint16_t idx = 0;
    if (!qs_find_element(QS_POOL_THINGS, p, &idx)) {
        fprintf(stderr, "QUICKSAVE: a Thing pointer outside the Thing pool was written as NULL\n");
        return 0;
    }
    return idx;
}

bool qs_thing_ref_ok(uint16_t idx)
{
    return idx < MAX_THINGS;
}

Thing* qs_thing_ptr(uint16_t idx)
{
    return idx ? TO_THING(idx) : nullptr;
}

// EWAY_mess_buffer byte offset, QS_NULL_OFFSET = NULL.
uint32_t qs_mess_offset(const CBYTE* p)
{
    if (!p) {
        return QS_NULL_OFFSET;
    }
    const uintptr_t base = (uintptr_t)EWAY_mess_buffer;
    const uintptr_t a = (uintptr_t)p;
    const uintptr_t size = uintptr_t(save_table[QS_POOL_EWAY_MESS_BUF].Maximum);
    if (a < base || a - base >= size) {
        return QS_NULL_OFFSET;
    }
    return uint32_t(a - base);
}

bool qs_mess_offset_ok(uint32_t off)
{
    return off == QS_NULL_OFFSET || off < uint32_t(save_table[QS_POOL_EWAY_MESS_BUF].Maximum);
}

CBYTE* qs_mess_ptr(uint32_t off)
{
    return off == QS_NULL_OFFSET ? nullptr : EWAY_mess_buffer + off;
}

// --- Animation frames and chunks ---

bool qs_encode_frame(const GameKeyFrame* f, uint32_t* ref)
{
    *ref = 0;
    if (!f) {
        return true;
    }
    const uintptr_t a = (uintptr_t)f;
    for (uint32_t kind = QS_CHUNK_KIND_GAME; kind <= QS_CHUNK_KIND_ANIM; kind++) {
        const GameKeyFrameChunk* chunks = (kind == QS_CHUNK_KIND_GAME) ? game_chunk : anim_chunk;
        const uint32_t n = (kind == QS_CHUNK_KIND_GAME) ? MAX_GAME_CHUNKS : MAX_ANIM_CHUNKS;
        for (uint32_t slot = 0; slot < n; slot++) {
            const GameKeyFrameChunk& c = chunks[slot];
            if (!c.AnimKeyFrames || c.MaxKeyFrames <= 0) {
                continue;
            }
            const uintptr_t base = (uintptr_t)c.AnimKeyFrames;
            const uintptr_t end = base + uintptr_t(c.MaxKeyFrames) * sizeof(GameKeyFrame);
            if (a >= base && a < end && (a - base) % sizeof(GameKeyFrame) == 0) {
                const uint32_t index = uint32_t((a - base) / sizeof(GameKeyFrame));
                *ref = (kind << QS_FRAME_KIND_SHIFT) | (slot << QS_FRAME_SLOT_SHIFT) | index;
                return true;
            }
        }
    }
    return false;
}

bool qs_decode_frame(uint32_t ref, GameKeyFrame** out)
{
    *out = nullptr;
    if (ref == 0) {
        return true;
    }
    const uint32_t kind = ref >> QS_FRAME_KIND_SHIFT;
    const uint32_t slot = (ref >> QS_FRAME_SLOT_SHIFT) & QS_FRAME_SLOT_MASK;
    const uint32_t index = ref & QS_FRAME_INDEX_MASK;
    const GameKeyFrameChunk* c = nullptr;
    if (kind == QS_CHUNK_KIND_GAME && slot < MAX_GAME_CHUNKS) {
        c = &game_chunk[slot];
    } else if (kind == QS_CHUNK_KIND_ANIM && slot < MAX_ANIM_CHUNKS) {
        c = &anim_chunk[slot];
    }
    if (!c || !c->AnimKeyFrames || SLONG(index) >= SLONG(c->MaxKeyFrames)) {
        return false;
    }
    *out = &c->AnimKeyFrames[index];
    return true;
}

bool qs_encode_chunk(const GameKeyFrameChunk* c, uint32_t* ref)
{
    *ref = 0;
    if (!c) {
        return true;
    }
    if (c >= game_chunk && c < game_chunk + MAX_GAME_CHUNKS) {
        *ref = (QS_CHUNK_KIND_GAME << QS_CHUNK_KIND_SHIFT) | uint32_t(c - game_chunk);
        return true;
    }
    if (c >= anim_chunk && c < anim_chunk + MAX_ANIM_CHUNKS) {
        *ref = (QS_CHUNK_KIND_ANIM << QS_CHUNK_KIND_SHIFT) | uint32_t(c - anim_chunk);
        return true;
    }
    return false;
}

bool qs_decode_chunk(uint32_t ref, GameKeyFrameChunk** out)
{
    *out = nullptr;
    if (ref == 0) {
        return true;
    }
    const uint32_t kind = ref >> QS_CHUNK_KIND_SHIFT;
    const uint32_t slot = ref & QS_CHUNK_SLOT_MASK;
    if (kind == QS_CHUNK_KIND_GAME && slot < MAX_GAME_CHUNKS) {
        *out = &game_chunk[slot];
        return true;
    }
    if (kind == QS_CHUNK_KIND_ANIM && slot < MAX_ANIM_CHUNKS) {
        *out = &anim_chunk[slot];
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// StateFn derivation. The handler is re-derived from Class/State, as the original
// loader did (convert_thing_to_pointer, missions/memory.cpp), and then cleared when
// the saved "was set" bit is 0.
// ---------------------------------------------------------------------------

QS_StateFn qs_derive_statefn(const Thing* t)
{
    switch (t->Class) {
    case CLASS_PLAYER:
        return process_hardware_level_input_for_player;
    case CLASS_PERSON:
        // A person is created in STATE_INIT with its type's init handler
        // (set_state_function in alloc_person, things/characters/person.cpp); every later state
        // change goes through set_generic_person_state_function.
        if (t->State == STATE_INIT && t->Genus.Person && t->Genus.Person->PersonType < PERSON_NUM_TYPES) {
            return people_functions[t->Genus.Person->PersonType].StateFunctions[STATE_INIT].StateFn;
        }
        // qs_check_thing_tables() refuses a file with a State past the table; the guard
        // keeps this function safe on its own.
        return t->State < QS_GENERIC_PERSON_STATE_FNS ? generic_people_functions[t->State].StateFn : nullptr;
    case CLASS_VEHICLE:
        // Not set_state_function: a parked car keeps State 6 with StateFn NULL and a
        // wreck's State 17 is past the 8-entry table (VEH_driving, things/vehicles/vehicle.cpp).
        return VEH_driving;
    case CLASS_SPECIAL:
        return special_normal;
    case CLASS_CHOPPER:
        return CHOPPER_fn_normal;
    case CLASS_PYRO:
        if (t->Genus.Pyro && t->Genus.Pyro->PyroType < PYRO_RANGE && t->State < QS_PYRO_STATE_FNS) {
            const StateFunction* fns = PYRO_functions[t->Genus.Pyro->PyroType].StateFunctions;
            return fns ? fns[t->State].StateFn : nullptr;
        }
        return nullptr;
    case CLASS_PLAT:
        return PLAT_process;
    case CLASS_BARREL:
        return BARREL_process_normal;
    case CLASS_BAT:
        return BAT_normal;
    case CLASS_ANIM_PRIM:
        // The case the port's convert_thing_to_pointer lacks (missions/memory.cpp).
        return fn_anim_prim_normal;
    case CLASS_BIKE:
        // No Bike type exists in the port; nothing to re-arm.
        return nullptr;
    default:
        // NONE, CAMERA, PROJECTILE, BUILDING, FURNITURE, SWITCH, TRACK and the dead
        // ANIMAL class never carry a handler.
        return nullptr;
    }
}

// ---------------------------------------------------------------------------
// The encoder. Builds the whole file in memory (header first, filled last), so the
// writer issues one FileWrite and the Debug verifier can re-encode the live state
// into a second buffer and compare.
// ---------------------------------------------------------------------------

struct QS_Section {
    const char* name;
    uint32_t offset; // from the start of the file
    uint32_t bytes;
    uint32_t element_bytes; // 0 for a fixed record
};

struct QS_Encoder {
    std::vector<uint8_t> buf;
    std::vector<QS_Section> sections;

    // Appends `n` zero bytes and returns their offset.
    uint32_t grow(uint32_t n)
    {
        const uint32_t at = uint32_t(buf.size());
        buf.resize(size_t(at) + n, 0);
        return at;
    }
    void align()
    {
        buf.resize(qs_align_up(uint32_t(buf.size())), 0);
    }
    void put(const void* data, uint32_t n)
    {
        const uint32_t at = grow(n);
        memcpy(buf.data() + at, data, n);
    }
};

void qs_put_record(QS_Encoder& enc, const char* name, uint32_t magic, const void* rec, uint32_t bytes)
{
    uint8_t hdr[sizeof(QS_RecordHeader)];
    qs_put_u32(hdr, magic);
    qs_put_u32(hdr + 4, bytes);
    enc.put(hdr, sizeof(hdr));
    enc.sections.push_back({ name, uint32_t(enc.buf.size()), bytes, 0 });
    enc.put(rec, bytes);
    enc.align();
}

void qs_capture_campaign(QS_Campaign* c)
{
    memset(c, 0, sizeof(*c));
    c->complete_point = complete_point;
    c->version = QS_CAMPAIGN_VERSION;
    c->darci[0] = the_game.DarciStrength;
    c->darci[1] = the_game.DarciConstitution;
    c->darci[2] = the_game.DarciSkill;
    c->darci[3] = the_game.DarciStamina;
    c->roper[0] = the_game.RoperStrength;
    c->roper[1] = the_game.RoperConstitution;
    c->roper[2] = the_game.RoperSkill;
    c->roper[3] = the_game.RoperStamina;
    c->darci_dead_civ_warnings = the_game.DarciDeadCivWarnings;
    memcpy(c->mission_hierarchy, mission_hierarchy, QS_MISSION_HIERARCHY_BYTES);
    memcpy(c->best_found, &best_found[0][0], QS_BEST_FOUND_MISSIONS * QS_BEST_FOUND_SLOTS);
}

// Mirror of FRONTEND_load_savegame (ui/frontend/frontend.cpp), same version gates.
void qs_apply_campaign(const QS_Campaign& c)
{
    complete_point = c.complete_point;
    if (c.version) {
        the_game.DarciStrength = c.darci[0];
        the_game.DarciConstitution = c.darci[1];
        the_game.DarciSkill = c.darci[2];
        the_game.DarciStamina = c.darci[3];
        the_game.RoperStrength = c.roper[0];
        the_game.RoperConstitution = c.roper[1];
        the_game.RoperSkill = c.roper[2];
        the_game.RoperStamina = c.roper[3];
        the_game.DarciDeadCivWarnings = c.darci_dead_civ_warnings;
    }
    if (c.version > 1) {
        memcpy(mission_hierarchy, c.mission_hierarchy, QS_MISSION_HIERARCHY_BYTES);
    }
    if (c.version > 2) {
        memcpy(&best_found[0][0], c.best_found, QS_BEST_FOUND_MISSIONS * QS_BEST_FOUND_SLOTS);
    }
}

// Progress rank of a campaign payload, compared component by component.
struct QS_CampaignRank {
    uint32_t part[4];
};

QS_CampaignRank qs_campaign_rank(const QS_Campaign& c)
{
    QS_CampaignRank r = {};
    r.part[0] = c.complete_point;
    for (uint32_t i = 0; i < QS_MISSION_HIERARCHY_BYTES; i++) {
        if (c.mission_hierarchy[i] & QS_MISSION_COMPLETE_BIT) {
            r.part[1]++;
        }
    }
    for (uint32_t i = 0; i < QS_BEST_FOUND_MISSIONS * QS_BEST_FOUND_SLOTS; i++) {
        r.part[2] += c.best_found[i];
    }
    for (uint32_t i = 0; i < QS_CHARACTER_STATS; i++) {
        r.part[3] += uint32_t(c.darci[i]) + c.roper[i];
    }
    r.part[3] += c.darci_dead_civ_warnings;
    return r;
}

QS_CampaignMatch qs_campaign_compare(const QS_Campaign& saved)
{
    QS_Campaign current;
    qs_capture_campaign(&current);
    if (memcmp(&current, &saved, sizeof(saved)) == 0) {
        return QS_CAMPAIGN_SAME;
    }
    const QS_CampaignRank rs = qs_campaign_rank(saved);
    const QS_CampaignRank rc = qs_campaign_rank(current);
    bool all_le = true;
    bool any_lt = false;
    for (int i = 0; i < 4; i++) {
        if (rs.part[i] > rc.part[i]) {
            all_le = false;
        }
        if (rs.part[i] < rc.part[i]) {
            any_lt = true;
        }
    }
    return (all_le && any_lt) ? QS_CAMPAIGN_SAVED_OLDER : QS_CAMPAIGN_DIFFERENT;
}

void qs_encode_scalars(QS_Scalars* s, uint64_t now)
{
    memset(s, 0, sizeof(*s));

    s->game_state = uint32_t(GAME_STATE);
    s->game_flags = uint32_t(GAME_FLAGS);
    s->person_count = PERSON_COUNT;
    s->player_count = PLAYER_COUNT;
    s->animal_count = ANIMAL_COUNT;
    s->chopper_count = CHOPPER_COUNT;
    s->pyro_count = PYRO_COUNT;
    s->projectile_count = PROJECTILE_COUNT;
    s->special_count = SPECIAL_COUNT;
    s->switch_count = SWITCH_COUNT;
    s->primary_thing_count = PRIMARY_COUNT;
    s->secondary_thing_count = SECONDARY_COUNT;
    s->used_primary = PRIMARY_USED;
    s->unused_primary = PRIMARY_UNUSED;
    s->used_secondary = SECONDARY_USED;
    s->unused_secondary = SECONDARY_UNUSED;

    s->barrel_sphere_last = BARREL_sphere_last;
    s->track_head = track_head;
    s->track_tail = track_tail;
    s->track_eob = track_eob;
    s->night_dlight_free = NIGHT_dlight_free;
    s->night_dlight_used = NIGHT_dlight_used;
    s->first_walkable_prim_point = first_walkable_prim_point;
    s->number_of_walkable_prim_points = number_of_walkable_prim_points;
    s->first_walkable_prim_face4 = first_walkable_prim_face4;
    s->number_of_walkable_prim_faces4 = number_of_walkable_prim_faces4;
    s->night_amb_colour = NIGHT_amb_colour;
    s->night_amb_specular = NIGHT_amb_specular;
    s->night_amb_red = NIGHT_amb_red;
    s->night_amb_green = NIGHT_amb_green;
    s->night_amb_blue = NIGHT_amb_blue;
    s->night_amb_norm_x = NIGHT_amb_norm_x;
    s->night_amb_norm_y = NIGHT_amb_norm_y;
    s->night_amb_norm_z = NIGHT_amb_norm_z;
    s->night_flag = NIGHT_flag;
    s->night_lampost_radius = NIGHT_lampost_radius;
    s->night_lampost_red = NIGHT_lampost_red;
    s->night_lampost_green = NIGHT_lampost_green;
    s->night_lampost_blue = NIGHT_lampost_blue;
    s->night_sky_red = NIGHT_sky_colour.red;
    s->night_sky_green = NIGHT_sky_colour.green;
    s->night_sky_blue = NIGHT_sky_colour.blue;

    s->crime_rate = CRIME_RATE;
    s->crime_rate_score_mul = CRIME_RATE_SCORE_MUL;
    s->texture_set = TEXTURE_SET;
    s->music_world = MUSIC_WORLD;
    s->boredom_rate = BOREDOM_RATE;
    s->indoors_room = INDOORS_ROOM;
    s->indoors_index_fade = INDOORS_INDEX_FADE;
    s->slow_mo = slow_mo;
    s->ware_in = WARE_in;

    s->eway_time_accurate = EWAY_time_accurate;
    s->eway_count_up = EWAY_count_up;
    s->eway_count_up_visible = EWAY_count_up_visible;
    s->eway_count_up_add_penalties = EWAY_count_up_add_penalties;
    s->eway_count_up_num_penalties = EWAY_count_up_num_penalties;
    s->eway_count_up_penalty_timer = EWAY_count_up_penalty_timer;

    s->game_cut_scene = GAME_cut_scene;
    s->eway_darci_move = EWAY_darci_move;
    s->eway_used_thing = EWAY_used_thing;
    s->eway_tutorial_counter = EWAY_tutorial_counter;
    s->eway_tutorial_string_offset = qs_mess_offset(EWAY_tutorial_string);
    s->eway_conv_active = EWAY_conv_active;
    s->eway_conv_waypoint = EWAY_conv_waypoint;
    s->eway_conv_person_a = EWAY_conv_person_a;
    s->eway_conv_person_b = EWAY_conv_person_b;
    s->eway_conv_str = EWAY_conv_str;
    s->eway_conv_str_count = EWAY_conv_str_count;
    s->eway_conv_timer = EWAY_conv_timer;
    s->eway_conv_skip = EWAY_conv_skip;
    s->eway_conv_ambient = EWAY_conv_ambient;
    s->eway_conv_talk = EWAY_conv_talk;
    s->talk_thing = qs_thing_ref(talk_thing);
    for (uint32_t i = 0; i < QS_FAKE_WANDER_FIELDS; i++) {
        s->eway_fake_wander[i] = *qs_fake_wander_fields[i];
    }
    s->eway_cam_jumped = EWAY_cam_jumped;
    for (uint32_t i = 0; i < QS_EWAY_CAM_FIELDS; i++) {
        s->eway_cam[i] = *qs_eway_cam_fields[i];
    }
    s->eway_cam_thing = EWAY_cam_thing;

    s->playcuts_playing = UBYTE(PLAYCUTS_playing);
    s->no_more_packets = no_more_packets;
    s->playcuts_slomo = UBYTE(PLAYCUTS_slomo);
    s->playcuts_slomo_ctr = PLAYCUTS_slomo_ctr;
    s->playcuts_fade_level = PLAYCUTS_fade_level;

    for (uint32_t i = 0; i < MAX_PLAYERS; i++) {
        s->net_person[i] = qs_thing_ref(NET_PERSON(i));
        s->net_player[i] = qs_thing_ref(NET_PLAYER(i));
    }

    s->stat_killed_thug = stat_killed_thug;
    s->stat_killed_innocent = stat_killed_innocent;
    s->stat_arrested_thug = stat_arrested_thug;
    s->stat_arrested_innocent = stat_arrested_innocent;
    s->stat_count_bonus = stat_count_bonus;
    // stat_game_time itself is only refreshed at the end of a mission (set_stats,
    // things/characters/person.cpp); the live span is now - stat_start_time.
    s->stat_game_time_ms = qs_age_ms(stat_start_time, now);

    for (uint32_t p = 0; p < MAX_PLAYERS; p++) {
        const Player* pl = TO_PLAYER(p);
        for (uint32_t b = 0; b < QS_PLAYER_BUTTONS; b++) {
            s->player_last_released_age_ms[p][b] = qs_age_ms(pl->LastReleased[b], now);
        }
    }

    for (uint32_t i = 0; i < MAX_THINGS; i++) {
        const Thing* t = TO_THING(i);
        if (t->Class != CLASS_NONE && t->StateFn) {
            s->statefn_present[i >> 3] |= UBYTE(1u << (i & 7));
        }
    }

    s->global_person = global_person;
    s->timer_bored = timer_bored;
    s->combo_display = combo_display;
    s->kick_or_punch = kick_or_punch;
    s->just_started_falling_off_backwards = just_started_falling_off_backwards;
}

void qs_encode_pcom(QS_Pcom* p)
{
    memset(p, 0, sizeof(*p));
    p->gang_person_upto = PCOM_gang_person_upto;
    for (uint32_t i = 0; i < PCOM_MAX_GANG_PEOPLE; i++) {
        p->gang_person[i] = PCOM_gang_person[i];
    }
    for (uint32_t i = 0; i < PCOM_MAX_GANGS; i++) {
        p->gang_index[i] = PCOM_gang[i].index;
        p->gang_number[i] = PCOM_gang[i].number;
    }
    for (uint32_t i = 0; i < PCOM_SUMMON_NUM_BODIES; i++) {
        p->summon[i] = PCOM_summon[i];
    }
    for (uint32_t i = 0; i < BAT_SUMMON_NUM_BODIES; i++) {
        p->bat_summon[i] = BAT_summon[i];
    }
    p->runover_scary_person = qs_thing_ref(PCOM_runover_scary_person);
}

void qs_encode_camera(QS_Camera* c)
{
    memset(c, 0, sizeof(*c));
    FC_Cam cam;
    memcpy(&cam, &FC_cam[QS_LIVE_CAMERA], sizeof(cam));
    const uint16_t focus = qs_thing_ref(cam.focus);
    const uint16_t track = qs_thing_ref(cam.fight_track_focus);
    cam.focus = nullptr;
    cam.fight_track_focus = nullptr;
    cam.fight_behind_until = 0;
    cam.last_manual_cam_turn = 0;
    cam.fight_auto_next = 0;
    memcpy(c->fc_cam.bytes, &cam, sizeof(cam));
    qs_put_u16(c->fc_cam.bytes + offsetof(FC_Cam, focus), focus);
    qs_put_u16(c->fc_cam.bytes + offsetof(FC_Cam, fight_track_focus), track);
    for (uint32_t i = 0; i < QS_SNIPE_FIELDS; i++) {
        c->snipe[i] = *qs_snipe_fields[i];
    }
}

void qs_encode_grenades(QS_Grenades* g)
{
    memset(g, 0, sizeof(*g));
    for (uint32_t i = 0; i < MAX_GRENADES; i++) {
        const Grenade& src = GrenadeArray[i];
        QS_Grenade_Disk& d = g->grenade[i];
        d.owner = qs_thing_ref(src.owner);
        d.x = src.x;
        d.y = src.y;
        d.z = src.z;
        d.yaw = src.yaw;
        d.pitch = src.pitch;
        d.dx = src.dx;
        d.dy = src.dy;
        d.dz = src.dz;
        d.dyaw = src.dyaw;
        d.dpitch = src.dpitch;
        d.timer = src.timer;
        d.rsvd = src.rsvd;
    }
    g->global_g = (global_g >= GrenadeArray && global_g < GrenadeArray + MAX_GRENADES)
        ? uint16_t(global_g - GrenadeArray + 1)
        : 0;
}

void qs_encode_hook(QS_Hook* h)
{
    memset(h, 0, sizeof(*h));
    memcpy(h->points, HOOK_point, sizeof(h->points));
    h->reeled = HOOK_reeled;
    h->state = HOOK_state;
    h->grapple_yaw = HOOK_grapple_yaw;
    h->grapple_pitch = HOOK_grapple_pitch;
    h->spin_x = HOOK_spin_x;
    h->spin_y = HOOK_spin_y;
    h->spin_z = HOOK_spin_z;
    h->spin_speed = HOOK_spin_speed;
    h->countdown = HOOK_countdown;
}

// --- Pointer-bearing pool elements ---

bool qs_encode_drawtween(const DrawTween& t, QS_DrawTween_Disk* d)
{
    memset(d, 0, sizeof(*d));
    d->tweak_speed = t.TweakSpeed;
    d->locked = t.Locked;
    d->frame_index = t.FrameIndex;
    d->queued_frame_index = t.QueuedFrameIndex;
    d->angle = t.Angle;
    d->angle_to = t.AngleTo;
    d->roll = t.Roll;
    d->droll = t.DRoll;
    d->tilt = t.Tilt;
    d->tilt_to = t.TiltTo;
    d->current_anim = t.CurrentAnim;
    d->anim_tween = t.AnimTween;
    d->tween_stage = t.TweenStage;
    d->flags = t.Flags;
    d->drawn = t.Drawn;
    d->mesh_id = t.MeshID;
    d->person_id = t.PersonID;
    // A live frame without an owning chunk would come back as NULL: refuse the save
    // instead of losing it.
    if (!qs_encode_frame(t.CurrentFrame, &d->current_frame) || !qs_encode_frame(t.NextFrame, &d->next_frame)
        || !qs_encode_frame(t.InterruptFrame, &d->interrupt_frame) || !qs_encode_frame(t.QueuedFrame, &d->queued_frame)) {
        fprintf(stderr, "QUICKSAVE: an animation frame has no owning chunk; save aborted\n");
        return false;
    }
    if (!qs_encode_chunk(t.TheChunk, &d->the_chunk)) {
        fprintf(stderr, "QUICKSAVE: DrawTween.TheChunk is not a known chunk; save aborted\n");
        return false;
    }
    return true;
}

bool qs_decode_drawtween(const QS_DrawTween_Disk& d, DrawTween* t)
{
    GameKeyFrame* frames[4];
    GameKeyFrameChunk* chunk;
    if (!qs_decode_frame(d.current_frame, &frames[0]) || !qs_decode_frame(d.next_frame, &frames[1])
        || !qs_decode_frame(d.interrupt_frame, &frames[2]) || !qs_decode_frame(d.queued_frame, &frames[3])
        || !qs_decode_chunk(d.the_chunk, &chunk)) {
        return false;
    }
    if (chunk && (d.mesh_id >= QS_CHUNK_MESH_SLOTS || (d.person_id & QS_PERSON_ID_BODY_MASK) >= MAX_PEOPLE_TYPES)) {
        return false;
    }
    if (!t) {
        return true;
    }
    memset(t, 0, sizeof(*t));
    t->TweakSpeed = d.tweak_speed;
    t->Locked = d.locked;
    t->FrameIndex = d.frame_index;
    t->QueuedFrameIndex = d.queued_frame_index;
    t->Angle = d.angle;
    t->AngleTo = d.angle_to;
    t->Roll = d.roll;
    t->DRoll = d.droll;
    t->Tilt = d.tilt;
    t->TiltTo = d.tilt_to;
    t->CurrentAnim = d.current_anim;
    t->AnimTween = d.anim_tween;
    t->TweenStage = d.tween_stage;
    t->CurrentFrame = frames[0];
    t->NextFrame = frames[1];
    t->InterruptFrame = frames[2];
    t->QueuedFrame = frames[3];
    t->TheChunk = chunk;
    t->Flags = d.flags;
    t->Drawn = d.drawn;
    t->MeshID = d.mesh_id;
    t->PersonID = d.person_id;
    return true;
}

bool qs_encode_thing(const Thing& t, uint32_t index, QS_Thing_Disk* d)
{
    memset(d, 0, sizeof(*d));
    d->class_ = t.Class;
    d->state = t.State;
    d->old_state = t.OldState;
    d->sub_state = t.SubState;
    d->flags = t.Flags & ~ULONG(FLAGS_HAS_ATTACHED_SOUND);
    d->child = t.Child;
    d->parent = t.Parent;
    d->link_child = t.LinkChild;
    d->link_parent = t.LinkParent;
    d->x = t.WorldPos.X;
    d->y = t.WorldPos.Y;
    d->z = t.WorldPos.Z;
    d->draw_type = t.DrawType;
    d->lead = t.Lead;
    d->velocity = t.Velocity;
    d->delta_velocity = t.DeltaVelocity;
    d->required_velocity = t.RequiredVelocity;
    d->dy = t.DY;
    d->index = t.Index;
    // GRAB_FLOOR / GRAB_SEWERS mean "on the ground" and are past prim_faces4[]: the game turns
    // them into 0 where it checks (plant_feet, engine/physics/collide.cpp; grab_ledge,
    // things/characters/person.cpp), but a landing path stores the raw result (fn_person_jumping).
    // Written as 0 so that the reader can refuse every other out-of-range value.
    d->on_face = (t.OnFace == GRAB_FLOOR || t.OnFace == GRAB_SEWERS) ? 0 : t.OnFace;
    d->next_link = t.NextLink;
    d->dog_poo1 = t.DogPoo1;
    d->dog_poo2 = t.DogPoo2;

    // Draw: by address, so the encoding does not depend on DrawType being in sync.
    // A free slot (CLASS_NONE) may hold a stale pointer; anything outside both pools is
    // written as NULL there and refused for a live Thing.
    const void* draw = (const void*)t.Draw.Tweened;
    if (draw) {
        uint16_t idx = 0;
        if (qs_find_element(QS_POOL_DRAWTWEENS, draw, &idx)) {
            d->draw_kind = QS_DRAW_TWEEN;
            d->draw_index = idx;
        } else if (qs_find_element(QS_POOL_DRAWMESHES, draw, &idx)) {
            d->draw_kind = QS_DRAW_MESH;
            d->draw_index = idx;
        } else if (t.Class != CLASS_NONE) {
            fprintf(stderr, "QUICKSAVE: Thing %u (class %d) has a Draw pointer outside the draw pools; save aborted\n", index, t.Class);
            return false;
        }
    }

    const void* genus = (const void*)t.Genus.Vehicle;
    if (genus) {
        bool found = false;
        for (uint32_t pool : QS_GENUS_POOLS) {
            uint16_t idx = 0;
            if (qs_find_element(pool, genus, &idx)) {
                d->genus_kind = uint16_t(pool + 1);
                d->genus_index = idx;
                found = true;
                break;
            }
        }
        // Furniture is allocated outside the arena and never populated.
        if (!found && t.Class != CLASS_NONE && t.Class != CLASS_FURNITURE) {
            fprintf(stderr, "QUICKSAVE: Thing %u (class %d) has a Genus pointer outside the pools; save aborted\n", index, t.Class);
            return false;
        }
    }
    return true;
}

bool qs_genus_pool_ok(uint16_t kind)
{
    if (kind == QS_GENUS_NONE) {
        return true;
    }
    for (uint32_t pool : QS_GENUS_POOLS) {
        if (pool + 1 == kind) {
            return true;
        }
    }
    return false;
}

// Validates (t == NULL) or applies a Thing record. StateFn is left NULL here; the
// StateFn pass runs after every pool is in place (it reads Genus).
bool qs_decode_thing(const QS_Thing_Disk& d, Thing* t)
{
    // thing_class_head[Class] (free_primary_thing, things/core/thing.cpp) and the Thing-index links the
    // list walks follow, including our own qs_allowed_now().
    if (d.class_ >= CLASS_END || !qs_thing_ref_ok(d.child) || !qs_thing_ref_ok(d.parent)
        || !qs_thing_ref_ok(d.link_child) || !qs_thing_ref_ok(d.link_parent) || !qs_thing_ref_ok(d.next_link)) {
        return false;
    }
    if (d.draw_kind == QS_DRAW_TWEEN) {
        if (d.draw_index >= save_table[QS_POOL_DRAWTWEENS].Maximum) {
            return false;
        }
    } else if (d.draw_kind == QS_DRAW_MESH) {
        if (d.draw_index >= save_table[QS_POOL_DRAWMESHES].Maximum) {
            return false;
        }
    } else if (d.draw_kind != QS_DRAW_NONE) {
        return false;
    }
    if (!qs_genus_pool_ok(d.genus_kind)) {
        return false;
    }
    if (d.genus_kind != QS_GENUS_NONE && d.genus_index >= save_table[d.genus_kind - 1].Maximum) {
        return false;
    }
    if (!t) {
        return true;
    }
    memset(t, 0, sizeof(*t));
    t->Class = d.class_;
    t->State = d.state;
    t->OldState = d.old_state;
    t->SubState = d.sub_state;
    t->Flags = d.flags;
    t->Child = d.child;
    t->Parent = d.parent;
    t->LinkChild = d.link_child;
    t->LinkParent = d.link_parent;
    t->WorldPos.X = d.x;
    t->WorldPos.Y = d.y;
    t->WorldPos.Z = d.z;
    t->StateFn = nullptr;
    if (d.draw_kind == QS_DRAW_TWEEN) {
        t->Draw.Tweened = (DrawTween*)qs_element(QS_POOL_DRAWTWEENS, d.draw_index);
    } else if (d.draw_kind == QS_DRAW_MESH) {
        t->Draw.Mesh = (DrawMesh*)qs_element(QS_POOL_DRAWMESHES, d.draw_index);
    }
    if (d.genus_kind != QS_GENUS_NONE) {
        t->Genus.Vehicle = (Vehicle*)qs_element(d.genus_kind - 1, d.genus_index);
    }
    t->DrawType = d.draw_type;
    t->Lead = d.lead;
    t->Velocity = d.velocity;
    t->DeltaVelocity = d.delta_velocity;
    t->RequiredVelocity = d.required_velocity;
    t->DY = d.dy;
    t->Index = d.index;
    t->OnFace = d.on_face;
    t->NextLink = d.next_link;
    t->DogPoo1 = d.dog_poo1;
    t->DogPoo2 = d.dog_poo2;
    return true;
}

void qs_encode_player(const Player& p, QS_Player_Disk* d)
{
    memset(d, 0, sizeof(*d));
    d->player_type = p.PlayerType;
    d->padding = p.padding;
    d->thing = p.Thing;
    d->flags = p.Flags;
    d->input = p.Input;
    d->input_done = p.InputDone;
    d->player_id = p.PlayerID;
    d->stamina = p.Stamina;
    d->constitution = p.Constitution;
    d->last_input = p.LastInput;
    d->this_input = p.ThisInput;
    d->pressed = p.Pressed;
    d->released = p.Released;
    d->done_something = p.DoneSomething;
    memcpy(d->double_click, p.DoubleClick, QS_PLAYER_BUTTONS);
    d->strength = p.Strength;
    d->red_marks = p.RedMarks;
    d->traffic_violations = p.TrafficViolations;
    d->danger = p.Danger;
    d->popup_fade = p.PopupFade;
    d->item_focus = p.ItemFocus;
    d->item_count = p.ItemCount;
    d->skill = p.Skill;
    d->player_person = qs_thing_ref(p.PlayerPerson);
}

bool qs_decode_player(const QS_Player_Disk& d, Player* p)
{
    if (d.player_type >= QS_PLAYER_TYPES || !qs_thing_ref_ok(d.player_person)) {
        return false;
    }
    if (!p) {
        return true;
    }
    memset(p, 0, sizeof(*p));
    p->PlayerType = d.player_type;
    p->padding = d.padding;
    p->Thing = d.thing;
    p->Flags = d.flags;
    p->Input = d.input;
    p->InputDone = d.input_done;
    p->PlayerID = d.player_id;
    p->Stamina = d.stamina;
    p->Constitution = d.constitution;
    p->LastInput = d.last_input;
    p->ThisInput = d.this_input;
    p->Pressed = d.pressed;
    p->Released = d.released;
    p->DoneSomething = d.done_something;
    memcpy(p->DoubleClick, d.double_click, QS_PLAYER_BUTTONS);
    p->Strength = d.strength;
    p->RedMarks = d.red_marks;
    p->TrafficViolations = d.traffic_violations;
    p->Danger = d.danger;
    p->PopupFade = d.popup_fade;
    p->ItemFocus = d.item_focus;
    p->ItemCount = d.item_count;
    p->Skill = d.skill;
    p->CameraThing = nullptr;
    p->PlayerPerson = qs_thing_ptr(d.player_person);
    return true;
}

void qs_encode_pyro(const Pyro& p, QS_Pyro_Disk* d)
{
    memset(d, 0, sizeof(*d));
    d->flags = p.Flags;
    d->pyro_type = p.PyroType;
    d->dlight = p.dlight;
    d->thing = qs_thing_ref(p.thing);
    d->victim = qs_thing_ref(p.victim);
    d->target_x = p.target.X;
    d->target_y = p.target.Y;
    d->target_z = p.target.Z;
    d->radius = p.radius;
    d->counter = p.counter;
    memcpy(d->radii, p.radii, sizeof(d->radii));
    d->scale = p.scale;
    d->tints[0] = p.tints[0];
    d->tints[1] = p.tints[1];
    d->soundid = p.soundid;
    d->timer1 = p.Timer1;
    d->dummy = p.Dummy;
    d->last_smoke_spawn = p.LastSmokeSpawn;
    d->last_draw_counter = p.LastDrawCounter;
}

// The Pyro fields that later index another array. radii[] and Dummy hold handles only for
// the types below; every other use masks or range-checks them first (radii[7] is clamped to
// the prim's own point range in PYRO_draw_pyro, effects/combat/pyro.cpp).
bool qs_pyro_indices_ok(const QS_Pyro_Disk& d)
{
    if (d.pyro_type >= PYRO_RANGE || d.dlight >= NIGHT_MAX_DLIGHTS) {
        return false;
    }
    switch (d.pyro_type) {
    case PYRO_IMMOLATE:
        for (uint32_t i = 0; i < QS_PYRO_IMMOLATE_RIBBONS; i++) {
            // 1-based, 0 = none (RIBBON_extend, effects/environment/ribbon.cpp).
            if (d.radii[i] > MAX_RIBBONS) {
                return false;
            }
        }
        return true;
    case PYRO_FLICKER:
        return d.dummy <= MAX_RIBBONS;
    case PYRO_GAMEOVER:
        for (uint32_t i = 0; i < QS_PYRO_GAMEOVER_DLIGHTS; i++) {
            if (d.radii[i] >= NIGHT_MAX_DLIGHTS) {
                return false;
            }
        }
        return true;
    default:
        return true;
    }
}

bool qs_decode_pyro(const QS_Pyro_Disk& d, Pyro* p)
{
    if (!qs_pyro_indices_ok(d) || !qs_thing_ref_ok(d.thing) || !qs_thing_ref_ok(d.victim)) {
        return false;
    }
    if (!p) {
        return true;
    }
    memset(p, 0, sizeof(*p));
    p->Flags = d.flags;
    p->PyroType = d.pyro_type;
    p->dlight = d.dlight;
    p->thing = qs_thing_ptr(d.thing);
    p->victim = qs_thing_ptr(d.victim);
    p->target.X = d.target_x;
    p->target.Y = d.target_y;
    p->target.Z = d.target_z;
    p->radius = d.radius;
    p->counter = d.counter;
    memcpy(p->radii, d.radii, sizeof(p->radii));
    p->scale = d.scale;
    p->tints[0] = d.tints[0];
    p->tints[1] = d.tints[1];
    p->soundid = d.soundid;
    p->Timer1 = d.timer1;
    p->Dummy = d.dummy;
    p->LastSmokeSpawn = d.last_smoke_spawn;
    p->LastDrawCounter = d.last_draw_counter;
    return true;
}

void qs_encode_chopper(const Chopper& c, QS_Chopper_Disk* d)
{
    memset(d, 0, sizeof(*d));
    d->thing = qs_thing_ref(c.thing);
    d->target = qs_thing_ref(c.target);
    d->home_x = c.home.X;
    d->home_y = c.home.Y;
    d->home_z = c.home.Z;
    d->dist = c.dist;
    d->radius = c.radius;
    d->patrol = c.patrol;
    d->spotx = c.spotx;
    d->spotz = c.spotz;
    d->spotdx = c.spotdx;
    d->spotdz = c.spotdz;
    d->dx = c.dx;
    d->dy = c.dy;
    d->dz = c.dz;
    d->victim = c.victim;
    d->rx = c.rx;
    d->ry = c.ry;
    d->rz = c.rz;
    d->counter = c.counter;
    d->rotors = c.rotors;
    d->rotorspeed = c.rotorspeed;
    d->speed = c.speed;
    d->chopper_type = c.ChopperType;
    d->rotorprim = c.rotorprim;
    d->substate = c.substate;
    d->light = c.light;
    d->since_takeoff = c.since_takeoff;
    d->padding = c.padding;
}

bool qs_decode_chopper(const QS_Chopper_Disk& d, Chopper* c)
{
    if (d.chopper_type >= CHOPPER_NUMB || !qs_thing_ref_ok(d.thing) || !qs_thing_ref_ok(d.target)) {
        return false;
    }
    if (!c) {
        return true;
    }
    memset(c, 0, sizeof(*c));
    c->thing = qs_thing_ptr(d.thing);
    c->target = qs_thing_ptr(d.target);
    c->home.X = d.home_x;
    c->home.Y = d.home_y;
    c->home.Z = d.home_z;
    c->dist = d.dist;
    c->radius = d.radius;
    c->patrol = d.patrol;
    c->spotx = d.spotx;
    c->spotz = d.spotz;
    c->spotdx = d.spotdx;
    c->spotdz = d.spotdz;
    c->dx = d.dx;
    c->dy = d.dy;
    c->dz = d.dz;
    c->victim = d.victim;
    c->rx = d.rx;
    c->ry = d.ry;
    c->rz = d.rz;
    c->counter = d.counter;
    c->rotors = d.rotors;
    c->rotorspeed = d.rotorspeed;
    c->speed = d.speed;
    c->ChopperType = d.chopper_type;
    c->rotorprim = d.rotorprim;
    c->substate = d.substate;
    c->light = d.light;
    c->since_takeoff = d.since_takeoff;
    c->padding = d.padding;
    return true;
}

void qs_encode_animal(const Animal& a, QS_Animal_Disk* d)
{
    memset(d, 0, sizeof(*d));
    d->counter = a.counter;
    d->dist = a.dist;
    d->starty = a.starty;
    d->animal_type = a.AnimalType;
    d->substate = a.substate;
    d->map_x = a.map_x;
    d->map_z = a.map_z;
    d->dest_x = a.dest_x;
    d->dest_z = a.dest_z;
    d->home_x = a.home_x;
    d->home_z = a.home_z;
    d->extra = a.extra;
    d->padding = a.padding;
}

void qs_decode_animal(const QS_Animal_Disk& d, Animal* a)
{
    memset(a, 0, sizeof(*a));
    a->target = nullptr;
    a->counter = d.counter;
    a->dist = d.dist;
    a->starty = d.starty;
    a->AnimalType = d.animal_type;
    a->substate = d.substate;
    a->map_x = d.map_x;
    a->map_z = d.map_z;
    a->dest_x = d.dest_x;
    a->dest_z = d.dest_z;
    a->home_x = d.home_x;
    a->home_z = d.home_z;
    a->extra = d.extra;
    a->padding = d.padding;
}

// ---------------------------------------------------------------------------
// Pools: count rule and the per-pool encoders
// ---------------------------------------------------------------------------

// Elements written for one pool: Maximum for MEM_STATIC, exactly the live cursor for
// MEM_DYNAMIC. The original level-file writer added MemTable.Extra as runtime headroom
// (save_whole_wad, original_game/.../memory.cpp) because its loader adopted the file image as
// the arena (load_whole_game, missions/memory.cpp). A quick save is copied into the full-size live pools,
// so the headroom has no purpose here, and it could not be written for a nearly full pool
// (wmove holds up to 191 faces, Maximum 192, Extra 64).
bool qs_pool_write_count(uint32_t id, uint32_t* count)
{
    const MemTable& t = save_table[id];
    if (t.Type == MEM_STATIC) {
        *count = uint32_t(t.Maximum);
        return true;
    }
    if (t.Type != MEM_DYNAMIC || !qs_pool_has_cursor(t)) {
        return false;
    }
    const SLONG cursor = qs_pool_cursor(t);
    if (cursor < 0 || cursor > t.Maximum) {
        fprintf(stderr, "QUICKSAVE: pool %s cursor %d is outside 0..%d; save aborted\n", t.Name, cursor, t.Maximum);
        return false;
    }
    *count = uint32_t(cursor);
    return true;
}

template <typename T>
T qs_load_elem(const uint8_t* base, uint32_t i)
{
    T v;
    memcpy(&v, base + size_t(i) * sizeof(T), sizeof(T));
    return v;
}

template <typename T>
void qs_store_elem(uint8_t* base, uint32_t i, const T& v)
{
    memcpy(base + size_t(i) * sizeof(T), &v, sizeof(T));
}

bool qs_encode_pool(QS_Encoder& enc, uint32_t slot)
{
    const uint32_t id = QS_POOL_IDS[slot];
    uint32_t count = 0;
    if (!qs_pool_write_count(id, &count)) {
        return false;
    }
    const uint32_t elem = QS_ELEMENT_BYTES[slot];
    const uint32_t bytes = count * elem;

    uint8_t hdr[sizeof(QS_PoolHeader)];
    qs_put_u32(hdr, id);
    qs_put_u32(hdr + 4, count);
    qs_put_u32(hdr + 8, elem);
    qs_put_u32(hdr + 12, bytes);
    enc.put(hdr, sizeof(hdr));
    const uint32_t at = enc.grow(bytes);
    enc.sections.push_back({ save_table[id].Name, at, bytes, elem });
    uint8_t* out = enc.buf.data() + at;
    const uint8_t* base = qs_pool_base(id);

    switch (id) {
    case QS_POOL_THINGS:
        for (uint32_t i = 0; i < count; i++) {
            QS_Thing_Disk d;
            if (!qs_encode_thing(qs_load_elem<Thing>(base, i), i, &d)) {
                return false;
            }
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_DRAWTWEENS:
        for (uint32_t i = 0; i < count; i++) {
            QS_DrawTween_Disk d;
            if (!qs_encode_drawtween(qs_load_elem<DrawTween>(base, i), &d)) {
                return false;
            }
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_VEHICLES:
        for (uint32_t i = 0; i < count; i++) {
            const Vehicle v = qs_load_elem<Vehicle>(base, i);
            QS_Vehicle_Disk d;
            memset(&d, 0, sizeof(d));
            if (!qs_encode_drawtween(v.Draw, &d.draw)) {
                return false;
            }
            memcpy(d.rest, (const uint8_t*)&v + sizeof(DrawTween), sizeof(d.rest));
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_PLAYERS:
        for (uint32_t i = 0; i < count; i++) {
            QS_Player_Disk d;
            qs_encode_player(qs_load_elem<Player>(base, i), &d);
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_PYRO:
        for (uint32_t i = 0; i < count; i++) {
            QS_Pyro_Disk d;
            qs_encode_pyro(qs_load_elem<Pyro>(base, i), &d);
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_CHOPPERS:
        for (uint32_t i = 0; i < count; i++) {
            QS_Chopper_Disk d;
            qs_encode_chopper(qs_load_elem<Chopper>(base, i), &d);
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_ANIMALS:
        for (uint32_t i = 0; i < count; i++) {
            QS_Animal_Disk d;
            qs_encode_animal(qs_load_elem<Animal>(base, i), &d);
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_EWAY_MESS:
        for (uint32_t i = 0; i < count; i++) {
            const QS_MessRef d = { qs_mess_offset(qs_load_elem<CBYTE*>(base, i)) };
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_MAP_BEACON: {
        const uint64_t now = sdl3_get_ticks();
        for (uint32_t i = 0; i < count; i++) {
            const MAP_Beacon b = qs_load_elem<MAP_Beacon>(base, i);
            QS_Beacon_Disk d;
            memset(&d, 0, sizeof(d));
            memcpy(d.prefix, &b, QS_BEACON_PREFIX_BYTES);
            d.age_ms = qs_age_ms(b.ticks, now);
            memcpy(out + size_t(i) * elem, &d, elem);
        }
    } break;
    case QS_POOL_CUTSCENE_DATA:
        for (uint32_t i = 0; i < count; i++) {
            const CPData c = qs_load_elem<CPData>(base, i);
            QS_CPData_Disk d;
            memset(&d, 0, sizeof(d));
            d.version = c.version;
            d.channelcount = c.channelcount;
            d.pad1 = c.pad1;
            d.pad2 = c.pad2;
            uint16_t idx = QS_NULL_INDEX16;
            if (c.channels && !qs_find_element(QS_POOL_CUTSCENE_TRKS, c.channels, &idx)) {
                idx = QS_NULL_INDEX16;
            }
            d.channels = idx;
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    case QS_POOL_CUTSCENE_TRKS:
        for (uint32_t i = 0; i < count; i++) {
            const CPChannel c = qs_load_elem<CPChannel>(base, i);
            QS_CPChannel_Disk d;
            memset(&d, 0, sizeof(d));
            d.type = c.type;
            d.flags = c.flags;
            d.pad1 = c.pad1;
            d.pad2 = c.pad2;
            d.index = c.index;
            d.packetcount = c.packetcount;
            uint16_t idx = QS_NULL_INDEX16;
            if (c.packets && !qs_find_element(QS_POOL_CUTSCENE_PKTS, c.packets, &idx)) {
                idx = QS_NULL_INDEX16;
            }
            d.packets = idx;
            memcpy(out + size_t(i) * elem, &d, elem);
        }
        break;
    default:
        // Pointer-free pool: a raw image of the live elements.
        memcpy(out, base, bytes);
        break;
    }

    // Per-element fields that must not survive a load, written as their reset value so
    // the file and the post-load state agree (and the Debug verifier sees no difference).
    if (id == QS_POOL_PEOPLE) {
        // Wall-clock muzzle-flash deadline (visual only): a stale stamp from another
        // process could read as "flash for hours".
        for (uint32_t i = 0; i < count; i++) {
            memset(out + size_t(i) * elem + offsetof(Person, MuzzleFlashUntilMs), 0, sizeof(uint64_t));
        }
    } else if (id == QS_POOL_DRAWMESHES) {
        // Lighting cache slot; the caches are rebuilt after a load (the level loader clears it
        // the same way: convert_drawtype_to_pointer, missions/memory.cpp).
        for (uint32_t i = 0; i < count; i++) {
            memset(out + size_t(i) * elem + offsetof(DrawMesh, Cache), 0, sizeof(CACHE_Index));
        }
    } else if (id == QS_POOL_DFACETS) {
        // Night cache index (cleared by uncache()) and the two visit stamps compared with
        // SUPERMAP_counter: both reset on load.
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* e = out + size_t(i) * elem;
            e[offsetof(DFacet, Dfcache)] = 0;
            memset(e + offsetof(DFacet, Counter), 0, QS_SUPERMAP_COUNTERS);
        }
    }

    enc.align();
    return true;
}

// Encodes the whole file (header included, still without its CRC) from live state.
bool qs_encode_file(QS_Encoder& enc)
{
    enc.buf.clear();
    enc.sections.clear();
    if (!qs_save_table_ok()) {
        return false;
    }
    const uint64_t now = sdl3_get_ticks();

    enc.grow(QS_HEADER_BYTES);

    QS_Campaign campaign;
    qs_capture_campaign(&campaign);
    enc.sections.push_back({ "campaign", uint32_t(enc.buf.size()), sizeof(campaign), 0 });
    enc.put(&campaign, sizeof(campaign));
    enc.align();

    QS_Scalars scalars;
    qs_encode_scalars(&scalars, now);
    qs_put_record(enc, "scalars", QS_REC_SCALARS, &scalars, sizeof(scalars));

    QS_Pcom pcom;
    qs_encode_pcom(&pcom);
    qs_put_record(enc, "pcom", QS_REC_PCOM, &pcom, sizeof(pcom));

    QS_Camera camera;
    qs_encode_camera(&camera);
    qs_put_record(enc, "camera", QS_REC_CAMERA, &camera, sizeof(camera));

    QS_Grenades grenades;
    qs_encode_grenades(&grenades);
    qs_put_record(enc, "grenades", QS_REC_GRENADES, &grenades, sizeof(grenades));

    static_assert(sizeof(QS_Ribbons) == sizeof(ribbon_Ribbons));
    qs_put_record(enc, "ribbons", QS_REC_RIBBONS, ribbon_Ribbons, sizeof(QS_Ribbons));

    static QS_Hook hook; // 5 KB: kept off the stack
    qs_encode_hook(&hook);
    qs_put_record(enc, "hook", QS_REC_HOOK, &hook, sizeof(hook));

    for (uint32_t slot = 0; slot < QS_POOL_COUNT; slot++) {
        if (!qs_encode_pool(enc, slot)) {
            return false;
        }
    }

    uint8_t trailer[sizeof(QS_Trailer)];
    qs_put_u32(trailer, QS_END_MAGIC);
    qs_put_u32(trailer + 4, 0);
    enc.sections.push_back({ "trailer", uint32_t(enc.buf.size()), sizeof(trailer), 0 });
    enc.put(trailer, sizeof(trailer));

    // Header.
    QS_Header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, &QS_MAGIC, sizeof(h.magic));
    h.format_version = QS_FORMAT_VERSION;
    h.header_bytes = QS_HEADER_BYTES;
    h.endian_marker = QS_ENDIAN_MARKER;
    h.pointer_bits = QS_POINTER_BITS;
    h.table_fingerprint = qs_table_fingerprint();
    h.payload_bytes = uint32_t(enc.buf.size()) - QS_HEADER_BYTES;
    h.payload_crc = ~qs_crc32(QS_CRC_INIT, enc.buf.data() + QS_HEADER_BYTES, h.payload_bytes);
    qs_level_tag(h.level_tag);
    strncpy(h.level_path, ELEV_fname_level, QS_LEVEL_PATH_BYTES - 1);
    memcpy(enc.buf.data(), &h, sizeof(h));
    return true;
}

#ifndef NDEBUG
// Output cap of the Debug-only checks: at most this many lines, then a count.
constexpr uint32_t QS_VERIFY_MAX_LINES_PER_SECTION = 20;

// Debug-only save-time check of the StateFn derivation the load relies on: every live
// Thing whose handler is not the one the loader will derive is reported (not refused),
// so an in-game test shows exactly which class/state the derivation misses.
void qs_debug_check_statefn()
{
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < MAX_THINGS; i++) {
        const Thing* t = TO_THING(i);
        if (t->Class == CLASS_NONE || !t->StateFn) {
            continue;
        }
        if (qs_derive_statefn(t) != t->StateFn) {
            if (mismatches < QS_VERIFY_MAX_LINES_PER_SECTION) {
                fprintf(stderr, "QUICKSAVE statefn: Thing %u class %d state %d: the live handler is not the one the load derives\n",
                    i, t->Class, t->State);
            }
            mismatches++;
        }
    }
    if (mismatches > QS_VERIFY_MAX_LINES_PER_SECTION) {
        fprintf(stderr, "QUICKSAVE statefn: %u mismatching Things in total\n", mismatches);
    }
}
#endif

bool qs_write()
{
    char fn[QS_FILE_NAME_BYTES];
    if (!qs_file_name(fn)) {
        return false;
    }
    QS_Encoder enc;
    if (!qs_encode_file(enc)) {
        return false;
    }
#ifndef NDEBUG
    qs_debug_check_statefn();
#endif
    MFFileHandle h = FileCreate(fn, UC_TRUE);
    if (h == FILE_CREATION_ERROR) {
        fprintf(stderr, "QUICKSAVE: cannot create %s\n", fn);
        return false;
    }
    const SLONG written = FileWrite(h, enc.buf.data(), ULONG(enc.buf.size()));
    FileClose(h);
    if (written != SLONG(enc.buf.size())) {
        // A half-written file must not look valid.
        fprintf(stderr, "QUICKSAVE: short write to %s (%d of %u bytes); file deleted\n", fn, written, unsigned(enc.buf.size()));
        FileDelete(fn);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// The reader: read, validate everything, and only then apply.
// ---------------------------------------------------------------------------

struct QS_PoolView {
    const uint8_t* data;
    uint32_t count;
};

struct QS_Parsed {
    QS_Campaign campaign;
    QS_Scalars scalars;
    QS_Pcom pcom;
    QS_Camera camera;
    QS_Grenades grenades;
    const uint8_t* ribbons; // sizeof(QS_Ribbons) bytes inside the file buffer
    const uint8_t* hook; // sizeof(QS_Hook) bytes inside the file buffer
    QS_PoolView pools[QS_POOL_COUNT];
    std::vector<QS_Section> sections;
};

// Checks a header against the running game and the current level.
bool qs_header_ok(const QS_Header& h)
{
    if (memcmp(h.magic, &QS_MAGIC, sizeof(h.magic)) != 0 || h.format_version != QS_FORMAT_VERSION
        || h.header_bytes != QS_HEADER_BYTES || h.endian_marker != QS_ENDIAN_MARKER
        || h.pointer_bits != QS_POINTER_BITS) {
        fprintf(stderr, "QUICKSAVE: not a version %u quick save of this build\n", QS_FORMAT_VERSION);
        return false;
    }
    char tag[QS_LEVEL_TAG_BYTES];
    qs_level_tag(tag);
    if (!tag[0] || memcmp(tag, h.level_tag, QS_LEVEL_TAG_BYTES) != 0) {
        fprintf(stderr, "QUICKSAVE: the quick save belongs to another level\n");
        return false;
    }
    if (!qs_save_table_ok() || h.table_fingerprint != qs_table_fingerprint()) {
        fprintf(stderr, "QUICKSAVE: the quick save was written by a build with another save_table\n");
        return false;
    }
    if (h.payload_bytes == 0 || h.payload_bytes > QS_MAX_PAYLOAD_BYTES) {
        fprintf(stderr, "QUICKSAVE: payload size %u out of bounds\n", h.payload_bytes);
        return false;
    }
    return true;
}

// Reads the header and the campaign block only (used by the level-lost menu hook).
bool qs_read_head(QS_Header* h, QS_Campaign* c)
{
    char fn[QS_FILE_NAME_BYTES];
    if (!qs_file_name(fn)) {
        return false;
    }
    MFFileHandle f = FileOpen(fn);
    if (f == FILE_OPEN_ERROR) {
        return false;
    }
    uint8_t head[QS_HEADER_BYTES + sizeof(QS_Campaign)];
    const SLONG got = FileRead(f, head, sizeof(head));
    FileClose(f);
    if (got != SLONG(sizeof(head))) {
        return false;
    }
    memcpy(h, head, sizeof(*h));
    memcpy(c, head + QS_CAMPAIGN_OFFSET, sizeof(*c));
    return qs_header_ok(*h);
}

// Reads the whole file and checks size and CRC.
bool qs_read_file(std::vector<uint8_t>& file)
{
    char fn[QS_FILE_NAME_BYTES];
    if (!qs_file_name(fn)) {
        return false;
    }
    MFFileHandle f = FileOpen(fn);
    if (f == FILE_OPEN_ERROR) {
        fprintf(stderr, "QUICKSAVE: no quick save for this level\n");
        return false;
    }
    QS_Header h;
    bool ok = FileRead(f, &h, sizeof(h)) == SLONG(sizeof(h)) && qs_header_ok(h);
    if (ok && FileSize(f) != SLONG(QS_HEADER_BYTES + h.payload_bytes)) {
        fprintf(stderr, "QUICKSAVE: file size does not match the header (truncated?)\n");
        ok = false;
    }
    if (ok) {
        file.assign(size_t(QS_HEADER_BYTES) + h.payload_bytes, 0);
        memcpy(file.data(), &h, sizeof(h));
        ok = FileRead(f, file.data() + QS_HEADER_BYTES, h.payload_bytes) == SLONG(h.payload_bytes);
    }
    FileClose(f);
    if (!ok) {
        file.clear();
        return false;
    }
    if (~qs_crc32(QS_CRC_INIT, file.data() + QS_HEADER_BYTES, h.payload_bytes) != h.payload_crc) {
        fprintf(stderr, "QUICKSAVE: payload CRC mismatch\n");
        file.clear();
        return false;
    }
    return true;
}

struct QS_Cursor {
    const uint8_t* data;
    uint32_t size;
    uint32_t pos;

    bool take(void* out, uint32_t n)
    {
        if (n > size - pos) {
            return false;
        }
        memcpy(out, data + pos, n);
        pos += n;
        return true;
    }
    bool skip_to_align()
    {
        const uint32_t to = qs_align_up(pos);
        if (to > size) {
            return false;
        }
        for (; pos < to; pos++) {
            if (data[pos] != 0) {
                return false;
            }
        }
        return true;
    }
};

bool qs_take_record(QS_Cursor& c, QS_Parsed& p, const char* name, uint32_t magic, void* out, uint32_t bytes, const uint8_t** raw)
{
    uint8_t hdr[sizeof(QS_RecordHeader)];
    if (!c.take(hdr, sizeof(hdr)) || qs_get_u32(hdr) != magic || qs_get_u32(hdr + 4) != bytes) {
        fprintf(stderr, "QUICKSAVE: record %s missing or of the wrong size\n", name);
        return false;
    }
    p.sections.push_back({ name, c.pos, bytes, 0 });
    if (raw) {
        if (bytes > c.size - c.pos) {
            return false;
        }
        *raw = c.data + c.pos;
        c.pos += bytes;
    } else if (!c.take(out, bytes)) {
        return false;
    }
    return c.skip_to_align();
}

// Checks every reference a pool's elements carry against the live (reloaded) level.
bool qs_check_pool_refs(uint32_t id, const uint8_t* data, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        bool ok = true;
        switch (id) {
        case QS_POOL_THINGS:
            ok = qs_decode_thing(qs_load_elem<QS_Thing_Disk>(data, i), nullptr);
            break;
        case QS_POOL_DRAWTWEENS:
            ok = qs_decode_drawtween(qs_load_elem<QS_DrawTween_Disk>(data, i), nullptr);
            break;
        case QS_POOL_VEHICLES: {
            const QS_Vehicle_Disk v = qs_load_elem<QS_Vehicle_Disk>(data, i);
            // Vehicle.dlight (process_car, things/vehicles/vehicle.cpp) sits in the raw remainder.
            ok = qs_decode_drawtween(v.draw, nullptr) && v.rest[offsetof(Vehicle, dlight) - sizeof(DrawTween)] < NIGHT_MAX_DLIGHTS;
        } break;
        case QS_POOL_PLAYERS:
            ok = qs_decode_player(qs_load_elem<QS_Player_Disk>(data, i), nullptr);
            break;
        case QS_POOL_PYRO:
            ok = qs_decode_pyro(qs_load_elem<QS_Pyro_Disk>(data, i), nullptr);
            break;
        case QS_POOL_CHOPPERS:
            ok = qs_decode_chopper(qs_load_elem<QS_Chopper_Disk>(data, i), nullptr);
            break;
        case QS_POOL_EWAY_MESS:
            ok = qs_mess_offset_ok(qs_load_elem<QS_MessRef>(data, i).offset);
            break;
        case QS_POOL_CUTSCENE_DATA: {
            const uint16_t ch = qs_load_elem<QS_CPData_Disk>(data, i).channels;
            ok = ch == QS_NULL_INDEX16 || ch < save_table[QS_POOL_CUTSCENE_TRKS].Maximum;
        } break;
        case QS_POOL_CUTSCENE_TRKS: {
            const uint16_t pk = qs_load_elem<QS_CPChannel_Disk>(data, i).packets;
            ok = pk == QS_NULL_INDEX16 || pk < save_table[QS_POOL_CUTSCENE_PKTS].Maximum;
        } break;
        case QS_POOL_THING_HEADS:
            ok = qs_thing_ref_ok(qs_load_elem<UWORD>(data, i));
            break;
        default:
            return true;
        }
        if (!ok) {
            fprintf(stderr, "QUICKSAVE: pool %s element %u has an invalid reference\n", save_table[id].Name, i);
            return false;
        }
    }
    return true;
}

// Index values held by the fixed records that need nothing but a compiled bound. Pools stored
// as raw images are not walked field by field: their inner indices are covered by
// the CRC, the save_table fingerprint and the level tag.
bool qs_check_fixed_refs(const QS_Parsed& p)
{
    const QS_Scalars& s = p.scalars;
    bool ok = (s.game_state & GS_PLAY_GAME) && qs_mess_offset_ok(s.eway_tutorial_string_offset);

    // Thing indices, 0 = NULL.
    const uint16_t things[] = {
        s.used_primary, s.unused_primary, s.used_secondary, s.unused_secondary,
        s.talk_thing, s.eway_used_thing, s.eway_conv_person_a, s.eway_conv_person_b, s.eway_cam_thing,
        p.pcom.runover_scary_person,
        qs_get_u16(p.camera.fc_cam.bytes + offsetof(FC_Cam, focus)),
        qs_get_u16(p.camera.fc_cam.bytes + offsetof(FC_Cam, fight_track_focus)),
    };
    for (const uint16_t idx : things) {
        ok = ok && qs_thing_ref_ok(idx);
    }
    for (uint32_t i = 0; i < MAX_PLAYERS; i++) {
        ok = ok && qs_thing_ref_ok(s.net_person[i]) && qs_thing_ref_ok(s.net_player[i]);
    }

    // Cursors the game uses as direct indices into the restored pools.
    ok = ok && s.track_eob <= uint32_t(save_table[QS_POOL_TRACKS].Maximum) && s.track_head < s.track_eob && s.track_tail < s.track_eob;
    ok = ok && s.night_dlight_free < NIGHT_MAX_DLIGHTS && s.night_dlight_used < NIGHT_MAX_DLIGHTS;
    ok = ok && s.barrel_sphere_last >= 0 && s.barrel_sphere_last < BARREL_MAX_SPHERES;
    ok = ok && s.eway_conv_str < uint32_t(save_table[QS_POOL_EWAY_MESS_BUF].Maximum);

    // The AI gang tables. A full table (upto == PCOM_MAX_GANG_PEOPLE) is legal: the game stops
    // adding at that point (PCOM_add_gang_member, ai/pcom.cpp). A gang always lies inside the
    // used part, which is what keeps PCOM_gang_person[index + number] in bounds when upto is
    // below the maximum.
    ok = ok && p.pcom.gang_person_upto >= 0 && p.pcom.gang_person_upto <= PCOM_MAX_GANG_PEOPLE;
    for (uint32_t i = 0; i < PCOM_MAX_GANG_PEOPLE; i++) {
        ok = ok && qs_thing_ref_ok(p.pcom.gang_person[i]);
    }
    for (uint32_t i = 0; i < PCOM_MAX_GANGS; i++) {
        ok = ok && int32_t(p.pcom.gang_index[i]) + p.pcom.gang_number[i] <= p.pcom.gang_person_upto;
    }
    for (uint32_t i = 0; i < PCOM_SUMMON_NUM_BODIES; i++) {
        ok = ok && qs_thing_ref_ok(p.pcom.summon[i]);
    }
    for (uint32_t i = 0; i < BAT_SUMMON_NUM_BODIES; i++) {
        ok = ok && qs_thing_ref_ok(p.pcom.bat_summon[i]);
    }

    for (uint32_t i = 0; i < MAX_GRENADES; i++) {
        ok = ok && qs_thing_ref_ok(p.grenades.grenade[i].owner);
    }
    ok = ok && p.grenades.global_g <= MAX_GRENADES;

    if (!ok) {
        fprintf(stderr, "QUICKSAVE: a fixed record holds an invalid reference\n");
    }
    return ok;
}

// A half-open element range [first, first + number) inside a pool of `count` elements.
bool qs_range_ok(int32_t first, int32_t number, uint32_t count)
{
    return first >= 0 && number >= 0 && int64_t(first) + number <= int64_t(count);
}

// Index values of the fixed records whose bound is another pool's live count.
bool qs_check_pool_bound_refs(const QS_Parsed& p)
{
    const QS_Scalars& s = p.scalars;
    const uint32_t ways = p.pools[QS_SLOT_EWAY_WAY].count;
    bool ok = s.eway_conv_waypoint == 0 || s.eway_conv_waypoint < ways;

    // EWAY_mess[index .. index + number) (EWAY_load_fake_wander_text, missions/eway.cpp); the fields are
    // stored as (index, number) pairs, see qs_fake_wander_fields[].
    for (uint32_t i = 0; i + 1 < QS_FAKE_WANDER_FIELDS; i += 2) {
        ok = ok && qs_range_ok(s.eway_fake_wander[i], s.eway_fake_wander[i + 1], p.pools[QS_SLOT_EWAY_MESS].count);
    }

    // The walkable ranges index the restored prim pools (NIGHT_generate_roof_walkable,
    // engine/graphics/lighting/night.cpp).
    ok = ok && qs_range_ok(s.first_walkable_prim_point, s.number_of_walkable_prim_points, p.pools[QS_SLOT_PRIM_POINTS].count);
    ok = ok && qs_range_ok(s.first_walkable_prim_face4, s.number_of_walkable_prim_faces4, p.pools[QS_SLOT_PRIM_FACES4].count);

    // Camera waypoints: EWAY_NO_MATCH is only a leftover of an inactive camera
    // (EWAY_create_camera, missions/eway.cpp); an active camera indexes EWAY_way with both.
    bool cam_active = false;
    for (uint32_t i = 0; i < QS_EWAY_CAM_FIELDS; i++) {
        if (qs_eway_cam_fields[i] == &EWAY_cam_active) {
            cam_active = s.eway_cam[i] != 0;
        }
    }
    for (uint32_t i = 0; i < QS_EWAY_CAM_FIELDS; i++) {
        if (qs_eway_cam_fields[i] == &EWAY_cam_waypoint || qs_eway_cam_fields[i] == &EWAY_cam_target) {
            const int32_t v = s.eway_cam[i];
            ok = ok && ((v == EWAY_NO_MATCH && !cam_active) || (v >= 0 && uint32_t(v) < ways));
        }
    }

    if (!ok) {
        fprintf(stderr, "QUICKSAVE: a fixed record holds an index outside its pool\n");
    }
    return ok;
}

// The Thing.Genus kind each class allocates from (convert_thing_to_pointer, missions/memory.cpp);
// QS_GENUS_NONE for a class with no pool of its own.
uint16_t qs_class_genus_kind(uint8_t cls)
{
    switch (cls) {
    case CLASS_PLAYER:
        return QS_POOL_PLAYERS + 1;
    case CLASS_PROJECTILE:
        return QS_POOL_PROJECTILES + 1;
    case CLASS_PERSON:
        return QS_POOL_PEOPLE + 1;
    case CLASS_ANIMAL:
        return QS_POOL_ANIMALS + 1;
    case CLASS_SWITCH:
        return QS_POOL_SWITCHES + 1;
    case CLASS_VEHICLE:
        return QS_POOL_VEHICLES + 1;
    case CLASS_SPECIAL:
        return QS_POOL_SPECIALS + 1;
    case CLASS_CHOPPER:
        return QS_POOL_CHOPPERS + 1;
    case CLASS_PYRO:
        return QS_POOL_PYRO + 1;
    case CLASS_TRACK:
        return QS_POOL_TRACKS + 1;
    case CLASS_PLAT:
        return QS_POOL_PLATS + 1;
    case CLASS_BARREL:
        return QS_POOL_BARRELS + 1;
    case CLASS_BAT:
        return QS_POOL_BATS + 1;
    default:
        return QS_GENUS_NONE;
    }
}

// Thing.OnFace: 0 = none, > 0 a prim_faces4[] index, < 0 either a hidden-roof encoding (map
// coordinates, masked on use, map/pap.h) or a roof_faces4[] index. The roof faces are
// not in the file; the running level (same level tag) holds them.
bool qs_on_face_ok(int16_t face, uint32_t prim_faces4_count)
{
    if (face >= 0) {
        return uint32_t(face) < prim_faces4_count || face == 0;
    }
    return face < ROOF_HIDDEN_INDEX || -int32_t(face) < int32_t(next_roof_face4);
}

// Everything qs_derive_statefn() and the game's per-type tables will index with a live
// Thing's file values: the Genus pool its class allocates from, State for
// the handler tables, PersonType and PyroType through the Genus record, OnFace.
bool qs_check_thing_tables(const QS_Parsed& p)
{
    const QS_PoolView& things = p.pools[QS_SLOT_THINGS];
    const QS_PoolView& people = p.pools[QS_SLOT_PEOPLE];
    const QS_PoolView& pyros = p.pools[QS_SLOT_PYRO];
    const uint32_t prim_faces4_count = p.pools[QS_SLOT_PRIM_FACES4].count;
    for (uint32_t i = 0; i < things.count; i++) {
        const QS_Thing_Disk d = qs_load_elem<QS_Thing_Disk>(things.data, i);
        bool ok = qs_on_face_ok(d.on_face, prim_faces4_count);
        if (d.class_ != CLASS_NONE) {
            const uint16_t kind = qs_class_genus_kind(d.class_);
            ok = ok && (kind == QS_GENUS_NONE || d.genus_kind == QS_GENUS_NONE || d.genus_kind == kind);
            const bool has_genus = d.genus_kind != QS_GENUS_NONE;
            const bool present = (p.scalars.statefn_present[i >> 3] >> (i & 7)) & 1;
            if (d.class_ == CLASS_PERSON) {
                ok = ok && d.state < QS_GENERIC_PERSON_STATE_FNS;
                ok = ok && (!has_genus || (d.genus_index < people.count && qs_load_elem<Person>(people.data, d.genus_index).PersonType < PERSON_NUM_TYPES));
            } else if (d.class_ == CLASS_PYRO) {
                ok = ok && (!present || d.state < QS_PYRO_STATE_FNS);
                if (ok && has_genus) {
                    // qs_decode_pyro() has already bounded the type. A set handler needs a
                    // real table: PYRO_functions[PYRO_NONE] is NULL.
                    ok = d.genus_index < pyros.count
                        && (!present || PYRO_functions[qs_load_elem<QS_Pyro_Disk>(pyros.data, d.genus_index).pyro_type].StateFunctions);
                }
            }
        }
        if (!ok) {
            fprintf(stderr, "QUICKSAVE: Thing %u (class %d state %d) holds an invalid table index\n", i, d.class_, d.state);
            return false;
        }
    }
    return true;
}

// Indices in the pool records whose bound is another pool's live count: a chopper's EWAY
// waypoint, a beacon's message, the cutscene channel and packet ranges.
bool qs_check_record_links(const QS_Parsed& p)
{
    const uint32_t ways = p.pools[QS_SLOT_EWAY_WAY].count;
    const QS_PoolView& choppers = p.pools[QS_SLOT_CHOPPERS];
    for (uint32_t i = 0; i < choppers.count; i++) {
        const int32_t victim = qs_load_elem<QS_Chopper_Disk>(choppers.data, i).victim;
        // EWAY_get_person(victim) indexes EWAY_way[], 0 = none (missions/eway.cpp).
        if (victim < 0 || (victim != 0 && uint32_t(victim) >= ways)) {
            fprintf(stderr, "QUICKSAVE: chopper %u has an invalid waypoint\n", i);
            return false;
        }
    }

    const uint32_t messages = p.pools[QS_SLOT_EWAY_MESS].count;
    const QS_PoolView& beacons = p.pools[QS_SLOT_MAP_BEACON];
    for (uint32_t i = 0; i < beacons.count; i++) {
        const QS_Beacon_Disk d = qs_load_elem<QS_Beacon_Disk>(beacons.data, i);
        MAP_Beacon b;
        memset(&b, 0, sizeof(b));
        memcpy(&b, d.prefix, QS_BEACON_PREFIX_BYTES);
        // TO_THING(track_thing) (MAP_process_beacons, ui/hud/eng_map.cpp), EWAY_get_mess(index)
        // (PANEL_last, ui/hud/panel.cpp).
        if (!qs_thing_ref_ok(b.track_thing) || (b.used && b.index >= messages)) {
            fprintf(stderr, "QUICKSAVE: beacon %u holds an invalid reference\n", i);
            return false;
        }
    }

    const QS_PoolView& cutscenes = p.pools[QS_SLOT_CUTSCENE_DATA];
    const QS_PoolView& tracks = p.pools[QS_SLOT_CUTSCENE_TRKS];
    for (uint32_t i = 0; i < cutscenes.count; i++) {
        const QS_CPData_Disk d = qs_load_elem<QS_CPData_Disk>(cutscenes.data, i);
        if (d.channelcount && (d.channels == QS_NULL_INDEX16 || uint32_t(d.channels) + d.channelcount > tracks.count)) {
            fprintf(stderr, "QUICKSAVE: cutscene %u has an invalid channel range\n", i);
            return false;
        }
    }
    const uint32_t packets = p.pools[QS_SLOT_CUTSCENE_PKTS].count;
    for (uint32_t i = 0; i < tracks.count; i++) {
        const QS_CPChannel_Disk d = qs_load_elem<QS_CPChannel_Disk>(tracks.data, i);
        const bool range_ok = !d.packetcount || (d.packets != QS_NULL_INDEX16 && uint32_t(d.packets) + d.packetcount <= packets);
        const bool person_ok = d.type != QS_CUTSCENE_CHANNEL_CHAR || (d.index >= 1 && d.index <= PERSON_NUM_TYPES);
        if (!range_ok || !person_ok) {
            fprintf(stderr, "QUICKSAVE: cutscene channel %u holds an invalid reference\n", i);
            return false;
        }
    }
    return true;
}

// Walks and validates the whole payload. Nothing is applied.
bool qs_parse(const std::vector<uint8_t>& file, QS_Parsed& p)
{
    QS_Cursor c = { file.data(), uint32_t(file.size()), QS_HEADER_BYTES };
    p.sections.clear();

    p.sections.push_back({ "campaign", c.pos, sizeof(QS_Campaign), 0 });
    if (!c.take(&p.campaign, sizeof(p.campaign)) || !c.skip_to_align() || p.campaign.version != QS_CAMPAIGN_VERSION) {
        fprintf(stderr, "QUICKSAVE: campaign block invalid\n");
        return false;
    }
    if (!qs_take_record(c, p, "scalars", QS_REC_SCALARS, &p.scalars, sizeof(p.scalars), nullptr)
        || !qs_take_record(c, p, "pcom", QS_REC_PCOM, &p.pcom, sizeof(p.pcom), nullptr)
        || !qs_take_record(c, p, "camera", QS_REC_CAMERA, &p.camera, sizeof(p.camera), nullptr)
        || !qs_take_record(c, p, "grenades", QS_REC_GRENADES, &p.grenades, sizeof(p.grenades), nullptr)
        || !qs_take_record(c, p, "ribbons", QS_REC_RIBBONS, nullptr, sizeof(QS_Ribbons), &p.ribbons)
        || !qs_take_record(c, p, "hook", QS_REC_HOOK, nullptr, sizeof(QS_Hook), &p.hook)) {
        return false;
    }

    if (!qs_check_fixed_refs(p)) {
        return false;
    }

    for (uint32_t slot = 0; slot < QS_POOL_COUNT; slot++) {
        const uint32_t id = QS_POOL_IDS[slot];
        const MemTable& t = save_table[id];
        uint8_t hdr[sizeof(QS_PoolHeader)];
        if (!c.take(hdr, sizeof(hdr))) {
            return false;
        }
        const uint32_t pool_id = qs_get_u32(hdr);
        const uint32_t count = qs_get_u32(hdr + 4);
        const uint32_t elem = qs_get_u32(hdr + 8);
        const uint32_t bytes = qs_get_u32(hdr + 12);
        bool ok = pool_id == id && elem == QS_ELEMENT_BYTES[slot] && uint64_t(count) * elem == bytes
            && count <= uint32_t(t.Maximum) && bytes <= c.size - c.pos;
        if (ok && t.Type == MEM_STATIC) {
            ok = count == uint32_t(t.Maximum);
        } else if (ok) {
            ok = t.Type == MEM_DYNAMIC && qs_pool_has_cursor(t) && (t.CountL || count <= QS_MAX_CURSOR_W);
        }
        if (!ok) {
            fprintf(stderr, "QUICKSAVE: pool record %u (%s) invalid\n", slot, t.Name);
            return false;
        }
        p.sections.push_back({ t.Name, c.pos, bytes, elem });
        p.pools[slot].data = c.data + c.pos;
        p.pools[slot].count = count;
        c.pos += bytes;
        if (!c.skip_to_align() || !qs_check_pool_refs(id, p.pools[slot].data, count)) {
            return false;
        }
    }

    if (!qs_check_pool_bound_refs(p) || !qs_check_thing_tables(p) || !qs_check_record_links(p)) {
        return false;
    }

    uint8_t trailer[sizeof(QS_Trailer)];
    p.sections.push_back({ "trailer", c.pos, sizeof(trailer), 0 });
    if (!c.take(trailer, sizeof(trailer)) || qs_get_u32(trailer) != QS_END_MAGIC || qs_get_u32(trailer + 4) != 0 || c.pos != c.size) {
        fprintf(stderr, "QUICKSAVE: trailer invalid\n");
        return false;
    }
    return true;
}

// --- Apply. Only called after qs_parse() succeeded. ---

void qs_apply_scalars(const QS_Scalars& s)
{
    // GAME_STATE is forced to the value an ordinary restart leaves; GF_PAUSED never survives.
    GAME_STATE = GS_PLAY_GAME | GS_REPLAY;
    GAME_FLAGS = SLONG(s.game_flags) & ~GF_PAUSED;
    PERSON_COUNT = s.person_count;
    PLAYER_COUNT = s.player_count;
    ANIMAL_COUNT = s.animal_count;
    CHOPPER_COUNT = s.chopper_count;
    PYRO_COUNT = s.pyro_count;
    PROJECTILE_COUNT = s.projectile_count;
    SPECIAL_COUNT = s.special_count;
    SWITCH_COUNT = s.switch_count;
    PRIMARY_COUNT = s.primary_thing_count;
    SECONDARY_COUNT = s.secondary_thing_count;
    PRIMARY_USED = s.used_primary;
    PRIMARY_UNUSED = s.unused_primary;
    SECONDARY_USED = s.used_secondary;
    SECONDARY_UNUSED = s.unused_secondary;

    BARREL_sphere_last = s.barrel_sphere_last;
    track_head = s.track_head;
    track_tail = s.track_tail;
    track_eob = s.track_eob;
    NIGHT_dlight_free = s.night_dlight_free;
    NIGHT_dlight_used = s.night_dlight_used;
    first_walkable_prim_point = s.first_walkable_prim_point;
    number_of_walkable_prim_points = s.number_of_walkable_prim_points;
    first_walkable_prim_face4 = s.first_walkable_prim_face4;
    number_of_walkable_prim_faces4 = s.number_of_walkable_prim_faces4;
    NIGHT_amb_colour = s.night_amb_colour;
    NIGHT_amb_specular = s.night_amb_specular;
    NIGHT_amb_red = s.night_amb_red;
    NIGHT_amb_green = s.night_amb_green;
    NIGHT_amb_blue = s.night_amb_blue;
    NIGHT_amb_norm_x = s.night_amb_norm_x;
    NIGHT_amb_norm_y = s.night_amb_norm_y;
    NIGHT_amb_norm_z = s.night_amb_norm_z;
    NIGHT_flag = s.night_flag;
    NIGHT_lampost_radius = s.night_lampost_radius;
    NIGHT_lampost_red = s.night_lampost_red;
    NIGHT_lampost_green = s.night_lampost_green;
    NIGHT_lampost_blue = s.night_lampost_blue;
    NIGHT_sky_colour.red = s.night_sky_red;
    NIGHT_sky_colour.green = s.night_sky_green;
    NIGHT_sky_colour.blue = s.night_sky_blue;

    CRIME_RATE = s.crime_rate;
    CRIME_RATE_SCORE_MUL = s.crime_rate_score_mul;
    TEXTURE_SET = s.texture_set;
    MUSIC_WORLD = s.music_world;
    BOREDOM_RATE = s.boredom_rate;
    INDOORS_ROOM = s.indoors_room;
    INDOORS_INDEX_FADE = s.indoors_index_fade;
    slow_mo = s.slow_mo;
    WARE_in = s.ware_in;

    EWAY_time_accurate = s.eway_time_accurate;
    EWAY_count_up = s.eway_count_up;
    EWAY_count_up_visible = s.eway_count_up_visible;
    EWAY_count_up_add_penalties = s.eway_count_up_add_penalties;
    EWAY_count_up_num_penalties = s.eway_count_up_num_penalties;
    EWAY_count_up_penalty_timer = s.eway_count_up_penalty_timer;

    GAME_cut_scene = s.game_cut_scene;
    EWAY_darci_move = s.eway_darci_move;
    EWAY_used_thing = s.eway_used_thing;
    EWAY_tutorial_counter = s.eway_tutorial_counter;
    EWAY_tutorial_string = qs_mess_ptr(s.eway_tutorial_string_offset);
    EWAY_conv_active = s.eway_conv_active;
    EWAY_conv_waypoint = s.eway_conv_waypoint;
    EWAY_conv_person_a = s.eway_conv_person_a;
    EWAY_conv_person_b = s.eway_conv_person_b;
    EWAY_conv_str = s.eway_conv_str;
    EWAY_conv_str_count = s.eway_conv_str_count;
    EWAY_conv_timer = s.eway_conv_timer;
    EWAY_conv_skip = s.eway_conv_skip;
    EWAY_conv_ambient = s.eway_conv_ambient;
    EWAY_conv_talk = s.eway_conv_talk;
    talk_thing = qs_thing_ptr(s.talk_thing);
    for (uint32_t i = 0; i < QS_FAKE_WANDER_FIELDS; i++) {
        *qs_fake_wander_fields[i] = s.eway_fake_wander[i];
    }
    EWAY_cam_jumped = s.eway_cam_jumped;
    for (uint32_t i = 0; i < QS_EWAY_CAM_FIELDS; i++) {
        *qs_eway_cam_fields[i] = s.eway_cam[i];
    }
    EWAY_cam_thing = s.eway_cam_thing;

    PLAYCUTS_playing = s.playcuts_playing;
    no_more_packets = s.no_more_packets;
    PLAYCUTS_slomo = s.playcuts_slomo;
    PLAYCUTS_slomo_ctr = s.playcuts_slomo_ctr;
    PLAYCUTS_fade_level = s.playcuts_fade_level;

    stat_killed_thug = s.stat_killed_thug;
    stat_killed_innocent = s.stat_killed_innocent;
    stat_arrested_thug = s.stat_arrested_thug;
    stat_arrested_innocent = s.stat_arrested_innocent;
    stat_count_bonus = s.stat_count_bonus;
    stat_game_time = s.stat_game_time_ms;
    stat_start_time = qs_rebase(s.stat_game_time_ms, sdl3_get_ticks());

    global_person = s.global_person;
    timer_bored = s.timer_bored;
    combo_display = s.combo_display;
    kick_or_punch = s.kick_or_punch;
    just_started_falling_off_backwards = s.just_started_falling_off_backwards;

    // The one input latch that is restored at all: "released".
    input_actions_clear_ak47_reload_gate();
}

void qs_apply_pool(uint32_t slot, const QS_PoolView& v)
{
    const uint32_t id = QS_POOL_IDS[slot];
    MemTable& t = save_table[id];
    uint8_t* base = qs_pool_base(id);
    const uint32_t elem = QS_ELEMENT_BYTES[slot];

    switch (id) {
    case QS_POOL_THINGS:
        for (uint32_t i = 0; i < v.count; i++) {
            Thing th;
            qs_decode_thing(qs_load_elem<QS_Thing_Disk>(v.data, i), &th);
            qs_store_elem(base, i, th);
        }
        break;
    case QS_POOL_DRAWTWEENS:
        for (uint32_t i = 0; i < v.count; i++) {
            DrawTween dt;
            qs_decode_drawtween(qs_load_elem<QS_DrawTween_Disk>(v.data, i), &dt);
            qs_store_elem(base, i, dt);
        }
        break;
    case QS_POOL_VEHICLES:
        for (uint32_t i = 0; i < v.count; i++) {
            const QS_Vehicle_Disk d = qs_load_elem<QS_Vehicle_Disk>(v.data, i);
            Vehicle veh;
            memcpy((uint8_t*)&veh + sizeof(DrawTween), d.rest, sizeof(d.rest));
            qs_decode_drawtween(d.draw, &veh.Draw);
            qs_store_elem(base, i, veh);
        }
        break;
    case QS_POOL_PLAYERS:
        for (uint32_t i = 0; i < v.count; i++) {
            Player pl;
            qs_decode_player(qs_load_elem<QS_Player_Disk>(v.data, i), &pl);
            qs_store_elem(base, i, pl);
        }
        break;
    case QS_POOL_PYRO:
        for (uint32_t i = 0; i < v.count; i++) {
            Pyro py;
            qs_decode_pyro(qs_load_elem<QS_Pyro_Disk>(v.data, i), &py);
            qs_store_elem(base, i, py);
        }
        break;
    case QS_POOL_CHOPPERS:
        for (uint32_t i = 0; i < v.count; i++) {
            Chopper ch;
            qs_decode_chopper(qs_load_elem<QS_Chopper_Disk>(v.data, i), &ch);
            qs_store_elem(base, i, ch);
        }
        break;
    case QS_POOL_ANIMALS:
        for (uint32_t i = 0; i < v.count; i++) {
            Animal an;
            qs_decode_animal(qs_load_elem<QS_Animal_Disk>(v.data, i), &an);
            qs_store_elem(base, i, an);
        }
        break;
    case QS_POOL_EWAY_MESS:
        for (uint32_t i = 0; i < v.count; i++) {
            CBYTE* m = qs_mess_ptr(qs_load_elem<QS_MessRef>(v.data, i).offset);
            qs_store_elem(base, i, m);
        }
        break;
    case QS_POOL_MAP_BEACON: {
        const uint64_t now = sdl3_get_ticks();
        for (uint32_t i = 0; i < v.count; i++) {
            const QS_Beacon_Disk d = qs_load_elem<QS_Beacon_Disk>(v.data, i);
            MAP_Beacon b;
            memset(&b, 0, sizeof(b));
            memcpy(&b, d.prefix, QS_BEACON_PREFIX_BYTES);
            b.ticks = qs_rebase(d.age_ms, now);
            qs_store_elem(base, i, b);
        }
    } break;
    case QS_POOL_CUTSCENE_DATA:
        for (uint32_t i = 0; i < v.count; i++) {
            const QS_CPData_Disk d = qs_load_elem<QS_CPData_Disk>(v.data, i);
            CPData c;
            memset(&c, 0, sizeof(c));
            c.version = d.version;
            c.channelcount = d.channelcount;
            c.pad1 = d.pad1;
            c.pad2 = d.pad2;
            c.channels = (d.channels == QS_NULL_INDEX16) ? nullptr : (CPChannel*)qs_element(QS_POOL_CUTSCENE_TRKS, d.channels);
            qs_store_elem(base, i, c);
        }
        break;
    case QS_POOL_CUTSCENE_TRKS:
        for (uint32_t i = 0; i < v.count; i++) {
            const QS_CPChannel_Disk d = qs_load_elem<QS_CPChannel_Disk>(v.data, i);
            CPChannel c;
            memset(&c, 0, sizeof(c));
            c.type = d.type;
            c.flags = d.flags;
            c.pad1 = d.pad1;
            c.pad2 = d.pad2;
            c.index = d.index;
            c.packetcount = d.packetcount;
            c.packets = (d.packets == QS_NULL_INDEX16) ? nullptr : (CPPacket*)qs_element(QS_POOL_CUTSCENE_PKTS, d.packets);
            qs_store_elem(base, i, c);
        }
        break;
    default:
        memcpy(base, v.data, size_t(v.count) * elem);
        break;
    }

    // Capacity always comes from the compiled table, never from the file.
    if (t.Type == MEM_DYNAMIC) {
        qs_set_pool_cursor(t, SLONG(v.count));
    }
}

void qs_apply_statefn(const QS_Scalars& s)
{
    for (uint32_t i = 0; i < MAX_THINGS; i++) {
        Thing* t = TO_THING(i);
        const bool present = (s.statefn_present[i >> 3] >> (i & 7)) & 1;
        t->StateFn = present ? qs_derive_statefn(t) : nullptr;
    }
}

void qs_apply_pcom(const QS_Pcom& p)
{
    PCOM_gang_person_upto = p.gang_person_upto;
    for (uint32_t i = 0; i < PCOM_MAX_GANG_PEOPLE; i++) {
        PCOM_gang_person[i] = p.gang_person[i];
    }
    for (uint32_t i = 0; i < PCOM_MAX_GANGS; i++) {
        PCOM_gang[i].index = p.gang_index[i];
        PCOM_gang[i].number = p.gang_number[i];
    }
    for (uint32_t i = 0; i < PCOM_SUMMON_NUM_BODIES; i++) {
        PCOM_summon[i] = p.summon[i];
    }
    for (uint32_t i = 0; i < BAT_SUMMON_NUM_BODIES; i++) {
        BAT_summon[i] = p.bat_summon[i];
    }
    PCOM_runover_scary_person = qs_thing_ptr(p.runover_scary_person);
}

void qs_apply_camera(const QS_Camera& c)
{
    FC_Cam cam;
    memcpy(&cam, c.fc_cam.bytes, sizeof(cam));
    cam.focus = qs_thing_ptr(qs_get_u16(c.fc_cam.bytes + offsetof(FC_Cam, focus)));
    cam.fight_track_focus = qs_thing_ptr(qs_get_u16(c.fc_cam.bytes + offsetof(FC_Cam, fight_track_focus)));
    memcpy(&FC_cam[QS_LIVE_CAMERA], &cam, sizeof(cam));
    for (uint32_t i = 0; i < QS_SNIPE_FIELDS; i++) {
        *qs_snipe_fields[i] = c.snipe[i];
    }
}

void qs_apply_grenades(const QS_Grenades& g)
{
    for (uint32_t i = 0; i < MAX_GRENADES; i++) {
        const QS_Grenade_Disk& d = g.grenade[i];
        Grenade& dst = GrenadeArray[i];
        dst.owner = qs_thing_ptr(d.owner);
        dst.x = d.x;
        dst.y = d.y;
        dst.z = d.z;
        dst.yaw = d.yaw;
        dst.pitch = d.pitch;
        dst.dx = d.dx;
        dst.dy = d.dy;
        dst.dz = d.dz;
        dst.dyaw = d.dyaw;
        dst.dpitch = d.dpitch;
        dst.timer = d.timer;
        dst.rsvd = d.rsvd;
    }
    global_g = g.global_g ? &GrenadeArray[g.global_g - 1] : nullptr;
}

void qs_apply_hook(const uint8_t* raw)
{
    static QS_Hook h; // 5 KB: kept off the stack
    memcpy(&h, raw, sizeof(h));
    memcpy(HOOK_point, h.points, sizeof(h.points));
    HOOK_reeled = h.reeled;
    HOOK_state = h.state;
    HOOK_grapple_yaw = h.grapple_yaw;
    HOOK_grapple_pitch = h.grapple_pitch;
    HOOK_spin_x = h.spin_x;
    HOOK_spin_y = h.spin_y;
    HOOK_spin_z = h.spin_z;
    HOOK_spin_speed = h.spin_speed;
    HOOK_countdown = h.countdown;
}

#ifndef NDEBUG
// ---------------------------------------------------------------------------
// Debug-only round-trip verifier. Right after a load it re-encodes
// the live state with the writer's encoder and compares it, section by section and
// element by element, with the file that was just applied.
// ---------------------------------------------------------------------------

constexpr uint32_t QS_VERIFY_DUMP_BYTES = 8;

// Fields the load recomputes on purpose. Every entry needs its reason; an unexplained
// mismatch is a bug. `offset` is inside one element for a pool, inside the record otherwise.
struct QS_VerifySkip {
    const char* section;
    uint32_t offset;
    uint32_t bytes;
    const char* reason;
};

const QS_VerifySkip QS_VERIFY_SKIPS[] = {
    { "scalars", offsetof(QS_Scalars, game_state), sizeof(uint32_t), "the load forces GS_PLAY_GAME | GS_REPLAY" },
    { "scalars", offsetof(QS_Scalars, game_flags), sizeof(uint32_t), "the load clears GF_PAUSED" },
    { "scalars", offsetof(QS_Scalars, stat_game_time_ms), sizeof(uint32_t), "wall-clock span, re-measured at verify time" },
    { "scalars", offsetof(QS_Scalars, player_last_released_age_ms), sizeof(uint32_t) * MAX_PLAYERS * QS_PLAYER_BUTTONS, "wall-clock ages, re-measured at verify time" },
    { "map_beacon", offsetof(QS_Beacon_Disk, age_ms), sizeof(uint32_t), "wall-clock age, re-measured at verify time" },
};

bool qs_verify_skipped(const char* section, uint32_t offset)
{
    for (const QS_VerifySkip& s : QS_VERIFY_SKIPS) {
        if (strcmp(s.section, section) == 0 && offset >= s.offset && offset < s.offset + s.bytes) {
            return true;
        }
    }
    return false;
}

void qs_verify(const std::vector<uint8_t>& file, const std::vector<QS_Section>& file_sections)
{
    QS_Encoder live;
    if (!qs_encode_file(live)) {
        fprintf(stderr, "QUICKSAVE verify: re-encoding the live state failed\n");
        CONSOLE_text_en((CBYTE*)"VERIFY FAILED: ENCODE", QS_MESSAGE_MS);
        return;
    }
    uint32_t mismatches = 0;
    uint32_t bad_sections = 0;
    if (live.sections.size() != file_sections.size()) {
        fprintf(stderr, "QUICKSAVE verify: %u sections live, %u in the file\n", unsigned(live.sections.size()), unsigned(file_sections.size()));
        mismatches++;
        bad_sections++;
    }
    const size_t n = live.sections.size() < file_sections.size() ? live.sections.size() : file_sections.size();
    for (size_t si = 0; si < n; si++) {
        const QS_Section& fs = file_sections[si];
        const QS_Section& ls = live.sections[si];
        uint32_t section_mismatches = 0;
        if (fs.bytes != ls.bytes) {
            fprintf(stderr, "QUICKSAVE verify: %s: %u bytes in the file, %u live\n", fs.name, fs.bytes, ls.bytes);
            section_mismatches++;
        }
        const uint32_t elem = fs.element_bytes ? fs.element_bytes : fs.bytes;
        const uint32_t bytes = fs.bytes < ls.bytes ? fs.bytes : ls.bytes;
        for (uint32_t e = 0; elem && e * elem < bytes; e++) {
            const uint8_t* a = file.data() + fs.offset + e * elem;
            const uint8_t* b = live.buf.data() + ls.offset + e * elem;
            for (uint32_t o = 0; o < elem && e * elem + o < bytes; o++) {
                if (a[o] == b[o] || qs_verify_skipped(fs.name, o)) {
                    continue;
                }
                if (section_mismatches < QS_VERIFY_MAX_LINES_PER_SECTION) {
                    char fa[3 * QS_VERIFY_DUMP_BYTES + 1] = {};
                    char lb[3 * QS_VERIFY_DUMP_BYTES + 1] = {};
                    for (uint32_t k = 0; k < QS_VERIFY_DUMP_BYTES && o + k < elem && e * elem + o + k < bytes; k++) {
                        snprintf(fa + 3 * k, 4, "%02x ", a[o + k]);
                        snprintf(lb + 3 * k, 4, "%02x ", b[o + k]);
                    }
                    fprintf(stderr, "QUICKSAVE verify: %s[%u] +%u: file %s| live %s\n", fs.name, e, o, fa, lb);
                }
                section_mismatches++;
                break; // one line per element
            }
        }
        if (section_mismatches > QS_VERIFY_MAX_LINES_PER_SECTION) {
            fprintf(stderr, "QUICKSAVE verify: %s: %u mismatching elements in total\n", fs.name, section_mismatches);
        }
        if (section_mismatches) {
            mismatches += section_mismatches;
            bad_sections++;
        }
    }
    if (mismatches == 0) {
        fprintf(stderr, "QUICKSAVE verify: OK\n");
        CONSOLE_text_en((CBYTE*)"VERIFY OK", QS_MESSAGE_MS);
    } else {
        fprintf(stderr, "QUICKSAVE verify: %u mismatches in %u pools\n", mismatches, bad_sections);
        char msg[64];
        snprintf(msg, sizeof(msg), "VERIFY FAILED: %u", mismatches);
        CONSOLE_text_en(msg, QS_MESSAGE_MS);
    }
}
#endif // NDEBUG

// Reads, validates and applies the pending quick save over the freshly reloaded level.
bool qs_read_apply()
{
    std::vector<uint8_t> file;
    QS_Parsed p;
    if (!qs_read_file(file) || !qs_parse(file, p)) {
        return false;
    }

    // Voices started by the reload are attached to Things that are about to be
    // overwritten; stop them while their Things are still the right ones.
    for (uint32_t i = 0; i < MAX_THINGS; i++) {
        Thing* t = TO_THING(i);
        if (t->Flags & FLAGS_HAS_ATTACHED_SOUND) {
            MFX_stop_attached(t);
        }
    }

    // Step 1: scalars and the campaign payload.
    qs_apply_scalars(p.scalars);
    qs_apply_campaign(p.campaign);

    // Step 2: pools, with their pointer fields decoded on the way in.
    for (uint32_t slot = 0; slot < QS_POOL_COUNT; slot++) {
        qs_apply_pool(slot, p.pools[slot]);
    }

    // Step 3: the references that live outside the pools.
    for (uint32_t i = 0; i < MAX_PLAYERS; i++) {
        NET_PERSON(i) = qs_thing_ptr(p.scalars.net_person[i]);
        NET_PLAYER(i) = qs_thing_ptr(p.scalars.net_player[i]);
    }
    {
        const uint64_t now = sdl3_get_ticks();
        for (uint32_t pl = 0; pl < MAX_PLAYERS; pl++) {
            for (uint32_t b = 0; b < QS_PLAYER_BUTTONS; b++) {
                TO_PLAYER(pl)->LastReleased[b] = qs_rebase(p.scalars.player_last_released_age_ms[pl][b], now);
            }
        }
    }

    // Step 4: StateFn, after Genus is in place (set_state_function-style lookups read it).
    qs_apply_statefn(p.scalars);

    // Step 5: the AI gang tables, after the reload's PCOM_init wiped them.
    qs_apply_pcom(p.pcom);

    // Step 6: camera seat, after the reload's FC_init / FC_look_at.
    qs_apply_camera(p.camera);

    // Step 7: grenades, ribbons (without RIBBON_init, which would wipe them), hook, EWAY clock.
    qs_apply_grenades(p.grenades);
    memcpy(ribbon_Ribbons, p.ribbons, sizeof(QS_Ribbons));
    qs_apply_hook(p.hook);
    EWAY_time = EWAY_time_accurate >> QS_EWAY_TIME_SHIFT;
    EWAY_tick = QS_EWAY_TICK_SCALE * TICK_RATIO >> TICK_SHIFT;

    // Step 8: render-side prim data derived from the restored prim prefix.
    calc_prim_normals();
    AENG_create_dx_prim_points();

    // Step 9: derived caches and stale pointers.
    uncache();
    NIGHT_generate_walkable_lighting();
    for (SLONG i = 0; i < QS_SUPERMAP_COUNTERS; i++) {
        SUPERMAP_counter[i] = 0;
    }
    EWAY_magic_radius_flag = nullptr;
    PANEL_new_text_init();
    form_leave_map = nullptr;

#ifndef NDEBUG
    qs_verify(file, p.sections);
#endif
    return true;
}

// ---------------------------------------------------------------------------
// The save-allowed condition
// ---------------------------------------------------------------------------

// Ground distance within which an NPC engaged with the player blocks a save, in world
// units (WorldPos carries 8 fraction bits; one map square is 256 units). It is the default
// NPC sight range (can_a_see_b with range 0, things/characters/person.cpp), which also
// covers the longest enemy attack, sniper fire at 0x600 (PCOM_process_snipe, ai/pcom.cpp).
// Beyond it an NPC can neither see nor hit the player.
constexpr SLONG QS_WORLDPOS_FRACTION_BITS = 8;
constexpr SLONG QS_MAP_SQUARE = 1 << 8;
constexpr SLONG QS_ENEMY_ENGAGE_RANGE = 8 * QS_MAP_SQUARE;

static bool qs_within_engage_range(const Thing* npc, const Thing* me)
{
    const SLONG dx = abs((npc->WorldPos.X - me->WorldPos.X) >> QS_WORLDPOS_FRACTION_BITS);
    const SLONG dz = abs((npc->WorldPos.Z - me->WorldPos.Z) >> QS_WORLDPOS_FRACTION_BITS);
    return QDIST2(dx, dz) < QS_ENEMY_ENGAGE_RANGE;
}

QS_Refusal qs_allowed_now()
{
    if (!qs_flag_on()) {
        return QS_REFUSED_FLAG;
    }
    if (!(GAME_STATE & GS_PLAY_GAME) || (GAME_STATE & (GS_LEVEL_WON | GS_LEVEL_LOST)) || !the_game.net_persons) {
        return QS_REFUSED_NOT_IN_MISSION;
    }
    Thing* me = NET_PERSON(0);
    char tag[QS_LEVEL_TAG_BYTES];
    qs_level_tag(tag);
    if (!me || me->Class != CLASS_PERSON || !me->Genus.Person || !tag[0]) {
        return QS_REFUSED_NOT_IN_MISSION;
    }
    if (me->State == STATE_DEAD || me->State == STATE_DYING) {
        return QS_REFUSED_DEAD;
    }

    const Person* pp = me->Genus.Person;
    if (pp->InCar != 0 || (pp->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING | FLAG_PERSON_PASSENGER))
        || me->State == STATE_DRIVING
        || me->SubState == SUB_STATE_ENTERING_VEHICLE || me->SubState == SUB_STATE_INSIDE_VEHICLE
        || me->SubState == SUB_STATE_EXITING_VEHICLE || me->SubState == SUB_STATE_MOUNTING_BIKE
        || me->SubState == SUB_STATE_RIDING_BIKE || me->SubState == SUB_STATE_DISMOUNTING_BIKE) {
        return QS_REFUSED_VEHICLE;
    }

    if (me->State == STATE_FIGHTING || me->State == STATE_CIRCLING || me->State == STATE_GRAPPLING
        || me->State == STATE_CARRY || me->State == STATE_HIT_RECOIL || me->State == STATE_HIT
        || (pp->Flags & (FLAG_PERSON_GRAPPLING | FLAG_PERSON_CANNING | FLAG_PERSON_BARRELING | FLAG_PERSON_ARRESTED))) {
        return QS_REFUSED_COMBAT;
    }

    // An NPC engaged with the player: pcom_ai_arg holds the player's Thing number in
    // exactly these four states (the PCOM_set_person_ai_* setters, ai/pcom.cpp). These
    // states never time out, so an NPC that lost the player far away keeps them; only
    // one within QS_ENEMY_ENGAGE_RANGE blocks the save.
    const UWORD me_index = THING_NUMBER(me);
    UWORD i = thing_class_head[CLASS_PERSON];
    for (SLONG guard = 0; i && guard < MAX_THINGS; guard++) {
        const Thing* t = TO_THING(i);
        i = t->NextLink;
        if (t == me || t->State == STATE_DEAD || t->State == STATE_DYING || !t->Genus.Person) {
            continue;
        }
        const Person* q = t->Genus.Person;
        if (q->Flags & FLAG_PERSON_HELPLESS) {
            continue;
        }
        const bool engaging = q->pcom_ai_state == PCOM_AI_STATE_KILLING || q->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL
            || q->pcom_ai_state == PCOM_AI_STATE_ARREST || q->pcom_ai_state == PCOM_AI_STATE_SNIPE;
        if (engaging && q->pcom_ai_arg == me_index && qs_within_engage_range(t, me)) {
            return QS_REFUSED_ENEMY;
        }
    }

    if (PLAYCUTS_playing || EWAY_cam_active || GAME_cut_scene) {
        return QS_REFUSED_CUTSCENE;
    }
    if (EWAY_tutorial_string || form_leave_map) {
        return QS_REFUSED_OTHER;
    }
    return QS_OK;
}

// ---------------------------------------------------------------------------
// Menu body text
// ---------------------------------------------------------------------------

float qs_line_width(const char* text)
{
    float w = 0.0F;
    for (const char* c = text; *c; c++) {
        w += float(FontInfo[(UBYTE)*c].width);
    }
    return w;
}

// Draws centred lines from the license-clean menu atlas, scaled down together when the
// widest one does not fit the frame, revealed by the same fade-in line as the items.
void qs_draw_lines(const char* const* lines, SLONG count, float first_y, float spacing, UBYTE alpha)
{
    MENUFONT_AltScope alt_font;
    float widest = 0.0F;
    for (SLONG i = 0; i < count; i++) {
        const float w = qs_line_width(lines[i]);
        widest = w > widest ? w : widest;
    }
    const float scale = (widest > QS_TEXT_MAX_WIDTH) ? QS_TEXT_MAX_WIDTH / widest : 1.0F;
    const ULONG colour = ULONG(alpha) | (ULONG(alpha) << 8) | (ULONG(alpha) << 16) | (ULONG(alpha) << 24);
    for (SLONG i = 0; i < count; i++) {
        float x = QS_MENU_CENTRE_X - qs_line_width(lines[i]) * scale * 0.5F;
        const float y = first_y + float(i) * spacing;
        for (const char* c = lines[i]; *c; c++) {
            const CharData& cd = FontInfo[(UBYTE)*c];
            const float w = float(cd.width) * scale;
            const float h = float(cd.height) * scale;
            if (x >= 0.0F && x + w < QS_MENU_FRAME_W && y + h < QS_MENU_FRAME_H && x + w < MENUFONT_fadein_x) {
                PANEL_draw_quad(x, y, x + w, y + h, FontPage, colour, cd.x, cd.y, cd.ox, cd.oy);
            }
            x += w;
        }
    }
}

const char* qs_refusal_text(QS_Refusal r)
{
    switch (r) {
    case QS_REFUSED_VEHICLE:
        return QS_REASON_VEHICLE;
    case QS_REFUSED_COMBAT:
        return QS_REASON_COMBAT;
    case QS_REFUSED_ENEMY:
        return QS_REASON_ENEMY;
    case QS_REFUSED_CUTSCENE:
        return QS_REASON_CUTSCENE;
    default:
        return QS_REASON_OTHER;
    }
}

// Rewrites a pause / level-lost row from its pristine copy: our items go after item 1.
void qs_rewrite_row(UBYTE* row, const UBYTE* orig, bool add_save, bool add_load)
{
    UBYTE out[QS_MENU_ROW_WORDS] = {};
    SLONG n = 0;
    for (; n < QS_MENU_FIRST_INSERT && orig[n]; n++) {
        out[n] = orig[n];
    }
    if (add_save && n < QS_MENU_ROW_WORDS) {
        out[n++] = X_SAVE_GAME;
    }
    if (add_load && n < QS_MENU_ROW_WORDS) {
        out[n++] = X_LOAD_GAME;
    }
    for (SLONG i = QS_MENU_FIRST_INSERT; i < QS_MENU_ROW_WORDS && orig[i] && n < QS_MENU_ROW_WORDS; i++) {
        out[n++] = orig[i];
    }
    memcpy(row, out, QS_MENU_ROW_WORDS);
}

// The level-lost menu offers a load only for a save that matches current progress.
bool qs_lost_menu_offers_load()
{
    QS_Header h;
    QS_Campaign c;
    return qs_read_head(&h, &c) && qs_campaign_compare(c) == QS_CAMPAIGN_SAME;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API (missions/quicksave.h)
// ---------------------------------------------------------------------------

void QUICKSAVE_menu_hook(UBYTE* row, QS_MenuKind kind)
{
    if (!qs_flag_on()) {
        return;
    }
    s_menu_kind = kind;
    if (!row) {
        // NONE, SURE, WON, HELP_*: the table and the transient state stay as they are
        // (SURE must keep s_warn_pending). Called with NONE at startup, before any level.
        return;
    }

    // Entering the pause or the level-lost menu: forget the last message and any dialog.
    s_body = QS_BODY_NONE;
    s_refusal = QS_OK;
    s_warn_pending = false;

    if (kind == QS_MENU_PAUSE) {
        if (!s_pause_row_captured) {
            memcpy(s_pause_row, row, QS_MENU_ROW_WORDS);
            s_pause_row_captured = true;
        }
        qs_rewrite_row(row, s_pause_row, true, qs_exists_for_level());
    } else if (kind == QS_MENU_LOST) {
        if (!s_lost_row_captured) {
            memcpy(s_lost_row, row, QS_MENU_ROW_WORDS);
            s_lost_row_captured = true;
        }
        qs_rewrite_row(row, s_lost_row, false, qs_lost_menu_offers_load());
    }
}

void QUICKSAVE_draw_menu_body(void)
{
    if (!qs_flag_on()) {
        return;
    }
    switch (s_body) {
    case QS_BODY_SAVE_REFUSED:
    case QS_BODY_LOAD_FAILED: {
        // Only on the menu that produced it (not on a SURE/abandon dialog opened after).
        if (s_menu_kind == QS_MENU_OTHER) {
            return;
        }
        const char* line = (s_body == QS_BODY_LOAD_FAILED) ? QS_MSG_LOAD_FAILED : qs_refusal_text(s_refusal);
        qs_draw_lines(&line, 1, QS_REASON_Y, QS_WARN_LINE_SPACING, QS_REASON_ALPHA);
    } break;
    case QS_BODY_WARN_OLDER:
    case QS_BODY_WARN_DIFFERENT:
        if (!s_warn_pending || s_menu_kind != QS_MENU_OTHER) {
            return;
        }
        qs_draw_lines(s_body == QS_BODY_WARN_OLDER ? QS_WARN_OLDER : QS_WARN_DIFF, QS_WARN_LINES, QS_WARN_Y,
            QS_WARN_LINE_SPACING, QS_WARN_ALPHA);
        break;
    default:
        break;
    }
}

bool QUICKSAVE_menu_save(void)
{
    if (!qs_flag_on()) {
        return false;
    }
    // Re-checked here even though only the pause menu calls it: the module never trusts
    // its caller.
    const QS_Refusal r = qs_allowed_now();
    if (r != QS_OK) {
        s_refusal = r;
        s_body = QS_BODY_SAVE_REFUSED;
        return false;
    }
    if (!qs_write()) {
        s_refusal = QS_REFUSED_OTHER;
        s_body = QS_BODY_SAVE_REFUSED;
        return false;
    }
    s_body = QS_BODY_NONE;
    CONSOLE_text_en((CBYTE*)QS_MSG_SAVED, QS_MESSAGE_MS);
    return true;
}

QS_Load QUICKSAVE_menu_load(void)
{
    if (!qs_flag_on()) {
        return QS_LOAD_NONE;
    }
    s_warn_pending = false;

    // The whole file is validated before anything is destroyed: a missing, foreign,
    // truncated or corrupt save leaves the running mission alone.
    std::vector<uint8_t> file;
    static QS_Parsed parsed; // ~2 KB plus the section list: kept off the stack
    if (!qs_read_file(file) || !qs_parse(file, parsed)) {
        s_body = QS_BODY_LOAD_FAILED;
        return QS_LOAD_NONE;
    }

    const QS_CampaignMatch cmp = qs_campaign_compare(parsed.campaign);
    if (cmp == QS_CAMPAIGN_SAME) {
        s_body = QS_BODY_NONE;
        s_pending_restore = true;
        return QS_LOAD_NOW;
    }
    if (s_menu_kind != QS_MENU_PAUSE) {
        // The level-lost screen never asks; its item is offered only for a match.
        s_body = QS_BODY_LOAD_FAILED;
        return QS_LOAD_NONE;
    }
    s_body = (cmp == QS_CAMPAIGN_SAVED_OLDER) ? QS_BODY_WARN_OLDER : QS_BODY_WARN_DIFFERENT;
    s_warn_pending = true;
    return QS_LOAD_NEEDS_CONFIRM;
}

bool QUICKSAVE_confirm_load(void)
{
    if (!qs_flag_on() || !s_warn_pending) {
        return false;
    }
    s_warn_pending = false;
    s_body = QS_BODY_NONE;
    s_pending_restore = true;
    return true;
}

void QUICKSAVE_apply_pending(void)
{
    if (!s_pending_restore) {
        return;
    }
    s_pending_restore = false;
    if (!qs_flag_on()) {
        return;
    }
    if (!qs_read_apply()) {
        // Nothing was applied (validation precedes every write), so the freshly loaded
        // mission simply continues from its start.
        CONSOLE_text_en((CBYTE*)QS_MSG_LOAD_FAILED, QS_MESSAGE_MS);
        return;
    }
    CONSOLE_text_en((CBYTE*)QS_MSG_LOADED, QS_MESSAGE_MS);
}

void QUICKSAVE_clear_pending(void)
{
    s_pending_restore = false;
    s_warn_pending = false;
}
