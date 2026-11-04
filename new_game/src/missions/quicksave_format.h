#ifndef MISSIONS_QUICKSAVE_FORMAT_H
#define MISSIONS_QUICKSAVE_FORMAT_H

// In-mission quick save: on-disk format v1.
//
// Header-only: constants, disk records and the static_asserts that pin every live
// layout the format depends on. The code that fills and reads these records is in
// missions/quicksave.cpp.
//
// File layout (all offsets from the start of the file, every section 4-byte aligned):
//
//   0     QS_Header       128 bytes
//   128   QS_Campaign     271 bytes, then 1 pad byte
//   400   QS_RecordHeader + QS_Scalars                    (padded to 4)
//         QS_RecordHeader + QS_Pcom / QS_Camera / QS_Grenades / QS_Ribbons / QS_Hook
//         QS_POOL_COUNT x (QS_PoolHeader + elements, padded to 4), ascending save_table[] index
//         QS_Trailer      8 bytes
//
// Byte order is little-endian only (asserted below). The framing (header, record and
// pool headers, trailer) is written through the qs_put_* helpers; the fixed-width
// members of the packed records are stored natively, which on the asserted
// little-endian host is the same byte sequence.
//
// No host pointer is ever stored. The encodings (one convention, used everywhere):
//   - a Thing reference is its THING_INDEX, 0 = NULL (slot 0 is never allocated);
//   - another pool's element is (pool kind, element index), kind 0 = NULL;
//   - an EWAY_mess string is a byte offset into EWAY_mess_buffer, QS_NULL_OFFSET = NULL;
//   - an animation frame is a packed (kind, chunk slot, frame index) reference, 0 = NULL;
//   - Thing.StateFn is not stored: one "was set" bit per Thing in QS_Scalars;
//   - a wall-clock stamp (sdl3_get_ticks()) is stored as an age in ms, rebased on load.

#include <bit>
#include <cstddef>
#include <cstdint>

#include "things/core/thing.h" // Thing, DrawTween, DrawMesh and every Genus pool type
#include "game/game_types.h"
#include "map/ob.h"
#include "map/pap.h"
#include "map/supermap.h" // DFacet
#include "map/level_pools.h" // PrimPoint, PrimFace4, PrimFace3, PrimObject, PrimMultiObject
#include "navigation/wmove.h"
#include "ai/mav_globals.h" // MAV_Opt
#include "engine/graphics/lighting/night.h" // NIGHT_Dlight
#include "things/items/barrel.h" // BARREL_Sphere
#include "missions/eway_globals.h" // EWAY_Way, EWAY_Cond, EWAY_Edef
#include "things/items/balloon.h"
#include "world_objects/tripwire.h"
#include "world_objects/door.h"
#include "missions/playcuts.h" // CPData, CPChannel, CPPacket
#include "missions/memory_globals.h" // MAP_Beacon
#include "missions/save.h" // SAVE_TABLE_PLATS, SAVE_TABLE_WMOVE
#include "camera/fc.h" // FC_Cam
#include "effects/environment/ribbon.h" // Ribbon, MAX_RIBBONS
#include "things/items/grenade_globals.h" // Grenade, MAX_GRENADES
#include "things/items/hook_globals.h" // HOOK_Point
#include "ai/pcom.h" // PCOM_MAX_GANG_PEOPLE, PCOM_MAX_GANGS
#include "ai/pcom_globals.h" // PCOM_SUMMON_NUM_BODIES
#include "things/animals/bat_globals.h" // BAT_SUMMON_NUM_BODIES

// The format is defined as little-endian; a big-endian port would need byte-swapping
// accessors for every record before it could read or write a quick save.
static_assert(unsigned(std::endian::native) == unsigned(std::endian::little),
    "quicksave format v1 assumes a little-endian host");

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

constexpr uint32_t qs_fourcc(char a, char b, char c, char d)
{
    return uint32_t(uint8_t(a)) | (uint32_t(uint8_t(b)) << 8) | (uint32_t(uint8_t(c)) << 16)
        | (uint32_t(uint8_t(d)) << 24);
}

// File identity: "OCQS" = OpenChaos Quick Save.
constexpr uint32_t QS_MAGIC = qs_fourcc('O', 'C', 'Q', 'S');
// Bumped whenever the pool list or any disk record changes; the reader keeps only the
// current version and refuses every other one.
constexpr uint32_t QS_FORMAT_VERSION = 1;
constexpr uint32_t QS_HEADER_BYTES = 128;
// Written as a u16; a byte-swapped reader would see 0x3412.
constexpr uint16_t QS_ENDIAN_MARKER = 0x1234;
constexpr uint16_t QS_POINTER_BITS = uint16_t(8 * sizeof(void*));
// Trailer magic "QSEN" = Quick Save END.
constexpr uint32_t QS_END_MAGIC = qs_fourcc('Q', 'S', 'E', 'N');
// Every section starts on this boundary; the writer pads with zero bytes.
constexpr uint32_t QS_SECTION_ALIGN = 4;
// Upper bound on the payload a reader accepts, checked before the payload buffer is
// allocated. A v1 file is ~3.5 MB, ~3.3 MB of it pool data; the bound
// leaves 4x headroom for larger levels while refusing a corrupt size field.
constexpr uint32_t QS_MAX_PAYLOAD_BYTES = 16u * 1024u * 1024u;
// Fixed-size text fields of the header.
constexpr uint32_t QS_LEVEL_TAG_BYTES = 40;
constexpr uint32_t QS_LEVEL_PATH_BYTES = 48;
constexpr uint32_t QS_HEADER_RESERVED_BYTES = 8;

