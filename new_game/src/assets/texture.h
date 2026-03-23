#ifndef ASSETS_TEXTURE_H
#define ASSETS_TEXTURE_H

#include "core/types.h"
#include "engine/graphics/resources/d3d_texture.h"
#include "engine/lighting/crinkle.h"

// Texture system manages all Direct3D texture pages. A "page" is an integer index
// into TEXTURE_texture[]. World textures, effect textures, and special pages all
// share this flat index space (max TEXTURE_MAX_TEXTURES = 22*64 + 160 entries).

// uc_orig: TEXTURE_SHADOW_SIZE (fallen/DDEngine/Headers/texture.h)
#define TEXTURE_SHADOW_SIZE 64

// uc_orig: TEXTURE_VIDEO_SIZE (fallen/DDEngine/Headers/texture.h)
#define TEXTURE_VIDEO_SIZE 64

// uc_orig: FACE_PAGE_OFFSET (fallen/DDEngine/Headers/texture.h)
// Mesh UV page indices are relative to this offset into the main texture array.
#define FACE_PAGE_OFFSET (8 * 64)

// uc_orig: TEXTURE_choose_set (fallen/DDEngine/Headers/texture.h)
// Switches the active texture set (world n). Frees world textures from the previous set.
void TEXTURE_choose_set(SLONG number);

// uc_orig: TEXTURE_fix_prim_textures (fallen/DDEngine/Headers/texture.h)
void TEXTURE_fix_prim_textures(void);

// uc_orig: TEXTURE_fix_texture_styles (fallen/DDEngine/Headers/texture.h)
void TEXTURE_fix_texture_styles(void);

// uc_orig: TEXTURE_page_num_standard (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_num_standard;

