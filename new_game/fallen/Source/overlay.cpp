// claude-ai: === OVERLAY.CPP — HUD / IN-GAME OVERLAY SYSTEM ===
// claude-ai: Главная функция: OVERLAY_handle() (строка ~947) — вызывается КАЖДЫЙ КАДР из game loop
// claude-ai:
// claude-ai: АКТИВНЫЙ КОД (PC build):
// claude-ai:   OVERLAY_handle() — main per-frame HUD dispatcher:
// claude-ai:     1. Сброс D3D viewport на full screen (для letterbox mode)
// claude-ai:     2. PANEL_start() / PANEL_finish() — bracket для HUD рендеринга
// claude-ai:     3. PANEL_draw_buffered() — таймер обратного отсчёта (MM:SS), до 8 одновременно
// claude-ai:     4. OVERLAY_draw_gun_sights() — прицелы на целях (track_gun_sight заполняет каждый кадр)
// claude-ai:     5. OVERLAY_draw_enemy_health() — полоска HP над целью в FIGHT/AIM_GUN режиме
// claude-ai:     6. PANEL_last() — ОСНОВНОЙ HUD: здоровье (круговой), оружие+патроны, стамина (5 квадратов),
// claude-ai:        радар с маяками, текстовые сообщения/диалоги, crime rate, обратный отсчёт гранаты
// claude-ai:     7. PANEL_inventory() — список оружия при переключении
// claude-ai:     8. Debug info (cheat==2): FPS + координаты
// claude-ai:     9. GS_LEVEL_LOST/WON text — ЗАКОММЕНТИРОВАН на PC (обрабатывается GAMEMENU)
// claude-ai:
// claude-ai:   track_gun_sight() — регистрирует цель для прицела (MAX_TRACK=4, сброс каждый кадр)
// claude-ai:   OVERLAY_draw_gun_sights() — рисует прицелы: Person→голова(bone 11), BAT_BALROG→scale 450,
// claude-ai:     Special→128, Barrel→200, Vehicle→450; + show_grenade_path() если граната в руках
// claude-ai:   OVERLAY_draw_enemy_health() — PANEL_draw_local_health() в 3D мире:
// claude-ai:     MIB: hp*100/700; обычные: hp*100/health[PersonType]; Balrog: (100*hp)>>8
// claude-ai:
// claude-ai: МЁРТВЫЙ КОД:
// claude-ai:   show_help_text() — тело полностью закомментировано
// claude-ai:   DAMAGE_TEXT система — #ifdef DAMAGE_TEXT (НЕ определён) — плавающие числа урона в 3D
// claude-ai:   Beacons (minimap маркеры) — полностью закомментированы
// claude-ai:   ScoresDraw в overlay — закомментирован (перенесён в GAMEMENU)
// claude-ai:   CRIME_RATE display — закомментирован
// claude-ai:   PC search progress text — закомментирован (PSX версия через PANEL_draw_search)
// claude-ai:   init_punch_kick() — PSX only (NTSC кнопки punch/kick)
// claude-ai:
// claude-ai: PANEL_last() (в panel.cpp) детали HUD:
// claude-ai:   - Здоровье = круговой индикатор (8 сегментов, радиус 66px, arc -43° to -227°)
// claude-ai:     Health * (1/200) для Darci, (1/400) для Roper, (1/300) для Vehicle
// claude-ai:   - Стамина = 5 цветных квадратов (Stamina/25): red→orange→yellow→green→bright green
// claude-ai:   - Оружие = спрайт + счётчик патронов (m_iPanelXPos+170, m_iPanelYPos-63)
// claude-ai:   - Радар/маяки = центр PLH_MID (m_iPanelXPos+74, m_iPanelYPos-90), радиус 50px
// claude-ai:   - Базовая позиция панели: m_iPanelXPos=0/32, m_iPanelYPos=448/480
// claude-ai:
// claude-ai: Для новой игры: заменить D3D viewport на OpenGL; PANEL_* рисование → свой HUD renderer;
// claude-ai: сохранить логику: gun sights (per-frame tracking), enemy health (FIGHT/AIM mode),
// claude-ai: circular health, stamina marks, weapon display, radar beacons, timer system.
// claude-ai: ===
#include "game.h"
#include "..\headers\cam.h"
#include "..\headers\statedef.h"
#include "..\ddengine\headers\panel.h"
#include "fc.h"
#include "animate.h"
#include "memory.h"
#include "mav.h"
#include "vehicle.h"
#include "eway.h"
#include "ddlib.h"
#include "..\ddengine\headers\planmap.h"
#include "..\ddengine\headers\poly.h"
#include "font2d.h"
#include "interfac.h"

