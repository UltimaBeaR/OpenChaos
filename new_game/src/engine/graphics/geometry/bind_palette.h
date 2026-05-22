#ifndef ENGINE_GRAPHICS_GEOMETRY_BIND_PALETTE_H
#define ENGINE_GRAPHICS_GEOMETRY_BIND_PALETTE_H

// Static bind palette for any skeletally-animated rig — both the 15-bone
// person rig and flat-skeleton creatures (Bane, Balrog, bats, Gargoyle).
// Per-(chunk) cache, computed once at first use.
//
// Two rig flavours:
//
//  - 15-bone person rig (DARCI / ROPER / ROPER2 / CIV).
//    Hierarchical FK in an A-pose: PELVIS at origin with identity world
//    rotation; every bone gets an identity parent-local rotation except
//    LEFT_HUMORUS (5) and RIGHT_HUMORUS (8) which get +/-30 degrees about
//    Z to spread the arms.
//
//  - Flat skeleton (any chunk with ElementCount != 15).
//    No parent/child relationship between bones; each bone independently
//    sits at its keyframe-0 offset with identity world rotation. No FK
//    chain. bind_world[i] = (identity rot, translation = float-inch
//    offset from keyframe 0).
//
// Consumed by the consolidated-skin world-space draw path for two purposes:
//
//   1. Mesh-build: convert authored bone-local vertex positions into a
//      shared bind-space:    vert_bind = bind_world[bMatIndex] * vert_local
//
//   2. Per frame, per character: build skin matrices
//                                skin[i] = current[i] * inv_bind[i]
//      and let the shader compute  world_pos = skin[bone] * vert_bind.
//
// The auto-rig (soft weights at leaf joints) is a 15-bone-person-only
// concept and lives in figure_build_consolidated_skin_world; it is not
// the bind palette's responsibility. Non-people get trivial (single-bone,
// w0=255) weights — same shader, same draw path, no anatomy assumptions.

#include "engine/graphics/geometry/pose_composer.h" // POSE_MAX_BONES (storage upper bound)

struct GEMatrix;
struct GameKeyFrameChunk;

// Look up the cached bind palette for one animation chunk. On first hit
// per chunk the palette is built from `chunk->AnimKeyFrames[<first valid
// keyframe>]` and cached.
//
// Returns false (and leaves out params untouched) when chunk is NULL,
// has no AnimKeyFrames, has no keyframe with element data, or has an
// element count outside [1, POSE_MAX_BONES].
//
// On success: *out_world and *out_inv_bind point at arrays of
// *out_bone_count GEMatrix owned by the cache — read-only, valid until
// bind_palette_invalidate_all(). Out params may individually be NULL if
// the caller only needs a subset.
bool bind_palette_get(const GameKeyFrameChunk* chunk,
                      const GEMatrix** out_world,
                      const GEMatrix** out_inv_bind,
                      int*             out_bone_count);

// Drop the entire cache. Call on level transition if animation chunks
// get reloaded with different rig data.
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
