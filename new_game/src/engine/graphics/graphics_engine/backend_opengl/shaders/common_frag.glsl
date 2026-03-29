#version 410 core

// Shared fragment shader for both TL and Lit vertices.
// Handles: texture sampling, texture blend modes, alpha test, color key,
// fog (from specular alpha), and specular highlight addition.

in vec4 v_color;
in vec4 v_specular;
in vec2 v_texcoord;
in float v_view_z;

uniform int       u_has_texture;
uniform sampler2D u_texture;
uniform int       u_texture_blend;       // 0=Modulate, 1=ModulateAlpha, 2=Decal
uniform int       u_alpha_test_enabled;
uniform float     u_alpha_ref;           // normalized 0-1
uniform int       u_alpha_func;          // GECompareFunc as int
uniform int       u_fog_enabled;
uniform vec3      u_fog_color;
uniform float     u_fog_near;
uniform float     u_fog_far;
uniform int       u_specular_enabled;
uniform int       u_color_key_enabled;
uniform int       u_tex_has_alpha;

out vec4 frag_color;

void main()
{
    vec4 color = v_color;

    if (u_has_texture != 0) {
        vec4 tex = texture(u_texture, v_texcoord);

        // D3D6 hardware returns alpha=1.0 when sampling textures without an
        // alpha channel.  Clump 5:6:5 textures arrive with alpha=0 in the data;
        // this substitution replicates D3D6 sampling behavior in the shader.
        float tex_alpha = (u_tex_has_alpha != 0) ? tex.a : 1.0;

        if (u_texture_blend == 0) {
            // D3DTBLEND_MODULATE (from D3D6 SDK documentation):
            //   COLOROP  = MODULATE:     color.rgb = diffuse.rgb * texture.rgb
            //   ALPHAOP  = SELECTARG1:   color.a   = texture.a    (if texture has alpha)
            //   ALPHAOP  = SELECTARG2:   color.a   = diffuse.a    (if texture has no alpha)
            color.rgb = color.rgb * tex.rgb;
            color.a = (u_tex_has_alpha != 0) ? tex_alpha : color.a;
        } else if (u_texture_blend == 1) {
            // D3DTBLEND_MODULATEALPHA (from D3D6 SDK documentation):
            //   COLOROP  = MODULATE:     color.rgb = diffuse.rgb * texture.rgb
            //   ALPHAOP  = MODULATE:     color.a   = diffuse.a   * texture.a
            color.rgb = color.rgb * tex.rgb;
            color.a = color.a * tex_alpha;
        } else {
            // Decal: texture only, ignore vertex color.
            color = tex;
        }
    }

    // Color key: discard black pixels.
    // D3D6 equivalent: D3DRENDERSTATE_COLORKEYENABLE — hardware discards pixels
    // matching the surface color key (default 0x0000 = black in 16-bit).
    if (u_color_key_enabled != 0 && color.r < 0.004 && color.g < 0.004 && color.b < 0.004)
        discard;

    // Alpha test.
    if (u_alpha_test_enabled != 0) {
        bool pass = true;
        // GECompareFunc: 0=Always, 1=Less, 2=LessEqual, 3=Greater, 4=GreaterEqual, 5=NotEqual
        if      (u_alpha_func == 1) pass = (color.a < u_alpha_ref);
        else if (u_alpha_func == 2) pass = (color.a <= u_alpha_ref);
        else if (u_alpha_func == 3) pass = (color.a > u_alpha_ref);
        else if (u_alpha_func == 4) pass = (color.a >= u_alpha_ref);
        else if (u_alpha_func == 5) pass = (abs(color.a - u_alpha_ref) > 0.001);
        if (!pass) discard;
    }

    // Specular highlight: add specular RGB to final color.
    if (u_specular_enabled != 0) {
        color.rgb += v_specular.rgb;
    }

    // Fog: two independent sources, matching D3D6 behavior.
    // 1) Vertex fog: POLY_fadeout_point() writes fog factor into specular.a (terrain only).
    // 2) Table fog: D3D6 hardware linear fog by Z-distance (walls, objects, meshes).
    // In D3D6, table fog doesn't affect TL terrain (screen Z=0-1, below fog start=42),
    // so terrain only gets vertex fog.  Walls/objects have specular.a=1.0 (no vertex fog)
    // and get table fog only.  We select: if vertex fog was applied, use it; otherwise
    // use Z-based table fog.
    if (u_fog_enabled != 0) {
        float vert_fog = v_specular.a;
        float fog_factor;
        if (vert_fog < 0.99) {
            // CPU fog was applied to this vertex (terrain) — use it exclusively.
            fog_factor = vert_fog;
        } else {
            // No CPU fog — compute D3D6 linear table fog from eye-space Z.
            fog_factor = clamp((u_fog_far - v_view_z) / (u_fog_far - u_fog_near), 0.0, 1.0);
        }
        color.rgb = mix(u_fog_color, color.rgb, fog_factor);
    }

    frag_color = color;
}
