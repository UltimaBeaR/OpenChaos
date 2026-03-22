#ifndef EFFECTS_GLOW_H
#define EFFECTS_GLOW_H

#include "core/types.h"

// Renders the rotating glow overlay used for the Guardian of Baalrog (final boss).
// Draws four overlapping rotated sprite quads that spin in alternating directions.
// fade: 0 = fully transparent, 255 = fully visible.
// uc_orig: DRAWXTRA_final_glow (fallen/DDEngine/Source/drawxtra.cpp)
void DRAWXTRA_final_glow(SLONG x, SLONG y, SLONG z, UBYTE fade);

#endif // EFFECTS_GLOW_H
