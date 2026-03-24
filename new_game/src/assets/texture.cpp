#include <platform.h>
#include <string.h>
#include <stdlib.h>
#include "missions/game_types.h"
#include "assets/texture.h"
#include "assets/texture_globals.h"
#include "assets/anim_globals.h"   // next_prim_face3, next_prim_face4
#include "engine/graphics/pipeline/aeng.h"
#include "assets/anim_tmap.h"
#include "world/map/supermap.h"
#include "engine/graphics/pipeline/poly.h"
#include "world/map/pap.h"
#include "engine/graphics/graphics_api/gd_display.h"
#include "engine/graphics/resources/d3d_texture.h"
#include "engine/lighting/crinkle.h"
#include "engine/graphics/pipeline/message.h"
#include "engine/audio/sound.h"
#include "world/environment/ware.h"
#include "engine/graphics/resources/truetype.h"
#include "engine/graphics/resources/font2d.h"
#include "engine/io/env.h"
#include "assets/tga.h"
#include "world/level_pools.h"
#include "assets/level_loader.h"
#include "assets/level_loader_globals.h"
#include "assets/anim_loader.h"
#include "assets/anim_loader_globals.h"
#include "world/environment/building_types.h"  // TEXTURE_PIECE_NUMBER, STOREY_TYPE_*, FACET_FLAG_2SIDED
#include "world/environment/building_globals.h" // dx_textures_xy, building_list
#include "world/map/supermap_globals.h" // next_dfacet
#include "world/environment/ware_globals.h" // WARE_rooftex, WARE_rooftex_upto
#include "world/map/map.h" // MAP_WIDTH, MAP_HEIGHT

// Internal page-count constants.
// uc_orig: TEXTURE_NORM_SIZE (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_NORM_SIZE    32
// uc_orig: TEXTURE_NORM_SQUARES (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_NORM_SQUARES 8

// DC packing system state (Dreamcast feature, disabled on PC).
// uc_orig: TEXTURE_ENABLE_DC_PACKING (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_ENABLE_DC_PACKING      0
// uc_orig: TEXTURE_DC_PACK_POS_WHOLE_PAGE (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_DC_PACK_POS_WHOLE_PAGE 42
// uc_orig: TEXTURE_DC_NORMAL_START (fallen/DDEngine/Source/texture.cpp)
#define TEXTURE_DC_NORMAL_START        128

// uc_orig: TEXTURE_DC_pack_init (fallen/DDEngine/Source/texture.cpp)
static void TEXTURE_DC_pack_init(void)
{
    memset(TEXTURE_DC_pack, 0, sizeof(TEXTURE_DC_pack));
    TEXTURE_DC_pack_page_upto = 0;
    TEXTURE_DC_pack_page_pos = 0;
    TEXTURE_DC_pack_normal_upto = TEXTURE_DC_NORMAL_START;
}


// uc_orig: TEXTURE_choose_set (fallen/DDEngine/Source/texture.cpp)
// Switches the active texture set (world n). Frees previously loaded world textures,
// updates directory paths, reloads texture flags and style definitions.
void TEXTURE_choose_set(SLONG number)
{
    SLONG i;
    CBYTE textures[] = "textures";

    if (FileExists("psx.txt")) {
        sprintf(textures, "gary16");
    }

    sprintf(TEXTURE_inside_dir, "server\\%s\\world%d\\insides\\", textures, number);
    sprintf(TEXTURE_prims_dir, "server\\%s\\shared\\prims\\", textures);
    sprintf(TEXTURE_people_dir, "server\\%s\\shared\\people\\", textures);
    sprintf(TEXTURE_people_dir2, "server\\%s\\shared\\people2\\", textures);
    sprintf(TEXTURE_world_dir, "server\\%s\\world%d\\", textures, number);
    sprintf(TEXTURE_shared_dir, "server\\%s\\shared\\", textures, number);
    strcpy(TEXTURE_WORLD_DIR, TEXTURE_world_dir);
    sprintf(TEXTURE_fx_inifile, "%ssoundfx.ini", TEXTURE_world_dir);
    sprintf(TEXTURE_shared_fx_inifile, "%ssoundfx.ini", TEXTURE_shared_dir);
    SOUND_InitFXGroups(TEXTURE_shared_fx_inifile);

    if (number != TEXTURE_set) {
        CRINKLE_init();

        for (i = 0; i < 64 * 4; i++) {
            if (TEXTURE_texture[i].Type != D3DTEXTURE_TYPE_UNUSED) {
                TEXTURE_texture[i].Destroy();
                TEXTURE_texture[i].Type = D3DTEXTURE_TYPE_UNUSED;
            }
        }

        memset(TEXTURE_dontexist, 0, sizeof(TEXTURE_dontexist));
        memset(TEXTURE_crinkle, 0, sizeof(TEXTURE_crinkle));

        {
            CBYTE world_texture_flags[256];
            CBYTE shared_texture_flags[256];

            sprintf(world_texture_flags, "%stextype.txt", TEXTURE_world_dir);
            sprintf(shared_texture_flags, "%stextype.txt", TEXTURE_shared_dir);

            POLY_init_texture_flags();
            POLY_load_texture_flags(world_texture_flags);
            POLY_load_texture_flags(shared_texture_flags);
            POLY_load_texture_flags("server\\textures\\shared\\prims\\textype.txt", 11 * 64);
        }

        extern void load_texture_styles(UBYTE editor, UBYTE world);
        load_texture_styles(UC_FALSE, number);
        extern void load_texture_instyles(UBYTE editor, UBYTE world);
        load_texture_instyles(UC_FALSE, number);
        TEXTURE_fix_texture_styles();
    }

    TEXTURE_set = number;
}

// uc_orig: TEXTURE_load_page (fallen/DDEngine/Source/texture.cpp)
// Loads one texture page from disk into TEXTURE_texture[page].
// Searches for highest-res TGA available (128 > 64 > 32 pixels).
// Sets TEXTURE_dontexist[page] if no file found.
// Also loads crinkle bump data (.sex file) for world/shared pages.
// If the page is masked-self-illuminating (2PASS), auto-loads page+1 as the mask.
static void TEXTURE_load_page(SLONG page)
{
    CBYTE name_res32[64];
    CBYTE name_res64[64];
    CBYTE name_res128[64];
    CBYTE name_sex[64] = "";
    CBYTE shortname_res32[14];
    CBYTE shortname_res64[14];
    CBYTE shortname_res128[14];
    SLONG fxref;

    FILE* exists32;
    FILE* exists64;
    FILE* exists128;

    ASSERT(TEXTURE_set);
    ASSERT(WITHIN(page, 0, TEXTURE_MAX_TEXTURES - 1));

    if (TEXTURE_dontexist[page]) {
        return;
    }

    if (page < 64 * 4) {
        sprintf(name_res32, "%stex%03d.tga", TEXTURE_world_dir, page);
        sprintf(name_res64, "%stex%03dhi.tga", TEXTURE_world_dir, page);
        sprintf(name_res128, "%stex%03dto.tga", TEXTURE_world_dir, page);
        sprintf(name_sex, "%ssex%03dhi.sex", TEXTURE_world_dir, page);
        sprintf(shortname_res32, "tex%03d.tga", page);
        sprintf(shortname_res64, "tex%03dhi.tga", page);
        sprintf(shortname_res128, "tex%03dto.tga", page);

        fxref = GetPrivateProfileInt("Textures", shortname_res64, -1, TEXTURE_fx_inifile);
        if (fxref == -1) {
            fxref = GetPrivateProfileInt("Textures", shortname_res32, -1, TEXTURE_fx_inifile);
        }

        if (SOUND_FXMapping != NULL) {
            SOUND_FXMapping[page] = fxref;
        }
    } else if (page < 64 * 8) {
        sprintf(name_res32, "%stex%03d.tga", TEXTURE_shared_dir, page);
        sprintf(name_res64, "%stex%03dhi.tga", TEXTURE_shared_dir, page);
        sprintf(name_res128, "%stex%03dto.tga", TEXTURE_shared_dir, page);
        sprintf(name_sex, "%ssex%03dhi.sex", TEXTURE_shared_dir, page);
        sprintf(shortname_res32, "tex%03d.tga", page);
        sprintf(shortname_res64, "tex%03dhi.tga", page);
        sprintf(shortname_res128, "tex%03dto.tga", page);

        fxref = GetPrivateProfileInt("Textures", shortname_res64, -1, TEXTURE_shared_fx_inifile);
        if (fxref == -1) {
            fxref = GetPrivateProfileInt("Textures", shortname_res32, -1, TEXTURE_shared_fx_inifile);
        }

        if (SOUND_FXMapping != NULL) {
            SOUND_FXMapping[page] = fxref;
        }
    } else if (page < 64 * 9) {
        {
            sprintf(name_res32, "%stex%03d.tga", TEXTURE_inside_dir, page - 64 * 8);
            sprintf(name_res64, "%stex%03dhi.tga", TEXTURE_inside_dir, page - 64 * 8);
            sprintf(name_res128, "%stex%03dto.tga", TEXTURE_inside_dir, page - 64 * 8);
        }
    } else if (page < 64 * 11) {
        sprintf(name_res32, "%stex%03d.tga", TEXTURE_people_dir, page - 64 * 9);
        sprintf(name_res64, "%stex%03dhi.tga", TEXTURE_people_dir, page - 64 * 9);
        sprintf(name_res128, "%stex%03dto.tga", TEXTURE_people_dir, page - 64 * 9);
    } else if (page < 64 * 18) {
        sprintf(name_res32, "%stex%03d.tga", TEXTURE_prims_dir, page - 64 * 11);
        sprintf(name_res64, "%stex%03dhi.tga", TEXTURE_prims_dir, page - 64 * 11);
        sprintf(name_res128, "%stex%03dto.tga", TEXTURE_prims_dir, page - 64 * 11);
    } else if (page < 64 * 21) {
        sprintf(name_res32, "%stex%03d.tga", TEXTURE_people_dir2, page - 64 * 18);
        sprintf(name_res64, "%stex%03dhi.tga", TEXTURE_people_dir2, page - 64 * 18);
        sprintf(name_res128, "%stex%03dto.tga", TEXTURE_people_dir2, page - 64 * 18);
    } else {
        ASSERT(0);
    }

    if (IndividualTextures || TEXTURE_create_clump) {
        exists128 = MF_Fopen(name_res128, "rb");
        if (exists128) {
            MF_Fclose(exists128);
            TEXTURE_texture[page].LoadTextureTGA(name_res128, page);
        } else {
            exists64 = MF_Fopen(name_res64, "rb");
            if (exists64) {
                MF_Fclose(exists64);
                TEXTURE_texture[page].LoadTextureTGA(name_res64, page);
            } else {
                exists32 = MF_Fopen(name_res32, "rb");
                if (exists32) {
                    MF_Fclose(exists32);
                    TEXTURE_texture[page].LoadTextureTGA(name_res32, page);
                } else {
                    TEXTURE_dontexist[page] = UC_TRUE;
                }
            }
        }
    } else {
        if (DoesTGAExist(name_res64, page)) {
            TEXTURE_texture[page].LoadTextureTGA(name_res64, page);
        } else if (DoesTGAExist(name_res32, page)) {
            TEXTURE_texture[page].LoadTextureTGA(name_res32, page);
        } else {
            TEXTURE_dontexist[page] = UC_TRUE;
        }
    }

    // Load crinkle (bump) data for world and shared texture pages.
    if (page < 64 * 8) {
        if (IndividualTextures || TEXTURE_create_clump) {
            TEXTURE_crinkle[page] = CRINKLE_load(name_sex);
            if (TEXTURE_create_clump && TEXTURE_crinkle[page]) {
                CRINKLE_write_bin(GetTGAClump(), TEXTURE_crinkle[page], TEXTURE_MAX_TEXTURES + page);
            }
        } else {
            if (!DoesTGAExist("", TEXTURE_MAX_TEXTURES + page)) {
                TEXTURE_crinkle[page] = NULL;
            } else {
                TEXTURE_crinkle[page] = CRINKLE_read_bin(GetTGAClump(), TEXTURE_MAX_TEXTURES + page);
            }
        }
    }

    if (POLY_page_is_masked_self_illuminating(page)) {
        if (TEXTURE_texture[page + 1].Type == D3DTEXTURE_TYPE_UNUSED) {
            TEXTURE_load_page(page + 1);
        }
    }
}

