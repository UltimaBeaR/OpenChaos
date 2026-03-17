//
// Poly-modelled animating waterfalls.
//

#ifndef FALLEN_DDENGINE_HEADERS_FALL_H
#define FALLEN_DDENGINE_HEADERS_FALL_H

//
// Initialises all the waterfalls.
//

void FALL_init(void);

//
// Creates a new waterfall. NULL => Couldn't create one.
//

UBYTE FALL_create(
    float x1, float z1, float u, float v,
    float x2, float z2, float u, float v,
    float dx,
    float dz,
    float du,
    float dv,
    float top_y,
    float bot_y);

//
// Destroys the given waterfall.
//

void FALL_destroy(void);

//
// Processes all the waterfalls.
//

void FALL_process(void);

//
// Draws them all.
//

void FALL_draw(void);

#endif // FALLEN_DDENGINE_HEADERS_FALL_H