// Record magics of the fixed records that follow the campaign block.
constexpr uint32_t QS_REC_SCALARS = qs_fourcc('S', 'C', 'A', 'L');
constexpr uint32_t QS_REC_PCOM = qs_fourcc('P', 'C', 'O', 'M');
constexpr uint32_t QS_REC_CAMERA = qs_fourcc('C', 'A', 'M', 'R');
constexpr uint32_t QS_REC_GRENADES = qs_fourcc('G', 'R', 'E', 'N');
constexpr uint32_t QS_REC_RIBBONS = qs_fourcc('R', 'I', 'B', 'N');
constexpr uint32_t QS_REC_HOOK = qs_fourcc('H', 'O', 'O', 'K');

// Byte-offset NULL for EWAY_mess entries and the tutorial string (the original
// writer's convention for EWAY_mess).
constexpr uint32_t QS_NULL_OFFSET = 0xFFFFFFFFu;
// u16 element-index NULL for the cutscene channel/packet links.
constexpr uint16_t QS_NULL_INDEX16 = 0xFFFF;

// Campaign payload version: the same value FRONTEND_save_savegame writes into a .wag slot.
constexpr uint8_t QS_CAMPAIGN_VERSION = 3;
constexpr uint32_t QS_MISSION_HIERARCHY_BYTES = 60; // mission_hierarchy[], ui/frontend/frontend_globals.h
constexpr uint32_t QS_BEST_FOUND_MISSIONS = 50; // best_found[][], ui/frontend/frontend_globals.h
constexpr uint32_t QS_BEST_FOUND_SLOTS = 4;
constexpr uint32_t QS_CHARACTER_STATS = 4; // Strength, Constitution, Skill, Stamina

// Animation frame reference: bits 31-30 kind, 29-24 chunk
// slot, 23-0 frame index; 0 = NULL.
constexpr uint32_t QS_FRAME_KIND_SHIFT = 30;
constexpr uint32_t QS_FRAME_SLOT_SHIFT = 24;
constexpr uint32_t QS_FRAME_SLOT_MASK = 0x3F;
constexpr uint32_t QS_FRAME_INDEX_MASK = 0x00FFFFFF;
// Chunk kinds, shared with the existing TheChunk encoding (convert_drawtype_to_pointer, missions/memory.cpp).
constexpr uint32_t QS_CHUNK_KIND_NONE = 0;
constexpr uint32_t QS_CHUNK_KIND_GAME = 1; // game_chunk[]
constexpr uint32_t QS_CHUNK_KIND_ANIM = 2; // anim_chunk[]
// DrawTween.TheChunk is (kind << QS_CHUNK_KIND_SHIFT) | slot, as the existing reader decodes it.
constexpr uint32_t QS_CHUNK_KIND_SHIFT = 16;
constexpr uint32_t QS_CHUNK_SLOT_MASK = 0xFFFF;

// Thing.Draw kinds.
constexpr uint16_t QS_DRAW_NONE = 0;
constexpr uint16_t QS_DRAW_TWEEN = 1; // DRAW_TWEENS[]
constexpr uint16_t QS_DRAW_MESH = 2; // DRAW_MESHES[]

// Thing.Genus kind: save_table[] index of the pool + 1; 0 = NULL.
constexpr uint16_t QS_GENUS_NONE = 0;

// One StateFn bit per Thing.
constexpr uint32_t QS_STATEFN_BITMAP_BYTES = (MAX_THINGS + 7) / 8;

// Wall-clock ages of Player.LastReleased[]: MAX_PLAYERS x 16 buttons (things/core/player.h).
constexpr uint32_t QS_PLAYER_BUTTONS = 16;

// The EWAY camera scalars (missions/eway_globals.h), stored in the order of
// qs_eway_cam_fields[] in quicksave.cpp.
constexpr uint32_t QS_EWAY_CAM_FIELDS = 30;
// The eight SNIPE_* scope scalars (things/characters/snipe_globals.h).
constexpr uint32_t QS_SNIPE_FIELDS = 8;
// The six EWAY_fake_wander_text_* values (missions/eway_globals.h).
constexpr uint32_t QS_FAKE_WANDER_FIELDS = 6;
// Only camera 0 is live (FC_cam[1] is unused in the shipped game, camera/fc.h).
constexpr uint32_t QS_LIVE_CAMERA = 0;

// ---------------------------------------------------------------------------
// The pool list: literal save_table[] indices of the 46 pools that are written.
// The ones missions/save.h names (SAVE_TABLE_PLATS, SAVE_TABLE_WMOVE) are checked
// against that header below.
// ---------------------------------------------------------------------------

constexpr uint32_t QS_POOL_COUNT = 46;
constexpr uint32_t QS_SAVE_TABLE_ENTRIES = 74; // save_table[], missions/memory_globals.cpp

