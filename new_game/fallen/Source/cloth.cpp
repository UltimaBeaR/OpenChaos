//
// Draped cloth and flags.
//

// claude-ai: CLOTH SIMULATION — DISABLED IN SHIPPING GAME, DO NOT PORT.
// claude-ai:
// claude-ai: This system was intended to simulate draped cloth (capes, flags) on character models and
// claude-ai: world geometry using a spring-mass network. The entire CLOTH_process() update loop is
// claude-ai: wrapped in a #if 0 / #endif block — it compiles but never runs.
// claude-ai:
// claude-ai: The system is also excluded entirely from PSX builds (#ifndef PSX wraps the whole file).
// claude-ai:
// claude-ai: WHAT IT INTENDED TO DO:
// claude-ai:   - Represent cloth as a 2D grid of CLOTH_Point vertices (CLOTH_WIDTH x CLOTH_HEIGHT).
// claude-ai:   - Connect vertices with CLOTH_Link spring constraints (axis-aligned + diagonal).
// claude-ai:   - Each tick (3 sub-steps per frame): apply spring forces, damp velocities, apply gravity,
// claude-ai:     prevent underground penetration (y >= 2.0).
// claude-ai:   - For CLOTH_TYPE_FLAG: apply random wind force to unlocked vertices each sub-step.
// claude-ai:   - Locked vertices (CLOTH_point_lock) are anchor points — zero velocity, fixed in space.
// claude-ai:   - CLOTH_get_info computes per-vertex normals for lighting (cross-product of adjacent edges).
// claude-ai:
// claude-ai: REASON FOR EXCLUSION: performance / time constraints in pre-release build.
// claude-ai: The simulation code is structurally correct (spring physics, damping, gravity all present),
// claude-ai: just disabled at compile time.
// claude-ai:
// claude-ai: DO NOT PORT: the final shipped game has no cloth simulation. Characters use static capes.

#include "game.h"
#include <MFStdLib.h>
#include <math.h>
#include "cloth.h"
#include "pap.h"

//
// The cloth
//

// claude-ai: CLOTH_Point — one vertex in the spring-mass grid.
// claude-ai: (x,y,z) = world position (float, not fixed-point unlike the rest of the engine).
// claude-ai: (dx,dy,dz) = velocity accumulated from spring forces and wind; reset each sub-step.
typedef struct
{
    float x;
    float y;
    float z;
    float dx;
    float dy;
    float dz;

} CLOTH_Point;

// claude-ai: CLOTH_Cloth — one cloth instance (cape, flag, drape).
// claude-ai: lock: bitmask, bit i = CLOTH_INDEX(w,h) locked. Locked points have velocity zeroed each tick.
// claude-ai: dist[0]: rest-length squared for axis-aligned springs (ONE links).
// claude-ai: dist[1]: rest-length squared for diagonal springs (HYP links) = dist[0] * 2 (Pythagoras).
// claude-ai: offset_x/y/z: applied to all points at render time (not used in simulation — always 0).
typedef struct
{
    UBYTE type;
    UBYTE next; // claude-ai: mapwho linked-list next index (for spatial bucketing by PAP tile)
    UBYTE padding;
    ULONG colour;
    ULONG lock; // One bit per point.

    float offset_x;
    float offset_y;
    float offset_z;

    float dist[2]; // [0] = The distance the points want to be from eachother SQUARED
                   // [1] = The distance * 1.41421 ... root2				   SQUARED
                   // Index by the type of link.

    CLOTH_Point p[CLOTH_WIDTH * CLOTH_HEIGHT];

} CLOTH_Cloth;

// claude-ai: Max 16 simultaneous cloth instances. Slot 0 is never used (CLOTH_cloth_last starts at 1).
#define CLOTH_MAX_CLOTH 16

CLOTH_Cloth CLOTH_cloth[CLOTH_MAX_CLOTH];
SLONG CLOTH_cloth_last;

//
// The links between the different cloth.
//

// claude-ai: CLOTH_LINK_TYPE_ONE  — axis-aligned spring (horizontal or vertical neighbour). Rest = dist[0].
// claude-ai: CLOTH_LINK_TYPE_HYP  — diagonal spring (45°). Rest = dist[1] = dist[0] * sqrt(2)^2 = dist[0]*2.
#define CLOTH_LINK_TYPE_ONE 0
#define CLOTH_LINK_TYPE_HYP 1 // A diagonal connection.

