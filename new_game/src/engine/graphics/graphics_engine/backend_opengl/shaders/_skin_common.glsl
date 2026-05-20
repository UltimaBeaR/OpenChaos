// Shared declarations for all skin-path vertex shaders (body, shadow,
// reflection). Substituted in place of `#include "_skin_common.glsl"` at
// shader-embed time by the CMake embed step — there is no GLSL preprocessor
// involved, just plain text replace.
//
// Keep this file minimal: only data layout (attribute locations, uniform
// palette). Per-shader skin-blend math stays inline in each shader so
// style differences (unrolled-for-Steam-Deck-perf in body vs helper-style
// for readability in shadow/reflect) can stay tuned per use case.

const int MAX_BONES = 32; // = GE_SKIN_MAX_BONES (game_graphics_engine.h)

// Bind-space position is the only required position input. Every skin path
// blends it through the per-frame world skin palette below.
layout(location = 0) in vec3  a_position; // BIND-space

// Multi-bone palette indices + weights (P2-D vertex format). Trivial-weight
// case (w0=1, rest=0) collapses the blend to single-bone. Soft-skinned seam
// vertices (P2-E auto-rig) distribute weight across up to 4 bones.
layout(location = 6) in uvec4 a_bones;
layout(location = 7) in vec4  a_weights;

// Per-bone WORLD skin: 3 vec4 per bone, rotation rows with translation in .w.
//   skin[bone] = current_palette[bone] * inverse(bind_palette[bone])
//   world_pos  = (r0.xyz · bind_pos + r0.w, ..., r2.xyz · bind_pos + r2.w)
// Rotation is pre-divided by 32768 on the CPU (Matrix33 is fixed-point;
// the CPU baker absorbs the divide so the shader stays branch-free). Body,
// shadow and reflection share the SAME per-frame palette — any soft-weight
// deformation on the body shows up identically in its shadow / reflection
// without separate setup.
uniform vec4 u_skin[MAX_BONES * 3];
