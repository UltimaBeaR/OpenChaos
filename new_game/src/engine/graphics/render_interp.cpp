#include "engine/graphics/render_interp.h"
#include "debug_interpolation_config.h"

#include "things/core/thing.h"
#include "things/core/drawtype.h"
#include "engine/animation/anim_types.h" // GameKeyFrame layout, ANIM_FLAG_LAST_FRAME
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks (cross-anim blend timing)
#include "game/game_types.h" // THINGS, TO_THING
#include "game/game_globals.h" // g_physics_hz (adaptive blend duration)
#include "missions/eway_globals.h" // EWAY_cam_* — cutscene camera globals
#include "world_objects/dirt.h" // DIRT_dirt pool — leaves, brass, cans, etc.
#include "things/items/grenade_globals.h" // GrenadeArray, MAX_GRENADES
#include "engine/graphics/geometry/pose_composer.h" // Phase 2: per-bone world-pose capture
#include "engine/core/quaternion.h" // Phase 3 apply: BuildTween for per-bone slerp

#include <stdint.h> // uint64_t
#include <stdio.h>  // fprintf — used by RENDER_INTERP_LOG diagnostic gates
#include <stdlib.h> // abs — Manhattan distance for diagnostic delta detector

// Set to 1 to write slot-reuse / teleport-class events to stderr (which
// the Makefile redirects to stderr.log next to the exe). Default 0 —
// flip to 1 only when chasing a specific "object teleports across the map
// for one frame" bug, then revert. Output is post-mortem only (no
// on-screen overlay) so the noise stays out of normal play.
//
// Events logged:
//   FIRST_CAPTURE slot=N class=C dt=D pos=(X,Y,Z) t=T
//     First capture into a snapshot slot after reset/invalidate. Useful
//     to correlate with subsequent BIG_DELTA in the same slot.
//   CLASS_CHG slot=N old=C1 new=C2 dt=D t=T
//     Slot held one Class last tick, holds another now. Strongest signal
//     of slot reuse without free_thing — previous occupant's snapshot
//     data is almost certainly stale.
//   VEH_REBIND slot=N old=ptr new=ptr t=T
//     CLASS_VEHICLE slot's Genus.Vehicle pointer changed. VEH_alloc
//     returned a different Vehicle struct — new car in the same Thing
//     slot.
//   BIG_DELTA slot=N class=C dt=D from=(...) to=(...) |d|=N t=T
//     Manhattan distance between snap.prev and snap.curr WorldPos exceeds
//     RI_BIG_DELTA_THRESHOLD. Per-tick threshold deliberately set above
//     fast-vehicle motion at 5 Hz physics so legitimate movement rarely
//     trips. Diagnostic only — fixes go at the source code path that
//     causes the jump (mark_teleport at the WorldPos write), not as a
//     threshold gate here (false-negative risk when stale prev happens
//     to be near the new curr for dense spawn ensembles in cutscenes).
#define RENDER_INTERP_LOG 0

// Diagnostic logging for chasing cutscene-camera cut timing — emits a
// [render_interp:CUT] line on each physics tick where EWAY_cam_x/y/z/yaw
// jumps by more than a "smooth motion" threshold (i.e. scene cut, not
// EWAY between-waypoint sweep). Off by default; flip to 1 to debug.
#define RENDER_INTERP_DIAG_LOG 0

float g_render_alpha = 0.0f;
bool  g_render_interp_enabled = true;
uint32_t g_render_interp_frame_counter = 0;

namespace {

struct ThingSnap {
    GameCoord pos_prev, pos_curr;
    bool valid;

    // Anim-transition / mesh / anim id tracking — used by capture_pose to
    // invalidate the per-bone snapshot (g_pose_snaps[idx]) when the skeleton
    // structure changes. last_seen_class is for RENDER_INTERP_LOG slot-reuse
    // diagnostics.
    UBYTE last_mesh_id;
    SLONG last_anim;
    UBYTE last_seen_class;

    // Vehicle yaw/tilt/roll. CLASS_VEHICLE stores these in Genus.Vehicle->
    // Angle/Tilt/Roll (SLONG), separate from Draw.Tweened. Angle is normalised
    // to [0, 2047] each tick by the vehicle physics; Tilt/Roll typically stay
    // small around 0 but are not normalised. veh_velr_curr is the post-tick
    // VelR — used as a direction hint when the per-tick angular delta is
    // large enough that short-path lerp would otherwise rotate backwards
    // (issue: very fast spin > 180°/tick).
    //
    // veh_ptr_curr caches Genus.Vehicle at the time of capture. The render
    // frame compares it against the live p->Genus.Vehicle and only applies
    // when they match. This guards against cutscene scenarios where a slot's
    // vehicle pointer is rebound between capture and apply (new actor cued
    // in, slot reused) — applying lerp values from a stale capture against
    // an unrelated live Vehicle struct, or worse against an invalid pointer
    // that survived the rebind, would crash.
    bool  has_vehicle_angles;
    Vehicle* veh_ptr_curr;
    SLONG veh_angle_prev, veh_angle_curr;
    SLONG veh_tilt_prev,  veh_tilt_curr;
    SLONG veh_roll_prev,  veh_roll_curr;
    SLONG veh_velr_curr;

    // DT_MESH props (barrels, cones, traffic items): orientation lives in
    // Draw.Mesh->Angle/Tilt/Roll (UWORD 0..2047) instead of Genus.* like
    // vehicles. Vel hints are signed unwrapped per-tick deltas (in [-1024,
    // 1024]) — used by lerp_angle_2048_directed when |delta| > 1024 (very
    // fast spin, e.g. tumbling barrel post-impact) so short-path lerp
    // doesn't rotate backwards through the wrap point.
    //
    // mesh_ptr_curr cached at capture so dtor restores the same DrawMesh
    // even if Draw.Mesh got rebound between ctor/dtor (mirror of veh_ptr).
    bool      has_mesh_angles;
    DrawMesh* mesh_ptr_curr;
    UWORD     mesh_angle_prev, mesh_angle_curr;
    UWORD     mesh_tilt_prev,  mesh_tilt_curr;
    UWORD     mesh_roll_prev,  mesh_roll_curr;
    SLONG     mesh_angle_vel_hint, mesh_tilt_vel_hint, mesh_roll_vel_hint;

    // Frame-scope save: live Thing values overwritten by RenderInterpFrame
    // ctor and restored by dtor. applied=false means this entry was not
    // touched this frame and dtor must skip it.
    GameCoord saved_pos;
    SLONG saved_veh_angle, saved_veh_tilt, saved_veh_roll;
    UWORD saved_mesh_angle, saved_mesh_tilt, saved_mesh_roll;
    bool applied;
    bool veh_angles_applied;  // set when ctor wrote interpolated values into
                              // Genus.Vehicle->Angle/Tilt/Roll; tells dtor to
                              // restore them.
    bool mesh_angles_applied; // mirror for Draw.Mesh->Angle/Tilt/Roll.
};

// Indexed by (p_thing - &THINGS[0]). MAX_THINGS == 701.
ThingSnap g_thing_snaps[MAX_THINGS] = {};

// Camera snapshot. Splitscreen unused — single slot, keyed by FC_Cam*.
struct CamSnap {
    FC_Cam* cam;
    SLONG x_prev, x_curr;
    SLONG y_prev, y_curr;
    SLONG z_prev, z_curr;
    SLONG yaw_prev, yaw_curr;
    SLONG pitch_prev, pitch_curr;
    SLONG roll_prev, roll_curr;
    bool valid;
    bool skip_once;

    // Frame-scope save.
    SLONG saved_x, saved_y, saved_z;
    SLONG saved_yaw, saved_pitch, saved_roll;
    bool applied;
};

// EWAY (in-game cutscene) camera snapshot. The cutscene camera lives in
// separate globals (EWAY_cam_x/y/z/yaw/pitch/lens) rather than FC_cam, and
// EWAY_grab_camera copies them into fc->* inside the render path — after
// our FC_cam apply, defeating interpolation. We snapshot the EWAY globals
// in physics-tick and apply lerped values from the renderer immediately
// after EWAY_grab_camera writes its raw post-tick state.
//
// EWAY_grab_camera always sets *cam_roll = 1024 << 8 (a constant), so roll
// is not part of the snapshot; apply just writes that constant through.
struct EwayCamSnap {
    SLONG x_prev, x_curr;
    SLONG y_prev, y_curr;
    SLONG z_prev, z_curr;
    SLONG yaw_prev, yaw_curr;
    SLONG pitch_prev, pitch_curr;
    SLONG lens_prev, lens_curr;
    bool valid;
    // Set by the delta-based cut detector inside capture. When true, after
    // prev/curr have been updated to the post-cut EWAY values, prev is
    // forced to equal curr — so render frames between this and the next
    // tick lerp(curr, curr, alpha) = curr, locking the camera at the new
    // position with no "fly across the cut" interpolation.
    bool skip_once;
};

EwayCamSnap g_eway_cam_snap = {};

// EWAY-grab roll is hard-coded to 1024 << 8 in eway.cpp:4510.
// uc_orig: EWAY_grab_camera (fallen/Source/eway.cpp)
static constexpr SLONG EWAY_GRAB_ROLL = 1024 << 8;

// FC_MAX_CAMS == 2; only [0] is used in the shipped game, but supporting both
// is trivial.
CamSnap g_cam_snaps[FC_MAX_CAMS] = {};

// DIRT pool snapshot (leaves, brass, cans, blood, snow, etc — see DIRT_TYPE_*).
// DIRT_dirt is a 1024-slot pool of physics-driven debris updated once per
// physics tick by DIRT_process. Without interpolation each leaf judders at
// physics rate while the world around it lerps smoothly.
//
// Coordinates are SWORD (16-bit, scaled local around DIRT focus). Per-tick
// motion stays well inside SWORD range. Angles (yaw/pitch/roll) are SWORD on
// the standard 0..2047 Tween range.
//
// `last_type` is the type observed at the previous capture. Detects slot
// reuse: if a slot transitions UNUSED → in-use, or one type → another
// (different debris altogether), we treat it as a fresh first-capture so
// the next render frame doesn't lerp from the previous occupant's state.
struct DirtSnap {
    SWORD x_prev, x_curr;
    SWORD y_prev, y_curr;
    SWORD z_prev, z_curr;
    SWORD yaw_prev, yaw_curr;
    SWORD pitch_prev, pitch_curr;
    SWORD roll_prev, roll_curr;
    UBYTE last_type;
    bool  valid;

