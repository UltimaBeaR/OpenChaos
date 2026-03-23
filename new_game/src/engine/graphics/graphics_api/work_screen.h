#ifndef ENGINE_GRAPHICS_GRAPHICS_API_WORK_SCREEN_H
#define ENGINE_GRAPHICS_GRAPHICS_API_WORK_SCREEN_H

#include <MFStdLib.h>
#include "work_screen_globals.h"

// uc_orig: ShowWorkScreen (fallen/DDLibrary/Headers/GWorkScreen.h)
void ShowWorkScreen(ULONG flags);

// uc_orig: LockWorkScreen (fallen/DDLibrary/Headers/GWorkScreen.h)
void* LockWorkScreen(void);

// uc_orig: UnlockWorkScreen (fallen/DDLibrary/Headers/GWorkScreen.h)
void UnlockWorkScreen(void);

// uc_orig: ClearWorkScreen (fallen/MFStdLib/Headers/MFStdLib.h)
void ClearWorkScreen(UBYTE colour);

// uc_orig: ShowWorkWindow (fallen/DDLibrary/Source/GWorkScreen.cpp)
void ShowWorkWindow(ULONG flags);

// uc_orig: SetWorkWindowBounds (fallen/DDLibrary/Headers/GWorkScreen.h)
void SetWorkWindowBounds(SLONG left, SLONG top, SLONG width, SLONG height);

// uc_orig: GlobalToLocal (fallen/DDLibrary/Headers/GWorkScreen.h)
MFPoint* GlobalToLocal(MFPoint* the_point);

// uc_orig: GlobalXYToLocal (fallen/DDLibrary/Headers/GWorkScreen.h)
void GlobalXYToLocal(SLONG* x, SLONG* y);

// uc_orig: SetPalette (fallen/DDLibrary/Source/GWorkScreen.cpp)
void SetPalette(UBYTE* the_palette);

// uc_orig: FindColour (fallen/DDLibrary/Source/GWorkScreen.cpp)
SLONG FindColour(UBYTE* the_palette, SLONG r, SLONG g, SLONG b);

// uc_orig: SetWorkWindow (fallen/DDLibrary/Headers/GWorkScreen.h)
inline void SetWorkWindow(void) { WorkWindow = (WorkScreen + WorkWindowRect.Left * WorkScreenDepth + (WorkWindowRect.Top * WorkScreenWidth)); }

// uc_orig: XYInRect (fallen/DDLibrary/Headers/GWorkScreen.h)
inline BOOL XYInRect(SLONG x, SLONG y, MFRect* the_rect)
{
    if (x >= the_rect->Left && y >= the_rect->Top && x <= the_rect->Right && y <= the_rect->Bottom)
        return UC_TRUE;
    else
        return UC_FALSE;
}

// uc_orig: PointInRect (fallen/DDLibrary/Headers/GWorkScreen.h)
inline BOOL PointInRect(MFPoint* the_point, MFRect* the_rect)
{
    if (the_point->X >= the_rect->Left && the_point->Y >= the_rect->Top && the_point->X <= the_rect->Right && the_point->Y <= the_rect->Bottom)
        return UC_TRUE;
    else
        return UC_FALSE;
}

#endif // ENGINE_GRAPHICS_GRAPHICS_API_WORK_SCREEN_H
