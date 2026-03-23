#ifndef ENGINE_GRAPHICS_PIPELINE_POLY_H
#define ENGINE_GRAPHICS_PIPELINE_POLY_H

#include "core/types.h"

// Polygon attribute flags. Stored per-face and passed to the renderer
// to select the draw mode (textured, masked, semi-transparent, etc.).
// uc_orig: POLY_FLAG_GOURAD (fallen/Headers/Game.h)
#define POLY_FLAG_GOURAD     (1 << 0)  // Gouraud (vertex colour) shading.
// uc_orig: POLY_FLAG_TEXTURED (fallen/Headers/Game.h)
#define POLY_FLAG_TEXTURED   (1 << 1)  // Texture-mapped.
// uc_orig: POLY_FLAG_MASKED (fallen/Headers/Game.h)
#define POLY_FLAG_MASKED     (1 << 2)  // Alpha-tested (1-bit mask).
// uc_orig: POLY_FLAG_SEMI_TRANS (fallen/Headers/Game.h)
#define POLY_FLAG_SEMI_TRANS (1 << 3)  // 50% semi-transparent blend.
// uc_orig: POLY_FLAG_ALPHA (fallen/Headers/Game.h)
#define POLY_FLAG_ALPHA      (1 << 4)  // Full alpha-blended.
// uc_orig: POLY_FLAG_TILED (fallen/Headers/Game.h)
#define POLY_FLAG_TILED      (1 << 5)  // Tiled texture (wraps).
// uc_orig: POLY_FLAG_DOUBLESIDED (fallen/Headers/Game.h)
#define POLY_FLAG_DOUBLESIDED (1 << 6) // Rendered from both sides (no backface cull).
// uc_orig: POLY_FLAG_WALKABLE (fallen/Headers/Game.h)
#define POLY_FLAG_WALKABLE   (1 << 7)  // Characters can walk on this face.

// uc_orig: MAX_RADIUS (fallen/Headers/Game.h)
#define MAX_RADIUS (24)

// uc_orig: ftol (fallen/DDEngine/Headers/poly.h)
// Fast float-to-int truncation. Used throughout the rendering pipeline.
static inline int ftol(float f)
{
    return (int)f;
}

// uc_orig: POLY_ZCLIP_PLANE (fallen/DDEngine/Headers/poly.h)
// Points closer than 1/64th of the draw range are not drawn (near-plane clip).
#define POLY_ZCLIP_PLANE (1.0F / 64.0F)

// uc_orig: POLY_init (fallen/DDEngine/Headers/poly.h)
// One-time init for the polygon renderer. Call once at program startup.
void POLY_init(void);

// uc_orig: calc_global_cloud (fallen/DDEngine/Headers/poly.h)
// Computes the cloud/fog colour at a world-space position.
void calc_global_cloud(SLONG x, SLONG y, SLONG z);

// uc_orig: apply_cloud (fallen/DDEngine/Headers/poly.h)
// Applies the cloud/fog colour to a vertex colour. Modulates col in-place.
void apply_cloud(SLONG x, SLONG y, SLONG z, ULONG* col);

// uc_orig: use_global_cloud (fallen/DDEngine/Headers/poly.h)
// Applies the last computed cloud colour to a vertex colour without recomputing position.
void use_global_cloud(ULONG* col);

// uc_orig: POLY_init_texture_flags (fallen/DDEngine/Headers/poly.h)
// Resets all per-page texture flags to zero.
void POLY_init_texture_flags(void);

// uc_orig: POLY_load_texture_flags (fallen/DDEngine/Headers/poly.h)
// Loads per-page texture flags from a file. The offset is added to page numbers in the file.
void POLY_load_texture_flags(CBYTE* fname, SLONG offset = 0);

// uc_orig: POLY_reset_render_states (fallen/DDEngine/Headers/poly.h)
// Resets all Direct3D render states to known defaults.
void POLY_reset_render_states();

// uc_orig: POLY_SPLITSCREEN_NONE (fallen/DDEngine/Headers/poly.h)
#define POLY_SPLITSCREEN_NONE   0
// uc_orig: POLY_SPLITSCREEN_TOP (fallen/DDEngine/Headers/poly.h)
#define POLY_SPLITSCREEN_TOP    1
// uc_orig: POLY_SPLITSCREEN_BOTTOM (fallen/DDEngine/Headers/poly.h)
#define POLY_SPLITSCREEN_BOTTOM 2

// Camera world-space position set by POLY_camera_set().
// uc_orig: POLY_cam_x (fallen/DDEngine/Headers/poly.h)
extern float POLY_cam_x;
// uc_orig: POLY_cam_y (fallen/DDEngine/Headers/poly.h)
extern float POLY_cam_y;
// uc_orig: POLY_cam_z (fallen/DDEngine/Headers/poly.h)
extern float POLY_cam_z;
// uc_orig: POLY_cam_matrix (fallen/DDEngine/Headers/poly.h)
// 3x3 rotation matrix from camera yaw/pitch/roll (row-major, float).
extern float POLY_cam_matrix[9];