    // Frame-scope save.
    SWORD saved_x, saved_y, saved_z;
    SWORD saved_yaw, saved_pitch, saved_roll;
    bool  applied;
};

DirtSnap g_dirt_snaps[DIRT_MAX_DIRT] = {};

// Grenade pool snapshot (GrenadeArray[MAX_GRENADES]).
// Thrown grenades live in a separate pool outside the Thing system and are
// rendered via DrawGrenades() which reads x/y/z/yaw/pitch directly. Without
// interpolation they judder at physics rate like DIRT. Same pattern: capture
// post-tick state, substitute lerped values in frame-scope, restore on dtor.
//
// Coordinates are SLONG (unlike DIRT's SWORD — grenade coords are world-space
// in sub-pixel units without the DIRT local-focus rescaling). Angles are SWORD
// with no 0..2047 masking in physics (gp->yaw += gp->dyaw plain wrap);
// dpitch=50/tick means a grenade never traverses the full SWORD range during
// its brief flight, so lerp_i16 (linear) is correct — no short-path needed.
//
// Slot liveness: gp->owner != NULL means active, same role as DIRT_TYPE_UNUSED.
struct GrenadeSnap {
    SLONG x_prev, x_curr;
    SLONG y_prev, y_curr;
    SLONG z_prev, z_curr;
    SWORD yaw_prev,   yaw_curr;
    SWORD pitch_prev, pitch_curr;
    bool  valid;

