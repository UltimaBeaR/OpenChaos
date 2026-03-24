#include "engine/graphics/lighting/crinkle.h"
#include "engine/input/keyboard.h"
#include "engine/input/keyboard_globals.h"
#include "engine/platform/uc_common.h"
#include <math.h>

// uc_orig: CRINKLE_MAX_POINTS_PER_CRINKLE (fallen/DDEngine/Source/Crinkle.cpp)
#define CRINKLE_MAX_POINTS_PER_CRINKLE 700

// uc_orig: CRINKLE_pp (fallen/DDEngine/Source/Crinkle.cpp)
// Temporary transformed crinkle vertex buffer for one CRINKLE_do call.
static POLY_Point CRINKLE_pp[CRINKLE_MAX_POINTS_PER_CRINKLE];

// Internal types -------------------------------------------------------

// uc_orig: CRINKLE_Point (fallen/DDEngine/Source/Crinkle.cpp)
// One vertex in a crinkle mesh. vec1/vec2 are parametric coords (0..1) on the
// base quad; vec3 is the extrusion weight; c[4] are bilinear colour weights.
typedef struct
{
    float vec1; // parametric U on the quad (0..1)
    float vec2; // parametric V on the quad (0..1)
    float vec3; // extrusion weight along surface normal
    UBYTE c[4]; // bilinear colour weights for the four quad corners (sum ~128)

} CRINKLE_Point;

// uc_orig: CRINKLE_MAX_POINTS (fallen/DDEngine/Source/Crinkle.cpp)
#define CRINKLE_MAX_POINTS 8192

// uc_orig: CRINKLE_point (fallen/DDEngine/Source/Crinkle.cpp)
static CRINKLE_Point CRINKLE_point[CRINKLE_MAX_POINTS];
// uc_orig: CRINKLE_point_upto (fallen/DDEngine/Source/Crinkle.cpp)
static SLONG CRINKLE_point_upto;

// uc_orig: CRINKLE_Face (fallen/DDEngine/Source/Crinkle.cpp)
// One triangle in a crinkle mesh, referencing three CRINKLE_point indices.
typedef struct
{
    UWORD point[3]; // indices into CRINKLE_point[]

} CRINKLE_Face;

// uc_orig: CRINKLE_MAX_FACES (fallen/DDEngine/Source/Crinkle.cpp)
#define CRINKLE_MAX_FACES 8192

// uc_orig: CRINKLE_face (fallen/DDEngine/Source/Crinkle.cpp)
static CRINKLE_Face CRINKLE_face[CRINKLE_MAX_FACES];
// uc_orig: CRINKLE_face_upto (fallen/DDEngine/Source/Crinkle.cpp)
static SLONG CRINKLE_face_upto;

// uc_orig: CRINKLE_Crinkle (fallen/DDEngine/Source/Crinkle.cpp)
// One crinkle entry: number of points/faces and pointers into the shared pools.
typedef struct
{
    SLONG num_points;
    SLONG num_faces;

    CRINKLE_Point* point;
    CRINKLE_Face* face;

} CRINKLE_Crinkle;

// uc_orig: CRINKLE_MAX_CRINKLES (fallen/DDEngine/Source/Crinkle.cpp)
#define CRINKLE_MAX_CRINKLES 256

// uc_orig: CRINKLE_crinkle (fallen/DDEngine/Source/Crinkle.cpp)
static CRINKLE_Crinkle CRINKLE_crinkle[CRINKLE_MAX_CRINKLES];
// uc_orig: CRINKLE_crinkle_upto (fallen/DDEngine/Source/Crinkle.cpp)
static SLONG CRINKLE_crinkle_upto;

// Public API -----------------------------------------------------------

// uc_orig: CRINKLE_init (fallen/DDEngine/Source/Crinkle.cpp)
void CRINKLE_init(void)
{
    CRINKLE_crinkle_upto = 1;
    CRINKLE_point_upto = 0;
    CRINKLE_face_upto = 0;
}

// uc_orig: CRINKLE_MAX_LINE (fallen/DDEngine/Source/Crinkle.cpp)
#define CRINKLE_MAX_LINE 256

// uc_orig: CRINKLE_load (fallen/DDEngine/Source/Crinkle.cpp)
// Loads a crinkle from an ASCII .sex file. In practice always returns NULL
// (early return at top of function body — ASCII loader was abandoned).
CRINKLE_Handle CRINKLE_load(CBYTE* asc_filename)
{
    return NULL;
}