// uc_orig: TEXTURE_page_snowflake (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_snowflake;
// uc_orig: TEXTURE_page_sparkle (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_sparkle;
// uc_orig: TEXTURE_page_explode2 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_explode2;
// uc_orig: TEXTURE_page_explode1 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_explode1;
// uc_orig: TEXTURE_page_bigbang (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bigbang;
// uc_orig: TEXTURE_page_face1 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_face1;
// uc_orig: TEXTURE_page_face2 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_face2;
// uc_orig: TEXTURE_page_face3 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_face3;
// uc_orig: TEXTURE_page_face4 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_face4;
// uc_orig: TEXTURE_page_face5 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_face5;
// uc_orig: TEXTURE_page_face6 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_face6;
// uc_orig: TEXTURE_page_fog (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_fog;
// uc_orig: TEXTURE_page_moon (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_moon;
// uc_orig: TEXTURE_page_clouds (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_clouds;
// uc_orig: TEXTURE_page_water (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_water;
// uc_orig: TEXTURE_page_puddle (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_puddle;
// uc_orig: TEXTURE_page_drip (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_drip;
// uc_orig: TEXTURE_page_shadow (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_shadow;
// uc_orig: TEXTURE_page_bang (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bang;
// uc_orig: TEXTURE_page_font (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_font;
// uc_orig: TEXTURE_page_logo (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_logo;
// uc_orig: TEXTURE_page_sky (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_sky;
// uc_orig: TEXTURE_page_flames (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_flames;
// uc_orig: TEXTURE_page_smoke (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_smoke;
// uc_orig: TEXTURE_page_flame2 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_flame2;
// uc_orig: TEXTURE_page_steam (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_steam;
// uc_orig: TEXTURE_page_menuflame (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_menuflame;
// uc_orig: TEXTURE_page_barbwire (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_barbwire;
// uc_orig: TEXTURE_page_font2d (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_font2d;
// uc_orig: TEXTURE_page_dustwave (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_dustwave;
// uc_orig: TEXTURE_page_flames3 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_flames3;
// uc_orig: TEXTURE_page_bloodsplat (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bloodsplat;
// uc_orig: TEXTURE_page_bloom1 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bloom1;
// uc_orig: TEXTURE_page_bloom2 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bloom2;
// uc_orig: TEXTURE_page_hitspang (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_hitspang;
// uc_orig: TEXTURE_page_lensflare (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_lensflare;
// uc_orig: TEXTURE_page_tyretrack (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_tyretrack;
// uc_orig: TEXTURE_page_envmap (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_envmap;
// uc_orig: TEXTURE_page_winmap (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_winmap;
// uc_orig: TEXTURE_page_leaf (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_leaf;
// uc_orig: TEXTURE_page_raindrop (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_raindrop;
// uc_orig: TEXTURE_page_footprint (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_footprint;
// uc_orig: TEXTURE_page_angel (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_angel;
// uc_orig: TEXTURE_page_devil (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_devil;
// uc_orig: TEXTURE_page_smoker (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_smoker;
// uc_orig: TEXTURE_page_target (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_target;
// uc_orig: TEXTURE_page_newfont (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_newfont;
// uc_orig: TEXTURE_page_droplet (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_droplet;
// uc_orig: TEXTURE_page_press1 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_press1;
// uc_orig: TEXTURE_page_press2 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_press2;
// uc_orig: TEXTURE_page_ic (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_ic;
// uc_orig: TEXTURE_page_ic2 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_ic2;
// uc_orig: TEXTURE_page_lcdfont (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_lcdfont;
// uc_orig: TEXTURE_page_smokecloud (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_smokecloud;
// uc_orig: TEXTURE_page_menulogo (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_menulogo;
// uc_orig: TEXTURE_page_polaroid (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_polaroid;
// uc_orig: TEXTURE_page_bigbutton (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bigbutton;
// uc_orig: TEXTURE_page_bigleaf (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bigleaf;
// uc_orig: TEXTURE_page_bigrain (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_bigrain;
// uc_orig: TEXTURE_page_finalglow (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_finalglow;
// uc_orig: TEXTURE_page_tinybutt (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_tinybutt;
// uc_orig: TEXTURE_page_lcdfont_alpha (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_lcdfont_alpha;
// uc_orig: TEXTURE_page_flames_alpha (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_flames_alpha;
// uc_orig: TEXTURE_page_tyretrack_alpha (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_tyretrack_alpha;
// uc_orig: TEXTURE_page_people3 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_people3;
// uc_orig: TEXTURE_page_ladder (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_ladder;
// uc_orig: TEXTURE_page_fadecat (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_fadecat;
// uc_orig: TEXTURE_page_fade_MF (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_fade_MF;
// uc_orig: TEXTURE_page_shadowoval (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_shadowoval;
// uc_orig: TEXTURE_page_rubbish (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_rubbish;
// uc_orig: TEXTURE_page_lastpanel (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_lastpanel;
// uc_orig: TEXTURE_page_lastpanel2 (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_lastpanel2;
// uc_orig: TEXTURE_page_sign (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_sign;
// uc_orig: TEXTURE_page_pcflamer (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_pcflamer;
// uc_orig: TEXTURE_page_shadowsquare (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_shadowsquare;
// uc_orig: TEXTURE_page_litebolt (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_litebolt;
// uc_orig: TEXTURE_page_ladshad (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_ladshad;
// uc_orig: TEXTURE_page_meteor (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_meteor;
// uc_orig: TEXTURE_page_splash (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_splash;

// uc_orig: TEXTURE_num_textures (fallen/DDEngine/Headers/texture.h)
// Total number of texture pages loaded.
extern SLONG TEXTURE_num_textures;

// uc_orig: TEXTURE_get_handle (fallen/DDEngine/Headers/texture.h)
// Returns the Direct3D texture handle for the given page.
LPDIRECT3DTEXTURE2 TEXTURE_get_handle(SLONG page);

// uc_orig: TEXTURE_get_D3DTexture (fallen/DDEngine/Headers/texture.h)
// Returns the D3DTexture object for the given page (used for TexOffset on PolyPages).
D3DTexture* TEXTURE_get_D3DTexture(SLONG page);

// uc_orig: TEXTURE_crinkle (fallen/DDEngine/Headers/texture.h)
// Per-page crinkle handle. NULL => no crinkle for that page.
extern CRINKLE_Handle TEXTURE_crinkle[22 * 64];

