// Sewer/cavern subsystem — chunk 1: init, cache init, vertex/face scratch, floors, wallstrip helpers.
// See ns.h for the full sewer system API.

// Temporary: game.h provides ASSERT, WITHIN, NULL, memset, rand, and base types.
#include "game.h"
#include "world/environment/ns.h"
#include "world/environment/ns_globals.h"
#include "core/heap.h"
#include "world/map/pap.h"

// Internal constants are defined in ns_globals.h (shared with remaining old ns.cpp chunks).

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
        NS_cache[i].used = FALSE;
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