#include "eway.h"
#include "xlat_str.h"

extern void add_damage_text(SWORD x, SWORD y, SWORD z, CBYTE* text);

//
// local prototypes
//
void OVERLAY_draw_damage_values(void);

//
// This is for everything that gets overlayed during the game
// like panels and text and maps and ...
//

//
// All the draw routines to support this module should go in ddengine\source\panel.cpp
//

#define INFO_NUMBER 1
#define INFO_TEXT 2

struct TrackEnemy {
    Thing* PThing;
    SWORD State;
    SWORD Timer;
    SWORD Face;
};

#define MAX_TRACK 4

#define STATE_TRACKING 1
#define STATE_UNUSED 0

/*
#define	MAX_BEACON	5
struct Beacon
{
        UWORD	X;
        UWORD	Z;
        UWORD	Type;
        UWORD	OTHER;
};

struct	Beacon	beacons[MAX_BEACON];
UWORD	beacon_upto=1;
*/

struct TrackEnemy panel_enemy[MAX_TRACK];

#define USE_CAR (1)
#define USE_CABLE (2)
#define USE_LADDER (3)
#define PICKUP_ITEM (4)
#define USE_MOTORBIKE (5)
#define TALK (6)

#define HELP_MAX_COL 16

#define HELP_GRAB_CABLE 0
#define HELP_PICKUP_ITEM 1
#define HELP_USE_CAR 2
#define HELP_USE_BIKE 3

CBYTE* help_text[] = {
    "Jump up to grab cables",
    "Press action to pickup items",
    "Press action to use vehicles",
    "Press action to use bikes",
    ""
};

UWORD help_xlat[] = { X_GRAB_CABLE, X_PICK_UP, X_ENTER_VEHICLE, X_USE_BIKE };

SLONG should_i_add_message(SLONG type)
{
    static SLONG last_message[4] = { 0, 0, 0, 0 }; // The gameturn when the last message was added.

    ASSERT(WITHIN(type, 0, 3));

    if (last_message[type] > GAME_TURN || last_message[type] <= GAME_TURN - 100) {
        last_message[type] = GAME_TURN;

        return TRUE;
    } else {
        return FALSE;
    }
}

void arrow_object(Thing* p_special, SLONG dir, SLONG type)
{
    SLONG x, y, z;

    if (!should_i_add_message(type)) {
        return;
    }

    x = p_special->WorldPos.X >> 8;
    y = p_special->WorldPos.Y >> 8;
    z = p_special->WorldPos.Z >> 8;

    y += ((GAME_TURN + THING_INDEX(p_special)) << 3) & 63;

    // add_damage_text(x,y,z, help_text[type]);
    add_damage_text(x, y, z, XLAT_str(help_xlat[type]));

    /*

    if(dir==1)
    {
            AENG_world_line(x,y,z,1,0xff0000,x,y+64,z,50,0x800000,FALSE);
            AENG_world_line(x,y+64,z,20,0xff0000,x,y+128,z,20,0x800000,FALSE);
    }
    else
    {
            AENG_world_line(x,y+128,z,1,0xff0000,x,y+64,z,50,0x800000,FALSE);
            AENG_world_line(x,y+64,z,20,0xff0000,x,y,z,20,0x800000,FALSE);
    }

    */
}

void arrow_pos(SLONG x, SLONG y, SLONG z, SLONG dir, SLONG type)
{
    y += ((GAME_TURN + x) << 3) & 63;

    if (!should_i_add_message(type)) {
        return;
    }

    //	add_damage_text(x,y,z, help_text[type]);
    add_damage_text(x, y, z, XLAT_str(help_xlat[type]));

    /*

    if(dir==1)
    {
            AENG_world_line(x,y,z,1,0xff0000,x,y+64,z,50,0x800000,FALSE);
            AENG_world_line(x,y+64,z,20,0x800000,x,y+128,z,20,0x400000,FALSE);
    }
    else
    {
            AENG_world_line(x,y-0,z,1,0xff0000,x,y-64,z,50,0x800000,FALSE);
            AENG_world_line(x,y-64,z,20,0x800000,x,y-128,z,20,0x400000,FALSE);
    }
    */
}

