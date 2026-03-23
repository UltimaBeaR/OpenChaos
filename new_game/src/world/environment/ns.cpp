// Sewer/cavern subsystem — chunk 1: init, cache init, vertex/face scratch, floors, wallstrip helpers.
// See ns.h for the full sewer system API.

#include "game.h"
#include "world/environment/ns.h"
#include "world/environment/ns_globals.h"
#include "core/heap.h"
#include "world/map/pap.h"
#include "engine/physics/collide.h"

// Internal constants are defined in ns_globals.h (shared with remaining old ns.cpp chunks).

// Forward declarations for internal helpers defined later in this file.
// uc_orig: NS_cache_create_wallstrip (fallen/Source/ns.cpp)
void NS_cache_create_wallstrip(SLONG px1, SLONG pz1, SLONG px2, SLONG pz2, SLONG bot, SLONG ty1, SLONG ty2, SLONG shared, SLONG norm);
// uc_orig: NS_cache_create_extra_bit_left (fallen/Source/ns.cpp)
void NS_cache_create_extra_bit_left(SLONG px1, SLONG pz1, SLONG px2, SLONG pz2, SLONG bottom, SLONG ty1, SLONG ty2, SLONG norm);
// uc_orig: NS_cache_create_extra_bit_right (fallen/Source/ns.cpp)
void NS_cache_create_extra_bit_right(SLONG px1, SLONG pz1, SLONG px2, SLONG pz2, SLONG bottom, SLONG ty1, SLONG ty2, SLONG norm);
// uc_orig: NS_cache_create_walls (fallen/Source/ns.cpp)
void NS_cache_create_walls(UBYTE mx, UBYTE mz);
// uc_orig: NS_cache_create_curve_sewer (fallen/Source/ns.cpp)
void NS_cache_create_curve_sewer(SLONG sx, SLONG sz);
// uc_orig: NS_cache_create_curve_top (fallen/Source/ns.cpp)
void NS_cache_create_curve_top(SLONG sx, SLONG sz);
// uc_orig: NS_cache_create_curves (fallen/Source/ns.cpp)
void NS_cache_create_curves(UBYTE mx, UBYTE mz);
// uc_orig: NS_cache_create_falls (fallen/Source/ns.cpp)
void NS_cache_create_falls(UBYTE mx, UBYTE mz, NS_Cache* nc);
// uc_orig: NS_cache_create_grates (fallen/Source/ns.cpp)
void NS_cache_create_grates(UBYTE mx, UBYTE mz);

// uc_orig: NS_init (fallen/Source/ns.cpp)
void NS_init()
{
    SLONG i;
    SLONG x;
    SLONG z;
    NS_Hi* nh;

    memset((UBYTE*)NS_hi, 0, sizeof(NS_hi));
    memset((UBYTE*)NS_lo, 0, sizeof(NS_lo));

    // Fill every hi-res cell with solid rock.
    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            nh = &NS_hi[x][z];
            nh->packed = NS_HI_TYPE_ROCK;
            nh->bot = 0x100 - 8 * 2;
        }

    // Initialise the sewer-things free list.
    NS_st_free = 1;
    for (i = 1; i < NS_MAX_STS - 1; i++) {
        NS_st[i].type = NS_ST_TYPE_UNUSED;
        NS_st[i].next = i + 1;
    }
    NS_st[NS_MAX_STS - 1].next = NULL;
}

// uc_orig: NS_precalculate (fallen/Source/ns.cpp)
void NS_precalculate()
{
    SLONG i;
    SLONG j;
    SLONG x;
    SLONG z;
    SLONG top;
    SLONG num;
    SLONG used;
    SLONG above;
    SLONG nx;
    SLONG nz;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG px;
    SLONG pz;
    SLONG sx;
    SLONG sz;
    SLONG px1;
    SLONG pz1;
    SLONG px2;
    SLONG pz2;
    ULONG flag;
    SLONG curve;
    NS_Hi* nh;
    NS_Hi* nh2;

    // Clear TOPUSED and LOCKTOP flags; align bot to mapsquare boundary; clear water from rock.
    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            nh = &NS_hi[x][z];
            nh->packed &= ~NS_HI_FLAG_TOPUSED;
            nh->packed &= ~NS_HI_FLAG_LOCKTOP;
            nh->bot &= 0xf8;
            if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
                nh->water = 0;
            }
            // Grates have water 1/8 mapsquare below their floor.
            if (nh->packed & NS_HI_FLAG_GRATE) {
                ASSERT(nh->bot >= 16);
                nh->water = nh->bot - 8;
            }
        }

    // Mark all corner points shared with a rock cell as TOPUSED.
    for (x = 1; x < PAP_SIZE_HI - 1; x++)
        for (z = 1; z < PAP_SIZE_HI - 1; z++) {
            nh = &NS_hi[x][z];
            if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
                for (i = 0; i < 4; i++) {
                    px = x + (i & 1);
                    pz = z + (i >> 1);
                    ASSERT(WITHIN(px, 0, PAP_SIZE_HI - 1));
                    ASSERT(WITHIN(pz, 0, PAP_SIZE_HI - 1));
                    nh2 = &NS_hi[px][pz];
                    nh2->packed |= NS_HI_FLAG_TOPUSED;
                }
            }
        }

    // Clamp wall heights to [8, 32] units above the adjacent sewer floor.
    for (x = 1; x < PAP_SIZE_HI - 1; x++)
        for (z = 1; z < PAP_SIZE_HI - 1; z++) {
            nh = &NS_hi[x][z];
            if (NS_HI_TYPE(nh) == NS_HI_TYPE_STONE) {
                for (dx = 0; dx <= 1; dx++)
                    for (dz = 0; dz <= 1; dz++) {
                        nx = x + dx;
                        nz = z + dz;
                        nh2 = &NS_hi[nx][nz];
                        if (nh2->packed & NS_HI_FLAG_TOPUSED) {
                            dy = nh2->top;
                            dy -= nh->bot;
                            if (dy > 32) {
                                dy = 30 + (rand() & 0x3);
                                nh2->packed |= NS_HI_FLAG_LOCKTOP;
                            }
                            if (dy < 8) {
                                dy = 8;
                                nh2->packed |= NS_HI_FLAG_LOCKTOP;
                            }
                            top = nh->bot + dy;
                            if (top > 255) {
                                top = 255;
                            }
                            nh2->top = top;
                        }
                    }
            }
        }

    // Lock the top heights of points on sewer edges to sewer.bot + 8.
    for (x = 1; x < PAP_SIZE_HI - 1; x++)
        for (z = 1; z < PAP_SIZE_HI - 1; z++) {
            nh = &NS_hi[x][z];
            if (NS_HI_TYPE(nh) == NS_HI_TYPE_SEWER) {
                top = nh->bot + 8;
                if (top > 255) {
                    top = 255;
                }

                const struct
                {
                    SBYTE dsx;
                    SBYTE dsz;
                    SBYTE dpx1;
                    SBYTE dpz1;
                    SBYTE dpx2;
                    SBYTE dpz2;
                } neighbour[4] = {
                    { -1, 0, 0, 0, 0, 1 },
                    { +1, 0, 1, 0, 1, 1 },
                    { 0, -1, 0, 0, 1, 0 },
                    { 0, +1, 0, 1, 1, 1 }
                };

                for (i = 0; i < 4; i++) {
                    sx = x + neighbour[i].dsx;
                    sz = z + neighbour[i].dsz;
                    ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
                    ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));
                    nh2 = &NS_hi[sx][sz];
                    if (NS_HI_TYPE(nh2) != NS_HI_TYPE_SEWER) {
                        px1 = x + neighbour[i].dpx1;
                        pz1 = z + neighbour[i].dpz1;
                        px2 = x + neighbour[i].dpx2;
                        pz2 = z + neighbour[i].dpz2;
                        ASSERT(WITHIN(px1, 0, PAP_SIZE_HI - 1));
                        ASSERT(WITHIN(pz1, 0, PAP_SIZE_HI - 1));
                        ASSERT(WITHIN(px2, 0, PAP_SIZE_HI - 1));
                        ASSERT(WITHIN(pz2, 0, PAP_SIZE_HI - 1));
                        NS_hi[px1][pz1].packed |= NS_HI_FLAG_LOCKTOP;
                        NS_hi[px2][pz2].packed |= NS_HI_FLAG_LOCKTOP;
                        NS_hi[px1][pz1].top = top;
                        NS_hi[px2][pz2].top = top;
                    }
                }
            }
        }

    // Smooth top heights using random neighbourhood averaging.
