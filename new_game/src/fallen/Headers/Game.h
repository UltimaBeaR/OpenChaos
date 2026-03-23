// Game.h — redirect to new/missions/game_types.h
// All content migrated in Stage 4, iteration 164.
// Original: Guy Simmons, 17th October 1997.

#ifndef FALLEN_HEADERS_GAME_H
#define FALLEN_HEADERS_GAME_H

// Suppress warnings that the original Game.h disabled.
#pragma warning(disable : 4244) // truncation warning: useless
#pragma warning(disable : 4101) // unreferenced local
#pragma warning(disable : 4554) // precedence: use brackets

// MFStdLib brings in the foundational types (UBYTE, SLONG, etc.) and string.h.
#include <MFStdLib.h>
#include <string.h>

// Include core types and the COMMON macro before struct definitions that use them.
#include "fallen/Headers/Structs.h"  // redirect: COMMON, MiniTextureBits, MAV_Action
#include "fallen/Headers/State.h"    // StateFunction — needed by animal.h, etc.
#include "fallen/Headers/drawtype.h" // DrawTween, DrawMesh — needed by Vehicle.h, Chopper.h

// Pool object type definitions. Order matches original Game.h.
#include "fallen/Headers/building.h"
#include "actors/vehicles/furn.h"
#include "fallen/Headers/Vehicle.h"
#include "fallen/Headers/inline.h"
#include "fallen/Headers/Person.h"
#include "fallen/Headers/Animal.h"
#include "fallen/Headers/barrel.h"
#include "fallen/Headers/Chopper.h"
#include "fallen/Headers/Pyro.h"
#include "fallen/Headers/Player.h"
#include "fallen/Headers/Plat.h"
#include "fallen/Headers/Pjectile.h"
#include "fallen/Headers/Special.h"
#include "fallen/Headers/bat.h"
#include "fallen/Headers/Switch.h"
#include "fallen/Headers/tracks.h"
#include "fallen/Headers/Thing.h"
#include "fallen/Headers/Controls.h"
#include "fallen/Headers/Map.h"
#include "fallen/Headers/collide.h"
#include "fallen/Headers/interact.h"
#include "engine/graphics/pipeline/aeng.h"

// The actual content (Game struct, macros, constants).
#include "missions/game_types.h"

#endif // FALLEN_HEADERS_GAME_H