// uc_orig: TEXTURE_initialise_clumping (fallen/DDEngine/Source/texture.cpp)
// Sets up the .txc texture bundle for fast level loading.
// On PC, all textures for a level are pre-bundled into a single .txc file.
void TEXTURE_initialise_clumping(CBYTE* fname_level)
{
    int clumping = 1;

    extern void SetLastClumpfile(char* file, size_t size);

    if (!clumping) {
        IndividualTextures = true;
        SetLastClumpfile("", 0);
    } else {
        char filename[256];
        char* leafname;

        do {
            leafname = fname_level;
            while (*fname_level && (*fname_level != '\\'))
                fname_level++;
        } while (*fname_level++ == '\\');

        sprintf(filename, ".\\clumps\\");
        char* fptr = filename + strlen(filename);
        while (*leafname != '.')
            *fptr++ = *leafname++;
        strcpy(fptr, ".txc");

        OpenTGAClump(filename, TEXTURE_MAX_TEXTURES + 64 * 8, !TEXTURE_create_clump);
        IndividualTextures = false;
        SetLastClumpfile(filename, TEXTURE_MAX_TEXTURES + 64 * 8);
    }
}

// uc_orig: TEXTURE_load_needed (fallen/DDEngine/Source/texture.cpp)
// Loads all textures needed for the given level file.
// Loads special effect textures (fog, sky, fire, etc.) then world textures
// referenced by the map, buildings, and prims. Shows a loading bar during load.
void TEXTURE_load_needed(CBYTE* fname_level,
    int iStartCompletionBar,
    int iEndCompletionBar,
    int iNumberTexturesProbablyLoaded)
{
    SLONG i, j, k;
    SLONG x;
    SLONG z;

    PrimFace3* f3;
    PrimFace4* f4;

    SLONG page;
    float u[4];
    float v[4];

    SLONG c0, c1;

    MapElement* me;

    extern UBYTE loading_screen_active;

#define HOW_MANY_UPDATES 20
    int iNumTexturesLoaded = 0;
    int iNumTexturesToDoNextChunk = iNumberTexturesProbablyLoaded / HOW_MANY_UPDATES;
    int iCurChunkVal = iStartCompletionBar;

    extern void ATTRACT_loadscreen_draw(SLONG completion);

#define LOADED_THIS_MANY_TEXTURES(numtex)                                              \
    iNumTexturesLoaded += numtex;                                                      \
    while (iNumTexturesLoaded > iNumTexturesToDoNextChunk) {                           \
        iNumTexturesToDoNextChunk += iNumberTexturesProbablyLoaded / HOW_MANY_UPDATES; \
        iCurChunkVal += (iEndCompletionBar - iStartCompletionBar) / HOW_MANY_UPDATES;  \
        ATTRACT_loadscreen_draw(iCurChunkVal);                                         \
    }

    TEXTURE_free();

    D3DTexture::BeginLoading();

    TEXTURE_initialise_clumping(fname_level);

    TEXTURE_load_page(1);

    TEXTURE_page_num_standard = TEXTURE_NUM_STANDARD + 0;

    TEXTURE_page_fog = TEXTURE_NUM_STANDARD + 0;
    TEXTURE_page_moon = TEXTURE_NUM_STANDARD + 1;
    TEXTURE_page_clouds = TEXTURE_NUM_STANDARD + 2;
    TEXTURE_page_water = TEXTURE_NUM_STANDARD + 3;
    TEXTURE_page_puddle = TEXTURE_NUM_STANDARD + 4;
    TEXTURE_page_drip = TEXTURE_NUM_STANDARD + 5;
    TEXTURE_page_shadow = TEXTURE_NUM_STANDARD + 6;
    TEXTURE_page_bang = TEXTURE_NUM_STANDARD + 7;
    TEXTURE_page_font = TEXTURE_NUM_STANDARD + 8;
    TEXTURE_page_logo = TEXTURE_NUM_STANDARD + 9;
    TEXTURE_page_sky = TEXTURE_NUM_STANDARD + 10;
    TEXTURE_page_flames = TEXTURE_NUM_STANDARD + 11;
    TEXTURE_page_smoke = TEXTURE_NUM_STANDARD + 12;
    TEXTURE_page_flame2 = TEXTURE_NUM_STANDARD + 13;
    TEXTURE_page_steam = TEXTURE_NUM_STANDARD + 14;
    TEXTURE_page_menuflame = TEXTURE_NUM_STANDARD + 15;
    TEXTURE_page_barbwire = TEXTURE_NUM_STANDARD + 16;
    TEXTURE_page_font2d = TEXTURE_NUM_STANDARD + 17;
    TEXTURE_page_face1 = TEXTURE_NUM_STANDARD + 18;
    TEXTURE_page_face2 = TEXTURE_NUM_STANDARD + 19;
    TEXTURE_page_face3 = TEXTURE_NUM_STANDARD + 20;
    TEXTURE_page_face4 = TEXTURE_NUM_STANDARD + 21;
    TEXTURE_page_face5 = TEXTURE_NUM_STANDARD + 22;
    TEXTURE_page_face6 = TEXTURE_NUM_STANDARD + 23;
    TEXTURE_page_bigbang = TEXTURE_NUM_STANDARD + 24;
    TEXTURE_page_dustwave = TEXTURE_NUM_STANDARD + 25;
    TEXTURE_page_flames3 = TEXTURE_NUM_STANDARD + 26;
    TEXTURE_page_bloodsplat = TEXTURE_NUM_STANDARD + 27;
    TEXTURE_page_bloom1 = TEXTURE_NUM_STANDARD + 28;
    TEXTURE_page_bloom2 = TEXTURE_NUM_STANDARD + 29;
    TEXTURE_page_hitspang = TEXTURE_NUM_STANDARD + 30;
    TEXTURE_page_lensflare = TEXTURE_NUM_STANDARD + 31;
    TEXTURE_page_envmap = TEXTURE_NUM_STANDARD + 32;
    TEXTURE_page_tyretrack = TEXTURE_NUM_STANDARD + 33;
    TEXTURE_page_winmap = TEXTURE_NUM_STANDARD + 34;
    TEXTURE_page_leaf = TEXTURE_NUM_STANDARD + 35;
    TEXTURE_page_raindrop = TEXTURE_NUM_STANDARD + 36;
    TEXTURE_page_footprint = TEXTURE_NUM_STANDARD + 37;
    TEXTURE_page_angel = TEXTURE_NUM_STANDARD + 38;
    TEXTURE_page_devil = TEXTURE_NUM_STANDARD + 39;
    TEXTURE_page_smoker = TEXTURE_NUM_STANDARD + 40;
    TEXTURE_page_target = TEXTURE_NUM_STANDARD + 41;
    TEXTURE_page_newfont = TEXTURE_NUM_STANDARD + 42;
    TEXTURE_page_droplet = TEXTURE_NUM_STANDARD + 43;
    TEXTURE_page_press1 = TEXTURE_NUM_STANDARD + 44;
    TEXTURE_page_press2 = TEXTURE_NUM_STANDARD + 45;
    TEXTURE_page_ic = TEXTURE_NUM_STANDARD + 46;
    TEXTURE_page_ic2 = TEXTURE_NUM_STANDARD + 47;
    TEXTURE_page_explode1 = TEXTURE_NUM_STANDARD + 48;
    TEXTURE_page_explode2 = TEXTURE_NUM_STANDARD + 49;
    TEXTURE_page_lcdfont = TEXTURE_NUM_STANDARD + 50;
    TEXTURE_page_smokecloud = TEXTURE_NUM_STANDARD + 51;
    TEXTURE_page_menulogo = TEXTURE_NUM_STANDARD + 52;
    TEXTURE_page_polaroid = TEXTURE_NUM_STANDARD + 53;
    TEXTURE_page_sparkle = TEXTURE_NUM_STANDARD + 54;
    TEXTURE_page_bigbutton = TEXTURE_NUM_STANDARD + 55;
    TEXTURE_page_bigleaf = TEXTURE_NUM_STANDARD + 56;
    TEXTURE_page_bigrain = TEXTURE_NUM_STANDARD + 57;
    TEXTURE_page_finalglow = TEXTURE_NUM_STANDARD + 58;
    TEXTURE_page_tinybutt = TEXTURE_NUM_STANDARD + 59;
    TEXTURE_page_tyretrack_alpha = TEXTURE_NUM_STANDARD + 60;
    TEXTURE_page_ladder = TEXTURE_NUM_STANDARD + 61;
    TEXTURE_page_fadecat = TEXTURE_NUM_STANDARD + 62;
    TEXTURE_page_shadowoval = TEXTURE_NUM_STANDARD + 63;
    TEXTURE_page_rubbish = TEXTURE_NUM_STANDARD + 64;
    TEXTURE_page_lastpanel = TEXTURE_NUM_STANDARD + 65;
    TEXTURE_page_pcflamer = TEXTURE_NUM_STANDARD + 66;
    TEXTURE_page_sign = TEXTURE_NUM_STANDARD + 67;
    TEXTURE_page_shadowsquare = TEXTURE_NUM_STANDARD + 68;
    TEXTURE_page_lastpanel2 = TEXTURE_NUM_STANDARD + 69;
    TEXTURE_page_litebolt = TEXTURE_NUM_STANDARD + 70;
    TEXTURE_page_ladshad = TEXTURE_NUM_STANDARD + 71;
    TEXTURE_page_meteor = TEXTURE_NUM_STANDARD + 72;
    TEXTURE_page_splash = TEXTURE_NUM_STANDARD + 73;
    TEXTURE_page_snowflake = TEXTURE_NUM_STANDARD + 74;
    TEXTURE_page_fade_MF = TEXTURE_NUM_STANDARD + 75;

    TEXTURE_num_textures = TEXTURE_NUM_STANDARD + 90 + 20;

#define TEXTURE_EXTRA_DIR "server\\textures\\extras\\"
#define TEXTURE_PEOPLE3_DIR "server\\textures\\shared\\people3\\"

    TEXTURE_texture[TEXTURE_page_font].FontOn();
    TEXTURE_needed[TEXTURE_page_font] = 1;

    TEXTURE_texture[TEXTURE_page_lcdfont].Font2On();
    TEXTURE_needed[TEXTURE_page_lcdfont] = 1;

    TEXTURE_texture[TEXTURE_page_fog].LoadTextureTGA(TEXTURE_EXTRA_DIR "fog.tga", TEXTURE_page_fog);
    TEXTURE_texture[TEXTURE_page_moon].LoadTextureTGA(TEXTURE_EXTRA_DIR "moon.tga", TEXTURE_page_moon);
    TEXTURE_texture[TEXTURE_page_clouds].LoadTextureTGA(TEXTURE_EXTRA_DIR "clouds.tga", TEXTURE_page_clouds);
    TEXTURE_texture[TEXTURE_page_water].LoadTextureTGA(TEXTURE_EXTRA_DIR "water.tga", TEXTURE_page_water);
    TEXTURE_texture[TEXTURE_page_puddle].LoadTextureTGA(TEXTURE_EXTRA_DIR "puddle01.tga", TEXTURE_page_puddle);
    LOADED_THIS_MANY_TEXTURES(5);
    TEXTURE_texture[TEXTURE_page_drip].LoadTextureTGA(TEXTURE_EXTRA_DIR "drip.tga", TEXTURE_page_drip);
    TEXTURE_texture[TEXTURE_page_shadow].CreateUserPage(TEXTURE_SHADOW_SIZE, the_display.GetDeviceInfo()->DestInvSourceColourSupported() ? UC_FALSE : UC_TRUE);
    TEXTURE_texture[TEXTURE_page_bang].LoadTextureTGA(TEXTURE_EXTRA_DIR "fireball.tga", TEXTURE_page_bang);
    TEXTURE_texture[TEXTURE_page_font].LoadTextureTGA(TEXTURE_EXTRA_DIR "font.tga", TEXTURE_page_font, UC_FALSE);
    TEXTURE_needed[TEXTURE_page_font] = 1;
    LOADED_THIS_MANY_TEXTURES(5);
    {
        CBYTE str[100];
        sprintf(str, "%ssky.tga", TEXTURE_world_dir);
        TEXTURE_texture[TEXTURE_page_sky].LoadTextureTGA(str, TEXTURE_page_sky, UC_FALSE);
    }

    TEXTURE_texture[TEXTURE_page_flames].LoadTextureTGA(TEXTURE_EXTRA_DIR "flame1.tga", TEXTURE_page_flames);
    TEXTURE_texture[TEXTURE_page_smoke].LoadTextureTGA(TEXTURE_EXTRA_DIR "smoke1.tga", TEXTURE_page_smoke);
    TEXTURE_texture[TEXTURE_page_flame2].LoadTextureTGA(TEXTURE_EXTRA_DIR "explode4.tga", TEXTURE_page_flame2);
    TEXTURE_texture[TEXTURE_page_steam].LoadTextureTGA(TEXTURE_EXTRA_DIR "fog_na.tga", TEXTURE_page_steam);
    TEXTURE_texture[TEXTURE_page_barbwire].LoadTextureTGA(TEXTURE_EXTRA_DIR "barbed.tga", TEXTURE_page_barbwire);
    LOADED_THIS_MANY_TEXTURES(6);

    TEXTURE_texture[TEXTURE_page_font2d].LoadTextureTGA(TEXTURE_EXTRA_DIR "multifontPC.tga", TEXTURE_page_font2d, UC_FALSE);
    TEXTURE_needed[TEXTURE_page_font2d] = 1;

    TEXTURE_texture[TEXTURE_page_lcdfont].LoadTextureTGA(TEXTURE_EXTRA_DIR "olyfont2.tga", TEXTURE_page_lcdfont, UC_FALSE);
    TEXTURE_needed[TEXTURE_page_lcdfont] = 1;

    TEXTURE_texture[TEXTURE_page_lastpanel].LoadTextureTGA(TEXTURE_EXTRA_DIR "PCdisplay.tga", TEXTURE_page_lastpanel, UC_FALSE);
    TEXTURE_needed[TEXTURE_page_lastpanel] = 1;

    FONT2D_init(TEXTURE_page_font2d);
    TEXTURE_texture[TEXTURE_page_face1].LoadTextureTGA(TEXTURE_EXTRA_DIR "face1.tga", TEXTURE_page_face1);
    TEXTURE_texture[TEXTURE_page_face2].LoadTextureTGA(TEXTURE_EXTRA_DIR "face2.tga", TEXTURE_page_face2);
    LOADED_THIS_MANY_TEXTURES(5);
    TEXTURE_texture[TEXTURE_page_bigbang].LoadTextureTGA(TEXTURE_EXTRA_DIR "exp_gunk.tga", TEXTURE_page_bigbang);
    TEXTURE_texture[TEXTURE_page_dustwave].LoadTextureTGA(TEXTURE_EXTRA_DIR "shockwave.tga", TEXTURE_page_dustwave);
    TEXTURE_texture[TEXTURE_page_flames3].LoadTextureTGA(TEXTURE_EXTRA_DIR "flame3.tga", TEXTURE_page_flames3);
    TEXTURE_texture[TEXTURE_page_bloodsplat].LoadTextureTGA(TEXTURE_EXTRA_DIR "bludsplt.tga", TEXTURE_page_bloodsplat);
    TEXTURE_texture[TEXTURE_page_bloom1].LoadTextureTGA(TEXTURE_EXTRA_DIR "bloom4.tga", TEXTURE_page_bloom1);
    LOADED_THIS_MANY_TEXTURES(5);
    TEXTURE_texture[TEXTURE_page_bloom2].LoadTextureTGA(TEXTURE_EXTRA_DIR "bloom2.tga", TEXTURE_page_bloom2);
    TEXTURE_texture[TEXTURE_page_hitspang].LoadTextureTGA(TEXTURE_EXTRA_DIR "hitspang.tga", TEXTURE_page_hitspang);
    TEXTURE_texture[TEXTURE_page_lensflare].LoadTextureTGA(TEXTURE_EXTRA_DIR "lensflar.tga", TEXTURE_page_lensflare);
    TEXTURE_texture[TEXTURE_page_envmap].LoadTextureTGA(TEXTURE_EXTRA_DIR "envmap.tga", TEXTURE_page_envmap);
    TEXTURE_texture[TEXTURE_page_tyretrack].LoadTextureTGA(TEXTURE_EXTRA_DIR "tyremark.tga", TEXTURE_page_tyretrack);
    LOADED_THIS_MANY_TEXTURES(5);
    TEXTURE_texture[TEXTURE_page_winmap].LoadTextureTGA(TEXTURE_EXTRA_DIR "winmap.tga", TEXTURE_page_winmap);
    TEXTURE_texture[TEXTURE_page_leaf].LoadTextureTGA(TEXTURE_EXTRA_DIR "leaf.tga", TEXTURE_page_leaf);
    TEXTURE_texture[TEXTURE_page_raindrop].LoadTextureTGA(TEXTURE_EXTRA_DIR "raindrop.tga", TEXTURE_page_raindrop);
    TEXTURE_texture[TEXTURE_page_footprint].LoadTextureTGA(TEXTURE_EXTRA_DIR "footprint.tga", TEXTURE_page_footprint);
    TEXTURE_texture[TEXTURE_page_smoker].LoadTextureTGA(TEXTURE_EXTRA_DIR "smoker2.tga", TEXTURE_page_smoker);
    TEXTURE_texture[TEXTURE_page_target].LoadTextureTGA(TEXTURE_EXTRA_DIR "targ1.tga", TEXTURE_page_target);
    TEXTURE_texture[TEXTURE_page_droplet].LoadTextureTGA(TEXTURE_EXTRA_DIR "droplet.tga", TEXTURE_page_droplet);
    LOADED_THIS_MANY_TEXTURES(7);
    TEXTURE_texture[TEXTURE_page_explode1].LoadTextureTGA(TEXTURE_EXTRA_DIR "explode1.tga", TEXTURE_page_explode1);
    TEXTURE_texture[TEXTURE_page_explode2].LoadTextureTGA(TEXTURE_EXTRA_DIR "explode2.tga", TEXTURE_page_explode2);
    TEXTURE_texture[TEXTURE_page_smokecloud].LoadTextureTGA(TEXTURE_EXTRA_DIR "explode3.tga", TEXTURE_page_smokecloud);
    TEXTURE_texture[TEXTURE_page_menulogo].LoadTextureTGA(TEXTURE_EXTRA_DIR "menulogo.tga", TEXTURE_page_menulogo);
    TEXTURE_needed[TEXTURE_page_menulogo] = 1;
    LOADED_THIS_MANY_TEXTURES(4);
    TEXTURE_texture[TEXTURE_page_sparkle].LoadTextureTGA(TEXTURE_EXTRA_DIR "sparkle.tga", TEXTURE_page_sparkle);
    TEXTURE_texture[TEXTURE_page_pcflamer].LoadTextureTGA(TEXTURE_EXTRA_DIR "PCflamer.tga", TEXTURE_page_pcflamer);
    TEXTURE_texture[TEXTURE_page_litebolt].LoadTextureTGA(TEXTURE_EXTRA_DIR "litebolt2.tga", TEXTURE_page_litebolt);
    TEXTURE_texture[TEXTURE_page_splash].LoadTextureTGA(TEXTURE_EXTRA_DIR "splashALL.tga", TEXTURE_page_splash);
    LOADED_THIS_MANY_TEXTURES(4);

    // Frontend/UI textures
    TEXTURE_texture[TEXTURE_page_bigbutton].LoadTextureTGA(TEXTURE_EXTRA_DIR "bigbutt.tga", TEXTURE_page_bigbutton, UC_FALSE);
    TEXTURE_needed[TEXTURE_page_bigbutton] = 1;
    TEXTURE_texture[TEXTURE_page_bigleaf].LoadTextureTGA(TEXTURE_EXTRA_DIR "bigleaf.tga", TEXTURE_page_bigleaf);
    TEXTURE_needed[TEXTURE_page_bigleaf] = 1;
    TEXTURE_texture[TEXTURE_page_bigrain].LoadTextureTGA(TEXTURE_EXTRA_DIR "raindrop2.tga", TEXTURE_page_bigrain);
    TEXTURE_needed[TEXTURE_page_bigrain] = 1;
    TEXTURE_texture[TEXTURE_page_tinybutt].LoadTextureTGA(TEXTURE_EXTRA_DIR "tinybutt.tga", TEXTURE_page_tinybutt, UC_FALSE);
    TEXTURE_needed[TEXTURE_page_tinybutt] = 1;
    TEXTURE_texture[TEXTURE_page_snowflake].LoadTextureTGA(TEXTURE_EXTRA_DIR "snowflake.tga", TEXTURE_page_snowflake);
    TEXTURE_needed[TEXTURE_page_snowflake] = 1;

    TEXTURE_texture[TEXTURE_page_finalglow].LoadTextureTGA(TEXTURE_EXTRA_DIR "finalglow.tga", TEXTURE_page_finalglow);
    TEXTURE_needed[TEXTURE_page_finalglow] = 1;

    TEXTURE_texture[TEXTURE_page_fade_MF].LoadTextureTGA(TEXTURE_EXTRA_DIR "fade_MF.tga", TEXTURE_page_fade_MF, UC_FALSE);
    TEXTURE_needed[TEXTURE_page_fade_MF] = 1;

    LOADED_THIS_MANY_TEXTURES(7);

    {
        TEXTURE_texture[TEXTURE_page_fadecat].LoadTextureTGA(TEXTURE_EXTRA_DIR "fadecat.tga", TEXTURE_page_fadecat, UC_FALSE);
        TEXTURE_texture[TEXTURE_page_tyretrack_alpha].LoadTextureTGA(TEXTURE_EXTRA_DIR "tyremark_alpha.tga", TEXTURE_page_tyretrack_alpha);
        TEXTURE_texture[TEXTURE_page_ladder].LoadTextureTGA(TEXTURE_EXTRA_DIR "secret.tga", TEXTURE_page_ladder);
        TEXTURE_texture[TEXTURE_page_shadowoval].LoadTextureTGA(TEXTURE_EXTRA_DIR "shadow.tga", TEXTURE_page_shadowoval);
        TEXTURE_texture[TEXTURE_page_rubbish].LoadTextureTGA(TEXTURE_EXTRA_DIR "rubbish.tga", TEXTURE_page_rubbish);
        LOADED_THIS_MANY_TEXTURES(5);

        TEXTURE_texture[TEXTURE_page_lastpanel2].LoadTextureTGA(TEXTURE_EXTRA_DIR "PCdisplay01.tga", TEXTURE_page_lastpanel2, UC_FALSE);
        TEXTURE_texture[TEXTURE_page_sign].LoadTextureTGA(TEXTURE_EXTRA_DIR "signs.tga", TEXTURE_page_sign);
        TEXTURE_texture[TEXTURE_page_shadowsquare].LoadTextureTGA(TEXTURE_EXTRA_DIR "shadowsquare.tga", TEXTURE_page_shadowsquare);
        TEXTURE_texture[TEXTURE_page_ladshad].LoadTextureTGA(TEXTURE_EXTRA_DIR "ladshad.tga", TEXTURE_page_ladshad);
        TEXTURE_texture[TEXTURE_page_meteor].LoadTextureTGA(TEXTURE_EXTRA_DIR "meteorALL.tga", TEXTURE_page_meteor);
        LOADED_THIS_MANY_TEXTURES(5);

        // Male civilian body part textures (people3)
        TEXTURE_page_people3 = 21 * 64;
        TEXTURE_texture[TEXTURE_page_people3 + 0].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "crotch1.tga", TEXTURE_page_people3 + 0);
        TEXTURE_texture[TEXTURE_page_people3 + 1].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "crotch2.tga", TEXTURE_page_people3 + 1);
        TEXTURE_texture[TEXTURE_page_people3 + 2].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "crotch3.tga", TEXTURE_page_people3 + 2);
        TEXTURE_texture[TEXTURE_page_people3 + 3].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "front1.tga", TEXTURE_page_people3 + 3);
        TEXTURE_texture[TEXTURE_page_people3 + 4].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "front2.tga", TEXTURE_page_people3 + 4);
        LOADED_THIS_MANY_TEXTURES(5);
        TEXTURE_texture[TEXTURE_page_people3 + 5].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "front3.tga", TEXTURE_page_people3 + 5);
        TEXTURE_texture[TEXTURE_page_people3 + 6].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "hatside1.tga", TEXTURE_page_people3 + 6);
        TEXTURE_texture[TEXTURE_page_people3 + 7].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "hatside2.tga", TEXTURE_page_people3 + 7);
        TEXTURE_texture[TEXTURE_page_people3 + 8].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "hatside3.tga", TEXTURE_page_people3 + 8);
        TEXTURE_texture[TEXTURE_page_people3 + 9].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "hattop1.tga", TEXTURE_page_people3 + 9);
        TEXTURE_texture[TEXTURE_page_people3 + 10].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "hattop2.tga", TEXTURE_page_people3 + 10);
        LOADED_THIS_MANY_TEXTURES(5);
        TEXTURE_texture[TEXTURE_page_people3 + 11].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "hattop3.tga", TEXTURE_page_people3 + 11);
        TEXTURE_texture[TEXTURE_page_people3 + 12].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "leg1.tga", TEXTURE_page_people3 + 12);
        TEXTURE_texture[TEXTURE_page_people3 + 13].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "leg2.tga", TEXTURE_page_people3 + 13);
        TEXTURE_texture[TEXTURE_page_people3 + 14].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "leg3.tga", TEXTURE_page_people3 + 14);
        LOADED_THIS_MANY_TEXTURES(4);

        // Female civilian body part textures (people3 female)
        TEXTURE_texture[TEXTURE_page_people3 + 15].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMARSE1.tga", TEXTURE_page_people3 + 15);
        TEXTURE_texture[TEXTURE_page_people3 + 16].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMARSE2.tga", TEXTURE_page_people3 + 16);
        TEXTURE_texture[TEXTURE_page_people3 + 17].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMARSE3.tga", TEXTURE_page_people3 + 17);
        TEXTURE_texture[TEXTURE_page_people3 + 18].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMCHEST1.tga", TEXTURE_page_people3 + 18);
        TEXTURE_texture[TEXTURE_page_people3 + 19].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMCHEST2.tga", TEXTURE_page_people3 + 19);
        TEXTURE_texture[TEXTURE_page_people3 + 20].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMCHEST3.tga", TEXTURE_page_people3 + 20);
        LOADED_THIS_MANY_TEXTURES(6);
        TEXTURE_texture[TEXTURE_page_people3 + 21].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "SEAM1.tga", TEXTURE_page_people3 + 21);
        TEXTURE_texture[TEXTURE_page_people3 + 22].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "SEAM2.tga", TEXTURE_page_people3 + 22);
        TEXTURE_texture[TEXTURE_page_people3 + 23].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "SEAM3.tga", TEXTURE_page_people3 + 23);
        TEXTURE_texture[TEXTURE_page_people3 + 24].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMSHOO1.tga", TEXTURE_page_people3 + 24);
        TEXTURE_texture[TEXTURE_page_people3 + 25].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMSHOO2.tga", TEXTURE_page_people3 + 25);
        TEXTURE_texture[TEXTURE_page_people3 + 26].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMSHOO3.tga", TEXTURE_page_people3 + 26);
        LOADED_THIS_MANY_TEXTURES(6);
        TEXTURE_texture[TEXTURE_page_people3 + 27].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMBAK1.tga", TEXTURE_page_people3 + 27);
        TEXTURE_texture[TEXTURE_page_people3 + 28].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMBAK2.tga", TEXTURE_page_people3 + 28);
        TEXTURE_texture[TEXTURE_page_people3 + 29].LoadTextureTGA(TEXTURE_PEOPLE3_DIR "FEMBAK3.tga", TEXTURE_page_people3 + 29);
        LOADED_THIS_MANY_TEXTURES(3);
    }

    // Load warehouse roof textures.
    {
        for (i = 0; i < WARE_rooftex_upto; i++) {
            TEXTURE_get_minitexturebits_uvs(
                WARE_rooftex[i],
                &page,
                u + 0,
                v + 0,
                u + 1,
                v + 1,
                u + 2,
                v + 2,
                u + 3,
                v + 3);

            if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                TEXTURE_load_page(page);
                LOADED_THIS_MANY_TEXTURES(1);
            }
        }

        // Load all map tile textures.
        for (x = 0; x < MAP_WIDTH - 1; x++)
            for (z = 0; z < MAP_HEIGHT - 1; z++) {
                TEXTURE_get_minitexturebits_uvs(
                    PAP_2HI(x, z).Texture,
                    &page,
                    u + 0,
                    v + 0,
                    u + 1,
                    v + 1,
                    u + 2,
                    v + 2,
                    u + 3,
                    v + 3);

                ASSERT(WITHIN(page, 0, TEXTURE_page_num_standard - 1));

                if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                    TEXTURE_load_page(page);
                    LOADED_THIS_MANY_TEXTURES(1);
                }
            }

        // Touch AnimTmap page references (no actual loading needed — loaded on demand).
        for (i = 1; i < MAX_ANIM_TMAPS; i++) {
            struct AnimTmap* p_a;
            p_a = &anim_tmaps[i];
            for (k = 0; k < MAX_TMAP_FRAMES; k++) {
                page = p_a->UV[k][0][0] & 0xc0;
                page <<= 2;
                page |= p_a->Page[k];
            }
        }

        // Force jacket alternative pages to be loaded (thugs).
        TEXTURE_load_page(18 * 64 + 2);
        TEXTURE_load_page(18 * 64 + 32);
        TEXTURE_load_page(18 * 64 + 3);
        TEXTURE_load_page(18 * 64 + 33);
        LOADED_THIS_MANY_TEXTURES(4);
        TEXTURE_load_page(18 * 64 + 4);
        TEXTURE_load_page(18 * 64 + 36);
        TEXTURE_load_page(18 * 64 + 5);
        TEXTURE_load_page(18 * 64 + 37);
        LOADED_THIS_MANY_TEXTURES(4);

        // Load prim face3 textures.
        for (i = 1; i < next_prim_face3; i++) {
            f3 = &prim_faces3[i];
            page = f3->UV[0][0] & 0xc0;
            page <<= 2;
            page |= f3->TexturePage;
            page += FACE_PAGE_OFFSET;
            if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                TEXTURE_load_page(page);
                LOADED_THIS_MANY_TEXTURES(1);
            }
        }

        // Load prim face4 textures.
        for (i = 1; i < next_prim_face4; i++) {
            f4 = &prim_faces4[i];
            page = f4->UV[0][0] & 0xc0;
            page <<= 2;
            page |= f4->TexturePage;
            page += FACE_PAGE_OFFSET;
            if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                TEXTURE_load_page(page);
                LOADED_THIS_MANY_TEXTURES(1);
            }
        }

        TEXTURE_load_page(156);

        // Load building facade textures.
        for (i = 1; i < next_dfacet; i++) {
            SLONG c0, c1;
            SLONG style, dstyle;

            if (dfacets[i].FacetType == STOREY_TYPE_NORMAL
                || dfacets[i].FacetType == STOREY_TYPE_INSIDE
                || dfacets[i].FacetType == STOREY_TYPE_OINSIDE
                || dfacets[i].FacetType == STOREY_TYPE_FENCE
                || dfacets[i].FacetType == STOREY_TYPE_FENCE_FLAT
                || dfacets[i].FacetType == STOREY_TYPE_LADDER
                || dfacets[i].FacetType == STOREY_TYPE_DOOR
                || dfacets[i].FacetType == STOREY_TYPE_OUTSIDE_DOOR
                || dfacets[i].FacetType == STOREY_TYPE_INSIDE_DOOR
                || dfacets[i].FacetType == STOREY_TYPE_FENCE_BRICK) {
                style = dfacets[i].StyleIndex;
                dstyle = dstyles[style];

                for (c0 = 0; c0 < ((dfacets[i].Height + 3) >> 2) * ((dfacets[i].FacetFlags & FACET_FLAG_2SIDED) ? 2 : 1); c0++) {
                    dstyle = dstyles[style + c0];

                    if (dstyle > 0) {
                        for (c1 = 0; c1 < TEXTURE_PIECE_NUMBER; c1++) {
                            page = dx_textures_xy[dstyle][c1].Page;
                            if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                                TEXTURE_load_page(page);
                                LOADED_THIS_MANY_TEXTURES(1);
                            }
                        }
                    } else {
                        struct DStorey* p_storey;
                        SLONG pos;

                        p_storey = &dstoreys[-dstyle];

                        for (pos = 0; pos < p_storey->Count; pos++) {
                            page = paint_mem[p_storey->Index + pos];
                            if (page)
                                if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                                    TEXTURE_load_page(page);
                                    LOADED_THIS_MANY_TEXTURES(1);
                                }
                        }
                        dstyle = p_storey->Style;
                        if (dstyle > 0) {
                            for (c1 = 0; c1 < TEXTURE_PIECE_NUMBER; c1++) {
                                page = dx_textures_xy[dstyle][c1].Page;
                                if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                                    TEXTURE_load_page(page);
                                    LOADED_THIS_MANY_TEXTURES(1);
                                }
                            }
                        }
                    }
                }
            }
            if (dfacets[i].FacetType == STOREY_TYPE_LADDER) {
                dstyle = dstyles[dfacets[i].StyleIndex];
                if (dstyle > 0)
                    for (c1 = 0; c1 < TEXTURE_PIECE_NUMBER; c1++) {
                        page = dx_textures_xy[dstyle][c1].Page;
                        if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
                            TEXTURE_load_page(page);
                            LOADED_THIS_MANY_TEXTURES(1);
                        }
                    }
            }
        }
    }

    CloseTGAClump();

    extern void AENG_guess_detail_levels();
    AENG_guess_detail_levels();

    NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit();

    POLY_frame_init(UC_FALSE, UC_FALSE);
}