// uc_orig: POLY_camera_set (fallen/DDEngine/Headers/poly.h)
// Sets camera position and orientation for the current frame.
// view_dist: max distance a point should be from the camera.
// lens: FOV multiplier — higher = more zoom. Typically ~1.5.
void POLY_camera_set(
    float x,
    float y,
    float z,
    float yaw,
    float pitch,
    float roll,
    float view_dist,
    float lens,
    SLONG splitscreen = POLY_SPLITSCREEN_NONE);

// Screen dimensions set by POLY_camera_set().
// uc_orig: POLY_screen_width (fallen/DDEngine/Headers/poly.h)
extern float POLY_screen_width;
// uc_orig: POLY_screen_height (fallen/DDEngine/Headers/poly.h)
extern float POLY_screen_height;

// uc_orig: POLY_CLIP_LEFT (fallen/DDEngine/Headers/poly.h)
#define POLY_CLIP_LEFT        (1 << 0)
// uc_orig: POLY_CLIP_RIGHT (fallen/DDEngine/Headers/poly.h)
#define POLY_CLIP_RIGHT       (1 << 1)
// uc_orig: POLY_CLIP_TOP (fallen/DDEngine/Headers/poly.h)
#define POLY_CLIP_TOP         (1 << 2)
// uc_orig: POLY_CLIP_BOTTOM (fallen/DDEngine/Headers/poly.h)
#define POLY_CLIP_BOTTOM      (1 << 3)
// uc_orig: POLY_CLIP_TRANSFORMED (fallen/DDEngine/Headers/poly.h)
// Point has been rotated into camera coordinates and projected onto the screen.
#define POLY_CLIP_TRANSFORMED (1 << 4)
// uc_orig: POLY_CLIP_FAR (fallen/DDEngine/Headers/poly.h)
// Point is behind the far plane (not transformed).
#define POLY_CLIP_FAR         (1 << 5)
// uc_orig: POLY_CLIP_NEAR (fallen/DDEngine/Headers/poly.h)
// Point is in front of the near plane (not transformed).
#define POLY_CLIP_NEAR        (1 << 6)
// uc_orig: POLY_CLIP_OFFSCREEN (fallen/DDEngine/Headers/poly.h)
// Any of these bits set means the point is off-screen.
#define POLY_CLIP_OFFSCREEN (POLY_CLIP_LEFT | POLY_CLIP_RIGHT | POLY_CLIP_TOP | POLY_CLIP_BOTTOM | POLY_CLIP_FAR | POLY_CLIP_NEAR)

// uc_orig: POLY_Point (fallen/DDEngine/Headers/poly.h)
// A single transformed vertex: holds both 3D view-space coords (x,y,z)
// and 2D screen coords (X,Y,Z=1/z), plus UV, colour, specular (fog), and clip flags.
typedef struct
{
    float x; // 3D view-space X
    float y; // 3D view-space Y
    float z; // 3D view-space Z

    float X; // 2D screen X
    float Y; // 2D screen Y
    float Z; // 2D screen Z (1/z for perspective)

    UWORD clip;
    UWORD user; // Available for caller use.

    float u;
    float v;
    ULONG colour;   // 0xXXRRGGBB — vertex diffuse colour
    ULONG specular; // 0xXXRRGGBB — upper byte used for fog alpha

    // Returns true if the point has been rotated and projected; screen coords are valid.
    inline bool IsValid() { return ((clip & POLY_CLIP_TRANSFORMED) != 0); }
    // Returns true if the point was rotated but may be in front of the near plane; screen coords may be invalid.
    inline bool MaybeValid() { return ((clip & (POLY_CLIP_TRANSFORMED | POLY_CLIP_NEAR)) != 0); }
    // Returns true if the point is in front of the near plane.
    inline bool NearClip() { return ((clip & POLY_CLIP_NEAR) != 0); }

} POLY_Point;

// uc_orig: POLY_flush_local_rot (fallen/DDEngine/Headers/poly.h)
// No-op on PC; present for PSX compatibility.
inline void POLY_flush_local_rot(void)
{
}

// uc_orig: POLY_transform (fallen/DDEngine/Headers/poly.h)
// Transforms a world-space point into view space and computes screen coords and clip flags.
void POLY_transform(
    float world_x,
    float world_y,
    float world_z,
    POLY_Point* pt,
    bool bUnused = UC_FALSE);

// uc_orig: POLY_transform_c (fallen/DDEngine/Headers/poly.h)
// Same as POLY_transform but applies cloud/fog colour during transform.
void POLY_transform_c(
    float world_x,
    float world_y,
    float world_z,
    POLY_Point* pt,
    bool bUnused = UC_FALSE);

// uc_orig: POLY_set_local_rotation_none (fallen/DDEngine/Headers/poly.h)
// Disables the local rotation matrix (reverts to camera-only transform).
void POLY_set_local_rotation_none(void);

// uc_orig: POLY_transform_abs (fallen/DDEngine/Headers/poly.h)
// Transforms a world-space point without local rotation, only camera matrix.
void POLY_transform_abs(
    float world_x,
    float world_y,
    float world_z,
    POLY_Point* pt);