// uc_orig: NS_EVEN_OUT_INNER (fallen/Source/ns.cpp)
#define NS_EVEN_OUT_INNER 3
// uc_orig: NS_EVEN_OUT_OUTER (fallen/Source/ns.cpp)
#define NS_EVEN_OUT_OUTER 5

    const static SLONG jitter[8] = { -2, -1, -1, 0, 0, +1, +1, +2 };

    for (i = 0; i < NS_EVEN_OUT_OUTER; i++) {
        for (x = 1; x < PAP_SIZE_HI - 1; x++)
            for (z = 1; z < PAP_SIZE_HI - 1; z++) {
                nh = &NS_hi[x][z];
                if ((nh->packed & NS_HI_FLAG_TOPUSED) && !(nh->packed & NS_HI_FLAG_LOCKTOP)) {
                    top = nh->top;
                    num = 1;
                    for (j = 0; j < NS_EVEN_OUT_INNER; j++) {
                        dx = jitter[rand() & 0x7];
                        dz = jitter[rand() & 0x7];
                        px = x + dx;
                        pz = z + dz;
                        if (WITHIN(px, 1, PAP_SIZE_HI - 1) && WITHIN(pz, 1, PAP_SIZE_HI - 1)) {
                            nh2 = &NS_hi[px][pz];
                            if (nh2->packed & NS_HI_FLAG_TOPUSED) {
                                top += nh2->top;
                                num += 1;
                            }
                        }
                    }
                    nh->top = top / num;
                }
            }
    }

    /*
    // Randomise top heights slightly. (Original comment: disabled.)
    for (x = 1; x < PAP_SIZE_HI - 1; x++)
    for (z = 1; z < PAP_SIZE_HI - 1; z++)
    {
        nh = &NS_hi[x][z];
        if (!(nh->packed & NS_HI_FLAG_LOCKTOP) && (nh->packed & NS_HI_FLAG_TOPUSED))
        {
            top = nh->top;
            top += rand() & 0x3;
            top -= 2;
            if (top > 255) { top = 255; }
            nh->top = top;
        }
    }
    */

    // Convert rock cells adjacent to sewers into NS_HI_TYPE_CURVE.
// uc_orig: FLAG_XS (fallen/Source/ns.cpp)
#define FLAG_XS (1 << 0)
// uc_orig: FLAG_XL (fallen/Source/ns.cpp)
#define FLAG_XL (1 << 1)
// uc_orig: FLAG_ZS (fallen/Source/ns.cpp)
#define FLAG_ZS (1 << 2)
// uc_orig: FLAG_ZL (fallen/Source/ns.cpp)
#define FLAG_ZL (1 << 3)

    for (x = 1; x < PAP_SIZE_HI - 1; x++)
        for (z = 1; z < PAP_SIZE_HI - 1; z++) {
            nh = &NS_hi[x][z];
            if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
                flag = 0;
                if (NS_HI_TYPE(&NS_hi[x + 1][z]) == NS_HI_TYPE_SEWER) flag |= FLAG_XS;
                if (NS_HI_TYPE(&NS_hi[x - 1][z]) == NS_HI_TYPE_SEWER) flag |= FLAG_XL;
                if (NS_HI_TYPE(&NS_hi[x][z + 1]) == NS_HI_TYPE_SEWER) flag |= FLAG_ZS;
                if (NS_HI_TYPE(&NS_hi[x][z - 1]) == NS_HI_TYPE_SEWER) flag |= FLAG_ZL;

                curve = -1;
                switch (flag) {
                case FLAG_XS:          curve = NS_HI_CURVE_XS;  break;
                case FLAG_XL:          curve = NS_HI_CURVE_XL;  break;
                case FLAG_ZS:          curve = NS_HI_CURVE_ZS;  break;
                case FLAG_ZL:          curve = NS_HI_CURVE_ZL;  break;
                case FLAG_XS | FLAG_ZS: curve = NS_HI_CURVE_ASS; break;
                case FLAG_XL | FLAG_ZS: curve = NS_HI_CURVE_ALS; break;
                case FLAG_XS | FLAG_ZL: curve = NS_HI_CURVE_ASL; break;
                case FLAG_XL | FLAG_ZL: curve = NS_HI_CURVE_ALL; break;
                }

                if (curve == -1) {
                    // Check diagonal neighbours for obtuse corners.
                    if (NS_HI_TYPE(&NS_hi[x + 1][z + 1]) == NS_HI_TYPE_SEWER) curve = NS_HI_CURVE_OSS;
                    if (NS_HI_TYPE(&NS_hi[x - 1][z + 1]) == NS_HI_TYPE_SEWER) curve = NS_HI_CURVE_OLS;
                    if (NS_HI_TYPE(&NS_hi[x + 1][z - 1]) == NS_HI_TYPE_SEWER) curve = NS_HI_CURVE_OSL;
                    if (NS_HI_TYPE(&NS_hi[x - 1][z - 1]) == NS_HI_TYPE_SEWER) curve = NS_HI_CURVE_OLL;
                }

                if (curve != -1) {
                    NS_HI_TYPE_SET(nh, NS_HI_TYPE_CURVE);
                    nh->water = curve;

                    // Copy bot height from the adjacent sewer cell.
                    for (dx = -1; dx <= 1; dx++)
                        for (dz = -1; dz <= 1; dz++) {
                            nh2 = &NS_hi[x + dx][z + dz];
                            if (NS_HI_TYPE(nh2) == NS_HI_TYPE_SEWER) {
                                nh->bot = nh2->bot;
                                goto found_adjacent_sewer_square;
                            }
                        }

                    ASSERT(0);

                found_adjacent_sewer_square:;
                }
            }
        }
}

// uc_orig: NS_cache_init (fallen/Source/ns.cpp)
void NS_cache_init()
{
    SLONG i;
    SLONG x;
    SLONG z;

    HEAP_init();

    // Build the cache free list (entries 1..NS_MAX_CACHES-1).
    NS_cache_free = 1;
    for (i = 1; i < NS_MAX_CACHES - 1; i++) {
        NS_cache[i].used = UC_FALSE;
        NS_cache[i].next = i + 1;
    }
    NS_cache[NS_MAX_CACHES - 1].next = NULL;

    // Mark all lo-res cells as uncached.
    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            NS_lo[x][z].cache = NULL;
        }

    // Build waterfall free list.
    NS_fall_free = 1;
    for (i = 1; i < NS_MAX_FALLS - 1; i++) {
        NS_fall[i].next = i + 1;
    }
    NS_fall[NS_MAX_FALLS - 1].next = NULL;
}

// Adds a lit vertex to the scratch buffer.
// x,z are world coords (fixed-8 per hi-res cell); y is in NS height units (1/8 mapsquare from -32).
// norm selects the face direction for lighting dot-product calculation.
// uc_orig: NS_add_point (fallen/Source/ns.cpp)
void NS_add_point(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG norm)
{
    SLONG i;
    SLONG px;
    SLONG py;
    SLONG pz;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG bright;
    SLONG light;
    SLONG dprod;
    NS_Slight* nss;

    ASSERT(WITHIN(NS_scratch_point_upto, 0, NS_MAX_SCRATCH_POINTS - 1));

    // Convert from world coords to scratch-relative coords (origin at mapsquare bottom-left).
    px = x - NS_scratch_origin_x >> 3;
    pz = z - NS_scratch_origin_z >> 3;
    py = y;

    ASSERT(WITHIN(px, 0, 128));
    ASSERT(WITHIN(pz, 0, 128));
    ASSERT(WITHIN(py, 0, 255));

    if (norm == NS_NORM_BLACK) {
        bright = 0;
    } else if (norm == NS_NORM_GREY) {
        bright = 32;
    } else {
        // Base ambient varies by facing direction.
        bright = 192 - (norm << 3);

        // Accumulate contribution from each local light source.
        for (i = 0; i < NS_slight_upto; i++) {
            nss = &NS_slight[i];
            dx = nss->x - px;
            dy = nss->y - py << 2;
            dz = nss->z - pz;
            dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

            switch (norm) {
            case NS_NORM_XL:
                if (dx <= 0) continue;
                else dprod = dx * 256 / dist;
                break;
            case NS_NORM_XS:
                if (dx >= 0) continue;
                else dprod = -dx * 256 / dist;
                break;
            case NS_NORM_ZL:
                if (dz <= 0) continue;
                else dprod = dz * 256 / dist;
                break;
            case NS_NORM_ZS:
                if (dz >= 0) continue;
                else dprod = -dz * 256 / dist;
                break;
            case NS_NORM_YL:
                if (dy <= 0) continue;
                else dprod = dy * 256 / dist;
                break;
            case NS_NORM_DUNNO:
                dprod = 200;
                break;
            default:
                ASSERT(0);
                break;
            }

            ASSERT(dprod >= 0);

            light = 256 - (dist << 1);
            if (light > 0) {
                light = light * dprod >> 8;
                bright += light;
            }
        }

        SATURATE(bright, 50, 255);
    }

    NS_scratch_point[NS_scratch_point_upto].x = px;
    NS_scratch_point[NS_scratch_point_upto].y = py;
    NS_scratch_point[NS_scratch_point_upto].z = pz;
    NS_scratch_point[NS_scratch_point_upto].bright = bright;
    NS_scratch_point_upto += 1;
}