void Time(struct MFTime* the_time);

void show_help_text(SLONG index)
{
    /*
#ifndef	PSX
    static	ULONG	last_message=0;
    struct MFTime 	the_time;
    ULONG	time_now;

    Time(&the_time);
    time_now=the_time.Ticks;

    if(time_now-last_message>14000)
    {
            last_message=time_now;
            PANEL_new_help_message(help_text[index]);
    }
#endif
    */
}

void track_enemy(Thing* p_thing)
{
}

struct TrackEnemy panel_gun_sight[MAX_TRACK];
UWORD track_count = 0;

void track_gun_sight(Thing* p_thing, SLONG accuracy)
{
    SLONG c0;
    SLONG unused = -1;
    for (c0 = 0; c0 < track_count; c0++) {
        if (panel_gun_sight[c0].PThing == p_thing) {
            if (accuracy < panel_gun_sight[c0].Timer)
                panel_gun_sight[c0].Timer = accuracy;
            return;
        }
    }
    if (track_count < MAX_TRACK) {
        panel_gun_sight[track_count].Timer = accuracy;
        panel_gun_sight[track_count].PThing = p_thing;
        track_count++;
    }
    /*
            for(c0=0;c0<MAX_TRACK;c0++)
            {
                    if(panel_gun_sight[c0].PThing==p_thing)
                    {
                            panel_gun_sight[c0].State=STATE_TRACKING;
                            panel_gun_sight[c0].Timer=5000; //5 seconds
                            panel_gun_sight[c0].Face=accuracy; //5 seconds
                            return;
                    }
                    else
                    if(panel_gun_sight[c0].State==STATE_UNUSED)
                    {
                            unused=c0;
                    }
            }
            if(unused>=0)
            {
                    SLONG	face;
                    panel_gun_sight[unused].PThing=p_thing;
                    panel_gun_sight[unused].State=STATE_TRACKING;
                    panel_gun_sight[unused].Timer=15000; //5 seconds
                    panel_gun_sight[unused].Face=accuracy; //5 seconds
            }
    */
}

void OVERLAY_draw_tracked_enemies(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_TRACK; c0++) {
        if (panel_enemy[c0].State == STATE_TRACKING) {
            SLONG h;

            /* draw face */

            h = panel_enemy[c0].PThing->Genus.Person->Health;
            if (h < 0)
                h = 0;
            void PANEL_draw_face(SLONG x, SLONG y, SLONG face, SLONG size);
            PANEL_draw_face(c0 * 150 + 5, 450 - 14, panel_enemy[c0].Face, 32);
            PANEL_draw_health_bar(40 + c0 * 150, 450, h >> 1);

            PANEL_draw_health_bar(40 + c0 * 150, 460, (GET_SKILL(panel_enemy[c0].PThing) * 100) / 15);

            panel_enemy[c0].Timer -= TICK_TOCK;
            //			if(panel_enemy[c0].Timer<0)
            {
                panel_enemy[c0].State = STATE_UNUSED;
                panel_enemy[c0].PThing = 0;
            }
        }
    }
}

