#version 410 core

// Shared fragment shader for both TL and Lit vertices.
// Handles: texture sampling, texture blend modes, alpha test, color key,
// fog (from specular alpha), and specular highlight addition.

in vec4 v_color;
in vec4 v_specular;
in vec2 v_texcoord;

uniform int       u_has_texture;
uniform sampler2D u_texture;
uniform int       u_texture_blend;       // 0=Modulate, 1=ModulateAlpha, 2=Decal
uniform int       u_alpha_test_enabled;
uniform float     u_alpha_ref;           // normalized 0-1
uniform int       u_alpha_func;          // GECompareFunc as int
uniform int       u_fog_enabled;
uniform vec3      u_fog_color;
uniform int       u_specular_enabled;
uniform int       u_color_key_enabled;

out vec4 frag_color;

void main()
{
    vec4 color = v_color;

    if (u_has_texture != 0) {
        vec4 tex = texture(u_texture, v_texcoord);

        if (u_texture_blend == 0) {
            // Modulate: vertex RGB * texture RGB, alpha from vertex.
            color.rgb = color.rgb * tex.rgb;
        } else if (u_texture_blend == 1) {
            // ModulateAlpha: vertex * texture (all channels).
            color = color * tex;
        } else {
            // Decal: texture only, ignore vertex color.
            color = tex;
        }
    }

    // Color key: discard fully transparent pixels.
    if (u_color_key_enabled != 0 && color.a < 0.004)
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

    // Fog: linear blend using specular alpha as fog factor.
    // D3D6 convention: specular.a = 1.0 → no fog, 0.0 → fully fogged.
    if (u_fog_enabled != 0) {
        float fog_factor = v_specular.a;
        color.rgb = mix(u_fog_color, color.rgb, fog_factor);
    }

    frag_color = color;
}