// uc_orig: CRINKLE_read_bin (fallen/DDEngine/Source/Crinkle.cpp)
// Reads a pre-built binary crinkle from a FileClump; used at level load time.
CRINKLE_Handle CRINKLE_read_bin(FileClump* tclump, int id)
{
    int ii;

    UBYTE* buffer = tclump->Read(id);
    if (!buffer)
        return NULL;

    UBYTE* bptr = buffer;

    ASSERT(WITHIN(CRINKLE_crinkle_upto, 1, CRINKLE_MAX_CRINKLES - 1));

    CRINKLE_Handle ans = CRINKLE_crinkle_upto;
    CRINKLE_Crinkle* cc = &CRINKLE_crinkle[CRINKLE_crinkle_upto];
    CRINKLE_crinkle_upto++;

    memcpy(cc, bptr, sizeof(*cc));
    bptr += sizeof(*cc);

    CRINKLE_Point* cp = &CRINKLE_point[CRINKLE_point_upto];
    CRINKLE_Face* cf = &CRINKLE_face[CRINKLE_face_upto];

    cc->point = cp;
    cc->face = cf;

    CRINKLE_point_upto += cc->num_points;
    CRINKLE_face_upto += cc->num_faces;

    for (ii = 0; ii < cc->num_points; ii++) {
        memcpy(cp, bptr, sizeof(*cp));
        cp++;
        bptr += sizeof(*cp);
    }

    for (ii = 0; ii < cc->num_faces; ii++) {
        memcpy(cf, bptr, sizeof(*cf));
        cf++;
        bptr += sizeof(*cf);
    }

    return ans;
}

// uc_orig: CRINKLE_write_bin (fallen/DDEngine/Source/Crinkle.cpp)
// Writes a crinkle to a FileClump archive (used by the level build tools).
void CRINKLE_write_bin(FileClump* tclump, CRINKLE_Handle hnd, int id)
{
    CRINKLE_Crinkle* cc = &CRINKLE_crinkle[hnd];

    int size = sizeof(CRINKLE_Crinkle) + cc->num_points * sizeof(CRINKLE_Point) + cc->num_faces * sizeof(CRINKLE_Face);

    UBYTE* buffer = new UBYTE[size];
    ASSERT(buffer);
    UBYTE* bptr = buffer;

    memcpy(bptr, cc, sizeof(*cc));
    bptr += sizeof(*cc);

    CRINKLE_Point* cp = cc->point;
    CRINKLE_Face* cf = cc->face;

    for (int ii = 0; ii < cc->num_points; ii++) {
        memcpy(bptr, cp, sizeof(*cp));
        cp++;
        bptr += sizeof(*cp);
    }

    for (int ii = 0; ii < cc->num_faces; ii++) {
        memcpy(bptr, cf, sizeof(*cf));
        cf++;
        bptr += sizeof(*cf);
    }

    ASSERT(bptr - buffer == size);

    tclump->Write(buffer, size, id);
}

// Lighting state -------------------------------------------------------

// uc_orig: CRINKLE_light_x (fallen/DDEngine/Source/Crinkle.cpp)
static float CRINKLE_light_x;
// uc_orig: CRINKLE_light_y (fallen/DDEngine/Source/Crinkle.cpp)
static float CRINKLE_light_y;
// uc_orig: CRINKLE_light_z (fallen/DDEngine/Source/Crinkle.cpp)
static float CRINKLE_light_z;

// uc_orig: CRINKLE_mul_x (fallen/DDEngine/Source/Crinkle.cpp)
static float CRINKLE_mul_x;
// uc_orig: CRINKLE_mul_y (fallen/DDEngine/Source/Crinkle.cpp)
static float CRINKLE_mul_y;
// uc_orig: CRINKLE_mul_recip_x (fallen/DDEngine/Source/Crinkle.cpp)
static float CRINKLE_mul_recip_x;
// uc_orig: CRINKLE_mul_recip_y (fallen/DDEngine/Source/Crinkle.cpp)
static float CRINKLE_mul_recip_y;

// uc_orig: CRINKLE_skew (fallen/DDEngine/Source/Crinkle.cpp)
void CRINKLE_skew(float aspect, float lens)
{
    CRINKLE_mul_x = aspect * lens;
    CRINKLE_mul_y = lens;
    CRINKLE_mul_recip_x = 1.0F / CRINKLE_mul_x;
    CRINKLE_mul_recip_y = 1.0F / CRINKLE_mul_y;
}