// Indices of the pools the code treats specially.
constexpr uint32_t QS_POOL_DFACETS = 6;
constexpr uint32_t QS_POOL_PRIM_POINTS = 15;
constexpr uint32_t QS_POOL_PRIM_FACES4 = 16;
constexpr uint32_t QS_POOL_EWAY_MESS = 22;
constexpr uint32_t QS_POOL_EWAY_MESS_BUF = 23;
constexpr uint32_t QS_POOL_EWAY_WAY = 26;
constexpr uint32_t QS_POOL_VEHICLES = 29;
constexpr uint32_t QS_POOL_PEOPLE = 30;
constexpr uint32_t QS_POOL_ANIMALS = 31;
constexpr uint32_t QS_POOL_CHOPPERS = 32;
constexpr uint32_t QS_POOL_PYRO = 33;
constexpr uint32_t QS_POOL_PLAYERS = 34;
constexpr uint32_t QS_POOL_PROJECTILES = 35;
constexpr uint32_t QS_POOL_SPECIALS = 36;
constexpr uint32_t QS_POOL_SWITCHES = 37;
constexpr uint32_t QS_POOL_BATS = 38;
constexpr uint32_t QS_POOL_THINGS = 39;
constexpr uint32_t QS_POOL_DRAWTWEENS = 40;
constexpr uint32_t QS_POOL_DRAWMESHES = 41;
constexpr uint32_t QS_POOL_BARRELS = 43;
constexpr uint32_t QS_POOL_PLATS = SAVE_TABLE_PLATS;
constexpr uint32_t QS_POOL_TRACKS = 50;
constexpr uint32_t QS_POOL_THING_HEADS = 62;
constexpr uint32_t QS_POOL_MAP_BEACON = 64;
constexpr uint32_t QS_POOL_CUTSCENE_DATA = 65;
constexpr uint32_t QS_POOL_CUTSCENE_TRKS = 66;
constexpr uint32_t QS_POOL_CUTSCENE_PKTS = 67;
constexpr uint32_t QS_POOL_CUTSCENE_TEXT = 68;

constexpr uint32_t QS_POOL_IDS[QS_POOL_COUNT] = {
    1, // Pap_Lo
    6, // dfacets
    15, 16, 17, 18, 19, // prim_points, prim_faces4, prim_faces3, prim_objects, prim_Mobjects
    20, 21, // ob_ob, ob_ mapwho
    22, 23, 24, 25, 26, 27, 28, // EWAY_mess, EWAY_mess buf, EWAY_timer, EWAY_cond, EWAY_way, EWAY_edef, EWAY_counter
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, // vehicles .. bats
    39, 40, 41, // thing, drawtween, drawmesh
    42, 43, 44, 45, 46, 47, // barrelsphere, barrels, plat, wmove, mav_opt, mav_nav
    49, 50, // balloons, tracks
    55, // night_dlight
    60, // Trip_Wire
    62, // Thing_heads
    64, // map_beacon
    65, 66, 67, 68, // cutscene_data, cutscene_trks, cutscene_pkts, cutscene_text
    71, // Doors-gates
};

// Slot of a save_table[] index in QS_POOL_IDS; QS_POOL_COUNT when the pool is not written.
constexpr uint32_t qs_pool_slot(uint32_t id)
{
    for (uint32_t i = 0; i < QS_POOL_COUNT; ++i) {
        if (QS_POOL_IDS[i] == id) {
            return i;
        }
    }
    return QS_POOL_COUNT;
}

// ---------------------------------------------------------------------------
// Disk records. All packed, fixed-width, filled field by field from zero.
// ---------------------------------------------------------------------------

#pragma pack(push, 1)

struct QS_Header {
    char magic[4]; // 'O','C','Q','S'
    uint32_t format_version; // QS_FORMAT_VERSION
    uint32_t header_bytes; // QS_HEADER_BYTES
    uint16_t endian_marker; // QS_ENDIAN_MARKER
    uint16_t pointer_bits; // 8 * sizeof(void*) of the writer
    uint32_t flags; // reserved, 0
    uint32_t table_fingerprint; // CRC32 of save_table[] (Name, StructSize, Maximum)
    uint32_t payload_bytes; // file size - QS_HEADER_BYTES
    uint32_t payload_crc; // CRC32 of bytes [QS_HEADER_BYTES, file size)
    char level_tag[QS_LEVEL_TAG_BYTES]; // NUL-padded lowercase tag (quicksave.cpp qs_level_tag)
    char level_path[QS_LEVEL_PATH_BYTES]; // ELEV_fname_level, diagnostics only, never compared
    uint8_t reserved[QS_HEADER_RESERVED_BYTES];
};
static_assert(sizeof(QS_Header) == QS_HEADER_BYTES);
static_assert(offsetof(QS_Header, format_version) == 4);
static_assert(offsetof(QS_Header, header_bytes) == 8);
static_assert(offsetof(QS_Header, endian_marker) == 12);
static_assert(offsetof(QS_Header, pointer_bits) == 14);
static_assert(offsetof(QS_Header, flags) == 16);
static_assert(offsetof(QS_Header, table_fingerprint) == 20);
static_assert(offsetof(QS_Header, payload_bytes) == 24);
static_assert(offsetof(QS_Header, payload_crc) == 28);
static_assert(offsetof(QS_Header, level_tag) == 32);
static_assert(offsetof(QS_Header, level_path) == 72);
static_assert(offsetof(QS_Header, reserved) == 120);