// Appends a quad face (4 point indices + page + texture) to the scratch face buffer.
// uc_orig: NS_add_face (fallen/Source/ns.cpp)
void NS_add_face(
    SLONG p[4],
    UBYTE page,
    UBYTE texture)
{
    ASSERT(WITHIN(NS_scratch_face_upto, 0, NS_MAX_SCRATCH_FACES - 1));
    ASSERT(WITHIN(p[0], 0, NS_scratch_point_upto - 1));
    ASSERT(WITHIN(p[1], 0, NS_scratch_point_upto - 1));
    ASSERT(WITHIN(p[2], 0, NS_scratch_point_upto - 1));
    ASSERT(WITHIN(p[3], 0, NS_scratch_point_upto - 1));
    ASSERT(WITHIN(page, 0, NS_PAGE_NUMBER - 1));
    ASSERT(WITHIN(texture, 0, NS_MAX_TEXTURES - 1));

    NS_scratch_face[NS_scratch_face_upto].p[0] = p[0];
    NS_scratch_face[NS_scratch_face_upto].p[1] = p[1];
    NS_scratch_face[NS_scratch_face_upto].p[2] = p[2];
    NS_scratch_face[NS_scratch_face_upto].p[3] = p[3];
    NS_scratch_face[NS_scratch_face_upto].page = page;
    NS_scratch_face[NS_scratch_face_upto].texture = texture;
    NS_scratch_face_upto += 1;
}

// Generates floor and ceiling geometry for a 4x4 hi-res block (one lo-res mapsquare).
// Top faces (rock ceiling) are emitted first, then bottom faces (sewer floor) sorted back-to-front.
// uc_orig: NS_cache_create_floors (fallen/Source/ns.cpp)
void NS_cache_create_floors(UBYTE mx, UBYTE mz)
{
    SLONG i;
    SLONG j;
    SLONG x;
    SLONG z;
    SLONG px;
    SLONG py;
    SLONG pz;
    SLONG bx;
    SLONG bz;
    SLONG sx;
    SLONG sz;
    SLONG page;
    SLONG p[4];
    NS_Hi* ns;

    // Emit ceiling (top) points for all 5x5 hi-res corners in this lo-res square.
    UBYTE pindex[5][5];
    for (x = 0; x <= 4; x++)
        for (z = 0; z <= 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;
            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));
            ns = &NS_hi[sx][sz];
            if (ns->packed & NS_HI_FLAG_TOPUSED) {
                px = sx << 8;
                pz = sz << 8;
                py = ns->top;
                pindex[x][z] = NS_scratch_point_upto;
                NS_add_point(px, py, pz, NS_NORM_YL);
            }
        }

    // Emit rock ceiling quads.
    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;
            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));
            ns = &NS_hi[sx][sz];
            if (NS_HI_TYPE(ns) == NS_HI_TYPE_ROCK) {
                p[0] = pindex[x + 0][z + 0];
                p[1] = pindex[x + 1][z + 0];
                p[2] = pindex[x + 0][z + 1];
                p[3] = pindex[x + 1][z + 1];
                ASSERT(p[0] < 25);
                ASSERT(p[1] < 25);
                ASSERT(p[2] < 25);
                ASSERT(p[3] < 25);
                NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_FULL);
            }
        }

    // Collect non-rock, non-nothing, non-curve squares and sort them by height (back-to-front).
    // uc_orig: Bsquare (fallen/Source/ns.cpp)
    typedef struct
    {
        UBYTE x;
        UBYTE z;
        UBYTE bot;
    } Bsquare;

// uc_orig: MAX_BSQUARES (fallen/Source/ns.cpp)
#define MAX_BSQUARES 16
    Bsquare bsquare[MAX_BSQUARES];
    SLONG bsquare_upto = 0;
    SLONG insert;

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;
            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));
            ns = &NS_hi[sx][sz];
            if (NS_HI_TYPE(ns) != NS_HI_TYPE_ROCK && NS_HI_TYPE(ns) != NS_HI_TYPE_NOTHING && NS_HI_TYPE(ns) != NS_HI_TYPE_CURVE) {
                // Skip: water-filled cells (opaque water hides the floor), and bot==0 (floor at -infinity).
                if ((ns->water && !(ns->packed & NS_HI_FLAG_GRATE)) || ns->bot == 0) {
                } else {
                    // Insertion sort by bot height ascending (back-to-front draw order).
                    insert = bsquare_upto;
                    while (1) {
                        if (insert == 0) {
                            bsquare[insert].x = sx;
                            bsquare[insert].z = sz;
                            bsquare[insert].bot = ns->bot;
                            break;
                        }
                        ASSERT(WITHIN(insert - 1, 0, MAX_BSQUARES - 1));
                        ASSERT(WITHIN(insert - 0, 0, MAX_BSQUARES - 1));
                        if (ns->bot >= bsquare[insert - 1].bot) {
                            bsquare[insert].x = sx;
                            bsquare[insert].z = sz;
                            bsquare[insert].bot = ns->bot;
                            break;
                        } else {
                            bsquare[insert] = bsquare[insert - 1];
                        }
                        insert -= 1;
                    }
                    bsquare_upto += 1;
                }
            }
        }

    // Reset point index table for floor pass.
    memset((UBYTE*)pindex, 255, sizeof(pindex));

    // Emit floor geometry in back-to-front order.
    for (i = 0; i < bsquare_upto; i++) {
        sx = bsquare[i].x;
        sz = bsquare[i].z;
        ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
        ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));
        ns = &NS_hi[sx][sz];
        bx = sx - (mx << 2);
        bz = sz - (mz << 2);
        ASSERT(WITHIN(bx, 0, 3));
        ASSERT(WITHIN(bz, 0, 3));

        // Reuse or create corner points.
        for (j = 0; j < 4; j++) {
            x = bx + (j & 1);
            z = bz + (j >> 1);
            if (pindex[x][z] != 255) {
                ASSERT(WITHIN(pindex[x][z], 0, NS_scratch_point_upto - 1));
                if (NS_scratch_point[pindex[x][z]].y == ns->bot) {
                    p[j] = pindex[x][z];
                    goto reusing_an_old_point;
                }
            }
            pindex[x][z] = NS_scratch_point_upto;
            p[j] = NS_scratch_point_upto;
            px = (x << 8) + (mx << PAP_SHIFT_LO);
            pz = (z << 8) + (mz << PAP_SHIFT_LO);
            py = ns->bot;
            NS_add_point(px, py, pz, NS_NORM_YL);
        reusing_an_old_point:;
        }

        page = (ns->packed & NS_HI_FLAG_GRATE) ? NS_PAGE_GRATE : NS_PAGE_STONE;
        NS_add_face(p, page, NS_TEXTURE_FULL);
    }
}

// Searches the scratch point buffer in [NS_search_start, NS_search_end) for an existing point
// at (x,y,z) with the given normal. Creates one if not found. Returns the point index.
// uc_orig: NS_create_wallstrip_point (fallen/Source/ns.cpp)
SLONG NS_create_wallstrip_point(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG norm)
{
    SLONG i;
    SLONG px;
    SLONG py;
    SLONG pz;
    SLONG ans;

    px = (x - NS_scratch_origin_x) >> 3;
    pz = (z - NS_scratch_origin_z) >> 3;
    py = y;

    ASSERT(WITHIN(px, 0, 128));
    ASSERT(WITHIN(pz, 0, 128));
    ASSERT(WITHIN(py, 0, 255));

    for (i = NS_search_start; i < NS_search_end; i++) {
        ASSERT(WITHIN(i, 0, NS_scratch_point_upto - 1));
        if (NS_scratch_point[i].x == px && NS_scratch_point[i].y == py && NS_scratch_point[i].z == pz) {
            return i;
        }
    }

    // Point not found — create it.
    ans = NS_scratch_point_upto;
    NS_add_point(x, y, z, norm);
    return ans;
}

// Overload: defaults to NS_NORM_DUNNO normal direction.
// uc_orig: NS_create_wallstrip_point (fallen/Source/ns.cpp)
SLONG NS_create_wallstrip_point(
    SLONG x,
    SLONG y,
    SLONG z)
{
    return NS_create_wallstrip_point(x, y, z, NS_NORM_DUNNO);
}