// uc_orig: TEXTURE_load_needed_object (fallen/DDEngine/Source/texture.cpp)
// Loads textures needed for a specific prim object (used for on-demand prim texture loading).
void TEXTURE_load_needed_object(SLONG prim)
{
    SLONG i;
    SLONG page;

    PrimObject* po;
    PrimFace3* f3;
    PrimFace4* f4;

    po = &prim_objects[prim];

    for (i = po->StartFace3; i < po->EndFace3; i++) {
        f3 = &prim_faces3[i];
        page = f3->UV[0][0] & 0xc0;
        page <<= 2;
        page |= f3->TexturePage;
        if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
            TEXTURE_load_page(page);
        }
    }

    for (i = po->StartFace4; i < po->EndFace4; i++) {
        f4 = &prim_faces4[i];
        page = f4->UV[0][0] & 0xc0;
        page <<= 2;
        page |= f4->TexturePage;
        if (TEXTURE_texture[page].Type == D3DTEXTURE_TYPE_UNUSED) {
            TEXTURE_load_page(page);
        }
    }
}

// uc_orig: TEXTURE_free (fallen/DDEngine/Source/texture.cpp)
// Frees all loaded textures and resets the texture system state.
void TEXTURE_free()
{
    SLONG i;

    CRINKLE_init();

    for (i = 0; i < TEXTURE_num_textures; i++) {
        TEXTURE_texture[i].Destroy();
        TEXTURE_texture[i].Type = D3DTEXTURE_TYPE_UNUSED;
    }

    the_display.RemoveAllLoadedTextures();

    memset(TEXTURE_dontexist, 0, sizeof(TEXTURE_dontexist));
    memset(TEXTURE_needed, 0, sizeof(TEXTURE_needed));

    POLY_reset_render_states();

    TEXTURE_DC_pack_init();
}

