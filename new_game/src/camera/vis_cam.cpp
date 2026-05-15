#include "camera/vis_cam.h"
#include "ai/mav.h"                   // MAV_inside, MAVHEIGHT
#include "buildings/prim.h"           // get_prim_info, PrimInfo
#include "buildings/ware.h"           // WARE_inside (indoor sampler)
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "engine/core/fixed_math.h"   // MUL64
#include "engine/core/macros.h"       // QDIST2, QDIST3, WITHIN
#include "engine/core/math.h"         // SIN / COS
#include "engine/physics/collide.h"   // there_is_a_los, LOS_FLAG_*
#include "buildings/prim_types.h"     // PrimFace4, PrimPoint
#include "game/game_types.h"          // TO_THING, THINGS, THING_NUMBER
#include "map/level_pools.h"          // prim_points, prim_faces4
#include "map/map.h"                  // MAP_WIDTH, MAP_HEIGHT
#include "map/pap.h"                  // PAP_calc_map_height_at
#include "navigation/wmove.h"         // WMOVE_face — vehicle top-surface tris
#include "things/characters/person_types.h" // Person, Person.InCar
#include "things/core/thing.h"        // Thing, thing_class_head, CLASS_VEHICLE/PERSON
#include "things/vehicles/vehicle.h"  // Vehicle, get_vehicle_body_prim/offset
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

// --- Vehicle (CLASS_VEHICLE) collision -----------------------------------
//
// Vehicles are dynamic Things, not part of the MAV/WARE/PAP static grids the
// other probes read, so they need their own probe.
//
// Hybrid model. Sides/footprint use the oriented bounding box (OBB): centre =
// Thing.WorldPos, yaw = Vehicle.Angle, local extents = the body prim's
// PrimInfo min/max — the exact representation the engine's own vehicle
// physics uses for blocking (collide_box_with_line / slide_around_box), and
// near-vertical car sides are well approximated by it. The TOP, however, is
// badly approximated by the flat box ceiling (PrimInfo.maxy held across the
// whole length), so we replace it per-(x,z) with the real authored top
// surface: the WMOVE walkable triangles the engine attaches to each vehicle
// (the same ridge a character runs along on a car roof). Those triangles are
// transformed to current-tick world space by WMOVE_process — which runs
// before FC_process/VC_process — so we read them live rather than
// duplicating the per-type tri tables. Where the ridge strip doesn't cover
// an (x,z), we fall back to the flat box top, so the hybrid is never worse
// than box-only, only better where authored geometry exists.

// Max nearby vehicles considered per tick. Matches VEH_MAX_COL (the physics
// side's own vehicle-collision candidate cap) — more than this many cars
// crowding a single camera shot doesn't happen in practice.
static const SLONG VC_MAX_VEHICLE_CANDIDATES = 8;

// Extra slack (world>>8 units) added to (camera distance + vehicle bounding
// radius) when deciding whether a vehicle is close enough to bother probing.
// 0x100 ≈ one map cell — comfortably covers the camera sphere radius
// (VC_CAMERA_RADIUS>>8 = 0x40) plus the ray extension
// ((VC_WALL_OFFSET+VC_NEAR_CLIP_PAD)>>8 = 0x58) with margin, so the
// broad-phase never rejects a vehicle the ray walk could still hit.
static const SLONG VC_VEHICLE_REACH_MARGIN = 0x100;

// Max WMOVE top-surface quads kept per vehicle. The authored tables top out
// at 5 walkable faces (car/taxi/police/sedan); 6 leaves headroom. Each WMOVE
// face is a parallelogram (4 points), not a triangle — see vc_quad_surface_y.
static const SLONG VC_MAX_VEHICLE_QUADS = 6;

// In-triangle test tolerance for the top-surface barycentric query, as a
// 16.16 fraction (0x1000 = 1/16 ≈ 6%). The ridge is a strip of separate
// triangles; a small tolerance bridges the seams between adjacent strip
// triangles so a sample landing exactly on a shared edge still gets the
// authored ceiling instead of dropping to the box-top fallback. Matches the
// spirit of the engine's own ~5% face expansion (point_in_quad, 268/256).
static const SLONG VC_TRI_BARY_TOLERANCE = 0x1000;