void OVERLAY_draw_gun_sights(void)
{
    SLONG c0;
    SLONG hx, hy, hz;
    Thing* p_thing;

    // Dodgy internals issue :-)
    POLY_flush_local_rot();

    for (c0 = 0; c0 < track_count; c0++) {
        p_thing = panel_gun_sight[c0].PThing;
        switch (p_thing->Class) {
        case CLASS_PERSON:

            // 11 is head
            calc_sub_objects_position(p_thing, p_thing->Draw.Tweened->AnimTween, 11, &hx, &hy, &hz);

            hx += p_thing->WorldPos.X >> 8;
            hy += p_thing->WorldPos.Y >> 8;
            hz += p_thing->WorldPos.Z >> 8;

            PANEL_draw_gun_sight(hx, hy, hz, panel_gun_sight[c0].Timer, 256);
            break;
        case CLASS_SPECIAL:
            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 30, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, 128);
            break;
        case CLASS_BAT:

        {
            SLONG scale;

            if (p_thing->Genus.Bat->type == BAT_TYPE_BALROG) {
                scale = 450;
            } else {
                scale = 128;
            }

            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 30, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, scale);
        }

        break;
        case CLASS_BARREL:
            p_thing = panel_gun_sight[c0].PThing;
            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 80, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, 200);
            break;
        case CLASS_VEHICLE:
            p_thing = panel_gun_sight[c0].PThing;
            PANEL_draw_gun_sight(p_thing->WorldPos.X >> 8, (p_thing->WorldPos.Y >> 8) + 80, p_thing->WorldPos.Z >> 8, panel_gun_sight[c0].Timer, 450);
            break;
        }
    }
    track_count = 0;

    // draw a grenade aim target
    extern SLONG person_holding_special(Thing * p_person, UBYTE special); // I am so naughty.  I blame Mark.

    Thing* p_player = NET_PERSON(0);

    if (person_holding_special(p_player, SPECIAL_GRENADE)) {
        /*
                        SLONG	angle = p_player->Draw.Tweened->Angle;

                        // now make the angle more conformant with normality
                        angle = 1536 - angle;

                        // mask.  I don't mind that.
                        angle &= 2047;

                        // fortunately the grenade is now frame-rate independant (thanks to me, hurrah!)
                        SLONG	addx = COS(angle) * 2150 / 65536;
                        SLONG	addz = SIN(angle) * 2150 / 65536;

                        PANEL_draw_gun_sight((p_player->WorldPos.X >> 8) + addx, (p_player->WorldPos.Y >> 8) + 128, (p_player->WorldPos.Z >> 8) + addz, 1000, 200);
        */

        if (!p_player->Genus.Person->Ware) {

            void show_grenade_path(Thing * p_person);
            show_grenade_path(p_player);
        }
    }
}

void OVERLAY_draw_health(void)
{
    SLONG ph;
    ph = NET_PERSON(0)->Genus.Person->Health;
    if (ph < 0)
        ph = 0;
    PANEL_draw_health_bar(10, 10, ph >> 1);
}

void OVERLAY_draw_stamina(void)
{
    SLONG ph;
    ph = NET_PERSON(0)->Genus.Person->Stamina;
    if (ph < 0)
        ph = 0;
    PANEL_draw_health_bar(10, 30, (ph * 100) >> 8);
}

extern void PANEL_draw_local_health(SLONG mx, SLONG my, SLONG mz, SLONG percentage, SLONG radius = 60);

void OVERLAY_draw_enemy_health(void)
{
    Thing* p_person;

    p_person = NET_PERSON(0);

    if (p_person->Genus.Person->Mode == PERSON_MODE_FIGHT || p_person->SubState == SUB_STATE_AIM_GUN) {
        if (p_person->Genus.Person->Target) {
            Thing* p_target;
            p_target = TO_THING(p_person->Genus.Person->Target);

            switch (p_target->Class) {
            case CLASS_BAT:
                if (p_target->Genus.Bat->type == BAT_TYPE_BALROG) {
                    PANEL_draw_local_health(p_target->WorldPos.X >> 8, p_target->WorldPos.Y >> 8, p_target->WorldPos.Z >> 8, (100 * p_target->Genus.Bat->health) >> 8, 300);
                }
                break;
            case CLASS_PERSON:

            {
                SLONG percent;

                extern BOOL PersonIsMIB(Thing * p_person);

                if (PersonIsMIB(p_target)) {
                    percent = p_target->Genus.Person->Health * 100 / 700;
                } else {
                    //							percent = p_target->Genus.Person->Health >> 1;

                    percent = p_target->Genus.Person->Health * 100 / health[p_target->Genus.Person->PersonType];
                }

                PANEL_draw_local_health(p_target->WorldPos.X >> 8, p_target->WorldPos.Y >> 8, p_target->WorldPos.Z >> 8, percent);
            }

            break;
            }
        }
    }
}

static SWORD timer_prev = 0;

