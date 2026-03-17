//
// Draws buildings.
//

#ifndef FALLEN_DDENGINE_HEADERS_BUILD_H
#define FALLEN_DDENGINE_HEADERS_BUILD_H

//
// This function uses the POLY module, and assumes
// that all the camera stuff has already been set up.
//

void BUILD_draw(Thing* building);

//
// Draws a building you are inside.
//

void BUILD_draw_inside(void);

#endif // FALLEN_DDENGINE_HEADERS_BUILD_H