// uc_orig: POLY_transform_c_saturate_z (fallen/DDEngine/Headers/poly.h)
// Like POLY_transform_c but clamps Z to the near plane instead of discarding the point.
void POLY_transform_c_saturate_z(
    float world_x,
    float world_y,
    float world_z,
    POLY_Point* pt);

// uc_orig: POLY_transform_from_view_space (fallen/DDEngine/Headers/poly.h)
// Given a point already in view space (x,y,z,u,v,colour,specular set), computes
// screen coordinates X,Y,Z and clip flags.
void POLY_transform_from_view_space(POLY_Point* pt);

// uc_orig: POLY_get_screen_pos (fallen/DDEngine/Headers/poly.h)
// Projects a world-space point to screen coordinates. Returns UC_TRUE if the point
// is in front of the near plane (z > 0). Does not enforce the far-plane range.
SLONG POLY_get_screen_pos(
    float world_x,
    float world_y,
    float world_z,
    float* screen_x,
    float* screen_y);

// uc_orig: POLY_perspective (fallen/DDEngine/Headers/poly.h)
// Applies the perspective divide to a view-space POLY_Point, filling in X,Y,Z and clip flags.
// wibble_key: non-zero applies sinusoidal screen-space wobble (water shimmer effect).
void POLY_perspective(POLY_Point* pt, UBYTE wibble_key = 0);

// uc_orig: POLY_setclip (fallen/DDEngine/Headers/poly.h)
// Sets clip flags on a POLY_Point that is already projected to screen space.
void POLY_setclip(POLY_Point* pt);

// uc_orig: POLY_world_length_to_screen (fallen/DDEngine/Headers/poly.h)
// Converts a world-space length to an approximate screen-space pixel width at the given depth.
float POLY_world_length_to_screen(float world_length);

// uc_orig: POLY_approx_len (fallen/DDEngine/Headers/poly.h)
// Returns the approximate length of a 2D screen-space vector using an octagonal approximation.
float POLY_approx_len(float dx, float dy);

// uc_orig: POLY_get_sphere_circle (fallen/DDEngine/Headers/poly.h)
// Projects a world-space sphere to screen circle. Returns UC_FALSE if the sphere is behind the camera.
// screen_radius is approximate (based on screen-space projection of world_radius).
SLONG POLY_get_sphere_circle(
    float world_x,
    float world_y,
    float world_z,
    float world_radius,
    SLONG* screen_x,
    SLONG* screen_y,
    SLONG* screen_radius);

// uc_orig: POLY_set_wibble (fallen/DDEngine/Headers/poly.h)
// Sets the wibble (sinusoidal wobble) parameters used during POLY_perspective.
void POLY_set_wibble(
    UBYTE wibble_y1,
    UBYTE wibble_y2,
    UBYTE wibble_g1,
    UBYTE wibble_g2,
    UBYTE wibble_s1,
    UBYTE wibble_s2);

// uc_orig: POLY_set_local_rotation (fallen/DDEngine/Headers/poly.h)
// Sets an additional object-to-world rotation matrix applied before camera transform.
// off_x/y/z: object origin in world space. matrix[9]: row-major 3x3 rotation.
void POLY_set_local_rotation(
    float off_x,
    float off_y,
    float off_z,
    float matrix[9]);

// uc_orig: POLY_transform_using_local_rotation (fallen/DDEngine/Headers/poly.h)
// Transforms a local-space point through the local rotation + camera matrices.
void POLY_transform_using_local_rotation(
    float local_x,
    float local_y,
    float local_z,
    POLY_Point* pt);

// uc_orig: POLY_transform_using_local_rotation_and_wibble (fallen/DDEngine/Headers/poly.h)
// Like POLY_transform_using_local_rotation but also applies wibble during perspective.
void POLY_transform_using_local_rotation_and_wibble(
    float local_x,
    float local_y,
    float local_z,
    POLY_Point* pt,
    UBYTE wibble_key);

// uc_orig: POLY_sphere_visible (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the world-space sphere is visible within the view frustum.
// radius should be world_radius / view_dist (fraction of view distance, not world units).
SLONG POLY_sphere_visible(
    float world_x,
    float world_y,
    float world_z,
    float radius);

// uc_orig: POLY_page_is_masked_self_illuminating (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the texture page is drawn two-pass with the next page as a mask.
SLONG POLY_page_is_masked_self_illuminating(SLONG page);

// uc_orig: POLY_BUFFER_SIZE (fallen/DDEngine/Headers/poly.h)
#define POLY_BUFFER_SIZE 8192
// uc_orig: POLY_SHADOW_SIZE (fallen/DDEngine/Headers/poly.h)
#define POLY_SHADOW_SIZE 8192

// uc_orig: POLY_buffer (fallen/DDEngine/Headers/poly.h)
// Scratch buffer for vertex transformation — filled per-object, then submitted.
extern POLY_Point POLY_buffer[POLY_BUFFER_SIZE];
// uc_orig: POLY_buffer_upto (fallen/DDEngine/Headers/poly.h)
extern SLONG POLY_buffer_upto;

