#ifndef ENGINE_GRAPHICS_GRAPHICS_ENGINE_H
#define ENGINE_GRAPHICS_GRAPHICS_ENGINE_H

// Graphics engine abstraction layer.
// Game code includes ONLY this header — never D3D/DDraw/OpenGL headers directly.
// Implementation lives in graphics_engine_d3d.cpp (or graphics_engine_opengl.cpp).

#include <cstdint>

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

// Opaque handle to a GPU texture. 0 = no texture.
using GETextureHandle = uint32_t;
constexpr GETextureHandle GE_TEXTURE_NONE = 0;

enum class GEBlendMode {
    Opaque,         // no blending
    Alpha,          // src*srcA + dst*(1-srcA)
    Additive,       // src*1 + dst*1
    Modulate,       // src*dst
    InvModulate,    // src*(1-dst)
};

// Individual blend factors for custom src/dst combinations.
enum class GEBlendFactor {
    Zero,
    One,
    SrcAlpha,
    InvSrcAlpha,
    SrcColor,
    InvSrcColor,
    DstColor,
    InvDstColor,
    DstAlpha,
    InvDstAlpha,
};

enum class GEDepthMode {
    Off,            // no depth test, no depth write
    ReadOnly,       // depth test on, depth write off
    WriteOnly,      // depth test off, depth write on
    ReadWrite,      // depth test on, depth write on (default)
};

enum class GECullMode {
    None,
    CW,
    CCW,            // default
};

enum class GETextureFilter {
    Nearest,
    Linear,
};

enum class GETextureBlend {
    Modulate,       // vertex color * texture
    ModulateAlpha,  // vertex color * texture, with alpha
    Decal,          // texture only, ignore vertex color
};

enum class GETextureAddress {
    Wrap,
    Clamp,
};

enum class GECompareFunc {
    Always,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    NotEqual,
};

enum class GEPrimitiveType {
    TriangleList,
    TriangleFan,
};

// Pre-transformed, pre-lit vertex (screen space). Replaces D3DTLVERTEX.
// Union aliases (dvSX, dcColor, etc.) provided for compatibility with legacy code.
struct GEVertexTL {
    union { float x;  float sx;  float dvSX; };
    union { float y;  float sy;  float dvSY; };
    union { float z;  float sz;  float dvSZ; };
    union { float rhw; float dvRHW; };
    union { uint32_t color; uint32_t dcColor; };
    union { uint32_t specular; uint32_t dcSpecular; };
    union { float u;  float tu;  float dvTU; };
    union { float v;  float tv;  float dvTV; };
};

// Lit vertex (world space, pre-lit). Replaces D3DLVERTEX.
// Union aliases (dvX, dcColor, etc.) provided for compatibility with legacy code.
struct GEVertexLit {
    union { float x;  float dvX; };
    union { float y;  float dvY; };
    union { float z;  float dvZ; };
    union { uint32_t _reserved; uint32_t dwReserved; }; // padding to match D3DLVERTEX layout
    union { uint32_t color; uint32_t dcColor; };
    union { uint32_t specular; uint32_t dcSpecular; };
    union { float u;  float tu;  float dvTU; };
    union { float v;  float tv;  float dvTV; };
};

// Unlit vertex (world space). Replaces D3DVERTEX.
// Union aliases (dvX, dvNX, etc.) provided for compatibility with legacy code.
struct GEVertex {
    union { float x;  float dvX; };
    union { float y;  float dvY; };
    union { float z;  float dvZ; };
    union { float nx; float dvNX; };
    union { float ny; float dvNY; };
    union { float nz; float dvNZ; };
    union { float u;  float dvTU; };
    union { float v;  float dvTV; };
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Called once at startup after window creation.
void ge_init();

// Called once at shutdown.
void ge_shutdown();

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void ge_begin_scene();
void ge_end_scene();
void ge_clear(bool color, bool depth);
void ge_flip();

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

void ge_set_blend_mode(GEBlendMode mode);
void ge_set_blend_factors(GEBlendFactor src, GEBlendFactor dst);
void ge_set_blend_enabled(bool enabled);
void ge_set_depth_mode(GEDepthMode mode);
void ge_set_depth_func(GECompareFunc func);
void ge_set_cull_mode(GECullMode mode);
void ge_set_texture_filter(GETextureFilter mag, GETextureFilter min);
void ge_set_texture_blend(GETextureBlend mode);
void ge_set_texture_address(GETextureAddress mode);
void ge_set_depth_bias(int32_t bias);
void ge_set_fog_enabled(bool enabled);
void ge_set_fog_params(uint32_t color, float near_dist, float far_dist);
void ge_set_specular_enabled(bool enabled);
void ge_set_perspective_correction(bool enabled);

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

void ge_bind_texture(GETextureHandle tex);

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void ge_draw_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t count);
void ge_draw_indexed_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t vert_count,
                               const uint16_t* indices, uint32_t index_count);

// Overloads for lit vertices (world-space, pre-lit).
void ge_draw_indexed_primitive_lit(GEPrimitiveType type, const GEVertexLit* verts, uint32_t vert_count,
                                   const uint16_t* indices, uint32_t index_count);

// Overloads for unlit vertices (world-space, requires transformation).
void ge_draw_indexed_primitive_unlit(GEPrimitiveType type, const GEVertex* verts, uint32_t vert_count,
                                     const uint16_t* indices, uint32_t index_count);

// ---------------------------------------------------------------------------
// Transforms
// ---------------------------------------------------------------------------

// 4x4 matrix in row-major order (same layout as D3DMATRIX).
// Named member aliases (_11.._44) provided for compatibility with legacy code.
struct GEMatrix {
    union {
        float m[4][4];
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
    };
};

enum class GETransform {
    World,
    View,
    Projection,
};

void ge_set_transform(GETransform type, const GEMatrix* matrix);

// ---------------------------------------------------------------------------
// Viewport
// ---------------------------------------------------------------------------

void ge_set_viewport(int32_t x, int32_t y, int32_t w, int32_t h);

// ---------------------------------------------------------------------------
// Background
// ---------------------------------------------------------------------------

enum class GEBackground { Black, White, User };
void ge_set_background(GEBackground bg);
void ge_set_background_color(uint8_t r, uint8_t g, uint8_t b);

#endif // ENGINE_GRAPHICS_GRAPHICS_ENGINE_H
