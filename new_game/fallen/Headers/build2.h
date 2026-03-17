//
// The other building file!  A simpler map...
//

#ifndef FALLEN_HEADERS_BUILD2_H
#define FALLEN_HEADERS_BUILD2_H

//
// Puts all the facets and walkable faces on the mapwho.
//

void build_quick_city(void);

//
// Puts an single walkable face onto the Walkable mapwho.
// Removes a walkable face from the mapwho.
//

void attach_walkable_to_map(SLONG face);
void remove_walkable_from_map(SLONG face);

//
// Adds a dfacet to the mapwho.
//

void add_facet_to_map(SLONG dfacet);

#endif // FALLEN_HEADERS_BUILD2_H