// uc_orig: CRINKLE_light (fallen/DDEngine/Source/Crinkle.cpp)
void CRINKLE_light(float dx, float dy, float dz)
{
    float len = sqrt(dx * dx + dy * dy + dz * dz);
    float overlen = 1.0F / len;

    dx *= overlen;
    dy *= overlen;
    dz *= overlen;

    CRINKLE_light_x = dx;
    CRINKLE_light_y = dy;
    CRINKLE_light_z = dz;
}

// uc_orig: CRINKLE_do (fallen/DDEngine/Source/Crinkle.cpp)
// Draws a crinkle by extruding its mesh from the four view-space corner points.
void CRINKLE_do(
    CRINKLE_Handle crinkle,
    SLONG page,
    float extrude,
    POLY_Point* pp[4],
    SLONG flip)
{
    SLONG i;
    CRINKLE_Crinkle* cc;

    ASSERT(WITHIN(crinkle, 0, CRINKLE_crinkle_upto - 1));

    cc = &CRINKLE_crinkle[crinkle];

    if (flip) {
#define PPSWAP(a, b)                \
    {                               \
        POLY_Point* pp_spare = (a); \
        (a) = (b);                  \
        (b) = pp_spare;             \
    }
        PPSWAP(pp[0], pp[1]);
        PPSWAP(pp[2], pp[3]);
    }

    // Un-warp view space to get a symmetric frustum for normal computation.
    pp[0]->x *= CRINKLE_mul_recip_x;
    pp[1]->x *= CRINKLE_mul_recip_x;
    pp[2]->x *= CRINKLE_mul_recip_x;
    pp[3]->x *= CRINKLE_mul_recip_x;

    pp[0]->y *= CRINKLE_mul_recip_y;
    pp[1]->y *= CRINKLE_mul_recip_y;
    pp[2]->y *= CRINKLE_mul_recip_y;
    pp[3]->y *= CRINKLE_mul_recip_y;

    // Compute base vectors of the quad.
    float ox = pp[0]->x; float oy = pp[0]->y; float oz = pp[0]->z;
    float ou = pp[0]->u; float ov = pp[0]->v;

    float ax = pp[1]->x - ox; float ay = pp[1]->y - oy; float az = pp[1]->z - oz;
    float au = pp[1]->u - ou; float av = pp[1]->v - ov;

    float bx = pp[2]->x - ox; float by = pp[2]->y - oy; float bz = pp[2]->z - oz;
    float bu = pp[2]->u - ou; float bv = pp[2]->v - ov;

    // Cross product gives normal direction; scale for extrusion.
    float cx = (ay * bz - az * by) * (1.0F / 256.0F);
    float cy = (az * bx - ax * bz) * (1.0F / 256.0F);
    float cz = (ax * by - ay * bx) * (1.0F / 256.0F);

    float len = sqrt(cx * cx + cy * cy + cz * cz);
    float overlen = 0.05F / len;

    if (flip) { overlen = -overlen; }

    SLONG pr[4], pg[4], pb[4], pa[4];

    for (i = 0; i < 4; i++) {
        pa[i] = (pp[i]->colour >> 24) & 0xff;
        pr[i] = (pp[i]->colour >> 16) & 0xff;
        pg[i] = (pp[i]->colour >> 8)  & 0xff;
        pb[i] = (pp[i]->colour >> 0)  & 0xff;
    }

    cx *= overlen; cy *= overlen; cz *= overlen;

    // Re-warp view space.
    pp[0]->x *= CRINKLE_mul_x; pp[1]->x *= CRINKLE_mul_x;
    pp[2]->x *= CRINKLE_mul_x; pp[3]->x *= CRINKLE_mul_x;
    pp[0]->y *= CRINKLE_mul_y; pp[1]->y *= CRINKLE_mul_y;
    pp[2]->y *= CRINKLE_mul_y; pp[3]->y *= CRINKLE_mul_y;

    // Transform all crinkle mesh points into view space.
    CRINKLE_Point* cp;
    POLY_Point* pt;

    ASSERT(WITHIN(cc->num_points, 4, CRINKLE_MAX_POINTS_PER_CRINKLE));

    for (i = 0; i < cc->num_points; i++) {
        cp = &cc->point[i];
        pt = &CRINKLE_pp[i];

        pt->x = ox + cp->vec1 * ax + cp->vec2 * bx + cp->vec3 * cx * extrude;
        pt->y = oy + cp->vec1 * ay + cp->vec2 * by + cp->vec3 * cy * extrude;
        pt->z = oz + cp->vec1 * az + cp->vec2 * bz + cp->vec3 * cz * extrude;
        pt->u = ou + cp->vec1 * au + cp->vec2 * bu;
        pt->v = ov + cp->vec1 * av + cp->vec2 * bv;

        SLONG r = pr[0] * cp->c[0] + pr[1] * cp->c[1] + pr[2] * cp->c[2] + pr[3] * cp->c[3] >> 7;
        SLONG g = pg[0] * cp->c[0] + pg[1] * cp->c[1] + pg[2] * cp->c[2] + pg[3] * cp->c[3] >> 7;
        SLONG b = pb[0] * cp->c[0] + pb[1] * cp->c[1] + pb[2] * cp->c[2] + pb[3] * cp->c[3] >> 7;
        SLONG a = pa[0] * cp->c[0] + pa[1] * cp->c[1] + pa[2] * cp->c[2] + pa[3] * cp->c[3] >> 7;

        ASSERT(WITHIN(r, 0, 255));
        ASSERT(WITHIN(g, 0, 255));
        ASSERT(WITHIN(b, 0, 255));
        ASSERT(WITHIN(a, 0, 255));

        pt->colour   = (a << 24) | (r << 16) | (g << 8) | (b << 0);
        pt->specular = 0xff000000;

        pt->x *= CRINKLE_mul_x;
        pt->y *= CRINKLE_mul_y;

        POLY_transform_from_view_space(pt);
    }

    // Render each face, adjusting vertex lighting by normal vs light dot product.
    POLY_Point* tri[3];
    CRINKLE_Face* cf;

    for (i = 0; i < cc->num_faces; i++) {
        cf = &cc->face[i];

        if (flip) {
            tri[0] = &CRINKLE_pp[cf->point[0]];
            tri[1] = &CRINKLE_pp[cf->point[2]];
            tri[2] = &CRINKLE_pp[cf->point[1]];
        } else {
            tri[0] = &CRINKLE_pp[cf->point[0]];
            tri[1] = &CRINKLE_pp[cf->point[1]];
            tri[2] = &CRINKLE_pp[cf->point[2]];
        }

        if (POLY_valid_triangle(tri)) {
            // Only shade faces that actually extrude (have nonzero vec3 on all corners).
            if (fabsf(cc->point[cf->point[0]].vec3) + fabsf(cc->point[cf->point[1]].vec3) + fabsf(cc->point[cf->point[2]].vec3) > 0.001F) {
                // Compute face normal and modulate vertex colour by light dot product.
                float ax2 = tri[1]->x - tri[0]->x; float ay2 = tri[1]->y - tri[0]->y; float az2 = tri[1]->z - tri[0]->z;
                float bx2 = tri[2]->x - tri[0]->x; float by2 = tri[2]->y - tri[0]->y; float bz2 = tri[2]->z - tri[0]->z;

                float nx = ay2 * bz2 - az2 * by2;
                float ny = az2 * bx2 - ax2 * bz2;
                float nz = ax2 * by2 - ay2 * bx2;

                float nlen = sqrt(nx * nx + ny * ny + nz * nz);
                float noverlen = 1.0F / nlen;
                nx *= noverlen; ny *= noverlen; nz *= noverlen;

                float dprod = nx * CRINKLE_light_x + ny * CRINKLE_light_y + nz * CRINKLE_light_z;
                float dbright = dprod * extrude;
                SLONG drgb = dbright * 64.0F;

                ULONG c0 = tri[0]->colour;
                ULONG c1 = tri[1]->colour;
                ULONG c2 = tri[2]->colour;

                SLONG r, g, b, a;

                r = ((c0 >> 16) & 0xff) + drgb; g = ((c0 >> 8) & 0xff) + drgb;
                b = ((c0 >> 0) & 0xff) + drgb;  a = ((c0 >> 24) & 0xff) + drgb;
                SATURATE(r, 0, 255); SATURATE(g, 0, 255); SATURATE(b, 0, 255); SATURATE(a, 0, 255);
                tri[0]->colour = (a << 24) | (r << 16) | (g << 8) | (b << 0);

                r = ((c1 >> 16) & 0xff) + drgb; g = ((c1 >> 8) & 0xff) + drgb;
                b = ((c1 >> 0) & 0xff) + drgb;  a = ((c1 >> 24) & 0xff) + drgb;
                SATURATE(r, 0, 255); SATURATE(g, 0, 255); SATURATE(b, 0, 255); SATURATE(a, 0, 255);
                tri[1]->colour = (a << 24) | (r << 16) | (g << 8) | (b << 0);

                r = ((c2 >> 16) & 0xff) + drgb; g = ((c2 >> 8) & 0xff) + drgb;
                b = ((c2 >> 0) & 0xff) + drgb;  a = ((c2 >> 24) & 0xff) + drgb;
                SATURATE(r, 0, 255); SATURATE(g, 0, 255); SATURATE(b, 0, 255); SATURATE(a, 0, 255);
                tri[2]->colour = (a << 24) | (r << 16) | (g << 8) | (b << 0);

                POLY_add_triangle(tri, page, UC_TRUE);

                // Restore original colours.
                tri[0]->colour = c0;
                tri[1]->colour = c1;
                tri[2]->colour = c2;
            } else {
                POLY_add_triangle(tri, page, UC_TRUE);
            }
        }
    }
}

