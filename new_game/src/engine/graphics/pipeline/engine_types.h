#ifndef ENGINE_GRAPHICS_PIPELINE_ENGINE_TYPES_H
#define ENGINE_GRAPHICS_PIPELINE_ENGINE_TYPES_H

#include <math.h>
#include "engine/core/types.h"
#include "engine/core/math.h"

// THING_INDEX is defined in game headers; replicate the guard pattern for standalone use.
#ifndef THING_INDEX
#define THING_INDEX UWORD
#endif

// Core 3D math structures and engine state types for the DDEngine renderer.
// These types are used by the 3D transform pipeline (aeng.cpp, poly.cpp, figure.cpp).

// uc_orig: M31 (fallen/DDEngine/Headers/Engine.h)
// Three-element float vector. R[0]=X, R[1]=Y, R[2]=Z.
struct M31 {
    float R[3];
};

// uc_orig: M31 (fallen/DDEngine/Headers/Engine.h)
typedef struct M31 M31;

// uc_orig: UV_X (fallen/DDEngine/Headers/Engine.h)
#define UV_X R[0]
// uc_orig: UV_Y (fallen/DDEngine/Headers/Engine.h)
#define UV_Y R[1]
// uc_orig: UV_Z (fallen/DDEngine/Headers/Engine.h)
#define UV_Z R[2]

// uc_orig: SET_M31 (fallen/DDEngine/Headers/Engine.h)
#define SET_M31(v, x, y, z) \
    (v)->UV_X = x;          \
    (v)->UV_Y = y;          \
    (v)->UV_Z = z;
// uc_orig: VECTOR_LENGTH_SQUARED (fallen/DDEngine/Headers/Engine.h)
#define VECTOR_LENGTH_SQUARED(v) (float)(((v)->UV_X * (v)->UV_X) + ((v)->UV_Y * (v)->UV_Y) + ((v)->UV_Z * (v)->UV_Z))
// uc_orig: VECTOR_LENGTH (fallen/DDEngine/Headers/Engine.h)
#define VECTOR_LENGTH(v) (float)sqrt(VECTOR_LENGTH_SQUARED(v))
// uc_orig: DOT_PRODUCT (fallen/DDEngine/Headers/Engine.h)
#define DOT_PRODUCT(v, w) ((v)->UV_X * (w)->UV_X) + ((v)->UV_Y * (w)->UV_Y) + ((v)->UV_Z * (w)->UV_Z)
// uc_orig: CROSS_PRODUCT (fallen/DDEngine/Headers/Engine.h)
#define CROSS_PRODUCT(r, v, w)                                       \
    (r)->UV_X = (((v)->UV_Y * (w)->UV_Z) - ((v)->UV_Z * (w)->UV_Y)); \
    (r)->UV_Y = (((v)->UV_Z * (w)->UV_X) - ((v)->UV_X * (w)->UV_Z)); \
    (r)->UV_Z = (((v)->UV_X * (w)->UV_Y) - ((v)->UV_Y * (w)->UV_X));

// uc_orig: approx_vector_length (fallen/DDEngine/Headers/Engine.h)
// Fast approximate magnitude using sorted axis dominance (error ±8%).
inline float approx_vector_length(M31* v)
{
    float max = (float)fabs((v->UV_X)),
          med = (float)fabs((v->UV_Y)),
          min = (float)fabs((v->UV_Z)),
          temp;

    if (max < min) {
        temp = max;
        max = min;
        min = temp;
    }
    if (med < min) {
        temp = med;
        med = min;
        min = temp;
    }
    if (max < med) {
        temp = max;
        max = med;
        med = temp;
    }

    return (float)(max + (med * 0.3f) + (min * 0.25f));
}

// uc_orig: vector_normalisation (fallen/DDEngine/Headers/Engine.h)
// Normalises v given its pre-computed length.
inline void vector_normalisation(M31* v, float length)
{
    length = (1.0f / length);
    v->UV_X = (v->UV_X) * length;
    v->UV_Y = (v->UV_Y) * length;
    v->UV_Z = (v->UV_Z) * length;
}

// uc_orig: M33 (fallen/DDEngine/Headers/Engine.h)
// 3x3 rotation matrix composed of three M31 row vectors.
struct M33 {
    M31 R0,
        R1,
        R2;
};

// uc_orig: M33 (fallen/DDEngine/Headers/Engine.h)
typedef struct M33 M33;