// The campaign payload: the fields FRONTEND_save_savegame writes to a .wag slot
// (ui/frontend/frontend.cpp), in the same order, minus the title string.
struct QS_Campaign {
    uint8_t complete_point;
    uint8_t version; // QS_CAMPAIGN_VERSION
    uint8_t darci[QS_CHARACTER_STATS]; // Strength, Constitution, Skill, Stamina
    uint8_t roper[QS_CHARACTER_STATS];
    uint8_t darci_dead_civ_warnings;
    uint8_t mission_hierarchy[QS_MISSION_HIERARCHY_BYTES];
    uint8_t best_found[QS_BEST_FOUND_MISSIONS * QS_BEST_FOUND_SLOTS];
};
static_assert(sizeof(QS_Campaign) == 271);
// The campaign block starts right after the header; the scalar record at the next
// aligned offset (128 + 271 = 399, padded to 400).
constexpr uint32_t QS_CAMPAIGN_OFFSET = QS_HEADER_BYTES;

// Precedes every fixed record after the campaign block.
struct QS_RecordHeader {
    uint32_t magic; // QS_REC_*
    uint32_t bytes; // sizeof of the record that follows
};
static_assert(sizeof(QS_RecordHeader) == 8);

// Precedes every pool's elements.
struct QS_PoolHeader {
    uint32_t pool_id; // save_table[] index
    uint32_t count; // elements written: Maximum for MEM_STATIC, the live cursor for MEM_DYNAMIC
    uint32_t element_bytes; // QS_ELEMENT_BYTES[slot]
    uint32_t payload_bytes; // count * element_bytes, without the alignment padding
};
static_assert(sizeof(QS_PoolHeader) == 16);

struct QS_Trailer {
    uint32_t end_magic; // QS_END_MAGIC
    uint32_t unused; // 0
};
static_assert(sizeof(QS_Trailer) == 8);

// --- The scalar block: game state that lives in plain globals rather than in a pool ---

struct QS_Scalars {
    // Pool and Thing bookkeeping (game/game_types.h).
    uint32_t game_state; // diagnostics only: the load forces GS_PLAY_GAME | GS_REPLAY
    uint32_t game_flags;
    int32_t person_count;
    int32_t player_count;
    int32_t animal_count;
    int32_t chopper_count;
    int32_t pyro_count;
    int32_t projectile_count;
    int32_t special_count;
    int32_t switch_count;
    int32_t primary_thing_count;
    int32_t secondary_thing_count;
    uint16_t used_primary;
    uint16_t unused_primary;
    uint16_t used_secondary;
    uint16_t unused_secondary;

    // World scalars that ride with in-file pools.
    int32_t barrel_sphere_last; // things/items/barrel_globals.h
    uint16_t track_head; // effects/environment/tracks_globals.h
    uint16_t track_tail;
    uint16_t track_eob;
    uint8_t night_dlight_free; // engine/graphics/lighting/night_globals.h
    uint8_t night_dlight_used;
    int32_t first_walkable_prim_point; // map/supermap_globals.h
    int32_t number_of_walkable_prim_points;
    int32_t first_walkable_prim_face4;
    int32_t number_of_walkable_prim_faces4;
    uint32_t night_amb_colour; // engine/graphics/lighting/night_globals.h
    uint32_t night_amb_specular;
    int32_t night_amb_red;
    int32_t night_amb_green;
    int32_t night_amb_blue;
    int32_t night_amb_norm_x;
    int32_t night_amb_norm_y;
    int32_t night_amb_norm_z;
    uint32_t night_flag;
    uint8_t night_lampost_radius;
    int8_t night_lampost_red;
    int8_t night_lampost_green;
    int8_t night_lampost_blue;
    uint8_t night_sky_red;
    uint8_t night_sky_green;
    uint8_t night_sky_blue;

    // Game scalars.
    int32_t crime_rate;
    int32_t crime_rate_score_mul;
    uint16_t texture_set;
    uint8_t music_world;
    uint8_t boredom_rate;
    uint16_t indoors_room;
    uint16_t indoors_index_fade;
    uint16_t slow_mo;
    uint8_t ware_in;

    // Script clock and mission timer. EWAY_time / EWAY_tick are derived on load.
    int32_t eway_time_accurate;
    int32_t eway_count_up;
    uint8_t eway_count_up_visible;
    uint8_t eway_count_up_add_penalties;
    int16_t eway_count_up_num_penalties;
    uint16_t eway_count_up_penalty_timer;

    // EWAY script scalars.
    uint8_t game_cut_scene;
    uint8_t eway_darci_move;
    uint16_t eway_used_thing;
    int32_t eway_tutorial_counter;
    uint32_t eway_tutorial_string_offset; // into EWAY_mess_buffer, QS_NULL_OFFSET = NULL
    uint8_t eway_conv_active;
    uint16_t eway_conv_waypoint;
    uint16_t eway_conv_person_a;
    uint16_t eway_conv_person_b;
    uint16_t eway_conv_str;
    uint16_t eway_conv_str_count;
    int32_t eway_conv_timer;
    int32_t eway_conv_skip;
    int32_t eway_conv_ambient;
    int32_t eway_conv_talk;
    uint16_t talk_thing; // THING_INDEX, 0 = NULL
    uint16_t eway_fake_wander[QS_FAKE_WANDER_FIELDS];
    int32_t eway_cam_jumped;

    // EWAY camera: saved even though saving in a cutscene is refused, so a
    // later relaxation needs no format change.
    int32_t eway_cam[QS_EWAY_CAM_FIELDS];
    uint16_t eway_cam_thing;

