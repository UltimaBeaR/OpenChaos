#include "assets/texture_globals.h"

// uc_orig: TEXTURE_texture (fallen/DDEngine/Source/texture.cpp)
D3DTexture TEXTURE_texture[TEXTURE_MAX_TEXTURES];

// uc_orig: TEXTURE_crinkle (fallen/DDEngine/Source/texture.cpp)
CRINKLE_Handle TEXTURE_crinkle[22 * 64];

// uc_orig: TEXTURE_dontexist (fallen/DDEngine/Source/texture.cpp)
UBYTE TEXTURE_dontexist[TEXTURE_MAX_TEXTURES];

// uc_orig: TEXTURE_needed (fallen/DDEngine/Source/texture.cpp)
UBYTE TEXTURE_needed[TEXTURE_MAX_TEXTURES];

// uc_orig: TEXTURE_set (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_set = 0;

// uc_orig: TEXTURE_fiddled (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_fiddled = 0;

// uc_orig: TEXTURE_shadow_bitmap (fallen/DDEngine/Source/texture.cpp)
UWORD* TEXTURE_shadow_bitmap = nullptr;
// uc_orig: TEXTURE_shadow_pitch (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_pitch = 0;
// uc_orig: TEXTURE_shadow_mask_red (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_mask_red = 0;
// uc_orig: TEXTURE_shadow_mask_green (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_mask_green = 0;
// uc_orig: TEXTURE_shadow_mask_blue (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_mask_blue = 0;
// uc_orig: TEXTURE_shadow_mask_alpha (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_mask_alpha = 0;
// uc_orig: TEXTURE_shadow_shift_red (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_shift_red = 0;
// uc_orig: TEXTURE_shadow_shift_green (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_shift_green = 0;
// uc_orig: TEXTURE_shadow_shift_blue (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_shift_blue = 0;
// uc_orig: TEXTURE_shadow_shift_alpha (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_shadow_shift_alpha = 0;

// uc_orig: TEXTURE_shared_dir (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_shared_dir[_MAX_PATH] = "";
// uc_orig: TEXTURE_world_dir (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_world_dir[_MAX_PATH] = "";
// uc_orig: TEXTURE_fx_inifile (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_fx_inifile[_MAX_PATH] = "";
// uc_orig: TEXTURE_shared_fx_inifile (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_shared_fx_inifile[_MAX_PATH] = "";
// uc_orig: TEXTURE_prims_dir (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_prims_dir[_MAX_PATH] = "";
// uc_orig: TEXTURE_inside_dir (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_inside_dir[_MAX_PATH] = "";
// uc_orig: TEXTURE_people_dir (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_people_dir[_MAX_PATH] = "";
// uc_orig: TEXTURE_people_dir2 (fallen/DDEngine/Source/texture.cpp)
CBYTE TEXTURE_people_dir2[_MAX_PATH] = "";

// uc_orig: TEXTURE_num_textures (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_num_textures = 0;