// Vertical datum correction for the vehicle box, mirrored verbatim from the
// engine's own vehicle Y-bounds (vehicle.cpp run-over check):
//   miny = ((WorldPos.Y - get_vehicle_body_offset(type)) >> 8) + pi->miny - 0x80
//   maxy = ((WorldPos.Y - get_vehicle_body_offset(type)) >> 8) + pi->maxy - 0x80
// The body offset places the body prim relative to the wheel-contact
// WorldPos.Y; WMOVE_get_pos already subtracts it (y -= offy>>8) so the ridge
// surface is on the correct datum, but the OBB must subtract it too or the
// box floor sits ~one body-height below the ridge and the collision slab is
// inflated ~2×. 0x80 is the engine's small extra bias; kept identical so our
// box matches the engine's vehicle bounds exactly.
static const SLONG VC_VEHICLE_BODY_Y_BIAS = 0x80;

// One triangle, world>>8 units. The barycentric primitive used to interpolate
// the surface Y; a WMOVE quad is tested as two of these.
struct VC_VehicleTri {
    SLONG x0, y0, z0;
    SLONG x1, y1, z1;
    SLONG x2, y2, z2;
};

// One vehicle top-surface WMOVE face, world>>8 units (same space as
// WorldPos>>8 and the ray samples). It is a parallelogram, not a triangle:
// WMOVE_process fills point [3] = P1 + P2 - P0 and the engine walks it as a
// full quad. Stored as the 4 corners P0..P3 (P0,P1,P3,P2 around the perimeter).
struct VC_VehicleQuad {
    SLONG x[4], y[4], z[4];
};

// Per-tick snapshot of one nearby vehicle's collision hull, world>>8 units
// (the space WorldPos>>8 and PrimInfo extents already live in, and the space
// collide_box_with_line operates in). The OBB gives footprint/sides/floor;
// `tri[]` is the authored top ridge that refines the ceiling per-(x,z).
// sin_a/cos_a are SIN/COS of the box-inverse angle (-yaw), 16.16 fixed-point,
// precomputed once at gather time so the per-sample box test is two MUL64s.
struct VC_VehicleBox {
    SLONG mid_x, mid_z;
    SLONG min_x, min_z;
    SLONG max_x, max_z;
    SLONG y_bottom;       // box floor: WorldPos.Y>>8 + PrimInfo.miny
    SLONG y_fallback_top; // flat box top, used where no ridge tri covers (x,z)
    SLONG sin_a, cos_a;
    VC_VehicleQuad quad[VC_MAX_VEHICLE_QUADS];
    SLONG nquad;
};

// XZ point-in-triangle test for one top-surface triangle; on success writes
// the barycentric-interpolated surface Y at (px,pz) to *surf_y. Pure 2D
// barycentric (the same model WMOVE_relative_pos uses), 64-bit intermediates
// so the world>>8-scale cross products can't overflow. Returns false for a
// degenerate (zero-area in XZ) triangle.
static bool vc_tri_surface_y(const VC_VehicleTri& t, SLONG px, SLONG pz,
    SLONG* surf_y)
{
    SLONG e1x = t.x1 - t.x0, e1z = t.z1 - t.z0;
    SLONG e2x = t.x2 - t.x0, e2z = t.z2 - t.z0;
    SLONG ppx = px - t.x0, ppz = pz - t.z0;

    std::int64_t denom = (std::int64_t)e1x * e2z - (std::int64_t)e2x * e1z;
    if (denom == 0) return false;

    // a, b as 16.16 fractions: a along e1 (v0→v1), b along e2 (v0→v2).
    std::int64_t a = (((std::int64_t)ppx * e2z - (std::int64_t)e2x * ppz) << 16) / denom;
    std::int64_t b = (((std::int64_t)e1x * ppz - (std::int64_t)ppx * e1z) << 16) / denom;

    const std::int64_t tol = VC_TRI_BARY_TOLERANCE;
    if (a < -tol || b < -tol || a + b > (std::int64_t)0x10000 + tol)
        return false;

    *surf_y = t.y0
        + (SLONG)((a * (t.y1 - t.y0)) >> 16)
        + (SLONG)((b * (t.y2 - t.y0)) >> 16);
    return true;
}