// uc_orig: TEXTURE_free_unneeded (fallen/DDEngine/Source/texture.cpp)
// Frees all non-frontend textures (called at level unload).
// Frontend textures marked in TEXTURE_needed[] are preserved.
void TEXTURE_free_unneeded(void)
{
    SLONG i;

    CRINKLE_init();

    for (i = 0; i < TEXTURE_num_textures; i++) {
        TEXTURE_texture[i].Destroy();
        TEXTURE_texture[i].Type = D3DTEXTURE_TYPE_UNUSED;
        TEXTURE_dontexist[i] = 0;
    }

    extern void FreeAllD3DPages(void);
    FreeAllD3DPages();

    POLY_reset_render_states();
}

// uc_orig: TEXTURE_get_handle (fallen/DDEngine/Source/texture.cpp)
// Returns the Direct3D texture interface for the given page. Returns NULL for page -1.
LPDIRECT3DTEXTURE2 TEXTURE_get_handle(SLONG page)
{
    if (page == -1) {
        return (NULL);
    }
    return TEXTURE_texture[page].GetD3DTexture();
}

// uc_orig: TEXTURE_get_D3DTexture (fallen/DDEngine/Source/texture.cpp)
// Returns a pointer to the D3DTexture object for the given page.
D3DTexture* TEXTURE_get_D3DTexture(SLONG page)
{
    return &(TEXTURE_texture[page]);
}

