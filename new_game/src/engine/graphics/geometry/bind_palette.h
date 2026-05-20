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

// Debug A/B toggle (P2-C.5): when true, the consolidated-skin draw path
// in figure.cpp uses the world-skin shader (skin_world_vert.glsl) with
// bind-space verts + per-frame  skin = current * inv_bind  uniforms.
// When false, the existing baked-palette path (skin_vert.glsl) is used.
// Only effective for 15-bone person rigs with a valid bind palette;
// other rigs ignore the toggle and stay on the legacy path.
//
// Toggled by a debug key (see game_tick.cpp). Default: false (baseline
// path) so a build with this disabled is identical to the P2-A artifact.
extern bool g_skin_world_path_enabled;

// Debug A/B toggle (P2-E): when true, figure_build_consolidated_skin_world
// emits soft per-vertex weights via the auto-rig (parent-bone falloff
// near joints, see skeletal_skinning_phase2_plan.md §4). When false,
// trivial single-bone weights (w0=1, rest=0) are emitted — math
// collapses to hard rigid skin.
//
// Has effect only when g_skin_world_path_enabled is also true (the world
// path is what reads the multi-bone palette). Toggling this changes how
// the bind-space VBO is BUILT, so caches must be invalidated — see
// FIGURE_invalidate_all_skin_consolidated_world() in figure.h.
//
// Default: true (NEW soft path is the development default until P2-J
// removes the toggle and the old path entirely).
extern bool g_skin_soft_rig_enabled;

// TEMP — live-tunable soft rig parameters (P2-E).
//
// Both are unitless fractions in [0, 1]:
//   - g_skin_soft_band_fraction: width of the parent→child blending band
//     as a fraction of the child bone's max vertex distance from its
//     joint (bone_length). 0.4 = narrow seam, 0.8 = broad blend.
//   - g_skin_soft_w_max: maximum parent contribution at the joint itself
//     (dist=0). 0.5 = 50/50, 0.7 = parent dominates near the joint.
//
// Changing either invalidates the bind-space VBO cache (weights are
// baked at build time). See FIGURE_invalidate_all_skin_consolidated_world.
// Removed together with the rest of the toggle infrastructure at P2-J.
extern float g_skin_soft_band_fraction;
extern float g_skin_soft_w_max;

#endif // ENGINE_GRAPHICS_GEOMETRY_BIND_PALETTE_H
