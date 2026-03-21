// Remaining not-yet-migrated functions from elev.cpp.
// ELEV_load_level and all globals are now in new/missions/elev.cpp + new/missions/elev_globals.cpp.
// These functions will be migrated in iteration 76.

#include "game.h"
#include "ddlib.h"
#include "elev.h"
#include "eway.h"
#include "mission.h"
#include "night.h"
#include "ob.h"
#include "trip.h"
#include "music.h"
#include "dirt.h"
#include "fog.h"
#include "hook.h"
#include "mist.h"
#include "water.h"
#include "puddle.h"
#include "az.h"
#include "drip.h"
#include "bang.h"
#include "glitter.h"
#include "spark.h"
#include "io.h"
#include "pow.h"
#include "build2.h"
#include "ns.h"
#include "road.h"
#include "mav.h"
#include "cnet.h"
#include "interfac.h"
#include "animtmap.h"
#include "shadow.h"
#include "attract.h"
#include "cam.h"
#include "psystem.h"
#include "tracks.h"
#include "pcom.h"
#include "wmove.h"
#include "balloon.h"
#include "wand.h"
#include "ribbon.h"
#include "barrel.h"
#include "fc.h"
#include "briefing.h"
#include "ware.h"
#include "memory.h"
#include "playcuts.h"
#include "grenade.h"
#include "env.h"
#include "panel.h"
#include "sound.h"
#include "DCLowLevel.h"

void save_dreamcast_wad(CBYTE* fname);
void load_dreamcast_wad(CBYTE* fname);

void ELEV_game_init_common(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level)
{
    extern void SND_BeginAmbient();
    MFX_load_wave_list();
    SND_BeginAmbient();
}