// uc_orig: TEXTURE_get_minitexturebits_uvs (fallen/DDEngine/Source/texture.cpp)
// Decodes a packed MiniTextureBits texture descriptor into page index and UV corners.
// The 16-bit value encodes: page (bits 0-9), rotation (bits 10-11), flip (bits 12-13), size (bits 14-15).
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
    float* v3)
{
    SLONG trot;
    SLONG num;

    static const float base_u = 0.0F;
    static const float base_v = 0.0F;
    static const float base_size = 1.0F;

    num = texture & 0x3ff;
    trot = (texture >> 0xa) & 0x3;

    *page = num;
    if (*page >= TEXTURE_page_num_standard) {
        *page = 0;
    }

    switch (trot) {
    case 0:
        *u0 = base_u;            *v0 = base_v;
        *u1 = base_u + base_size; *v1 = base_v;
        *u2 = base_u;            *v2 = base_v + base_size;
        *u3 = base_u + base_size; *v3 = base_v + base_size;
        break;
    case 1:
        *u2 = base_u;            *v2 = base_v;
        *u0 = base_u + base_size; *v0 = base_v;
        *u3 = base_u;            *v3 = base_v + base_size;
        *u1 = base_u + base_size; *v1 = base_v + base_size;
        break;
    case 2:
        *u3 = base_u;            *v3 = base_v;
        *u2 = base_u + base_size; *v2 = base_v;
        *u1 = base_u;            *v1 = base_v + base_size;
        *u0 = base_u + base_size; *v0 = base_v + base_size;
        break;
    case 3:
        *u1 = base_u;            *v1 = base_v;
        *u3 = base_u + base_size; *v3 = base_v;
        *u0 = base_u;            *v0 = base_v + base_size;
        *u2 = base_u + base_size; *v2 = base_v + base_size;
        break;
    }
}

