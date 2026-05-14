#include "camera/vis_cam.h"
#include "ai/mav.h"                   // MAV_inside, MAVHEIGHT
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "engine/core/macros.h"       // QDIST3, WITHIN
#include "engine/core/math.h"         // SIN / COS
#include "engine/input/input_frame.h" // input_key_press_pending / consume
#include "engine/input/keyboard.h"    // KB_BACKSLASH
#include "map/map.h"                  // MAP_WIDTH, MAP_HEIGHT
#include <cstdint>
#include <cstdio>  // fprintf, stderr — dump-on-key
#include <cstdlib> // abs

// \  key dumps detailed per-sample state for the centre ray into stderr
// (which the Makefile pipes into stderr.log next to the exe). Useful for
// chasing issues with the obstacle-detection heuristic — e.g. small bumps
// that aren't being caught, or false positives. Set by the input poll in
// VC_process, consumed by vc_process_one once it has all the data.
static bool s_vc_dump_pending = false;
static SLONG s_vc_dump_counter = 0;

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

// MAVHEIGHT delta above the focus cell needed to count a sample as "wall".
// Without this, MAV_inside returns true on every step along flat ground —
// MAV_inside is "below the MAVHEIGHT surface", and on a flat outdoor stretch
// the surface is everywhere at ground level, so every below-surface point
// looks like an obstacle. A real wall/building/curb raises MAVHEIGHT enough
// in its own cell to clear this threshold. Units are raw MAVHEIGHT (1 unit
// = 64 shifted Y).
static const SLONG VC_OBSTACLE_MAVH_THRESHOLD = 4;