    // Frame-scope save.
    SLONG saved_x, saved_y, saved_z;
    SWORD saved_yaw, saved_pitch;
    bool  applied;
};

GrenadeSnap g_grenade_snaps[MAX_GRENADES] = {};

// Per-bone snapshot (Phase 2 of world-pose-snapshot work). Stored format
// matches the architecture decision in
// new_game_devlog/fps_unlock/render_interpolation/world_pose_snapshot_plan.md:
//   bone[0] (PELVIS)  — VISIBLE pelvis in WORLD coords (= the smooth anchor
//                       confirmed by Phase 0 ladder/jump testing).
//   bones[1..14]      — local-to-parent (offset + rotation in parent's local
//                       frame). At apply time, hierarchy is rebuilt by
//                       chaining parent_world × local_to_parent → child_world.
//                       This preserves rigid kinematics across interpolation
//                       (parent's interpolated rotation propagates to children
//                       so joints stay connected even on fast rotational
//                       motion).
//
// Rotation stored as CMatrix33 (10-bit-per-component packed, same format
// as keyframes) — halves storage vs Matrix33 and matches CQuaternion::
// BuildTween's input format directly. Lossy (each element rounded to nearest
// multiple of 64) but anim keyframes already use this precision.
struct BoneSnap {
    SLONG x, y, z;       // bone[0]: WORLD pos; bones[1..14]: parent-local offset
    CMatrix33 cmat;      // bone[0]: WORLD rot;  bones[1..14]: parent-local rot
};

struct PoseSnap {
    BoneSnap bones_prev[POSE_MAX_BONES];
    BoneSnap bones_curr[POSE_MAX_BONES];
    UBYTE last_mesh_id;     // invalidate when this changes (different skeleton)
    UBYTE bone_count;       // number of valid bones (15 person; 1..20 flat)
    bool  flat_skeleton;    // true: bones[i] = world transform; false: bone[0]=world, [1..N]=parent-local
    bool  valid;            // false until first successful capture
};

// 24 bytes per bone × 15 bones × 2 (prev+curr) = 720 bytes per Thing slot.
// × MAX_THINGS = ~500 KB total. Memory budget per
// world_pose_snapshot_plan.md.
PoseSnap g_pose_snaps[MAX_THINGS] = {};

// Inverse of uncompress_matrix (hierarchy.cpp): pack a Matrix33 into a
// CMatrix33 via 10-bit-per-component packing. Each input element (scale
// 32768) is divided by 64 then clamped to signed 10-bit [-512, 511].
//
// ⚠️ Clamping is critical at the upper boundary: an unclamped 32768 → /64 = 512
// → packed as 0x200 → uncompresses (per hierarchy.cpp uncompress_matrix's
// arithmetic-shift sign extension) to -512 → ×64 = -32768. Sign-flipped
// identity = reflection — bones project on opposite side of pelvis. The
// identity case is hit any time mat_final has a unit diagonal (Darci standing
// still with no body angles). Clamp to 511 → uncompresses to 32704 (off by
// 0.2% from true 1.0 — imperceptible). Negative boundary is safe: -32768/64 =
// -512 fits exactly.
inline SLONG clamp_10bit_signed(SLONG v)
{
    if (v >  511) return  511;
    if (v < -512) return -512;
    return v;
}

inline void compress_matrix(const Matrix33* m, CMatrix33* cm)
{
    SLONG r0c0 = clamp_10bit_signed(m->M[0][0] >> 6);
    SLONG r0c1 = clamp_10bit_signed(m->M[0][1] >> 6);
    SLONG r0c2 = clamp_10bit_signed(m->M[0][2] >> 6);
    SLONG r1c0 = clamp_10bit_signed(m->M[1][0] >> 6);
    SLONG r1c1 = clamp_10bit_signed(m->M[1][1] >> 6);
    SLONG r1c2 = clamp_10bit_signed(m->M[1][2] >> 6);
    SLONG r2c0 = clamp_10bit_signed(m->M[2][0] >> 6);
    SLONG r2c1 = clamp_10bit_signed(m->M[2][1] >> 6);
    SLONG r2c2 = clamp_10bit_signed(m->M[2][2] >> 6);
    cm->M[0] = ((r0c0 & 0x3ff) << 20) | ((r0c1 & 0x3ff) << 10) | (r0c2 & 0x3ff);
    cm->M[1] = ((r1c0 & 0x3ff) << 20) | ((r1c1 & 0x3ff) << 10) | (r1c2 & 0x3ff);
    cm->M[2] = ((r2c0 & 0x3ff) << 20) | ((r2c1 & 0x3ff) << 10) | (r2c2 & 0x3ff);
}

inline SLONG lerp_i32(SLONG a, SLONG b, float t)
{
    return a + SLONG(float(b - a) * t);
}

// Linear lerp on SWORD-stored coords (DIRT pool x/y/z). Goes through SLONG
// for the math because SWORD diff can overflow on extremes (e.g. b=32000,
// a=-32000 → diff=64000 doesn't fit in SWORD).
inline SWORD lerp_i16(SWORD a, SWORD b, float t)
{
    SLONG ai = a;
    SLONG bi = b;
    return SWORD(ai + SLONG(float(bi - ai) * t));
}

// Short-path lerp on a circular 0..2047 range (Tween Angle/Tilt/Roll units).
// Inputs may be negative (e.g. -1 == 2047). Output always in [0, 2047].
inline SWORD lerp_angle_2048(SWORD a, SWORD b, float t)
{
    int ai = int(a) & 2047;
    int bi = int(b) & 2047;
    int diff = bi - ai;
    if (diff > 1024) diff -= 2048;
    else if (diff < -1024) diff += 2048;
    int result = ai + int(float(diff) * t);
    return SWORD(((result % 2048) + 2048) % 2048);
}

// Same circular 0..2047 lerp but accepting SLONG inputs. Vehicle Tilt/Roll
// are SLONG and not always normalised — mask to [0, 2047] before short-path.
inline SLONG lerp_angle_2048_slong(SLONG a, SLONG b, float t)
{
    int ai = ((int(a) % 2048) + 2048) % 2048;
    int bi = ((int(b) % 2048) + 2048) % 2048;
    int diff = bi - ai;
    if (diff > 1024) diff -= 2048;
    else if (diff < -1024) diff += 2048;
    int result = ai + int(float(diff) * t);
    return SLONG(((result % 2048) + 2048) % 2048);
}

// Direction-aware lerp on 0..2047 range. Used for Vehicle->Angle so that
// very fast spins (>180°/tick) don't render as backwards rotation.
//
// Logic:
//  - Raw diff = (b mod 2048) - (a mod 2048), range [-2047, +2047].
//  - When |diff| <= 1024: physics rotated by less than half a circle in
//    the direction matching `diff`. There is no ambiguity, hint is ignored,
//    and we lerp along `diff` as-is. Critical: a tiny backwards diff
//    (e.g. -1 from a collision micro-correction) must NOT be flipped to
//    +2047 just because vel_hint says "forward" — that would lerp the model
//    almost a full circle forward instead of -1 backward.
//  - When |diff| > 1024: short-path would pick the opposite-sign wrap (e.g.
//    diff=+1500 short-paths to -548, diff=-1500 short-paths to +548).
//    This is the ambiguous case. We prefer the side whose sign matches
//    `vel_hint`: positive vel_hint -> keep/produce positive diff
//    (long-path forward), negative -> keep/produce negative.
//  - vel_hint == 0 (not spinning) is treated as "no preference" -> standard
//    short-path. Reasonable because if vehicle isn't rotating and |diff|
//    is suddenly huge, both wraps are equally suspect; short-path
//    minimises visual artifact.
//
// In practice |diff| > 1024 means >180° per physics tick (>10 turns/sec at
// 20 Hz) — rare for normal driving (VelR caps near KERB_TURN scale ~10
// units/tick). The hint is mostly a safety net for impact-induced spin.
inline SLONG lerp_angle_2048_directed(SLONG a, SLONG b, float t, SLONG vel_hint)
{
    int ai = ((int(a) % 2048) + 2048) % 2048;
    int bi = ((int(b) % 2048) + 2048) % 2048;
    int diff = bi - ai;
    if (diff > 1024) {
        // Raw diff is positive-large. Short-path picks (diff - 2048) which
        // is small-negative. If vel_hint says forward, keep raw long-path;
        // otherwise use short-path.
        if (vel_hint <= 0) diff -= 2048;
    } else if (diff < -1024) {
        // Raw diff is negative-large. Short-path picks (diff + 2048).
        if (vel_hint >= 0) diff += 2048;
    }
    // |diff| <= 1024: lerp as-is regardless of vel_hint.
    int result = ai + int(float(diff) * t);
    return SLONG(((result % 2048) + 2048) % 2048);
}

// Short-path lerp on a circular 0..(2048*256) range (FC_Cam yaw/pitch/roll
// units — 1 angle unit = 256 sub-units). Output always in [0, 2048*256).
constexpr int CAM_ANGLE_FULL = 2048 * 256;
constexpr int CAM_ANGLE_HALF = CAM_ANGLE_FULL / 2;

inline SLONG lerp_angle_cam(SLONG a, SLONG b, float t)
{
    int ai = ((a % CAM_ANGLE_FULL) + CAM_ANGLE_FULL) % CAM_ANGLE_FULL;
    int bi = ((b % CAM_ANGLE_FULL) + CAM_ANGLE_FULL) % CAM_ANGLE_FULL;
    int diff = bi - ai;
    if (diff > CAM_ANGLE_HALF) diff -= CAM_ANGLE_FULL;
    else if (diff < -CAM_ANGLE_HALF) diff += CAM_ANGLE_FULL;
    int result = ai + int(float(diff) * t);
    return SLONG(((result % CAM_ANGLE_FULL) + CAM_ANGLE_FULL) % CAM_ANGLE_FULL);
}

inline UWORD thing_index(Thing* p_thing)
{
    // Pointer arithmetic against the global THINGS array. Returns 0..MAX_THINGS-1.
    return UWORD(p_thing - THINGS);
}

// Canonical-address sanity check. x86_64 user-mode pointers have bits 47-63
// all zero (the addressable virtual range is the low 47 bits). Garbage
// values that survive in snapshot fields (observed pattern in cutscenes:
// 0xff00007800000000) fail this check before we dereference and crash.
//
// This is a belt-and-suspenders guard. Snapshot pointer fields normally
// contain genuine GameKeyFrame*/Vehicle* allocated from static pools; if
// one is corrupted by an edge case (slot reuse without proper invalidation,
// DrawType transition leaving stale prev/curr) the canonicity check skips
// the dereference instead of segfaulting.
inline bool render_interp_addr_looks_valid(const void* p)
{
    uintptr_t u = reinterpret_cast<uintptr_t>(p);
    return u != 0 && (u >> 47) == 0;
}


// Tween-based DrawTypes: angles live in Draw.Tweened (SWORD, range 0..2047).
// ONLY include DrawTypes that actually allocate via alloc_draw_tween() and
// store a real DrawTween* in Draw.Tweened — otherwise we read DrawMesh
// (or uninitialised) memory through the union and get garbage in
// CurrentFrame/NextFrame snapshot fields, which later crashes the AnimTween
// apply path on a non-canonical pointer.
//
// Verified via grep on `p_thing->DrawType = DT_*` and the save/load
// convert_thing_to_pointer switch in [missions/memory.cpp]:
//   DT_ROT_MULTI  — person/cop/darci/roper, alloc_draw_tween, real Tweened
//   DT_ANIM_PRIM  — bat (and animal/bike via memory.cpp), real Tweened
//   DT_TWEEN      — generic, treated as Tweened in convert path
//   DT_CHOPPER    — chopper.cpp:85 sets p_thing->Draw.Mesh = dm (DrawMesh!)
//                   — memory.cpp converts as DT_MESH. NOT a tween. (Was
//                   incorrectly listed here; investigation root-caused
//                   the cutscene 0xff... crash on slot=143 chopper to this.)
//   DT_PYRO       — alloc_pyro never assigns Draw.X; pyro state lives in
//                   Genus.Pyro. Reading Draw.Tweened gives whatever the
//                   slot's previous occupant left there. NOT a tween.
//   DT_VEHICLE    — Genus.Vehicle->Angle/Tilt/Roll — handled separately
//                   via has_vehicle_angles below.
//   DT_MESH/DT_BIKE rotators — DrawMesh, not yet covered.
inline bool draw_type_uses_tween(UBYTE dt)
{
    switch (dt) {
        case DT_TWEEN:
        case DT_ROT_MULTI:
        case DT_ANIM_PRIM:
            return true;
        default:
            return false;
    }
}

CamSnap* cam_find_or_alloc(FC_Cam* fc)
{
    if (!fc) return nullptr;
    for (int i = 0; i < FC_MAX_CAMS; ++i) {
        if (g_cam_snaps[i].cam == fc) return &g_cam_snaps[i];
    }
    for (int i = 0; i < FC_MAX_CAMS; ++i) {
        if (g_cam_snaps[i].cam == nullptr) {
            g_cam_snaps[i] = {};
            g_cam_snaps[i].cam = fc;
            return &g_cam_snaps[i];
        }
    }
    return nullptr;
}

// Phase 2: capture full per-bone pose into g_pose_snaps[idx].
// Called once per physics tick from render_interp_capture for Tween-family
// person Things. Output format per BoneSnap comment above:
//   bones_curr[0]    = visible PELVIS world transform (pos + compressed rot).
//   bones_curr[1..N] = parent-local offset + rotation (derived from composer's
//                      body-local intermediates, independent of body angles).
//
// On first capture (or after invalidation) prev is seeded equal to curr so
// the first apply lerp returns curr exactly (no slide from stale state).
// On MeshID change the snapshot is invalidated — next capture re-seeds; the
// skeleton may have a different bone count or layout, so previous bone
// data is meaningless.
// Hierarchical capture for 15-bone person rig. bone[0] = visible PELVIS world
// transform; bones[1..14] = parent-local (derived from composer's body-local
// intermediates so R_body / WorldPos cancel out and the data lerps smoothly
// across body rotations while preserving rigid joint connections).
static bool capture_pose_hierarchical(PoseSnap& s, DrawTween* dt, Thing* p_thing,
                                      BoneSnap (&new_bones)[POSE_MAX_BONES])
{
    ComposedSkeletalPose pose;
    if (!compose_full_skeletal_pose(p_thing, &pose) || pose.bone_count != POSE_PERSON_BONE_COUNT) {
        return false;
    }

    new_bones[0].x = SLONG(pose.bones[0].pos_x);
    new_bones[0].y = SLONG(pose.bones[0].pos_y);
    new_bones[0].z = SLONG(pose.bones[0].pos_z);
    compress_matrix(&pose.bones[0].rot, &new_bones[0].cmat);

    for (int i = 1; i < POSE_PERSON_BONE_COUNT; i++) {
        int p = body_part_parent[i];

        // Transpose parent's body-local rotation in-place (= inverse for
        // orthogonal rotation matrix). Same swap pattern as
        // HIERARCHY_Get_Body_Part_Offset (hierarchy.cpp:58-60).
        Matrix33 parent_rot_inv = pose.bones[p].body_local_rot;
        SWAP(parent_rot_inv.M[0][1], parent_rot_inv.M[1][0]);
        SWAP(parent_rot_inv.M[0][2], parent_rot_inv.M[2][0]);
        SWAP(parent_rot_inv.M[1][2], parent_rot_inv.M[2][1]);

        Matrix31 diff;
        diff.M[0] = pose.bones[i].body_local_pos.M[0] - pose.bones[p].body_local_pos.M[0];
        diff.M[1] = pose.bones[i].body_local_pos.M[1] - pose.bones[p].body_local_pos.M[1];
        diff.M[2] = pose.bones[i].body_local_pos.M[2] - pose.bones[p].body_local_pos.M[2];
        Matrix31 local_pos;
        matrix_transformZMY(&local_pos, &parent_rot_inv, &diff);
        new_bones[i].x = local_pos.M[0];
        new_bones[i].y = local_pos.M[1];
        new_bones[i].z = local_pos.M[2];

        Matrix33 local_rot;
        matrix_mult33(&local_rot, &parent_rot_inv, &pose.bones[i].body_local_rot);
        compress_matrix(&local_rot, &new_bones[i].cmat);
    }
    return true;
}

// Flat capture for DT_ANIM_PRIM (bats / Bane / Balrog / Gargoyle) and DT_TWEEN
// with non-15 ElementCount. Each element stored as world transform directly
// — no parent walk, no parent-local derivation. Apply path lerps world pos /
// slerps world rot per element independently.
static bool capture_pose_flat(BoneSnap (&new_bones)[POSE_MAX_BONES], int* out_count, Thing* p_thing)
{
    ComposedFlatPose pose;
    if (!compose_flat_skeletal_pose(p_thing, &pose) || pose.bone_count <= 0) {
        return false;
    }

    for (int i = 0; i < pose.bone_count; i++) {
        new_bones[i].x = SLONG(pose.bones[i].pos_x);
        new_bones[i].y = SLONG(pose.bones[i].pos_y);
        new_bones[i].z = SLONG(pose.bones[i].pos_z);
        compress_matrix(&pose.bones[i].rot, &new_bones[i].cmat);
    }

    *out_count = pose.bone_count;
    return true;
}

void capture_pose(int idx, Thing* p_thing)
{
    PoseSnap& s = g_pose_snaps[idx];

    if (!draw_type_uses_tween(p_thing->DrawType)) {
        s.valid = false;
        return;
    }
    DrawTween* dt = p_thing->Draw.Tweened;
    if (!dt) { s.valid = false; return; }

    // Path selection: 15-bone person hierarchy vs flat skeleton.
    bool is_hierarchical = (p_thing->Class == CLASS_PERSON
                            && p_thing->DrawType == DT_ROT_MULTI
                            && dt->TheChunk
                            && dt->TheChunk->ElementCount == POSE_PERSON_BONE_COUNT);

    // Skeleton type or MeshID change → previous bone data is meaningless.
    if (s.valid && (s.last_mesh_id != dt->MeshID || s.flat_skeleton == is_hierarchical)) {
        s.valid = false;
    }

    BoneSnap new_bones[POSE_MAX_BONES];
    int new_count = 0;
    bool ok = false;
    if (is_hierarchical) {
        ok = capture_pose_hierarchical(s, dt, p_thing, new_bones);
        if (ok) new_count = POSE_PERSON_BONE_COUNT;
    } else {
        ok = capture_pose_flat(new_bones, &new_count, p_thing);
    }
    if (!ok) {
        s.valid = false;
        return;
    }

    if (!s.valid) {
        // First capture: prev = curr = new — apply lerps to identity.
        for (int i = 0; i < new_count; i++) {
            s.bones_prev[i] = new_bones[i];
            s.bones_curr[i] = new_bones[i];
        }
        s.valid = true;
    } else {
        // Standard window shift. bone_count is stable across captures (mesh-id
        // change above invalidates the slot if the skeleton size changed).
        for (int i = 0; i < new_count; i++) {
            s.bones_prev[i] = s.bones_curr[i];
            s.bones_curr[i] = new_bones[i];
        }
    }
    s.last_mesh_id = dt->MeshID;
    s.bone_count = (UBYTE)new_count;
    s.flat_skeleton = !is_hierarchical;
}

}  // namespace

