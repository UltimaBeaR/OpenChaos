#include "camera/vis_cam.h"
#include "ai/mav.h"                   // MAV_inside, MAVHEIGHT
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "engine/core/macros.h"       // QDIST3, WITHIN
#include "engine/core/math.h"         // SIN / COS
#include "map/map.h"                  // MAP_WIDTH, MAP_HEIGHT
#include <cstdint>
#include <cstdlib> // abs

// Camera-position smoothing — explicit three-mode state machine.
//
//   OFF: nothing — vc passes through as fc identity copy. No lerp, no
//        offset, no state. This is the dominant case in normal play.
//   COLLISION: a wall is currently shifting vc toward focus. Per-tick lerp
//        prev_vc → computed_vc with constant alpha smooths the jitter of
//        min_d_hit as the player slides along the wall. Entry from OFF is
//        a hard snap (no lerp on the entry tick) so the wall visibly stops
//        the camera.
//   EXIT: wall just cleared. We keep the SAME lerp mechanism but with
//        target = fc (identity, since vc_process_one didn't shift vc) and
//        the "keep" weight shrinking linearly from its COLLISION value
//        toward 0 over VC_EXIT_TICKS ticks. As keep → 0 the lerp degenerates
//        into vc = fc, fading our smoothing out continuously. Using the same
//        lerp formula as COLLISION (only changing target and weight) keeps
//        the camera's velocity continuous across the COL→EXIT boundary —
//        a static offset-taper here would create a velocity discontinuity
//        visible as a 1–2-frame jerk at the moment of separation.
enum VC_SmoothMode {
    VC_SM_OFF,
    VC_SM_COLLISION,
    VC_SM_EXIT,
};

struct VC_Smooth {
    VC_SmoothMode mode;
    // Previous tick's final vc, blended into current tick by both
    // COLLISION and EXIT modes.
    SLONG prev_x, prev_y, prev_z;
    // EXIT countdown. Drops from VC_EXIT_TICKS to 0; reaching 0 returns to
    // OFF. Scales the "keep" weight of the lerp linearly so smoothing
    // strength tapers from full to none over the fade.
    SLONG exit_ticks_left;
};

static VC_Smooth s_smooth[FC_MAX_CAMS] = {};

// Length of the EXIT fade in physics ticks. Physics runs at
// UC_PHYSICS_DESIGN_HZ = 20 Hz by default, so 10 ≈ 0.5 s — short enough
// to feel responsive, long enough that the lerp visibly tapers.
static const SLONG VC_EXIT_TICKS = 10;

// Cutscene gates — these globals live in other subsystems and we only need
// to read them. Local extern declarations avoid pulling EWAY/PLAYCUTS
// headers into the camera layer for two flags.
extern SLONG EWAY_cam_active;
extern BOOL PLAYCUTS_playing;

VC_State VC_state[FC_MAX_CAMS] = {};

// Focus point passed into the processing layer. World-space, shifted units
// (matching FC_cam fields and WorldPos). Source of the value is decided in
// VC_process; the processing function below treats it as opaque input. If
// we later swap the source for a better point, only the assignment in
// VC_process changes — processing stays put.
struct VC_FocusPoint {
    SLONG x;
    SLONG y;
    SLONG z;
};

// Camera treated as a sphere of this half-extent in world units when sampling
// for wall collisions. Half a metre — large enough that the four "side"
// samples actually catch walls grazing the side of the frustum, small enough
// to avoid pulling the camera in for walls that wouldn't be visible.
// uc_orig: Darci is ~0x10000 tall (~1.7m), so half a metre ≈ 0x4B43; rounded
// to 0x4000 here as a starting tuning point.
static const SLONG VC_CAMERA_RADIUS = 0x4000;

// How far back from a hit point the camera should keep itself, measured
// from focus toward the hit. Half a metre — keeps a visible breathing
// margin so decorative bumps on the wall geometry don't punch into the
// near edge of the frame.
static const SLONG VC_WALL_OFFSET = 0x5000;

// Extra padding subtracted on top of VC_WALL_OFFSET to account for the
// near-clip plane: the renderer's frustum extends slightly past the camera
// position toward focus (POLY_ZCLIP_PLANE), so a wall that is exactly at
// the offset distance can still clip through the near plane and show
// back-faces. Small padding gives a numerical safety margin without
// noticeably changing the camera distance.
static const SLONG VC_NEAR_CLIP_PAD = 0x800;