// uc_orig: UV_XX (fallen/DDEngine/Headers/Engine.h)
#define UV_XX R0.R[0]
// uc_orig: UV_YX (fallen/DDEngine/Headers/Engine.h)
#define UV_YX R0.R[1]
// uc_orig: UV_ZX (fallen/DDEngine/Headers/Engine.h)
#define UV_ZX R0.R[2]

// uc_orig: UV_XY (fallen/DDEngine/Headers/Engine.h)
#define UV_XY R1.R[0]
// uc_orig: UV_YY (fallen/DDEngine/Headers/Engine.h)
#define UV_YY R1.R[1]
// uc_orig: UV_ZY (fallen/DDEngine/Headers/Engine.h)
#define UV_ZY R1.R[2]

// uc_orig: UV_XZ (fallen/DDEngine/Headers/Engine.h)
#define UV_XZ R2.R[0]
// uc_orig: UV_YZ (fallen/DDEngine/Headers/Engine.h)
#define UV_YZ R2.R[1]
// uc_orig: UV_ZZ (fallen/DDEngine/Headers/Engine.h)
#define UV_ZZ R2.R[2]

// uc_orig: init_M33 (fallen/DDEngine/Headers/Engine.h)
// Initialises m to an identity rotation matrix.
inline void init_M33(M33* m)
{
    m->UV_XX = (float)1.0;
    m->UV_XY = (float)0.0;
    m->UV_XZ = (float)0.0;
    m->UV_YX = (float)0.0;
    m->UV_YY = (float)1.0;
    m->UV_YZ = (float)0.0;
    m->UV_ZX = (float)0.0;
    m->UV_ZY = (float)0.0;
    m->UV_ZZ = (float)1.0;
}

// uc_orig: rotate_on_x (fallen/DDEngine/Headers/Engine.h)
// Applies a rotation around the X axis to matrix, using SIN/COS lookup tables.
inline void rotate_on_x(SLONG angle, M33* matrix)
{
    float sin = SIN_F(angle & 2047),
          cos = COS_F(angle & 2047);
    float t10 = matrix->UV_XY,
          t11 = matrix->UV_YY,
          t12 = matrix->UV_ZY,
          t20 = matrix->UV_XZ,
          t21 = matrix->UV_YZ,
          t22 = matrix->UV_ZZ;

    matrix->UV_XY = ((cos * t10) + (-sin * t20));
    matrix->UV_YY = ((cos * t11) + (-sin * t21));
    matrix->UV_ZY = ((cos * t12) + (-sin * t22));
    matrix->UV_XZ = ((sin * t10) + (cos * t20));
    matrix->UV_YZ = ((sin * t11) + (cos * t21));
    matrix->UV_ZZ = ((sin * t12) + (cos * t22));
}

// uc_orig: rotate_on_y (fallen/DDEngine/Headers/Engine.h)
// Applies a rotation around the Y axis.
inline void rotate_on_y(SLONG angle, M33* matrix)
{
    float sin = SIN_F(angle & 2047),
          cos = COS_F(angle & 2047);
    float t00 = matrix->UV_XX,
          t01 = matrix->UV_YX,
          t02 = matrix->UV_ZX,
          t20 = matrix->UV_XZ,
          t21 = matrix->UV_YZ,
          t22 = matrix->UV_ZZ;

    matrix->UV_XX = ((cos * t00) + (sin * t20));
    matrix->UV_YX = ((cos * t01) + (sin * t21));
    matrix->UV_ZX = ((cos * t02) + (sin * t22));
    matrix->UV_XZ = ((-sin * t00) + (cos * t20));
    matrix->UV_YZ = ((-sin * t01) + (cos * t21));
    matrix->UV_ZZ = ((-sin * t02) + (cos * t22));
}

// uc_orig: rotate_on_z (fallen/DDEngine/Headers/Engine.h)
// Applies a rotation around the Z axis.
inline void rotate_on_z(SLONG angle, M33* matrix)
{
    float sin = SIN_F(angle & 2047),
          cos = COS_F(angle & 2047);
    float t00 = matrix->UV_XX,
          t01 = matrix->UV_YX,
          t02 = matrix->UV_ZX,
          t10 = matrix->UV_XY,
          t11 = matrix->UV_YY,
          t12 = matrix->UV_ZY;

    matrix->UV_XX = ((cos * t00) + (-sin * t10));
    matrix->UV_YX = ((cos * t01) + (-sin * t11));
    matrix->UV_ZX = ((cos * t02) + (-sin * t12));
    matrix->UV_XY = ((sin * t00) + (cos * t10));
    matrix->UV_YY = ((sin * t01) + (cos * t11));
    matrix->UV_ZY = ((sin * t02) + (cos * t12));
}