void render_interp_reset(void)
{
    for (int i = 0; i < MAX_THINGS; ++i) g_thing_snaps[i] = {};
    for (int i = 0; i < FC_MAX_CAMS; ++i) g_cam_snaps[i] = {};
    for (int i = 0; i < DIRT_MAX_DIRT; ++i) g_dirt_snaps[i] = {};
    for (int i = 0; i < MAX_GRENADES; ++i) g_grenade_snaps[i] = {};
    for (int i = 0; i < MAX_THINGS; ++i) g_pose_snaps[i] = {};
    g_eway_cam_snap = {};
}

// Manhattan-distance threshold for "WorldPos jumped a lot in one tick".
// Diagnostic only — see RENDER_INTERP_LOG comment. World coords are sub-units
// of inches (256 sub-units/inch); 30000 sub-units ≈ 117 inches ≈ 3 m.
// Set just above fast-vehicle per-tick motion at 5 Hz physics
// (~28000 sub-units/tick at 100 km/h) so genuine motion rarely trips, but
// low enough to catch sub-room teleports like NPC spawn-and-fly in dense
// cutscene ensembles.
static constexpr SLONG RI_BIG_DELTA_THRESHOLD = 30000;

void render_interp_capture(Thing* p_thing)
{
    if (!p_thing) return;
    UWORD idx = thing_index(p_thing);
    if (idx >= MAX_THINGS) return;

    ThingSnap& s = g_thing_snaps[idx];
    DrawTween* dt = draw_type_uses_tween(p_thing->DrawType) ? p_thing->Draw.Tweened : nullptr;
    Vehicle* vp = (p_thing->Class == CLASS_VEHICLE) ? p_thing->Genus.Vehicle : nullptr;

#if RENDER_INTERP_LOG
    // Diagnostic events for chasing slot-reuse / teleport-source bugs.
    // No effect on interpolation — exists to identify the game-code paths
    // that produce mid-tick teleports so render_interp_mark_teleport can
    // be inserted at the source.
    if (s.valid) {
        if (s.last_seen_class != p_thing->Class) {
            fprintf(stderr,
                "[render_interp] CLASS_CHG slot=%u old=%u new=%u dt=%u t=%llu\n",
                idx, (unsigned)s.last_seen_class, (unsigned)p_thing->Class,
                (unsigned)p_thing->DrawType,
                (unsigned long long)sdl3_get_ticks());
        }
        if (vp && s.has_vehicle_angles && s.veh_ptr_curr && s.veh_ptr_curr != vp) {
            fprintf(stderr,
                "[render_interp] VEH_REBIND slot=%u old=%p new=%p t=%llu\n",
                idx, (void*)s.veh_ptr_curr, (void*)vp,
                (unsigned long long)sdl3_get_ticks());
        }
    }
#endif
    s.last_seen_class = p_thing->Class;

    // Track skeleton id for per-bone snapshot invalidation in capture_pose.
    if (dt) {
        s.last_mesh_id = dt->MeshID;
        s.last_anim = dt->CurrentAnim;
    } else {
        s.last_mesh_id = 0;
        s.last_anim = 0;
    }

    DrawMesh* mp = (p_thing->DrawType == DT_MESH) ? p_thing->Draw.Mesh : nullptr;

    if (!s.valid) {
#if RENDER_INTERP_LOG
        fprintf(stderr,
            "[render_interp] FIRST_CAPTURE slot=%u class=%d dt=%d pos=(%d,%d,%d) t=%llu\n",
            idx, p_thing->Class, p_thing->DrawType,
            p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z,
            (unsigned long long)sdl3_get_ticks());
#endif
        s.pos_curr = p_thing->WorldPos;
        s.pos_prev = s.pos_curr;
        if (vp) {
            s.veh_ptr_curr = vp;
            s.veh_angle_curr = vp->Angle;
            s.veh_tilt_curr  = vp->Tilt;
            s.veh_roll_curr  = vp->Roll;
            s.veh_velr_curr  = vp->VelR;
            s.veh_angle_prev = s.veh_angle_curr;
            s.veh_tilt_prev  = s.veh_tilt_curr;
            s.veh_roll_prev  = s.veh_roll_curr;
            s.has_vehicle_angles = true;
        } else {
            s.has_vehicle_angles = false;
            s.veh_ptr_curr = nullptr;
        }
        if (mp) {
            s.mesh_ptr_curr = mp;
            s.mesh_angle_curr = mp->Angle;
            s.mesh_tilt_curr  = mp->Tilt;
            s.mesh_roll_curr  = mp->Roll;
            s.mesh_angle_prev = s.mesh_angle_curr;
            s.mesh_tilt_prev  = s.mesh_tilt_curr;
            s.mesh_roll_prev  = s.mesh_roll_curr;
            s.mesh_angle_vel_hint = 0;
            s.mesh_tilt_vel_hint  = 0;
            s.mesh_roll_vel_hint  = 0;
            s.has_mesh_angles = true;
        } else {
            s.has_mesh_angles = false;
            s.mesh_ptr_curr = nullptr;
        }
        s.valid = true;
        capture_pose(idx, p_thing);
        return;
    }

    s.pos_prev = s.pos_curr;
    s.veh_angle_prev = s.veh_angle_curr;
    s.veh_tilt_prev  = s.veh_tilt_curr;
    s.veh_roll_prev  = s.veh_roll_curr;

    s.pos_curr = p_thing->WorldPos;

#if RENDER_INTERP_LOG
    // BIG_DELTA diagnostic. Manhattan distance, threshold above legitimate
    // per-tick motion at 5 Hz physics for fast vehicles.
    {
        SLONG dx = s.pos_curr.X - s.pos_prev.X;
        SLONG dy = s.pos_curr.Y - s.pos_prev.Y;
        SLONG dz = s.pos_curr.Z - s.pos_prev.Z;
        SLONG manh = abs(dx) + abs(dy) + abs(dz);
        if (manh > RI_BIG_DELTA_THRESHOLD) {
            fprintf(stderr,
                "[render_interp] BIG_DELTA slot=%u class=%d dt=%d "
                "from=(%d,%d,%d) to=(%d,%d,%d) |d|=%d t=%llu\n",
                idx, p_thing->Class, p_thing->DrawType,
                s.pos_prev.X, s.pos_prev.Y, s.pos_prev.Z,
                s.pos_curr.X, s.pos_curr.Y, s.pos_curr.Z,
                manh, (unsigned long long)sdl3_get_ticks());
        }
    }
#endif

    if (vp) {
        // Vehicle pointer changed (cutscene rebind, slot's vehicle pool
        // entry reallocated) — treat as a fresh capture so we don't lerp
        // across two unrelated Vehicle structs.
        bool ptr_changed = (s.veh_ptr_curr != vp);
        s.veh_ptr_curr = vp;
        s.veh_angle_curr = vp->Angle;
        s.veh_tilt_curr  = vp->Tilt;
        s.veh_roll_curr  = vp->Roll;
        s.veh_velr_curr  = vp->VelR;
        // First capture as a vehicle (slot was non-vehicle before, or this
        // is the first tick its vehicle pointer is non-null), or pointer
        // identity changed — collapse prev=curr so the next render frame
        // doesn't lerp from stale data.
        if (!s.has_vehicle_angles || ptr_changed) {
            s.veh_angle_prev = s.veh_angle_curr;
            s.veh_tilt_prev  = s.veh_tilt_curr;
            s.veh_roll_prev  = s.veh_roll_curr;
            s.has_vehicle_angles = true;
        }
    } else {
        // Class changed away from CLASS_VEHICLE (slot reused). Drop vehicle
        // angle interpolation; will re-establish on next CLASS_VEHICLE capture.
        s.has_vehicle_angles = false;
        s.veh_ptr_curr = nullptr;
    }

    if (mp) {
        // DT_MESH props (barrels, cones, …). Mirror of vehicle path: identity
        // check on the DrawMesh pointer, collapse prev=curr on first capture
        // or pointer change so we don't lerp across two unrelated meshes.
        const bool mp_changed = (s.mesh_ptr_curr != mp);
        s.mesh_ptr_curr = mp;
        s.mesh_angle_prev = s.mesh_angle_curr;
        s.mesh_tilt_prev  = s.mesh_tilt_curr;
        s.mesh_roll_prev  = s.mesh_roll_curr;
        s.mesh_angle_curr = mp->Angle;
        s.mesh_tilt_curr  = mp->Tilt;
        s.mesh_roll_curr  = mp->Roll;
        if (!s.has_mesh_angles || mp_changed) {
            s.mesh_angle_prev = s.mesh_angle_curr;
            s.mesh_tilt_prev  = s.mesh_tilt_curr;
            s.mesh_roll_prev  = s.mesh_roll_curr;
            s.mesh_angle_vel_hint = 0;
            s.mesh_tilt_vel_hint  = 0;
            s.mesh_roll_vel_hint  = 0;
            s.has_mesh_angles = true;
        } else {
            // Signed unwrapped per-tick delta in [-1024, 1024]. Used by
            // lerp_angle_2048_directed to disambiguate fast spin (|delta|
            // close to 1024) where short-path lerp would otherwise rotate
            // backwards through the wrap point.
            auto unwrap = [](SLONG diff) -> SLONG {
                if (diff >  1024) diff -= 2048;
                else if (diff < -1024) diff += 2048;
                return diff;
            };
            s.mesh_angle_vel_hint = unwrap(SLONG(s.mesh_angle_curr) - SLONG(s.mesh_angle_prev));
            s.mesh_tilt_vel_hint  = unwrap(SLONG(s.mesh_tilt_curr)  - SLONG(s.mesh_tilt_prev));
            s.mesh_roll_vel_hint  = unwrap(SLONG(s.mesh_roll_curr)  - SLONG(s.mesh_roll_prev));
        }
    } else {
        s.has_mesh_angles = false;
        s.mesh_ptr_curr = nullptr;
    }

    // World-space per-bone pose capture (writes g_pose_snaps[idx]).
    capture_pose(idx, p_thing);
}

