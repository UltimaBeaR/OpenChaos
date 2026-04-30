#include "engine/graphics/render_interp.h"

#include "things/core/thing.h"
#include "things/core/drawtype.h"
#include "engine/animation/anim_types.h" // GameKeyFrame layout, ANIM_FLAG_LAST_FRAME
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks (cross-anim blend timing)
#include "game/game_types.h" // THINGS, TO_THING

#include <stdio.h> // debug logging
#include <stdint.h> // uint64_t

// Set to 1 to log first-capture and free events to stderr for diagnosing
// "things flicker across the scene for one frame" bugs.
#define RENDER_INTERP_LOG 1

float g_render_alpha = 0.0f;
bool  g_render_interp_enabled = true;

namespace {

struct ThingSnap {
    GameCoord pos_prev, pos_curr;
    SWORD angle_prev, angle_curr;
    SWORD tilt_prev, tilt_curr;
    SWORD roll_prev, roll_curr;
    bool valid;
    bool skip_once;
    bool has_angles;       // captured Draw.Tweened angles (Tween-based drawables)

    // Anim-transition detection. When MeshID or CurrentAnim change between
    // captures, the pose has discontinuously jumped — interpolating across
    // that boundary produces visual squashing. Detected at capture time and
    // converted into skip_once.
    UBYTE last_mesh_id;
    SLONG last_anim;

    // Vertex-morph fraction (Draw.Tweened->AnimTween, range [0, 256)) plus
    // the morph-endpoint keyframe pointers it indexes. AnimTween is advanced
    // once per physics tick by anim handlers (e.g. person_normal_animate_speed
    // scales by TICK_RATIO), so without interpolation the morph judders at
    // physics rate while body position is smooth.
    //
    // We snapshot the keyframe pointers (CurrentFrame/NextFrame) too so that
    // when a tick crosses a keyframe boundary (`tween_step` >= 256-AnimTween),
    // the render path can lerp through the boundary in a virtual extended
    // coordinate: v = prev_AT + alpha * ((256 - prev_AT) + curr_AT). For
    // v < 256 we render with prev's keyframe pair, for v >= 256 with curr's
    // pair (AnimTween = v - 256). FrameIndex is captured purely as the cheap
    // boundary-detector — its diff selects the apply branch.
    SLONG anim_tween_prev, anim_tween_curr;
    UBYTE frame_index_prev, frame_index_curr;
    struct GameKeyFrame* current_frame_prev;
    struct GameKeyFrame* current_frame_curr;
    struct GameKeyFrame* next_frame_prev;
    struct GameKeyFrame* next_frame_curr;

    // Cross-anim blend (variant A "snap-aligned blend", multi-segment).
    // Activated when CurrentAnim or MeshID change is detected and same-mesh
    // blend is possible. Overrides the standard AnimTween logic for the
    // duration of the blend window.
    //
    // We snapshot the old anim's full state at the moment of transition
    // (CF/NF/AT) and morph in two segments using a virtual extended
    // coordinate v = blend_old_at + blend_t * total where
    // total = (256 - blend_old_at) + 256:
    //
    //   v < 256   — first segment: render(blend_old_cf, blend_old_nf, v)
    //               i.e. complete the old anim's pending morph through
    //               its NextFrame. Starts at v=blend_old_at which equals
    //               the actual pre-transition pose — no start-snap.
    //   v >= 256  — second segment: render(blend_old_nf, live_new_cf, v-256)
    //               morph from old's NextFrame to live new-anim's
    //               CurrentFrame. live_new_cf tracks physics so the new
    //               anim's keyframe progression during the blend is
    //               followed.
    //
    // At v=512 the morph lands at live_new_cf and the standard path
    // resumes. Hand-off discontinuity is bounded by live_at/256 of one
    // keyframe distance — small after only ~2 physics ticks of blend.
    //
    // Skipped when MeshID changes — different vertex layouts cannot be
    // morphed safely.
    bool blend_active;
    uint64_t blend_start_ms;
    struct GameKeyFrame* blend_old_cf;
    struct GameKeyFrame* blend_old_nf;
    SLONG blend_old_at;