extern UWORD local_next_prim_face3;
extern UWORD local_next_prim_face4;

// uc_orig: TEXTURE_fix_texture_styles (fallen/DDEngine/Source/texture.cpp)
// Converts texture style UV coordinates from PSX-style absolute pixel coords
// to Direct3D page indices (stored in dx_textures_xy[]).
void TEXTURE_fix_texture_styles(void)
{
    SLONG style, piece;
    SLONG page;
    SLONG av_u;
    SLONG av_v;
    SLONG base_u;
    SLONG base_v;

    for (style = 0; style < 200; style++) {
        for (piece = 0; piece < 5; piece++) {
            base_u = textures_xy[style][piece].Tx * 32;
            base_v = textures_xy[style][piece].Ty * 32;

            av_u = base_u / TEXTURE_NORM_SIZE;
            av_v = base_v / TEXTURE_NORM_SIZE;

            page = av_u + av_v * TEXTURE_NORM_SQUARES + textures_xy[style][piece].Page * TEXTURE_NORM_SQUARES * TEXTURE_NORM_SQUARES;
            dx_textures_xy[style][piece].Page = page;
            dx_textures_xy[style][piece].Flip = textures_xy[style][piece].Flip;
        }
    }
}

// uc_orig: TEXTURE_fix_prim_textures (fallen/DDEngine/Source/texture.cpp)
// Converts prim face UV coordinates from raw PSX format to normalised D3D format.
// Also resolves thug jacket texture variants (people vs people2 pages).
void TEXTURE_fix_prim_textures()
{
    SLONG i;
    SLONG j;
    SLONG k;

    PrimFace3* f3;
    PrimFace4* f4;
    struct AnimTmap* p_a;

    SLONG page;
    SLONG av_u;
    SLONG av_v;
    SLONG u;
    SLONG v;
    SLONG base_u;
    SLONG base_v;

    for (i = 1; i < next_prim_face3; i++) {
        f3 = &prim_faces3[i];

        if (!(f3->FaceFlags & FACE_FLAG_FIXED)) {
            av_u = (f3->UV[0][0] + f3->UV[1][0] + f3->UV[2][0]) / 3;
            av_v = (f3->UV[0][1] + f3->UV[1][1] + f3->UV[2][1]) / 3;

            av_u /= TEXTURE_NORM_SIZE;
            av_v /= TEXTURE_NORM_SIZE;

            base_u = av_u * TEXTURE_NORM_SIZE;
            base_v = av_v * TEXTURE_NORM_SIZE;

            for (j = 0; j < 3; j++) {
                u = f3->UV[j][0];
                v = f3->UV[j][1];
                u -= base_u;
                v -= base_v;
                SATURATE(u, 0, 32);
                SATURATE(v, 0, 32);
                if (u == 31) { u = 32; }
                if (v == 31) { v = 32; }
                f3->UV[j][0] = u;
                f3->UV[j][1] = v;
            }

            page = av_u + av_v * TEXTURE_NORM_SQUARES + f3->TexturePage * TEXTURE_NORM_SQUARES * TEXTURE_NORM_SQUARES;
            f3->FaceFlags &= ~FACE_FLAG_THUG_JACKET;

            switch (page) {
            case 9 * 64 + 21:
            case 18 * 64 + 2:
            case 18 * 64 + 32:
                f3->FaceFlags |= FACE_FLAG_THUG_JACKET;
                page = 9 * 64 + 21;
                break;
            case 9 * 64 + 22:
            case 18 * 64 + 3:
            case 18 * 64 + 33:
                f3->FaceFlags |= FACE_FLAG_THUG_JACKET;
                page = 9 * 64 + 22;
                break;
            case 9 * 64 + 24:
            case 18 * 64 + 4:
            case 18 * 64 + 36:
                f3->FaceFlags |= FACE_FLAG_THUG_JACKET;
                page = 9 * 64 + 24;
                break;
            case 9 * 64 + 25:
            case 18 * 64 + 5:
            case 18 * 64 + 37:
                f3->FaceFlags |= FACE_FLAG_THUG_JACKET;
                page = 9 * 64 + 25;
                break;
            }
            page -= FACE_PAGE_OFFSET;
            SATURATE(page, 0, 64 * 14);
            f3->UV[0][0] |= (page >> 2) & 0xc0;
            f3->TexturePage = (page >> 0) & 0xff;
            f3->FaceFlags |= FACE_FLAG_FIXED;
        }
    }

    for (i = 1; i < next_prim_face4; i++) {
        f4 = &prim_faces4[i];

        if (!(f4->FaceFlags & FACE_FLAG_ANIMATE)) {
            if (!(f4->FaceFlags & FACE_FLAG_FIXED)) {
                av_u = (f4->UV[0][0] + f4->UV[1][0] + f4->UV[2][0] + f4->UV[3][0]) >> 2;
                av_v = (f4->UV[0][1] + f4->UV[1][1] + f4->UV[2][1] + f4->UV[3][1]) >> 2;

                av_u /= TEXTURE_NORM_SIZE;
                av_v /= TEXTURE_NORM_SIZE;

                base_u = av_u * TEXTURE_NORM_SIZE;
                base_v = av_v * TEXTURE_NORM_SIZE;

                for (j = 0; j < 4; j++) {
                    u = f4->UV[j][0];
                    v = f4->UV[j][1];
                    u -= base_u;
                    v -= base_v;
                    SATURATE(u, 0, 32);
                    SATURATE(v, 0, 32);
                    if (u == 31) { u = 32; }
                    if (v == 31) { v = 32; }
                    f4->UV[j][0] = u;
                    f4->UV[j][1] = v;
                }

                page = av_u + av_v * TEXTURE_NORM_SQUARES + f4->TexturePage * TEXTURE_NORM_SQUARES * TEXTURE_NORM_SQUARES;
                f4->FaceFlags &= ~FACE_FLAG_THUG_JACKET;
                switch (page) {
                case 9 * 64 + 21:
                case 18 * 64 + 2:
                case 18 * 64 + 32:
                    f4->FaceFlags |= FACE_FLAG_THUG_JACKET;
                    page = 9 * 64 + 21;
                    break;
                case 9 * 64 + 22:
                case 18 * 64 + 3:
                case 18 * 64 + 33:
                    f4->FaceFlags |= FACE_FLAG_THUG_JACKET;
                    page = 9 * 64 + 22;
                    break;
                case 9 * 64 + 24:
                case 18 * 64 + 4:
                case 18 * 64 + 36:
                    f4->FaceFlags |= FACE_FLAG_THUG_JACKET;
                    page = 9 * 64 + 24;
                    break;
                case 9 * 64 + 25:
                case 18 * 64 + 5:
                case 18 * 64 + 37:
                    f4->FaceFlags |= FACE_FLAG_THUG_JACKET;
                    page = 9 * 64 + 25;
                    break;
                }
                page -= FACE_PAGE_OFFSET;
                SATURATE(page, 0, 14 * 64);
                f4->UV[0][0] |= (page >> 2) & 0xc0;
                f4->TexturePage = (page >> 0) & 0xff;
                f4->FaceFlags |= FACE_FLAG_FIXED;
            }
        }
    }

    for (i = 1; i < MAX_ANIM_TMAPS; i++) {
        p_a = &anim_tmaps[i];

        for (k = 0; k < MAX_TMAP_FRAMES; k++) {
            av_u = (p_a->UV[k][0][0] + p_a->UV[k][1][0] + p_a->UV[k][2][0] + p_a->UV[k][3][0]) >> 2;
            av_v = (p_a->UV[k][0][1] + p_a->UV[k][1][1] + p_a->UV[k][2][1] + p_a->UV[k][3][1]) >> 2;

            av_u /= TEXTURE_NORM_SIZE;
            av_v /= TEXTURE_NORM_SIZE;

            base_u = av_u * TEXTURE_NORM_SIZE;
            base_v = av_v * TEXTURE_NORM_SIZE;

            for (j = 0; j < 4; j++) {
                p_a->UV[k][j][0] -= base_u;
                p_a->UV[k][j][1] -= base_v;
                SATURATE(p_a->UV[k][j][0], 0, 32);
                SATURATE(p_a->UV[k][j][1], 0, 32);
                if (p_a->UV[k][j][0] == 31) { p_a->UV[k][j][0] = 32; }
                if (p_a->UV[k][j][1] == 31) { p_a->UV[k][j][1] = 32; }
            }

            page = av_u + av_v * TEXTURE_NORM_SQUARES + p_a->Page[k] * TEXTURE_NORM_SQUARES * TEXTURE_NORM_SQUARES;
            SATURATE(page, 0, 575);
            p_a->UV[k][0][0] |= (page >> 2) & 0xc0;
            p_a->Page[k] = (page >> 0) & 0xff;
        }
    }
}

