#pragma once

// Aspect-ratio clamps for the 3D scene FBO. Engine-internal hard limits —
// not user-tuneable via config because the rectilinear projection breaks
// outside this range:
//
//   - Wider than the cap: the rectilinear projection turns into visible
//     fish-eye at the edges (corners stretch radially as horizontal FOV
//     opens up). Above the cap, composition pillarboxes the scene at
//     the cap aspect to keep the projection well-behaved.
//
//   - Narrower than the floor: with isotropic pixels the horizontal FOV
//     shrinks aggressively (atan(W/H) ≈ 14° at 1:4 vs 53° at 4:3) which
//     reads as extreme zoom — character looks huge, the world barely
//     fits sideways. Below the floor, composition letterboxes at the
//     floor aspect.
//
// Defaults preserve full 4:3 / 16:10 / 16:9 behaviour exactly: the cap
// only triggers at 21:9+ ultra-wide, the floor only at portrait / square.
//
// Used by:
//   - graphics_engine/backend_opengl/game/core.cpp (FBO sizing in
//     compute_fbo_size)
//   - graphics/pipeline/poly.cpp + aeng.cpp (auto-zoom-out / wideify
//     letterbox math when the FBO sits at the clamped aspect)
//
// Pre-rename note: these were `OC_FOV_CAP_ASPECT` / `OC_FOV_MIN_ASPECT`
// in config.h. Moved out + dropped `OC_` prefix because tuning them via
// config is not a supported workflow.

#define FOV_CAP_ASPECT (18.0F / 9.0F)
#define FOV_MIN_ASPECT (2.0F / 3.0F)