    // Frame-scope save: live Thing values overwritten by RenderInterpFrame
    // ctor and restored by dtor. applied=false means this entry was not
    // touched this frame and dtor must skip it.
    GameCoord saved_pos;
    SWORD saved_angle, saved_tilt, saved_roll;
    SLONG saved_anim_tween;
    struct GameKeyFrame* saved_current_frame;
    struct GameKeyFrame* saved_next_frame;
    bool applied;
    bool anim_tween_applied;  // covers AnimTween + CurrentFrame + NextFrame
                              // override. Skipped on >1-keyframe jumps where
                              // we lack the intermediate pointer history.
};

// Cross-anim blend duration (wall-clock ms). 100ms ≈ 2 physics ticks at
// 20 Hz — short enough that fast actions (sprint kick-in, hit reactions)
// don't feel laggy, long enough to hide the pose discontinuity at the
// keyframe-snap endpoints.
static constexpr uint64_t BLEND_DURATION_MS = 100;

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

// FC_MAX_CAMS == 2; only [0] is used in the shipped game, but supporting both
// is trivial.
CamSnap g_cam_snaps[FC_MAX_CAMS] = {};

inline SLONG lerp_i32(SLONG a, SLONG b, float t)
{
    return a + SLONG(float(b - a) * t);
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

// Tween-based DrawTypes: angles live in Draw.Tweened (SWORD, range 0..2047).
// Other DrawTypes either store angles elsewhere (Vehicle: Genus.Vehicle->Draw)
// or have no rotation. For this iteration we interpolate angles only for
// Tween-based things; vehicle rotation will judder visually until added.
inline bool draw_type_uses_tween(UBYTE dt)
{
    switch (dt) {
        case DT_TWEEN:
        case DT_ROT_MULTI:
        case DT_ANIM_PRIM:
        case DT_PYRO:
        case DT_CHOPPER:
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

}  // namespace

void render_interp_reset(void)
{
    for (int i = 0; i < MAX_THINGS; ++i) g_thing_snaps[i] = {};
    for (int i = 0; i < FC_MAX_CAMS; ++i) g_cam_snaps[i] = {};
}

void render_interp_capture(Thing* p_thing)
{
    if (!p_thing) return;
    UWORD idx = thing_index(p_thing);
    if (idx >= MAX_THINGS) return;

    ThingSnap& s = g_thing_snaps[idx];
    DrawTween* dt = draw_type_uses_tween(p_thing->DrawType) ? p_thing->Draw.Tweened : nullptr;

    // Detect anim transitions: if MeshID or CurrentAnim changed since last
    // capture, the pose jumped and lerping in the standard tween path
    // would visibly squash the model across the change. Treat as a
    // teleport, AND attempt to start a render-side cross-anim blend if
    // the mesh structure stayed the same.
    //
    // When DrawType is not Tween-based (dt == nullptr) we drop the cached
    // mesh/anim — otherwise on a later return to Tween the comparison would
    // run against a stale pre-leave value and could miss a real anim
    // transition if the new MeshID happened to match the old one.
    if (dt) {
        if (s.last_mesh_id != dt->MeshID || s.last_anim != dt->CurrentAnim) {
            // Pre-transition state is currently in s.*_curr (set by the
            // previous tick's capture, not yet shifted to *_prev). Use it
            // to seed the blend's "from" representative keyframe.
            //
            // Conditions to start a blend:
            //   - MeshID unchanged across the transition (same vertex
            //     layout — morph between arbitrary keyframes is meaningful)
            //   - We had valid pre-transition keyframe state
            bool mesh_stable = (s.last_mesh_id == dt->MeshID);
            if (mesh_stable && s.valid && s.has_angles
                && s.current_frame_curr && s.next_frame_curr) {
                // Snapshot old anim's full state so first blend segment
                // can render the exact pre-transition pose at v=blend_old_at
                // (no start-snap), then morph through old's NextFrame to
                // the new anim's CurrentFrame in the second segment.
                s.blend_old_cf = s.current_frame_curr;
                s.blend_old_nf = s.next_frame_curr;
                s.blend_old_at = s.anim_tween_curr;
                s.blend_active = true;
                s.blend_start_ms = sdl3_get_ticks();
            } else {
                s.blend_active = false;
            }
            s.skip_once = true;
            s.last_mesh_id = dt->MeshID;
            s.last_anim = dt->CurrentAnim;
        }
    } else {
        s.last_mesh_id = 0;
        s.last_anim = 0;
        s.blend_active = false;
    }

    if (!s.valid) {
#if RENDER_INTERP_LOG
        fprintf(stderr,
            "[render_interp] FIRST_CAPTURE slot=%u class=%d dt=%d pos=(%d,%d,%d)\n",
            idx, p_thing->Class, p_thing->DrawType,
            p_thing->WorldPos.X, p_thing->WorldPos.Y, p_thing->WorldPos.Z);
#endif
        s.pos_curr = p_thing->WorldPos;
        s.pos_prev = s.pos_curr;
        if (dt) {
            s.angle_curr = dt->Angle;
            s.tilt_curr = dt->Tilt;
            s.roll_curr = dt->Roll;
            s.anim_tween_curr = dt->AnimTween;
            s.frame_index_curr = dt->FrameIndex;
            s.current_frame_curr = dt->CurrentFrame;
            s.next_frame_curr = dt->NextFrame;
            s.has_angles = true;
        } else {
            s.has_angles = false;
        }
        s.angle_prev = s.angle_curr;
        s.tilt_prev = s.tilt_curr;
        s.roll_prev = s.roll_curr;
        s.anim_tween_prev = s.anim_tween_curr;
        s.frame_index_prev = s.frame_index_curr;
        s.current_frame_prev = s.current_frame_curr;
        s.next_frame_prev = s.next_frame_curr;
        s.valid = true;
        s.skip_once = false;
        return;
    }

    s.pos_prev = s.pos_curr;
    s.angle_prev = s.angle_curr;
    s.tilt_prev = s.tilt_curr;
    s.roll_prev = s.roll_curr;
    s.anim_tween_prev = s.anim_tween_curr;
    s.frame_index_prev = s.frame_index_curr;
    s.current_frame_prev = s.current_frame_curr;
    s.next_frame_prev = s.next_frame_curr;

    s.pos_curr = p_thing->WorldPos;
    if (dt) {
        s.angle_curr = dt->Angle;
        s.tilt_curr = dt->Tilt;
        s.roll_curr = dt->Roll;
        s.anim_tween_curr = dt->AnimTween;
        s.frame_index_curr = dt->FrameIndex;
        s.current_frame_curr = dt->CurrentFrame;
        s.next_frame_curr = dt->NextFrame;
        s.has_angles = true;
    } else {
        // DrawType changed away from Tween — drop angle interpolation.
        s.has_angles = false;
    }

    if (s.skip_once) {
        // Anim-only skip: keyframe-pair / AnimTween / FrameIndex were
        // discontinuously reset by an anim transition (set_anim/MeshID
        // change). Without this, the standard tween path would lerp
        // across two unrelated keyframe pairs and visibly squash the
        // model. Position and angles are NOT skipped — set_anim leaves
        // WorldPos/Angle/Tilt/Roll alone, so they keep lerping smoothly
        // across the transition. (Previously this skipped everything,
        // freezing position for one tick interval at every transition.)
        s.anim_tween_prev = s.anim_tween_curr;
        s.frame_index_prev = s.frame_index_curr;
        s.current_frame_prev = s.current_frame_curr;
        s.next_frame_prev = s.next_frame_curr;
        s.skip_once = false;
    }
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
}

void render_interp_mark_teleport(Thing* p_thing)
{
    if (!p_thing) return;
    UWORD idx = thing_index(p_thing);
    if (idx >= MAX_THINGS) return;
    g_thing_snaps[idx].skip_once = true;
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

RenderInterpFrame::RenderInterpFrame()
{
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
        p->WorldPos.X = lerp_i32(s.pos_prev.X, s.pos_curr.X, alpha);
        p->WorldPos.Y = lerp_i32(s.pos_prev.Y, s.pos_curr.Y, alpha);
        p->WorldPos.Z = lerp_i32(s.pos_prev.Z, s.pos_curr.Z, alpha);

        if (s.has_angles && draw_type_uses_tween(p->DrawType) && p->Draw.Tweened) {
            DrawTween* dt = p->Draw.Tweened;
            s.saved_angle = dt->Angle;
            s.saved_tilt = dt->Tilt;
            s.saved_roll = dt->Roll;
            dt->Angle = lerp_angle_2048(s.angle_prev, s.angle_curr, alpha);
            dt->Tilt  = lerp_angle_2048(s.tilt_prev,  s.tilt_curr,  alpha);
            dt->Roll  = lerp_angle_2048(s.roll_prev,  s.roll_curr,  alpha);

            // Cross-anim blend supersedes the per-tick AnimTween logic
            // while active. Multi-segment in a virtual extended coordinate
            // v = blend_old_at + blend_t * total, total = (256 - blend_old_at) + 256.
            // First segment (v < 256) renders (blend_old_cf, blend_old_nf, v) —
            // starts at v=blend_old_at = exact pre-transition pose, no
            // start-snap. Second segment (v >= 256) renders
            // (blend_old_nf, live_new_cf, v - 256) — morphs to live new-
            // anim CurrentFrame, which tracks physics during the blend.
            bool blend_apply = false;
            if (s.blend_active && s.blend_old_cf && s.blend_old_nf
                && dt->CurrentFrame) {
                uint64_t now_ms = sdl3_get_ticks();
                uint64_t elapsed = now_ms - s.blend_start_ms;
                if (elapsed >= BLEND_DURATION_MS) {
                    s.blend_active = false;
                } else {
                    float blend_t = float(elapsed) / float(BLEND_DURATION_MS);
                    SLONG total = (256 - s.blend_old_at) + 256;
                    SLONG v = s.blend_old_at + SLONG(blend_t * float(total));

                    s.saved_anim_tween = dt->AnimTween;
                    s.saved_current_frame = dt->CurrentFrame;
                    s.saved_next_frame = dt->NextFrame;

                    if (v < 256) {
                        dt->CurrentFrame = s.blend_old_cf;
                        dt->NextFrame    = s.blend_old_nf;
                        dt->AnimTween    = v;
                    } else {
                        dt->CurrentFrame = s.blend_old_nf;
                        dt->NextFrame    = s.saved_current_frame; // live new CF
                        dt->AnimTween    = v - 256;
                    }
                    s.anim_tween_applied = true;
                    blend_apply = true;
                }
            }

            // Vertex-morph: AnimTween + the keyframe pointers it indexes.
            // Two cases:
            //   (a) FrameIndex stable across tick — same morph endpoints,
            //       straight lerp on AnimTween.
            //   (b) FrameIndex advanced by exactly 1 (or wrapped via
            //       LAST_FRAME) — span the keyframe boundary using a virtual
            //       extended coordinate, switching keyframe pointers when
            //       v crosses 256.
            // Cases >1 keyframes per tick: no intermediate pointer history,
            // skip lerp (rare — needs very fast TweenStep).
            bool apply_anim = false;
            SLONG new_anim_tween = 0;
            struct GameKeyFrame* new_current = nullptr;
            struct GameKeyFrame* new_next = nullptr;
            UBYTE frame_diff = (UBYTE)(s.frame_index_curr - s.frame_index_prev);
            // Loop wrap: anim with LAST_FRAME-flagged terminal keyframe resets
            // FrameIndex to 0 instead of incrementing. UBYTE diff is then
            // (0 - N) wrapping to 256-N, not 1. Detect via the prev-tick's
            // CurrentFrame->Flags — that frame *was* the last-frame terminal,
            // so a single keyframe transition still happened semantically.
            // The new CurrentFrame is the loop's start frame and prev's
            // NextFrame should point to the same keyframe (loop continuity),
            // so the v=256 boundary lines up visually.
            bool is_loop_wrap = (frame_diff != 0 && frame_diff != 1
                                 && s.current_frame_prev
                                 && (s.current_frame_prev->Flags & ANIM_FLAG_LAST_FRAME));
            if (frame_diff == 0) {
                new_anim_tween = lerp_i32(s.anim_tween_prev, s.anim_tween_curr, alpha);
                new_current = s.current_frame_curr;
                new_next = s.next_frame_curr;
                apply_anim = (new_current != nullptr && new_next != nullptr);
            } else if ((frame_diff == 1 || is_loop_wrap)
                       && s.current_frame_prev && s.next_frame_prev
                       && s.current_frame_curr && s.next_frame_curr) {
                // Total fractional advancement from prev to curr (unitless,
                // 256 = one full keyframe). Always > 0 because frame
                // advanced.
                SLONG total = (256 - s.anim_tween_prev) + s.anim_tween_curr;
                SLONG v = s.anim_tween_prev + SLONG(float(total) * alpha);
                if (v < 256) {
                    new_current = s.current_frame_prev;
                    new_next = s.next_frame_prev;
                    new_anim_tween = v;
                } else {
                    new_current = s.current_frame_curr;
                    new_next = s.next_frame_curr;
                    new_anim_tween = v - 256;
                }
                apply_anim = true;
            }

            if (!blend_apply) {
                if (apply_anim) {
                    s.saved_anim_tween = dt->AnimTween;
                    s.saved_current_frame = dt->CurrentFrame;
                    s.saved_next_frame = dt->NextFrame;
                    dt->AnimTween = new_anim_tween;
                    dt->CurrentFrame = new_current;
                    dt->NextFrame = new_next;
                    s.anim_tween_applied = true;
                } else {
                    s.anim_tween_applied = false;
                }
            }
        } else {
            // No angle apply; but mark applied so dtor restores position.
            s.has_angles = false;  // local guard for restore path
            s.anim_tween_applied = false;
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

        fc->x     = lerp_i32(s.x_prev, s.x_curr, alpha);
        fc->y     = lerp_i32(s.y_prev, s.y_curr, alpha);
        fc->z     = lerp_i32(s.z_prev, s.z_curr, alpha);
        fc->yaw   = lerp_angle_cam(s.yaw_prev,   s.yaw_curr,   alpha);
        fc->pitch = lerp_angle_cam(s.pitch_prev, s.pitch_curr, alpha);
        fc->roll  = lerp_angle_cam(s.roll_prev,  s.roll_curr,  alpha);

        s.applied = true;
    }
}

RenderInterpFrame::~RenderInterpFrame()
{
    // Restore in reverse order (camera then Things) just to mirror ctor —
    // ordering doesn't actually matter since the saves are independent.
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
        if (s.has_angles && p->Draw.Tweened) {
            DrawTween* dt = p->Draw.Tweened;
            dt->Angle = s.saved_angle;
            dt->Tilt  = s.saved_tilt;
            dt->Roll  = s.saved_roll;
            if (s.anim_tween_applied) {
                dt->AnimTween = s.saved_anim_tween;
                dt->CurrentFrame = s.saved_current_frame;
                dt->NextFrame = s.saved_next_frame;
                s.anim_tween_applied = false;
            }
        }
        s.applied = false;
    }
}