// Sample count per ray. 32 is what the previous baseline (013b1e82) used and
// what the original 8-step push covered with much wider steps; 32 gives ~1
// MAV cell of resolution along a typical 3–4m third-person distance.
static const SLONG VC_RAY_SAMPLES = 32;

// Lerp weight applied to the freshly computed camera position when both the
// previous and current physics tick were in collision. Lower = smoother
// (and laggier). Applied per-tick: vc = lerp(prev_vc, computed_vc, num/den).
// Only kicks in for sustained collision — entry into and exit from collision
// stay snap-responsive on purpose.
static const SLONG VC_SMOOTH_ALPHA_NUM = 6;
static const SLONG VC_SMOOTH_ALPHA_DEN = 10;

// MAVHEIGHT delta above the focus cell needed to count a sample as "wall".
// Without this, MAV_inside returns true on every step along flat ground —
// MAV_inside is "below the MAVHEIGHT surface", and on a flat outdoor stretch
// the surface is everywhere at ground level, so every below-surface point
// looks like an obstacle. A real wall/building/curb raises MAVHEIGHT enough
// in its own cell to clear this threshold. Units are raw MAVHEIGHT (1 unit
// ≈ 0.42 m).
//
// Threshold = 1 catches decorative outdoor platforms / large steps
// (≈ 1 m tall → MAVHEIGHT ≈ 2-3 units) in addition to full-height buildings
// (MAVHEIGHT = 127). Tighter than this risks false positives on uneven
// ground; looser misses the platforms.
static const SLONG VC_OBSTACLE_MAVH_THRESHOLD = 1;

// Ray-start Y offset above focus — lifts the first sample just out of
// focus's own body so the very first step doesn't read its own cell.
// Same value the baseline used.
static const SLONG VC_RAY_START_Y_OFFSET = 0x80;

// Vertical lift applied to the focus point on top of fc->lookabove
// (Darci ≈ 0x10000 ≈ 1.7 m, so 0x2000 ≈ 0.21 m).
//
// Why: when Darci is hanging on a ledge (pre-pull-up) and the player rotates
// the camera around to her face, an unlifted focus sits at her head height,
// which on these poses is at or below the ledge's top edge. Rays from focus
// to the camera then intersect the ledge corner, the wall-collision pulls
// the camera onto focus, and we end up framed into her body. Lifting focus
// just above the ledge surface keeps the rays clear of the corner. Value
// is the smallest lift that empirically removes the artefact — bigger
// would also work but starts visibly shifting the gameplay framing.
static const SLONG VC_FOCUS_LIFT = 0x2000;