SLONG ELEV_game_init(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level)
{
    SLONG i;

    /*

    extern UWORD darci_dlight;

    darci_dlight = 0;

    */

    ATTRACT_loadscreen_draw(10 * 256 / 100);

    strcpy(ELEV_last_map_loaded, fname_map);

    //	revert_to_prim_status();

    mark_prim_objects_as_unloaded();

    void global_load(void);
    global_load();

    ATTRACT_loadscreen_draw(15 * 256 / 100);

    extern SLONG kick_or_punch; // This is in person.cpp

    kick_or_punch = 0;

    void init_anim_prims(void);
    void init_overlay(void);

    init_overlay();
    WMOVE_init();
    PCOM_init();
    init_things();
    init_draw_meshes();
    //	init_draw_tweens(); // this is in global_load()

    init_anim_prims();
    init_persons();
    init_choppers();
    init_pyros();
    //	init_furniture();
    init_vehicles();
    init_projectiles();
    init_specials();
    BAT_init();
    void init_gangattack(void);
    init_gangattack();
    init_map();
    init_user_interface();

    ATTRACT_loadscreen_draw(20 * 256 / 100);

    SOUND_reset();
    PARTICLE_Reset();
    //	TRACKS_Reset();
    TRACKS_InitOnce();
    RIBBON_init();
    load_palette("data\\tex01.pal");

    extern CBYTE PANEL_wide_text[256];
    extern THING_INDEX PANEL_wide_top_person;
    extern THING_INDEX PANEL_wide_bot_person;

    PANEL_wide_text[0] = 0;
    PANEL_wide_top_person = NULL;
    PANEL_wide_bot_person = NULL;

    load_animtmaps();

    ATTRACT_loadscreen_draw(25 * 256 / 100);

    FC_init();
    BARREL_init();
    BALLOON_init();
    NIGHT_init();
    OB_init();
    TRIP_init();
    FOG_init();
    MIST_init();
    //	WATER_init();
    PUDDLE_init();
    //	AZ_init();
    DRIP_init();
    //	BANG_init();
    GLITTER_init();
    POW_init();
    SPARK_init();
    CONSOLE_clear();
    PLAT_init();
    MAV_init();
    PANEL_new_text_init();

    ATTRACT_loadscreen_draw(35 * 256 / 100);

    void init_ambient(void);
    init_ambient();

    extern void MAP_beacon_init();
    extern void MAP_pulse_init();

    MAP_pulse_init();
    MAP_beacon_init();

    load_game_map(fname_map);

    calc_prim_info();

    ATTRACT_loadscreen_draw(50 * 256 / 100);

    build_quick_city();

    // tum te tum
    init_animals();

    // what? function call? i din... oh, _that_ function call

    DIRT_init(100, 1, 0, INFINITY, INFINITY, INFINITY, INFINITY);

    OB_convert_dustbins_to_barrels();
    ROAD_sink();
    ROAD_wander_calc();
    WAND_init();
    WARE_init();
    MAV_precalculate();
    extern void BUILD_car_facets();
    BUILD_car_facets();
    SHADOW_do();
    //	AZ_create_lines();
    COLLIDE_calc_fastnav_bits();
    COLLIDE_find_seethrough_fences();
    clear_all_wmove_flags();
    InitGrenades();

    // now the map's loaded we can precalc audio polys, if req'd
    SOUND_SewerPrecalc();

    if (fname_lighting == NULL || !NIGHT_load_ed_file(fname_lighting)) {
        NIGHT_ambient(255, 255, 255, 110, -148, -177);
    }

    NIGHT_generate_walkable_lighting();

    if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME)) {
        GAME_FLAGS |= GF_RAINING;
    }

    ATTRACT_loadscreen_draw(60 * 256 / 100);

    ELEV_load_level(fname_level);

    ATTRACT_loadscreen_draw(65 * 256 / 100);

    TEXTURE_fix_prim_textures();
    //	TEXTURE_fix_texture_styles();

    AENG_create_dx_prim_points();

    extern void envmap_specials(void);

    envmap_specials();

    calc_prim_info();
    calc_prim_normals();
    find_anim_prim_bboxes();

    loading_screen_active = TRUE;

    if (!quick_load) {
        TEXTURE_load_needed(fname_level, 0, 256, 400);

        extern void PACK_do(void);

        // PACK_do();
    }

    loading_screen_active = FALSE;

    EWAY_process(); // pre process map, stick it here Or we get stack overflow
    //	MUSIC_WORLD=(Random()%6)+1;

    ELEV_game_init_common(fname_map, fname_lighting, fname_citsez, fname_level);

    ATTRACT_loadscreen_draw(95 * 256 / 100);

    EWAY_load_fake_wander_text(fname_citsez);

    OB_make_all_the_switches_be_at_the_proper_height();

    OB_add_walkable_faces();

    calc_slide_edges();

    ATTRACT_loadscreen_draw(100 * 256 / 100);

    if (CNET_network_game) {
        FC_look_at(0, THING_NUMBER(NET_PERSON(PLAYER_ID)));
    } else {

        for (i = 0; i < NO_PLAYERS; i++) {
            if (NET_PERSON(i)) {
                FC_look_at(i, THING_NUMBER(NET_PERSON(i)));
                FC_setup_initial_camera(i);
            }
        }
    }

    OB_Mapwho* om;
    OB_Ob* oo;
    SLONG num;
    SLONG index;
    SLONG j;

    for (i = 0; i < OB_SIZE - 1; i++)
        for (j = 0; j < OB_SIZE - 1; j++) {
            om = &OB_mapwho[i][j];
            index = om->index;
            num = om->num;
            while (num--) {
                ASSERT(WITHIN(index, 1, OB_ob_upto - 1));
                oo = &OB_ob[index];
                if (oo->prim == 61)
                    MFX_play_xyz(0, S_ACIEEED, MFX_LOOPED | MFX_OVERLAP, ((i << 10) + (oo->x << 2)) << 8, oo->y << 8, ((j << 10) + (oo->z << 2)) << 8);
                index++;
            }
        }

    if (GAME_FLAGS & GF_RAINING) {
        PUDDLE_precalculate();
    }

    insert_collision_facets();

    Keys[KB_SPACE] = 0;
    Keys[KB_ENTER] = 0;
    Keys[KB_A] = 0;
    Keys[KB_Z] = 0;
    Keys[KB_X] = 0;
    Keys[KB_C] = 0;
    Keys[KB_V] = 0;
    Keys[KB_LEFT] = 0;
    Keys[KB_RIGHT] = 0;
    Keys[KB_UP] = 0;
    Keys[KB_DOWN] = 0;

    ATTRACT_loadscreen_draw(100 * 256 / 100);

    return TRUE;
}

void ELEV_create_similar_name(
    CBYTE* dest,
    CBYTE* src,
    CBYTE* ext)
{
    CBYTE* ch;
    CBYTE* ci;

    for (ch = src; *ch; ch++)
        ;

    while (1) {
        if (ch == src) {
            break;
        }

        if (*ch == '\\') {
            ch++;

            break;
        }

        ch--;
    }

    ci = dest;

    while (1) {
        *ci++ = *ch++;

        if (*ch == '.' || *ch == '\000') {
            break;
        }
    }

    *ci++ = '.';

    strcpy(ci, ext);
}