// Emits a vertical strip of quads from y=bot up to y=ty1/ty2 along the edge from
// (px1,pz1) to (px2,pz2). The 'shared' index lets adjacent strips reuse boundary points.
// uc_orig: NS_cache_create_wallstrip (fallen/Source/ns.cpp)
void NS_cache_create_wallstrip(
    SLONG px1, SLONG pz1,
    SLONG px2, SLONG pz2,
    SLONG bot,
    SLONG ty1, SLONG ty2,
    SLONG shared,
    SLONG norm)
{
    SLONG last1;
    SLONG last2;
    SLONG now1;
    SLONG now2;
    SLONG py1;
    SLONG py2;
    SLONG p[4];
    SLONG darken_bottom;
    SLONG usenorm;

    ASSERT((bot & 0x7) == 0);

    if (bot == 0) {
        bot = MIN(ty1, ty2);
        bot -= 24;
        bot &= ~7;
        darken_bottom = UC_TRUE;
    } else {
        darken_bottom = UC_FALSE;
    }

    NS_search_start = shared;
    NS_search_end = NS_scratch_point_upto;

    last1 = NS_create_wallstrip_point(px1, bot, pz1, (darken_bottom) ? NS_NORM_BLACK : norm);
    last2 = NS_create_wallstrip_point(px2, bot, pz2, (darken_bottom) ? NS_NORM_BLACK : norm);

    py1 = bot + 8;
    py2 = bot + 8;

    while (1) {
        if (py1 >= ty1 || py2 >= ty2) {
            py1 = ty1;
            py2 = ty2;
        }

        if (darken_bottom) {
            usenorm = NS_NORM_GREY;
            darken_bottom = UC_FALSE;
        } else {
            usenorm = norm;
        }

        now1 = NS_create_wallstrip_point(px1, py1, pz1, usenorm);
        now2 = NS_create_wallstrip_point(px2, py2, pz2, usenorm);

        p[0] = last1;
        p[1] = last2;
        p[2] = now1;
        p[3] = now2;

        NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_FULL);

        if (py1 >= ty1 || py2 >= ty2) {
            return;
        }

        py1 += 8;
        py2 += 8;

        if (py1 > 255) {
            py1 = 255;
        }
        if (py2 > 255) {
            py2 = 255;
        }

        last1 = now1;
        last2 = now2;
    }
}

// Emits the angled cap geometry on the left side of a sewer wall opening.
// uc_orig: NS_cache_create_extra_bit_left (fallen/Source/ns.cpp)
void NS_cache_create_extra_bit_left(
    SLONG px1, SLONG pz1,
    SLONG px2, SLONG pz2,
    SLONG bottom,
    SLONG ty1, SLONG ty2,
    SLONG norm)
{
    SLONG dx = px2 - px1 >> 8;
    SLONG dz = pz2 - pz1 >> 8;
    SLONG pindex[6];
    SLONG p[4];

    pindex[0] = NS_create_wallstrip_point(px1, bottom, pz1, norm);
    pindex[1] = NS_create_wallstrip_point(px1, ty1, pz1, norm);
    pindex[2] = NS_create_wallstrip_point(px1 + dx * 128, bottom + 8, pz1 + dz * 128, norm);
    pindex[3] = NS_create_wallstrip_point(px1 + dx * 192, bottom + 3, pz1 + dz * 192, norm);
    pindex[4] = NS_create_wallstrip_point(px1 + dx * 256, bottom + 2, pz1 + dz * 256, norm);
    pindex[5] = NS_create_wallstrip_point(px1 + dx * 256, bottom, pz1 + dz * 256, norm);

    p[0] = pindex[2];
    p[1] = pindex[1];
    p[2] = pindex[3];
    p[3] = pindex[0];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_EL1);

    p[0] = pindex[4];
    p[1] = pindex[3];
    p[2] = pindex[5];
    p[3] = pindex[0];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_EL2);
}

// Emits the angled cap geometry on the right side of a sewer wall opening.
// uc_orig: NS_cache_create_extra_bit_right (fallen/Source/ns.cpp)
void NS_cache_create_extra_bit_right(
    SLONG px1, SLONG pz1,
    SLONG px2, SLONG pz2,
    SLONG bottom,
    SLONG ty1, SLONG ty2,
    SLONG norm)
{
    SLONG dx = px2 - px1 >> 8;
    SLONG dz = pz2 - pz1 >> 8;
    SLONG pindex[6];
    SLONG p[4];

    pindex[0] = NS_create_wallstrip_point(px2, bottom, pz2, norm);
    pindex[1] = NS_create_wallstrip_point(px2, ty2, pz2, norm);
    pindex[2] = NS_create_wallstrip_point(px2 - dx * 128, bottom + 8, pz2 - dz * 128, norm);
    pindex[3] = NS_create_wallstrip_point(px2 - dx * 192, bottom + 3, pz2 - dz * 192, norm);
    pindex[4] = NS_create_wallstrip_point(px2 - dx * 256, bottom + 2, pz2 - dz * 256, norm);
    pindex[5] = NS_create_wallstrip_point(px2 - dx * 256, bottom, pz2 - dz * 256, norm);

    p[0] = pindex[1];
    p[1] = pindex[2];
    p[2] = pindex[0];
    p[3] = pindex[3];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_ER1);

    p[0] = pindex[3];
    p[1] = pindex[4];
    p[2] = pindex[0];
    p[3] = pindex[5];

    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_ER2);
}

// Generates all vertical wall strips for one lo-res mapsquare (4x4 hi-res cells).
// Handles transitions between sewer/stone cells and between sewer floors at different heights.
// uc_orig: NS_cache_create_walls (fallen/Source/ns.cpp)
void NS_cache_create_walls(UBYTE mx, UBYTE mz)
{
    SLONG x;
    SLONG z;
    SLONG sx;
    SLONG sz;
    SLONG px1, pz1;
    SLONG px2, pz2;
    SLONG ty1;
    SLONG ty2;
    SLONG bot;
    SLONG shared_last;
    SLONG shared_now;
    NS_Hi* nh;
    NS_Hi* nh2;

    // Walls parallel to z-axis (x-facing walls).
    for (x = 0; x < 4; x++) {
        // Left wall strip (from x to x-1).
        shared_last = NS_scratch_point_upto - 1;
        if (shared_last < 0) {
            shared_last = 0;
        }

        for (z = 0; z < 4; z++) {
            shared_now = NS_scratch_point_upto - 1;
            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx - 1][sz];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx << 8;
                pz1 = sz << 8;
                px2 = sx << 8;
                pz2 = sz + 1 << 8;
                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx][sz + 0].top;
                    ty2 = NS_hi[sx][sz + 1].top;
                    NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_XL);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;
                        NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_XL);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            ty1 = NS_hi[sx][sz + 0].top;
                            ty2 = NS_hi[sx][sz + 1].top;

                            if (NS_HI_TYPE(&NS_hi[sx - 1][sz - 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_XL);
                            } else {
                                NS_cache_create_extra_bit_right(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_XL);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }

        // Right wall strip (from x to x+1).
        shared_last = NS_scratch_point_upto - 1;
        if (shared_last < 0) {
            shared_last = 0;
        }

        for (z = 0; z < 4; z++) {
            shared_now = NS_scratch_point_upto - 1;
            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx + 1][sz];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx + 1 << 8;
                pz1 = sz + 1 << 8;
                px2 = sx + 1 << 8;
                pz2 = sz << 8;
                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx + 1][sz + 1].top;
                    ty2 = NS_hi[sx + 1][sz + 0].top;
                    NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_XS);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;
                        NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_XS);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            ty1 = NS_hi[sx + 1][sz + 1].top;
                            ty2 = NS_hi[sx + 1][sz + 0].top;

                            if (NS_HI_TYPE(&NS_hi[sx + 1][sz + 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_XS);
                            } else {
                                NS_cache_create_extra_bit_right(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_XS);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }
    }

    // Walls parallel to x-axis (z-facing walls).
    for (z = 0; z < 4; z++) {
        // Near wall strip (from z to z-1).
        shared_last = NS_scratch_point_upto - 1;
        if (shared_last < 0) {
            shared_last = 0;
        }

        for (x = 0; x < 4; x++) {
            shared_now = NS_scratch_point_upto - 1;
            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx][sz - 1];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx + 1 << 8;
                pz1 = sz << 8;
                px2 = sx << 8;
                pz2 = sz << 8;
                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx + 1][sz].top;
                    ty2 = NS_hi[sx + 0][sz].top;
                    NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_ZL);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;
                        NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_ZL);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            ty1 = NS_hi[sx + 1][sz].top;
                            ty2 = NS_hi[sx + 0][sz].top;

                            if (NS_HI_TYPE(&NS_hi[sx + 1][sz - 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_ZL);
                            } else {
                                NS_cache_create_extra_bit_right(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_ZL);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }

        // Far wall strip (from z to z+1).
        shared_last = NS_scratch_point_upto - 1;
        if (shared_last < 0) {
            shared_last = 0;
        }

        for (x = 0; x < 4; x++) {
            shared_now = NS_scratch_point_upto - 1;
            if (shared_now < 0) {
                shared_now = 0;
            }

            sx = (mx << 2) + x;
            sz = (mz << 2) + z;

            ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];
            nh2 = &NS_hi[sx][sz + 1];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE) {
                px1 = sx << 8;
                pz1 = sz + 1 << 8;
                px2 = sx + 1 << 8;
                pz2 = sz + 1 << 8;
                bot = nh->bot;

                if (NS_HI_TYPE(nh2) == NS_HI_TYPE_ROCK) {
                    ty1 = NS_hi[sx + 0][sz + 1].top;
                    ty2 = NS_hi[sx + 1][sz + 1].top;
                    NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_ZS);
                } else {
                    if (nh2->bot > nh->bot) {
                        ty1 = nh2->bot;
                        ty2 = nh2->bot;
                        NS_cache_create_wallstrip(px1, pz1, px2, pz2, bot, ty1, ty2, shared_last, NS_NORM_ZS);

                        if (NS_HI_TYPE(nh2) == NS_HI_TYPE_CURVE) {
                            ty1 = NS_hi[sx + 0][sz + 1].top;
                            ty2 = NS_hi[sx + 1][sz + 1].top;

                            if (NS_HI_TYPE(&NS_hi[sx - 1][sz + 1]) == NS_HI_TYPE_ROCK) {
                                NS_cache_create_extra_bit_left(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_ZS);
                            } else {
                                NS_cache_create_extra_bit_right(px1, pz1, px2, pz2, nh2->bot, ty1, ty2, NS_NORM_ZS);
                            }
                        }
                    }
                }
            }

            shared_last = shared_now;
        }
    }
}