// uc_orig: POLY_shadow (fallen/DDEngine/Headers/poly.h)
// Scratch buffer for shadow polygon vertices.
extern POLY_Point POLY_shadow[POLY_SHADOW_SIZE];
// uc_orig: POLY_shadow_upto (fallen/DDEngine/Headers/poly.h)
extern SLONG POLY_shadow_upto;

// uc_orig: POLY_colour_restrict (fallen/DDEngine/Headers/poly.h)
// Bitmask ANDed (inverted) with all vertex colours before submission.
// Set to restrict colour channels (e.g. force-dark scenes).
extern ULONG POLY_colour_restrict;

// uc_orig: POLY_force_additive_alpha (fallen/DDEngine/Headers/poly.h)
// When non-zero, forces additive alpha blending for all polygons.
extern ULONG POLY_force_additive_alpha;

// uc_orig: POLY_FADEOUT_START (fallen/DDEngine/Headers/poly.h)
// Distance fraction at which distance-based fog begins to fade in.
#define POLY_FADEOUT_START (0.60F)
// uc_orig: POLY_FADEOUT_END (fallen/DDEngine/Headers/poly.h)
// Distance fraction at which distance-based fog is fully opaque.
#define POLY_FADEOUT_END   (0.95F)

// uc_orig: fade_point_more (fallen/DDEngine/Headers/poly.h)
// Extended fade calculation; used as an alternative to the inline POLY_fadeout_point path.
SLONG fade_point_more(POLY_Point* pp);

// uc_orig: POLY_fadeout_point (fallen/DDEngine/Headers/poly.h)
// Applies distance-based fog to a single vertex. Writes the fog factor into specular alpha.
// Also applies POLY_colour_restrict to both colour and specular channels.
static void inline POLY_fadeout_point(POLY_Point* pp)
{
    {
        SLONG multi = 255 - ftol((pp->z - POLY_FADEOUT_START) * (256.0F / (POLY_FADEOUT_END - POLY_FADEOUT_START)));

        if (multi > 255) {
            multi = 255;
        }

        if (multi < 0) {
            multi = 0;
        }

        pp->specular &= 0x00ffffff;
        pp->specular |= multi << 24;
    }

    pp->colour &= ~POLY_colour_restrict;
    pp->specular &= ~POLY_colour_restrict;
}

// uc_orig: POLY_fadeout_buffer (fallen/DDEngine/Headers/poly.h)
// Applies POLY_fadeout_point to all points in POLY_buffer[0..POLY_buffer_upto].
void POLY_fadeout_buffer(void);

// uc_orig: POLY_NUM_PAGES (fallen/DDEngine/Headers/poly.h)
// Total number of texture page slots: 22 sets * 64 textures + 111 special pages = 1508.
#define POLY_NUM_PAGES (22 * 64 + 111)