// Ray-start Y offset above focus — lifts the first sample just out of
// focus's own body so the very first step doesn't read its own cell.
// Same value the baseline used.
static const SLONG VC_RAY_START_Y_OFFSET = 0x80;

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
// focus when geometry would otherwise punch into the frame.
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
static void vc_process_one(FC_Cam* fc, VC_State& vc, const VC_FocusPoint& focus)
{
    // Cutscenes run their camera through EWAY_grab_camera / PLAYCUTS and own
    // their framing — we leave vc as identity in that case.
    if (EWAY_cam_active || PLAYCUTS_playing) return;

    // No focus → vc stays as identity copy of fc->* (which is itself stale,
    // but we don't have a meaningful raycast origin without focus).
    if (fc->focus == NULL) return;

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

    // \  press: log per-sample detail for the centre ray to stderr.log.
    // Captures MAV_inside / MAVHEIGHT readouts at each step so we can see
    // where (and why) the algorithm decided clear vs hit — useful for
    // tracking down small obstacles the heuristic misses.
    if (s_vc_dump_pending) {
        s_vc_dump_pending = false;
        s_vc_dump_counter++;
        fprintf(stderr,
            "=== VC dump #%d ===\n"
            "  focus=(%d,%d,%d) cell=(%d,%d) mavh=%d\n"
            "  vc=(%d,%d,%d)\n"
            "  yaw_psx=%d  rx=%d rz=%d ry=%d\n"
            "  threshold=%d  fails=%d min_d=0x%X\n",
            (int)s_vc_dump_counter,
            (int)focus.x, (int)focus.y, (int)focus.z,
            (int)focus_cell_x, (int)focus_cell_z, (int)focus_mavh,
            (int)vc.x, (int)vc.y, (int)vc.z,
            (int)yaw_psx, (int)rx, (int)rz, (int)ry,
            (int)VC_OBSTACLE_MAVH_THRESHOLD, (int)fail_count, (unsigned)min_d_hit);

        SLONG sx = focus.x;
        SLONG sy = focus.y + VC_RAY_START_Y_OFFSET;
        SLONG sz = focus.z;
        SLONG step_x = (vc.x - sx) / VC_RAY_SAMPLES;
        SLONG step_y = (vc.y - sy) / VC_RAY_SAMPLES;
        SLONG step_z = (vc.z - sz) / VC_RAY_SAMPLES;
        for (SLONG step = 1; step <= VC_RAY_SAMPLES; step++) {
            sx += step_x;
            sy += step_y;
            sz += step_z;
            SLONG mi = MAV_inside(sx >> 8, sy >> 8, sz >> 8);
            SLONG cx_dbg = sx >> 16;
            SLONG cz_dbg = sz >> 16;
            SLONG mavh_dbg = 0;
            if (WITHIN(cx_dbg, 0, MAP_WIDTH - 1) && WITHIN(cz_dbg, 0, MAP_HEIGHT - 1)) {
                mavh_dbg = MAVHEIGHT(cx_dbg, cz_dbg);
            }
            const char* verdict = (mi && mavh_dbg > focus_mavh + VC_OBSTACLE_MAVH_THRESHOLD)
                ? "HIT"
                : (mi ? "inside-but-filtered" : "outside");
            fprintf(stderr,
                "  [%02d] s=(%d,%d,%d) cell=(%d,%d) MAV_inside=%d mavh=%d %s\n",
                (int)step, (int)sx, (int)sy, (int)sz,
                (int)cx_dbg, (int)cz_dbg, (int)mi, (int)mavh_dbg, verdict);
        }
        fflush(stderr);
    }

    if (fail_count == 0) return;

    SLONG dx = vc.x - focus.x;
    SLONG dy = vc.y - focus.y;
    SLONG dz = vc.z - focus.z;
    SLONG d_now = QDIST3(std::abs(dx), std::abs(dy), std::abs(dz));
    if (d_now <= 0) return; // degenerate — camera coincident with focus

    SLONG d_new = min_d_hit - VC_WALL_OFFSET;

    // Guard against moving the camera OUT (away from focus). vc_probe_mav
    // extends rays past the corner by (WALL_OFFSET + NEAR_PAD) for early
    // detection, which means d_hit can register past d_now — in that case
    // d_new would land at or above d_now, and writing it back would push
    // the camera further from focus. Skip in that range. The boundary
    // (d_new == d_now) is where the wall first enters its NEAR_PAD margin;
    // a fraction past that, d_new < d_now and we start pulling the camera
    // in smoothly.
    if (d_new >= d_now) return;

    if (d_new <= 0) {
        // Hit closer than the wall-offset clearance — slide the camera all
        // the way onto focus. Overshooting focus (camera ends up on the far
        // side) is acceptable by design when Darci is flush against a wall.
        vc.x = focus.x;
        vc.y = focus.y;
        vc.z = focus.z;
        return;
    }

    // Scale (vc - focus) by d_new/d_now. 64-bit intermediate to avoid
    // overflow — dx and d_new can each reach hundreds of thousands in
    // shifted units, and their product can exceed 2^31.
    vc.x = focus.x + (SLONG)((std::int64_t)dx * d_new / d_now);
    vc.y = focus.y + (SLONG)((std::int64_t)dy * d_new / d_now);
    vc.z = focus.z + (SLONG)((std::int64_t)dz * d_new / d_now);
}

void VC_process(void)
{
    // Latch \  press here so the dump captures the same tick as the user
    // pressing it. Press-pending+consume survives FPS > physics rate
    // (same pattern as F5/F6/F7).
    if (input_key_press_pending(KB_BACKSLASH)) {
        input_key_consume(KB_BACKSLASH);
        s_vc_dump_pending = true;
    }

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

        // Focus point passed into processing. Currently the FC_look_at_focus
        // want-target: focus_x/z (object position) and focus_y + lookabove
        // (Y-target the camera aims at). Treated as opaque by vc_process_one
        // — if a better focus source is found, only this construction
        // changes.
        VC_FocusPoint focus = {
            fc->focus_x,
            fc->focus_y + fc->lookabove,
            fc->focus_z,
        };

        vc_process_one(fc, vc, focus);
    }
}
