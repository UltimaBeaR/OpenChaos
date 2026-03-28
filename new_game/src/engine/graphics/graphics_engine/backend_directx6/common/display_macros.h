#ifndef ENGINE_GRAPHICS_GRAPHICS_API_DISPLAY_MACROS_H
#define ENGINE_GRAPHICS_GRAPHICS_API_DISPLAY_MACROS_H

// Convenience macros that forward to the_display methods.
// All rendering calls funnel through GDisplay — these macros
// existed in the original codebase to avoid repeating "the_display." everywhere.

#include "engine/graphics/graphics_engine/backend_directx6/common/gd_display.h"

// uc_orig: SET_BLACK_BACKGROUND (fallen/DDLibrary/Headers/DDlib.h)
#define SET_BLACK_BACKGROUND the_display.SetBlackBackground()
// uc_orig: SET_WHITE_BACKGROUND (fallen/DDLibrary/Headers/DDlib.h)
#define SET_WHITE_BACKGROUND the_display.SetWhiteBackground()
// uc_orig: SET_BLUE_BACKGROUND (fallen/DDLibrary/Headers/DDlib.h)
#define SET_BLUE_BACKGROUND  the_display.SetBlueBackground()
// uc_orig: SET_USER_BACKGROUND (fallen/DDLibrary/Headers/DDlib.h)
#define SET_USER_BACKGROUND  the_display.SetUserBackground()

// uc_orig: BEGIN_SCENE (fallen/DDLibrary/Headers/DDlib.h)
#define BEGIN_SCENE the_display.BeginScene()
// uc_orig: END_SCENE (fallen/DDLibrary/Headers/DDlib.h)
#define END_SCENE   the_display.EndScene()
// uc_orig: CLEAR_VIEWPORT (fallen/DDLibrary/Headers/DDlib.h)
#define CLEAR_VIEWPORT the_display.ClearViewport()
// uc_orig: FLIP (fallen/DDLibrary/Headers/DDlib.h)
#define FLIP(a, f) the_display.Flip(a, f)
// uc_orig: DRAW_PRIMITIVE (fallen/DDLibrary/Headers/DDlib.h)
#define DRAW_PRIMITIVE(pt, vt, v, vc, f) the_display.DrawPrimitive(pt, vt, v, vc, f)
// uc_orig: DRAW_INDEXED_PRIMITIVE (fallen/DDLibrary/Headers/DDlib.h)
#define DRAW_INDEXED_PRIMITIVE(pt, vt, v, vc, i, ic, f) the_display.DrawIndexedPrimitive(pt, vt, v, vc, i, ic, f)

// Render state / texture helpers — thin wrappers over Direct3D calls.
// uc_orig: REALLY_SET_RENDER_STATE (fallen/DDLibrary/Headers/DDlib.h)
#define REALLY_SET_RENDER_STATE(t, s) the_display.SetRenderState(t, s)
// uc_orig: REALLY_SET_TEXTURE (fallen/DDLibrary/Headers/DDlib.h)
#define REALLY_SET_TEXTURE(tex) the_display.SetTexture(tex)
// uc_orig: REALLY_SET_NO_TEXTURE (fallen/DDLibrary/Headers/DDlib.h)
#define REALLY_SET_NO_TEXTURE the_display.SetTexture(NULL)
// uc_orig: REALLY_SET_TEXTURE_STATE (fallen/DDLibrary/Headers/DDlib.h)
#define REALLY_SET_TEXTURE_STATE(n, t, s) the_display.SetTextureState(n, t, s)

// Debug error-reporting stubs — in Release these expand to nothing.
// In the original Debug build they called actual logging functions (Debug.h).
// uc_orig: dd_error (fallen/DDLibrary/Headers/Debug.h)
#define dd_error
// uc_orig: d3d_error (fallen/DDLibrary/Headers/Debug.h)
#define d3d_error
// uc_orig: di_error (fallen/DDLibrary/Headers/Debug.h)
#define di_error

#endif // ENGINE_GRAPHICS_GRAPHICS_API_DISPLAY_MACROS_H