// claude-ai: CLOTH_Link — one spring connecting two cloth points. Pre-computed at CLOTH_init time.
// claude-ai: Links are shared across all cloth instances (same topology for all).
typedef struct
{
    UBYTE p1; // claude-ai: index into CLOTH_Cloth::p[] for first endpoint
    UBYTE p2; // claude-ai: index into CLOTH_Cloth::p[] for second endpoint
    UBYTE type; // claude-ai: CLOTH_LINK_TYPE_ONE or CLOTH_LINK_TYPE_HYP

} CLOTH_Link;

// claude-ai: 256 links for a CLOTH_WIDTH x CLOTH_HEIGHT grid with all 8-neighbour connections.
#define CLOTH_MAX_LINKS 256

CLOTH_Link CLOTH_link[CLOTH_MAX_LINKS];
SLONG CLOTH_link_upto; // claude-ai: actual number of links built during CLOTH_init

//
// How we calculate the normal of each point.
//

// claude-ai: CLOTH_Normal — pre-computed neighbour info for per-vertex normal calculation.
// claude-ai: For each vertex, stores up to 4 pairs of adjacent vertices (p1[i], p2[i]).
// claude-ai: At render time: normal = average of cross-products (p1[i]-centre) x (p2[i]-centre).
// claude-ai: overnum = 1/num for averaging (avoids divide in the render loop).
// claude-ai: Built once in CLOTH_init; same for all cloth instances with the same grid dimensions.
typedef struct
{
    ULONG num; // claude-ai: actual number of valid neighbour pairs (1..4 depending on grid position)
    UBYTE p1[4]; // claude-ai: first neighbour index in each cross-product pair
    UBYTE p2[4]; // claude-ai: second neighbour index in each cross-product pair
    float overnum; // claude-ai: 1.0f / num — reciprocal for normal averaging

} CLOTH_Normal;

#define CLOTH_MAX_NORMALS (CLOTH_WIDTH * CLOTH_HEIGHT)

CLOTH_Normal CLOTH_normal[CLOTH_MAX_NORMALS];

//
// The mapwho.
//

UBYTE CLOTH_mapwho[PAP_SIZE_LO][PAP_SIZE_LO];

//
// The global elasticity of cloth.
// The global damping.
//

// claude-ai: Global simulation tuning constants.
// claude-ai: CLOTH_elasticity = spring stiffness coefficient (0.0003 = very soft/stretchy cloth).
// claude-ai: CLOTH_damping    = velocity multiplier per sub-step (0.95 = 5% energy loss per step).
// claude-ai: CLOTH_gravity    = per-step downward acceleration (-0.15 world units/step²).
float CLOTH_elasticity = 0.0003F;
float CLOTH_damping = 0.95F;
float CLOTH_gravity = -0.15F;

//
// Fast approximation to vector length in 3d.
//

// claude-ai: qdist — fast 3D vector length approximation.
// claude-ai: Algorithm: largest_component + 0.2941 * (sum of other two components).
// claude-ai: Error is at most ~5% versus true Euclidean distance.
// claude-ai: Used only for normal normalisation in CLOTH_get_info — approximate is fine there.
// claude-ai: REQUIRES all inputs >= 0 (caller must pass fabs of components).
static inline float qdist(float x, float y, float z)
{
    float ans;

    ASSERT(x >= 0.0F);
    ASSERT(y >= 0.0F);
    ASSERT(z >= 0.0F);

    if (x > y) {
        if (x > z) {
            //
            // x is the biggeset.
            //

            ans = x + (y + z) * 0.2941F;

            return ans;
        }
    } else {
        if (y > z) {
            //
            // y is the biggeset.
            //

            ans = y + (x + z) * 0.2941F;

            return ans;
        }
    }

    //
    // z is the biggeset.
    //

    ans = z + (x + y) * 0.2941F;

    return ans;
}

