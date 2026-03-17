// grenade.h
//
// grenade code

#ifndef FALLEN_HEADERS_GRENADE_H
#define FALLEN_HEADERS_GRENADE_H

// initialize system
void InitGrenades();

// create a grenade with these parameters
bool CreateGrenade(Thing* owner, SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, SLONG timer);

// create a grenade from the person data
bool CreateGrenadeFromPerson(Thing* p_person, SLONG timer);

// draw the grenades
void DrawGrenades();

// process the grenades
void ProcessGrenades();

// create a grenade explosion
void CreateGrenadeExplosion(SLONG x, SLONG y, SLONG z, Thing* owner);

#endif // FALLEN_HEADERS_GRENADE_H