SLONG ELEV_load_name(CBYTE* fname_level)
{
    SLONG ans;

    CBYTE* fname_map;
    CBYTE* fname_lighting;
    CBYTE* fname_citsez;

    MFFileHandle handle;

    // Play FMV now, when we have enough memory to do so!
    if (strstr(ELEV_fname_level, "Finale1.ucm")) {
        if (GAME_STATE & GS_REPLAY) {
            //
            // Don't play cutscenes on replay.
            //

        } else {
            stop_all_fx_and_music();
            the_display.RunCutscene(2);

            // Reshow the "loading" screen.
            ATTRACT_loadscreen_init();
        }
    }

    if (fname_level == NULL) {
        return FALSE;
    }

    handle = FileOpen(fname_level);

    if (handle == FILE_OPEN_ERROR) {
        ASSERT(FALSE);
        return FALSE;
    }

    strcpy(ELEV_fname_level, fname_level); // I hope this is OK

    char junk[1000];

    if (FileRead(handle, junk, sizeof(SLONG)) == FILE_READ_ERROR)
        goto file_error; // Version number
    if (FileRead(handle, junk, sizeof(SLONG)) == FILE_READ_ERROR)
        goto file_error; // Used
    if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // BriefName

    if (FileRead(handle, ELEV_fname_lighting, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // LightMapName
    if (FileRead(handle, ELEV_fname_map, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // MapName
    if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // MissionName
    if (FileRead(handle, ELEV_fname_citsez, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // CitSezName

    FileClose(handle);

    fname_map = (ELEV_fname_map[0]) ? ELEV_fname_map : NULL;
    fname_lighting = (ELEV_fname_lighting[0]) ? ELEV_fname_lighting : NULL;
    fname_citsez = (ELEV_fname_citsez[0]) ? ELEV_fname_citsez : NULL;

    ans = ELEV_game_init(
        fname_map,
        fname_lighting,
        fname_citsez,
        fname_level);

    return ans;

file_error:;

    FileClose(handle);

    return TRUE;
}

extern MFFileHandle playback_file;

extern CBYTE tab_map_name[];

// extern SLONG EWAY_conv_active;
extern SLONG PSX_inv_open;

SLONG ELEV_load_user(SLONG mission)
{
    CBYTE* fname_map;
    CBYTE* fname_lighting;
    CBYTE* fname_citsez;
    CBYTE* fname_level;
    CBYTE curr_directory[_MAX_PATH];

    OPENFILENAME ofn;

    MFX_QUICK_stop();
    MUSIC_mode(0);
    MUSIC_mode_process();

try_again:;

    /*
            if(mission<0)
            {
                    //
                    // bodge for publishing meeting
                    //
                            {
                                    SLONG	c0;
                                    strcpy(tab_map_name,my_mission_names[-mission]);
                                    for(c0=0;c0<strlen(tab_map_name);c0++)
                                    {
                                            if(tab_map_name[c0]=='.')
                                            {
                                                    tab_map_name[c0+1]='t';
                                                    tab_map_name[c0+2]='g';
                                                    tab_map_name[c0+3]='a';

                                                    break;
                                            }
                                    }
                            }
                    return ELEV_load_name(my_mission_names[-mission]);

            }
    */

    GetCurrentDirectory(_MAX_PATH, curr_directory);

    if (GAME_STATE & GS_PLAYBACK) {
        UWORD c;

        // marker to indicate level name is included
        FileRead(playback_file, &c, 2);
        if (c == 1) {
            CBYTE temp[_MAX_PATH];
            // restore string
            FileRead(playback_file, &c, 2);
            FileRead(playback_file, temp, c);

            strcpy(ELEV_fname_level, curr_directory);
            if (ELEV_fname_level[strlen(ELEV_fname_level) - 1] != '\\')
                strcat(ELEV_fname_level, "\\");
            strcat(ELEV_fname_level, temp);
            return ELEV_load_name(ELEV_fname_level);

            /*
            strcpy(ELEV_fname_level,temp);
            return ELEV_load_name(&ELEV_fname_level[23]);  // The stupid thing saves the absolute address
            */
        }
    }

    // extern CBYTE* STARTSCR_mission;
    extern CBYTE STARTSCR_mission[_MAX_PATH];
    if (*STARTSCR_mission) {
        strcpy(ELEV_fname_level, STARTSCR_mission);

        if (GAME_STATE & GS_RECORD) {
            UWORD c = 1;
            CBYTE fname[_MAX_PATH], *cname;

            // marker to indicate level name is included
            FileWrite(playback_file, &c, 2);
            // store string
            strcpy(fname, ELEV_fname_level);
            cname = fname;
            if (strnicmp(fname, curr_directory, strlen(curr_directory)) == 0) {
                cname += strlen(curr_directory);
                if (*cname = '\\')
                    cname++;
            }
            c = strlen(cname) + 1; // +1 is to include terminating zero
            FileWrite(playback_file, &c, 2);
            FileWrite(playback_file, cname, c);
        }

        //		MemFree(STARTSCR_mission);
        SLONG res = ELEV_load_name(STARTSCR_mission);
        *STARTSCR_mission = 0;
        return res;
    }

    the_display.toGDI();

    SLONG ans = MessageBox(
        hDDLibWindow,
        "Do you want to load a level file?",
        "Load a (map + lighting + citsez) file or a single level file",
        MB_YESNOCANCEL | MB_APPLMODAL | MB_ICONQUESTION);

    switch (ans) {
    case IDYES:

        ELEV_fname_level[0] = 0;

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hDDLibWindow;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = "Level files\0*.ucm\0Wad files\0*.wad\0\0";
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = ELEV_fname_level;
        ofn.nMaxFile = _MAX_PATH;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = "Levels";
        ofn.lpstrTitle = "Load a level";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = "ucm";
        ofn.lCustData = NULL;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;

        if (!GetOpenFileName(&ofn)) {
            return FALSE;
        }

        SetCurrentDirectory(curr_directory);

        if (GAME_STATE & GS_RECORD) {
            UWORD c = 1;
            CBYTE fname[_MAX_PATH], *cname;

            // marker to indicate level name is included
            FileWrite(playback_file, &c, 2);
            // store string
            strcpy(fname, ELEV_fname_level);
            cname = fname;
            if (strnicmp(fname, curr_directory, strlen(curr_directory)) == 0) {
                cname += strlen(curr_directory);
                if (*cname = '\\')
                    cname++;
            }
            c = strlen(cname) + 1; // +1 is to include terminating zero
            FileWrite(playback_file, &c, 2);
            FileWrite(playback_file, cname, c);
        }

        /*

        {
                SLONG	c0;
                strcpy(tab_map_name,ELEV_fname_level);
                for(c0=0;c0<strlen(tab_map_name);c0++)
                {
                        if(tab_map_name[c0]=='.')
                        {
                                tab_map_name[c0+1]='t';
                                tab_map_name[c0+2]='g';
                                tab_map_name[c0+3]='a';

                                break;
                        }
                }
        }

        */

        if (ELEV_fname_level[strlen(ELEV_fname_level) - 3] == 'w') {
            extern void load_whole_game(CBYTE * gamename);

            load_whole_game(ELEV_fname_level);
            return (4);
        } else {
            if (ELEV_load_name(ELEV_fname_level))
                return (5);
            else
                return (0);
        }

        return (ELEV_load_name(ELEV_fname_level) ? 5 : 0);

    case IDNO:

        ELEV_fname_map[0] = 0;

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hDDLibWindow;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = "Game map files\0*.iam\0\0";
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = ELEV_fname_map;
        ofn.nMaxFile = _MAX_PATH;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = "data";
        ofn.lpstrTitle = "Load a game map";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = "iam";
        ofn.lCustData = NULL;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;

        if (!GetOpenFileName(&ofn)) {
            goto try_again;
        } else {
            fname_map = ELEV_fname_map;
        }

        ELEV_create_similar_name(
            ELEV_fname_lighting,
            ELEV_fname_map,
            "lgt");

        SetCurrentDirectory(curr_directory);

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hDDLibWindow;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = "Lighting files\0*.lgt\0\0";
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = ELEV_fname_lighting;
        ofn.nMaxFile = _MAX_PATH;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = "data\\lighting";
        ofn.lpstrTitle = "Load a lighting file";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = "lgt";
        ofn.lCustData = NULL;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;

        if (!GetOpenFileName(&ofn)) {
            fname_lighting = NULL;
        } else {
            fname_lighting = ELEV_fname_lighting;
        }

        ELEV_create_similar_name(
            ELEV_fname_citsez,
            ELEV_fname_map,
            "txt");

        SetCurrentDirectory(curr_directory);

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hDDLibWindow;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = "Text files\0*.txt\0\0";
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = ELEV_fname_citsez;
        ofn.nMaxFile = _MAX_PATH;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = "\\text";
        ofn.lpstrTitle = "Load a Citizen-sez text file";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = "txt";
        ofn.lCustData = NULL;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;

        if (!GetOpenFileName(&ofn)) {
            fname_citsez = NULL;
        } else {
            fname_citsez = ELEV_fname_citsez;
        }

        fname_level = NULL;

        SetCurrentDirectory(curr_directory);

        return ELEV_game_init(
            fname_map,
            fname_lighting,
            fname_citsez,
            fname_level);

    case IDCANCEL:
        return FALSE;

    default:
        return FALSE;
    }
}

void reload_level(void)
{
    ELEV_load_name(ELEV_fname_level);
}
