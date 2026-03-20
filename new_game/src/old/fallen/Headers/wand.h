//
// A system for wandering people.
//

#ifndef FALLEN_HEADERS_WAND_H
#define FALLEN_HEADERS_WAND_H

//
// Initialises the wander system. Looks for nice places to walk and
// sets the PAP_FLAG_WANDER flag.
//

void WAND_init(void);

//
// Returns the next place for a wandering person to wander to.
//

void WAND_get_next_place(
    Thing* p_person,
    SLONG* wand_world_x,
    SLONG* wand_world_z);

//
// Returns TRUE if the given square is a good place to wander.
//

SLONG WAND_square_is_wander(SLONG map_x, SLONG map_z);

//
// Draws a cross over all the wander squares near the given place.
//

void WAND_draw(SLONG map_x, SLONG map_z);

#endif // FALLEN_HEADERS_WAND_H
