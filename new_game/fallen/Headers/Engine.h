// Engine.h
// Guy Simmons, 22nd October 1997.

#ifndef FALLEN_HEADERS_ENGINE_H
#define FALLEN_HEADERS_ENGINE_H
#include "../../MFStdLib/Headers/MFStdLib.h"
//---------------------------------------------------------------

// Camera, M31, M33, Engine types are defined in DDEngine/Headers/Engine.h.
// That is the canonical definition used at link time.
// The stale copies that used to live here have been removed (see uc_also_in comments there).
#include "..\DDEngine\Headers\Engine.h"

BOOL init_3d_engine(void);
void fini_3d_engine(void);
void game_engine(Camera* the_view);
void engine_attract(void);
void engine_win_level(void);
void engine_lose_level(void);

//---------------------------------------------------------------

void temp_setup_map(void);
BOOL new_init_3d_engine(void);
void new_engine(Camera* the_view);

//---------------------------------------------------------------

//
// After you have loaded all the prims, call this function. It
// fixes the texture coordinates of the prims if the engine has
// fiddled with the texture pages.
//

void engine_fiddle_prim_textures(void);

//
// Given a texture square coordinate on a page, it returns the
// page and texture square coordinates of the fiddled position.
//
// 'square_u' and 'square_v' should be between 0 and 7, and the
// fiddled position are returned.
//

SLONG TEXTURE_get_fiddled_position(
    SLONG square_u,
    SLONG square_v,
    SLONG page,
    float* u,
    float* v);

//
// Debugging aids.
//

void e_draw_3d_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);
void e_draw_3d_line_dir(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);
void e_draw_3d_line_col(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG r, SLONG g, SLONG b);
void e_draw_3d_line_col_sorted(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG r, SLONG g, SLONG b);
void e_draw_3d_mapwho(SLONG x1, SLONG z1);
void e_draw_3d_mapwho_y(SLONG x1, SLONG y1, SLONG z1);

//
// Messages drawn straight to the screen.
//

void MSG_add(CBYTE* message, ...);

//
// Engine stuff to help you draw straight to the screen...
//

void ENGINE_clear_screen(void);
void ENGINE_flip(void);
SLONG ENGINE_lock(void); // Locks the screen.. returns TRUE on success.
void ENGINE_unlock(void);

#include "..\ddengine\headers\font.h"

#endif // FALLEN_HEADERS_ENGINE_H