// uc_orig: angle_vector (fallen/DDEngine/Headers/Engine.h)
// Rotates the XZ components of matrix by angle (yaw).
inline void angle_vector(SLONG angle, M31* matrix)
{
    float sin = SIN_F(angle),
          cos = COS_F(angle);
    float t_x = matrix->UV_X,
          t_z = matrix->UV_Z;

    matrix->UV_X = ((cos * t_x) + (sin * t_z));
    matrix->UV_Z = ((-sin * t_x) + (cos * t_z));
}

// uc_orig: roll_vector (fallen/DDEngine/Headers/Engine.h)
// Rotates the XY components of matrix by roll.
inline void roll_vector(SLONG roll, M31* matrix)
{
    float sin = SIN_F(roll),
          cos = COS_F(roll);
    float t_x = matrix->UV_X,
          t_y = matrix->UV_Y;

    matrix->UV_X = ((cos * t_x) + (sin * t_y));
    matrix->UV_Y = ((-sin * t_x) + (cos * t_y));
}

// uc_orig: tilt_vector (fallen/DDEngine/Headers/Engine.h)
// Rotates the YZ components of matrix by tilt (pitch).
inline void tilt_vector(SLONG tilt, M31* matrix)
{
    float sin = SIN_F(tilt),
          cos = COS_F(tilt);
    float t_y = matrix->UV_Y,
          t_z = matrix->UV_Z;

    matrix->UV_Y = ((cos * t_y) + (sin * t_z));
    matrix->UV_Z = ((-sin * t_y) + (cos * t_z));
}

// View frustum clip flag bits returned by transform_point.
// uc_orig: EF_OFF_LEFT (fallen/DDEngine/Headers/Engine.h)
#define EF_OFF_LEFT (1 << 0)
// uc_orig: EF_OFF_RIGHT (fallen/DDEngine/Headers/Engine.h)
#define EF_OFF_RIGHT (1 << 1)
// uc_orig: EF_OFF_TOP (fallen/DDEngine/Headers/Engine.h)
#define EF_OFF_TOP (1 << 2)
// uc_orig: EF_OFF_BOTTOM (fallen/DDEngine/Headers/Engine.h)
#define EF_OFF_BOTTOM (1 << 3)
// uc_orig: EF_FAR_OUT (fallen/DDEngine/Headers/Engine.h)
#define EF_FAR_OUT (1 << 4)
// uc_orig: EF_BEHIND_YOU (fallen/DDEngine/Headers/Engine.h)
#define EF_BEHIND_YOU (1 << 5)
// uc_orig: EF_TRANSLATED (fallen/DDEngine/Headers/Engine.h)
#define EF_TRANSLATED (1 << 6)
// uc_orig: EF_TOO_BIG (fallen/DDEngine/Headers/Engine.h)
#define EF_TOO_BIG (1 << 7)

// uc_orig: EF_CLIPFLAGS (fallen/DDEngine/Headers/Engine.h)
#define EF_CLIPFLAGS (EF_OFF_LEFT + EF_OFF_RIGHT + EF_OFF_TOP + EF_OFF_BOTTOM)
// uc_orig: EF_CLIPZFLAGS (fallen/DDEngine/Headers/Engine.h)
#define EF_CLIPZFLAGS (EF_FAR_OUT + EF_BEHIND_YOU + EF_TOO_BIG)

// uc_orig: SVECTOR_F (fallen/DDEngine/Headers/Engine.h)
// A world-space point with both pre-transform (X,Y,Z) and view-space (RotX,RotY,RotZ) coordinates.
typedef struct
{
    float X,
        Y,
        Z;

    float RotX;
    float RotY;
    float RotZ;

} SVECTOR_F;

// uc_orig: Coord (fallen/DDEngine/Headers/Engine.h)
// Simple float 3D coordinate. Used for CameraPos in the Engine struct.
typedef struct
{
    float X,
        Y,
        Z;
} Coord;