// Sample-based ray from focus toward (tx,ty,tz). Walks VC_RAY_SAMPLES points
// along the segment, asking MAV_inside at each. A sample counts as a wall
// hit only if its cell's MAVHEIGHT is meaningfully above focus's own cell
// (see VC_OBSTACLE_MAVH_THRESHOLD). On hit, *d_hit is the distance from
// focus to the hit sample. Returns true if no hit was found.
//
// Why this and not there_is_a_los: outdoor city buildings in Urban Chaos
// are encoded as raised cells in the MAV/PAP height grid, not as mesh
// DFacets. The DFacet-based LOS functions return CLEAR through such walls,
// while MAV_inside + MAVHEIGHT does see them correctly.
//
// Coordinate convention: focus and target are in shifted world units
// (the same as FC_cam fields). MAV_inside expects its args already shifted
// right by 8 (i.e. world/256) — we apply that at the call site. MAVHEIGHT
// is a 2D array indexed by PAP-HI cell, which is world >> 16.
static bool vc_probe_mav(const VC_FocusPoint& focus,
    SLONG tx, SLONG ty, SLONG tz,
    SLONG focus_mavh, SLONG* d_hit_out)
{
    // Extend the ray past the target by (VC_WALL_OFFSET + VC_NEAR_CLIP_PAD)
    // along focus→target. Without this extension, the first time a wall
    // enters contact with the camera (d_hit ≈ d_now) the response immediately
    // pulls the camera in by VC_WALL_OFFSET — a visible jump on the first
    // contact frame. Extending the ray lets us detect the wall earlier (when
    // it's still WALL_OFFSET past the camera), so d_hit ≈ d_now + OFFSET and
    // d_new ≈ d_now: contact starts smooth. As the wall keeps approaching,
    // d_new shrinks continuously.
    SLONG cdx = tx - focus.x;
    SLONG cdy = ty - focus.y;
    SLONG cdz = tz - focus.z;
    SLONG corner_len = QDIST3(std::abs(cdx), std::abs(cdy), std::abs(cdz));
    SLONG extended_tx = tx;
    SLONG extended_ty = ty;
    SLONG extended_tz = tz;
    SLONG full_extension = VC_WALL_OFFSET + VC_NEAR_CLIP_PAD;
    if (corner_len > 0) {
        extended_tx = tx + (SLONG)((std::int64_t)cdx * full_extension / corner_len);
        extended_ty = ty + (SLONG)((std::int64_t)cdy * full_extension / corner_len);
        extended_tz = tz + (SLONG)((std::int64_t)cdz * full_extension / corner_len);
    }

    SLONG sx = focus.x;
    SLONG sy = focus.y + VC_RAY_START_Y_OFFSET;
    SLONG sz = focus.z;
    SLONG step_x = (extended_tx - sx) / VC_RAY_SAMPLES;
    SLONG step_y = (extended_ty - sy) / VC_RAY_SAMPLES;
    SLONG step_z = (extended_tz - sz) / VC_RAY_SAMPLES;

    for (SLONG step = 1; step <= VC_RAY_SAMPLES; step++) {
        sx += step_x;
        sy += step_y;
        sz += step_z;

        if (!MAV_inside(sx >> 8, sy >> 8, sz >> 8)) continue;

        SLONG cell_x = sx >> 16;
        SLONG cell_z = sz >> 16;
        if (!WITHIN(cell_x, 0, MAP_WIDTH - 1) || !WITHIN(cell_z, 0, MAP_HEIGHT - 1)) continue;

        SLONG sample_mavh = MAVHEIGHT(cell_x, cell_z);
        if (sample_mavh <= focus_mavh + VC_OBSTACLE_MAVH_THRESHOLD) continue;

        // Hit. Distance is the fraction of the full focus→extended-target
        // span we traversed, mapped back to QDIST3 of that extended vector
        // (so the hit-distance metric matches the per-sample step length).
        SLONG full_len = QDIST3(std::abs(extended_tx - focus.x),
            std::abs(extended_ty - focus.y),
            std::abs(extended_tz - focus.z));
        *d_hit_out = (SLONG)((std::int64_t)full_len * step / VC_RAY_SAMPLES);
        return false;
    }
    return true;
}