void render_interp_invalidate(Thing* p_thing)
{
    if (!p_thing) return;
    UWORD idx = thing_index(p_thing);
    if (idx >= MAX_THINGS) return;
#if RENDER_INTERP_LOG
    if (g_thing_snaps[idx].valid) {
        fprintf(stderr,
            "[render_interp] FREE slot=%u class=%d was_pos=(%d,%d,%d)\n",
            idx, p_thing->Class,
            g_thing_snaps[idx].pos_curr.X,
            g_thing_snaps[idx].pos_curr.Y,
            g_thing_snaps[idx].pos_curr.Z);
    }
#endif
    g_thing_snaps[idx] = {};
    g_pose_snaps[idx] = {};
}

void render_interp_mark_teleport(Thing* p_thing)
{
    // Full snapshot reset. Previous implementation only set skip_once,
    // but skip_once is scoped to anim-only fields (AnimTween, FrameIndex,
    // CurrentFrame, NextFrame) — the same comment block in render_interp_capture
    // explicitly states pos/angles are NOT collapsed by it, because that
    // path is shared with anim-transition handling where pose changes but
    // WorldPos should keep lerping. Genuine teleports need full
    // pos/angle/vehicle/anim collapse, which is exactly what
    // render_interp_invalidate does (next capture goes through !valid
    // path and sets prev=curr=current).
    render_interp_invalidate(p_thing);
}

void render_interp_capture_camera(FC_Cam* fc)
{
    CamSnap* s = cam_find_or_alloc(fc);
    if (!s) return;
    if (!s->valid) {
        s->x_curr = fc->x;     s->x_prev = s->x_curr;
        s->y_curr = fc->y;     s->y_prev = s->y_curr;
        s->z_curr = fc->z;     s->z_prev = s->z_curr;
        s->yaw_curr = fc->yaw; s->yaw_prev = s->yaw_curr;
        s->pitch_curr = fc->pitch; s->pitch_prev = s->pitch_curr;
        s->roll_curr = fc->roll;   s->roll_prev = s->roll_curr;
        s->valid = true;
        s->skip_once = false;
        return;
    }

    s->x_prev = s->x_curr;
    s->y_prev = s->y_curr;
    s->z_prev = s->z_curr;
    s->yaw_prev = s->yaw_curr;
    s->pitch_prev = s->pitch_curr;
    s->roll_prev = s->roll_curr;

    s->x_curr = fc->x;
    s->y_curr = fc->y;
    s->z_curr = fc->z;
    s->yaw_curr = fc->yaw;
    s->pitch_curr = fc->pitch;
    s->roll_curr = fc->roll;

    if (s->skip_once) {
        s->x_prev = s->x_curr;
        s->y_prev = s->y_curr;
        s->z_prev = s->z_curr;
        s->yaw_prev = s->yaw_curr;
        s->pitch_prev = s->pitch_curr;
        s->roll_prev = s->roll_curr;
        s->skip_once = false;
    }
}

void render_interp_mark_camera_teleport(FC_Cam* fc)
{
    CamSnap* s = cam_find_or_alloc(fc);
    if (s) s->skip_once = true;
}