// uc_orig: TEXTURE_page_num_standard (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_num_standard = 0;
// uc_orig: TEXTURE_page_snowflake (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_snowflake = 0;
// uc_orig: TEXTURE_page_sparkle (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_sparkle = 0;
// uc_orig: TEXTURE_page_explode2 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_explode2 = 0;
// uc_orig: TEXTURE_page_explode1 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_explode1 = 0;
// uc_orig: TEXTURE_page_bigbang (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bigbang = 0;
// uc_orig: TEXTURE_page_face1 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_face1 = 0;
// uc_orig: TEXTURE_page_face2 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_face2 = 0;
// uc_orig: TEXTURE_page_face3 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_face3 = 0;
// uc_orig: TEXTURE_page_face4 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_face4 = 0;
// uc_orig: TEXTURE_page_face5 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_face5 = 0;
// uc_orig: TEXTURE_page_face6 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_face6 = 0;
// uc_orig: TEXTURE_page_fog (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_fog = 0;
// uc_orig: TEXTURE_page_moon (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_moon = 0;
// uc_orig: TEXTURE_page_clouds (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_clouds = 0;
// uc_orig: TEXTURE_page_water (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_water = 0;
// uc_orig: TEXTURE_page_puddle (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_puddle = 0;
// uc_orig: TEXTURE_page_drip (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_drip = 0;
// uc_orig: TEXTURE_page_shadow (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_shadow = 0;
// uc_orig: TEXTURE_page_bang (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bang = 0;
// uc_orig: TEXTURE_page_font (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_font = 0;
// uc_orig: TEXTURE_page_logo (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_logo = 0;
// uc_orig: TEXTURE_page_sky (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_sky = 0;
// uc_orig: TEXTURE_page_flames (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_flames = 0;
// uc_orig: TEXTURE_page_smoke (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_smoke = 0;
// uc_orig: TEXTURE_page_flame2 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_flame2 = 0;
// uc_orig: TEXTURE_page_steam (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_steam = 0;
// uc_orig: TEXTURE_page_menuflame (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_menuflame = 0;
// uc_orig: TEXTURE_page_barbwire (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_barbwire = 0;
// uc_orig: TEXTURE_page_font2d (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_font2d = 0;
// uc_orig: TEXTURE_page_dustwave (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_dustwave = 0;
// uc_orig: TEXTURE_page_flames3 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_flames3 = 0;
// uc_orig: TEXTURE_page_bloodsplat (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bloodsplat = 0;
// uc_orig: TEXTURE_page_bloom1 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bloom1 = 0;
// uc_orig: TEXTURE_page_bloom2 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bloom2 = 0;
// uc_orig: TEXTURE_page_hitspang (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_hitspang = 0;
// uc_orig: TEXTURE_page_lensflare (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_lensflare = 0;
// uc_orig: TEXTURE_page_tyretrack (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_tyretrack = 0;
// uc_orig: TEXTURE_page_envmap (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_envmap = 0;
// uc_orig: TEXTURE_page_winmap (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_winmap = 0;
// uc_orig: TEXTURE_page_leaf (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_leaf = 0;
// uc_orig: TEXTURE_page_raindrop (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_raindrop = 0;
// uc_orig: TEXTURE_page_footprint (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_footprint = 0;
// uc_orig: TEXTURE_page_angel (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_angel = 0;
// uc_orig: TEXTURE_page_devil (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_devil = 0;
// uc_orig: TEXTURE_page_smoker (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_smoker = 0;
// uc_orig: TEXTURE_page_target (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_target = 0;
// uc_orig: TEXTURE_page_newfont (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_newfont = 0;
// uc_orig: TEXTURE_page_droplet (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_droplet = 0;
// uc_orig: TEXTURE_page_press1 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_press1 = 0;
// uc_orig: TEXTURE_page_press2 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_press2 = 0;
// uc_orig: TEXTURE_page_ic (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_ic = 0;
// uc_orig: TEXTURE_page_ic2 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_ic2 = 0;
// uc_orig: TEXTURE_page_lcdfont (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_lcdfont = 0;
// uc_orig: TEXTURE_page_smokecloud (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_smokecloud = 0;
// uc_orig: TEXTURE_page_menulogo (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_menulogo = 0;
// uc_orig: TEXTURE_page_polaroid (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_polaroid = 0;
// uc_orig: TEXTURE_page_bigbutton (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bigbutton = 0;
// uc_orig: TEXTURE_page_bigleaf (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bigleaf = 0;
// uc_orig: TEXTURE_page_bigrain (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_bigrain = 0;
// uc_orig: TEXTURE_page_finalglow (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_finalglow = 0;
// uc_orig: TEXTURE_page_tinybutt (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_tinybutt = 0;
// uc_orig: TEXTURE_page_lcdfont_alpha (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_lcdfont_alpha = 0;
// uc_orig: TEXTURE_page_flames_alpha (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_flames_alpha = 0;
// uc_orig: TEXTURE_page_tyretrack_alpha (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_tyretrack_alpha = 0;
// uc_orig: TEXTURE_page_people3 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_people3 = 0;
// uc_orig: TEXTURE_page_ladder (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_ladder = 0;
// uc_orig: TEXTURE_page_fadecat (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_fadecat = 0;
// uc_orig: TEXTURE_page_fade_MF (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_fade_MF = 0;
// uc_orig: TEXTURE_page_shadowoval (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_shadowoval = 0;
// uc_orig: TEXTURE_page_rubbish (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_rubbish = 0;
// uc_orig: TEXTURE_page_lastpanel (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_lastpanel = 0;
// uc_orig: TEXTURE_page_lastpanel2 (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_lastpanel2 = 0;
// uc_orig: TEXTURE_page_sign (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_sign = 0;
// uc_orig: TEXTURE_page_pcflamer (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_pcflamer = 0;
// uc_orig: TEXTURE_page_shadowsquare (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_shadowsquare = 0;
// uc_orig: TEXTURE_page_litebolt (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_litebolt = 0;
// uc_orig: TEXTURE_page_ladshad (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_ladshad = 0;
// uc_orig: TEXTURE_page_meteor (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_meteor = 0;
// uc_orig: TEXTURE_page_splash (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_page_splash = 0;