// Surface Y of a WMOVE parallelogram face at (px,pz). The face is a quad
// P0,P1,P3,P2 (P3 = P1+P2-P0); the engine walks it as two triangles split on
// the P1–P2 diagonal — {P0,P1,P2} and {P1,P3,P2} (cf.
// get_height_on_face_quad64_at). Test both; first that covers (px,pz) wins.
// This is what makes the whole roof collide, not just half of it.
static bool vc_quad_surface_y(const VC_VehicleQuad& q, SLONG px, SLONG pz,
    SLONG* surf_y)
{
    VC_VehicleTri t1 = { q.x[0], q.y[0], q.z[0],
                         q.x[1], q.y[1], q.z[1],
                         q.x[2], q.y[2], q.z[2] };
    if (vc_tri_surface_y(t1, px, pz, surf_y)) return true;

    VC_VehicleTri t2 = { q.x[1], q.y[1], q.z[1],
                         q.x[3], q.y[3], q.z[3],
                         q.x[2], q.y[2], q.z[2] };
    return vc_tri_surface_y(t2, px, pz, surf_y);
}

// Collect vehicles whose bounding sphere is within reach of the focus→camera
// span into `out`. `exclude` is the focus Thing (the car becomes focus while
// driving); `exclude_vehicle` is the vehicle the focus person is entering /
// in (Person.InCar, set from the first frame of the enter animation). Both
// are skipped — probing the car you're getting into glues the camera to
// focus and makes the enter-zoom glitch. Returns the candidate count.
static SLONG vc_gather_vehicles(const VC_FocusPoint& focus, SLONG d_now,
    const Thing* exclude, const Thing* exclude_vehicle, VC_VehicleBox* out)
{
    SLONG fx8 = focus.x >> 8;
    SLONG fz8 = focus.z >> 8;
    // Broad-phase reach: camera distance plus per-vehicle radius (added
    // below) plus slack. d_now is QDIST3 in full units; the XZ test works
    // in world>>8, so shift d_now down to match.
    SLONG reach_xz = (d_now >> 8) + VC_VEHICLE_REACH_MARGIN;

    SLONG n = 0;
    for (UWORD idx = thing_class_head[CLASS_VEHICLE]; idx;
         idx = TO_THING(idx)->NextLink) {
        Thing* p = TO_THING(idx);
        if (p == exclude || p == exclude_vehicle) continue;

        Vehicle* v = p->Genus.Vehicle;
        PrimInfo* pi = get_prim_info(get_vehicle_body_prim(v->Type));

        SLONG mx = p->WorldPos.X >> 8;
        SLONG mz = p->WorldPos.Z >> 8;
        SLONG ddx = std::abs(mx - fx8);
        SLONG ddz = std::abs(mz - fz8);
        if (QDIST2(ddx, ddz) > reach_xz + pi->radius) continue;

        if (n >= VC_MAX_VEHICLE_CANDIDATES) break;

        // Vertical datum = engine's vehicle Y-bounds formula
        // (vehicle.cpp run-over check). Body offset + bias put the box on
        // the same datum as the WMOVE ridge (which already has -offset).
        SLONG vy = ((p->WorldPos.Y - get_vehicle_body_offset(v->Type)) >> 8)
                   - VC_VEHICLE_BODY_Y_BIAS;
        VC_VehicleBox& b = out[n++];
        b.mid_x = mx;
        b.mid_z = mz;
        b.min_x = pi->minx;
        b.min_z = pi->minz;
        b.max_x = pi->maxx;
        b.max_z = pi->maxz;
        b.y_bottom = vy + pi->miny;
        b.y_fallback_top = vy + pi->maxy;

        // Same inverse-rotation setup as collide_box_with_line:
        // useangle = (-yaw) & 2047, matrix = {cos, sin, -sin, cos}.
        SLONG useangle = (-(v->Angle)) & 2047;
        b.sin_a = SIN(useangle);
        b.cos_a = COS(useangle);

        // Authored top surface: this vehicle's WMOVE walkable faces, in
        // current-tick world>>8 (WMOVE_process already transformed them by
        // the full yaw/tilt/roll matrix this tick — see file header). Each
        // WMOVE_Face is a 4-point parallelogram (point [3] = P1+P2-P0); we
        // keep all four so the whole roof collides, not half. Filter by
        // owner Thing index.
        SLONG vidx = THING_NUMBER(p);
        b.nquad = 0;
        for (SLONG fi = 1;
             fi < WMOVE_face_upto && b.nquad < VC_MAX_VEHICLE_QUADS; fi++) {
            const WMOVE_Face* wf = &WMOVE_face[fi];
            if (wf->thing != (UWORD)vidx) continue;

            const PrimFace4* f4 = &prim_faces4[wf->face4];
            VC_VehicleQuad& qd = b.quad[b.nquad++];
            for (SLONG k = 0; k < 4; k++) {
                const PrimPoint& pp = prim_points[f4->Points[k]];
                qd.x[k] = pp.X;
                qd.y[k] = pp.Y;
                qd.z[k] = pp.Z;
            }
        }
    }
    return n;
}

