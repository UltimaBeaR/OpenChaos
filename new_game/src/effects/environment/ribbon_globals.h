#ifndef EFFECTS_ENVIRONMENT_RIBBON_GLOBALS_H
#define EFFECTS_ENVIRONMENT_RIBBON_GLOBALS_H

#include "effects/environment/ribbon.h"

// All ribbon slots. Was a static (file-private) array named "Ribbons" in the original;
// renamed to ribbon_Ribbons to avoid collision when made extern.
// uc_orig: Ribbons (fallen/Source/ribbon.cpp)
extern Ribbon ribbon_Ribbons[MAX_RIBBONS];

#endif // EFFECTS_ENVIRONMENT_RIBBON_GLOBALS_H