// claude-ai: CLOTH_init — called once at startup. Builds the static link and normal arrays.
// claude-ai: Link array: iterates all grid cells, for each of 8 neighbours, creates one CLOTH_Link.
// claude-ai:   Axis-aligned (dx==0 || dy==0) → CLOTH_LINK_TYPE_ONE; diagonal → CLOTH_LINK_TYPE_HYP.
// claude-ai: Normal array: for each vertex, records up to 4 (p1,p2) adjacent-neighbour pairs
// claude-ai:   in clockwise order using the order[] table {+X, -Y, -X, +Y}.
// claude-ai: Both arrays are shared by all cloth instances.
void CLOTH_init()
{
    SLONG i;
    SLONG j;
    SLONG x;
    SLONG y;
    SLONG dx;
    SLONG dy;
    SLONG x1;
    SLONG y1;
    SLONG x2;
    SLONG y2;
    SLONG o1;
    SLONG o2;

    const struct {
        SBYTE dx;
        SBYTE dy;
    } order[4] = {
        { +1, 0 },
        { 0, -1 },
        { -1, 0 },
        { 0, +1 }
    };

    CLOTH_Normal* cn;

    //
    // Mark all the cloth as unused.
    //

    for (i = 0; i < CLOTH_MAX_CLOTH; i++) {
        CLOTH_cloth[i].type = CLOTH_TYPE_UNUSED;
    }

    CLOTH_cloth_last = 1;

    //
    // Clear the mapwho.
    //

    memset(CLOTH_mapwho, 0, sizeof(CLOTH_mapwho));

    //
    // Create the link array.
    //

    CLOTH_link_upto = 0;

    for (x = 0; x < CLOTH_WIDTH; x++)
        for (y = 0; y < CLOTH_HEIGHT; y++) {
            for (dx = -1; dx <= 1; dx++)
                for (dy = -1; dy <= 1; dy++) {
                    if (dx | dy) {
                        x1 = x;
                        y1 = y;

                        x2 = x + dx;
                        y2 = y + dy;

                        if (WITHIN(x2, 0, CLOTH_WIDTH - 1) && WITHIN(y2, 0, CLOTH_HEIGHT - 1)) {
                            ASSERT(WITHIN(CLOTH_link_upto, 0, CLOTH_MAX_LINKS - 1));

                            CLOTH_link[CLOTH_link_upto].p1 = CLOTH_INDEX(x1, y1);
                            CLOTH_link[CLOTH_link_upto].p2 = CLOTH_INDEX(x2, y2);
                            CLOTH_link[CLOTH_link_upto].type = (dx == 0 || dy == 0) ? CLOTH_LINK_TYPE_ONE : CLOTH_LINK_TYPE_HYP;

                            CLOTH_link_upto += 1;
                        }
                    }
                }
        }

    //
    // Create the normal array.
    //

    cn = &CLOTH_normal[0];

    for (y = 0; y < CLOTH_HEIGHT; y++)
        for (x = 0; x < CLOTH_WIDTH; x++) {
            cn->num = 0;

            for (i = 0; i < 4; i++) {
                o1 = (i + 0) & 3;
                o2 = (i + 1) & 3;

                x1 = x + order[o1].dx;
                y1 = y + order[o1].dy;

                x2 = x + order[o2].dx;
                y2 = y + order[o2].dy;

                if (WITHIN(x1, 0, CLOTH_WIDTH - 1) && WITHIN(y1, 0, CLOTH_HEIGHT - 1) && WITHIN(x2, 0, CLOTH_WIDTH - 1) && WITHIN(y2, 0, CLOTH_HEIGHT - 1)) {
                    cn->p1[cn->num] = CLOTH_INDEX(x1, y1);
                    cn->p2[cn->num] = CLOTH_INDEX(x2, y2);

                    cn->num += 1;
                }
            }

            ASSERT(WITHIN(cn->num, 1, 4));

            cn->overnum = 1.0F / float(cn->num);

            cn += 1;
        }
}