void render_interp_capture_eway_camera(void)
{
    // Cutscene not active — drop the snapshot so the next activation
    // starts fresh with prev=curr (no lerp from stale pre-cutscene state).
    if (!EWAY_cam_active) {
        g_eway_cam_snap.valid = false;
        return;
    }

    if (!g_eway_cam_snap.valid) {
        g_eway_cam_snap.x_curr = EWAY_cam_x;
        g_eway_cam_snap.y_curr = EWAY_cam_y;
        g_eway_cam_snap.z_curr = EWAY_cam_z;
        g_eway_cam_snap.yaw_curr = EWAY_cam_yaw;
        g_eway_cam_snap.pitch_curr = EWAY_cam_pitch;
        g_eway_cam_snap.lens_curr = EWAY_cam_lens;
        g_eway_cam_snap.x_prev = g_eway_cam_snap.x_curr;
        g_eway_cam_snap.y_prev = g_eway_cam_snap.y_curr;
        g_eway_cam_snap.z_prev = g_eway_cam_snap.z_curr;
        g_eway_cam_snap.yaw_prev = g_eway_cam_snap.yaw_curr;
        g_eway_cam_snap.pitch_prev = g_eway_cam_snap.pitch_curr;
        g_eway_cam_snap.lens_prev = g_eway_cam_snap.lens_curr;
        g_eway_cam_snap.valid = true;
        g_eway_cam_snap.skip_once = false;
        return;
    }

    g_eway_cam_snap.x_prev = g_eway_cam_snap.x_curr;
    g_eway_cam_snap.y_prev = g_eway_cam_snap.y_curr;
    g_eway_cam_snap.z_prev = g_eway_cam_snap.z_curr;
    g_eway_cam_snap.yaw_prev = g_eway_cam_snap.yaw_curr;
    g_eway_cam_snap.pitch_prev = g_eway_cam_snap.pitch_curr;
    g_eway_cam_snap.lens_prev = g_eway_cam_snap.lens_curr;

    g_eway_cam_snap.x_curr = EWAY_cam_x;
    g_eway_cam_snap.y_curr = EWAY_cam_y;
    g_eway_cam_snap.z_curr = EWAY_cam_z;
    g_eway_cam_snap.yaw_curr = EWAY_cam_yaw;
    g_eway_cam_snap.pitch_curr = EWAY_cam_pitch;
    g_eway_cam_snap.lens_curr = EWAY_cam_lens;

    // Delta-based cut detection. EWAY scripts have multiple cut paths
    // (conversation swap, scene change via EWAY_create_camera, target
    // change in EWAY_cam_converse) and not all of them set EWAY_cam_jumped.
    // Instead of trying to enumerate every cut signal in the script
    // pipeline, detect cuts by inspecting the per-tick position/yaw delta:
    // smooth EWAY motion (waypoint-to-waypoint via velocity, or midpoint
    // tracking of moving speakers) never exceeds a few world units per
    // tick, while a cut produces tens of thousands of sub-units. Anything
    // above threshold = cut, collapse prev=curr immediately to prevent
    // render from lerping across.
    {
        SLONG dx = g_eway_cam_snap.x_curr - g_eway_cam_snap.x_prev;
        SLONG dy = g_eway_cam_snap.y_curr - g_eway_cam_snap.y_prev;
        SLONG dz = g_eway_cam_snap.z_curr - g_eway_cam_snap.z_prev;
        SLONG dyaw = g_eway_cam_snap.yaw_curr - g_eway_cam_snap.yaw_prev;
        // Position threshold ~5m in world coords: smooth EWAY camera
        // velocity caps far below this per physics tick even at 5 Hz.
        const SLONG POS_CUT_THRESHOLD = 50000;
        // Yaw threshold ~30° on the 524288-unit camera angle range.
        const SLONG YAW_CUT_THRESHOLD = 43690;
        SLONG dyaw_abs = abs(dyaw);
        if (dyaw_abs > 524288 / 2) dyaw_abs = 524288 - dyaw_abs;
        bool is_cut = (abs(dx) > POS_CUT_THRESHOLD
                    || abs(dy) > POS_CUT_THRESHOLD
                    || abs(dz) > POS_CUT_THRESHOLD
                    || dyaw_abs > YAW_CUT_THRESHOLD);
        if (is_cut) {
            // prev/curr now both hold the post-cut EWAY values. Collapsing
            // them below locks the camera at the post-cut position so
            // render frames between this and the next tick lerp to curr
            // (no "fly across the cut").
            g_eway_cam_snap.skip_once = true;
#if RENDER_INTERP_DIAG_LOG
            extern SLONG EWAY_cam_jumped;
            fprintf(stderr,
                "[render_interp:CUT] auto-detect EWAY cut: dx=%d dy=%d dz=%d dyaw=%d "
                "EWAY_cam_jumped=%d "
                "prev_pos=(%d,%d,%d) curr_pos=(%d,%d,%d)\n",
                dx, dy, dz, dyaw,
                (int)EWAY_cam_jumped,
                g_eway_cam_snap.x_prev, g_eway_cam_snap.y_prev, g_eway_cam_snap.z_prev,
                g_eway_cam_snap.x_curr, g_eway_cam_snap.y_curr, g_eway_cam_snap.z_curr);
#endif
        }
    }

    if (g_eway_cam_snap.skip_once) {
        g_eway_cam_snap.x_prev = g_eway_cam_snap.x_curr;
        g_eway_cam_snap.y_prev = g_eway_cam_snap.y_curr;
        g_eway_cam_snap.z_prev = g_eway_cam_snap.z_curr;
        g_eway_cam_snap.yaw_prev = g_eway_cam_snap.yaw_curr;
        g_eway_cam_snap.pitch_prev = g_eway_cam_snap.pitch_curr;
        g_eway_cam_snap.lens_prev = g_eway_cam_snap.lens_curr;
        g_eway_cam_snap.skip_once = false;
    }
}

bool render_interp_apply_eway_camera(
    SLONG* cam_x, SLONG* cam_y, SLONG* cam_z,
    SLONG* cam_yaw, SLONG* cam_pitch, SLONG* cam_roll,
    SLONG* cam_lens)
{
    if (!g_render_interp_enabled || !g_eway_cam_snap.valid) return false;
    if constexpr (!ri_cfg::INTERP_EWAY_CAM) return false;
    const float alpha = g_render_alpha;
    if (cam_x)     *cam_x = lerp_i32(g_eway_cam_snap.x_prev, g_eway_cam_snap.x_curr, alpha);
    if (cam_y)     *cam_y = lerp_i32(g_eway_cam_snap.y_prev, g_eway_cam_snap.y_curr, alpha);
    if (cam_z)     *cam_z = lerp_i32(g_eway_cam_snap.z_prev, g_eway_cam_snap.z_curr, alpha);
    if (cam_yaw)   *cam_yaw   = lerp_angle_cam(g_eway_cam_snap.yaw_prev,   g_eway_cam_snap.yaw_curr,   alpha);
    if (cam_pitch) *cam_pitch = lerp_angle_cam(g_eway_cam_snap.pitch_prev, g_eway_cam_snap.pitch_curr, alpha);
    if (cam_roll)  *cam_roll  = EWAY_GRAB_ROLL; // EWAY_grab_camera writes a constant; preserve.
    if (cam_lens)  *cam_lens  = lerp_i32(g_eway_cam_snap.lens_prev, g_eway_cam_snap.lens_curr, alpha);
    return true;
}

void render_interp_mark_dirt_teleport(int idx)
{
    if (idx < 0 || idx >= DIRT_MAX_DIRT) return;
    // Wipe the snapshot. Next render_interp_capture_dirt sees !valid and
    // takes the first-capture path → prev=curr=current. No lerp through
    // the teleport.
    g_dirt_snaps[idx] = {};
}

void render_interp_capture_dirt(void)
{
    // One pass over the whole 1024-slot pool. Active slots shift
    // prev <- curr <- live; freed/never-allocated slots clear validity so
    // re-allocation goes through the !valid (first-capture) path and the
    // next render frame doesn't lerp from the previous occupant's pose.
    for (int i = 0; i < DIRT_MAX_DIRT; ++i) {
        const DIRT_Dirt& d = DIRT_dirt[i];
        DirtSnap& s = g_dirt_snaps[i];

        if (d.type == DIRT_TYPE_UNUSED) {
            s.valid = false;
            s.last_type = DIRT_TYPE_UNUSED;
            continue;
        }

        // Slot was either freed-and-re-allocated since last capture, or just
        // started its life. Either way: collapse prev=curr=live so render
        // doesn't slide across the type/identity transition.
        if (!s.valid || s.last_type != d.type) {
            s.x_curr = d.x;     s.x_prev = s.x_curr;
            s.y_curr = d.y;     s.y_prev = s.y_curr;
            s.z_curr = d.z;     s.z_prev = s.z_curr;
            s.yaw_curr = d.yaw; s.yaw_prev = s.yaw_curr;
            s.pitch_curr = d.pitch; s.pitch_prev = s.pitch_curr;
            s.roll_curr = d.roll;   s.roll_prev = s.roll_curr;
            s.last_type = d.type;
            s.valid = true;
            continue;
        }

        // Standard window-shift.
        s.x_prev = s.x_curr;     s.x_curr = d.x;
        s.y_prev = s.y_curr;     s.y_curr = d.y;
        s.z_prev = s.z_curr;     s.z_curr = d.z;
        s.yaw_prev = s.yaw_curr;     s.yaw_curr = d.yaw;
        s.pitch_prev = s.pitch_curr; s.pitch_curr = d.pitch;
        s.roll_prev = s.roll_curr;   s.roll_curr = d.roll;
    }
}