// Emits the curved sewer wall geometry for a single CURVE hi-res cell.
// The curve type stored in nh->water selects between flat, inside-corner, and outside-corner shapes.
// uc_orig: NS_cache_create_curve_sewer (fallen/Source/ns.cpp)
void NS_cache_create_curve_sewer(SLONG sx, SLONG sz)
{
    SLONG px1;
    SLONG pz1;
    SLONG px2;
    SLONG pz2;
    SLONG dx;
    SLONG dz;
    SLONG dx1;
    SLONG dz1;
    SLONG dx2;
    SLONG dz2;
    SLONG px;
    SLONG py;
    SLONG pz;
    SLONG p[4];
    SLONG curve;
    NS_Hi* nh;
    UBYTE pindex[16];

    ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

    nh = &NS_hi[sx][sz];
    curve = nh->water;

    if (curve < 4) {
        // Flat sewer wall curve.
        switch (curve) {
        case NS_HI_CURVE_XS:
            px1 = sx + 1 << 8;
            pz1 = sz + 0 << 8;
            px2 = sx + 1 << 8;
            pz2 = sz + 1 << 8;
            dx = -256;
            dz = 0;
            break;

        case NS_HI_CURVE_XL:
            px1 = sx + 0 << 8;
            pz1 = sz + 1 << 8;
            px2 = sx + 0 << 8;
            pz2 = sz + 0 << 8;
            dx = +256;
            dz = 0;
            break;

        case NS_HI_CURVE_ZS:
            px1 = sx + 1 << 8;
            pz1 = sz + 1 << 8;
            px2 = sx + 0 << 8;
            pz2 = sz + 1 << 8;
            dx = 0;
            dz = -256;
            break;

        case NS_HI_CURVE_ZL:
            px1 = sx + 0 << 8;
            pz1 = sz + 0 << 8;
            px2 = sx + 1 << 8;
            pz2 = sz + 0 << 8;
            dx = 0;
            dz = +256;
            break;

        default:
            ASSERT(0);
            break;
        }

        pindex[0] = NS_create_wallstrip_point(px1, nh->bot + 0, pz1);
        pindex[1] = NS_create_wallstrip_point(px2, nh->bot + 0, pz2);
        pindex[2] = NS_create_wallstrip_point(px1, nh->bot + 2, pz1);
        pindex[3] = NS_create_wallstrip_point(px2, nh->bot + 2, pz2);

        px1 += dx >> 2;
        pz1 += dz >> 2;
        px2 += dx >> 2;
        pz2 += dz >> 2;

        pindex[4] = NS_create_wallstrip_point(px1, nh->bot + 3, pz1);
        pindex[5] = NS_create_wallstrip_point(px2, nh->bot + 3, pz2);

        px1 += dx >> 2;
        pz1 += dz >> 2;
        px2 += dx >> 2;
        pz2 += dz >> 2;

        pindex[6] = NS_create_wallstrip_point(px1, nh->bot + 8, pz1);
        pindex[7] = NS_create_wallstrip_point(px2, nh->bot + 8, pz2);

        p[0] = pindex[0]; p[1] = pindex[1]; p[2] = pindex[2]; p[3] = pindex[3];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP1);

        p[0] = pindex[2]; p[1] = pindex[3]; p[2] = pindex[4]; p[3] = pindex[5];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP2);

        p[0] = pindex[4]; p[1] = pindex[5]; p[2] = pindex[6]; p[3] = pindex[7];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP3);

    } else if (curve < 8) {
        // Inside corner curve.
        switch (curve) {
        case NS_HI_CURVE_ASS:
            px = sx + 1 << 8; pz = sz + 1 << 8;
            dx1 = 0;  dz1 = -1; dx2 = -1; dz2 = 0;
            break;

        case NS_HI_CURVE_ALS:
            px = sx + 0 << 8; pz = sz + 1 << 8;
            dx1 = +1; dz1 = 0;  dx2 = 0;  dz2 = -1;
            break;

        case NS_HI_CURVE_ASL:
            px = sx + 1 << 8; pz = sz + 0 << 8;
            dx1 = -1; dz1 = 0;  dx2 = 0;  dz2 = +1;
            break;

        case NS_HI_CURVE_ALL:
            px = sx + 0 << 8; pz = sz + 0 << 8;
            dx1 = 0;  dz1 = +1; dx2 = +1; dz2 = 0;
            break;

        default:
            ASSERT(0);
            break;
        }

        pindex[0]  = NS_create_wallstrip_point(px + dx1*0   + dx2*0,   nh->bot+0, pz + dz1*0   + dz2*0);
        pindex[1]  = NS_create_wallstrip_point(px + dx1*0   + dx2*0,   nh->bot+2, pz + dz1*0   + dz2*0);
        pindex[2]  = NS_create_wallstrip_point(px + dx1*64  + dx2*64,  nh->bot+3, pz + dz1*64  + dz2*64);
        pindex[3]  = NS_create_wallstrip_point(px + dx1*160 + dx2*160, nh->bot+8, pz + dz1*160 + dz2*160);

        pindex[4]  = NS_create_wallstrip_point(px + dx1*0   + dx2*256, nh->bot+0, pz + dz1*0   + dz2*256);
        pindex[5]  = NS_create_wallstrip_point(px + dx1*0   + dx2*256, nh->bot+2, pz + dz1*0   + dz2*256);
        pindex[6]  = NS_create_wallstrip_point(px + dx1*64  + dx2*256, nh->bot+3, pz + dz1*64  + dz2*256);
        pindex[7]  = NS_create_wallstrip_point(px + dx1*128 + dx2*256, nh->bot+8, pz + dz1*128 + dz2*256);

        pindex[8]  = NS_create_wallstrip_point(px + dx1*256 + dx2*0,   nh->bot+0, pz + dz1*256 + dz2*0);
        pindex[9]  = NS_create_wallstrip_point(px + dx1*256 + dx2*0,   nh->bot+2, pz + dz1*256 + dz2*0);
        pindex[10] = NS_create_wallstrip_point(px + dx1*256 + dx2*64,  nh->bot+3, pz + dz1*256 + dz2*64);
        pindex[11] = NS_create_wallstrip_point(px + dx1*256 + dx2*128, nh->bot+8, pz + dz1*256 + dz2*128);

        p[0] = pindex[8];  p[1] = pindex[0];  p[2] = pindex[9];  p[3] = pindex[1];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP1);

        p[0] = pindex[0];  p[1] = pindex[4];  p[2] = pindex[1];  p[3] = pindex[5];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP1);

        p[0] = pindex[9];  p[1] = pindex[1];  p[2] = pindex[10]; p[3] = pindex[2];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP4);

        p[0] = pindex[1];  p[1] = pindex[5];  p[2] = pindex[2];  p[3] = pindex[6];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP5);

        p[0] = pindex[10]; p[1] = pindex[2];  p[2] = pindex[11]; p[3] = pindex[3];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP6);

        p[0] = pindex[2];  p[1] = pindex[6];  p[2] = pindex[3];  p[3] = pindex[7];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP7);

    } else {
        // Outside corner curve.
        switch (curve) {
        case NS_HI_CURVE_OSS:
            px = sx + 1 << 8; pz = sz + 1 << 8;
            dx1 = 0;  dz1 = -1; dx2 = -1; dz2 = 0;
            break;

        case NS_HI_CURVE_OLS:
            px = sx + 0 << 8; pz = sz + 1 << 8;
            dx1 = +1; dz1 = 0;  dx2 = 0;  dz2 = -1;
            break;

        case NS_HI_CURVE_OSL:
            px = sx + 1 << 8; pz = sz + 0 << 8;
            dx1 = -1; dz1 = 0;  dx2 = 0;  dz2 = +1;
            break;

        case NS_HI_CURVE_OLL:
            px = sx + 0 << 8; pz = sz + 0 << 8;
            dx1 = 0;  dz1 = +1; dx2 = +1; dz2 = 0;
            break;

        default:
            ASSERT(0);
            break;
        }

        pindex[0] = NS_create_wallstrip_point(px + dx1*0  + dx2*0,  nh->bot+2, pz + dz1*0  + dz2*0);
        pindex[1] = NS_create_wallstrip_point(px + dx1*64 + dx2*64, nh->bot+3, pz + dz1*64 + dz2*64);
        pindex[2] = NS_create_wallstrip_point(px + dx1*96 + dx2*96, nh->bot+8, pz + dz1*96 + dz2*96);
        pindex[3] = NS_create_wallstrip_point(px + dx1*64 + dx2*0,  nh->bot+3, pz + dz1*64 + dz2*0);
        pindex[4] = NS_create_wallstrip_point(px + dx1*128+ dx2*0,  nh->bot+8, pz + dz1*128+ dz2*0);
        pindex[5] = NS_create_wallstrip_point(px + dx1*0  + dx2*64, nh->bot+3, pz + dz1*0  + dz2*64);
        pindex[6] = NS_create_wallstrip_point(px + dx1*0  + dx2*128,nh->bot+8, pz + dz1*0  + dz2*128);

        p[0] = pindex[3]; p[1] = pindex[0]; p[2] = pindex[1]; p[3] = pindex[5];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP8);

        p[0] = pindex[3]; p[1] = pindex[1]; p[2] = pindex[4]; p[3] = pindex[2];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP9);

        p[0] = pindex[1]; p[1] = pindex[5]; p[2] = pindex[2]; p[3] = pindex[6];
        NS_add_face(p, NS_PAGE_SWALL, NS_TEXTURE_HSTRIP10);
    }
}