// Vehicle version of vc_probe_mav. Walks the same extended ray and reports
// the first sample that lands inside any candidate vehicle's hybrid hull:
// the OBB footprint/sides/floor (rotate the focus-relative sample into
// box-local space the same way collide_box_with_line does, range-check XZ,
// lower-bound Y) capped by the real authored top surface — the WMOVE ridge
// triangles, barycentric-interpolated per-(x,z), with the flat box top as
// the fallback where the strip doesn't reach. The top cap also stops a
// camera looking down over a low car from above false-triggering. Hit
// distance uses the same metric as the other probes so the shared
// offset/smoothing path treats a car exactly like a wall.
static bool vc_probe_vehicles(const VC_FocusPoint& focus,
    SLONG tx, SLONG ty, SLONG tz,
    const VC_VehicleBox* cands, SLONG ncands, SLONG* d_hit_out)
{
    if (ncands == 0) return true;

    // Same ray-extension trick as vc_probe_mav: walk past the camera by
    // WALL_OFFSET + NEAR_CLIP_PAD so first contact is smooth instead of
    // jumping the camera in by the offset on the first hit frame.
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

        SLONG px = sx >> 8;
        SLONG py = sy >> 8;
        SLONG pz = sz >> 8;

        for (SLONG c = 0; c < ncands; c++) {
            const VC_VehicleBox& b = cands[c];

            // Footprint / sides / floor: OBB in XZ + lower bound. Same
            // rotation as collide_box_with_line: matrix = {cos, sin, -sin,
            // cos}, rx = tx·m0 + tz·m1, rz = tx·m2 + tz·m3.
            if (py < b.y_bottom) continue;
            SLONG dxp = px - b.mid_x;
            SLONG dzp = pz - b.mid_z;
            SLONG rx = MUL64(dxp, b.cos_a) + MUL64(dzp, b.sin_a);
            SLONG rz = MUL64(dxp, -b.sin_a) + MUL64(dzp, b.cos_a);
            if (!WITHIN(rx, b.min_x, b.max_x)
                || !WITHIN(rz, b.min_z, b.max_z))
                continue;

            // Ceiling: real authored top surface at this (x,z) if the ridge
            // strip covers it, else the flat box top. Sample is inside the
            // car only if at or below that ceiling.
            SLONG ceil_y = b.y_fallback_top;
            for (SLONG qi = 0; qi < b.nquad; qi++) {
                SLONG surf_y;
                if (vc_quad_surface_y(b.quad[qi], px, pz, &surf_y)) {
                    ceil_y = surf_y;
                    break;
                }
            }
            if (py > ceil_y) continue;

            SLONG full_len = QDIST3(std::abs(extended_tx - focus.x),
                std::abs(extended_ty - focus.y),
                std::abs(extended_tz - focus.z));
            *d_hit_out = (SLONG)((std::int64_t)full_len * step / VC_RAY_SAMPLES);
            return false;
        }
    }
    return true;
}