// uc_orig: TEXTURE_shadow_bitmap (fallen/DDEngine/Headers/texture.h)
// Pointer to locked shadow texture pixels (valid only between lock/unlock).
extern UWORD* TEXTURE_shadow_bitmap;
// uc_orig: TEXTURE_shadow_pitch (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_pitch;
// uc_orig: TEXTURE_shadow_mask_red (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_red;
// uc_orig: TEXTURE_shadow_mask_green (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_green;
// uc_orig: TEXTURE_shadow_mask_blue (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_blue;
// uc_orig: TEXTURE_shadow_mask_alpha (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_alpha;
// uc_orig: TEXTURE_shadow_shift_red (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_red;
// uc_orig: TEXTURE_shadow_shift_green (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_green;
// uc_orig: TEXTURE_shadow_shift_blue (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_blue;
// uc_orig: TEXTURE_shadow_shift_alpha (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_alpha;

// uc_orig: TEXTURE_shadow_lock (fallen/DDEngine/Headers/texture.h)
SLONG TEXTURE_shadow_lock(void);
// uc_orig: TEXTURE_shadow_unlock (fallen/DDEngine/Headers/texture.h)
void TEXTURE_shadow_unlock(void);
// uc_orig: TEXTURE_shadow_update (fallen/DDEngine/Headers/texture.h)
void TEXTURE_shadow_update(void);

// uc_orig: TEXTURE_86_lock (fallen/DDEngine/Headers/texture.h)
SLONG TEXTURE_86_lock(void);
// uc_orig: TEXTURE_86_unlock (fallen/DDEngine/Headers/texture.h)
void TEXTURE_86_unlock(void);
// uc_orig: TEXTURE_86_update (fallen/DDEngine/Headers/texture.h)
void TEXTURE_86_update(void);

// uc_orig: TEXTURE_get_minitexturebits_uvs (fallen/DDEngine/Headers/texture.h)
// Returns UV coordinates for a MiniTextureBits texture descriptor.
void TEXTURE_get_minitexturebits_uvs(
    UWORD texture,
    SLONG* page,
    float* u0,
    float* v0,
    float* u1,
    float* v1,
    float* u2,
    float* v2,
    float* u3,
    float* v3);

// uc_orig: TEXTURE_get_fiddled_position (fallen/DDEngine/Headers/texture.h)
// Given texture square coords on a page, returns the fiddled page and UV position.
SLONG TEXTURE_get_fiddled_position(
    SLONG square_u,
    SLONG square_v,
    SLONG page,
    float* u,
    float* v);

// uc_orig: TEXTURE_set_colour_key (fallen/DDEngine/Headers/texture.h)
void TEXTURE_set_colour_key(SLONG page);

// uc_orig: TEXTURE_set_greyscale (fallen/DDEngine/Headers/texture.h)
void TEXTURE_set_greyscale(SLONG is_greyscale);

// uc_orig: TEXTURE_set_tga (fallen/DDEngine/Headers/texture.h)
// Replaces a specific texture page with a different TGA file.
void TEXTURE_set_tga(SLONG page, CBYTE* fn);

// uc_orig: TEXTURE_free (fallen/DDEngine/Headers/texture.h)
void TEXTURE_free(void);

// uc_orig: TEXTURE_free_unneeded (fallen/DDEngine/Headers/texture.h)
// Frees all non-frontend texture pages (called at level unload).
void TEXTURE_free_unneeded(void);

// uc_orig: TEXTURE_LOOK_ROAD (fallen/DDEngine/Headers/texture.h)
#define TEXTURE_LOOK_ROAD     0
// uc_orig: TEXTURE_LOOK_GRASS (fallen/DDEngine/Headers/texture.h)
#define TEXTURE_LOOK_GRASS    1
// uc_orig: TEXTURE_LOOK_DIRT (fallen/DDEngine/Headers/texture.h)
#define TEXTURE_LOOK_DIRT     2
// uc_orig: TEXTURE_LOOK_SLIPPERY (fallen/DDEngine/Headers/texture.h)
#define TEXTURE_LOOK_SLIPPERY 3

// uc_orig: TEXTURE_liney (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_liney;
// uc_orig: TEXTURE_av_r (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_av_r;
// uc_orig: TEXTURE_av_g (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_av_g;
// uc_orig: TEXTURE_av_b (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_av_b;

// uc_orig: TEXTURE_looks_like (fallen/DDEngine/Headers/texture.h)
// Returns one of TEXTURE_LOOK_* constants based on pixel analysis of the page.
SLONG TEXTURE_looks_like(SLONG page);

#endif // ASSETS_TEXTURE_H