// Emits the rock ceiling cap for a single CURVE hi-res cell. Each curve type produces a
// different fan shape using the pre-computed top heights from NS_precalculate.
// uc_orig: NS_cache_create_curve_top (fallen/Source/ns.cpp)
void NS_cache_create_curve_top(SLONG sx, SLONG sz)
{
    UBYTE pindex[16];
    UBYTE curve;
    SLONG ox = sx << 8;
    SLONG oz = sz << 8;
    SLONG p[4];
    SLONG px;
    SLONG py;
    SLONG pz;
    NS_Hi* nh;

    ASSERT(WITHIN(sx, 1, PAP_SIZE_HI - 2));
    ASSERT(WITHIN(sz, 1, PAP_SIZE_HI - 2));

    nh = &NS_hi[sx][sz];
    ASSERT(NS_HI_TYPE(nh) == NS_HI_TYPE_CURVE);

    curve = nh->water;

    switch (curve) {
    case NS_HI_CURVE_XS:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT1);
        break;

    case NS_HI_CURVE_XL:
        pindex[0] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT2);
        break;

    case NS_HI_CURVE_ZS:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+128,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+128,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT3);
        break;

    case NS_HI_CURVE_ZL:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz+128,   NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz+128,   NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT4);
        break;

    case NS_HI_CURVE_ASS:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+128,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+96,    NS_hi[sx+1][sz+1].top, oz+96,    NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT5);
        break;

    case NS_HI_CURVE_ALS:
        pindex[0] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox+160,   NS_hi[sx+0][sz+1].top, oz+96,    NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+128,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT6);
        break;

    case NS_HI_CURVE_ASL:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz+128,   NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+96,    NS_hi[sx+1][sz+0].top, oz+160,   NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT7);
        break;

    case NS_HI_CURVE_ALL:
        pindex[0] = NS_create_wallstrip_point(ox+160,   NS_hi[sx+0][sz+0].top, oz+160,   NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz+128,   NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT8);
        break;

    case NS_HI_CURVE_OSS:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[4] = NS_create_wallstrip_point(ox+160,   NS_hi[sx+1][sz+1].top, oz+160,   NS_NORM_YL);
        pindex[5] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+128,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[4]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT9);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[4]; p[3]=pindex[5];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT10);
        break;

    case NS_HI_CURVE_OLS:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+128,   NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+96,    NS_hi[sx+0][sz+1].top, oz+160,   NS_NORM_YL);
        pindex[4] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[5] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[2]; p[3]=pindex[3];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT11);
        p[0]=pindex[3]; p[1]=pindex[1]; p[2]=pindex[4]; p[3]=pindex[5];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT12);
        break;

    case NS_HI_CURVE_OSL:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz,       NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+1][sz+0].top, oz,       NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox+160,   NS_hi[sx+1][sz+0].top, oz+96,    NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz+128,   NS_NORM_YL);
        pindex[4] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[5] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[4]; p[3]=pindex[2];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT13);
        p[0]=pindex[2]; p[1]=pindex[3]; p[2]=pindex[4]; p[3]=pindex[5];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT14);
        break;

    case NS_HI_CURVE_OLL:
        pindex[0] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+0].top, oz+128,   NS_NORM_YL);
        pindex[1] = NS_create_wallstrip_point(ox+96,    NS_hi[sx+0][sz+0].top, oz+96,    NS_NORM_YL);
        pindex[2] = NS_create_wallstrip_point(ox+128,   NS_hi[sx+0][sz+0].top, oz+0,     NS_NORM_YL);
        pindex[3] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+0].top, oz+0,     NS_NORM_YL);
        pindex[4] = NS_create_wallstrip_point(ox,       NS_hi[sx+0][sz+1].top, oz+256,   NS_NORM_YL);
        pindex[5] = NS_create_wallstrip_point(ox+256,   NS_hi[sx+1][sz+1].top, oz+256,   NS_NORM_YL);
        p[0]=pindex[0]; p[1]=pindex[1]; p[2]=pindex[4]; p[3]=pindex[5];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT15);
        p[0]=pindex[2]; p[1]=pindex[3]; p[2]=pindex[1]; p[3]=pindex[5];
        NS_add_face(p, NS_PAGE_ROCK, NS_TEXTURE_CT16);
        break;
    }
}

// Generates curved sewer wall and curved rock ceiling geometry for a full lo-res mapsquare.
// uc_orig: NS_cache_create_curves (fallen/Source/ns.cpp)
void NS_cache_create_curves(UBYTE mx, UBYTE mz)
{
    SLONG x;
    SLONG z;
    SLONG sx;
    SLONG sz;
    NS_Hi* nh;

    ASSERT(WITHIN(mx, 1, PAP_SIZE_LO - 2));
    ASSERT(WITHIN(mz, 1, PAP_SIZE_LO - 2));

    // Sewer wall curves — allow point reuse across adjacent cells.
    NS_search_start = NS_scratch_point_upto;
    NS_search_end = NS_scratch_point_upto;

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;
            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));
            nh = &NS_hi[sx][sz];
            if (NS_HI_TYPE(nh) == NS_HI_TYPE_CURVE) {
                NS_cache_create_curve_sewer(sx, sz);
                NS_search_end = NS_scratch_point_upto;
            }
        }

    // Rock ceiling caps for curve cells.
    NS_search_start = NS_scratch_point_upto;
    NS_search_end = NS_scratch_point_upto;

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;
            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));
            nh = &NS_hi[sx][sz];
            if (NS_HI_TYPE(nh) == NS_HI_TYPE_CURVE) {
                NS_cache_create_curve_top(sx, sz);
                NS_search_end = NS_scratch_point_upto;
            }
        }
}

// Finds waterfalls — pairs of adjacent sewer cells where one has water above the other's floor —
// and adds NS_Fall entries to the given cache slot.
// uc_orig: NS_cache_create_falls (fallen/Source/ns.cpp)
void NS_cache_create_falls(UBYTE mx, UBYTE mz, NS_Cache* nc)
{
    SLONG i;
    SLONG x;
    SLONG z;
    SLONG sx;
    SLONG sz;
    SLONG dx;
    SLONG dz;
    SLONG nx;
    SLONG nz;
    SLONG fall;
    NS_Hi* nh;
    NS_Hi* nh2;
    NS_Fall* nf;

    ASSERT(WITHIN(mx, 1, PAP_SIZE_LO - 2));
    ASSERT(WITHIN(mz, 1, PAP_SIZE_LO - 2));

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;
            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE && NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK) {
                const struct {
                    SBYTE dx;
                    SBYTE dz;
                } dir[4] = {
                    { +1, 0 },
                    { -1, 0 },
                    { 0, +1 },
                    { 0, -1 }
                };

                for (i = 0; i < 4; i++) {
                    dx = dir[i].dx;
                    dz = dir[i].dz;
                    nx = sx + dx;
                    nz = sz + dz;

                    ASSERT(WITHIN(nx, 0, PAP_SIZE_HI - 1));
                    ASSERT(WITHIN(nz, 0, PAP_SIZE_HI - 1));

                    nh2 = &NS_hi[nx][nz];

                    if (nh2->water && (NS_HI_TYPE(nh2) != NS_HI_TYPE_CURVE) && (NS_HI_TYPE(nh2) != NS_HI_TYPE_ROCK)) {
                        if ((nh->water && nh2->water > nh->water) || (!nh->water && nh2->water > nh->bot)) {
                            if (NS_fall_free) {
                                ASSERT(WITHIN(NS_fall_free, 1, NS_MAX_FALLS - 1));
                                fall = NS_fall_free;
                                nf = &NS_fall[fall];
                                NS_fall_free = nf->next;

                                nf->x = sx;
                                nf->z = sz;
                                nf->dx = dx;
                                nf->dz = dz;
                                nf->top = nh2->water;
                                nf->bot = (nh->water) ? nh->water : nh->bot;
                                nf->counter = 0;

                                nf->next = nc->fall;
                                nc->fall = fall;
                            }
                        }
                    }
                }
            }
        }
}