void render_interp_capture_grenades(void)
{
    for (int i = 0; i < MAX_GRENADES; ++i) {
        const Grenade& g = GrenadeArray[i];
        GrenadeSnap& s = g_grenade_snaps[i];

        if (!g.owner) {
            s.valid = false;
            continue;
        }

        if (!s.valid) {
            // First capture after slot became active — collapse prev=curr=live
            // so the first render frame doesn't lerp from stale/zero state.
            s.x_curr = g.x;     s.x_prev = s.x_curr;
            s.y_curr = g.y;     s.y_prev = s.y_curr;
            s.z_curr = g.z;     s.z_prev = s.z_curr;
            s.yaw_curr = g.yaw;     s.yaw_prev = s.yaw_curr;
            s.pitch_curr = g.pitch; s.pitch_prev = s.pitch_curr;
            s.valid = true;
            continue;
        }

        // Standard window-shift.
        s.x_prev = s.x_curr;     s.x_curr = g.x;
        s.y_prev = s.y_curr;     s.y_curr = g.y;
        s.z_prev = s.z_curr;     s.z_curr = g.z;
        s.yaw_prev = s.yaw_curr;     s.yaw_curr = g.yaw;
        s.pitch_prev = s.pitch_curr; s.pitch_curr = g.pitch;
    }
}

// Per-frame pose cache shared by all render-path consumers (figure.cpp body
// draw, FIGURE_draw_prim_tween_reflection, SMAP_add_tweened_points). Keyed
// by (Thing*, g_render_interp_frame_counter) so the same Thing drawn through
// multiple paths within one render frame walks the hierarchy only once.
namespace {
Thing* g_pose_cache_thing = nullptr;
uint32_t g_pose_cache_frame = 0xffffffffu;
bool g_pose_cache_valid = false;
BoneInterpTransform g_pose_cache[POSE_MAX_BONES];
}  // namespace

const BoneInterpTransform* render_interp_get_cached_pose(Thing* p_thing)
{
    if (!p_thing) return nullptr;
    if (g_pose_cache_thing == p_thing && g_pose_cache_frame == g_render_interp_frame_counter) {
        return g_pose_cache_valid ? g_pose_cache : nullptr;
    }
    g_pose_cache_thing = p_thing;
    g_pose_cache_frame = g_render_interp_frame_counter;
    g_pose_cache_valid = render_interp_compute_pose(p_thing, g_pose_cache);
    return g_pose_cache_valid ? g_pose_cache : nullptr;
}

bool render_interp_compute_pose(Thing* p_thing, BoneInterpTransform out[POSE_MAX_BONES])
{
    if (!p_thing || !out) return false;
    if (!g_render_interp_enabled) return false;
    if constexpr (!ri_cfg::INTERP_THING_WORLD_POSE) return false;

    UWORD idx = thing_index(p_thing);
    if (idx >= MAX_THINGS) return false;
    PoseSnap& s = g_pose_snaps[idx];
    if (!s.valid) return false;

    const float alpha = g_render_alpha;
    SLONG slerp_t = SLONG(alpha * 256.0f);
    if (slerp_t <   0) slerp_t =   0;
    if (slerp_t > 256) slerp_t = 256;

    if (s.flat_skeleton) {
        // Flat skeleton (DT_ANIM_PRIM bats / Bane / Balrog / Gargoyle, or
        // DT_TWEEN with non-15 ElementCount). Each bone stored as world
        // transform — lerp pos, slerp rot, no hierarchy chain.
        int n = s.bone_count;
        for (int i = 0; i < n; i++) {
            out[i].pos_x = float(lerp_i32(s.bones_prev[i].x, s.bones_curr[i].x, alpha));
            out[i].pos_y = float(lerp_i32(s.bones_prev[i].y, s.bones_curr[i].y, alpha));
            out[i].pos_z = float(lerp_i32(s.bones_prev[i].z, s.bones_curr[i].z, alpha));
            CQuaternion::BuildTween(&out[i].rot, &s.bones_prev[i].cmat, &s.bones_curr[i].cmat, slerp_t);
        }
        return true;
    }

    // Bone 0 (PELVIS): WORLD pos + WORLD rot directly from snapshot.
    out[0].pos_x = float(lerp_i32(s.bones_prev[0].x, s.bones_curr[0].x, alpha));
    out[0].pos_y = float(lerp_i32(s.bones_prev[0].y, s.bones_curr[0].y, alpha));
    out[0].pos_z = float(lerp_i32(s.bones_prev[0].z, s.bones_curr[0].z, alpha));
    CQuaternion::BuildTween(&out[0].rot, &s.bones_prev[0].cmat, &s.bones_curr[0].cmat, slerp_t);

    // Bones 1..14: lerp parent-local pos, slerp parent-local rot, then chain
    // through parent's interpolated world transform. Hierarchy walk in index
    // order is safe because body_part_parent[i] < i for all i (pre-order DFS).
    for (int i = 1; i < POSE_PERSON_BONE_COUNT; i++) {
        int p = body_part_parent[i];

        Matrix31 local_pos;
        local_pos.M[0] = lerp_i32(s.bones_prev[i].x, s.bones_curr[i].x, alpha);
        local_pos.M[1] = lerp_i32(s.bones_prev[i].y, s.bones_curr[i].y, alpha);
        local_pos.M[2] = lerp_i32(s.bones_prev[i].z, s.bones_curr[i].z, alpha);

        Matrix33 local_rot;
        CQuaternion::BuildTween(&local_rot, &s.bones_prev[i].cmat, &s.bones_curr[i].cmat, slerp_t);

        // world_pos[i] = world_pos[parent] + (world_rot[parent] × local_pos) / 256.
        // matrix_transformZMY does (mat × vec) / 32768. With world_rot at scale
        // 32768 and local_pos at body-frame ×256 scale, output is at ×256 scale.
        // Convert to inches for figure.cpp's float pos space by dividing by 256.
        Matrix31 rotated_local;
        matrix_transformZMY(&rotated_local, &out[p].rot, &local_pos);
        out[i].pos_x = out[p].pos_x + float(rotated_local.M[0]) / 256.0f;
        out[i].pos_y = out[p].pos_y + float(rotated_local.M[1]) / 256.0f;
        out[i].pos_z = out[p].pos_z + float(rotated_local.M[2]) / 256.0f;

        // world_rot[i] = world_rot[parent] × local_rot, both at scale 32768 →
        // matrix_mult33 outputs at scale 32768 (its >>15 normalises).
        matrix_mult33(&out[i].rot, &out[p].rot, &local_rot);
    }
    return true;
}

