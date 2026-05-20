#ifndef ENGINE_GRAPHICS_GEOMETRY_BIND_PALETTE_H
#define ENGINE_GRAPHICS_GEOMETRY_BIND_PALETTE_H

// Static A-pose bind palette for the 15-bone person rig.
// Per-(anim-type) cache, computed once at first use.
//
// Consumed by the consolidated-skin world-space draw path
// (skeletal_skinning_phase2_plan.md, step P2-C) for two purposes:
//
//   1. Mesh-build: convert authored bone-local vertex positions into a
//      shared bind-space:    vert_bind = bind_world[bMatIndex] * vert_local
//
//   2. Per frame, per character: build skin matrices
//                                skin[i] = current[i] * bind_inv[i]
//      and let the shader compute  world_pos = skin[bone] * vert_bind.
//
// Bind pose: PELVIS at origin with identity world rotation; every bone gets
// an identity parent-local rotation except LEFT_HUMORUS (5) and RIGHT_HUMORUS
// (8) which get +/-30 degrees about Z to spread the arms into the A-pose.
// Bone parent-local offsets are derived from chunk->AnimKeyFrames[0] of the
// matching ANIM_TYPE_* slot (the rest keyframe), in the same way
// capture_pose_hierarchical (render_interp.cpp) extracts parent-local data
// from a composed pose.
//
// Only valid for the 15-bone person rig (chunk->ElementCount == 15). The
// accessor returns false for non-human animation chunks (animals, bats,
// Bane, Balrog, ...) — those keep the legacy single-bone rigid path.

struct GEMatrix;

// Look up the cached bind palette for one anim_type
// (ANIM_TYPE_DARCI / ROPER / ROPER2 / CIV). On first hit per anim_type the
// palette is built from game_chunk[anim_type].AnimKeyFrames[0] and cached.
//
// Returns false (and leaves out params untouched) when anim_type is out of
// range, the chunk isn't loaded, or the chunk isn't a 15-bone person rig.
// On success: *out_world and *out_inv_bind point at arrays of 15 GEMatrix
// owned by the cache — read-only, valid until bind_palette_invalidate_all().
bool bind_palette_get(int anim_type,
                      const GEMatrix** out_world,
                      const GEMatrix** out_inv_bind);

// Drop the entire cache. Call on level transition if game_chunk[] gets
// reloaded with different rig data.
void bind_palette_invalidate_all(void);


// Soft rig per-joint blend parameters. Five (parent, leaf) skin pairs
// (TORSO↔HEAD, L_RADIUS↔L_HAND, R_RADIUS↔R_HAND, L_TIBIA↔L_FOOT,
// R_TIBIA↔R_FOOT) collapse into 3 GROUPS — left/right mirrors share:
//   group 0: HEAD   (1 pair)
//   group 1: HANDS  (L + R, 2 pairs share params)
//   group 2: FEET   (L + R, 2 pairs share params)
//
// Each group exposes 4 constants:
//   parent_band / parent_wmax  — pull on the PARENT body part's verts
//                                (TORSO, RADIUS, TIBIA) toward the leaf
//                                joint. parent_band is in absolute world
//                                units; parent_wmax is a 0..1 fraction.
//   child_band  / child_wmax   — pull on the LEAF body part's verts
//                                (HEAD, HAND, FOOT) toward the same
//                                leaf joint, blending TOWARD parent.
//
// Values are eyeballed once and live in bind_palette.cpp — only
// HEAD parent is non-zero (band=20, w_max=1.0); HANDS and FEET stay
// zero (collapses to hard binding) because no universal tune works
// across all character meshes (see known_issues_and_bugs.md). Removed
// at P2-J: when soft becomes the only path, these get inlined as
// constants in figure_build_consolidated_skin_world.
struct SkinTuneGroup {
    float parent_band;
    float parent_wmax;
    float child_band;
    float child_wmax;
};
constexpr int SKIN_TUNE_GROUP_COUNT = 3;
constexpr int SKIN_TUNE_GROUP_HEAD  = 0;
constexpr int SKIN_TUNE_GROUP_HANDS = 1;
constexpr int SKIN_TUNE_GROUP_FEET  = 2;

extern SkinTuneGroup g_skin_tune_groups[SKIN_TUNE_GROUP_COUNT];

// Debug overlay (kept past P2-J — useful diagnostic for all future
// skinning work, not specifically tied to P2-E tuning).
//
// When true, FIGURE_draw paths render the animated skeleton on top of
// the model: bone lines parent→child plus a per-bone wireframe ball at
// each joint. Per-bone colours pair symmetric bones (left/right same
// hue, left bright vs right darker) so the rig is readable at a glance.
//
// When false, the entire debug-draw block is skipped — zero per-frame
// cost. Toggled by B in bangunsnotgames mode (see game_tick.cpp).
extern bool g_skin_debug_draw_skeleton;

#endif // ENGINE_GRAPHICS_GEOMETRY_BIND_PALETTE_H
