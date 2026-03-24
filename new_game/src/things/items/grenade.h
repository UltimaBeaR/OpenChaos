#ifndef THINGS_ITEMS_GRENADE_H
#define THINGS_ITEMS_GRENADE_H

#include "engine/core/types.h"

// Forward declaration — avoid pulling in all of Game.h in this header.
struct Thing;

// uc_orig: InitGrenades (fallen/Source/grenade.cpp)
void InitGrenades();

// uc_orig: CreateGrenade (fallen/Source/grenade.cpp)
bool CreateGrenade(Thing* owner, SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, SLONG timer);

// uc_orig: CreateGrenadeFromPerson (fallen/Source/grenade.cpp)
bool CreateGrenadeFromPerson(Thing* p_person, SLONG timer);

// uc_orig: DrawGrenades (fallen/Source/grenade.cpp)
void DrawGrenades();

// uc_orig: ProcessGrenades (fallen/Source/grenade.cpp)
void ProcessGrenades();

// uc_orig: CreateGrenadeExplosion (fallen/Source/grenade.cpp)
void CreateGrenadeExplosion(SLONG x, SLONG y, SLONG z, Thing* owner);

// uc_orig: show_grenade_path (fallen/Source/grenade.cpp)
void show_grenade_path(Thing* p_person);

#endif // THINGS_ITEMS_GRENADE_H