// Emits the small square walls that line the underside of grate cells
// (holes through which water pours into the sewer).
// uc_orig: NS_cache_create_grates (fallen/Source/ns.cpp)
void NS_cache_create_grates(UBYTE mx, UBYTE mz)
{
    SLONG i;
    SLONG x;
    SLONG z;
    SLONG x1;
    SLONG y1;
    SLONG z1;
    SLONG x2;
    SLONG y2;
    SLONG z2;
    SLONG sx;
    SLONG sz;
    SLONG fall;
    NS_Hi* nh;
    SLONG p[4];

    static struct {
        SBYTE dx;
        SBYTE dz;
        UWORD norm;
    } order[4] = {
        { +1, 0, NS_NORM_XS },
        { -1, 0, NS_NORM_XL },
        { 0, +1, NS_NORM_ZS },
        { 0, -1, NS_NORM_ZL }
    };

    ASSERT(WITHIN(mx, 1, PAP_SIZE_LO - 2));
    ASSERT(WITHIN(mz, 1, PAP_SIZE_LO - 2));

    for (x = 0; x < 4; x++)
        for (z = 0; z < 4; z++) {
            sx = (mx << 2) + x;
            sz = (mz << 2) + z;
            ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
            ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

            nh = &NS_hi[sx][sz];

            if (nh->packed & NS_HI_FLAG_GRATE) {
                y1 = nh->bot;
                y2 = nh->bot - 8;

                for (i = 0; i < 4; i++) {
                    x1 = (sx << 8) + 0x80 + (order[i].dx + (-order[i].dz) << 7);
                    z1 = (sz << 8) + 0x80 + (order[i].dz + (+order[i].dx) << 7);
                    x2 = (sx << 8) + 0x80 + (order[i].dx - (-order[i].dz) << 7);
                    z2 = (sz << 8) + 0x80 + (order[i].dz - (+order[i].dx) << 7);

                    p[0] = NS_scratch_point_upto + 0;
                    p[1] = NS_scratch_point_upto + 1;
                    p[2] = NS_scratch_point_upto + 2;
                    p[3] = NS_scratch_point_upto + 3;

                    NS_add_point(x1, y2, z1, order[i].norm);
                    NS_add_point(x2, y2, z2, order[i].norm);
                    NS_add_point(x1, y1, z1, NS_NORM_BLACK);
                    NS_add_point(x2, y1, z2, NS_NORM_BLACK);

                    NS_add_face(p, NS_PAGE_STONE, NS_TEXTURE_FULL);
                }
            }
        }
}

// Builds the full geometry cache for lo-res mapsquare (mx,mz). Allocates from the HEAP.
// Returns UC_TRUE on success, UC_FALSE if the square is at the map edge (cannot generate geometry there).
// uc_orig: NS_cache_create (fallen/Source/ns.cpp)
SLONG NS_cache_create(UBYTE mx, UBYTE mz)
{
    SLONG dmx;
    SLONG dmz;
    NS_Cache* nc;
    NS_Lo* nl;
    SLONG c_index;
    SLONG memory_in_bytes;
    SLONG memory_points;
    SLONG memory_faces;

    ASSERT(WITHIN(mx, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_LO - 1));
    ASSERT(NS_lo[mx][mz].cache == NULL);
    ASSERT(NS_cache_free != NULL);

    if (mx == PAP_SIZE_LO - 1 || mz == PAP_SIZE_LO - 1 || mx == 0 || mz == 0) {
        return UC_FALSE;
    }

    NS_scratch_point_upto = 0;
    NS_scratch_face_upto = 0;

    NS_scratch_origin_x = mx << PAP_SHIFT_LO;
    NS_scratch_origin_z = mz << PAP_SHIFT_LO;

    c_index = NS_cache_free;
    ASSERT(WITHIN(c_index, 1, NS_MAX_CACHES - 1));

    nc = &NS_cache[c_index];
    NS_cache_free = nc->next;

    // Collect all light sources from the 3x3 block of lo-res squares around (mx,mz).
    NS_slight_upto = 0;
    for (dmx = -1; dmx <= +1; dmx++)
        for (dmz = -1; dmz <= +1; dmz++) {
            ASSERT(WITHIN(mx + dmx, 0, PAP_SIZE_LO - 1));
            ASSERT(WITHIN(mz + dmz, 0, PAP_SIZE_LO - 1));

            nl = &NS_lo[mx + dmx][mz + dmz];
            if (nl->light_y) {
                ASSERT(WITHIN(NS_slight_upto, 0, NS_MAX_SLIGHTS - 1));
                NS_slight[NS_slight_upto].x = nl->light_x + dmx * 128;
                NS_slight[NS_slight_upto].z = nl->light_z + dmz * 128;
                NS_slight[NS_slight_upto].y = nl->light_y;
                NS_slight_upto += 1;
            }
        }

    NS_cache_create_floors(mx, mz);
    NS_cache_create_walls(mx, mz);
    NS_cache_create_curves(mx, mz);
    NS_cache_create_grates(mx, mz);

    nc->next = NULL;
    nc->used = UC_TRUE;
    nc->map_x = mx;
    nc->map_z = mz;
    nc->num_points = NS_scratch_point_upto;
    nc->num_faces = NS_scratch_face_upto;
    nc->fall = NULL;

    memory_points = nc->num_points * sizeof(NS_Point);
    memory_faces = nc->num_faces * sizeof(NS_Face);
    memory_in_bytes = memory_points + memory_faces;

    nc->memory = (UBYTE*)HEAP_get(memory_in_bytes);
    ASSERT(nc->memory != NULL);

    memcpy(nc->memory, NS_scratch_point, memory_points);
    memcpy(nc->memory + memory_points, NS_scratch_face, memory_faces);

    NS_cache_create_falls(mx, mz, nc);

    NS_lo[mx][mz].cache = c_index;
    return UC_TRUE;
}

// Frees the geometry cache for slot 'cache': returns HEAP memory, frees waterfalls, unlinks
// the lo-res square, and puts the slot back on the free list.
// uc_orig: NS_cache_destroy (fallen/Source/ns.cpp)
void NS_cache_destroy(UBYTE cache)
{
    SLONG memory_in_bytes;
    SLONG fall;
    SLONG next;
    NS_Fall* nf;
    NS_Cache* nc;

    ASSERT(WITHIN(cache, 1, NS_MAX_CACHES - 1));

    nc = &NS_cache[cache];
    ASSERT(nc->used);
    ASSERT(WITHIN(nc->map_x, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(nc->map_z, 0, PAP_SIZE_LO - 1));

    memory_in_bytes = nc->num_points * sizeof(NS_Point);
    memory_in_bytes += nc->num_faces * sizeof(NS_Face);
    HEAP_give(nc->memory, memory_in_bytes);

    fall = nc->fall;
    while (fall) {
        ASSERT(WITHIN(fall, 1, NS_MAX_FALLS - 1));
        nf = &NS_fall[fall];
        next = nf->next;
        nf->next = NS_fall_free;
        NS_fall_free = fall;
        fall = next;
    }

    nc->used = UC_FALSE;
    nc->next = NS_cache_free;
    NS_cache_free = cache;

    NS_lo[nc->map_x][nc->map_z].cache = NULL;
}

// Tears down and re-initialises the cache (frees all cached geometry).
// uc_orig: NS_cache_fini (fallen/Source/ns.cpp)
void NS_cache_fini()
{
    NS_cache_init();
}

// Returns the sewer floor height (in world Y units) at world position (x, z).
// Returns -0x200 if the position is solid rock (no sewer here).
// uc_orig: NS_calc_height_at (fallen/Source/ns.cpp)
SLONG NS_calc_height_at(SLONG x, SLONG z)
{
    NS_Hi* nh;
    SLONG ans;
    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
        return 0;
    }

    nh = &NS_hi[mx][mz];

    if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
        return -0x200;
    } else {
        ans = (nh->bot << 5) + (-32 * 0x100);
    }

    return ans;
}