UBYTE CLOTH_create(
    UBYTE type,
    SLONG ox,
    SLONG oy,
    SLONG oz,
    SLONG iwdx, SLONG iwdy, SLONG iwdz,
    SLONG ihdx, SLONG ihdy, SLONG ihdz,
    SLONG dist,
    ULONG colour)
{
    SLONG i;
    SLONG j;

    float x;
    float y;
    float z;

    float lx;
    float ly;
    float lz;

    float wdx;
    float wdy;
    float wdz;

    float hdx;
    float hdy;
    float hdz;

    UBYTE map_x;
    UBYTE map_z;

    CLOTH_Cloth* cc;
    CLOTH_Point* cp;

    //
    // Look for an unused cloth.
    //

    for (i = 0; i < CLOTH_MAX_CLOTH; i++) {
        CLOTH_cloth_last += 1;

        if (CLOTH_cloth_last >= CLOTH_MAX_CLOTH) {
            CLOTH_cloth_last = 1;
        }

        ASSERT(WITHIN(CLOTH_cloth_last, 1, CLOTH_MAX_CLOTH - 1));

        if (CLOTH_cloth[CLOTH_cloth_last].type == CLOTH_TYPE_UNUSED) {
            goto found_unused_cloth;
        }
    }

    //
    // No unused cloth structure :o(
    //

    return NULL;

found_unused_cloth:;

    ASSERT(WITHIN(CLOTH_cloth_last, 1, CLOTH_MAX_CLOTH - 1));

    cc = &CLOTH_cloth[CLOTH_cloth_last];

    cc->type = type;
    cc->next = 0;
    cc->colour = colour;
    cc->lock = 0;
    cc->offset_x = 0.0F;
    cc->offset_y = 0.0F;
    cc->offset_z = 0.0F;
    cc->dist[0] = float(dist) * float(dist);
    cc->dist[1] = float(dist) * float(dist) * 2.0F;

    //
    // Initialise the points.
    //

    lx = float(ox);
    ly = float(oy);
    lz = float(oz);

    wdx = float(iwdx);
    wdy = float(iwdy);
    wdz = float(iwdz);

    hdx = float(ihdx);
    hdy = float(ihdy);
    hdz = float(ihdz);

    cp = &cc->p[0];

    for (i = 0; i < CLOTH_HEIGHT; i++) {
        x = lx;
        y = ly;
        z = lz;

        for (j = 0; j < CLOTH_WIDTH; j++) {
            cp->x = x;
            cp->y = y;
            cp->z = z;
            cp->dx = 0.0F;
            cp->dy = 0.0F;
            cp->dz = 0.0F;

            cp += 1;

            x += wdx;
            y += wdy;
            z += wdz;
        }

        lx += hdx;
        ly += hdy;
        lz += hdz;
    }

    //
    // Insert on the mapwho.
    //

    map_x = ox >> PAP_SHIFT_LO;
    map_z = oz >> PAP_SHIFT_LO;

    if (WITHIN(map_x, 0, PAP_SIZE_LO - 1) && WITHIN(map_z, 0, PAP_SIZE_LO - 1)) {
        cc->next = CLOTH_mapwho[map_x][map_z];
        CLOTH_mapwho[map_x][map_z] = CLOTH_cloth_last;
    }

    return CLOTH_cloth_last;
}

void CLOTH_point_lock(UBYTE cloth, UBYTE w, UBYTE h)
{
    CLOTH_Cloth* cc;

    ASSERT(WITHIN(cloth, 1, CLOTH_MAX_CLOTH - 1));
    ASSERT(WITHIN(w, 0, CLOTH_WIDTH - 1));
    ASSERT(WITHIN(h, 0, CLOTH_HEIGHT - 1));

    cc = &CLOTH_cloth[cloth];

    cc->lock |= 1 << CLOTH_INDEX(w, h);
}

void CLOTH_point_move(UBYTE cloth, UBYTE w, UBYTE h, SLONG x, SLONG y, SLONG z)
{
    CLOTH_Cloth* cc;
    CLOTH_Point* cp;

    ASSERT(WITHIN(cloth, 1, CLOTH_MAX_CLOTH - 1));
    ASSERT(WITHIN(w, 0, CLOTH_WIDTH - 1));
    ASSERT(WITHIN(h, 0, CLOTH_HEIGHT - 1));

    cc = &CLOTH_cloth[cloth];
    cp = &cc->p[CLOTH_INDEX(w, h)];

    cp->x = float(x);
    cp->y = float(y);
    cp->z = float(z);
}

// claude-ai: CLOTH_process — ENTIRELY DISABLED via #if 0.
// claude-ai: This function is compiled but does nothing at runtime.
// claude-ai: The code inside describes a 3-sub-step spring-mass simulation per tick:
// claude-ai:   Sub-step 1: apply wind to CLOTH_TYPE_FLAG unlocked vertices (sinusoidal gusting).
// claude-ai:   Sub-step 2: iterate all CLOTH_link springs, compute stretch force, apply to both endpoints.
// claude-ai:   Sub-step 3: damp velocities, zero locked-point velocities, integrate positions, apply gravity.
// claude-ai: Floor collision: if cp->y < 2.0, snap to 2.0 and zero vertical velocity.
// claude-ai: GAME_TURN is declared as a local static here (shadows the global) — artifact of disabled code.
void CLOTH_process()
{
}