    // Cutscene playback, same reasoning.
    uint8_t playcuts_playing;
    uint8_t no_more_packets;
    uint8_t playcuts_slomo;
    uint8_t playcuts_slomo_ctr;
    uint8_t playcuts_fade_level;

    // Live player Things: two slots, never the 10-slot save_table entries 2-3.
    uint16_t net_person[MAX_PLAYERS];
    uint16_t net_player[MAX_PLAYERS];

    // Statistics. The live play time is a uint64_t wall-clock span; it is
    // narrowed to 32 bits (49.7 days) and widened again on load.
    int32_t stat_killed_thug;
    int32_t stat_killed_innocent;
    int32_t stat_arrested_thug;
    int32_t stat_arrested_innocent;
    int32_t stat_count_bonus;
    uint32_t stat_game_time_ms;

    // Wall-clock rebase: saturated age of Player.LastReleased[] per button.
    uint32_t player_last_released_age_ms[MAX_PLAYERS][QS_PLAYER_BUTTONS];

    // Per-Thing "StateFn was non-NULL" bitmap.
    uint8_t statefn_present[QS_STATEFN_BITMAP_BYTES];

    // Misc.
    uint8_t global_person;
    uint32_t timer_bored;
    uint8_t combo_display;
    int32_t kick_or_punch;
    uint8_t just_started_falling_off_backwards;
};
static_assert(sizeof(QS_Scalars) == 603, "update this number with the struct");

// --- The extra records ---

struct QS_Pcom {
    int32_t gang_person_upto;
    uint16_t gang_person[PCOM_MAX_GANG_PEOPLE];
    uint8_t gang_index[PCOM_MAX_GANGS];
    uint8_t gang_number[PCOM_MAX_GANGS];
    uint16_t summon[PCOM_SUMMON_NUM_BODIES];
    uint16_t bat_summon[BAT_SUMMON_NUM_BODIES];
    uint16_t runover_scary_person; // THING_INDEX, 0 = NULL
};
static_assert(sizeof(QS_Pcom) == 4 + 2 * PCOM_MAX_GANG_PEOPLE + 2 * PCOM_MAX_GANGS + 2 * PCOM_SUMMON_NUM_BODIES + 2 * BAT_SUMMON_NUM_BODIES + 2);

// FC_Cam as a raw image of the live struct with the two Thing pointers replaced by a
// THING_INDEX plus zero padding and the three GAME_TURN stamps zeroed (GAME_TURN
// restarts at 0 on the reload, so 0 means "long ago / inactive").
struct QS_FC_Cam_Disk {
    uint8_t bytes[sizeof(FC_Cam)];
};
static_assert(sizeof(QS_FC_Cam_Disk) == sizeof(FC_Cam));
static_assert(offsetof(FC_Cam, focus) == 0);
static_assert(sizeof(((FC_Cam*)nullptr)->focus) == 8);
static_assert(sizeof(((FC_Cam*)nullptr)->fight_track_focus) == 8);
static_assert(alignof(FC_Cam) <= 8);

struct QS_Camera {
    QS_FC_Cam_Disk fc_cam;
    int32_t snipe[QS_SNIPE_FIELDS];
};

struct QS_Grenade_Disk {
    uint16_t owner; // THING_INDEX, 0 = NULL
    uint16_t pad0;
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t yaw;
    int16_t pitch;
    int32_t dx;
    int32_t dy;
    int32_t dz;
    int16_t dyaw;
    int16_t dpitch;
    uint16_t timer;
    uint16_t rsvd;
};
static_assert(sizeof(QS_Grenade_Disk) == 40);

struct QS_Grenades {
    QS_Grenade_Disk grenade[MAX_GRENADES];
    uint16_t global_g; // GrenadeArray index + 1, 0 = NULL
};

// The ribbon pool is pointer-free: a raw image of the live array.
static_assert(sizeof(Ribbon) == 408);
static_assert(alignof(Ribbon) <= 8);
struct QS_Ribbons {
    uint8_t bytes[MAX_RIBBONS * sizeof(Ribbon)];
};

// HOOK_Point is pointer-free (20 bytes); the rope is a raw image.
static_assert(sizeof(HOOK_Point) == 20);
constexpr uint32_t QS_HOOK_POINTS = 256; // things/items/hook_globals.h
static_assert(sizeof(HOOK_point) == QS_HOOK_POINTS * sizeof(HOOK_Point));
struct QS_Hook {
    uint8_t points[QS_HOOK_POINTS * sizeof(HOOK_Point)];
    uint8_t reeled;
    uint8_t state;
    int32_t grapple_yaw;
    int32_t grapple_pitch;
    int32_t spin_x;
    int32_t spin_y;
    int32_t spin_z;
    int32_t spin_speed;
    int32_t countdown;
};
static_assert(sizeof(QS_Hook) == 5150);

// --- Pool records of the pointer-bearing types ---

// EWAY_mess element: byte offset into EWAY_mess_buffer, QS_NULL_OFFSET = NULL.
struct QS_MessRef {
    uint32_t offset;
};
static_assert(sizeof(QS_MessRef) == 4);