void OVERLAY_handle(void)
{
    Thing* darci = NET_PERSON(0);
    Thing* player = NET_PLAYER(0);
    SLONG panel = 1;

    // TRACE ( "OHi" );

    CBYTE str[8];

    /*

    if (darci && !(darci->Flags & FLAGS_ON_MAPWHO))
    {
            panel=0;
    }
    */

    // Internal gubbins.
    POLY_flush_local_rot();

    // Reset the viewport so that text, etc gets drawn even when in letterbox mode.

    // HRESULT hres = (the_display.lp_D3D_Viewport)->EndScene();

    D3DVIEWPORT2 viewData;
    memset(&viewData, 0, sizeof(D3DVIEWPORT2));
    viewData.dwSize = sizeof(D3DVIEWPORT2);

    // A horrible hack for letterbox mode.
    viewData.dwWidth = RealDisplayWidth;
    viewData.dwHeight = RealDisplayHeight;
    viewData.dwX = 0;
    viewData.dwY = 0;
    viewData.dvClipX = -1.0f;
    viewData.dvClipY = 1.0f;
    viewData.dvClipWidth = 2.0f;
    viewData.dvClipHeight = 2.0f;
    viewData.dvMinZ = 0.0f;
    viewData.dvMaxZ = 1.0f;
    HRESULT hres = (the_display.lp_D3D_Viewport)->SetViewport2(&viewData);

    // hres = (the_display.lp_D3D_Viewport)->BeginScene();

    PANEL_start();

    if (!EWAY_stop_player_moving()) {
        if (panel) {
            // This is all fucked - don't do it.
            PANEL_draw_buffered();
            OVERLAY_draw_gun_sights();
            OVERLAY_draw_enemy_health();
        }
    }

    //	OVERLAY_draw_damage_values();
    if (panel) {

        /*
                        if (Keys[KB_B])
                        {
                                PANEL_new_funky();
                        }
                        else
        */
        {

            if (!(GAME_STATE & GS_LEVEL_WON)) {
                PANEL_last();
            }
        }
    }

    //	if((GAME_TURN&15)==0)

    extern UBYTE draw_map_screen;
    /*
            if (!draw_map_screen)
            {
                    help_system();
            }
    */

    if (!draw_map_screen) {
        // Waste not Want no, why have we got 50 bytes.

        //		CBYTE	str[50];
        SLONG crime;
        /*
                        if(!EWAY_stop_player_moving())
                        if(CRIME_RATE_MAX)
                        {
                                //
                                // we have a crime rate
                                //
                                crime=(CRIME_RATE*100)/CRIME_RATE_MAX;
                                sprintf(str,"CRIME RATE %d%%",crime);
                                FONT2D_DrawString(str,400,20);
                        }
        */

        if (darci) {
            if (darci->State == STATE_SEARCH) {
                /*

                SLONG percent = darci->Genus.Person->Timer1 >> 8;

                SATURATE(percent, 0, 100);

//				CBYTE	str[50];
                sprintf(str,"%d%%", percent);

                if ((darci->Genus.Person->Timer1 & 0xfff) < 3000 || percent == 100)
                {
                        FONT2D_DrawStringCentred(
//						(percent == 100 ? "Complete" : "Searching"),
                                XLAT_str( (percent == 100 ? X_COMPLETE : X_SEARCHING) ),
                                DisplayWidth  / 2,
                                DisplayHeight / 2 - 10,
                                0x00ff00,
                                256);
                }

                FONT2D_DrawStringCentred(
                        str,
                        DisplayWidth  / 2,
                        DisplayHeight / 2 + 20,
                        0x00ff00,
                        512);

                */
            }
        }
    }

    PANEL_inventory(darci, player);

    // #if 0
    //  I have found the offending code, Holmes!
    //  Now we must teach these heathen a lesson in coding manners.
    //  I shall fetch the larger of my beating sticks.
    extern UBYTE cheat;

    if (cheat == 2) {
        CBYTE str[50];
        ULONG in;

        extern SLONG tick_tock_unclipped;
        if (tick_tock_unclipped == 0)
            tick_tock_unclipped = 1;

        extern SLONG SW_tick1;
        extern SLONG SW_tick2;
        extern ULONG debug_input;
        in = debug_input;

        extern SLONG geom;
        extern SLONG EWAY_cam_jumped;
        extern SLONG look_pitch;

        //		sprintf(str,"(%d,%d,%d) fps %d up %d down %d left %d right %d geom %d",darci->WorldPos.X>>16,darci->WorldPos.Y>>16,darci->WorldPos.Z>>16,((1000)/tick_tock_unclipped)+1,in&INPUT_MASK_FORWARDS,in&INPUT_MASK_BACKWARDS,in&INPUT_MASK_LEFT,in&INPUT_MASK_RIGHT,geom);

        // for eidos build		sprintf(str,"(%d,%d,%d) fps %d render ticks %d",darci->WorldPos.X>>16,darci->WorldPos.Y>>16,darci->WorldPos.Z>>16,((1000)/tick_tock_unclipped)+1, SW_tick2 - SW_tick1);
        sprintf(str, "(%d,%d,%d) fps %d", darci->WorldPos.X >> 16, darci->WorldPos.Y >> 16, darci->WorldPos.Z >> 16, ((1000) / tick_tock_unclipped) + 1);

        FONT2D_DrawString(str, 2, 2, 0xffffff, 256);
    }

    // #endif

    if (GAME_STATE & GS_LEVEL_LOST) {

        /*

        FONT2D_DrawStringCentred(
                "Level Lost",
                DisplayWidth  / 2,
                DisplayHeight / 2 - 10,
                0x00ff00,
                512);

                if(SAVE_VALID)
                {
                        FONT2D_DrawStringCentred(
                                "R-Load AutoSave    Space- Quit",
                                DisplayWidth  / 2,
                                DisplayHeight / 2 + 40,
                                0x00ff00,
                                512);
                }
                else
                {
                        FONT2D_DrawStringCentred(
                                "R- replay    Space- Quit",
                                DisplayWidth  / 2,
                                DisplayHeight / 2 + 40,
                                0x00ff00,
                                512);
                }

        */

    } else if (GAME_STATE & GS_LEVEL_WON) {

        /*

        FONT2D_DrawStringCentred(
                "Level Complete",
                DisplayWidth  / 2,
                DisplayHeight / 2 - 10,
                0x00ff00,
                512);

        FONT2D_DrawStringCentred(
                "Space to Continue",
                DisplayWidth  / 2,
                DisplayHeight / 2 + 40,
                0x00ff00,
                512);

        */
    }

    /*

    //
    // Draw useful info...
    //

    {
            CBYTE str[58];

            sprintf(str, "%d", NET_PERSON(0)->Draw.Tweened->Roll);

            FONT2D_DrawStringCentred(
                    str,
                    DisplayWidth  / 2,
                    30,
                    0x00ff00,
                    16);
    }

    */

    PANEL_finish();
    /*
            if (GAME_STATE & GS_LEVEL_WON)
            {
                    extern void ScoresDraw(void);	// From attract

                    ScoresDraw();
            }
    */

    // TRACE ( "OHo" );
}