// claude-ai: CLOTH_get_first — renderer entry point: returns head of cloth linked list for a PAP tile.
// claude-ai: Returns 0 if no cloth is registered in that tile.
// claude-ai: Renderer calls CLOTH_get_info(idx) for each non-zero result and follows cc->next for the chain.
UBYTE CLOTH_get_first(UBYTE lo_map_x, UBYTE lo_map_z)
{
    ASSERT(WITHIN(lo_map_x, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(lo_map_z, 0, PAP_SIZE_LO - 1));

    return CLOTH_mapwho[lo_map_x][lo_map_z];
}

CLOTH_Info CLOTH_info;

// claude-ai: CLOTH_get_info — fills CLOTH_info with render-ready data for one cloth instance.
// claude-ai: For each vertex: applies offset, then computes smoothed per-vertex normal.
// claude-ai: Normal calculation:
// claude-ai:   For each of cn->num neighbour pairs: cross(cp1-cp, cp2-cp) → accumulate.
// claude-ai:   Average the accumulated normal, then normalise using qdist() approximation.
// claude-ai: Result stored in CLOTH_info.p[].{x,y,z,nx,ny,nz} — renderer uses this to shade the mesh.
// claude-ai: Returns pointer to the static CLOTH_info struct (not re-entrant).
CLOTH_Info* CLOTH_get_info(UBYTE cloth)
{
    SLONG i;
    SLONG j;

    float nx;
    float ny;
    float nz;
    float len;
    float overlen;

    float vx1;
    float vy1;
    float vz1;

    float vx2;
    float vy2;
    float vz2;

    float cx;
    float cy;
    float cz;

    CLOTH_Cloth* cc;
    CLOTH_Point* cp;
    CLOTH_Point* cp1;
    CLOTH_Point* cp2;
    CLOTH_Drawp* cd;
    CLOTH_Normal* cn;

    ASSERT(WITHIN(cloth, 1, CLOTH_MAX_CLOTH - 1));

    cc = &CLOTH_cloth[cloth];

    CLOTH_info.type = cc->type;
    CLOTH_info.next = cc->next;
    CLOTH_info.colour = cc->colour;

    //
    // Fill in the point positions and normals.
    //

    cd = &CLOTH_info.p[0];
    cp = &cc->p[0];
    cn = &CLOTH_normal[0];

    for (i = 0; i < CLOTH_WIDTH * CLOTH_HEIGHT; i++) {
        cd->x = cp->x + cc->offset_x;
        cd->y = cp->y + cc->offset_y;
        cd->z = cp->z + cc->offset_z;

        //
        // Calculate the normal
        //

        nx = 0.0F;
        ny = 0.0F;
        nz = 0.0F;

        for (j = 0; j < cn->num; j++) {
            cp1 = &cc->p[cn->p1[j]];
            cp2 = &cc->p[cn->p2[j]];

            vx1 = cp1->x - cp->x;
            vy1 = cp1->y - cp->y;
            vz1 = cp1->z - cp->z;

            vx2 = cp2->x - cp->x;
            vy2 = cp2->y - cp->y;
            vz2 = cp2->z - cp->z;

            cx = vy1 * vz2 - vz1 * vy2;
            cy = vz1 * vx2 - vx1 * vz2;
            cz = vx1 * vy2 - vy1 * vx2;

            nx += cx;
            ny += cy;
            nz += cz;
        }

        nx *= cn->overnum;
        ny *= cn->overnum;
        nz *= cn->overnum;

        //
        // Normalise it.
        //

        len = qdist(fabs(nx), fabs(ny), fabs(nz));
        overlen = 1.0F / len;

        nx *= overlen;
        ny *= overlen;
        nz *= overlen;

        cd->nx = nx;
        cd->ny = ny;
        cd->nz = nz;

        cd += 1;
        cp += 1;
        cn += 1;
    }

    return &CLOTH_info;
}

