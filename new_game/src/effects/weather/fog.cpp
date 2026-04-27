#include "effects/weather/fog.h"
#include "effects/weather/fog_globals.h"

// uc_orig: FOG_init (fallen/Source/fog.cpp)
void FOG_init()
{
    SLONG i;

    for (i = 0; i < FOG_MAX_FOG; i++) {
        FOG_fog[i].type = FOG_TYPE_UNUSED;
    }
}
