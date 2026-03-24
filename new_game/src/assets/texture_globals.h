#ifndef ASSETS_TEXTURE_GLOBALS_H
#define ASSETS_TEXTURE_GLOBALS_H

#include "engine/core/types.h"
#include "engine/graphics/resources/d3d_texture.h"
#include "engine/graphics/lighting/crinkle.h"

// Internal constants also needed by external consumers.
// uc_orig: TEXTURE_NUM_STANDARD (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_NUM_STANDARD (22 * 64)
// uc_orig: TEXTURE_MAX_TEXTURES (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_MAX_TEXTURES (TEXTURE_NUM_STANDARD + 160)

// Struct used in the DC packing system (Dreamcast texture atlas).
// Not used on PC (TEXTURE_ENABLE_DC_PACKING=0) but the init function clears it.
// uc_orig: TEXTURE_DC_Pack (fallen/DDEngine/Source/texture.cpp)
typedef struct {
    UBYTE page;
    UBYTE pos; // 0-8 or TEXTURE_DC_PACK_POS_WHOLE_PAGE (42)
} TEXTURE_DC_Pack;

// uc_orig: TEXTURE_texture (fallen/DDEngine/Source/texture.cpp)
extern D3DTexture TEXTURE_texture[TEXTURE_MAX_TEXTURES];

// uc_orig: TEXTURE_crinkle (fallen/DDEngine/Source/texture.cpp)
extern CRINKLE_Handle TEXTURE_crinkle[22 * 64];

// uc_orig: TEXTURE_dontexist (fallen/DDEngine/Source/texture.cpp)
// Flags set to UC_TRUE for pages where the TGA file does not exist.
extern UBYTE TEXTURE_dontexist[TEXTURE_MAX_TEXTURES];

// uc_orig: TEXTURE_needed (fallen/DDEngine/Source/texture.cpp)
// Frontend/UI textures that must not be freed between levels.
extern UBYTE TEXTURE_needed[TEXTURE_MAX_TEXTURES];

// uc_orig: TEXTURE_set (fallen/DDEngine/Source/texture.cpp)
// Currently active texture world set number.
extern SLONG TEXTURE_set;

// uc_orig: TEXTURE_fiddled (fallen/DDEngine/Source/texture.cpp)
// Non-zero when using fiddled (remapped) texture pages.
extern SLONG TEXTURE_fiddled;

// Shadow texture locked pixel data (set by TEXTURE_shadow_lock).
// uc_orig: TEXTURE_shadow_bitmap (fallen/DDEngine/Source/texture.cpp)
extern UWORD* TEXTURE_shadow_bitmap;
// uc_orig: TEXTURE_shadow_pitch (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_pitch;
// uc_orig: TEXTURE_shadow_mask_red (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_mask_red;
// uc_orig: TEXTURE_shadow_mask_green (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_mask_green;
// uc_orig: TEXTURE_shadow_mask_blue (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_mask_blue;
// uc_orig: TEXTURE_shadow_mask_alpha (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_mask_alpha;
// uc_orig: TEXTURE_shadow_shift_red (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_shift_red;
// uc_orig: TEXTURE_shadow_shift_green (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_shift_green;
// uc_orig: TEXTURE_shadow_shift_blue (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_shift_blue;
// uc_orig: TEXTURE_shadow_shift_alpha (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_shadow_shift_alpha;

// Directory paths set by TEXTURE_choose_set().
// uc_orig: TEXTURE_shared_dir (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_shared_dir[_MAX_PATH];
// uc_orig: TEXTURE_world_dir (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_world_dir[_MAX_PATH];
// uc_orig: TEXTURE_fx_inifile (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_fx_inifile[_MAX_PATH];
// uc_orig: TEXTURE_shared_fx_inifile (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_shared_fx_inifile[_MAX_PATH];
// uc_orig: TEXTURE_prims_dir (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_prims_dir[_MAX_PATH];
// uc_orig: TEXTURE_inside_dir (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_inside_dir[_MAX_PATH];
// uc_orig: TEXTURE_people_dir (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_people_dir[_MAX_PATH];
// uc_orig: TEXTURE_people_dir2 (fallen/DDEngine/Source/texture.cpp)
extern CBYTE TEXTURE_people_dir2[_MAX_PATH];