struct QS_DrawTween_Disk {
    uint8_t tweak_speed;
    int8_t locked;
    uint8_t frame_index;
    uint8_t queued_frame_index;
    int16_t angle;
    int16_t angle_to;
    int16_t roll;
    int16_t droll;
    int16_t tilt;
    int16_t tilt_to;
    int32_t current_anim;
    int32_t anim_tween;
    int32_t tween_stage;
    uint32_t pad0;
    uint32_t current_frame; // packed frame reference
    uint32_t pad1;
    uint32_t next_frame;
    uint32_t pad2;
    uint32_t interrupt_frame;
    uint32_t pad3;
    uint32_t queued_frame;
    uint32_t pad4;
    uint32_t the_chunk; // (kind << QS_CHUNK_KIND_SHIFT) | slot, 0 = NULL
    uint32_t pad5;
    uint8_t flags;
    uint8_t drawn;
    uint8_t mesh_id;
    uint8_t person_id;
    uint32_t pad6;
};
static_assert(sizeof(QS_DrawTween_Disk) == sizeof(DrawTween));
static_assert(sizeof(DrawTween) == 80);
static_assert(offsetof(DrawTween, CurrentFrame) == 32);
static_assert(offsetof(DrawTween, NextFrame) == 40);
static_assert(offsetof(DrawTween, InterruptFrame) == 48);
static_assert(offsetof(DrawTween, QueuedFrame) == 56);
static_assert(offsetof(DrawTween, TheChunk) == 64);

struct QS_Thing_Disk {
    uint8_t class_;
    uint8_t state;
    uint8_t old_state;
    uint8_t sub_state;
    uint32_t flags; // FLAGS_HAS_ATTACHED_SOUND is always written clear (no voice survives a load)
    uint16_t child;
    uint16_t parent;
    uint16_t link_child;
    uint16_t link_parent;
    int32_t x;
    int32_t y;
    int32_t z;
    uint32_t pad0;
    uint64_t state_fn; // always 0: see QS_Scalars.statefn_present
    uint16_t draw_kind; // QS_DRAW_*
    uint16_t draw_index;
    uint32_t pad1;
    uint16_t genus_kind; // save_table[] index of the pool + 1, QS_GENUS_NONE = NULL
    uint16_t genus_index;
    uint32_t pad2;
    uint8_t draw_type;
    uint8_t lead;
    int16_t velocity;
    int16_t delta_velocity;
    int16_t required_velocity;
    int16_t dy;
    uint16_t index;
    int16_t on_face;
    uint16_t next_link;
    uint16_t dog_poo1;
    uint16_t dog_poo2;
    uint32_t pad3;
};
static_assert(sizeof(QS_Thing_Disk) == sizeof(Thing));
static_assert(sizeof(Thing) == 80);
static_assert(offsetof(Thing, StateFn) == 32);
static_assert(offsetof(Thing, Draw) == 40);
static_assert(offsetof(Thing, Genus) == 48);

struct QS_Vehicle_Disk {
    QS_DrawTween_Disk draw; // Vehicle.Draw is an embedded DrawTween at offset 0
    uint8_t rest[sizeof(Vehicle) - sizeof(DrawTween)]; // pointer-free remainder, raw
};
static_assert(offsetof(Vehicle, Draw) == 0);
static_assert(sizeof(QS_Vehicle_Disk) == sizeof(Vehicle));
static_assert(sizeof(Vehicle) == 232);

struct QS_Player_Disk {
    uint8_t player_type;
    uint8_t padding;
    uint16_t thing;
    uint32_t flags;
    uint32_t input;
    uint32_t input_done;
    uint16_t player_id;
    uint8_t stamina;
    uint8_t constitution;
    uint32_t last_input;
    uint32_t this_input;
    uint32_t pressed;
    uint32_t released;
    uint32_t done_something;
    uint64_t last_released[QS_PLAYER_BUTTONS]; // always 0: the ages live in QS_Scalars
    uint8_t double_click[QS_PLAYER_BUTTONS];
    uint8_t strength;
    uint8_t red_marks;
    uint8_t traffic_violations;
    uint8_t danger;
    uint8_t popup_fade;
    int8_t item_focus;
    uint8_t item_count;
    uint8_t skill;
    uint64_t camera_thing; // always 0: nothing uses Player.CameraThing
    uint16_t player_person; // THING_INDEX, 0 = NULL
    uint8_t pad0[6];
};
static_assert(sizeof(QS_Player_Disk) == sizeof(Player));
static_assert(sizeof(Player) == 208);
static_assert(offsetof(Player, LastReleased) == 40);
static_assert(offsetof(Player, PlayerPerson) == 200);

struct QS_Pyro_Disk {
    uint16_t flags;
    uint8_t pyro_type;
    uint8_t dlight;
    uint32_t pad0;
    uint16_t thing; // THING_INDEX, 0 = NULL
    uint8_t pad1[6];
    uint16_t victim; // THING_INDEX, 0 = NULL
    uint8_t pad2[6];
    int32_t target_x;
    int32_t target_y;
    int32_t target_z;
    int32_t radius;
    int32_t counter;
    uint32_t radii[8];
    int32_t scale;
    int32_t tints[2];
    int32_t soundid;
    int16_t timer1;
    uint16_t dummy;
    uint8_t last_smoke_spawn;
    uint8_t pad3[3];
    int32_t last_draw_counter;
};
static_assert(sizeof(QS_Pyro_Disk) == sizeof(Pyro));
static_assert(sizeof(Pyro) == 104);
static_assert(offsetof(Pyro, thing) == 8);
static_assert(offsetof(Pyro, victim) == 16);

