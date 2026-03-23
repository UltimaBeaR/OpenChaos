#ifndef FALLEN_HEADERS_BUILDING_H
#define FALLEN_HEADERS_BUILDING_H

// Redirect: all building types have been migrated to the new structure.
#include "world/environment/prim_types.h"
#include "world/environment/building_types.h"

// Functions declared in the legacy building.h that are now in world/environment/building.h.
// Include building.h directly if you need the full building API.
// (This redirect is here so that Game.h / legacy consumers still compile.)
#include "world/environment/building.h"

#endif // FALLEN_HEADERS_BUILDING_H
