#include "effects/environment/fire.h"
#include "effects/environment/fire_globals.h"

// uc_orig: FIRE_get_start (fallen/Source/fire.cpp)
// Initialise renderer iterator with screen-space filter parameters.
void FIRE_get_start(UBYTE z, UBYTE xmin, UBYTE xmax)
{
    FIRE_get_fire_upto = 0;
    FIRE_get_flame = NULL;
    FIRE_get_z = z;
    FIRE_get_xmin = xmin;
    FIRE_get_xmax = xmax;
}

