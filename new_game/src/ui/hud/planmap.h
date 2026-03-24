#ifndef UI_HUD_PLANMAP_H
#define UI_HUD_PLANMAP_H

#include "engine/core/types.h"

// Flags for map_beacon_draw.
// uc_orig: BEACON_FLAG_BEACON (fallen/DDEngine/Headers/planmap.h)
#define BEACON_FLAG_BEACON (1 << 0)
// uc_orig: BEACON_FLAG_POINTY (fallen/DDEngine/Headers/planmap.h)
#define BEACON_FLAG_POINTY (1 << 1)

// Renders the plan view (top-down) of the city into a raw 24-bit RGB memory buffer.
// wx/wz: center world position in fixed-point (8 bits per mapsquare).
// pixelw: pixels per map square.
// sx/sy/w/h: destination rect on screen.
// mem: destination 640x480x3 RGB buffer.
// uc_orig: plan_view_shot (fallen/DDEngine/Headers/planmap.h)
void plan_view_shot(SLONG wx, SLONG wz, SLONG pixelw, SLONG sx, SLONG sy, SLONG w, SLONG h, UBYTE* mem);

// Draws a blip on the planmap at world position (x, z) with given colour, flags, and direction.
// uc_orig: map_beacon_draw (fallen/DDEngine/Headers/planmap.h)
void map_beacon_draw(SLONG x, SLONG z, ULONG col, ULONG flags, UWORD dir);

#endif // UI_HUD_PLANMAP_H