// Sample-based ray from focus toward (tx,ty,tz). Walks VC_RAY_SAMPLES points
// along the segment, testing two conditions at each sample:
//   1. Wall/platform — MAV_inside is true AND the cell's MAVHEIGHT is
//      meaningfully above focus's own cell (see VC_OBSTACLE_MAVH_THRESHOLD).
//   2. Ground/slope — the sample's Y is below the local ground height
//      (PAP_calc_map_height_at). Catches the camera diving under the
//      terrain on rising slopes / hills.
// Either match counts as a hit. On hit, *d_hit is the distance from focus
// to the hit sample. Returns true if no hit was found.
//
// Why MAV_inside in addition to ground check: outdoor city buildings in
// Urban Chaos are encoded as raised cells in the MAV/PAP height grid, not
// as mesh DFacets. The DFacet-based LOS functions return CLEAR through
// such walls. MAV_inside + MAVHEIGHT sees them. PAP_calc_map_height_at
// fills in the terrain side of the picture (slopes and hills, which are
// stored in PAP_Hi.Alt corner heights rather than MAVHEIGHT outcrops).
//
// Coordinate convention: focus and target are in shifted world units
// (the same as FC_cam fields). MAV_inside expects its args already shifted
// right by 8 (i.e. world/256) — we apply that at the call site. MAVHEIGHT
// is a 2D array indexed by PAP-HI cell, which is world >> 16.
// PAP_calc_map_height_at takes (x, z) in world/256 units and returns the
// ground height in the same units, so we shift its result up by 8 to
// compare against shifted sy.
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

        bool hit = false;

        // Condition 1: wall/platform via MAVHEIGHT outcrop.
        if (MAV_inside(sx >> 8, sy >> 8, sz >> 8)) {
            SLONG cell_x = sx >> 16;
            SLONG cell_z = sz >> 16;
            if (WITHIN(cell_x, 0, MAP_WIDTH - 1) && WITHIN(cell_z, 0, MAP_HEIGHT - 1)) {
                SLONG sample_mavh = MAVHEIGHT(cell_x, cell_z);
                if (sample_mavh > focus_mavh + VC_OBSTACLE_MAVH_THRESHOLD) {
                    hit = true;
                }
            }
        }

        // Condition 2: sample below local ground (slope/hill).
        // NOTE: don't try to skip PAP_FLAG_HIDDEN cells here — outdoor raised
        // geometry (decorative platforms, steps) sets that flag too, and
        // those cells are precisely the ones whose elevation we need.
        // Indoor focus uses vc_probe_ware instead, so this path won't fire
        // inside warehouses where PAP_calc_map_height_at would return a
        // ceiling-height sentinel.
        if (!hit) {
            SLONG ground_y = PAP_calc_map_height_at(sx >> 8, sz >> 8) << 8;
            if (sy < ground_y) hit = true;
        }

        if (!hit) continue;

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

