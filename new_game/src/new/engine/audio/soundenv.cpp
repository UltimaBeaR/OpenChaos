#include <MFStdLib.h>
#include "engine/audio/soundenv.h"
#include "engine/audio/soundenv_globals.h"

// Callback type used by SOUNDENV_Quadify to report discovered quads.
// uc_orig: SOUNDENV_CB_fn (fallen/Source/soundenv.cpp)
typedef void (*SOUNDENV_CB_fn)(SLONG, SLONG, SLONG, SLONG);

// Check if a row of cells in map[min..max][y] are all unvisited.
// Sets visited flag (bit 1) on each cell if clear.
// Returns 1 if the whole range was free, 0 otherwise.
// uc_orig: SOUNDENV_ClearRange (fallen/Source/soundenv.cpp)
static BOOL SOUNDENV_ClearRange(CBYTE map[128][128], SLONG min, SLONG max, SLONG y)
{
    SLONG x;

    for (x = min; x <= max; x++) {
        if (map[x][y])
            return 0;
    }
    for (x = min; x <= max; x++) {
        map[x][y] |= 2;
    }
    return 1;
}

// Scan map region [mx,my]..[mox,moy] for contiguous rectangular free areas.
// Calls CBfn for each discovered rectangle (greedy row-expansion algorithm).
// uc_orig: SOUNDENV_Quadify (fallen/Source/soundenv.cpp)
static void SOUNDENV_Quadify(CBYTE map[128][128], SLONG mx, SLONG my, SLONG mox, SLONG moy, SOUNDENV_CB_fn CBfn)
{
    SLONG x, y, blx, bly, blox, bloy;

    for (y = my; y < moy; y++) {
        for (x = mx; x < mox; x++) {
            if (!map[x][y]) {
                blx = blox = x;
                bly = bloy = y;
                while ((blox < mox) && !map[blox][y]) {
                    map[blox][y] |= 2;
                    blox++;
                }
                blox--;
                while ((bloy < moy - 1) && SOUNDENV_ClearRange(map, blx, blox, bloy + 1))
                    bloy++;
                CBfn(blx, bly, blox, bloy);
            }
        }
    }

    for (y = my; y < moy; y++)
        for (x = mx; x < mox; x++)
            map[x][y] &= 1;
}

// Callback: stores a discovered quad into SOUNDENV_gndquads[].
// uc_orig: cback (fallen/Source/soundenv.cpp)
static void cback(SLONG x, SLONG y, SLONG ox, SLONG oy)
{
    SOUNDENV_gndquads[SOUNDENV_gndctr].x = x << 16;
    SOUNDENV_gndquads[SOUNDENV_gndctr].y = y << 16;
    SOUNDENV_gndquads[SOUNDENV_gndctr].ox = (ox + 1) << 16;
    SOUNDENV_gndquads[SOUNDENV_gndctr].oy = (oy + 1) << 16;
    SOUNDENV_gndctr++;
}

// uc_orig: SOUNDENV_precalc (fallen/Source/soundenv.cpp)
void SOUNDENV_precalc(void)
{
    // Body is commented out in the original — sewer quad detection was disabled.
}

// Camera position externs used by SOUNDENV_upload (currently stub).
extern SLONG CAM_pos_x,
    CAM_pos_y,
    CAM_pos_z;

// uc_orig: SOUNDENV_upload (fallen/Source/soundenv.cpp)
void SOUNDENV_upload(void)
{
    // Stub in the original.
}