// uc_orig: TEXTURE_liney (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_liney = 0;
// uc_orig: TEXTURE_av_r (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_av_r = 0;
// uc_orig: TEXTURE_av_g (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_av_g = 0;
// uc_orig: TEXTURE_av_b (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_av_b = 0;

// uc_orig: TEXTURE_create_clump (fallen/DDEngine/Source/texture.cpp)
int TEXTURE_create_clump = 0;

// Alternative texture page lookup for people3 body parts (used by figure.cpp for character skinning).
// Indexed as: alt_texture[page - 10*64] where page > 10*64.
// Non-zero entries remap people2 pages (people_dir) to people3 equivalents (people3 at 21*64).
// uc_orig: alt_texture (fallen/DDEngine/Source/texture.cpp)
#define PEOPLE3_ALT (21 * 64)
#define PEOPLE3_CROTCH   0
#define PEOPLE3_FRONT    3
#define PEOPLE3_HAT_SIDE 6
#define PEOPLE3_HAT_FRONT 9
#define PEOPLE3_LEG      12
#define PEOPLE3_F_ARSE   15
#define PEOPLE3_F_CHEST  18
#define PEOPLE3_F_SEAM   21
#define PEOPLE3_F_SHOE   24
#define PEOPLE3_F_BACK   27

UWORD alt_texture[] = {
    // 0..107: all zeros
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0,
    // 108-118: people2 -> people3 remapping
    PEOPLE3_ALT + PEOPLE3_LEG,
    PEOPLE3_ALT + PEOPLE3_CROTCH,
    0,
    PEOPLE3_ALT + PEOPLE3_FRONT,
    PEOPLE3_ALT + PEOPLE3_HAT_SIDE,
    PEOPLE3_ALT + PEOPLE3_HAT_FRONT,
    PEOPLE3_ALT + PEOPLE3_F_ARSE,
    PEOPLE3_ALT + PEOPLE3_F_SEAM,
    PEOPLE3_ALT + PEOPLE3_F_SHOE,
    PEOPLE3_ALT + PEOPLE3_F_CHEST,
    PEOPLE3_ALT + PEOPLE3_F_BACK,
    // 119..319: zeros (201 entries)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
};

#undef PEOPLE3_ALT
#undef PEOPLE3_CROTCH
#undef PEOPLE3_FRONT
#undef PEOPLE3_HAT_SIDE
#undef PEOPLE3_HAT_FRONT
#undef PEOPLE3_LEG
#undef PEOPLE3_F_ARSE
#undef PEOPLE3_F_CHEST
#undef PEOPLE3_F_SEAM
#undef PEOPLE3_F_SHOE
#undef PEOPLE3_F_BACK

// uc_orig: TEXTURE_DC_pack (fallen/DDEngine/Source/texture.cpp)
TEXTURE_DC_Pack TEXTURE_DC_pack[256];
// uc_orig: TEXTURE_DC_pack_page_upto (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_DC_pack_page_upto = 0;
// uc_orig: TEXTURE_DC_pack_page_pos (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_DC_pack_page_pos = 0;
// uc_orig: TEXTURE_DC_pack_normal_upto (fallen/DDEngine/Source/texture.cpp)
SLONG TEXTURE_DC_pack_normal_upto = 0;

// uc_orig: IndividualTextures (fallen/DDEngine/Source/texture.cpp)
bool IndividualTextures = false;