// Indoor (warehouse) version of vc_probe_mav. Walks a ray from focus toward
// (tx,ty,tz) and reports the first sample that is NOT inside the warehouse's
// passable volume (i.e. inside a wall or above ceiling). Uses the WARE_*
// system instead of MAV_inside/MAVHEIGHT, which don't represent warehouse
// interiors usefully — MAVHEIGHT inside a building is a sentinel and
// PAP_calc_map_height_at falls back to MAVHEIGHT << 6 (ceiling height),
// neither of which discriminates walls from air.
//
// WARE_inside semantics (same as MAV_inside — TRUE = blocked / inside an
// obstacle, FALSE = passable air). Cross-checked against MAV_height_los_slow
// which calls WARE_inside as the LOS-blocked predicate. Outside-footprint
// also returns TRUE (LOS treats stepping outside the warehouse as blocked,
// preventing the line of sight from leaking through the building shell).
// We treat TRUE the same way: any sample reporting "blocked" is a wall hit.
//
// Coordinate convention: WARE_inside expects **unshifted** world coords
// (see uc_orig fc.cpp wall-collision call site, which passes focus_x >> 8).
// Our `sx`, `sy`, `sz` are shifted-world (FC_cam space), so we right-shift
// by 8 at the call. MAV_inside in the outdoor probe takes the same
// convention but we shifted at the call there too.
//
// Same ray-extension trick as vc_probe_mav: walk past the camera by
// WALL_OFFSET + NEAR_CLIP_PAD so first contact is smooth rather than
// jumping the camera in by the offset on the first hit frame.
static bool vc_probe_ware(const VC_FocusPoint& focus,
    SLONG tx, SLONG ty, SLONG tz,
    UBYTE ware, SLONG* d_hit_out)
{
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

    // LOS flags for the DFacet-aware check below. Some interior walls (e.g.
    // RTA police station) aren't in the WARE height grid — they're DFacets.
    // Physics blocks Darci through there_is_a_los; we mirror that here so
    // the camera sees the same walls.
    //   IGNORE_PRIMS              — small objects (lamps, bins) shouldn't block.
    //   IGNORE_SEETHROUGH_FENCE_FLAG — see-through fences must not block.
    //   IGNORE_UNDERGROUND_CHECK  — mandatory indoors: the default underground
    //     guard inside there_is_a_los calls PAP_calc_map_height_at, which in
    //     PAP_FLAG_HIDDEN cells returns the MAVHEIGHT<<6 sentinel (ceiling
    //     height) and reports every sample "below ground" — every LOS call
    //     would then be blocked and camera glued to focus.
    const SLONG LOS_FLAGS = LOS_FLAG_IGNORE_PRIMS
        | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG
        | LOS_FLAG_IGNORE_UNDERGROUND_CHECK;

    // Phase 1 — WARE_inside sample walk. Catches walls represented in the
    // warehouse height grid (most interior walls of standard warehouses).
    SLONG ware_hit_step = 0;
    bool ware_hit = false;

    SLONG sample_x[VC_RAY_SAMPLES + 1]; // index 0 = focus start, 1..N = each step
    SLONG sample_y[VC_RAY_SAMPLES + 1];
    SLONG sample_z[VC_RAY_SAMPLES + 1];
    sample_x[0] = sx;
    sample_y[0] = sy;
    sample_z[0] = sz;

    for (SLONG step = 1; step <= VC_RAY_SAMPLES; step++) {
        sx += step_x;
        sy += step_y;
        sz += step_z;
        sample_x[step] = sx;
        sample_y[step] = sy;
        sample_z[step] = sz;

        if (WARE_inside(ware, sx >> 8, sy >> 8, sz >> 8)) {
            ware_hit = true;
            ware_hit_step = step;
            break;
        }
    }

    // Phase 2 — full-ray DFacet LOS check. Catches walls that exist as
    // DFacets but not in WARE_height (some boundary walls). One full-length
    // call rather than per-segment calls because there_is_a_los bypasses
    // segments shorter than 16 unshifted units (our per-sample step is
    // ~12), which would silently skip the DFacet test. If blocked,
    // binary-search the sample steps for the first one where LOS becomes
    // blocked — log2(N) ≈ 5 calls per blocked ray.
    SLONG los_hit_step = 0;
    bool los_hit = false;

    bool full_los_clear = there_is_a_los(
        focus.x >> 8, focus.y >> 8, focus.z >> 8,
        extended_tx >> 8, extended_ty >> 8, extended_tz >> 8,
        LOS_FLAGS);

    if (!full_los_clear) {
        // Step 0 (focus itself) is trivially clear; the full ray is known
        // blocked. Invariant: LOS(focus, sample[lo]) clear, LOS(focus,
        // sample[hi]) blocked. Converges with `hi` being the smallest step
        // where LOS first fails.
        SLONG lo = 0;
        SLONG hi = VC_RAY_SAMPLES;
        while (hi - lo > 1) {
            SLONG mid = (lo + hi) / 2;
            bool clear = there_is_a_los(
                focus.x >> 8, focus.y >> 8, focus.z >> 8,
                sample_x[mid] >> 8, sample_y[mid] >> 8, sample_z[mid] >> 8,
                LOS_FLAGS);
            if (clear) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        los_hit = true;
        los_hit_step = hi;
    }

    // Combine — take the closer of the two hits, if any.
    SLONG hit_step = 0;
    bool hit_found = false;
    if (ware_hit && los_hit) {
        hit_step = (ware_hit_step < los_hit_step) ? ware_hit_step : los_hit_step;
        hit_found = true;
    } else if (ware_hit) {
        hit_step = ware_hit_step;
        hit_found = true;
    } else if (los_hit) {
        hit_step = los_hit_step;
        hit_found = true;
    }

    if (!hit_found) return true;

    SLONG full_len = QDIST3(std::abs(extended_tx - focus.x),
        std::abs(extended_ty - focus.y),
        std::abs(extended_tz - focus.z));
    *d_hit_out = (SLONG)((std::int64_t)full_len * hit_step / VC_RAY_SAMPLES);
    return false;
}

// Processing layer for one camera. Reads the identity-copy `vc` plus the
// focus point and mutates `vc.*` to push the visible camera in toward the
// focus when geometry would otherwise punch into the frame. Returns true
// if `vc` was actually shifted (i.e. a wall was detected and acted upon).
// Smoothing is applied later, in VC_process — this function only computes
// the raw target position.
//
// `ware` selects the geometry source:
//   ware == 0  → outdoor probe (MAV_inside + MAVHEIGHT + PAP ground check).
//   ware != 0  → indoor probe (WARE_inside against that warehouse id).
// The dispatch is per-camera (one branch picked for all five rays) rather
// than per-sample, because focus_in_warehouse is fixed per tick.
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
// `focus_thing` is the Thing the camera looks at; `incar_vehicle` is the
// vehicle the focus person is entering / in (or NULL). Both are passed
// through to vehicle gathering so the car the player is getting into is not
// probed (see vc_gather_vehicles).
static bool vc_process_one(VC_State& vc, const VC_FocusPoint& focus,
    UBYTE ware, const Thing* focus_thing, const Thing* incar_vehicle)
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

    // Focus→camera vector and distance. Needed both as the broad-phase reach
    // for vehicle gathering and, later, to scale the camera toward focus.
    SLONG dx = vc.x - focus.x;
    SLONG dy = vc.y - focus.y;
    SLONG dz = vc.z - focus.z;
    SLONG d_now = QDIST3(std::abs(dx), std::abs(dy), std::abs(dz));
    if (d_now <= 0) return false; // degenerate — camera coincident with focus

    // Nearby vehicles, gathered once per tick (not per ray/sample) to keep
    // the cost bounded on weak hardware.
    VC_VehicleBox veh[VC_MAX_VEHICLE_CANDIDATES];
    SLONG nveh = vc_gather_vehicles(focus, d_now, focus_thing, incar_vehicle, veh);

    SLONG min_d_hit = 0;
    SLONG fail_count = 0;
    SLONG d_hit;

    // Indoor (warehouse) branch uses WARE_inside; outdoor branch uses the
    // MAV/MAVHEIGHT + PAP ground combo. They probe different data so each
    // gets its own per-ray dispatch macro to avoid a runtime test inside
    // the probe loop (32 samples × 5 rays per tick).
    if (ware != 0) {

#define VC_PROBE(SX, SY, SZ)                                                   \
    do {                                                                       \
        if (!vc_probe_ware(focus, (SX), (SY), (SZ), ware, &d_hit)) {           \
            if (fail_count == 0 || d_hit < min_d_hit) min_d_hit = d_hit;       \
            fail_count++;                                                      \
        }                                                                      \
        if (!vc_probe_vehicles(focus, (SX), (SY), (SZ), veh, nveh, &d_hit)) {  \
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

    } else {

        // Focus cell's MAVHEIGHT — the baseline that ray samples are compared
        // against. Without this filter, MAV_inside fires on every sample along
        // flat ground (see VC_OBSTACLE_MAVH_THRESHOLD comment).
        SLONG focus_cell_x = focus.x >> 16;
        SLONG focus_cell_z = focus.z >> 16;
        SLONG focus_mavh = 0;
        if (WITHIN(focus_cell_x, 0, MAP_WIDTH - 1) && WITHIN(focus_cell_z, 0, MAP_HEIGHT - 1)) {
            focus_mavh = MAVHEIGHT(focus_cell_x, focus_cell_z);
        }

#define VC_PROBE(SX, SY, SZ)                                                   \
    do {                                                                       \
        if (!vc_probe_mav(focus, (SX), (SY), (SZ), focus_mavh, &d_hit)) {      \
            if (fail_count == 0 || d_hit < min_d_hit) min_d_hit = d_hit;       \
            fail_count++;                                                      \
        }                                                                      \
        if (!vc_probe_vehicles(focus, (SX), (SY), (SZ), veh, nveh, &d_hit)) {  \
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

    }

    if (fail_count == 0) return false;

    // dx/dy/dz/d_now were computed up front (also used for vehicle reach).
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

        // Vehicle the focus person is entering / in (Person.InCar is set
        // from the first frame of the enter animation through to exit).
        // Excluded from vehicle collision so the enter-zoom doesn't glitch.
        const Thing* incar_vehicle = NULL;
        if (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->InCar) {
            incar_vehicle = TO_THING(fc->focus->Genus.Person->InCar);
        }

        bool in_collision = vc_process_one(vc, focus, fc->focus_in_warehouse,
            fc->focus, incar_vehicle);

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