// uc_orig: TEXTURE_num_textures (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_num_textures;

// Named texture page indices — set during TEXTURE_load_needed().
// uc_orig: TEXTURE_page_num_standard (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_num_standard;
// uc_orig: TEXTURE_page_snowflake (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_snowflake;
// uc_orig: TEXTURE_page_sparkle (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_sparkle;
// uc_orig: TEXTURE_page_explode2 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_explode2;
// uc_orig: TEXTURE_page_explode1 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_explode1;
// uc_orig: TEXTURE_page_bigbang (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bigbang;
// uc_orig: TEXTURE_page_face1 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_face1;
// uc_orig: TEXTURE_page_face2 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_face2;
// uc_orig: TEXTURE_page_face3 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_face3;
// uc_orig: TEXTURE_page_face4 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_face4;
// uc_orig: TEXTURE_page_face5 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_face5;
// uc_orig: TEXTURE_page_face6 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_face6;
// uc_orig: TEXTURE_page_fog (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_fog;
// uc_orig: TEXTURE_page_moon (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_moon;
// uc_orig: TEXTURE_page_clouds (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_clouds;
// uc_orig: TEXTURE_page_water (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_water;
// uc_orig: TEXTURE_page_puddle (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_puddle;
// uc_orig: TEXTURE_page_drip (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_drip;
// uc_orig: TEXTURE_page_shadow (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_shadow;
// uc_orig: TEXTURE_page_bang (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bang;
// uc_orig: TEXTURE_page_font (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_font;
// uc_orig: TEXTURE_page_logo (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_logo;
// uc_orig: TEXTURE_page_sky (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_sky;
// uc_orig: TEXTURE_page_flames (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_flames;
// uc_orig: TEXTURE_page_smoke (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_smoke;
// uc_orig: TEXTURE_page_flame2 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_flame2;
// uc_orig: TEXTURE_page_steam (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_steam;
// uc_orig: TEXTURE_page_menuflame (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_menuflame;
// uc_orig: TEXTURE_page_barbwire (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_barbwire;
// uc_orig: TEXTURE_page_font2d (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_font2d;
// uc_orig: TEXTURE_page_dustwave (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_dustwave;
// uc_orig: TEXTURE_page_flames3 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_flames3;
// uc_orig: TEXTURE_page_bloodsplat (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bloodsplat;
// uc_orig: TEXTURE_page_bloom1 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bloom1;
// uc_orig: TEXTURE_page_bloom2 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bloom2;
// uc_orig: TEXTURE_page_hitspang (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_hitspang;
// uc_orig: TEXTURE_page_lensflare (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_lensflare;
// uc_orig: TEXTURE_page_tyretrack (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_tyretrack;
// uc_orig: TEXTURE_page_envmap (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_envmap;
// uc_orig: TEXTURE_page_winmap (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_winmap;
// uc_orig: TEXTURE_page_leaf (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_leaf;
// uc_orig: TEXTURE_page_raindrop (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_raindrop;
// uc_orig: TEXTURE_page_footprint (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_footprint;
// uc_orig: TEXTURE_page_angel (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_angel;
// uc_orig: TEXTURE_page_devil (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_devil;
// uc_orig: TEXTURE_page_smoker (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_smoker;
// uc_orig: TEXTURE_page_target (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_target;
// uc_orig: TEXTURE_page_newfont (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_newfont;
// uc_orig: TEXTURE_page_droplet (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_droplet;
// uc_orig: TEXTURE_page_press1 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_press1;
// uc_orig: TEXTURE_page_press2 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_press2;
// uc_orig: TEXTURE_page_ic (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_ic;
// uc_orig: TEXTURE_page_ic2 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_ic2;
// uc_orig: TEXTURE_page_lcdfont (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_lcdfont;
// uc_orig: TEXTURE_page_smokecloud (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_smokecloud;
// uc_orig: TEXTURE_page_menulogo (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_menulogo;
// uc_orig: TEXTURE_page_polaroid (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_polaroid;
// uc_orig: TEXTURE_page_bigbutton (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bigbutton;
// uc_orig: TEXTURE_page_bigleaf (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bigleaf;
// uc_orig: TEXTURE_page_bigrain (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_bigrain;
// uc_orig: TEXTURE_page_finalglow (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_finalglow;
// uc_orig: TEXTURE_page_tinybutt (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_tinybutt;
// uc_orig: TEXTURE_page_lcdfont_alpha (fallen/DDEngine/Source/texture.cpp)
// Note: Not set in TEXTURE_load_needed (was unused in this build).
extern SLONG TEXTURE_page_lcdfont_alpha;
// uc_orig: TEXTURE_page_flames_alpha (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_flames_alpha;
// uc_orig: TEXTURE_page_tyretrack_alpha (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_tyretrack_alpha;
// uc_orig: TEXTURE_page_people3 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_people3;
// uc_orig: TEXTURE_page_ladder (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_ladder;
// uc_orig: TEXTURE_page_fadecat (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_fadecat;
// uc_orig: TEXTURE_page_fade_MF (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_fade_MF;
// uc_orig: TEXTURE_page_shadowoval (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_shadowoval;
// uc_orig: TEXTURE_page_rubbish (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_rubbish;
// uc_orig: TEXTURE_page_lastpanel (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_lastpanel;
// uc_orig: TEXTURE_page_lastpanel2 (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_lastpanel2;
// uc_orig: TEXTURE_page_sign (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_sign;
// uc_orig: TEXTURE_page_pcflamer (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_pcflamer;
// uc_orig: TEXTURE_page_shadowsquare (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_shadowsquare;
// uc_orig: TEXTURE_page_litebolt (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_litebolt;
// uc_orig: TEXTURE_page_ladshad (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_ladshad;
// uc_orig: TEXTURE_page_meteor (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_meteor;
// uc_orig: TEXTURE_page_splash (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_page_splash;