// Processing layer for one camera. Reads the identity-copy `vc` plus the
// focus point and mutates `vc.*` to push the visible camera in toward the
// focus when geometry would otherwise punch into the frame. Returns true
// if `vc` was actually shifted (i.e. a wall was detected and acted upon).
// Smoothing is applied later, in VC_process — this function only computes
// the raw target position.
//
// Sampling model: camera treated as a sphere of radius VC_CAMERA_RADIUS.
// Five rays are sampled from focus — one to the centre, four to side offsets
// (±right, ±up) — and the minimum hit distance among the failing rays is
// used to pull the camera in along the focus→camera axis. This catches
// walls that graze the side of the frustum even when the central line of
// sight is clear, which a single-ray test misses.
//
// The right vector is built from yaw only (roll is always 0 in gameplay
// and pitch is intentionally ignored — at gameplay-typical pitches the
// horizontal-right approximation is accurate enough, and ignoring pitch
// keeps the side rays in a stable horizontal band rather than rolling
// across the frame).
static bool vc_process_one(VC_State& vc, const VC_FocusPoint& focus)
{
    // PSX angle index 0..2047. vc.yaw is in shifted units (PSX angle × 256).
    SLONG yaw_psx = (vc.yaw >> 8) & 2047;

    // SIN/COS return 16.16 fixed-point (65536 = 1.0). Camera-position vector
    // from focus uses (SIN(yaw), COS(yaw)) — see FC_force_camera_behind in
    // fc.cpp, behind_x = focus_x + SIN(focus_yaw) * dist. So camera→focus
    // forward is -(SIN, 0, COS), and right = forward × world_up gives
    // (COS, 0, -SIN).
    SLONG rx = (COS(yaw_psx) * VC_CAMERA_RADIUS) >> 16;
    SLONG rz = -(SIN(yaw_psx) * VC_CAMERA_RADIUS) >> 16;
    SLONG ry = VC_CAMERA_RADIUS; // world up; Y grows upward in this engine

    // Focus cell's MAVHEIGHT — the baseline that ray samples are compared
    // against. Without this filter, MAV_inside fires on every sample along
    // flat ground (see VC_OBSTACLE_MAVH_THRESHOLD comment).
    SLONG focus_cell_x = focus.x >> 16;
    SLONG focus_cell_z = focus.z >> 16;
    SLONG focus_mavh = 0;
    if (WITHIN(focus_cell_x, 0, MAP_WIDTH - 1) && WITHIN(focus_cell_z, 0, MAP_HEIGHT - 1)) {
        focus_mavh = MAVHEIGHT(focus_cell_x, focus_cell_z);
    }

    SLONG min_d_hit = 0;
    SLONG fail_count = 0;
    SLONG d_hit;

#define VC_PROBE(SX, SY, SZ)                                                   \
    do {                                                                       \
        if (!vc_probe_mav(focus, (SX), (SY), (SZ), focus_mavh, &d_hit)) {      \
            if (fail_count == 0 || d_hit < min_d_hit) min_d_hit = d_hit;       \
            fail_count++;                                                      \
        }                                                                      \
    } while (0)

    VC_PROBE(vc.x, vc.y, vc.z);            // centre
    VC_PROBE(vc.x + rx, vc.y, vc.z + rz);  // right
    VC_PROBE(vc.x - rx, vc.y, vc.z - rz);  // left
    VC_PROBE(vc.x, vc.y + ry, vc.z);       // up
    VC_PROBE(vc.x, vc.y - ry, vc.z);       // down

#undef VC_PROBE

    if (fail_count == 0) return false;

    SLONG dx = vc.x - focus.x;
    SLONG dy = vc.y - focus.y;
    SLONG dz = vc.z - focus.z;
    SLONG d_now = QDIST3(std::abs(dx), std::abs(dy), std::abs(dz));
    if (d_now <= 0) return false; // degenerate — camera coincident with focus

    SLONG d_new = min_d_hit - VC_WALL_OFFSET;

    // Guard against moving the camera OUT (away from focus). vc_probe_mav
    // extends rays past the corner by (WALL_OFFSET + NEAR_PAD) for early
    // detection, which means d_hit can register past d_now — in that case
    // d_new would land at or above d_now, and writing it back would push
    // the camera further from focus. Skip in that range. The boundary
    // (d_new == d_now) is where the wall first enters its NEAR_PAD margin;
    // a fraction past that, d_new < d_now and we start pulling the camera
    // in smoothly.
    if (d_new >= d_now) return false;

    if (d_new <= 0) {
        // Hit closer than the wall-offset clearance — slide the camera all
        // the way onto focus. Overshooting focus (camera ends up on the far
        // side) is acceptable by design when Darci is flush against a wall.
        vc.x = focus.x;
        vc.y = focus.y;
        vc.z = focus.z;
    } else {
        // Scale (vc - focus) by d_new/d_now. 64-bit intermediate to avoid
        // overflow — dx and d_new can each reach hundreds of thousands in
        // shifted units, and their product can exceed 2^31.
        vc.x = focus.x + (SLONG)((std::int64_t)dx * d_new / d_now);
        vc.y = focus.y + (SLONG)((std::int64_t)dy * d_new / d_now);
        vc.z = focus.z + (SLONG)((std::int64_t)dz * d_new / d_now);
    }
    return true;
}