// Special texture page indices (relative to POLY_NUM_PAGES).
// uc_orig: POLY_PAGE_FADE_MF (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FADE_MF          (POLY_NUM_PAGES - 110)
// uc_orig: POLY_PAGE_SNOWFLAKE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SNOWFLAKE        (POLY_NUM_PAGES - 109)
// uc_orig: POLY_PAGE_SPLASH (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SPLASH           (POLY_NUM_PAGES - 108)
// uc_orig: POLY_PAGE_METEOR (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_METEOR           (POLY_NUM_PAGES - 107)
// uc_orig: POLY_PAGE_LITE_BOLT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LITE_BOLT        (POLY_NUM_PAGES - 106)
// uc_orig: POLY_PAGE_LADSHAD (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LADSHAD          (POLY_NUM_PAGES - 105)
// uc_orig: POLY_PAGE_LASTPANEL_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL_ALPHA  (POLY_NUM_PAGES - 104)
// uc_orig: POLY_PAGE_LASTPANEL_SUB (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL_SUB    (POLY_NUM_PAGES - 103)
// uc_orig: POLY_PAGE_LASTPANEL2_SUB (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL2_SUB   (POLY_NUM_PAGES - 102)
// uc_orig: POLY_PAGE_LASTPANEL2_ADDALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL2_ADDALPHA (POLY_NUM_PAGES - 101)
// uc_orig: POLY_PAGE_LASTPANEL_ADDALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL_ADDALPHA (POLY_NUM_PAGES - 100)
// uc_orig: POLY_PAGE_LASTPANEL2_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL2_ALPHA (POLY_NUM_PAGES - 99)
// uc_orig: POLY_PAGE_LASTPANEL2_ADD (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL2_ADD   (POLY_NUM_PAGES - 98)
// uc_orig: POLY_PAGE_EXPLODE2_ADDITIVE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_EXPLODE2_ADDITIVE (POLY_NUM_PAGES - 97)
// uc_orig: POLY_PAGE_EXPLODE1_ADDITIVE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_EXPLODE1_ADDITIVE (POLY_NUM_PAGES - 96)
// uc_orig: POLY_PAGE_SHADOW_SQUARE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SHADOW_SQUARE    (POLY_NUM_PAGES - 95)
// uc_orig: POLY_PAGE_PCFLAMER (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_PCFLAMER         (POLY_NUM_PAGES - 94)
// uc_orig: POLY_PAGE_SIGN (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SIGN             (POLY_NUM_PAGES - 93)
// uc_orig: POLY_PAGE_LASTPANEL_ADD (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LASTPANEL_ADD    (POLY_NUM_PAGES - 92)
// uc_orig: POLY_PAGE_LEAF (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LEAF             (POLY_NUM_PAGES - 90)
// uc_orig: POLY_PAGE_RUBBISH (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_RUBBISH          (POLY_NUM_PAGES - 89)
// uc_orig: POLY_PAGE_FADECAT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FADECAT          (POLY_NUM_PAGES - 88)
// uc_orig: POLY_PAGE_LADDER (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LADDER           (POLY_NUM_PAGES - 87)
// uc_orig: POLY_PAGE_SMOKECLOUD2 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SMOKECLOUD2      (POLY_NUM_PAGES - 86)
// uc_orig: POLY_PAGE_SMOKECLOUD (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SMOKECLOUD       (POLY_NUM_PAGES - 85)
// uc_orig: POLY_PAGE_TINY_BUTTONS (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_TINY_BUTTONS     (POLY_NUM_PAGES - 84)
// uc_orig: POLY_PAGE_FINALGLOW (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FINALGLOW        (POLY_NUM_PAGES - 83)
// uc_orig: POLY_PAGE_BIG_LEAF (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BIG_LEAF         (POLY_NUM_PAGES - 82)
// uc_orig: POLY_PAGE_BIG_RAIN (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BIG_RAIN         (POLY_NUM_PAGES - 81)
// uc_orig: POLY_PAGE_BIG_BUTTONS (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BIG_BUTTONS      (POLY_NUM_PAGES - 80)
// uc_orig: POLY_PAGE_COLOUR_WITH_FOG (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_COLOUR_WITH_FOG  (POLY_NUM_PAGES - 79)
// uc_orig: POLY_PAGE_ALPHA_OVERLAY (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_ALPHA_OVERLAY    (POLY_NUM_PAGES - 78)
// uc_orig: POLY_PAGE_TYRESKID (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_TYRESKID         (POLY_NUM_PAGES - 77)
// uc_orig: POLY_PAGE_COLOUR (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_COLOUR           (POLY_NUM_PAGES - 76)
// uc_orig: POLY_PAGE_POLAROID (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_POLAROID         (POLY_NUM_PAGES - 75)
// uc_orig: POLY_PAGE_MENULOGO (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_MENULOGO         (POLY_NUM_PAGES - 74)
// uc_orig: POLY_PAGE_SUBTRACTIVEALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SUBTRACTIVEALPHA (POLY_NUM_PAGES - 73)
// uc_orig: POLY_PAGE_NEWFONT_INVERSE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_NEWFONT_INVERSE  (POLY_NUM_PAGES - 72)
// uc_orig: POLY_PAGE_IC_NORMAL (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC_NORMAL        (POLY_NUM_PAGES - 71)
// uc_orig: POLY_PAGE_IC2_NORMAL (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC2_NORMAL       (POLY_NUM_PAGES - 70)
// uc_orig: POLY_PAGE_IC_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC_ALPHA         (POLY_NUM_PAGES - 69)
// uc_orig: POLY_PAGE_IC2_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC2_ALPHA        (POLY_NUM_PAGES - 68)
// uc_orig: POLY_PAGE_IC_ADDITIVE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC_ADDITIVE      (POLY_NUM_PAGES - 67)
// uc_orig: POLY_PAGE_IC2_ADDITIVE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC2_ADDITIVE     (POLY_NUM_PAGES - 66)
// uc_orig: POLY_PAGE_IC_ALPHA_END (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC_ALPHA_END     (POLY_NUM_PAGES - 65)
// uc_orig: POLY_PAGE_IC2_ALPHA_END (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_IC2_ALPHA_END    (POLY_NUM_PAGES - 64)
// uc_orig: POLY_PAGE_PRESS1 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_PRESS1           (POLY_NUM_PAGES - 63)
// uc_orig: POLY_PAGE_PRESS2 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_PRESS2           (POLY_NUM_PAGES - 62)
// uc_orig: POLY_PAGE_NEWFONT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_NEWFONT          (POLY_NUM_PAGES - 61)
// uc_orig: POLY_PAGE_TARGET (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_TARGET           (POLY_NUM_PAGES - 60)
// uc_orig: POLY_PAGE_SMOKER (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SMOKER           (POLY_NUM_PAGES - 59)
// uc_orig: POLY_PAGE_DEVIL (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_DEVIL            (POLY_NUM_PAGES - 58)
// uc_orig: POLY_PAGE_ANGEL (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_ANGEL            (POLY_NUM_PAGES - 57)
// uc_orig: POLY_PAGE_ADDITIVEALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_ADDITIVEALPHA    (POLY_NUM_PAGES - 56)
// uc_orig: POLY_PAGE_TYRETRACK (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_TYRETRACK        (POLY_NUM_PAGES - 55)
// uc_orig: POLY_PAGE_WINMAP (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_WINMAP           (POLY_NUM_PAGES - 54)
// uc_orig: POLY_PAGE_LENSFLARE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LENSFLARE        (POLY_NUM_PAGES - 53)
// uc_orig: POLY_PAGE_HITSPANG (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_HITSPANG         (POLY_NUM_PAGES - 52)
// uc_orig: POLY_PAGE_BLOOM2 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BLOOM2           (POLY_NUM_PAGES - 51)
// uc_orig: POLY_PAGE_BLOOM1 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BLOOM1           (POLY_NUM_PAGES - 50)
// uc_orig: POLY_PAGE_BLOODSPLAT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BLOODSPLAT       (POLY_NUM_PAGES - 49)
// uc_orig: POLY_PAGE_FLAMES3 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAMES3          (POLY_NUM_PAGES - 48)
// uc_orig: POLY_PAGE_DUSTWAVE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_DUSTWAVE         (POLY_NUM_PAGES - 47)
// uc_orig: POLY_PAGE_BIGBANG (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BIGBANG          (POLY_NUM_PAGES - 46)
// uc_orig: POLY_PAGE_FACE1 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FACE1            (POLY_NUM_PAGES - 45)
// uc_orig: POLY_PAGE_FACE2 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FACE2            (POLY_NUM_PAGES - 44)
// uc_orig: POLY_PAGE_FACE3 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FACE3            (POLY_NUM_PAGES - 43)
// uc_orig: POLY_PAGE_FACE4 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FACE4            (POLY_NUM_PAGES - 42)
// uc_orig: POLY_PAGE_FACE5 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FACE5            (POLY_NUM_PAGES - 41)
// uc_orig: POLY_PAGE_FACE6 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FACE6            (POLY_NUM_PAGES - 40)
// uc_orig: POLY_PAGE_FONT2D (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FONT2D           (POLY_NUM_PAGES - 39)
// uc_orig: POLY_PAGE_FOOTPRINT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FOOTPRINT        (POLY_NUM_PAGES - 38)
// uc_orig: POLY_PAGE_BARBWIRE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BARBWIRE         (POLY_NUM_PAGES - 37)
// uc_orig: POLY_PAGE_MENUPASS (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_MENUPASS         (POLY_NUM_PAGES - 36)
// uc_orig: POLY_PAGE_MENUTEXT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_MENUTEXT         (POLY_NUM_PAGES - 35)
// uc_orig: POLY_PAGE_MENUFLAME (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_MENUFLAME        (POLY_NUM_PAGES - 34)
// uc_orig: POLY_PAGE_FLAMES2 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAMES2          (POLY_NUM_PAGES - 33)
// uc_orig: POLY_PAGE_SMOKE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SMOKE            (POLY_NUM_PAGES - 32)
// uc_orig: POLY_PAGE_FLAMES (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAMES           (POLY_NUM_PAGES - 31)
// uc_orig: POLY_PAGE_SEWATER (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SEWATER          (POLY_NUM_PAGES - 28)
// uc_orig: POLY_PAGE_SKY (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SKY              (POLY_NUM_PAGES - 27)
// uc_orig: POLY_PAGE_SHADOW (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SHADOW           (POLY_NUM_PAGES - 26)
// uc_orig: POLY_PAGE_SHADOW_OVAL (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SHADOW_OVAL      (POLY_NUM_PAGES - 25)
// uc_orig: POLY_PAGE_PUDDLE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_PUDDLE           (POLY_NUM_PAGES - 24)
// uc_orig: POLY_PAGE_CLOUDS (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_CLOUDS           (POLY_NUM_PAGES - 23)
// uc_orig: POLY_PAGE_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_ALPHA            (POLY_NUM_PAGES - 22)
// uc_orig: POLY_PAGE_ADDITIVE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_ADDITIVE         (POLY_NUM_PAGES - 21)
// uc_orig: POLY_PAGE_MOON (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_MOON             (POLY_NUM_PAGES - 20)
// uc_orig: POLY_PAGE_MANONMOON (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_MANONMOON        (POLY_NUM_PAGES - 19)
// uc_orig: POLY_PAGE_MASKED (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_MASKED           (POLY_NUM_PAGES - 18)
// uc_orig: POLY_PAGE_ENVMAP (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_ENVMAP           (POLY_NUM_PAGES - 17)
// uc_orig: POLY_PAGE_WATER (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_WATER            (POLY_NUM_PAGES - 16)
// uc_orig: POLY_PAGE_DRIP (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_DRIP             (POLY_NUM_PAGES - 15)
// uc_orig: POLY_PAGE_FOG (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FOG              (POLY_NUM_PAGES - 14)
// uc_orig: POLY_PAGE_STEAM (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_STEAM            (POLY_NUM_PAGES - 13)
// uc_orig: POLY_PAGE_BANG (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_BANG             (POLY_NUM_PAGES - 11)
// uc_orig: POLY_PAGE_TEXT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_TEXT             (POLY_NUM_PAGES - 10)
// uc_orig: POLY_PAGE_LOGO (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_LOGO             (POLY_NUM_PAGES - 9)
// uc_orig: POLY_PAGE_DROPLET (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_DROPLET          (POLY_NUM_PAGES - 8)
// uc_orig: POLY_PAGE_RAINDROP (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_RAINDROP         (POLY_NUM_PAGES - 7)
// uc_orig: POLY_PAGE_SPARKLE (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_SPARKLE          (POLY_NUM_PAGES - 6)
// uc_orig: POLY_PAGE_EXPLODE2 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_EXPLODE2         (POLY_NUM_PAGES - 5)
// uc_orig: POLY_PAGE_EXPLODE1 (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_EXPLODE1         (POLY_NUM_PAGES - 4)
// uc_orig: POLY_PAGE_COLOUR_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_COLOUR_ALPHA     (POLY_NUM_PAGES - 1)
// uc_orig: POLY_PAGE_TEST_SHADOWMAP (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_TEST_SHADOWMAP   (POLY_NUM_PAGES - 2)

