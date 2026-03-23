#ifndef ACTORS_ITEMS_DIKE_H
#define ACTORS_ITEMS_DIKE_H

#include "core/types.h"

// uc_orig: DIKE_CONTROL_ACCEL (fallen/Headers/dike.h)
#define DIKE_CONTROL_ACCEL    (1 << 0)
// uc_orig: DIKE_CONTROL_LEFT (fallen/Headers/dike.h)
#define DIKE_CONTROL_LEFT     (1 << 1)
// uc_orig: DIKE_CONTROL_RIGHT (fallen/Headers/dike.h)
#define DIKE_CONTROL_RIGHT    (1 << 2)
// uc_orig: DIKE_CONTROL_BRAKE (fallen/Headers/dike.h)
#define DIKE_CONTROL_BRAKE    (1 << 3)
// uc_orig: DIKE_CONTORL_WHEELIE (fallen/Headers/dike.h)
#define DIKE_CONTORL_WHEELIE  (1 << 4)

// uc_orig: DIKE_FLAG_ON_GROUND_FRONT (fallen/Headers/dike.h)
#define DIKE_FLAG_ON_GROUND_FRONT (1 << 0)
// uc_orig: DIKE_FLAG_ON_GROUND_BACK (fallen/Headers/dike.h)
#define DIKE_FLAG_ON_GROUND_BACK  (1 << 1)

// uc_orig: DIKE_Dike (fallen/Headers/dike.h)
typedef struct {
    SLONG fx;     // Front wheel position.
    SLONG fy;
    SLONG fz;
    SLONG fdx;    // Front wheel velocity.
    SLONG fdy;
    SLONG fdz;

    SLONG bx;     // Back wheel position.
    SLONG by;
    SLONG bz;
    SLONG bdx;    // Back wheel velocity.
    SLONG bdy;
    SLONG bdz;

    SLONG fsy;    // Front suspension position.
    SLONG fsdy;   // Front suspension velocity.

    SLONG bsy;    // Back suspension position.
    SLONG bsdy;   // Back suspension velocity.

    UBYTE control;
    SBYTE steer;
    SBYTE power;

    UBYTE flag;

    UWORD yaw;
    UWORD pitch;
} DIKE_Dike;

// uc_orig: DIKE_MAX_DIKES (fallen/Headers/dike.h)
#define DIKE_MAX_DIKES 8

// uc_orig: DIKE_init (fallen/Source/dike.cpp)
void DIKE_init(void);

// uc_orig: DIKE_create (fallen/Source/dike.cpp)
DIKE_Dike* DIKE_create(SLONG x, SLONG z, SLONG yaw);

// uc_orig: DIKE_process (fallen/Source/dike.cpp)
void DIKE_process(DIKE_Dike* dd);

// uc_orig: DIKE_draw (fallen/Source/dike.cpp)
void DIKE_draw(DIKE_Dike* dd);

#endif // ACTORS_ITEMS_DIKE_H