/*
void	set_beacon(SLONG bx,SLONG by,SLONG bz)
{

        if(beacon_upto<(MAX_BEACON-1))
        {
                beacons[beacon_upto].X=bx;
                beacons[beacon_upto].Z=bz;
                beacon_upto++;
        }

}


ULONG	col_type[]=
{
        0xff0000,
        0xff0000,
        0x00ff00,
        0x00ffff,
        0x000080,
        0x0000a0,
        0x0000c0,
        0x0000e0,
        0x0000ff,
        0x0000ff,
        0x0000ff,
        0x0000ff,
        0x0000ff
};
*/

void overlay_beacons(void)
{
    /*
            SLONG	c0;
            Thing			*t_thing;
            THING_INDEX		current_thing;

            current_thing	=	PRIMARY_USED;
            while(current_thing)
            {
                    t_thing			=	TO_THING(current_thing);
                    current_thing	=	t_thing->LinkChild;

                    if(t_thing->Class==CLASS_PERSON)
                    {
                            map_beacon_draw(
                                    t_thing->WorldPos.X>>8,
                                    t_thing->WorldPos.Z>>8,
                                    col_type[t_thing->Genus.Person->PersonType],
                                    (t_thing->Genus.Person->PlayerID) ? BEACON_FLAG_POINTY : 0,
                                    t_thing->Draw.Tweened->Angle);
                    }
            }
    //	map_beacon_draw(NET_PERSON(0)->WorldPos.X>>8,NET_PERSON(0)->WorldPos.Z>>8);

            for(c0=1;c0<beacon_upto;c0++)
            {
                    map_beacon_draw(beacons[c0].X,beacons[c0].Z,0x0000ff,BEACON_FLAG_BEACON,0);

            }
    */
}

void add_damage_value(SWORD x, SWORD y, SWORD z, SLONG value)
{
}

void add_damage_text(SWORD x, SWORD y, SWORD z, CBYTE* text)
{
}

void add_damage_value_thing(Thing* p_thing, SLONG value)
{
}

void init_overlay(void)
{
    //	beacon_upto=1;
    //	memset((UBYTE*)beacons,0,sizeof(struct	Beacon)*MAX_BEACON);
    memset((UBYTE*)panel_enemy, 0, sizeof(struct TrackEnemy) * MAX_TRACK);
}
