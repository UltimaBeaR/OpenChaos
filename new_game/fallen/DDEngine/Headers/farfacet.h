//
// Faster far-facets...
//

#ifndef FALLEN_DDENGINE_HEADERS_FARFACET_H
#define FALLEN_DDENGINE_HEADERS_FARFACET_H

//
// Creates optimised data for drawing farfacets.
// Call after all facets have been loaded.
//

void FARFACET_init(void);

//
// Draws the far facets that can be seen from this view.
//

void FARFACET_draw(
    float x,
    float y,
    float z,
    float yaw,
    float pitch,
    float roll,
    float draw_dist,
    float lens);

//
// Frees up memory.
//

void FARFACET_fini(void);

#endif // FALLEN_DDENGINE_HEADERS_FARFACET_H