// uc_orig: Camera (fallen/DDEngine/Headers/Engine.h)
// Camera parameters passed to set_camera() / AENG_set_camera().
typedef struct
{
    float CameraX,
        CameraY,
        CameraZ;
    SLONG CameraAngle,
        CameraRoll,
        CameraTilt;
} Camera;

// uc_orig: Engine (fallen/DDEngine/Headers/Engine.h)
// Renderer state: viewport dimensions, camera matrix, lens factor, bucket count.
typedef struct
{
    float HalfViewHeight,
        HalfViewWidth,
        OriginX,
        OriginY,
        OriginZ,
        ViewHeight,
        ViewWidth,
        Lens;
    SLONG BucketSize;
    M33 CameraMatrix;
    Coord CameraPos;

} Engine;

// uc_orig: DDEnginePoint (fallen/DDEngine/Headers/Engine.h)
// Result of transform_point: projected 2D screen position and depth.
typedef struct
{
    float Distance,
        ScrX,
        ScrY,
        ScrZ;

} DDEnginePoint;

// uc_orig: EnginePointF (fallen/DDEngine/Headers/Engine.h)
// Extended projected point with both screen (ScrX/Y/Z) and world (X/Y/Z) coordinates.
struct EnginePointF {
    float ScrX,
        ScrY,
        ScrZ,
        X,
        Y,
        Z;
};

// Vertex pool size and UV scaling constants.
// uc_orig: MAX_VERTICES (fallen/DDEngine/Headers/Engine.h)
#define MAX_VERTICES (32000)
// uc_orig: ELE_SHIFT (fallen/DDEngine/Headers/Engine.h)
#define ELE_SHIFT (8)
// uc_orig: ELE_SHIFT_F (fallen/DDEngine/Headers/Engine.h)
#define ELE_SHIFT_F (1.0f / (float)(ELE_SIZE))
// uc_orig: ELE_SIZE_F (fallen/DDEngine/Headers/Engine.h)
#define ELE_SIZE_F ((float)(1 << ELE_SHIFT))
// uc_orig: TEXTURE_MUL (fallen/DDEngine/Headers/Engine.h)
// Converts integer UV coordinates to normalised [0,1] range (1/256).
#define TEXTURE_MUL (0.00390625f)
// uc_orig: SHADE_MUL (fallen/DDEngine/Headers/Engine.h)
// Normalises a 0-128 shade value to [0,1].
#define SHADE_MUL ((float)(1.0f / 128.0f))

// uc_orig: ALT_SHIFT (fallen/DDEngine/Headers/Engine.h)
#define ALT_SHIFT (3)

// Depth range for the view frustum.
// uc_orig: MAX_Z (fallen/DDEngine/Headers/Engine.h)
#define MAX_Z 10000.0F
// uc_orig: MIN_Z (fallen/DDEngine/Headers/Engine.h)
#define MIN_Z 32.0F
// uc_orig: SCALE_SZ (fallen/DDEngine/Headers/Engine.h)
#define SCALE_SZ (MIN_Z / MAX_Z)

// uc_orig: ENGINE_multiply_colour (fallen/DDEngine/Headers/Engine.h)
// Multiplies each RGB channel of a packed D3D colour by (r,g,b)/256.
inline ULONG ENGINE_multiply_colour(ULONG colour, SLONG r, SLONG g, SLONG b)
{
    ULONG ans;

    SLONG cr = (colour >> 8) & 0xff;
    SLONG cg = (colour >> 16) & 0xff;
    SLONG cb = (colour >> 24) & 0xff;

    cr *= r;
    cg *= g;
    cb *= b;

    cr >>= 8;
    cg >>= 8;
    cb >>= 8;

    ans = (cr << 0) | (cg << 8) | (cb << 16);

    return ans;
}

// Transform and rendering functions (implementations in DDEngine — not migrated yet).

// uc_orig: set_camera (fallen/DDEngine/Headers/Engine.h)
// Applies a Camera to the_engine, rebuilding the view matrix.
void set_camera(Camera* the_camera);

// uc_orig: render_buckets (fallen/DDEngine/Headers/Engine.h)
// Flushes all queued bucket lists to the GPU.
void render_buckets(void);

// uc_orig: do_map_who (fallen/DDEngine/Headers/Engine.h)
// Updates the map-who cell at (map_x, map_z) for the given thing.
void do_map_who(THING_INDEX thing, SLONG map_x, SLONG map_z);

#endif // ENGINE_GRAPHICS_PIPELINE_ENGINE_TYPES_H