// uc_orig: TEXTURE_set_colour_key (fallen/DDEngine/Source/texture.cpp)
// Attempts to set a colour key on a texture page (returns immediately — was never used on D3D path).
void TEXTURE_set_colour_key(SLONG page)
{
    DDCOLORKEY ck;

    return;

    ASSERT(WITHIN(page, 0, TEXTURE_num_textures - 1));

    if (TEXTURE_fiddled) {
        ck.dwColorSpaceLowValue = 0x00000000;
        ck.dwColorSpaceHighValue = 0x00000000;
    } else {
        ck.dwColorSpaceLowValue = 0;
        ck.dwColorSpaceHighValue = 0;
    }

    TEXTURE_texture[page].SetColorKey(DDCKEY_SRCBLT, &ck);
}

// uc_orig: TEXTURE_shadow_lock (fallen/DDEngine/Source/texture.cpp)
// Locks the shadow texture for CPU write. Returns UC_TRUE if successful.
// Sets TEXTURE_shadow_bitmap/pitch/mask/shift globals on success.
SLONG TEXTURE_shadow_lock(void)
{
    HRESULT res;

    res = TEXTURE_texture[TEXTURE_page_shadow].LockUser(
        &TEXTURE_shadow_bitmap,
        &TEXTURE_shadow_pitch);

    if (FAILED(res)) {
        TEXTURE_shadow_bitmap = NULL;
        TEXTURE_shadow_pitch = 0;
        return UC_FALSE;
    } else {
        TEXTURE_shadow_mask_red = TEXTURE_texture[TEXTURE_page_shadow].mask_red;
        TEXTURE_shadow_mask_green = TEXTURE_texture[TEXTURE_page_shadow].mask_green;
        TEXTURE_shadow_mask_blue = TEXTURE_texture[TEXTURE_page_shadow].mask_blue;
        TEXTURE_shadow_mask_alpha = TEXTURE_texture[TEXTURE_page_shadow].mask_alpha;
        TEXTURE_shadow_shift_red = TEXTURE_texture[TEXTURE_page_shadow].shift_red;
        TEXTURE_shadow_shift_green = TEXTURE_texture[TEXTURE_page_shadow].shift_green;
        TEXTURE_shadow_shift_blue = TEXTURE_texture[TEXTURE_page_shadow].shift_blue;
        TEXTURE_shadow_shift_alpha = TEXTURE_texture[TEXTURE_page_shadow].shift_alpha;
        return UC_TRUE;
    }
}

// uc_orig: TEXTURE_shadow_unlock (fallen/DDEngine/Source/texture.cpp)
void TEXTURE_shadow_unlock()
{
    TEXTURE_texture[TEXTURE_page_shadow].UnlockUser();
}

// uc_orig: TEXTURE_shadow_update (fallen/DDEngine/Source/texture.cpp)
// No-op stub — shadow texture update is handled by D3D internally.
void TEXTURE_shadow_update(void)
{
}

// uc_orig: TEXTURE_set_greyscale (fallen/DDEngine/Source/texture.cpp)
// Sets all texture pages to greyscale or colour mode (used for some visual effect).
void TEXTURE_set_greyscale(SLONG is_greyscale)
{
    SLONG i;
    for (i = 0; i < TEXTURE_MAX_TEXTURES; i++) {
        TEXTURE_texture[i].set_greyscale(is_greyscale);
    }
}

// uc_orig: TEXTURE_86_lock (fallen/DDEngine/Source/texture.cpp)
// Locks texture page 86 (the video/screensaver page) for CPU write.
SLONG TEXTURE_86_lock()
{
    HRESULT res;

    res = TEXTURE_texture[86].LockUser(
        &TEXTURE_shadow_bitmap,
        &TEXTURE_shadow_pitch);

    if (FAILED(res)) {
        TEXTURE_shadow_bitmap = NULL;
        TEXTURE_shadow_pitch = 0;
        return UC_FALSE;
    } else {
        TEXTURE_shadow_mask_red = TEXTURE_texture[86].mask_red;
        TEXTURE_shadow_mask_green = TEXTURE_texture[86].mask_green;
        TEXTURE_shadow_mask_blue = TEXTURE_texture[86].mask_blue;
        TEXTURE_shadow_mask_alpha = TEXTURE_texture[86].mask_alpha;
        TEXTURE_shadow_shift_red = TEXTURE_texture[86].shift_red;
        TEXTURE_shadow_shift_green = TEXTURE_texture[86].shift_green;
        TEXTURE_shadow_shift_blue = TEXTURE_texture[86].shift_blue;
        TEXTURE_shadow_shift_alpha = TEXTURE_texture[86].shift_alpha;
        return UC_TRUE;
    }
}

// uc_orig: TEXTURE_86_unlock (fallen/DDEngine/Source/texture.cpp)
void TEXTURE_86_unlock()
{
    TEXTURE_texture[86].UnlockUser();
}

// uc_orig: TEXTURE_86_update (fallen/DDEngine/Source/texture.cpp)
// No-op stub — video texture update handled by D3D.
void TEXTURE_86_update()
{
}

// uc_orig: TEXTURE_set_tga (fallen/DDEngine/Source/texture.cpp)
// Replaces a specific texture page with a different TGA file from the extras dir.
void TEXTURE_set_tga(SLONG page, CBYTE* fn)
{
    CBYTE fn2[_MAX_PATH];
    MFFileHandle file;

    strcpy(fn2, TEXTURE_EXTRA_DIR);
    strcat(fn2, fn);
    file = FileOpen(fn2);
    if (file != FILE_OPEN_ERROR) {
        TEXTURE_texture[page].ChangeTextureTGA(fn2);
        FileClose(file);
    }
}

