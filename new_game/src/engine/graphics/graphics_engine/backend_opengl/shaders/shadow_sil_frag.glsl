#version 410 core

// Character shadow-silhouette fragment shader — Milestone 1E.
//
// The shadow page blends greyscale-density (ge_supports_dest_inv_src_color
// path in poly_render.cpp): result = dest * (1 - srcColor), so a brighter
// texel = darker shadow; black = no shadow. The FBO is cleared to 0
// (no shadow) and covered fragments write a constant density.
//
// The original SMAP rasteriser stored coverage>>1 (GetShadowPixelMapping),
// i.e. a fully-covered interior texel was ~127/255, never full black.
// We emit that same max interior density so the shadow darkness matches
// the original ~1:1. Edge anti-aliasing of the software rasteriser is
// intentionally NOT reproduced (plan 1E: "похоже", not byte-exact; the
// blob is projected onto the ground and distance-faded anyway).

// = 127/255: original max shadow density (coverage 255 >> 1 = 127).
const float SHADOW_DENSITY = 0.498039216;

out vec4 o_color;

void main()
{
    o_color = vec4(SHADOW_DENSITY, SHADOW_DENSITY, SHADOW_DENSITY, 1.0);
}