// TEXTURE_liney, TEXTURE_av_r/g/b — set as side-effect of TEXTURE_looks_like().
// uc_orig: TEXTURE_liney (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_liney;
// uc_orig: TEXTURE_av_r (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_av_r;
// uc_orig: TEXTURE_av_g (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_av_g;
// uc_orig: TEXTURE_av_b (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_av_b;

// Alternative texture mapping table for people3 body-part substitution.
// Used by figure.cpp for character skinning.
// uc_orig: alt_texture (fallen/DDEngine/Source/texture.cpp)
extern UWORD alt_texture[];

// Whether TEXTURE_create_clump is enabled (creates .txc bundles rather than loading them).
// uc_orig: TEXTURE_create_clump (fallen/DDEngine/Source/texture.cpp)
extern int TEXTURE_create_clump;

// DC packing atlas state (Dreamcast-only, TEXTURE_ENABLE_DC_PACKING=0 on PC).
// uc_orig: TEXTURE_DC_pack (fallen/DDEngine/Source/texture.cpp)
extern TEXTURE_DC_Pack TEXTURE_DC_pack[256];
// uc_orig: TEXTURE_DC_pack_page_upto (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_DC_pack_page_upto;
// uc_orig: TEXTURE_DC_pack_page_pos (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_DC_pack_page_pos;
// uc_orig: TEXTURE_DC_pack_normal_upto (fallen/DDEngine/Source/texture.cpp)
extern SLONG TEXTURE_DC_pack_normal_upto;

// Whether textures load individually from TGA files rather than from .txc bundle.
// uc_orig: IndividualTextures (fallen/DDEngine/Source/texture.cpp)
extern bool IndividualTextures;

#endif // ASSETS_TEXTURE_GLOBALS_H