// uc_orig: POLY_frame_init (fallen/DDEngine/Headers/poly.h)
// Clears polygon buffers for a new frame. keep_shadow_page/keep_text_page = UC_TRUE to preserve
// previously submitted shadow/text polygons across frames.
void POLY_frame_init(SLONG keep_shadow_page, SLONG keep_text_page);

// uc_orig: POLY_valid_triangle (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the triangle passes backface culling and all vertices are on-screen.
SLONG POLY_valid_triangle(POLY_Point* p[3]);

// uc_orig: POLY_valid_quad (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the quad passes backface culling and all vertices are on-screen.
SLONG POLY_valid_quad(POLY_Point* p[4]);

// uc_orig: POLY_valid_line (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the line endpoints are not both off-screen in the same direction.
SLONG POLY_valid_line(POLY_Point* p1, POLY_Point* p2);

// uc_orig: POLY_add_poly (fallen/DDEngine/Headers/poly.h)
// Submits a convex polygon with poly_points vertices to a texture page bucket.
void POLY_add_poly(POLY_Point** poly, SLONG poly_points, SLONG page);

// uc_orig: POLY_add_triangle (fallen/DDEngine/Headers/poly.h)
// Submits a triangle to a texture page bucket, with optional backface cull and clip-flag generation.
void POLY_add_triangle(POLY_Point* p[3], SLONG page, SLONG shall_i_backface_cull, SLONG generate_clip_flags = UC_FALSE);