// Returns the water surface height at (x,z), or the floor height if there is no water.
// uc_orig: NS_calc_splash_height_at (fallen/Source/ns.cpp)
SLONG NS_calc_splash_height_at(SLONG x, SLONG z)
{
    NS_Hi* nh;
    SLONG ans;
    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
        return 0;
    }

    nh = &NS_hi[mx][mz];

    if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
        return -0x200;
    } else {
        if (nh->water) {
            ans = (nh->water << 5) + (-32 * 0x100);
        } else {
            ans = (nh->bot << 5) + (-32 * 0x100);
        }
    }

    return ans;
}

// Slides the destination position (*x2,*y2,*z2) so it does not penetrate sewer geometry.
// All coordinates in 16-bit fixed point; radius is 8-bit fixed point.
// Clamps to the playable map area first, then collides against the four adjacent hi-res cells.
// uc_orig: NS_slide_along (fallen/Source/ns.cpp)
void NS_slide_along(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG radius)
{
    SLONG i;
    SLONG height;
    SLONG collided = UC_FALSE;
    SLONG mx;
    SLONG mz;
    SLONG dx;
    SLONG dz;
    SLONG vx1;
    SLONG vz1;
    SLONG vx2;
    SLONG vz2;
    SLONG sx1;
    SLONG sz1;
    SLONG sx2;
    SLONG sz2;
    SLONG sradius;
    NS_Hi* nh;

    const struct {
        SBYTE dx;
        SBYTE dz;
    } order[4] = {
        { +1, 0 },
        { -1, 0 },
        { 0, +1 },
        { 0, -1 }
    };

    SLONG collide;

#define NS_SLIDE_MIN ((1 << PAP_SHIFT_LO) << 8)
#define NS_SLIDE_MAX (((PAP_SIZE_LO - 1) << PAP_SHIFT_LO) << 8)

    if (*x2 < NS_SLIDE_MIN) { *x2 = NS_SLIDE_MIN; }
    if (*x2 > NS_SLIDE_MAX) { *x2 = NS_SLIDE_MAX; }
    if (*z2 < NS_SLIDE_MIN) { *z2 = NS_SLIDE_MIN; }
    if (*z2 > NS_SLIDE_MAX) { *z2 = NS_SLIDE_MAX; }

    radius <<= 8;

    for (i = 0; i < 4; i++) {
        mx = (x1 >> 16) + order[i].dx;
        mz = (z1 >> 16) + order[i].dz;

        ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
        ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

        nh = &NS_hi[mx][mz];

        switch (NS_HI_TYPE(nh)) {
        case NS_HI_TYPE_ROCK:
        case NS_HI_TYPE_CURVE:
        case NS_HI_TYPE_NOTHING:
            collide = UC_TRUE;
            break;

        default:
            height = (nh->bot << (5 + 8)) + (-32 * 0x100 * 0x100);
            // Allow stepping up a quarter of a block.
            if (*y2 + 0x4000 < height) {
                collide = UC_TRUE;
            } else {
                collide = UC_FALSE;
            }
            break;
        }

        if (collide) {
            dx = order[i].dx << 7;
            dz = order[i].dz << 7;
            mx = x1 >> 16;
            mz = z1 >> 16;

            sx1 = (mx << 8) + 0x80 + dx - dz;
            sz1 = (mz << 8) + 0x80 + dz + dx;
            sx2 = (mx << 8) + 0x80 + dx + dz;
            sz2 = (mz << 8) + 0x80 + dz - dx;

            sradius = radius >> 8;
            vx1 = x1 >> 8;
            vz1 = z1 >> 8;
            vx2 = *x2 >> 8;
            vz2 = *z2 >> 8;

            if (slide_around_sausage(sx1, sz1, sx2, sz2, sradius, vx1, vz1, &vx2, &vz2)) {
                *x2 = vx2 << 8;
                *z2 = vz2 << 8;
            }
        }
    }
}

// Returns UC_TRUE if (x,y,z) is below the sewer floor — i.e., inside the rock.
// uc_orig: NS_inside (fallen/Source/ns.cpp)
SLONG NS_inside(SLONG x, SLONG y, SLONG z)
{
    return y < NS_calc_height_at(x, z);
}

// Returns UC_TRUE if the line segment from (x1,y1,z1) to (x2,y2,z2) has clear line-of-sight
// through the sewer (no rock intersections). Sets NS_los_fail_* to the last clear point on failure.
// uc_orig: NS_there_is_a_los (fallen/Source/ns.cpp)
SLONG NS_there_is_a_los(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG i;
    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dx = x2 - x1;
    SLONG dy = y2 - y1;
    SLONG dz = z2 - z1;

    SLONG len = QDIST3(abs(dx), abs(dy), abs(dz));
    SLONG steps = len >> 5;

    if (len == 0) {
        return UC_TRUE;
    }

    dx = (dx << 5) / len;
    dy = (dy << 5) / len;
    dz = (dz << 5) / len;

    x = x1 + dx;
    y = y1 + dy;
    z = z1 + dz;

    for (i = 1; i < steps; i++) {
        if (NS_inside(x, y, z)) {
            NS_los_fail_x = x - dx;
            NS_los_fail_y = y - dy;
            NS_los_fail_z = z - dz;
            return UC_FALSE;
        }
    }

    return UC_TRUE;
}

// Finds a free NS_St slot by random search. Returns the slot index, or NULL if the pool is full.
// uc_orig: NS_get_unused_st (fallen/Source/ns.cpp)
SLONG NS_get_unused_st(void)
{
    SLONG i;
    SLONG pick;
    NS_St* nst;

    pick = rand() % (NS_MAX_STS - 1);
    pick += 1;

    for (i = 0; i < NS_MAX_STS; i++) {
        ASSERT(WITHIN(pick, 1, NS_MAX_STS - 1));
        nst = &NS_st[pick];
        if (nst->type == NS_ST_TYPE_UNUSED) {
            return pick;
        }
        pick += 1;
        if (pick >= NS_MAX_STS) {
            pick = 1;
        }
    }

    return NULL;
}

// Adds a ladder sewer-thing spanning from (x1,z1) to (x2,z2) at the given height.
// Coordinates are in hi-res mapsquare units; the ladder is placed in the lo-res square
// that contains the midpoint of the segment offset by a small perpendicular bias.
// uc_orig: NS_add_ladder (fallen/Source/ns.cpp)
void NS_add_ladder(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG height)
{
    SLONG mx = (x1 + x2 << 7) + ((z2 - z1) << 2) >> PAP_SHIFT_LO;
    SLONG mz = (z1 + z2 << 7) - ((x2 - x1) << 2) >> PAP_SHIFT_LO;

    if (!WITHIN(mx, 1, PAP_SIZE_LO - 2) || !WITHIN(mz, 1, PAP_SIZE_LO - 2)) {
        return;
    }

    NS_Lo* nl = &NS_lo[mx][mz];
    SLONG index = NS_get_unused_st();

    if (index == NULL) {
        return;
    }

    ASSERT(WITHIN(index, 1, NS_MAX_STS - 1));

    NS_St* nst = &NS_st[index];
    nst->type = NS_ST_TYPE_LADDER;
    nst->ladder.x1 = x1;
    nst->ladder.z1 = z1;
    nst->ladder.x2 = x2;
    nst->ladder.z2 = z2;
    nst->ladder.height = height;

    nst->next = nl->st;
    nl->st = index;
}

// Adds a prim sewer-thing at fixed-8 sewer coordinates (x,z) and sewer Y.
// The prim is placed in the lo-res square containing (x,z); position and yaw are
// stored relative to that square.
// uc_orig: NS_add_prim (fallen/Source/ns.cpp)
void NS_add_prim(
    SLONG prim,
    SLONG yaw,
    SLONG x,
    SLONG y,
    SLONG z)
{
    SLONG mx = x >> PAP_SHIFT_LO;
    SLONG mz = z >> PAP_SHIFT_LO;

    if (!WITHIN(mx, 1, PAP_SIZE_LO - 2) || !WITHIN(mz, 1, PAP_SIZE_LO - 2)) {
        return;
    }

    NS_Lo* nl = &NS_lo[mx][mz];
    SLONG index = NS_get_unused_st();

    if (index == NULL) {
        return;
    }

    ASSERT(WITHIN(index, 1, NS_MAX_STS - 1));

    NS_St* nst = &NS_st[index];
    nst->type = NS_ST_TYPE_PRIM;
    nst->prim.prim = prim;
    nst->prim.yaw = yaw >> 3;
    nst->prim.x = (x - (mx << PAP_SHIFT_LO)) >> 3;
    nst->prim.z = (z - (mz << PAP_SHIFT_LO)) >> 3;
    nst->prim.y = y;

    nst->next = nl->st;
    nl->st = index;
}