struct QS_Chopper_Disk {
    uint16_t thing; // THING_INDEX, 0 = NULL
    uint8_t pad0[6];
    uint16_t target; // THING_INDEX, 0 = NULL
    uint8_t pad1[6];
    int32_t home_x;
    int32_t home_y;
    int32_t home_z;
    uint32_t dist;
    int32_t radius;
    int32_t patrol;
    int32_t spotx;
    int32_t spotz;
    int32_t spotdx;
    int32_t spotdz;
    int32_t dx;
    int32_t dy;
    int32_t dz;
    int32_t victim; // already an EWAY person index
    int16_t rx;
    int16_t ry;
    int16_t rz;
    uint16_t counter;
    uint16_t rotors;
    uint16_t rotorspeed;
    uint16_t speed;
    uint8_t chopper_type;
    uint8_t rotorprim;
    uint8_t substate;
    uint8_t light;
    uint8_t since_takeoff;
    uint8_t padding;
    uint8_t pad2[4];
};
static_assert(sizeof(QS_Chopper_Disk) == sizeof(Chopper));
static_assert(sizeof(Chopper) == 96);

// The animals pool is always empty (no CLASS_ANIMAL Thing can exist); its one pointer is
// written as 0 and forced to NULL on load.
struct QS_Animal_Disk {
    uint64_t target;
    uint16_t counter;
    uint16_t dist;
    uint16_t starty;
    uint8_t animal_type;
    uint8_t substate;
    uint8_t map_x;
    uint8_t map_z;
    uint8_t dest_x;
    uint8_t dest_z;
    uint8_t home_x;
    uint8_t home_z;
    uint8_t extra;
    uint8_t padding;
};
static_assert(sizeof(QS_Animal_Disk) == sizeof(Animal));
static_assert(sizeof(Animal) == 24);

// MAP_Beacon: the seven fields before the 8-byte wall-clock stamp copied byte for
// byte, then the saturated age and explicit padding in place of the stamp.
constexpr uint32_t QS_BEACON_PREFIX_BYTES = 16;
struct QS_Beacon_Disk {
    uint8_t prefix[QS_BEACON_PREFIX_BYTES]; // used, counter, track_thing, index, pad, wx, wz
    uint32_t age_ms;
    uint32_t pad0;
};
static_assert(sizeof(QS_Beacon_Disk) == sizeof(MAP_Beacon));
static_assert(sizeof(MAP_Beacon) == 24);
static_assert(offsetof(MAP_Beacon, ticks) == QS_BEACON_PREFIX_BYTES);

struct QS_CPData_Disk {
    uint8_t version;
    uint8_t channelcount;
    uint8_t pad1;
    uint8_t pad2;
    uint32_t pad3;
    uint16_t channels; // index into PLAYCUTS_tracks, QS_NULL_INDEX16 = NULL
    uint8_t pad4[6];
};
static_assert(sizeof(QS_CPData_Disk) == sizeof(CPData));
static_assert(sizeof(CPData) == 16);

struct QS_CPChannel_Disk {
    uint8_t type;
    uint8_t flags;
    uint8_t pad1;
    uint8_t pad2;
    uint16_t index;
    uint16_t packetcount;
    uint16_t packets; // index into PLAYCUTS_packets, QS_NULL_INDEX16 = NULL
    uint8_t pad3[6];
};
static_assert(sizeof(QS_CPChannel_Disk) == sizeof(CPChannel));
static_assert(sizeof(CPChannel) == 16);

#pragma pack(pop)

// ---------------------------------------------------------------------------
// Element sizes. QS_ELEMENT_BYTES is the disk element size the writer emits and the
// reader demands; QS_LIVE_BYTES repeats the sizeof expressions of save_table[]
// (missions/memory_globals.cpp). They must agree for every pool but
// EWAY_mess, whose live element is a CBYTE* and whose disk element is a QS_MessRef.
// A wrong entry is a build error, not a runtime check.
// ---------------------------------------------------------------------------

constexpr uint32_t QS_ELEMENT_BYTES[QS_POOL_COUNT] = {
    8, // 1 Pap_Lo
    26, // 6 DFacet
    6, 34, 28, 16, 8, // 15-19 PrimPoint, PrimFace4, PrimFace3, PrimObject, PrimMultiObject
    8, 2, // 20-21 OB_Ob, OB_Mapwho
    4, 1, 2, 6, 32, 12, 1, // 22-28 QS_MessRef, CBYTE, UWORD, EWAY_Cond, EWAY_Way, EWAY_Edef, UBYTE
    232, 168, 24, 96, 104, 208, 8, 20, 28, 32, // 29-38 Vehicle .. Bat
    80, 80, 10, // 39-41 Thing, DrawTween, DrawMesh
    28, 6, 12, 24, 4, 2, // 42-47 BARREL_Sphere, Barrel, Plat, WMOVE_Face, MAV_Opt, UWORD
    104, 36, // 49-50 BALLOON_Balloon, Track
    12, // 55 NIGHT_Dlight
    14, // 60 TRIP_Wire
    2, // 62 UWORD
    24, // 64 MAP_Beacon
    16, 16, 24, 1, // 65-68 CPData, CPChannel, CPPacket, CBYTE
    2, // 71 DOOR_Door
};