// uc_orig: POLY_add_quad (fallen/DDEngine/Headers/poly.h)
// Submits a quad to a texture page bucket, with optional backface cull and clip-flag generation.
void POLY_add_quad(POLY_Point* p[4], SLONG page, SLONG shall_i_backface_cull, SLONG generate_clip_flags = UC_FALSE);

// uc_orig: POLY_add_quad_split2 (fallen/DDEngine/Headers/poly.h)
// Submits a quad split into two triangles to handle non-planar quads correctly.
void POLY_add_quad_split2(POLY_Point* pp[4], SLONG page, SLONG backface_cull);

// uc_orig: POLY_create_cylinder_points (fallen/DDEngine/Headers/poly.h)
// Fills four POLY_Points forming a screen-space rectangle around a 3D line segment
// with given widths, for rendering as a billboard cylinder.
void POLY_create_cylinder_points(POLY_Point* p1, POLY_Point* p2, float width, POLY_Point* pout);

// uc_orig: POLY_add_line (fallen/DDEngine/Headers/poly.h)
// Submits a world-space line as a billboard quad to a texture page bucket.
// width1/width2 in world units. sort_to_front draws after everything else.
void POLY_add_line(POLY_Point* p1, POLY_Point* p2, float width1, float width2, SLONG page, UBYTE sort_to_front);

// uc_orig: POLY_add_line_tex (fallen/DDEngine/Headers/poly.h)
// Like POLY_add_line but preserves UV coordinates from the input POLY_Points.
void POLY_add_line_tex(POLY_Point* p1, POLY_Point* p2, float width1, float width2, SLONG page, UBYTE sort_to_front);

// uc_orig: POLY_add_line_tex_uv (fallen/DDEngine/Source/poly.cpp)
// Like POLY_add_line_tex but allows independent widths for p1 and p2 ends.
void POLY_add_line_tex_uv(POLY_Point* p1, POLY_Point* p2, float width1, float width2, SLONG page, UBYTE sort_to_front);

// uc_orig: POLY_add_line_2d (fallen/DDEngine/Headers/poly.h)
// Draws a 2D screen-space line directly (no 3D transform).
void POLY_add_line_2d(float sx1, float sy1, float sx2, float sy2, ULONG colour);

// uc_orig: POLY_clip_line_box (fallen/DDEngine/Headers/poly.h)
// Sets the 2D clip rectangle for POLY_clip_line_add.
void POLY_clip_line_box(float sx1, float sy1, float sx2, float sy2);