RenderInterpFrame::RenderInterpFrame()
{
    // Bump the frame counter unconditionally (even when the master toggle
    // is off) so consumer caches always invalidate each render frame.
    ++g_render_interp_frame_counter;

    // Master toggle — when off, no apply happens and the dtor has nothing
    // to restore (every entry stays applied=false from previous cleanup
    // or initial zero state).
    if (!g_render_interp_enabled) return;

    const float alpha = g_render_alpha;

    // Things: walk the snapshot array. Apply interpolated values into the
    // live Thing fields. Only entries with valid snapshots are touched.
    for (int i = 0; i < MAX_THINGS; ++i) {
        ThingSnap& s = g_thing_snaps[i];
        if (!s.valid) { s.applied = false; continue; }

        Thing* p = TO_THING(THING_INDEX(i));
        // Defensive: only apply if Thing slot is still in use. CLASS_NONE is
        // the unused-slot marker.
        if (p->Class == CLASS_NONE) { s.applied = false; continue; }

        s.saved_pos = p->WorldPos;
        if constexpr (ri_cfg::INTERP_THING_POS) {
            p->WorldPos.X = lerp_i32(s.pos_prev.X, s.pos_curr.X, alpha);
            p->WorldPos.Y = lerp_i32(s.pos_prev.Y, s.pos_curr.Y, alpha);
            p->WorldPos.Z = lerp_i32(s.pos_prev.Z, s.pos_curr.Z, alpha);
        }

        // Vehicle yaw/tilt/roll apply. Independent of the Tween branch above —
        // CLASS_VEHICLE's DrawType (DT_VEHICLE) is not in draw_type_uses_tween,
        // so vehicle angles live in Genus.Vehicle->Angle/Tilt/Roll instead of
        // Draw.Tweened.
        //
        // Pointer-identity check: only apply when the live Genus.Vehicle is
        // the same pointer we cached during capture. Cutscenes can rebind
        // a slot's vehicle (cue actor in / out, slot reuse) between physics
        // capture and the next render frame; in that case the captured
        // angles refer to the previous Vehicle struct and the live pointer
        // is either a different valid Vehicle or — observed in mission cut-
        // scene — an uninitialised pointer that survived a partial bind.
        // Comparing identity skips apply on mismatch (vehicle just renders
        // without interpolation that frame, no crash).
        if (s.has_vehicle_angles
            && p->Class == CLASS_VEHICLE
            && p->Genus.Vehicle
            && p->Genus.Vehicle == s.veh_ptr_curr) {
            Vehicle* v = p->Genus.Vehicle;
            s.saved_veh_angle = v->Angle;
            s.saved_veh_tilt  = v->Tilt;
            s.saved_veh_roll  = v->Roll;
            if constexpr (ri_cfg::INTERP_VEHICLE_ANGLE) {
                // Yaw uses VelR as direction hint so very fast spins (post-impact
                // cars, debris) don't reverse-rotate from short-path lerp.
                v->Angle = lerp_angle_2048_directed(s.veh_angle_prev, s.veh_angle_curr,
                                                    alpha, s.veh_velr_curr);
                // Tilt/Roll have no per-axis angular-velocity counterpart in
                // Vehicle struct; use plain short-path. Values are typically
                // small (vehicle leaning) so wraparound is irrelevant in practice.
                v->Tilt  = lerp_angle_2048_slong(s.veh_tilt_prev, s.veh_tilt_curr, alpha);
                v->Roll  = lerp_angle_2048_slong(s.veh_roll_prev, s.veh_roll_curr, alpha);
            }
            s.veh_angles_applied = true;
        } else {
            s.veh_angles_applied = false;
        }

        // DT_MESH props (barrels, cones). Identity-check Draw.Mesh same way
        // as Genus.Vehicle above. Each axis carries its own vel_hint computed
        // at capture (signed unwrapped delta) so fast tumbling doesn't
        // reverse-rotate through the wrap point.
        if (s.has_mesh_angles
            && p->DrawType == DT_MESH
            && p->Draw.Mesh
            && p->Draw.Mesh == s.mesh_ptr_curr) {
            DrawMesh* m = p->Draw.Mesh;
            s.saved_mesh_angle = m->Angle;
            s.saved_mesh_tilt  = m->Tilt;
            s.saved_mesh_roll  = m->Roll;
            if constexpr (ri_cfg::INTERP_MESH_ANGLE) {
                m->Angle = UWORD(lerp_angle_2048_directed(SLONG(s.mesh_angle_prev), SLONG(s.mesh_angle_curr),
                                                          alpha, s.mesh_angle_vel_hint));
                m->Tilt  = UWORD(lerp_angle_2048_directed(SLONG(s.mesh_tilt_prev),  SLONG(s.mesh_tilt_curr),
                                                          alpha, s.mesh_tilt_vel_hint));
                m->Roll  = UWORD(lerp_angle_2048_directed(SLONG(s.mesh_roll_prev),  SLONG(s.mesh_roll_curr),
                                                          alpha, s.mesh_roll_vel_hint));
            }
            s.mesh_angles_applied = true;
        } else {
            s.mesh_angles_applied = false;
        }

        s.applied = true;
    }

    // Camera(s).
    for (int i = 0; i < FC_MAX_CAMS; ++i) {
        CamSnap& s = g_cam_snaps[i];
        if (!s.valid || !s.cam) { s.applied = false; continue; }

        FC_Cam* fc = s.cam;
        s.saved_x = fc->x;
        s.saved_y = fc->y;
        s.saved_z = fc->z;
        s.saved_yaw = fc->yaw;
        s.saved_pitch = fc->pitch;
        s.saved_roll = fc->roll;

        if constexpr (ri_cfg::INTERP_FC_CAM) {
            fc->x     = lerp_i32(s.x_prev, s.x_curr, alpha);
            fc->y     = lerp_i32(s.y_prev, s.y_curr, alpha);
            fc->z     = lerp_i32(s.z_prev, s.z_curr, alpha);
            fc->yaw   = lerp_angle_cam(s.yaw_prev,   s.yaw_curr,   alpha);
            fc->pitch = lerp_angle_cam(s.pitch_prev, s.pitch_curr, alpha);
            fc->roll  = lerp_angle_cam(s.roll_prev,  s.roll_curr,  alpha);
        }

        s.applied = true;
    }

    // DIRT pool (leaves, brass, cans, blood, etc). 1024 slots, most usually
    // UNUSED — early skip on those keeps the loop cheap. valid=true active
    // slots get pos+angles substituted for the duration of the render frame.
    if constexpr (ri_cfg::INTERP_DIRT) {
        for (int i = 0; i < DIRT_MAX_DIRT; ++i) {
            DirtSnap& s = g_dirt_snaps[i];
            if (!s.valid) { s.applied = false; continue; }

            DIRT_Dirt& d = DIRT_dirt[i];
            // Defensive: if the slot got freed since capture (DIRT_process this
            // frame already ran, but we are between physics ticks here), skip.
            if (d.type == DIRT_TYPE_UNUSED) { s.applied = false; continue; }

            s.saved_x = d.x;
            s.saved_y = d.y;
            s.saved_z = d.z;
            s.saved_yaw = d.yaw;
            s.saved_pitch = d.pitch;
            s.saved_roll = d.roll;

            d.x = lerp_i16(s.x_prev, s.x_curr, alpha);
            d.y = lerp_i16(s.y_prev, s.y_curr, alpha);
            d.z = lerp_i16(s.z_prev, s.z_curr, alpha);
            d.yaw   = lerp_angle_2048(s.yaw_prev,   s.yaw_curr,   alpha);
            d.pitch = lerp_angle_2048(s.pitch_prev, s.pitch_curr, alpha);
            d.roll  = lerp_angle_2048(s.roll_prev,  s.roll_curr,  alpha);

            s.applied = true;
        }
    } else {
        // Mark all snaps as not-applied so dtor doesn't try to restore.
        for (int i = 0; i < DIRT_MAX_DIRT; ++i) g_dirt_snaps[i].applied = false;
    }

    // Grenade pool (thrown grenades, MAX_GRENADES=6 slots).
    if constexpr (ri_cfg::INTERP_GRENADE) {
        for (int i = 0; i < MAX_GRENADES; ++i) {
            GrenadeSnap& s = g_grenade_snaps[i];
            if (!s.valid) { s.applied = false; continue; }

            Grenade& g = GrenadeArray[i];
            if (!g.owner) { s.applied = false; continue; }

            s.saved_x = g.x;
            s.saved_y = g.y;
            s.saved_z = g.z;
            s.saved_yaw   = g.yaw;
            s.saved_pitch = g.pitch;

            g.x = lerp_i32(s.x_prev, s.x_curr, alpha);
            g.y = lerp_i32(s.y_prev, s.y_curr, alpha);
            g.z = lerp_i32(s.z_prev, s.z_curr, alpha);
            g.yaw   = lerp_i16(s.yaw_prev,   s.yaw_curr,   alpha);
            g.pitch = lerp_i16(s.pitch_prev, s.pitch_curr, alpha);

            s.applied = true;
        }
    } else {
        for (int i = 0; i < MAX_GRENADES; ++i) g_grenade_snaps[i].applied = false;
    }
}

RenderInterpFrame::~RenderInterpFrame()
{
    // Restore in reverse order (grenades, DIRT, camera, Things) just to mirror
    // ctor — ordering doesn't actually matter since the saves are independent.
    for (int i = 0; i < MAX_GRENADES; ++i) {
        GrenadeSnap& s = g_grenade_snaps[i];
        if (!s.applied) continue;
        Grenade& g = GrenadeArray[i];
        g.x = s.saved_x;
        g.y = s.saved_y;
        g.z = s.saved_z;
        g.yaw   = s.saved_yaw;
        g.pitch = s.saved_pitch;
        s.applied = false;
    }

    for (int i = 0; i < DIRT_MAX_DIRT; ++i) {
        DirtSnap& s = g_dirt_snaps[i];
        if (!s.applied) continue;
        DIRT_Dirt& d = DIRT_dirt[i];
        d.x = s.saved_x;
        d.y = s.saved_y;
        d.z = s.saved_z;
        d.yaw = s.saved_yaw;
        d.pitch = s.saved_pitch;
        d.roll = s.saved_roll;
        s.applied = false;
    }

    for (int i = 0; i < FC_MAX_CAMS; ++i) {
        CamSnap& s = g_cam_snaps[i];
        if (!s.applied) continue;
        FC_Cam* fc = s.cam;
        fc->x = s.saved_x;
        fc->y = s.saved_y;
        fc->z = s.saved_z;
        fc->yaw = s.saved_yaw;
        fc->pitch = s.saved_pitch;
        fc->roll = s.saved_roll;
        s.applied = false;
    }

    for (int i = 0; i < MAX_THINGS; ++i) {
        ThingSnap& s = g_thing_snaps[i];
        if (!s.applied) continue;
        Thing* p = TO_THING(THING_INDEX(i));
        p->WorldPos = s.saved_pos;
        // Restore through the same pointer ctor wrote to (cached at capture
        // time and identity-checked in ctor). Using p->Genus.Vehicle here
        // would risk restoring into a different Vehicle if the pointer was
        // somehow rebound after ctor — instead we touch exactly the struct
        // we modified.
        if (s.veh_angles_applied && s.veh_ptr_curr) {
            Vehicle* v = s.veh_ptr_curr;
            v->Angle = s.saved_veh_angle;
            v->Tilt  = s.saved_veh_tilt;
            v->Roll  = s.saved_veh_roll;
            s.veh_angles_applied = false;
        }
        if (s.mesh_angles_applied && s.mesh_ptr_curr) {
            DrawMesh* m = s.mesh_ptr_curr;
            m->Angle = s.saved_mesh_angle;
            m->Tilt  = s.saved_mesh_tilt;
            m->Roll  = s.saved_mesh_roll;
            s.mesh_angles_applied = false;
        }
        s.applied = false;
    }
}