constexpr uint32_t QS_LIVE_BYTES[QS_POOL_COUNT] = {
    sizeof(PAP_Lo),
    sizeof(struct DFacet),
    sizeof(struct PrimPoint), sizeof(struct PrimFace4), sizeof(struct PrimFace3), sizeof(struct PrimObject), sizeof(struct PrimMultiObject),
    sizeof(OB_Ob), sizeof(OB_Mapwho),
    sizeof(QS_MessRef), sizeof(CBYTE), sizeof(UWORD), sizeof(EWAY_Cond), sizeof(EWAY_Way), sizeof(EWAY_Edef), sizeof(UBYTE),
    sizeof(Vehicle), sizeof(Person), sizeof(Animal), sizeof(Chopper), sizeof(Pyro), sizeof(Player), sizeof(Projectile), sizeof(Special), sizeof(Switch), sizeof(Bat),
    sizeof(Thing), sizeof(DrawTween), sizeof(DrawMesh),
    sizeof(BARREL_Sphere), sizeof(Barrel), sizeof(Plat), sizeof(WMOVE_Face), sizeof(MAV_Opt), sizeof(UWORD),
    sizeof(BALLOON_Balloon), sizeof(Track),
    sizeof(NIGHT_Dlight),
    sizeof(TRIP_Wire),
    sizeof(UWORD),
    sizeof(MAP_Beacon),
    sizeof(CPData), sizeof(CPChannel), sizeof(CPPacket), sizeof(CBYTE),
    sizeof(DOOR_Door),
};

// Slot of EWAY_mess in QS_POOL_IDS (its disk element differs from the live one).
constexpr uint32_t QS_SLOT_EWAY_MESS = 9;
static_assert(QS_POOL_IDS[QS_SLOT_EWAY_MESS] == QS_POOL_EWAY_MESS);
static_assert(QS_ELEMENT_BYTES[QS_SLOT_EWAY_MESS] == sizeof(QS_MessRef));

constexpr bool qs_element_bytes_ok()
{
    for (uint32_t i = 0; i < QS_POOL_COUNT; ++i) {
        if (QS_ELEMENT_BYTES[i] != QS_LIVE_BYTES[i]) {
            return false;
        }
    }
    return true;
}
static_assert(qs_element_bytes_ok(), "a live pool type changed size: update QS_ELEMENT_BYTES and bump QS_FORMAT_VERSION");

constexpr bool qs_pool_ids_ascending()
{
    for (uint32_t i = 1; i < QS_POOL_COUNT; ++i) {
        if (QS_POOL_IDS[i] <= QS_POOL_IDS[i - 1]) {
            return false;
        }
    }
    return QS_POOL_IDS[QS_POOL_COUNT - 1] < QS_SAVE_TABLE_ENTRIES;
}
static_assert(qs_pool_ids_ascending());
static_assert(qs_pool_slot(SAVE_TABLE_PLATS) != QS_POOL_COUNT && qs_pool_slot(SAVE_TABLE_WMOVE) != QS_POOL_COUNT,
    "plat and wmove pools must be in QS_POOL_IDS");

// The pool element types written as raw images (pointer-free): their size
// is pinned above through QS_ELEMENT_BYTES; their alignment must not exceed 8 so a
// memcpy of one element is the whole story.
static_assert(alignof(PAP_Lo) <= 8 && alignof(struct DFacet) <= 8 && alignof(Person) <= 8);
static_assert(alignof(Special) <= 8 && alignof(Switch) <= 8 && alignof(Bat) <= 8 && alignof(Projectile) <= 8);
static_assert(alignof(DrawMesh) <= 8 && alignof(BARREL_Sphere) <= 8 && alignof(Barrel) <= 8 && alignof(Plat) <= 8);
static_assert(alignof(WMOVE_Face) <= 8 && alignof(MAV_Opt) <= 8 && alignof(BALLOON_Balloon) <= 8 && alignof(Track) <= 8);
static_assert(alignof(NIGHT_Dlight) <= 8 && alignof(TRIP_Wire) <= 8 && alignof(CPPacket) <= 8 && alignof(DOOR_Door) <= 8);
static_assert(alignof(EWAY_Way) <= 8 && alignof(EWAY_Cond) <= 8 && alignof(EWAY_Edef) <= 8 && alignof(OB_Ob) <= 8);
// Person carries one wall-clock stamp (MuzzleFlashUntilMs, visual) that the writer
// zeroes; pin its position so a layout change is noticed.
static_assert(offsetof(Person, MuzzleFlashUntilMs) + sizeof(uint64_t) <= sizeof(Person));

// ---------------------------------------------------------------------------
// Little-endian helpers for the framing.
// ---------------------------------------------------------------------------

inline void qs_put_u16(uint8_t* p, uint16_t v)
{
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
}

inline void qs_put_u32(uint8_t* p, uint32_t v)
{
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
    p[2] = uint8_t(v >> 16);
    p[3] = uint8_t(v >> 24);
}

inline void qs_put_u64(uint8_t* p, uint64_t v)
{
    qs_put_u32(p, uint32_t(v));
    qs_put_u32(p + 4, uint32_t(v >> 32));
}

inline uint16_t qs_get_u16(const uint8_t* p)
{
    return uint16_t(p[0] | (p[1] << 8));
}

inline uint32_t qs_get_u32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

inline uint64_t qs_get_u64(const uint8_t* p)
{
    return uint64_t(qs_get_u32(p)) | (uint64_t(qs_get_u32(p + 4)) << 32);
}

#endif // MISSIONS_QUICKSAVE_FORMAT_H