// uc_orig: TEXTURE_looks_like (fallen/DDEngine/Source/texture.cpp)
// Analyses a texture page's pixel data to classify it as road, grass, dirt, or slippery.
// Sets TEXTURE_liney, TEXTURE_av_r/g/b as side effects.
SLONG TEXTURE_looks_like(SLONG page)
{
    SLONG i;
    SLONG j;
    SLONG px;
    SLONG py;
    SLONG dx;
    SLONG dy;
    SLONG px1;
    SLONG py1;
    SLONG px2;
    SLONG py2;
    UWORD* bitmap;
    SLONG pitch;
    UWORD pixel;
    SLONG r;
    SLONG g;
    SLONG b;
    SLONG r1;
    SLONG g1;
    SLONG b1;
    SLONG r2;
    SLONG g2;
    SLONG b2;
    SLONG av_r;
    SLONG av_g;
    SLONG av_b;
    SLONG diff_r;
    SLONG diff_g;
    SLONG diff_b;
    SLONG dir;
    SLONG diff;
    SLONG pdiff_r;
    SLONG pdiff_g;
    SLONG pdiff_b;
    SLONG diff_along;
    SLONG diff_left;
    SLONG diff_right;
    SLONG ddiff_l;
    SLONG ddiff_r;
    SLONG lines;

    ASSERT(WITHIN(page, 0, 511));

    D3DTexture* dt = &TEXTURE_texture[page];

    TEXTURE_liney = 0;
    TEXTURE_av_r = 0;
    TEXTURE_av_g = 0;
    TEXTURE_av_b = 0;

    if (SUCCEEDED(dt->LockUser(&bitmap, &pitch))) {
        av_r = 0;
        av_g = 0;
        av_b = 0;

        for (px = 0; px < TEXTURE_texture[page].size; px++)
            for (py = 0; py < TEXTURE_texture[page].size; py++) {
                pixel = bitmap[px + py * pitch];
                r = ((pixel >> dt->shift_red) & (0xff >> dt->mask_red)) << dt->mask_red;
                g = ((pixel >> dt->shift_green) & (0xff >> dt->mask_green)) << dt->mask_green;
                b = ((pixel >> dt->shift_blue) & (0xff >> dt->mask_blue)) << dt->mask_blue;
                av_r += r;
                av_g += g;
                av_b += b;
            }

        av_r /= dt->size * dt->size;
        av_g /= dt->size * dt->size;
        av_b /= dt->size * dt->size;

        TEXTURE_av_r = av_r;
        TEXTURE_av_g = av_g;
        TEXTURE_av_b = av_b;

        if (av_g > av_r && av_g > av_b) {
            SLONG rb = av_r + av_b;
            rb >>= 1;
            rb += rb >> 1;
            if (av_g > rb) {
                dt->UnlockUser();
                return TEXTURE_LOOK_GRASS;
            }
        }

        diff_r = 0;
        diff_g = 0;
        diff_b = 0;

#define TEXTURE_SAMPLE_DIFF1 128
#define TEXTURE_SAMPLE_DIFF2 4

        for (i = 0; i < TEXTURE_SAMPLE_DIFF1; i++) {
            px1 = rand() & (dt->size - 1);
            py1 = rand() & (dt->size - 1);
            pixel = bitmap[px1 + py1 * pitch];
            r1 = ((pixel >> dt->shift_red) & (0xff >> dt->mask_red)) << dt->mask_red;
            g1 = ((pixel >> dt->shift_green) & (0xff >> dt->mask_green)) << dt->mask_green;
            b1 = ((pixel >> dt->shift_blue) & (0xff >> dt->mask_blue)) << dt->mask_blue;

            for (j = 0; j < TEXTURE_SAMPLE_DIFF2; j++) {
                px2 = px1 + (rand() & 0x7) - 0x3;
                py2 = py1 + (rand() & 0x7) - 0x3;
                px2 &= dt->size - 1;
                py2 &= dt->size - 1;
                pixel = bitmap[px2 + py2 * pitch];
                r2 = ((pixel >> dt->shift_red) & (0xff >> dt->mask_red)) << dt->mask_red;
                g2 = ((pixel >> dt->shift_green) & (0xff >> dt->mask_green)) << dt->mask_green;
                b2 = ((pixel >> dt->shift_blue) & (0xff >> dt->mask_blue)) << dt->mask_blue;
                diff_r += abs(r2 - r1);
                diff_g += abs(g2 - g1);
                diff_b += abs(b2 - b1);
            }
        }

        diff_r /= TEXTURE_SAMPLE_DIFF1 * TEXTURE_SAMPLE_DIFF2;
        diff_g /= TEXTURE_SAMPLE_DIFF1 * TEXTURE_SAMPLE_DIFF2;
        diff_b /= TEXTURE_SAMPLE_DIFF1 * TEXTURE_SAMPLE_DIFF2;
        diff = diff_r + diff_g + diff_b;

        lines = 0;

#define TEXTURE_SAMPLE_STRAIGHT1 2048
#define TEXTURE_SAMPLE_STRAIGHT2 16

        static struct {
            SBYTE dx;
            SBYTE dy;
        } offset[4] = {
            { -1, 0 },
            { +1, 0 },
            { 0, -1 },
            { 0, +1 }
        };

        dir = 0;

        for (px = 0; px < dt->size; px++)
            for (py = 0; py < dt->size; py++) {
                px1 = px & (dt->size - 1);
                py1 = py & (dt->size - 1);

                pixel = bitmap[px1 + py1 * pitch];
                r1 = ((pixel >> dt->shift_red) & (0xff >> dt->mask_red)) << dt->mask_red;
                g1 = ((pixel >> dt->shift_green) & (0xff >> dt->mask_green)) << dt->mask_green;
                b1 = ((pixel >> dt->shift_blue) & (0xff >> dt->mask_blue)) << dt->mask_blue;

                diff_left = 0;
                diff_right = 0;
                diff_along = 0;

                dx = offset[dir & 0x3].dx;
                dy = offset[dir & 0x3].dy;
                dir += 1;

                for (j = 0; j < TEXTURE_SAMPLE_STRAIGHT2; j++) {
                    px2 = px1 - (dy * 4); py2 = py1 + (dx * 4);
                    px2 &= dt->size - 1; py2 &= dt->size - 1;
                    pixel = bitmap[px2 + py2 * pitch];
                    r2 = ((pixel >> dt->shift_red) & (0xff >> dt->mask_red)) << dt->mask_red;
                    g2 = ((pixel >> dt->shift_green) & (0xff >> dt->mask_green)) << dt->mask_green;
                    b2 = ((pixel >> dt->shift_blue) & (0xff >> dt->mask_blue)) << dt->mask_blue;
                    pdiff_r = abs(r2 - r1); pdiff_g = abs(g2 - g1); pdiff_b = abs(b2 - b1);
                    diff_left += pdiff_r + pdiff_g + pdiff_b;

                    px2 = px1 + (dy * 4); py2 = py1 - (dx * 4);
                    px2 &= dt->size - 1; py2 &= dt->size - 1;
                    pixel = bitmap[px2 + py2 * pitch];
                    r2 = ((pixel >> dt->shift_red) & (0xff >> dt->mask_red)) << dt->mask_red;
                    g2 = ((pixel >> dt->shift_green) & (0xff >> dt->mask_green)) << dt->mask_green;
                    b2 = ((pixel >> dt->shift_blue) & (0xff >> dt->mask_blue)) << dt->mask_blue;
                    pdiff_r = abs(r2 - r1); pdiff_g = abs(g2 - g1); pdiff_b = abs(b2 - b1);
                    diff_right += pdiff_r + pdiff_g + pdiff_b;

                    px2 = px1 + dx; py2 = py1 + dy;
                    px2 &= dt->size - 1; py2 &= dt->size - 1;
                    pixel = bitmap[px2 + py2 * pitch];
                    r2 = ((pixel >> dt->shift_red) & (0xff >> dt->mask_red)) << dt->mask_red;
                    g2 = ((pixel >> dt->shift_green) & (0xff >> dt->mask_green)) << dt->mask_green;
                    b2 = ((pixel >> dt->shift_blue) & (0xff >> dt->mask_blue)) << dt->mask_blue;
                    pdiff_r = abs(r2 - r1); pdiff_g = abs(g2 - g1); pdiff_b = abs(b2 - b1);
                    diff_along += pdiff_r + pdiff_g + pdiff_b;

                    px1 = px2; py1 = py2;
                    r1 = r2; g1 = g2; b1 = b2;
                }

                diff_along /= TEXTURE_SAMPLE_STRAIGHT2;
                diff_left /= TEXTURE_SAMPLE_STRAIGHT2;
                diff_right /= TEXTURE_SAMPLE_STRAIGHT2;

                if (diff_left > 0x60 && diff_along < 0x10) { lines += 1; }
                if (diff_right > 0x60 && diff_along < 0x10) { lines += 1; }
            }

        TEXTURE_liney = lines;

        dt->UnlockUser();

        if (av_r > av_g && av_g > av_b) {
            if (av_r > (av_b + (av_b >> 1))) {
                if (lines > 30) {
                    return TEXTURE_LOOK_ROAD;
                } else {
                    return TEXTURE_LOOK_DIRT;
                }
            }
        }

        if (av_r > 215 && av_g > 215 && av_b > 215) {
            if (lines > 30) {
                return TEXTURE_LOOK_ROAD;
            } else {
                return TEXTURE_LOOK_SLIPPERY;
            }
        }

        return TEXTURE_LOOK_ROAD;
    }

    return TEXTURE_LOOK_ROAD;
}

// uc_orig: TEXTURE_get_fiddled_position (fallen/Glide Engine/Source/gltexture.cpp)
// Glide engine function (not used in D3D build). Stub to satisfy declaration in texture.h.
SLONG TEXTURE_get_fiddled_position(
    SLONG square_u,
    SLONG square_v,
    SLONG page,
    float* u,
    float* v)
{
    // Not implemented in the D3D build — Glide-only feature.
    (void)square_u; (void)square_v; (void)u; (void)v;
    return page;
}