// uc_orig: POLY_clip_line_add (fallen/DDEngine/Headers/poly.h)
// Draws a 2D line clipped to the current clip box.
void POLY_clip_line_add(float sx1, float sy1, float sx2, float sy2, ULONG colour);

// uc_orig: POLY_frame_draw (fallen/DDEngine/Headers/poly.h)
// Flushes all buffered polygons to D3D. draw_shadow_page/draw_text_page = UC_FALSE to skip those buckets.
void POLY_frame_draw(SLONG draw_shadow_page, SLONG draw_text_page);

// uc_orig: POLY_sort_sewater_page (fallen/DDEngine/Headers/poly.h)
// Sorts sewer-water page polygons in back-to-front order for correct alpha blending.
void POLY_sort_sewater_page(void);

// uc_orig: POLY_frame_draw_odd (fallen/DDEngine/Headers/poly.h)
// Draws only normal-texture polygons with MODULATEALPHA blend mode.
void POLY_frame_draw_odd(void);

// uc_orig: POLY_frame_draw_puddles (fallen/DDEngine/Headers/poly.h)
// Draws only the puddle texture page.
void POLY_frame_draw_puddles(void);

// uc_orig: POLY_frame_draw_sewater (fallen/DDEngine/Headers/poly.h)
// Draws only the sewer-water texture page.
void POLY_frame_draw_sewater(void);

// uc_orig: POLY_interpolate_colour (fallen/DDEngine/Headers/poly.h)
// Linearly interpolates between two ARGB colours at fraction v in [0,1].
ULONG POLY_interpolate_colour(float v, ULONG colour1, ULONG colour2);

// uc_orig: POLY_frame_draw_focused (fallen/DDEngine/Headers/poly.h)
// Draws the frame with depth-of-field focus applied at 'focus' (0=near, 1=far).
void POLY_frame_draw_focused(float focus);

// uc_orig: POLY_add_shared_start (fallen/DDEngine/Headers/poly.h)
// Begins a shared-vertex polygon group for a single texture page.
// All subsequent shared points/tris share vertices in the page's vertex buffer.
void POLY_add_shared_start(SLONG page);

// uc_orig: POLY_add_shared_point (fallen/DDEngine/Headers/poly.h)
// Adds a vertex to the current shared group.
void POLY_add_shared_point(POLY_Point* pp);

// uc_orig: POLY_add_shared_tri (fallen/DDEngine/Headers/poly.h)
// Adds a triangle by index into the current shared group's vertex list.
// p1/p2/p3 are 0-based indices relative to the first shared point added.
void POLY_add_shared_tri(UWORD p1, UWORD p2, UWORD p3);

// uc_orig: POLY_inside_quad (fallen/DDEngine/Headers/poly.h)
// Returns UC_TRUE if the 2D screen point is inside the given parallelogram quad.
// along_01 and along_02 receive the barycentric coordinates along the two edge vectors.
SLONG POLY_inside_quad(
    float screen_x,
    float screen_y,
    POLY_Point* quad[3],
    float* along_01,
    float* along_02);

// Per-page rendering flags.
// uc_orig: POLY_PAGE_FLAG_TRANSPARENT (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_TRANSPARENT   (1 << 0)
// uc_orig: POLY_PAGE_FLAG_WRAP (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_WRAP          (1 << 1)
// uc_orig: POLY_PAGE_FLAG_ADD_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_ADD_ALPHA     (1 << 2)
// uc_orig: POLY_PAGE_FLAG_2PASS (fallen/DDEngine/Headers/poly.h)
// Drawn two-pass with next texture page as a mask (masked self-illuminating).
#define POLY_PAGE_FLAG_2PASS         (1 << 3)
// uc_orig: POLY_PAGE_FLAG_SELF_ILLUM (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_SELF_ILLUM    (1 << 4)
// uc_orig: POLY_PAGE_FLAG_NO_FOG (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_NO_FOG        (1 << 5)
// uc_orig: POLY_PAGE_FLAG_WINDOW (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_WINDOW        (1 << 6)
// uc_orig: POLY_PAGE_FLAG_WINDOW_2ND (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_WINDOW_2ND    (1 << 7)
// uc_orig: POLY_PAGE_FLAG_ALPHA (fallen/DDEngine/Headers/poly.h)
#define POLY_PAGE_FLAG_ALPHA         (1 << 8)

// uc_orig: draw_3d (fallen/DDEngine/Headers/poly.h)
// When non-zero, 3D rendering is active (not a pure 2D menu frame).
extern SLONG draw_3d;

// uc_orig: POLY_page_flag (fallen/DDEngine/Headers/poly.h)
// Array of per-page flags. Indexed by texture page number.
extern UWORD POLY_page_flag[POLY_NUM_PAGES];

// uc_orig: POLY_init_render_states (fallen/DDEngine/Headers/poly.h)
// Initialises the D3D render state configuration for the rendering pipeline.
extern void POLY_init_render_states();

#endif // ENGINE_GRAPHICS_PIPELINE_POLY_H