void VC_process(void)
{
    for (SLONG cam = 0; cam < FC_MAX_CAMS; cam++) {
        FC_Cam* fc = &FC_cam[cam];
        VC_State& vc = VC_state[cam];

        // Identity copy of FC_process output. Processing layer mutates vc.*
        // below.
        vc.x = fc->x;
        vc.y = fc->y;
        vc.z = fc->z;
        vc.yaw = fc->yaw;
        vc.pitch = fc->pitch;
        vc.roll = fc->roll;
        vc.lens = fc->lens;

        VC_Smooth& s = s_smooth[cam];

        // Cutscenes own their framing through EWAY/PLAYCUTS — leave vc as
        // identity copy and force OFF, so that on return to gameplay the
        // first tick is treated as a fresh start.
        if (EWAY_cam_active || PLAYCUTS_playing || fc->focus == NULL) {
            s.mode = VC_SM_OFF;
            continue;
        }

        // Focus point passed into processing.
        VC_FocusPoint focus = {
            fc->focus_x,
            fc->focus_y + fc->lookabove + VC_FOCUS_LIFT,
            fc->focus_z,
        };

        bool in_collision = vc_process_one(vc, focus);

        if (in_collision) {
            // COLLISION mode. First tick of contact is a snap (no lerp);
            // sustained ticks blend prev → freshly computed vc.
            if (s.mode == VC_SM_COLLISION) {
                SLONG keep = VC_SMOOTH_ALPHA_DEN - VC_SMOOTH_ALPHA_NUM;
                vc.x = (SLONG)(((std::int64_t)s.prev_x * keep
                        + (std::int64_t)vc.x * VC_SMOOTH_ALPHA_NUM)
                    / VC_SMOOTH_ALPHA_DEN);
                vc.y = (SLONG)(((std::int64_t)s.prev_y * keep
                        + (std::int64_t)vc.y * VC_SMOOTH_ALPHA_NUM)
                    / VC_SMOOTH_ALPHA_DEN);
                vc.z = (SLONG)(((std::int64_t)s.prev_z * keep
                        + (std::int64_t)vc.z * VC_SMOOTH_ALPHA_NUM)
                    / VC_SMOOTH_ALPHA_DEN);
            } else {
                s.mode = VC_SM_COLLISION;
            }
            s.prev_x = vc.x;
            s.prev_y = vc.y;
            s.prev_z = vc.z;
        } else {
            // No collision this tick.
            if (s.mode == VC_SM_COLLISION) {
                // Just exited — start the smoothing fade-out. prev_vc is
                // already up to date from the last COLLISION tick.
                s.exit_ticks_left = VC_EXIT_TICKS;
                s.mode = VC_SM_EXIT;
            }
            if (s.mode == VC_SM_EXIT) {
                s.exit_ticks_left--;
                if (s.exit_ticks_left <= 0) {
                    s.mode = VC_SM_OFF;
                    // vc already equals fc (identity copy); prev_vc no longer used.
                } else {
                    // Same lerp formula as COLLISION mode, but
                    //   (a) target = fc (vc was left as identity by
                    //       vc_process_one since in_collision is false), and
                    //   (b) keep weight is linearly scaled by ticks_left so
                    //       it fades from the COLLISION base value to 0.
                    // Net effect: lerp converges toward fc as keep → 0,
                    // exactly degenerating to vc = fc at timer end. No
                    // velocity discontinuity at the COL→EXIT boundary
                    // because the lerp mechanism is unchanged — only the
                    // target switched (from collision-shifted to identity)
                    // and the weight started shrinking.
                    SLONG keep_num = (VC_SMOOTH_ALPHA_DEN - VC_SMOOTH_ALPHA_NUM)
                                     * s.exit_ticks_left;
                    SLONG keep_den = VC_EXIT_TICKS * VC_SMOOTH_ALPHA_DEN;
                    SLONG drop_num = keep_den - keep_num;
                    SLONG fc_x = vc.x, fc_y = vc.y, fc_z = vc.z;
                    vc.x = (SLONG)(((std::int64_t)s.prev_x * keep_num
                                    + (std::int64_t)fc_x * drop_num)
                                   / keep_den);
                    vc.y = (SLONG)(((std::int64_t)s.prev_y * keep_num
                                    + (std::int64_t)fc_y * drop_num)
                                   / keep_den);
                    vc.z = (SLONG)(((std::int64_t)s.prev_z * keep_num
                                    + (std::int64_t)fc_z * drop_num)
                                   / keep_den);
                    s.prev_x = vc.x;
                    s.prev_y = vc.y;
                    s.prev_z = vc.z;
                }
            }
            // mode == OFF: vc stays as identity copy of fc, nothing to do.
        }
    }
}