// uc_orig: CRINKLE_project (fallen/DDEngine/Source/Crinkle.cpp)
// Projects an SMAP shadow crinkle over a world-space quad (debug visualization).
void CRINKLE_project(
    CRINKLE_Handle crinkle,
    float extrude,
    SVector_F pp[4],
    SLONG flip)
{
    SLONG i;
    CRINKLE_Crinkle* cc;

    ASSERT(WITHIN(crinkle, 0, CRINKLE_crinkle_upto - 1));

    cc = &CRINKLE_crinkle[crinkle];

    if (Keys[KB_RSHIFT]) {
        flip = UC_TRUE;
    }

    if (flip) {
#define SVSWAP(a, b)              \
    {                             \
        SVector_F sv_spare = (a); \
        (a) = (b);                \
        (b) = sv_spare;           \
    }
        SVSWAP(pp[0], pp[1]);
        SVSWAP(pp[2], pp[3]);
    }

    float ox = pp[0].X; float oy = pp[0].Y; float oz = pp[0].Z;

    float ax = pp[1].X - ox; float ay = pp[1].Y - oy; float az = pp[1].Z - oz;
    float bx = pp[2].X - ox; float by = pp[2].Y - oy; float bz = pp[2].Z - oz;

    float cx = (ay * bz - az * by) * (1.0F / 256.0F);
    float cy = (az * bx - ax * bz) * (1.0F / 256.0F);
    float cz = (ax * by - ay * bx) * (1.0F / 256.0F);

    float len = sqrt(cx * cx + cy * cy + cz * cz);
    float overlen = 0.05F / len;

    if (flip) { overlen = -overlen; }

    cx *= overlen; cy *= overlen; cz *= overlen;

    CRINKLE_Point* cp;
    POLY_Point* pt;

    ASSERT(WITHIN(cc->num_points, 4, CRINKLE_MAX_POINTS_PER_CRINKLE));

    for (i = 0; i < cc->num_points; i++) {
        cp = &cc->point[i];
        pt = &CRINKLE_pp[i];

        pt->x = ox + cp->vec1 * ax + cp->vec2 * bx + cp->vec3 * cx * extrude;
        pt->y = oy + cp->vec1 * ay + cp->vec2 * by + cp->vec3 * cy * extrude;
        pt->z = oz + cp->vec1 * az + cp->vec2 * bz + cp->vec3 * cz * extrude;
    }

    POLY_Point* tri[3];
    CRINKLE_Face* cf;

    for (i = 0; i < cc->num_faces; i++) {
        cf = &cc->face[i];

        if (flip) {
            tri[0] = &CRINKLE_pp[cf->point[0]];
            tri[1] = &CRINKLE_pp[cf->point[2]];
            tri[2] = &CRINKLE_pp[cf->point[1]];
        } else {
            tri[0] = &CRINKLE_pp[cf->point[0]];
            tri[1] = &CRINKLE_pp[cf->point[1]];
            tri[2] = &CRINKLE_pp[cf->point[2]];
        }

        // Debug visualization: draw the crinkle triangles as world-space lines.
        AENG_world_line(
            tri[0]->x, tri[0]->y, tri[0]->z, 4, 0xffffff,
            tri[1]->x, tri[1]->y, tri[1]->z, 3, 0xffffff,
            UC_TRUE);
        AENG_world_line(
            tri[1]->x, tri[1]->y, tri[1]->z, 4, 0xffffff,
            tri[2]->x, tri[2]->y, tri[2]->z, 3, 0xffffff,
            UC_TRUE);
        AENG_world_line(
            tri[2]->x, tri[2]->y, tri[2]->z, 4, 0xffffff,
            tri[0]->x, tri[0]->y, tri[0]->z, 3, 0xffffff,
            UC_TRUE);
    }
}
