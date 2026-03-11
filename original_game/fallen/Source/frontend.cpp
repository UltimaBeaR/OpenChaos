//
// frontend.cpp
//
// matthew rosenfeld 8 july 99
//
// this is our new front end thingy to replace the hideous startscr.cpp
//


//#define BIN_STUFF_PLEASE_BOB I have been defined
//#define FORCE_STUFF_PLEASE_BOB I have been defined
#ifdef DEBUG
//#define BIN_BACKGROUNDS_PLEASE_BOB I have been defined
#else
//#define BIN_BACKGROUNDS_PLEASE_BOB I have been defined
#endif

// On a PC, you need an exit. On a console, you don't.
#define WANT_AN_EXIT_MENU_ITEM defined
// Keyboard not currently supported on DC - might be in future.
#define WANT_A_KEYBOARD_ITEM define
// On DC, the "Start" button is not allowed to be remapped.
#define WANT_A_START_JOYSTICK_ITEM defined


#ifndef PSX

#include	"demo.h"
#include	"DDLib.h"
#include "frontend.h"
#include "xlat_str.h"
#include "menufont.h"
#include "font2d.h"
#include "env.h"
#include "drive.h"
#include "sound.h"
#include "MFX.h"
#include "music.h"
#include "poly.h"
#include "drawxtra.h"
#include "fmatrix.h"
#include "..\DDLibrary\headers\D3DTexture.h"
#include "..\DDLibrary\headers\GDisplay.h"
#include "..\DDEngine\headers\polypage.h"
#include "io.h"
#include "truetype.h"
#include "..\ddlibrary\headers\dclowlevel.h"
#include "panel.h"

#include "interfac.h"
extern	BOOL	allow_debug_keys;

//----------------------------------------------------------------------------
// EXTERNS
//

extern SLONG FontPage;
extern UBYTE InkeyToAscii[];
extern UBYTE InkeyToAsciiShift[];
extern CBYTE STARTSCR_mission[_MAX_PATH];
extern DIJOYSTATE			the_state;

extern void	init_joypad_config(void);

void	FRONTEND_display ( void );

//----------------------------------------------------------------------------
// DEFINES
//

#define ALLOW_JOYPAD_IN_FRONTEND


// font sizes, where 256 is "normal".
#define BIG_FONT_SCALE (192)
#define BIG_FONT_SCALE_FRENCH ( 176 );

#ifdef TARGET_DC
#define BIG_FONT_SCALE_SELECTED (256)
#endif
#define SMALL_FONT_SCALE (256)
// small font being larger DOES make sense because they're different bitmaps.

// In milliseconds.
#ifdef DEBUG
// For debugging the looper
#define AUTOPLAY_FMV_DELAY (1000 * 10)
#else
#define AUTOPLAY_FMV_DELAY (1000 * 60 * 2)
#endif

// #define ALLOW_DANGEROUS_OPTIONS

//#define MISSION_SCRIPT "data\\urban.sty"
// see below...

#if 0
#define	STARTS_START	1
#define	STARTS_EDITOR	2
#define	STARTS_LOAD		3	
#define	STARTS_EXIT		4	
#define STARTS_LANGUAGE_CHANGE 10
#else
#include "startscr.h"
#endif

// just know someone's gonna want me to take it back out again soooo.....
#ifndef VERSION_DEMO
  //    ^^^^ see, i was right
  #define ANNOYING_HACK_FOR_SIMON		1
#endif


#define FE_MAINMENU			( 1)
#define FE_MAPSCREEN		( 2)
#define FE_MISSIONSELECT	( 3)
#define FE_MISSIONBRIEFING	( 4)
#define FE_LOADSCREEN		( 5)
#define FE_SAVESCREEN		( 6)
#define FE_CONFIG			( 7)
#define FE_CONFIG_VIDEO		( 8)
#define FE_CONFIG_AUDIO		( 9)
#ifdef WANT_A_KEYBOARD_ITEM
#define FE_CONFIG_INPUT_KB  (10)
#endif
#define FE_CONFIG_INPUT_JP  (11)
#define FE_CONFIG_OPTIONS	(13)
#ifdef WANT_AN_EXIT_MENU_ITEM
#define FE_QUIT				(12)
#endif
#define FE_SAVE_CONFIRM		(14)

#ifdef WANT_AN_EXIT_MENU_ITEM
#define FE_NO_REALLY_QUIT	(-1)
#endif
#define FE_BACK				(-2)
#define FE_START			(-3)
#define FE_EDITOR			(-4)
#define FE_CREDITS			(-5)
#ifdef TARGET_DC
#define FE_CHANGE_JOYPAD	(-6)
#endif

#define OT_BUTTON			( 1)
#define OT_BUTTON_L			( 2)
#define OT_SLIDER			( 3)
#define OT_MULTI			( 4)
#define OT_KEYPRESS			( 5)
#define OT_LABEL			( 6)
#define OT_PADPRESS			( 7)
#define OT_RESET			( 8)
#define OT_PADMOVE			( 9)

#define	MC_YN				(CBYTE*)( 1)
#define	MC_SCANNER			(CBYTE*)( 2)


//----------------------------------------------------------------------------
// STRUCTS
//

struct MissionData {
	SLONG	ObjID, GroupID, ParentID, ParentIsGroup;
	SLONG	District;
	SLONG	Type, Flags;
	CBYTE	fn[255], ttl[255], brief[4096];
};

struct RawMenuData {
	UBYTE	Menu;
	UBYTE	Type;
//	CBYTE*	Label;
	SWORD	Label;
	CBYTE*	Choices;
	SLONG	Data;
};

struct MenuData {
	UBYTE	Type;
	UBYTE	LabelID;
	CBYTE*	Label;
	CBYTE*	Choices;
	SLONG	Data;
	UWORD	X,Y;
};

struct MenuStack {
	UBYTE		mode;
	UBYTE		selected;
	SWORD		scroll;
	CBYTE*		title;
};

struct MenuState {
	UBYTE		items;
	UBYTE		selected;
	SWORD		scroll;
	MenuStack	stack[10];
	UBYTE		stackpos;
	SBYTE		mode;
	SWORD		base;
	CBYTE*		title;
};

struct Kibble {
	SLONG	page;
	SLONG	x, y;
	SWORD	dx,dy;
	SWORD	r, t, p, rd, td, pd;
	UBYTE	type, size;
	ULONG	rgb;
};

struct MissionCache {
	CBYTE name[255];
	UBYTE id;
	UBYTE district;
};

//
// This is the order we recommend the missions be played in...
//

CBYTE *suggest_order[] =
{
	"testdrive1a.ucm",
	"FTutor1.ucm",
	"Assault1.ucm",
	"police1.ucm",
	"police2.ucm",
	"Bankbomb1.ucm",
	"police3.ucm",
	"Gangorder2.ucm",
	"police4.ucm",
	"bball2.ucm",
	"wstores1.ucm",
	"e3.ucm",
	"westcrime1.ucm",
	"carbomb1.ucm",
	"botanicc.ucm",
	"Semtex.ucm",
	"AWOL1.ucm",
	"mission2.ucm",
	"park2.ucm",
	"MIB.ucm",
	"skymiss2.ucm",
	"factory1.ucm",
	"estate2.ucm",
	"Stealtst1.ucm",
	"snow2.ucm",
	"Gordout1.ucm",
	"Baalrog3.ucm",
	"Finale1.ucm",

	"Gangorder1.ucm",
	"FreeCD1.ucm",
	"jung3.ucm",

	"fight1.ucm",
	"fight2.ucm",
	"testdrive2.ucm",
	"testdrive3.ucm",

	"Album1.ucm",

	"!"
};

//
// The script... biggest one at the moment is 17k
// 

#define SCRIPT_MEMORY (20 * 1024)

CBYTE  loaded_in_script[SCRIPT_MEMORY];
CBYTE *loaded_in_script_read_upto;

void CacheScriptInMemory(CBYTE *script_fname)
{
	//FILE *handle = MF_Fopen(script_fname, "rb");
	// Er... it's a TEXT FILE
	FILE *handle = MF_Fopen(script_fname, "rt");

	ASSERT(handle);

	if (handle)
	{
		int iLoaded = fread(loaded_in_script, 1, SCRIPT_MEMORY, handle);
		ASSERT ( iLoaded < sizeof(loaded_in_script) );
		// Stupid thing doesn't always terminate with a
		// zero, and sometimes writes crap past the end.
		loaded_in_script[iLoaded] = '\0';

		MF_Fclose(handle);
	}
}

void FileOpenScript(void)
{
	loaded_in_script_read_upto = loaded_in_script;
}

CBYTE *LoadStringScript(CBYTE *txt) {
	CBYTE *ptr=txt;

	ASSERT(loaded_in_script_read_upto);

	*ptr=0;
	while (1) {

		*ptr = *loaded_in_script_read_upto++;

		if (*ptr == 0) {
			return txt;
		};
		if (*ptr==10) break;
		ptr++;
	}
	*(++ptr)=0;
	return txt;
}

void FileCloseScript(void)
{
	loaded_in_script_read_upto = NULL;
}


//----------------------------------------------------------------------------
// CONSTANTS
//

RawMenuData raw_menu_data[] = {
	{		FE_MAINMENU,	OT_BUTTON,	X_START,		0,	FE_MAPSCREEN		},
#ifndef VERSION_DEMO
	{				  0,	OT_BUTTON,	X_LOAD_GAME,	0,	FE_LOADSCREEN		},
	{				  0,	OT_BUTTON,	X_SAVE_GAME,	(CBYTE*)1,	FE_SAVESCREEN		},
#endif
	{				  0,	OT_BUTTON,	X_OPTIONS,		0,	FE_CONFIG			},
#if !defined(NDEBUG) && !defined(TARGET_DC)
	// sheer MADNESS!
	{				  0,	OT_BUTTON,	X_LOAD_UCM, 	0,	FE_START			},
	{				  0,	OT_BUTTON,	X_EDITOR,		0,  FE_EDITOR			},
#endif
#ifndef VERSION_DEMO
	{				  0,	OT_BUTTON,	X_CREDITS,		0,	FE_CREDITS			},
#endif
#ifdef WANT_AN_EXIT_MENU_ITEM
	{				  0,	OT_BUTTON,	X_EXIT,			0,	FE_QUIT				},
#endif
    {     FE_LOADSCREEN,    OT_BUTTON,  X_CANCEL,       0,  FE_BACK             },
    {     FE_SAVESCREEN,    OT_BUTTON,  X_CANCEL,       0,  FE_MAPSCREEN        },
	{	FE_SAVE_CONFIRM,    OT_LABEL,	X_ARE_YOU_SURE ,0,	0					},
	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_MAPSCREEN     	},
	{				  0,	OT_BUTTON,	X_CANCEL,		0,	FE_BACK				},
	{		  FE_CONFIG,	OT_BUTTON,	X_OPTIONS,		0,	FE_CONFIG_OPTIONS	},
	{		          0,	OT_BUTTON,	X_GRAPHICS,		0,	FE_CONFIG_VIDEO		},
	{				  0,	OT_BUTTON,	X_SOUND,		0,	FE_CONFIG_AUDIO		},
#ifdef WANT_A_KEYBOARD_ITEM
	{				  0,	OT_BUTTON,	X_KEYBOARD,		0,	FE_CONFIG_INPUT_KB	},
#endif
	{				  0,	OT_BUTTON,	X_JOYPAD,		0,	FE_CONFIG_INPUT_JP	},
	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_BACK				},
	{	FE_CONFIG_AUDIO,	OT_SLIDER,	X_VOLUME,		0,	128					},
	{				  0,	OT_SLIDER,	X_AMBIENT,		0,	128					},
	{				  0,	OT_SLIDER,	X_MUSIC,		0,	128					},
	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_BACK				},
#ifdef WANT_A_KEYBOARD_ITEM
	{FE_CONFIG_INPUT_KB,  OT_KEYPRESS,	X_LEFT,			0,	0,					},
	{				  0,  OT_KEYPRESS,	X_RIGHT,		0,	0,					},
	{				  0,  OT_KEYPRESS,	X_FORWARDS,		0,	0,					},
	{				  0,  OT_KEYPRESS,	X_BACKWARDS,	0,	0,					},
	{				  0,  OT_KEYPRESS,	X_PUNCH,		0,	0,					},
	{				  0,  OT_KEYPRESS,	X_KICK,			0,	0,					},
	{				  0,  OT_KEYPRESS,	X_ACTION,		0,	0,					},
//	{				  0,  OT_KEYPRESS,	X_RUN,			0,	0,					},
	{				  0,  OT_KEYPRESS,	X_JUMP,			0,	0,					},
#ifdef WANT_A_START_JOYSTICK_ITEM
	{				  0,  OT_KEYPRESS,	X_START,		0,	0,					},
#endif
	{				  0,  OT_KEYPRESS,	X_SELECT,		0,	0,					},
	{				  0,	 OT_LABEL,	X_CAMERA,		0,	0,					},
	{				  0,  OT_KEYPRESS,	X_JUMP,			0,	0,					},
	{				  0,  OT_KEYPRESS,	X_LEFT,			0,	0,					},
	{				  0,  OT_KEYPRESS,	X_RIGHT,		0,	0,					},
	{				  0,  OT_KEYPRESS,	X_LOOK_AROUND,	0,	0,					},
	{				  0,     OT_RESET,  X_RESET_DEFAULT,0,  0,					},
	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_BACK,			},
#endif

	{FE_CONFIG_INPUT_JP,  OT_PADPRESS,	X_KICK,			0,	0,					},
	{				  0,  OT_PADPRESS,	X_PUNCH,		0,	0,					},
	{				  0,  OT_PADPRESS,	X_JUMP,			0,	0,					},
	{				  0,  OT_PADPRESS,	X_ACTION,		0,	0,					},
	{				  0,  OT_PADPRESS,	X_RUN,			0,	0,					},
#ifdef WANT_A_START_JOYSTICK_ITEM
	{				  0,  OT_PADPRESS,	X_START,		0,	0,					},
#endif
	{				  0,  OT_PADPRESS,	X_SELECT,		0,	0,					},
	{				  0,	 OT_LABEL,	X_CAMERA,		0,	0,					},
	{				  0,  OT_PADPRESS,	X_JUMP_CAM,		0,	0,					},
	{				  0,  OT_PADPRESS,	X_LEFT,			0,	0,					},
	{				  0,  OT_PADPRESS,	X_RIGHT,		0,	0,					},
	{				  0,  OT_PADPRESS,	X_LOOK_AROUND,	0,	0,					},
	{				  0,     OT_RESET,  X_RESET_DEFAULT,0,  0,					},

	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_BACK,			},


//	{	FE_CONFIG_VIDEO,	OT_SLIDER,	X_DETAIL,		0,	128,				},
#ifdef ALLOW_DANGEROUS_OPTIONS
	{   FE_CONFIG_VIDEO,     OT_LABEL,  X_GRAPHICS,     0,  1,                  },
	{                 0,     OT_MULTI,  X_RESOLUTION,   0,  1,                  },
	{                 0,     OT_MULTI,  X_DRIVERS,	    0,  1,                  },
	{				  0,	 OT_MULTI,  X_COLOUR_DEPTH, 0,  1,					},
#endif
//	{                 0,     OT_LABEL,  X_DETAIL,	    0,  1,                  },
	{   FE_CONFIG_VIDEO,     OT_MULTI,  X_STARS,	MC_YN,  1,                  },
	{                 0,     OT_MULTI,  X_SHADOWS,	MC_YN,  1,                  },
	{                 0,     OT_MULTI,  X_PUDDLES,	MC_YN,  1,                  },
	{                 0,     OT_MULTI,  X_DIRT, 	MC_YN,  1,                  },
	{                 0,     OT_MULTI,  X_MIST, 	MC_YN,  1,                  },
	{                 0,     OT_MULTI,  X_RAIN, 	MC_YN,  1,                  },
	{                 0,     OT_MULTI,  X_SKYLINE,	MC_YN,  1,                  },
	{				  0,	 OT_MULTI,  X_CRINKLES, MC_YN,  1,					},
	{				  0,	 OT_LABEL,	X_REFLECTIONS  ,0,	0					},
	{                 0,     OT_MULTI,  X_MOON,		MC_YN,  1,				    },
	{                 0,     OT_MULTI,  X_PEOPLE,	MC_YN,  1,					},
	{				  0,     OT_LABEL,  X_TEXTURE_MAP  ,0,  0,                  },
	{                 0,     OT_MULTI,  X_PERSPECTIVE,MC_YN,1,                  },
	{                 0,     OT_MULTI,  X_BILINEAR, MC_YN,  1,                  },
//	{				  0,	OT_SLIDER,	"Gamma",		0,	128,				},
	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_BACK,			},
	{ FE_CONFIG_OPTIONS,	 OT_LABEL,	X_SCANNER,      0,  0					},
	{				  0,	 OT_MULTI,	X_TRACK,MC_SCANNER,	0       			},
	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_BACK,			},
#ifdef WANT_AN_EXIT_MENU_ITEM
	{			FE_QUIT,	 OT_LABEL,	X_ARE_YOU_SURE ,0,	0					},
	{				  0,	OT_BUTTON,	X_OKAY,			0,	FE_NO_REALLY_QUIT	},
	{				  0,	OT_BUTTON,	X_CANCEL,		0,	FE_BACK				},
#endif

	{				 -1,			0,				    0,	0					},
};

CBYTE menu_choice_yesno[20];// = { "no\0yes" };
CBYTE menu_choice_scanner[255];

CBYTE* menu_back_names[] = { "title leaves1.tga", "title rain1.tga", 
	   					     "title snow1.tga", "title blood1.tga" };
CBYTE* menu_map_names[]  = { "map leaves darci.tga", "map rain darci.tga", 
						     "map snow darci.tga", "map blood darci.tga" };
CBYTE* menu_brief_names[]= { "briefing leaves darci.tga", "briefing rain darci.tga", 
						     "briefing snow darci.tga", "briefing blood darci.tga" };
CBYTE* menu_config_names[]= { "config leaves.tga", "config rain.tga", 
						     "config snow.tga", "config blood.tga" };

CBYTE frontend_fonttable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!\":;'#$*-()[]\\/?ÈÀÜÖÄÙÚÓÁÉ";

ULONG FRONTEND_leaf_colours[4] =
	{
		0x665a3a,
		0x486448,
		0x246640,
		0x665E0E
	};

//----------------------------------------------------------------------------
// GLOBALS
//

MenuData  menu_data[50]; // max menu items...
MenuState menu_state;
CBYTE	  menu_buffer[2048];
BOOL	  grabbing_key=0;
BOOL	  grabbing_pad=0;
BOOL	  m_bMovingPanel = FALSE;
Kibble	  kibble[512];
UBYTE	  kibble_off[512];
SLONG	  fade_state=0;
UBYTE	  fade_mode=1;
SLONG	  fade_rgb=0x000000;
SBYTE	  menu_mode_queued=0;
UBYTE	  menu_theme=1;
UBYTE	  menu_thrash=0; // cunningly thrash the stack...
SWORD	  districts[40][3]; // x, y, id
SWORD	  district_count=0;
SWORD	  district_selected=0;
SWORD     district_flash=0;
UBYTE	  district_valid[40];
SWORD	  mission_count=0;
SWORD	  mission_selected=0;
UBYTE	  mission_hierarchy[60];
MissionCache mission_cache[60];
#ifdef	NDEBUG	
UBYTE	  complete_point=0;
#else
UBYTE	  complete_point=0;
#endif
UBYTE	  mission_launch=0;
UBYTE	  previous_mission_launch=0;
BOOL	  cheating=0;
SLONG	  MidX=0, MidY;
float	  ScaleX, ScaleY; // bwahahaha... and lo! the floats creep in! see the extent of my evil powers! muahahaha!  *cough*  er...
#ifdef DEBUG
// Allow saves from init, just so I can test the damn things.
UBYTE	  AllowSave=1;
#else
UBYTE	  AllowSave=0;
#endif
SLONG	  CurrentVidMode=0;
UBYTE	  CurrentBitDepth=16;
UBYTE	  save_slot;
UBYTE	  bonus_this_turn = 0;
UBYTE	  bonus_state = 0;

CBYTE	  the_script_file[MAX_PATH];
#define MISSION_SCRIPT	the_script_file

UBYTE	  IsEnglish=0;

char *pcSpeechLanguageDir = "talk2\\";

DWORD dwAutoPlayFMVTimeout = 0;


#ifdef TARGET_DC
int iNextSuggestedMission = 0;
int g_iLevelNumber;
#endif

// Kludge!
SLONG	  GammaIndex;

// That's not a kludge. THIS is a kludge.
bool m_bGoIntoSaveScreen = FALSE;


BOOL bCanChangeJoypadButtons = FALSE;

LPDIRECTDRAWSURFACE4 screenfull_back = NULL;
LPDIRECTDRAWSURFACE4 screenfull_map = NULL;
LPDIRECTDRAWSURFACE4 screenfull_config = NULL;
LPDIRECTDRAWSURFACE4 screenfull_brief = NULL;

//
// The surface we use to do swipes\fades.
//

LPDIRECTDRAWSURFACE4 screenfull = NULL;


//----------------------------------------------------------------------------
// FUNCTIONS
//

void	FRONTEND_scr_add(LPDIRECTDRAWSURFACE4 *screen, UBYTE *image_data)
{
	DDSURFACEDESC2 back;
	DDSURFACEDESC2 mine;

	// Remember this if we have to reload.

	// Create a mirror surface to the back buffer.

	InitStruct(back);

	the_display.lp_DD_BackSurface->GetSurfaceDesc(&back);


	//
	// Create the mirror surface in system memory.
	//

	InitStruct(mine);

	mine.dwFlags         = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	mine.dwWidth         = back.dwWidth;
	mine.dwHeight        = back.dwHeight;
	mine.ddpfPixelFormat = back.ddpfPixelFormat;
	mine.ddsCaps.dwCaps	 = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	HRESULT result = the_display.lp_DD4->CreateSurface(&mine, screen, NULL);


	if (FAILED(result))
	{
		// Probably out of memory.
		ASSERT ( FALSE );
		*screen = NULL;
		return;
	}

	// Copy the image into the surface...

extern void CopyBackground(UBYTE* image_data, IDirectDrawSurface4* surface);

	CopyBackground(image_data, *screen);

	return;
}

void	FRONTEND_scr_img_load_into_screenfull(CBYTE *name, LPDIRECTDRAWSURFACE4 *screen)
{
	MFFileHandle	image_file;
	SLONG	height;
	CBYTE	fname[200];
	UBYTE*	image;
	UBYTE  *image_data;

	//if (screenfull) FRONTEND_scr_del();

	*screen = NULL;

	sprintf(fname,"%sdata\\%s",DATA_DIR,name);

	image_data =	(UBYTE*)MemAlloc(640*480*3);

	if(image_data)
	{

		image_file	=	FileOpen(fname);
		if(image_file>0)
		{
			TRACE ( "Loading background <%s>\n", fname );
			FileSeek(image_file,SEEK_MODE_BEGINNING,18);
			image	=	image_data +(640*479*3);
			for(height=480;height;height--,image-=(640*3))
			{
				FileRead(image_file,image,640*3);
			}
			FileClose(image_file);
		}
		else
		{
			TRACE ( "Unable to load background <%s>\n", fname );
		}

		FRONTEND_scr_add(screen, image_data);

		MemFree(image_data);
	}
	else
	{
		TRACE ( "No memory to load background <%s>\n", fname );
	}
}

void FRONTEND_scr_unload_theme()
{
	the_display.lp_DD_Background_use_instead = NULL;

	if (screenfull_back  ) {screenfull_back  ->Release(); screenfull_back   = NULL;}
	if (screenfull_map   ) {screenfull_map   ->Release(); screenfull_map    = NULL;}
	if (screenfull_brief ) {screenfull_brief ->Release(); screenfull_brief  = NULL;}
	if (screenfull_config) {screenfull_config->Release(); screenfull_config = NULL;}

	screenfull = NULL;
}


void FRONTEND_scr_new_theme(
		CBYTE *fname_back,
		CBYTE *fname_map,
		CBYTE *fname_brief,
		CBYTE *fname_config)
{
	SLONG last = 1;

	// Stop all music while we load stuff from disk.
	stop_all_fx_and_music();
	
	if (the_display.lp_DD_Background_use_instead == screenfull_back  ) {last = 1;}
	if (the_display.lp_DD_Background_use_instead == screenfull_map   ) {last = 2;}
	if (the_display.lp_DD_Background_use_instead == screenfull_brief ) {last = 3;}
	if (the_display.lp_DD_Background_use_instead == screenfull_config) {last = 4;}

	FRONTEND_scr_unload_theme();

	FRONTEND_scr_img_load_into_screenfull(fname_back  , &screenfull_back  );
	FRONTEND_scr_img_load_into_screenfull(fname_map   , &screenfull_map   );
	FRONTEND_scr_img_load_into_screenfull(fname_brief , &screenfull_brief );
	FRONTEND_scr_img_load_into_screenfull(fname_config, &screenfull_config);

	switch(last)
	{
		case 1: the_display.lp_DD_Background_use_instead = screenfull_back;   break;
		case 2:	the_display.lp_DD_Background_use_instead = screenfull_map;	  break;
		case 3:	the_display.lp_DD_Background_use_instead = screenfull_brief;  break;
		case 4:	the_display.lp_DD_Background_use_instead = screenfull_config; break;
		default:
			ASSERT ( FALSE );
			break;
	}

	// And then restart the music.
	MUSIC_mode(MUSIC_MODE_FRONTEND);
}

void FRONTEND_restore_screenfull_surfaces(void)
{
	FRONTEND_scr_new_theme(
		menu_back_names  [menu_theme],
		menu_map_names   [menu_theme],
		menu_brief_names [menu_theme],
		menu_config_names[menu_theme]);
}

void	FRONTEND_ParseMissionData(CBYTE *text, CBYTE version, MissionData *mdata) {
	UWORD a,n;
	switch(version) {
	case 2:
		sscanf(text,"%d : %d : %d : %d : %d : %s : *%d : %*d : %[^:] : %*s",
			&mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup, 
			&mdata->Type, mdata->fn, mdata->ttl);
		mdata->Flags=0; mdata->District=-1; n=9;
		break;
	case 3:
		sscanf(text,"%d : %d : %d : %d : %d : %d : %s : %[^:] : %*s",
			&mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup, 
			&mdata->Type, &mdata->District, mdata->fn, mdata->ttl);
		mdata->Flags=0; n=8;
		break;
	case 4:
		sscanf(text,"%d : %d : %d : %d : %d : %d : %d : %s : %[^:] : %*s",
			&mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup, 
			&mdata->Type, &mdata->Flags, &mdata->District, mdata->fn, mdata->ttl);
		n=9;
		break;
	default:
		sscanf(text,"%d : %d : %d : %d : %d : %s : %[^:] : %*s", 
			&mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup, 
			&mdata->Type, mdata->fn, mdata->ttl);
		mdata->Flags=0; mdata->District=-1; n=7;
	}
	for (a=0;a<n;a++)
	{
		text=strchr(text,':')+1;
	}
	strcpy(mdata->brief,text+1);

	// Change all @ signs to \r (linefeeds). Are we ugly yet mummy?
	char *pch = mdata->brief;
	while ( *pch != '\0' )
	{
		if ( *pch == '@' )
		{
			*pch = '\r';
		}
		pch++;
	}

	text=mdata->brief+strlen(mdata->brief)-2;
	if (*text==13) *text=0;
}

CBYTE* FRONTEND_LoadString(MFFileHandle &file, CBYTE *txt) {
	CBYTE *ptr=txt;

	*ptr=0;
	while (1) {
		if (FileRead(file,ptr,1)==FILE_READ_ERROR) {
			*ptr=0; return txt;
		};
		if (*ptr==10) break;
		ptr++;
	}
	*(++ptr)=0;
	return txt;
}

void FRONTEND_SaveString(MFFileHandle &file, CBYTE *txt) {
	CBYTE *ptr=txt;
	CBYTE crlf[] = { 13, 10};

	FileWrite(file,txt,strlen(txt));
	FileWrite(file,crlf,2);
}

SLONG FRONTEND_AlterAlpha(SLONG rgb, SWORD add, SBYTE shift) {
	SLONG alpha=rgb>>24;
	alpha<<=shift;
	alpha+=add;
	if (alpha>0xff) alpha=0xff;
	if (alpha<0) alpha=0;
	rgb&=0xffffff;
	return rgb|(alpha<<24);
}


// Recenters (vertically) whatever menu has been put down.
void FRONTEND_recenter_menu ( void )
{
	MenuData *md=menu_data;
	int iY = 0;
	for ( int i = 0; i < menu_state.items; i++ )
	{
		md->Y = iY;
		md++;
		iY += 50;
	}

	menu_state.base = ( 480 - menu_state.items * 50 ) >> 1;
	if ( menu_state.base < 100 )
	{
		menu_state.base = 100;
	}
	menu_state.scroll = 0;
}

ULONG	FRONTEND_fix_rgb(ULONG rgb, BOOL sel)
{
	rgb=fade_rgb;
	if (sel) rgb=FRONTEND_AlterAlpha(rgb,0,1);
	return rgb;
}

//--- drawy stuff ---

#define RandStream(s) ((UWORD)((s = ((s*69069)+1) )>>7))

void	FRONTEND_draw_title(SLONG x, SLONG y, SLONG cutx, CBYTE *str, BOOL wibble, BOOL r_to_l) {
#ifdef TARGET_DC
	SLONG rgb=wibble?0xffffffff:0x70ffffff;
#else
	SLONG rgb=wibble?(fade_rgb<<1)|0xffffff:fade_rgb|0xffffff;
#endif
	SLONG seed=*str;
	SWORD xo=0, yo=0;

	for (;*str;str++) {
		if (!wibble)
		{
#ifdef TARGET_DC
			// Ugh - don't like this wibble.
			xo=4;
			yo=4;
#else
			xo=(RandStream(seed)&0x1f)-0xf;
			yo=4;
#endif
		}

		if (((r_to_l)&&(x>cutx))||((!r_to_l)&&(x<cutx))) {
			MENUFONT_Draw(x+xo,y+yo,BIG_FONT_SCALE+(wibble<<5),str,rgb,0,1);
		}
		x+=(MENUFONT_CharWidth(*str,BIG_FONT_SCALE))-2;
	}

}

void	FRONTEND_init_xition ( void ) {
	MidX=RealDisplayWidth/2;
	MidY=RealDisplayHeight/2;
	ScaleX=MidX/64.0f;
	ScaleY=MidY/64.0f;
	switch (menu_state.mode) {
	case FE_MAPSCREEN:

		screenfull = screenfull_map;

		//FRONTEND_scr_img(menu_map_names[menu_theme]);
		break;
	case FE_MAINMENU:
		screenfull = screenfull_back;

		//FRONTEND_scr_img(menu_map_names[menu_theme]);
		break;
	case FE_LOADSCREEN:
	case FE_SAVESCREEN:
	case FE_CONFIG: 
		screenfull = screenfull_config;

		//FRONTEND_scr_img(menu_config_names[menu_theme]);
		break;
	default:
		if (menu_state.mode>=100)
		{
			screenfull = screenfull_brief;

			//FRONTEND_scr_img(menu_brief_names[menu_theme]);
		}
		else
		{
			screenfull = screenfull_config;
		}
	}
}

LPDIRECTDRAWSURFACE4 lpFRONTEND_show_xition_LastBlit = NULL;

void	FRONTEND_show_xition() {
	RECT rc;

	bool bDoBlit = FALSE;

	if (menu_state.mode>=100)
	{
		rc.top=MidY-(fade_state*ScaleY); rc.bottom=MidY+(fade_state*ScaleY); // set to 3.75 ...
		rc.left=MidX-(fade_state*ScaleX); rc.right=MidX+(fade_state*ScaleX); // set to 5...
		if ( rc.left < rc.right )
		{
			bDoBlit = TRUE;
		}
	}
	else
	{
		rc.top=0; rc.bottom=RealDisplayHeight;
		rc.left=0; 
		if (RealDisplayWidth==640)
		{
			rc.right=fade_state*10;
		}
		else
		{
			rc.right=fade_state*10*RealDisplayWidth/640;
		}
		if ( rc.right > 0 )
		{
			bDoBlit = TRUE;
		}
	}


	if ( bDoBlit )
	{
		the_display.lp_DD_BackSurface->Blt(&rc,screenfull,&rc,DDBLT_WAIT,0);
	}
}

extern UBYTE* image_mem;

void	FRONTEND_stop_xition()
{
	switch(menu_state.mode)
	{
#ifdef WANT_AN_EXIT_MENU_ITEM
		case FE_QUIT:
#endif
		case FE_MAINMENU:
			UseBackSurface(screenfull_back);
			break;
		case FE_CONFIG:
		case FE_CONFIG_VIDEO:
		case FE_CONFIG_AUDIO:
#ifdef WANT_A_KEYBOARD_ITEM
		case FE_CONFIG_INPUT_KB:
#endif
		case FE_CONFIG_INPUT_JP:
		case FE_LOADSCREEN:
		case FE_SAVESCREEN:
			UseBackSurface(screenfull_config);
			break;
		case FE_MAPSCREEN:
			UseBackSurface(screenfull_map);
			break;
		default:
			if (menu_state.mode>=100)
			{
				UseBackSurface(screenfull_brief);
			}
			else
			{
				UseBackSurface(screenfull_config);
			}
			break;
	}
	
	/*

	if (screenfull) {
		ResetBackImage();
		the_display.lp_DD_Background=screenfull;
		image_mem=screenimage;
		screenfull=NULL; screenimage=NULL;
	}

	*/

/*	if (menu_state.mode==FE_MAPSCREEN) InitBackImage("MAP SELECT LEAVES.tga");
	if (menu_state.mode>=100) InitBackImage("MISSION BRIEF LEAVES.tga");
	FRONTEND_scr_del();*/
}


void	FRONTEND_draw_button(SLONG x, SLONG y, UBYTE which, UBYTE flash = FALSE) {
	POLY_Point  pp[4];
	POLY_Point *quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
	float u,v,w,h;
	UBYTE size=(which<4)?64:32;
	UBYTE grow;
	
	if (flash)
	{
		grow = 8;
		
		if (GetTickCount() & 0x200)
		{
			which = 7;
		}
	}
	else
	{
		grow = 0;
	}

	switch (which) {
	case 0: case 4: u=0.0; v=0.0; w=0.5; h=0.5;	break;
	case 1: case 5: u=0.5; v=0.0; w=1.0; h=0.5;	break;
	case 2: case 6: u=0.0; v=0.5; w=0.5; h=1.0;	break;
	case 3: case 7: u=0.5; v=0.5; w=1.0; h=1.0;	break;
	}

	pp[0].colour=(which<4)?0xffFFFFFF:(fade_rgb<<1)|0xffffff; pp[0].specular=0; pp[0].Z=0.5;
	pp[1]=pp[2]=pp[3]=pp[0];

	pp[0].X=x-(grow>>1); pp[0].Y=y-grow; 
	pp[0].u=u; pp[0].v=v;

	pp[1].X=x+(grow>>1)+size; pp[1].Y=y-grow; 
	pp[1].u=w; pp[1].v=v;

	pp[2].X=x; pp[2].Y=y+size; 
	pp[2].u=u; pp[2].v=h;

	pp[3].X=x+size; pp[3].Y=y+size; 
	pp[3].u=w; pp[3].v=h;

	POLY_add_quad(quad,(which<4)?POLY_PAGE_BIG_BUTTONS:POLY_PAGE_TINY_BUTTONS,FALSE,TRUE);
}

#define KIBBLE_Z 0.5

void	FRONTEND_kibble_draw() {
	UWORD c0;
	Kibble*k;
	POLY_Point  pp[4];
	POLY_Point *quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
	SLONG matrix[9],x,y,z;

	ASSERT ( kibble != NULL );

	for (c0=0,k=kibble;c0<512;c0++,k++) 
	  if (k->type>0) {

		pp[0].colour=(k->rgb) | 0xff000000; pp[0].specular=0;
		pp[1]=pp[2]=pp[3]=pp[0];

		FMATRIX_calc(matrix, k->r, k->t, k->p);
	
		x=- k->size; 
		y=- k->size;
		z=0;
		FMATRIX_MUL(matrix,x,y,z);
		pp[0].X=(k->x>>8)+x; pp[0].Y=(k->y>>8)+y; pp[0].Z=KIBBLE_Z;
		pp[0].u=0; pp[0].v=0;

		x=+ k->size; 
		y=- k->size;
		z=0;
		FMATRIX_MUL(matrix,x,y,z);
		pp[1].X=(k->x>>8)+x; pp[1].Y=(k->y>>8)+y; pp[1].Z=KIBBLE_Z;
		pp[1].u=1; pp[1].v=0;

		x=- k->size; 
		y=+ k->size;
		z=0;
		FMATRIX_MUL(matrix,x,y,z);
		pp[2].X=(k->x>>8)+x; pp[2].Y=(k->y>>8)+y; pp[2].Z=KIBBLE_Z;
		pp[2].u=0; pp[2].v=1;

		x=+ k->size; 
		y=+ k->size;
		z=0;
		FMATRIX_MUL(matrix,x,y,z);
		pp[3].X=(k->x>>8)+x; pp[3].Y=(k->y>>8)+y; pp[3].Z=KIBBLE_Z;
		pp[3].u=1; pp[3].v=1;

		POLY_add_quad(quad,k->page,FALSE,TRUE);

	}
}


// Oh yuk this is pants - really could look better.
void	FRONTEND_DrawSlider(MenuData *md) {
	SLONG y;
	ULONG rgb=FRONTEND_fix_rgb(fade_rgb,0);
	y=md->Y+menu_state.base-menu_state.scroll;
	DRAW2D_Box(320,y-2,610,y+2,rgb,0,192);
	DRAW2D_Box(337,y-4,341,y+4,rgb,0,192);
	DRAW2D_Box(337+255,y-4,341+255,y+4,rgb,0,192);
	DRAW2D_Box(337+(md->Data),y-8,341+(md->Data),y+8,rgb,0,192);
} 

void	FRONTEND_DrawMulti(MenuData *md, ULONG rgb) {
	SLONG x,y,dy,c0;
	CBYTE *str;
	//ULONG rgb=FRONTEND_fix_rgb(fade_rgb,0);
	dy=md->Y+menu_state.base-menu_state.scroll;
	str=md->Choices;
	c0=md->Data&0xff;

	if (!str) return;

	while ((*str)&&c0--) {
		str+=strlen(str)+1;
	}
#ifndef TARGET_DC
	if (IsEnglish)
#endif
	{
		MENUFONT_Dimensions(str,x,y,-1,BIG_FONT_SCALE);
		if (320+x>630)
		{
			if (320+(x>>1)>630)
			{
				c0=MENUFONT_CharFit(str,310,128);
				MENUFONT_Draw(320,dy-15,128,str,rgb,0,c0);
				MENUFONT_Draw(320,dy+15,128,str+c0,rgb,0);
			}
			else
			{
				MENUFONT_Draw(620-(x>>1),dy,128,str,rgb,0);
			}
		}
		else
		{
			MENUFONT_Draw(620-x,dy,BIG_FONT_SCALE,str,rgb,0);
		}
	}
#ifndef TARGET_DC
	else
	{
		rgb=FRONTEND_fix_rgb(fade_rgb,1);
		FONT2D_DrawStringRightJustify(str,620,dy,rgb,SMALL_FONT_SCALE + 64,POLY_PAGE_FONT2D);
	}
#endif
}

void	FRONTEND_DrawKey(MenuData *md) {
	SLONG x,y,dy,c0,rgb;
	CBYTE key;
	CBYTE str[25];
	rgb=FRONTEND_fix_rgb(fade_rgb,(grabbing_key&&((menu_data+menu_state.selected==md)&&((GetTickCount()&0x7ff)<0x3ff))));
	dy=md->Y+menu_state.base-menu_state.scroll;
/*	switch (md->Data) {
	case KB_LEFT:
		c0=md->Data &~ 0x80;
		c0<<=16;
		c0|=1<<24;
		GetKeyNameText(c0,str,20);
		break;
//		strcpy(str,"Left");			break;
	case KB_RIGHT:
		strcpy(str,"Right");		break;
	case KB_UP:
		strcpy(str,"Up");			break;
	case KB_DOWN:
		strcpy(str,"Down");			break;
	case KB_ENTER:
		strcpy(str,"Enter");		break;
	case KB_SPACE:
		strcpy(str,"Space");		break;
	case KB_LSHIFT: 
		strcpy(str,"Left Shift");	break;
	case KB_RSHIFT: 
		strcpy(str,"Right Shift");	break;
	case KB_LALT: 
		strcpy(str,"Left Alt");		break;
	case KB_RALT: 
		strcpy(str,"Right Alt");	break;
	case KB_LCONTROL: 
		strcpy(str,"Left Ctrl");	break;
	case KB_RCONTROL:
		strcpy(str,"Right Ctrl");break;
	case KB_TAB:
		strcpy(str,"Tab");			break;
	case KB_END:
		strcpy(str,"End");			break;
	case KB_HOME:
		strcpy(str,"Home");			break;
	case KB_DEL:
		strcpy(str,"Delete");		break;
	case KB_INS:
		strcpy(str,"Insert");		break;
	case KB_PGUP:
		strcpy(str,"Page Up");		break;
	case KB_PGDN:
		strcpy(str,"Page Down");	break;
	case 0:
		strcpy(str,"Unused");		break;
	default:
		//key=InkeyToAscii[md->Data];
		//str[0]=key; str[1]=0;
		GetKeyNameText(md->Data<<16,str,20);
//		if (!stricmp(str,"Circumflex")) strcpy(str,"^");
	}*/

	c0=md->Data;
	if (c0&0x80)
	{
		c0=md->Data &~ 0x80;
		c0<<=16;
		c0|=1<<24;
	} else {
		c0<<=16;
	}
	
	GetKeyNameText(c0,str,25);

	if (IsEnglish)
	{
		MENUFONT_Dimensions(str,x,y,-1,BIG_FONT_SCALE);
		MENUFONT_Draw(620-x,dy,BIG_FONT_SCALE,str,rgb,0);
	} 
	else
	{
		FONT2D_DrawStringRightJustify(str,620,dy,rgb,SMALL_FONT_SCALE,POLY_PAGE_FONT2D);
	}
}

void	FRONTEND_DrawPad(MenuData *md) {
	SLONG x,y,dy,c0,rgb;
	CBYTE str[20];
	rgb=FRONTEND_fix_rgb(fade_rgb,(grabbing_pad&&((menu_data+menu_state.selected==md)&&((GetTickCount()&0x7ff)<0x3ff))));
	dy=md->Y+menu_state.base-menu_state.scroll;
	if (md->Data<31) sprintf(str,"%s %d",XLAT_str(X_BUTTON),md->Data); else strcpy(str,"Unused");
	MENUFONT_Dimensions(str,x,y,-1,BIG_FONT_SCALE);
	MENUFONT_Draw(620-x,dy,BIG_FONT_SCALE,str,rgb,0);
}


//--- kibbly stuff ---

void	FRONTEND_kibble_init_one(Kibble*k, UBYTE type) {
	
	SLONG kibble_index = k - kibble;

	ASSERT ( kibble != NULL );

	ASSERT(WITHIN(kibble_index, 0, 511));

#ifndef FORCE_STUFF_PLEASE_BOB
	if (kibble_off[kibble_index])
	{
		return;
	}
#endif

	if (!(type&128)) {
		if ((menu_state.mode==FE_MAPSCREEN)||(menu_state.mode>=100)) return;
	}
	switch(type&0x7f) {
	case 1:
		k->dx=(20+(Random()&15))<<5; 
		k->dy=0;
		k->page=POLY_PAGE_BIG_LEAF;
		k->rgb=FRONTEND_leaf_colours[Random()&3];
		k->x=(-10-(Random()&0x1ff))<<8; 
		k->y=((Random()%520)-20)<<8;
		k->size=35+(Random()&0x1f);
		k->r=Random()&2047; k->t=Random()&2047; k->p=0;
		k->rd=1+(Random()&7); k->td=1+(Random()&7); k->pd=0;
		k->type=type;
		break;
	case 2:
		k->dx=k->dy=(20+(Random()&15))<<5; 
		k->page=POLY_PAGE_BIG_RAIN;
		k->rgb=0x3f7f7fff;
		if (Random()&1) {
			k->x=(-10-(Random()&0x1ff))<<8; 
			k->y=((Random()%520)-20)<<8;
		} else {
			k->x=((Random()%680)-20)<<8; 
			k->y=(-10-(Random()&0x1ff)-20)<<8;
		}
		k->size=25+(Random()&0x1f);
		k->r=0; k->t=0; k->p=1792;
		k->rd=0; k->td=0; k->pd=0;
		k->type=type;
		break;
	case 3:
		k->dx=(15+(Random()&15))<<5;
		k->dy=(5+(Random()&15))<<5; 
		k->page=POLY_PAGE_SNOWFLAKE;
		k->rgb=0xffafafff;
		if (Random()&1) {
			k->x=(-10-(Random()&0x1ff))<<8; 
			k->y=((Random()%520)-20)<<8;
		} else {
			k->x=((Random()%680)-20)<<8; 
			k->y=(-10-(Random()&0x1ff)-20)<<8;
		}
		k->size=25+(Random()&0x1f);
		k->r=0; k->t=0; k->p=0;
		k->rd=1; k->td=2; k->pd=0;
		k->type=type;
		break;
	}
}

void	FRONTEND_kibble_init() {
	UWORD c0, densities[] = { 25, 255, 40, 10 };
	Kibble*k;

	densities[0] = 25;
	densities[1] = 255;
	densities[2] = 40;
	densities[3] = 10;

	ZeroMemory(kibble,sizeof(kibble));

	for (c0=0,k=kibble;c0<densities[menu_theme];c0++,k++) FRONTEND_kibble_init_one(k,menu_theme+1);
}

void	FRONTEND_kibble_flurry() {
	UWORD n, c0, densities[4];
	Kibble*k;

	ASSERT ( kibble != NULL );

	densities[0] = 125;
	densities[1] = 512;
	densities[2] = 125;
	densities[3] = 10;

	n=densities[menu_theme];

	for (c0=0,k=kibble;c0<n;c0++,k++) 
	  if (!k->type) {
		  switch(menu_theme) {
		  case 0:
			  FRONTEND_kibble_init_one(k,1|128);
			  k->dx=(25+(Random()&15))<<5; 
			  k->x=(-60-(Random()&0xff))<<8; 
			  k->y=(Random()%480)<<8;
			  k->size=5+(Random()&0x1f);
			  k->r=Random()&2047; k->t=Random()&2047;
			  k->rd=1+(Random()&7); k->td=1+(Random()&7);
			  //k->type|=128;
			  break;
		  case 1:
			  FRONTEND_kibble_init_one(k,2|128);
			  //k->type|=128;
			  break;
		  case 2:
			  FRONTEND_kibble_init_one(k,3|128);
			  //k->type|=128;
			  break;
		  }
	}
}

void	FRONTEND_kibble_process() {
	SLONG c0;
	Kibble*k;

	static SLONG last = 0;
	static SLONG now  = 0;

	ASSERT ( kibble != NULL );

	now = GetTickCount();

	if (last < now - 250)
	{
		last = now - 250;
	}

#ifndef FORCE_STUFF_PLEASE_BOB
	if (now > last + 100)
	{
		//
		// Front-end running at less than 10 fps! Turn off a random kibble!
		//

		kibble_off[rand() & 0x1ff] = TRUE;
		kibble_off[rand() & 0x1ff] = TRUE;
		kibble_off[rand() & 0x1ff] = TRUE;
		kibble_off[rand() & 0x1ff] = TRUE;
		kibble_off[rand() & 0x1ff] = TRUE;
		kibble_off[rand() & 0x1ff] = TRUE;
		kibble_off[rand() & 0x1ff] = TRUE;
		kibble_off[rand() & 0x1ff] = TRUE;
	}
	else
	if (now < last + 50)
	{
		//
		// More than 20 fps...
		//

		kibble_off[rand() & 0x1ff] = FALSE;
		kibble_off[rand() & 0x1ff] = FALSE;
		kibble_off[rand() & 0x1ff] = FALSE;
		kibble_off[rand() & 0x1ff] = FALSE;
		kibble_off[rand() & 0x1ff] = FALSE;
		kibble_off[rand() & 0x1ff] = FALSE;
		kibble_off[rand() & 0x1ff] = FALSE;
		kibble_off[rand() & 0x1ff] = FALSE;
	}
#endif

	SLONG i;
	SLONG num_on;

	for (i = 0, num_on = 0; i < 512; i++) {if (!kibble_off[i]) {num_on += 1;}}

	while(last < now)
	{
		//
		// Process at 40 frames a second.
		//

		last += 1000 / 40;

		for (c0=0,k=kibble;c0<512;c0++,k++) 
		  if (k->type>0) {
			  k->x+=k->dx; k->y+=k->dy;
			  k->r+=k->rd; k->t+=k->td;
			  k->r&=2047; k->t&=2047;
			  switch (k->type) {
			  case 1:
				k->dy++;
				k->dx++;
				if ((k->y>>8)>240) k->dy-=Random()%((k->y-240)>>14);
	//			if ((k->x>>8)+k->size<10) FRONTEND_kibble_init_one(k,1);
				if ((k->x>>8)-k->size>650) FRONTEND_kibble_init_one(k,1);
				break;
			  case 129:
	//			if ((k->x>>8)<10) k->type=0;
				if ((k->x>>8)-k->size>650) k->type=0;
			  case 3:
			  case 131:
				{
					SWORD x=k->x>>8, y=k->y>>8;
					k->dx++;
					if ((x>320)&&(x<480)) k->dx-=Random()%((k->x-320)>>14);
					if ((y>240)&&(y<280)) k->dy-=Random()%((k->y-240)>>14);
				}
			  case 2:
			  case 130:
				  if ((((k->x>>8)-k->size)>640)||(((k->y>>8)-k->size)>480)) 
					  if (k->type<128) 
						  FRONTEND_kibble_init_one(k,k->type&127);
					  else
						  k->type=0;
				  break;
			  }
		  }
	}
}

//--- filing stuff ---

void FRONTEND_fetch_title_from_id(CBYTE *script, CBYTE *ttl, UBYTE id) {
	CBYTE *text;
	SLONG ver;
	MissionData *mdata = MFnew<MissionData>();

	*ttl=0;

	text = (CBYTE*)MemAlloc(4096); 
	memset(text,0,4096);

	// Default value = no valid mission.
	// This can happen when saving a game that has been fully completed -
	// no next suggested mission.
	strcpy ( ttl, "Urban Chaos" );


	FileOpenScript();
	while (1) {
		LoadStringScript(text);
		if (*text==0) break;
		if (text[0]=='[') { // we've hit the districts
			break; // ignore them for the moment.
		}
		if ((text[0]=='/')&&(text[1]=='/')) {
			if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
		} else  {
			FRONTEND_ParseMissionData(text,ver,mdata);

			if (mdata->ObjID==id) {
				strcpy(ttl,mdata->ttl);
				break;
			}			
		}
	}
	FileCloseScript();

	MemFree(text);
	MFdelete(mdata);

	// Bin any trailing spaces.
	char *pEnd = ttl + strlen ( ttl ) - 1;
	while ( *pEnd == ' ' )
	{
		*pEnd = '\0';
		pEnd--;
	}
}

UBYTE	best_found[50][4];
void	init_best_found(void)
{
	memset(&best_found[0][0],50*4,0);
}

bool FRONTEND_save_savegame(CBYTE *mission_name, UBYTE slot) {
	CBYTE fn[_MAX_PATH];
	MFFileHandle file;
	UBYTE version=3;

	CreateDirectory("saves",NULL);

	sprintf(fn,"saves\\slot%d.wag",slot);
	file=FileCreate(fn,1);
	FRONTEND_SaveString(file,mission_name);
	FileWrite(file,&complete_point,1);
	FileWrite(file,&version,1);
	FileWrite(file,&the_game.DarciStrength,1);
	FileWrite(file,&the_game.DarciConstitution,1);
	FileWrite(file,&the_game.DarciSkill,1);
	FileWrite(file,&the_game.DarciStamina,1);
	FileWrite(file,&the_game.RoperStrength,1);
	FileWrite(file,&the_game.RoperConstitution,1);
	FileWrite(file,&the_game.RoperSkill,1);
	FileWrite(file,&the_game.RoperStamina,1);
	FileWrite(file,&the_game.DarciDeadCivWarnings,1);
	// mark, if you add stuff again, please remember to inc. the version number
	FileWrite(file,mission_hierarchy,60);

	FileWrite(file,&best_found[0][0],50*4);

	FileClose(file);

	return TRUE;
}

bool FRONTEND_load_savegame(UBYTE slot) {
	CBYTE fn[_MAX_PATH];
	MFFileHandle file;
	UBYTE version=0;

	sprintf(fn,"saves\\slot%d.wag",slot);
	file=FileOpen(fn);
	FRONTEND_LoadString(file,fn);
	FileRead(file,&complete_point,1);
	FileRead(file,&version,1);	// yes, i know, strange place to have version number.
								// it's for Historical Reasons(tm).
	if (version) {
		FileRead(file,&the_game.DarciStrength,1);
		FileRead(file,&the_game.DarciConstitution,1);
		FileRead(file,&the_game.DarciSkill,1);
		FileRead(file,&the_game.DarciStamina,1);
		FileRead(file,&the_game.RoperStrength,1);
		FileRead(file,&the_game.RoperConstitution,1);
		FileRead(file,&the_game.RoperSkill,1);
		FileRead(file,&the_game.RoperStamina,1);		
		FileRead(file,&the_game.DarciDeadCivWarnings,1); 
	}
	if (version>1) 
	{
		FileRead(file,mission_hierarchy,60);
	}
	if (version>2) 
	{
		FileRead(file,&best_found[0][0],50*4);
	}
	FileClose(file);

	return TRUE;
}


void FRONTEND_find_savegames ( bool bGreyOutEmpties=FALSE, bool bCheckSaveSpace=FALSE )
{
	CBYTE dir[_MAX_PATH],ttl[_MAX_PATH];
	WIN32_FIND_DATA data;
	HANDLE handle;
	BOOL   ok;
	SLONG	c0;
	MenuData *md=menu_data;
	CBYTE *str=menu_buffer;
	SLONG x,y,y2=0;
	FILETIME time, high_time={0,0};

	for (c0=1;c0<11;c0++)
	{
		md->Type=OT_BUTTON;
		//md->Data=0;
		md->Data=FE_SAVE_CONFIRM;
		// Not greyed.
		md->Choices = (CBYTE*)0;

		MFFileHandle file;
		sprintf(dir,"saves\\slot%d.wag",c0);
		file = FileOpen(dir);
		GetFileTime(file,NULL,NULL,&time);
		if (file!=FILE_OPEN_ERROR)
		{
			FRONTEND_LoadString(file,ttl);
			FileClose(file);
		}
		else
		{
			strcpy(ttl,XLAT_str(X_EMPTY));
			if ( bGreyOutEmpties )
			{
				// Grey this out then.
				md->Choices = (CBYTE*)1;
			}
		}
		sprintf(dir,"%d: %s",c0,ttl);
		md->Label=str;
		strcpy(str,ttl);
		str+=strlen(str)+1;
		MENUFONT_Dimensions(md->Label,x,y,-1,BIG_FONT_SCALE);
		md->X=320-(x>>1);
		md->Y=y2;
		y2+=50;
		if (CompareFileTime(&time,&high_time)>0)
		{
			high_time = time;
			menu_state.selected=menu_state.items;
		}
		md++;
		menu_state.items++;
	}
}

CBYTE*	FRONTEND_MissionFilename(CBYTE *script, UBYTE i) {
	MFFileHandle file;
	CBYTE *text, *str=menu_buffer;
	SLONG ver;
	MissionData *mdata = MFnew<MissionData>();
	MenuData *md=menu_data;
	SLONG x,y,y2=0;

	*str=0;

	text = (CBYTE*)MemAlloc(4096); 
	memset(text,0,4096);

	i++;

	FileOpenScript();
	while (1) {
		LoadStringScript(text);
		if (*text==0) break;
		if (text[0]=='[') { // we've hit the districts
			break; // ignore them for the moment.
		}
		if ((text[0]=='/')&&(text[1]=='/')) {
			if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
		} else  {
			FRONTEND_ParseMissionData(text,ver,mdata);

			if (mdata->District==districts[district_selected][2])
			{
				//
				// Only missions available to play are put in the
				// list nowadays...
				//

				if (mission_hierarchy[mdata->ObjID] & 4)
				{
					i--;
				}
			}
			if (!i) {
				strcpy(str,mdata->fn);
				mission_launch=mdata->ObjID;
				break;
			}
			
		}
	}
	FileCloseScript();

	MemFree(text);
	MFdelete(mdata);

	return str;
}

void	FRONTEND_MissionHierarchy(CBYTE *script) {
	MFFileHandle file;
	SLONG best_score;
	CBYTE *text;
	SLONG ver;
	MissionData *mdata = MFnew<MissionData>();
	MenuData *md=menu_data;
	UBYTE i=0, j, flag;
	UBYTE newtheme;

	bonus_this_turn = 0;

	// this is always called whenever the complete_point changes, soooo...
	newtheme=3;
	if (complete_point<24) newtheme=2;
	if (complete_point<16) newtheme=1;
	if (complete_point<8) newtheme=0;
	if (newtheme!=menu_theme) {

		menu_theme = newtheme;
 
		FRONTEND_scr_new_theme(
			menu_back_names  [menu_theme],
			menu_map_names   [menu_theme],
			menu_brief_names [menu_theme],
			menu_config_names[menu_theme]);

		switch(menu_state.mode) {
		case FE_MAINMENU:
			//InitBackImage(menu_back_names[menu_theme]);

			UseBackSurface(screenfull_back);

			break;
		case FE_CONFIG:
		case FE_CONFIG_VIDEO:
		case FE_CONFIG_AUDIO:
		case FE_CONFIG_OPTIONS:
#ifdef WANT_A_KEYBOARD_ITEM
		case FE_CONFIG_INPUT_KB:
#endif
		case FE_CONFIG_INPUT_JP:
		case FE_LOADSCREEN:
		case FE_SAVESCREEN:
			//InitBackImage(menu_config_names[menu_theme]);
			UseBackSurface(screenfull_config);
			break;
		case FE_MAPSCREEN:
			//InitBackImage(menu_map_names[menu_theme]);
			UseBackSurface(screenfull_map);
			break;
		default:
			UseBackSurface(screenfull_brief);
			//InitBackImage(menu_brief_names[menu_theme]);
		}
		FRONTEND_kibble_init();
	}
	
	text = (CBYTE*)MemAlloc(4096); 
	memset(text,0,4096);

//	ZeroMemory(mission_hierarchy,sizeof(mission_hierarchy));
	ZeroMemory(district_valid,sizeof(district_valid));
	mission_hierarchy[1]=3; // the root; 1 - exists, 2 - complete, 4 - waiting

#ifdef ANNOYING_HACK_FOR_SIMON

	// this hack does an end run around the hierarchy system
	// it forces fight1, assault1, & testdrive1a (the bronze fighting, driving test and 
	// assault course) to be complete before allowing police1 to open up

	SLONG fightID = -1, assaultID = -1, testdriveID = -1, policeID = -1, fight2ID = -1, testdrive3ID = -1;
	SLONG bonusID1 = -1, bonusID2 = -1, bonusID3 = -1;
	SLONG secretIDbreakout = -1, estateID = -1;

	FileOpenScript();
	while (1) {
		LoadStringScript(text);
		if (*text==0) break;
		if (text[0]=='[') break; 
		if ((text[0]=='/')&&(text[1]=='/')) {
			if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
		} else
		{
			FRONTEND_ParseMissionData(text,ver,mdata);

			_strlwr(mdata->fn);

			if (strstr(mdata->fn,"ftutor1.ucm"))
			{
				fightID=mdata->ObjID;
			}

			if (strstr(mdata->fn,"assault1.ucm"))
			{
				assaultID=mdata->ObjID;
			}

			if (strstr(mdata->fn,"testdrive1a.ucm"))
			{
				testdriveID=mdata->ObjID;
			}

			if (strstr(mdata->fn,"police1.ucm"))
			{
				policeID=mdata->ObjID;
			}

			if (strstr(mdata->fn,"fight2.ucm"))
			{
				fight2ID=mdata->ObjID;
			}

			if (strstr(mdata->fn,"testdrive3.ucm"))
			{
				testdrive3ID=mdata->ObjID;
			}

			if (strstr(mdata->fn,"gangorder1.ucm"))
			{
				bonusID1=mdata->ObjID;
			}
			if (strstr(mdata->fn,"gangorder2.ucm"))
			{
				bonusID2=mdata->ObjID;
			}
			if (strstr(mdata->fn,"bankbomb1.ucm"))
			{
				bonusID3=mdata->ObjID;
			}

			if (strstr(mdata->fn,"estate2.ucm"))
			{
				estateID=mdata->ObjID;
			}
		}
	}
	FileCloseScript();

	ASSERT(WITHIN(fightID     , 0, 39));
	ASSERT(WITHIN(fight2ID    , 0, 39));
	ASSERT(WITHIN(assaultID   , 0, 39));
	ASSERT(WITHIN(testdriveID , 0, 39));
	ASSERT(WITHIN(testdrive3ID, 0, 39));
	ASSERT(WITHIN(policeID    , 0, 39));
	ASSERT(WITHIN(bonusID1	  , 0, 39));
	ASSERT(WITHIN(bonusID2	  , 0, 39));
	ASSERT(WITHIN(bonusID3	  , 0, 39));
	ASSERT(WITHIN(estateID	  , 0, 39));

#endif

#if 0
	best_score        = -INFINITY;
#else
	best_score        = 1000;
#endif
	district_flash    =  -1;
	district_selected =  0;
#ifdef TARGET_DC
	iNextSuggestedMission = -1;
#endif

	FileOpenScript();
	while (1) {
		LoadStringScript(text);
		if (*text==0) break;
		if (text[0]=='[') { // we've hit the districts
			break; // ignore them for the moment.
		}
		if ((text[0]=='/')&&(text[1]=='/')) {
			if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
		} else  {
			FRONTEND_ParseMissionData(text,ver,mdata);

			flag=mission_hierarchy[mdata->ObjID]|1; // exists

//			this dodgy method is no longer used, cos it sucks.		
//			if (mdata->ObjID<=complete_point) flag|=2; // complete
//			instead, the entire hierarchy is preserved in savegames and
//			completing a mission sets the appropriate flag. ie, the proper way.			


#ifdef TARGET_DC
			// If Estate of Emergency has been done, and the secret environment var is set, then
			// expose Breakout to the world....
			// Also, you have to be playing in English. Sorry.
			if ( IsEnglish && ( ( mission_hierarchy[estateID] & 2 ) != 0 ) && ENV_get_value_number ( "cheat_driving_bronze", 0, "" ) )
			{
				// Hurrah!
				mission_hierarchy[secretIDbreakout] |= 4;
			}
			else
			{
				// Nope - not yet.
				mission_hierarchy[secretIDbreakout] = 0;
			}
#endif

#ifndef VERSION_DEMO
			if (mission_hierarchy[mdata->ParentID]&2) 
#endif
			{
#ifndef VERSION_DEMO
				if (mdata->ObjID == fight2ID && menu_theme < 1)
				{
					//
					// Ignore this mission until the first theme...
					//
				}
				else
				if (mdata->ObjID == testdrive3ID && menu_theme < 2)
				{
					//
					// Ignore this mission until the last theme...
					//
				}
				else
#endif
				{
					for (j=0;j<40;j++) {
						if (districts[j][2]==mdata->District) {
							district_valid[j]|=1;
							if ( !(mission_hierarchy[mdata->ObjID]&2) &&
								 (mission_hierarchy[mdata->ObjID]&4) )
							{
								district_valid[j]|=2; // highlight zones with ready missions
							}

							/*

							if ((!district_valid[district_selected])||(mdata->ObjID==complete_point+1))
							{
								district_flash=district_selected=j;

								// there's a good chance this is the mission we wanna attempt now
							}

							*/

							break;
						}
					}

					flag|=4; // available
				}
			}

#ifdef ANNOYING_HACK_FOR_SIMON
			if (mdata->ObjID==policeID) {
				if (  (mission_hierarchy[fightID]&2)
					&&(mission_hierarchy[assaultID]&2)
					&&(mission_hierarchy[testdriveID]&2))
					flag|=4;
				else
				{
					flag&=~4;

					//
					// Search for the district for this mission.
					//

					for (j = 0; j < 40; j++)
					{
						if (districts[j][2] == mdata->District)
						{
							district_valid[j] = 0;
						}
					}
				}
			}

#endif

			if (complete_point>=40) // bodge!
				flag|=2;

			mission_hierarchy[mdata->ObjID]=flag;

			if ((flag & 4) && !(flag & 2))	// 4 means available and 2 means complete
			{
				//
				// This mission is active. Where abouts is it in the suggest_order[].
				// The later it is in the suggest order, the later the mission is.
				//
#ifndef VERSION_DEMO
				if ((mdata->ObjID==bonusID1)&&!(bonus_state & 1))
				{
					bonus_state|=1;
					bonus_this_turn = 1;
				}
				if ((mdata->ObjID==bonusID2)&&!(bonus_state & 2))
				{
					bonus_state|=2;
					bonus_this_turn = 1;
				}
				if ((mdata->ObjID==bonusID3)&&!(bonus_state & 4))
				{
					bonus_state|=4;
					bonus_this_turn = 1;
				}
#endif
				SLONG order = 0;

				while(1)
				{
					if (stricmp(mdata->fn, suggest_order[order]) == 0)
					{
						//
						// Found the mission!
						//

#if 0
						// No, this is selecting them in the wrong order.
						if (order > best_score)
#else
						if (order < best_score)
#endif
						{
							best_score = order;
#ifdef TARGET_DC
							// For the savegame.
							iNextSuggestedMission = mdata->ObjID;
#endif

							//
							// Find which district has this id.
							//

							for (j = 0; j < 40; j++)
							{
								if (districts[j][2] == mdata->District)
								{
									district_flash    = j;
									district_selected = j;

									goto found_mission;
								}
							}
						}
					}

					order += 1;

					if (suggest_order[order][0] == '!')
					{
						break;
					}

				  found_mission:;
				}
			}
		}
	}
	FileCloseScript();

#if 1
	if ( !IsEnglish )
	{
		// Never show breakout in French.
		mission_hierarchy[secretIDbreakout] = 0;
	}
#else

	// If Estate of Emergency has been done, and the secret environment var is set, then
	// expose Breakout to the world....
	// Also, you have to be playing in English. Sorry.
	if ( IsEnglish && ( ( mission_hierarchy[estateID] & 2 ) != 0 ) && ENV_get_value_number ( "cheat_driving_bronze", 0, "" ) )
	{
		// Hurrah!
		mission_hierarchy[secretIDbreakout] |= 4;
	}
	else
	{
		// Nope - not yet.
		mission_hierarchy[secretIDbreakout] = 0;
	}

#endif

	MemFree(text);
	MFdelete(mdata);
}

CBYTE	*brief_wav[]=
{
	"none", //0
	"none", //1
	"none", //2
	"none", //3
	"none", //4
	"none", //5
	"policem1.wav",  //6
	"policem.wav",   //7
	"policem.wav",   //8
	"policem2.wav",   //9
	"none", //10
	"none", //11
	"policem3.wav", //12
	"none", //13
	"policem4.wav", //14
	"policem5.wav", //15
	"policem6.wav",  //16
	"policem7.wav",   //17
	"policem8.wav",   //18
	"policem9.wav",   //19
	"policem10.wav", //20
	"policem11.wav", //21
	"policem12.wav", //22
	"policem13.wav", //23
	"policem14.wav", //24
	"policem15.wav", //25
	"policem16.wav",  //26
	"policem17.wav",   //27
	"policem18.wav",   //28
	"roperm19.wav",   //29
	"roperm20.wav",   //30
	"roperm21.wav",   //31
//	"roperm22.wav",   //32
	"roperm23.wav",   //33
	"roperm24.wav",   //34
	""

};
void	FRONTEND_MissionBrief(CBYTE *script, UBYTE i) {
	MFFileHandle file;
	CBYTE *text, *str=menu_buffer;
	SLONG ver;
	MissionData *mdata = MFnew<MissionData>();
	MenuData *md=menu_data;
	SLONG x,y,y2=0;

	*str=0;
	i++;

	text = (CBYTE*)MemAlloc(4096); 
	memset(text,0,4096);

	FileOpenScript();
	while (1) {
		LoadStringScript(text);
		if (*text==0) break;
		if (text[0]=='[') { // we've hit the districts
			break; // ignore them for the moment.
		}
		if ((text[0]=='/')&&(text[1]=='/')) {
			if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
		} else  {
			FRONTEND_ParseMissionData(text,ver,mdata);

			if (mdata->District==districts[district_selected][2])
			{
				//
				// Only missions available to play are put in the
				// list nowadays...
				//

				if (mission_hierarchy[mdata->ObjID] & 4)
				{
					i--;
				}
			}

			if (!i) {
				strcpy(str,mdata->brief);
				str+=strlen(str)+1;
				strcpy(str,mdata->ttl);
				menu_state.title=str;
				break;
			}
			
		}
	}
	FileCloseScript();

	MemFree(text);

	if ( ( mdata->ObjID ) && ( mdata->ObjID<34 ) &&
		 ( 0 != strcmp ( brief_wav[mdata->ObjID], "none" ) ) )
	{
		CBYTE path[_MAX_PATH];
		//MFX_QUICK_wait();
		strcpy(path,GetSpeechPath());

#ifdef TARGET_DC
		strcat(path,pcSpeechLanguageDir);
#else
		// On PC it's always talk2.
		strcat ( path, "talk2\\" );
#endif
		strcat(path,brief_wav[mdata->ObjID]);

		MFX_QUICK_play(path);
	}

	MFdelete(mdata);
}


void	FRONTEND_MissionList(CBYTE *script, UBYTE district) {
/*	MFFileHandle file;
	CBYTE *text, *str=menu_buffer;
	SLONG ver;
	MissionData *mdata = MFnew<MissionData>();
//	MenuData *md=menu_data;
//	SLONG x,y,y2=0;
//	UBYTE i=100;

	//district--;

	text = (CBYTE*)MemAlloc(4096); 
	memset(text,0,4096);
	mission_count=mission_selected=0;
	file = FileOpen(script);
	while (1) {
		FRONTEND_LoadString(file,text);
		if (*text==0) break;
		if (text[0]=='[') { // we've hit the districts
			break; // ignore them for the moment.
		}
		if ((text[0]=='/')&&(text[1]=='/')) {
			if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
		} else  {
			FRONTEND_ParseMissionData(text,ver,mdata);
			if (mdata->District==district) {
				*str=mdata->ObjID; str++;
				strcpy(str,mdata->ttl);
				str+=strlen(mdata->ttl)+1;
				mission_count++;
			}
			
		}
	}
	FileClose(file);

	MemFree(text);
	MFdelete(mdata);
*/
	UBYTE i=0;
	CBYTE *str=menu_buffer;

	mission_count=mission_selected=0;

	while (*mission_cache[i].name) 
	{
		if (mission_cache[i].district == district)
		{
			if (mission_hierarchy[mission_cache[i].id] & 4)
			{
				//
				// This mission is available to play.
				//

				mission_selected = mission_count;

				//
				// First byte is the mission ID
				//

				*str++ = mission_cache[i].id;

				//
				// Then comes the mission name.
				//

				strcpy(str,mission_cache[i].name);

				str+=strlen(mission_cache[i].name)+1;

				mission_count++;
			}
		}

		i++;
	}

	/*

	//
	// Default to selecting the last available mission.
	//

	mission_selected = mission_count - 1;

	if (mission_selected < 0)
	{
		mission_selected = 0;
	}

	*/
}

void	FRONTEND_CacheMissionList(CBYTE *script) {
	MFFileHandle file;
	CBYTE *text, *str;
	SLONG ver;
	MissionData *mdata = MFnew<MissionData>();
	UBYTE i=0;

	//district--;

	text = (CBYTE*)MemAlloc(4096); 
	memset(text,0,4096);
	FileOpenScript();
	while (1) {
		LoadStringScript(text);
		if (*text==0) break;
		if (text[0]=='[') { // we've hit the districts
			break; // ignore them for the moment.
		}
		if ((text[0]=='/')&&(text[1]=='/')) {
			if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
		} else  {
			FRONTEND_ParseMissionData(text,ver,mdata);
			mission_cache[i].district=mdata->District;
			mission_cache[i].id=mdata->ObjID;
			strcpy(mission_cache[i].name,mdata->ttl);
			i++;
		}
	}
	FileCloseScript();

	MemFree(text);
	MFdelete(mdata);

	//*mission_cache[i].name=0;

}

void	FRONTEND_districts(CBYTE *script) {
	MFFileHandle file;
	CBYTE *text, *str=menu_buffer;
	SLONG ver, mapx=0, mapy=0;
	MenuData *md=menu_data;
	SLONG x,y;
	UBYTE i=0,ct,index=0;
	SWORD temp_dist[40][3];
	UBYTE crap_remap[640][10];

	text = (CBYTE*)MemAlloc(4096); 
	memset(text,0,4096);

	district_count=0;
	//district_selected=0;
	ZeroMemory(crap_remap,sizeof(crap_remap));

	FileOpenScript();
	while (1) {
		LoadStringScript(text);
		if (*text==0) break;
		if (text[0]=='[') { // we've hit the districts
			i=1;
		} else {
			if (i) {
			  ct=sscanf(text,"%[^=]=%d,%d",str,&mapx,&mapy);
			  if (strlen(str)&&mapx&&mapy&&(ct==3)) {
				  temp_dist[district_count][0]=mapx-16;
				  temp_dist[district_count][1]=mapy-32;
				  temp_dist[district_count][2]=index;
				  crap_remap[mapx][0]++;
				  crap_remap[mapx][ crap_remap[mapx][0] ]=district_count;
				  //districts[district_count][0]=mapx;
				  //districts[district_count][1]=mapy;
				  district_count++;
			  }
			  index++;
			} else
				if ((text[0]=='/')&&(text[1]=='/')) {
					if (strstr(text,"Version")) sscanf(text,"%*s Version %d",&ver);
				} 

		} 
	}
	FileCloseScript();

	if (district_count) {
		i=0;
		for (x=0;x<640;x++)
			for (y=1;y<=crap_remap[x][0];y++) {
				districts[i][0]=temp_dist[crap_remap[x][y]][0];
				districts[i][1]=temp_dist[crap_remap[x][y]][1];
				//districts[i][2]=crap_remap[x][y];
				districts[i][2]=temp_dist[crap_remap[x][y]][2];
				i++;
			}
		FRONTEND_MissionList(script,districts[district_selected][2]);
	}
		  

	MemFree(text);

}

CBYTE*	FRONTEND_gettitle(UBYTE mode, UBYTE selection) {
	RawMenuData *pt=raw_menu_data;
	while (pt->Menu!=mode) pt++;
	for (;selection;selection--,pt++);
	return XLAT_str_ptr(pt->Label);
}

void	FRONTEND_easy(UBYTE mode) {
	SLONG x,y,y2=0;
	RawMenuData *pt=raw_menu_data;
	MenuData *md=menu_data+menu_state.items;

	if (menu_state.items) y2=(md-1)->Y+50;


	int iBigFontScale = BIG_FONT_SCALE;
	if ( !IsEnglish )
	{
		if ( ( menu_state.mode == FE_SAVESCREEN ) ||
			 ( menu_state.mode == FE_LOADSCREEN ) ||
			 ( menu_state.mode == FE_SAVE_CONFIRM ) )
		{
			// Reduce it a bit to fit long French words on screen.
			iBigFontScale = BIG_FONT_SCALE_FRENCH;
		}
	}
	
	while (pt->Menu!=mode) pt++;
	while ((pt->Menu==mode)||!pt->Menu) {
		md->Type=pt->Type;
		//XLAT_str(pt->Label,md->Label);
		md->LabelID=pt->Label;
		md->Label=XLAT_str_ptr(pt->Label);
		md->Data=pt->Data;
		switch (md->Type) {
		case OT_BUTTON:
			md->Choices=pt->Choices; // secret code... :P
			// Fallthrough.
		case OT_LABEL:
			MENUFONT_Dimensions(md->Label,x,y,-1,iBigFontScale);
			md->X=320-(x>>1);
			break;
		case OT_MULTI:
			md->X=30;
			if (pt->Choices==MC_YN) {
				md->Choices=menu_choice_yesno;
				md->Data|=(2<<8);
			}
			else if (pt->Choices==MC_SCANNER) {
				md->Choices=menu_choice_scanner;
				md->Data|=(2<<8);
			}
			break;
		default:
			md->X=30;
		}
		md->Y=y2;
		y2+=50;
		md++;
		pt++;
		menu_state.items++;
	}
	for (md=menu_data;md->Type==OT_LABEL;md++,menu_state.selected++);
#if 0
	// Move to FRONTEND_recenter_menu.
	menu_state.base=240-(menu_state.items*25);
	if (menu_state.base<50) menu_state.base=50;
#endif

	FRONTEND_recenter_menu();
}

//----------------------------------------------------------------------------
// STORE/RESTORE OPTIONS DATA
//

// Video

UBYTE LabelToIndex(SLONG label)
{
	switch(label) {
		case X_STARS:		return 0;
		case X_SHADOWS:		return 1;
		case X_PUDDLES:		return 4;
		case X_DIRT:		return 5;
		case X_MIST:		return 6;
		case X_RAIN:		return 7;
		case X_SKYLINE:		return 8;
		case X_CRINKLES:	return 11;
		case X_MOON:		return 2;
		case X_PEOPLE:		return 3;
		case X_BILINEAR:	return 9;
		case X_PERSPECTIVE:	return 10;
	}
	return 19;
}

void FRONTEND_restore_video_data()
{
	int data[20]; // int for compatability :P
	UBYTE i,j;
#ifdef TARGET_DC
	AENG_get_detail_levels( /*data,*/ data+1, /*data+2, data+3,*/ data+4, data+5, data+6, data+7, data+8, /*data+9, data+10,*/ data+11);
#else
	AENG_get_detail_levels( data, data+1, data+2, data+3, data+4, data+5, data+6, data+7, data+8, data+9, data+10, data+11);
#endif
	for (i=0;i<menu_state.items;i++)
	switch(menu_data[i].LabelID)
	{
/*		case X_RESOLUTION:
			CurrentVidMode=menu_data[i].Data&0xff;
			ShellPauseOn();
			switch(CurrentVidMode)
			{
			case 0:
				SetDisplay(640,480,16);
				break;
			case 1:
				SetDisplay(800,600,16);
				break;
			case 2:
				SetDisplay(1024,768,16);
				break;
			}
			ShellPauseOff();
			break;
		case X_DRIVERS:
			// er...
			break;*/
		case X_STARS:
		case X_SHADOWS:
		case X_PUDDLES:
		case X_DIRT:
		case X_MIST:
		case X_RAIN:
		case X_SKYLINE:
		case X_CRINKLES:
		case X_MOON:
		case X_PEOPLE:
		case X_PERSPECTIVE:
		case X_BILINEAR:
			j=LabelToIndex(menu_data[i].LabelID);
			data[j]|=menu_data[i].Data&0xff00;
			menu_data[i].Data=data[j];
			break;
	}

}

void FRONTEND_store_video_data()
{
	int data[20], i, mode, bit_depth; 

	// Get the current ones.
#ifdef TARGET_DC
	AENG_get_detail_levels( /*data,*/ data+1, /*data+2, data+3,*/ data+4, data+5, data+6, data+7, data+8, /*data+9, data+10,*/ data+11);
#else
	AENG_get_detail_levels( data, data+1, data+2, data+3, data+4, data+5, data+6, data+7, data+8, data+9, data+10, data+11);
#endif

	// Override with menu entries:
	for (i=0;i<menu_state.items;i++)
	switch(menu_data[i].LabelID)
	{
		case X_RESOLUTION:
			mode=menu_data[i].Data&0xff;
			break;
		case X_COLOUR_DEPTH:
			bit_depth=menu_data[i].Data&0xff ? 32 : 16;
			break;
		case X_DRIVERS:
			// er...
			break;
		case X_STARS:
		case X_SHADOWS:
		case X_PUDDLES:
		case X_DIRT:
		case X_MIST:
		case X_RAIN:
		case X_SKYLINE:
		case X_CRINKLES:
		case X_MOON:
		case X_PEOPLE:
		case X_PERSPECTIVE:
		case X_BILINEAR:
			data[LabelToIndex(menu_data[i].LabelID)]=menu_data[i].Data&0xff;
			break;
		case X_LOW:

			break;
	}
#ifndef TARGET_DC
	ENV_set_value_number("Mode",mode,"Video");
	ENV_set_value_number("BitDepth",bit_depth,"Video");
#endif
#ifdef TARGET_DC
	AENG_set_detail_levels(/*data[0],*/data[1],/*data[2],data[3],*/data[4],data[5],data[6],data[7],data[8],/*data[9],data[10],*/data[11] );
#else
	AENG_set_detail_levels(data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11] );
#endif

#ifdef ALLOW_DANGEROUS_OPTIONS

	if ((mode!=CurrentVidMode)||(bit_depth!=CurrentBitDepth))
	{
		CurrentVidMode=mode;
		CurrentBitDepth=bit_depth;
		ShellPauseOn();
		switch(CurrentVidMode)
		{
		case 0:
			SetDisplay(640,480,bit_depth);
			break;
		case 1:
			SetDisplay(800,600,bit_depth);
			break;
		case 2:
			SetDisplay(1024,768,bit_depth);
			break;
		case 3:
			SetDisplay(320,240,bit_depth);
			break;
		case 4:
			SetDisplay(512,384,bit_depth);
			break;
		}
		ShellPauseOff();
	}

#endif

}

#ifndef TARGET_DC

void	FRONTEND_do_drivers() {
	SLONG			result, count=0, selected=0;
	ChangeDDInfo	*change_info;
	DDDriverInfo	*current_driver=0,
					*driver_list;
	GUID			*DD_guid;
	TCHAR			szBuff[80];
	CBYTE			*str=menu_buffer, *str_tmp;

	switch (RealDisplayWidth)
	{
	case 640:		
		CurrentVidMode=0;
		break;
	case 800:
		CurrentVidMode=1;
		break;
	case 1024:
		CurrentVidMode=2;
		break;
	case 320:		
		CurrentVidMode=3;
		break;
	case 512:		
		CurrentVidMode=4;
		break;

	default:
		CurrentVidMode=0;
		break;
	}
	CurrentBitDepth=DisplayBPP;

	strcpy(str,"640x480");
	str+=strlen(str)+1;
	strcpy(str,"800x600");
	str+=strlen(str)+1;
	strcpy(str,"1024x768");
	str+=strlen(str)+1;
	strcpy(str,"320x240");
	str+=strlen(str)+1;
	strcpy(str,"512x384");
	str+=strlen(str)+1;
	str_tmp=str;
	menu_data[1].Data=CurrentVidMode|(5<<8);
	menu_data[1].Choices=menu_buffer;

	strcpy(str,"16 bit");
	str+=strlen(str)+1;
	strcpy(str,"32 bit");
	str+=strlen(str)+1;
	menu_data[3].Data=((UBYTE)(CurrentBitDepth==32))|(2<<8);
	menu_data[3].Choices=str_tmp;
	str_tmp=str;

	selected=0;

	current_driver=the_manager.CurrDriver;

	// Dump Driver list to Combo Box
	driver_list	=	the_manager.DriverList;
	while(driver_list)
	{
		if(driver_list->IsPrimary())
			wsprintf(szBuff,TEXT("%s (Primary)"),driver_list->szName);
		else
			wsprintf(szBuff,TEXT("%s"),driver_list->szName);

		// Add String to multi-choice
		strcpy(str,szBuff);
		str+=strlen(str)+1;

/*		// Set up pointer to driver for this item
		SendDlgItemMessage(hDlg, IDC_DRIVERS, CB_SETITEMDATA, (WPARAM)result, (LPARAM)(void *)driver_list);*/

		// Is it the current Driver
		if(current_driver==driver_list)
			selected=count;

		count++;
		driver_list	=	driver_list->Next;
	}
	menu_data[2].Data=selected|(count<<8);
	menu_data[2].Choices=str_tmp;

/*	UBYTE   c0;
	UWORD	ct;
	CBYTE  *str=menu_buffer;

	ct=Get3DProviderList(&prov);
	menu_data[3].Data=Get3DProvider()|(ct<<8);
	for (c0=0;c0<ct;c0++) {
		strcpy(str,prov->name);
		str+=strlen(str)+1;
		prov++;
	}
	*str=0;
	menu_data[3].Choices=menu_buffer;*/
}

void	FRONTEND_gamma_update() {
/*	if ((menu_state.selected==11)||(menu_state.selected==12)) {
		the_display.SetGamma(menu_data[11].Data, menu_data[12].Data);
	}*/
	if (menu_state.selected==GammaIndex)
		the_display.SetGamma(menu_data[GammaIndex].Data&0xff, 256);
}

#else //#ifndef TARGET_DC

// Spoof them.
void	FRONTEND_do_drivers() {
}

void	FRONTEND_gamma_update() {
}


#endif //#else //#ifndef TARGET_DC

void	FRONTEND_do_gamma() {
	SLONG x,y,y2=0;
	MenuData keepsafe;
	MenuData *md=menu_data+menu_state.items-1;

	keepsafe=*md; // we're going to insert the extra items before the last ("go back") item

	md->Label=XLAT_str_ptr(X_GAMMA);
	MENUFONT_Dimensions(md->Label,x,y,-1,BIG_FONT_SCALE);
	md->Type=OT_LABEL;
	md->X=320-(x>>1);
	y2=md->Y; md++; y2+=50;

	md->Label=XLAT_str_ptr(X_LOW);
	md->LabelID=X_LOW;
	GammaIndex=md-menu_data;
	MENUFONT_Dimensions(md->Label,x,y,-1,BIG_FONT_SCALE);
	md->Type=OT_SLIDER;
	md->X=30;
	md->Y=y2; md++; y2+=50;

/*	md->Label=XLAT_str_ptr(X_HIGH);
	MENUFONT_Dimensions(md->Label,x,y);
	md->Type=OT_SLIDER;
	md->X=30;
	md->Y=y2; md++; y2+=50;*/

	*md=keepsafe;
	md->Y=y2;

	md++; y2+=50;

	menu_state.items+=2; //3;

	int a,b;
	the_display.GetGamma(&a,&b);
	menu_data[GammaIndex].Data=a;
//	menu_data[12].Data=b;

}

void	FRONTEND_mode(SBYTE mode, bool bDoTransition=TRUE) {
	// Reset this now.
	dwAutoPlayFMVTimeout = timeGetTime() + AUTOPLAY_FMV_DELAY;

	SBYTE last=menu_state.mode;
	fade_mode=1;
	ZeroMemory(menu_data,sizeof(menu_data));
	menu_state.items=0;
	if (mode==-2) {
		if (menu_state.stackpos>0) {
			menu_state.stackpos--;
			mode=menu_state.stack[menu_state.stackpos].mode;
			menu_state.scroll=menu_state.stack[menu_state.stackpos].scroll;
			menu_state.selected=menu_state.stack[menu_state.stackpos].selected;
			menu_state.title=menu_state.stack[menu_state.stackpos].title;
		} else {
			mode=menu_state.mode;
		}
	} else {
		if (menu_thrash) {
				menu_state.stack[menu_state.stackpos].mode=menu_thrash;
				menu_state.stack[menu_state.stackpos].scroll=0;
				menu_state.stack[menu_state.stackpos].selected=0;
				menu_state.stack[menu_state.stackpos].title=0;
				menu_state.stackpos++;
				menu_thrash=0;
		} else 
			if (menu_state.mode!=-1) {
				menu_state.stack[menu_state.stackpos].mode=menu_state.mode;
				menu_state.stack[menu_state.stackpos].scroll=menu_state.scroll;
				menu_state.stack[menu_state.stackpos].selected=menu_state.selected;
				menu_state.stack[menu_state.stackpos].title=menu_state.title;
				menu_state.stackpos++;
			}
		menu_state.selected=0;
	}

	switch ( mode )
	{
	case FE_MAPSCREEN:
		menu_state.title=XLAT_str_ptr(X_START);
		break;
	case FE_MAINMENU:
		// No title.
		menu_state.title=NULL;
		break;
	case FE_LOADSCREEN:
		menu_state.title=XLAT_str_ptr(X_LOAD_GAME);
		break;
	case FE_SAVESCREEN:
		menu_state.title=XLAT_str_ptr(X_SAVE_GAME);
		break;
	default:
		if ( menu_state.stackpos && ( mode < 100 ) )
		{
			menu_state.title=FRONTEND_gettitle(menu_state.stack[menu_state.stackpos-1].mode,menu_state.stack[menu_state.stackpos-1].selected);
		}
		else
		{
			menu_state.title=0;
		}
		break;
	}

	menu_state.mode=mode;

	if (mode>=100) {
		//InitBackImage("TITLE LEAVES1.TGA");
		FRONTEND_init_xition();
		FRONTEND_MissionBrief(MISSION_SCRIPT,mode-100);
//		MUSIC_mode(MUSIC_MODE_BRIEFING|MUSIC_MODE_FORCE);
		return;
	}
	switch(mode) {
	case FE_MAPSCREEN:
		//InitBackImage("MAP SELECT LEAVES.TGA");
		FRONTEND_init_xition();
		FRONTEND_districts(MISSION_SCRIPT);
		//FRONTEND_MissionList(MISSION_SCRIPT);
		//MUSIC_mode(MUSIC_MODE_FRONTEND);
		break;
	case FE_MAINMENU:
		//InitBackImage(menu_back_names[menu_theme]);
		//UseBackSurface(screenfull_back);


		FRONTEND_init_xition();

		FRONTEND_easy(mode);

		// Always reset the stack in the main menu.
		menu_state.stackpos = 0;

		MUSIC_mode(MUSIC_MODE_FRONTEND);
		if (AllowSave) menu_data[2].Choices=NULL;
		break;
	case FE_SAVESCREEN:
		AllowSave=1;
		if (!menu_state.stackpos) 
		{
			//InitBackImage(menu_config_names[menu_theme]);
			UseBackSurface(screenfull_config);
		}
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_find_savegames ( FALSE, TRUE );
		FRONTEND_easy(mode);
		menu_state.title=XLAT_str_ptr(X_SAVE_GAME);
		break;
	case FE_LOADSCREEN:
		FRONTEND_find_savegames ( TRUE, FALSE );
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_easy(mode);
		break;
	case FE_CONFIG:
		FRONTEND_init_xition();
		FRONTEND_easy(mode);
		break;
#ifdef WANT_AN_EXIT_MENU_ITEM
	case FE_QUIT:
		FRONTEND_easy(mode);
		menu_state.title=XLAT_str_ptr(X_EXIT);
		break;
#endif
	case FE_CONFIG_VIDEO:
		{
		int a,b,c,d,e,f,g,h,i,j,k; // <-- int for compatability with the prototype in aeng.h *shrug*
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_easy(mode);
#ifndef TARGET_DC
#ifdef ALLOW_DANGEROUS_OPTIONS
		FRONTEND_do_drivers();
#endif
#endif
		if (the_display.IsGammaAvailable())
			FRONTEND_do_gamma();
		FRONTEND_restore_video_data();
		}
		break;
	case FE_CONFIG_AUDIO:
		{
		SLONG fx,amb,mus;
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_easy(mode);
		// now put in correct values...

		MFX_get_volumes(&fx,&amb,&mus);



		menu_data[0].Data=fx<<1;
		menu_data[1].Data=amb<<1;
		menu_data[2].Data=mus<<1;
		}
		break;
#ifdef WANT_A_KEYBOARD_ITEM
	case FE_CONFIG_INPUT_KB:
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_easy(mode);
		// now put in correct values...
		menu_data[0].Data = ENV_get_value_number("keyboard_left",		203, "Keyboard");
		menu_data[1].Data = ENV_get_value_number("keyboard_right",		205, "Keyboard");
		menu_data[2].Data = ENV_get_value_number("keyboard_forward",	200, "Keyboard");
		menu_data[3].Data = ENV_get_value_number("keyboard_back",		208, "Keyboard");
		menu_data[4].Data = ENV_get_value_number("keyboard_punch",		 44, "Keyboard");
		menu_data[5].Data = ENV_get_value_number("keyboard_kick",		 45, "Keyboard");
		menu_data[6].Data = ENV_get_value_number("keyboard_action",		 46, "Keyboard");
//		menu_data[7].Data = ENV_get_value_number("keyboard_run",		 47, "Keyboard");
		menu_data[7].Data = ENV_get_value_number("keyboard_jump",		 57, "Keyboard");
		menu_data[8].Data = ENV_get_value_number("keyboard_start",		 15, "Keyboard");
		menu_data[9].Data = ENV_get_value_number("keyboard_select",	 28, "Keyboard");
		// gap for label
		menu_data[11].Data = ENV_get_value_number("keyboard_camera",	207, "Keyboard");
		menu_data[12].Data = ENV_get_value_number("keyboard_cam_left",	211, "Keyboard");
		menu_data[13].Data= ENV_get_value_number("keyboard_cam_right",	209, "Keyboard");
		menu_data[14].Data= ENV_get_value_number("keyboard_1stperson",	 30, "Keyboard");
		break;
#endif
	case FE_CONFIG_INPUT_JP:
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_easy(mode);
		menu_data[0].Data = ENV_get_value_number("joypad_kick",		4, "Joypad");
		menu_data[1].Data = ENV_get_value_number("joypad_punch",	3, "Joypad");
		menu_data[2].Data = ENV_get_value_number("joypad_jump",		0, "Joypad");
		menu_data[3].Data = ENV_get_value_number("joypad_action",	1, "Joypad");
		menu_data[4].Data = ENV_get_value_number("joypad_move",		7, "Joypad");
		menu_data[5].Data = ENV_get_value_number("joypad_start",	8, "Joypad");
		menu_data[6].Data = ENV_get_value_number("joypad_select",	2, "Joypad");
		// gap for label
		menu_data[8].Data = ENV_get_value_number("joypad_camera",	6, "Joypad");
		menu_data[9].Data = ENV_get_value_number("joypad_cam_left",	9, "Joypad");
		menu_data[10].Data = ENV_get_value_number("joypad_cam_right",10, "Joypad");
		menu_data[11].Data =ENV_get_value_number("joypad_1stperson",5, "Joypad");
		break;
	case FE_CONFIG_OPTIONS:
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_easy(mode);
#ifdef TARGET_DC
		menu_data[0].Data|=ENV_get_value_number("scanner_follows",		1, "Game");
		menu_data[1].Data|=ENV_get_value_number("analogue_pad_mode",	0, "Game");
		menu_data[2].Data|=ENV_get_value_number("vibration_mode",		1, "Game");
		menu_data[3].Data|=ENV_get_value_number("vibration_engine",		1, "Game");
#else
		menu_data[1].Data|=ENV_get_value_number("scanner_follows",		1, "Game");
#endif
		break;
	case FE_SAVE_CONFIRM:
		if ( bDoTransition )
		{
			FRONTEND_init_xition();
		}
		FRONTEND_easy(mode);
		menu_state.scroll=0;
		menu_state.title=XLAT_str_ptr(X_SAVE_GAME);
		break;
	}

	FRONTEND_recenter_menu();
}

void	FRONTEND_draw_districts() {
	UBYTE i,j,id;
	SWORD x,y;
	CBYTE *str;
	UWORD fade;
	ULONG rgb;

	if (bonus_this_turn)
	{
		if (bonus_this_turn==1)
		{
			bonus_this_turn++;
			MFX_play_stereo(0,S_TUNE_BONUS,0);
		}
		fade = (64 - fade_state) << 2;	
		if (IsEnglish)
		{
			str="Bonus mission unlocked!"; // XLAT_str(X_BONUS_MISSION_UNLOCKED)
			FONT2D_DrawStringCentred(str,322,10,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D, fade);
#ifdef TARGET_DC
			// A bit darker please.
			FONT2D_DrawStringCentred(str,322,6,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D, fade);
			FONT2D_DrawStringCentred(str,318,10,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D, fade);
			FONT2D_DrawStringCentred(str,318,6,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D, fade);
#endif
			FONT2D_DrawStringCentred(str,320,8,0xffffff,SMALL_FONT_SCALE,POLY_PAGE_FONT2D, fade);
		}
	}


	fade = (64 - fade_state) << 2;

	{
		CBYTE	str2[200];
		sprintf(str2,"%s: %03d  %s: %03d  %s: %03d  %s: %03d\n",XLAT_str_ptr(X_CON_INCREASED),the_game.DarciConstitution,XLAT_str_ptr(X_STA_INCREASED),the_game.DarciStamina,XLAT_str_ptr(X_STR_INCREASED),the_game.DarciStrength,XLAT_str_ptr(X_REF_INCREASED),the_game.DarciSkill);
		int iYpos;
		if ( eDisplayType == DT_NTSC )
		{
			// Move it further up so it's on screen for the yanks.
			iYpos = 434;
		}
		else
		{
			iYpos = 454;
		}
		FONT2D_DrawStringCentred(str2,320+2,iYpos+2,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D,          fade);
		FONT2D_DrawStringCentred(str2,320  ,iYpos  ,0xffffff,SMALL_FONT_SCALE,POLY_PAGE_FONT2D,          fade);
	}

	for (i=0;i<district_count;i++) {
		switch(district_valid[i]) {
		case 1:
			FRONTEND_draw_button(districts[i][0],districts[i][1],(UBYTE)(i==district_selected)|4,i==district_flash);
			break;
		case 2:
		case 3:
			FRONTEND_draw_button(districts[i][0],districts[i][1],(i==district_selected)?5:6,i==district_flash);
			break;
		}

		/*

		{
			CBYTE num[16];

			sprintf(num, "%d", i);

			FONT2D_DrawString(num, districts[i][0],districts[i][1],0xffffff);
		}

		*/

		if (i==district_selected)
		{
			y = districts[i][1];
			x = districts[i][0];

			str = menu_buffer;

			for (j = 0; j < mission_count; j++)
			{
				id = *str++;

				//
				// What colour do we draw this bit of text?
				//

				if (j == mission_selected)
				{
					rgb = 0xffffff;
				}
				else
				{
					rgb = 0x667788;
				}

			  /*

			  rgb=FRONTEND_fix_rgb(fade_rgb,j==mission_selected);
			  fade=(j==mission_selected)?0:127;


			  */

				fade = (64 - fade_state) << 2;

				if (fade > 255) fade = 255;

				if (mission_hierarchy[id] & 4) // 4 => available.
				{
					if (x>320)
					{
						FONT2D_DrawStringRightJustify(str,x+18+2,y+2,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D, fade);
						FONT2D_DrawStringRightJustify(str,x+18,y,rgb,SMALL_FONT_SCALE,POLY_PAGE_FONT2D,          fade);

						//
						// Draw a line through completed missions!
						//

// These are just tweaked values - dunno why they work/don't work. Just accept it.
#define STRIKETHROUGH_HANGOVER_L 5
#define STRIKETHROUGH_HANGOVER_R 25

						if (mission_hierarchy[id] & 2)	// 2 => complete
						{
							FONT2D_DrawStrikethrough(
								FONT2D_leftmost_x + 2 - STRIKETHROUGH_HANGOVER_L,
								x                 + 2 + STRIKETHROUGH_HANGOVER_R,
								y                 + 2,
								0x000000,
								256,
								POLY_PAGE_FONT2D,
								fade, FALSE);

							FONT2D_DrawStrikethrough(
								FONT2D_leftmost_x + 0 - STRIKETHROUGH_HANGOVER_L,
								x				  + 0 + STRIKETHROUGH_HANGOVER_R,
								y                ,
								rgb,
								256,
								POLY_PAGE_FONT2D,
								fade, TRUE);
						}
					}
					else
					{
						FONT2D_DrawString(str,x+32+2,y+2,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D, fade);
						FONT2D_DrawString(str,x+32,y,rgb,SMALL_FONT_SCALE,POLY_PAGE_FONT2D,          fade);

						//
						// Draw a line through completed missions!
						//

#undef STRIKETHROUGH_HANGOVER_L
#undef STRIKETHROUGH_HANGOVER_R
// These are just tweaked values - dunno why they work/don't work. Just accept it.
#define STRIKETHROUGH_HANGOVER_L (-15)
#define STRIKETHROUGH_HANGOVER_R 30

						if (mission_hierarchy[id] & 2)	// 2 => complete
						{
							FONT2D_DrawStrikethrough(
								x                  + 2 - STRIKETHROUGH_HANGOVER_L,
								FONT2D_rightmost_x + 2 + STRIKETHROUGH_HANGOVER_R,
								y                  + 2,
								0x000000,
								256,
								POLY_PAGE_FONT2D,
								fade, FALSE);

							FONT2D_DrawStrikethrough(
								x				   - STRIKETHROUGH_HANGOVER_L,
								FONT2D_rightmost_x + STRIKETHROUGH_HANGOVER_R,
								y,
								rgb,
								256,
								POLY_PAGE_FONT2D,
								fade, TRUE);
						}
					}
				}

				str+=strlen(str)+1;
#ifdef TRUETYPE
				y += GetTextHeightTT() * 3/4;
#else
				y+=20;
#endif
			}
		}
	}
}


void FRONTEND_shadowed_text ( char *pcString, int iX, int iY, DWORD dwColour )
{
		FONT2D_DrawString(
			pcString,
			iX+2,
			iY+2,
			0,
			256,
			POLY_PAGE_FONT2D,
			0);

		FONT2D_DrawString(
			pcString,
			iX-1,
			iY-1,
			0,
			256,
			POLY_PAGE_FONT2D,
			0);

		FONT2D_DrawString(
			pcString,
			iX+1,
			iY+1,
			( dwColour & 0xfefefefe ) >> 1,
			256,
			POLY_PAGE_FONT2D,
			0);

		FONT2D_DrawString(
			pcString,
			iX,
			iY,
			dwColour,
			256,
			POLY_PAGE_FONT2D,
			0);
}


void	FRONTEND_display()
{
	UBYTE i;
	SLONG rgb, x,x2,y;
	MenuData *md=menu_data;
	UBYTE whichmap[]={2,0,1,3};
	UBYTE arrow=0;


	//DumpTracies();


#if 1
	LPDIRECT3DDEVICE3 dev = the_display.lp_D3D_Device;
	HRESULT hres;


	D3DVIEWPORT2 vp;
	vp.dwSize = sizeof ( vp );
	vp.dwX = the_display.ViewportRect.x1;
	vp.dwY = the_display.ViewportRect.y1;
	vp.dwWidth = the_display.ViewportRect.x2 - the_display.ViewportRect.x1;
	vp.dwHeight = the_display.ViewportRect.y2 - the_display.ViewportRect.y1;
	vp.dvClipX = (float)vp.dwX;
	vp.dvClipY = (float)vp.dwY;
	vp.dvClipWidth = (float)vp.dwWidth;
	vp.dvClipHeight = (float)vp.dwHeight;
	vp.dvMinZ = 0.0f;
	vp.dvMaxZ = 1.0f;
	hres = the_display.lp_D3D_Viewport->SetViewport2 ( &vp );
#endif



	the_display.lp_D3D_Viewport->Clear(1, &the_display.ViewportRect, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET);

	ShowBackImage();
	if ((fade_mode&3)==1) FRONTEND_show_xition();
#ifndef TARGET_DC
	POLY_frame_init(FALSE, FALSE);
#endif
	FRONTEND_kibble_draw();
	//POLY_frame_draw(FALSE, FALSE);


	int iBigFontScale = BIG_FONT_SCALE;
	if ( !IsEnglish )
	{
		if ( ( menu_state.mode == FE_SAVESCREEN ) ||
			 ( menu_state.mode == FE_LOADSCREEN ) ||
			 ( menu_state.mode == FE_SAVE_CONFIRM ) )
		{
			// Reduce it a bit to fit long French words on screen.
			iBigFontScale = BIG_FONT_SCALE_FRENCH;
		}
	}

	//POLY_frame_init(FALSE, FALSE);
	for (i=0;i<menu_state.items;i++,md++) {
		y=md->Y+menu_state.base-menu_state.scroll;
		if ((y>=100)&&(y<=400)) {
			rgb=FRONTEND_fix_rgb(fade_rgb,i==menu_state.selected);
			if ((md->Type==OT_BUTTON)&&(md->Choices==(CBYTE*)1)) // 'greyed out'
			{
				SLONG rgbtemp=rgb&0xff000000;
				//rgb>>=25; //rgb<<=1;
				rgb=(rgb&0xff)>>1;
				rgb|=(rgb<<8)|(rgb<<16);
				rgb|=rgbtemp;
			}


			MENUFONT_Draw(md->X,y,iBigFontScale,md->Label,rgb,0);

#ifdef TARGET_DC
			if ( i==menu_state.selected )
			{
				// Draw a rectangle around the text.
				MENUFONT_Draw_Selection_Box(md->X,y,iBigFontScale,md->Label,rgb,0);
			}
#endif
			switch (md->Type) {
			case OT_SLIDER:		FRONTEND_DrawSlider(md);	break;
			case OT_MULTI:		FRONTEND_DrawMulti(md,rgb);		break;
			case OT_KEYPRESS:	FRONTEND_DrawKey(md);		break;
			case OT_PADPRESS:	FRONTEND_DrawPad(md);		break;
			}
		} else {
			if (i==menu_state.selected) { // better do some scrolling
#ifdef TARGET_DC
				// As the man say, it's so slow I want to knaw out my own liver rather than wait for this.
				if (y<100) menu_state.scroll-=20;
				if (y>400) menu_state.scroll+=20;
#else
				if (y<100) menu_state.scroll-=10;
				if (y>400) menu_state.scroll+=10;
#endif
			}
			if (y<100) arrow|=1;
			if (y>400) arrow|=2;
		}
	}
	if (arrow&1) // draw a "more..." arrow at the top of the screen
	{
		DRAW2D_Tri(320,50, 335,65, 305,65,fade_rgb,0);
	}
	if (arrow&2) // draw a "more..." arrow at the bottom of the screen
	{
		DRAW2D_Tri(320,430, 335,415, 305,415,fade_rgb,0);
	}
	if (menu_state.title && (menu_state.mode != FE_MAINMENU)) {
		BOOL dir;
		x2=SIN(fade_state<<3)>>10;
		switch(menu_state.mode) {
		case FE_MAPSCREEN:
			MENUFONT_Dimensions(menu_state.title,x,y,-1,BIG_FONT_SCALE);
			x=560-x;
			x2=(x2*10)-63;
			dir=0;
			break;
		default:
			x=80;
			x2=642-(x2*10);
			dir=1;
			break;
		}
		FontPage=POLY_PAGE_NEWFONT;
		FRONTEND_draw_title(x+2,44,x2,menu_state.title,0,dir);
#ifndef TARGET_DC
		POLY_frame_draw(FALSE, FALSE);
		POLY_frame_init(FALSE, FALSE);
#endif
		FontPage=POLY_PAGE_NEWFONT_INVERSE;
		FRONTEND_draw_title(x,40,x2,menu_state.title,1,dir);
#ifndef TARGET_DC
		POLY_frame_draw(FALSE, FALSE);
		POLY_frame_init(FALSE, FALSE);
#endif
		FRONTEND_draw_button(x2,8,whichmap[menu_theme]);
	}
	if ((menu_state.mode==FE_MAPSCREEN)&&((fade_mode==2)||(fade_state==63))) FRONTEND_draw_districts();
	if ((menu_state.mode>=100)&&*menu_buffer) {
		//DRAW2D_Box(0,48,640,432,fade_rgb&0xff000000,1,127);
#ifndef TARGET_DC
		POLY_frame_draw(FALSE, FALSE);
		POLY_frame_init(FALSE, FALSE);
#endif
		x=SIN(fade_state<<3)>>10;
		FRONTEND_draw_button(642-(x*10),8,whichmap[menu_theme]);
		FONT2D_DrawStringWrapTo(menu_buffer,20+2,100+2,0x000000,SMALL_FONT_SCALE,POLY_PAGE_FONT2D,255-(fade_state<<2),402);
		FONT2D_DrawStringWrapTo(menu_buffer,20,100,fade_rgb,SMALL_FONT_SCALE,POLY_PAGE_FONT2D,255-(fade_state<<2),400);
	}

	if ( m_bMovingPanel )
	{
		// Display the panel at the current position.

		// REMEMBER THAT THESE NUMBERS ARE DIVIDED BY 4!
		int iXPos = ENV_get_value_number ( "panel_x", 32 / 4, "" );
		int iYPos = ENV_get_value_number ( "panel_y", (480 - 32) / 4, "" );

extern void PANEL_draw_quad( float left,float top,float right,float bottom,SLONG page,ULONG colour,
				float u1,float v1,float u2,float v2);

		PANEL_draw_quad(
			(float)( iXPos * 4 + 0 ),
			(float)( iYPos * 4 - 165 ),
			(float)( iXPos * 4 + 212 ),
			(float)( iYPos * 4 - 0 ),
			POLY_PAGE_LASTPANEL_ALPHA,
			0xffffffff,
			0.0F,
			90.0F / 256.0F,
			212.0F / 256.0F,
			1.0F);
	}

#ifndef TARGET_DC
	POLY_frame_draw(FALSE, FALSE);
#endif

}


void	FRONTEND_storedata() {
	switch(menu_state.mode) {
	case FE_CONFIG_VIDEO:
		FRONTEND_store_video_data();
		break;
	case FE_CONFIG_AUDIO:
		MFX_stop(WEATHER_REF,MFX_WAVE_ALL);
		//MFX_stop(SIREN_REF,MFX_WAVE_ALL);
//		MFX_stop(MFX_CHANNEL_ALL,MFX_WAVE_ALL);
//		MFX_free_wave_list();

		MFX_set_volumes(menu_data[0].Data>>1,menu_data[1].Data>>1,menu_data[2].Data>>1);
		break;

#ifdef WANT_A_KEYBOARD_ITEM
	case FE_CONFIG_INPUT_KB:
		ENV_set_value_number("keyboard_left",		menu_data[0].Data, "Keyboard");
		ENV_set_value_number("keyboard_right",		menu_data[1].Data, "Keyboard");
		ENV_set_value_number("keyboard_forward",	menu_data[2].Data, "Keyboard");
		ENV_set_value_number("keyboard_back",		menu_data[3].Data, "Keyboard");
		ENV_set_value_number("keyboard_punch",		menu_data[4].Data, "Keyboard");
		ENV_set_value_number("keyboard_kick",		menu_data[5].Data, "Keyboard");
		ENV_set_value_number("keyboard_action",		menu_data[6].Data, "Keyboard");
//		ENV_set_value_number("keyboard_run",		menu_data[7].Data, "Keyboard");
		ENV_set_value_number("keyboard_jump",		menu_data[7].Data, "Keyboard");
		ENV_set_value_number("keyboard_start",		menu_data[8].Data, "Keyboard");
		ENV_set_value_number("keyboard_select",		menu_data[9].Data, "Keyboard");
		// gap for label
		ENV_set_value_number("keyboard_camera",		menu_data[11].Data, "Keyboard");
		ENV_set_value_number("keyboard_cam_left",	menu_data[12].Data, "Keyboard");
		ENV_set_value_number("keyboard_cam_right",	menu_data[13].Data , "Keyboard");
		ENV_set_value_number("keyboard_1stperson",	menu_data[14].Data, "Keyboard");
		break;
#endif

	case FE_CONFIG_INPUT_JP:
		ENV_set_value_number("joypad_kick",			menu_data[0].Data, "Joypad");
		ENV_set_value_number("joypad_punch",		menu_data[1].Data, "Joypad");
		ENV_set_value_number("joypad_jump",			menu_data[2].Data, "Joypad");
		ENV_set_value_number("joypad_action",		menu_data[3].Data, "Joypad");
		ENV_set_value_number("joypad_move",			menu_data[4].Data, "Joypad");
		ENV_set_value_number("joypad_start",		menu_data[5].Data, "Joypad");
		ENV_set_value_number("joypad_select",		menu_data[6].Data, "Joypad");
		// gap for label
		ENV_set_value_number("joypad_camera",		menu_data[8].Data, "Joypad");
		ENV_set_value_number("joypad_cam_left",		menu_data[9].Data, "Joypad");
		ENV_set_value_number("joypad_cam_right",	menu_data[10].Data , "Joypad");
		ENV_set_value_number("joypad_1stperson",	menu_data[11].Data, "Joypad");
		break;

	case FE_CONFIG_OPTIONS:
#ifdef TARGET_DC
		ENV_set_value_number("scanner_follows",		menu_data[0].Data&1, "Game");
		ENV_set_value_number("analogue_pad_mode",	menu_data[1].Data&1, "Game");
		ENV_set_value_number("vibration_mode",		menu_data[2].Data&1, "Game");
		ENV_set_value_number("vibration_engine",	menu_data[3].Data&1, "Game");
#else
		ENV_set_value_number("scanner_follows",		menu_data[1].Data&1, "Game");
#endif
		break;

	}
}

BOOL	FRONTEND_ValidMission(SWORD sel) {
	CBYTE *str=menu_buffer;
	UBYTE id=*str;

	while (sel) {
		sel--;
		str++;
		str+=strlen(str)+1;
		id=*str;
	}
	return (BOOL)(mission_hierarchy[id]&4);
}

UBYTE	FRONTEND_input() {
	UBYTE scan, any_button=0;
	static SLONG last_input=0;
	static UBYTE last_button=0;
	static UBYTE first_pad=1;

	SLONG input=0;

	if (grabbing_pad&&!last_input)
	{
		UBYTE i,j;
		MenuData *item=menu_data+menu_state.selected;
		ReadInputDevice();
		if (Keys[KB_ESC]||(input&INPUT_MASK_CANCEL))
		{
			Keys[KB_ESC]=0;
#if 0
			// Should we not just leave this as it is, rather than setting it to "unused"?
			item->Data=31;
#endif
			grabbing_pad=0;
		}
		else
		{
			for (i=0;i<32;i++)
			{
				if (the_state.rgbButtons[i] & 0x80)
				{
					for (j=0;j<menu_state.items;j++)
					{
						if (menu_data[j].Data==i)
						{
							menu_data[j].Data=31;
						}
					}
					item->Data=i;
					last_button=1;
					grabbing_pad=0;
					break;
				}
			}
		}
		return 0;
	} else {

#ifdef ALLOW_JOYPAD_IN_FRONTEND


// these are in interfac.cpp -- but the NOISE_TOLERANCE there, while appropriate for the game,
// is SHITTY for the menu. so here's a new one.

#define	AXIS_CENTRE			32768
#define	NOISE_TOLERANCE		4096
#define	AXIS_MIN			(AXIS_CENTRE-NOISE_TOLERANCE)
#define	AXIS_MAX			(AXIS_CENTRE+NOISE_TOLERANCE)

		input = get_hardware_input(INPUT_TYPE_JOY);

		input&=~(INPUT_MASK_LEFT|INPUT_MASK_RIGHT|INPUT_MASK_FORWARDS|INPUT_MASK_BACKWARDS);
		if(the_state.lX>AXIS_MAX)
		{
			input |= INPUT_MASK_RIGHT;
		}
		else if(the_state.lX<AXIS_MIN)
		{
			input |= INPUT_MASK_LEFT;
		}

		// let's not allow diagonals, they're silly.

		if(the_state.lY>AXIS_MAX)
		{
			input&=~(INPUT_MASK_LEFT|INPUT_MASK_RIGHT);
			input|=INPUT_MASK_BACKWARDS;
		}
		else if(the_state.lY<AXIS_MIN)
		{
			input&=~(INPUT_MASK_LEFT|INPUT_MASK_RIGHT);
			input|=INPUT_MASK_FORWARDS;
		}

		if (input==last_input) {
			input=0;
			any_button=0;
		}
		else
		{
			last_input=input;
			for (scan=0;scan<8;scan++) 
				any_button |= the_state.rgbButtons[scan];
			// Supress very first movement - PC joysticks have a
			// strange habit of doing one movement on boot-up for some reason.
			if (first_pad)
			{
				if (any_button)
					first_pad=0;
				else
				if (input& (INPUT_MASK_LEFT|INPUT_MASK_RIGHT|INPUT_MASK_FORWARDS|INPUT_MASK_BACKWARDS)) {
					first_pad=0;
					input=0;
				}
			}
			if (last_button)
			{
				if (!any_button)
					last_button=0;
				else
					any_button=0;
			}
		}
//		TRACE("proc'd input: %d\n",input);

#endif

	}

	if (grabbing_key&&LastKey) {
		MenuData *item=menu_data+menu_state.selected;
//		CBYTE key=(Keys[KB_LSHIFT]||Keys[KB_RSHIFT]) ?  InkeyToAsciiShift[LastKey] : InkeyToAscii[LastKey];
/*		switch(LastKey){
		case KB_LEFT: case KB_RIGHT: case KB_UP: case KB_DOWN: case KB_ENTER: case KB_SPACE:
		case KB_LSHIFT: case KB_RSHIFT: case KB_LALT: case KB_RALT: case KB_LCONTROL: case KB_RCONTROL:
		case KB_TAB: case KB_END: case KB_HOME: case KB_DEL: case KB_INS: case KB_PGUP: case KB_PGDN:
			item->Data=LastKey; 
			Keys[LastKey]=0;
			break;
		case KB_ESC: break;
		default:
			item->Data=key;
		}*/
		if (LastKey!=KB_ESC) {
			UBYTE j;
			for (j=0;j<menu_state.items;j++)
				if (menu_data[j].Data==LastKey) menu_data[j].Data=0;
			item->Data=LastKey;

//			CBYTE moo[20];
//			GetKeyNameText(LastKey<<16,moo,20);

		}
		Keys[LastKey]=0;
		grabbing_key=0;
		//Keys[KB_ESC]=Keys[KB_UP]=Keys[KB_DOWN]=Keys[KB_LEFT]=Keys[KB_RIGHT]=Keys[KB_ENTER]=0;
		return 0;
	}
	if(allow_debug_keys)
	{
		if (Keys[KB_1]||Keys[KB_2]||Keys[KB_3]||Keys[KB_4]) 
		{
			if (Keys[KB_1]) { Keys[KB_1]=0; menu_theme=0; }
			if (Keys[KB_2]) { Keys[KB_2]=0; menu_theme=1; }
			if (Keys[KB_3]) { Keys[KB_3]=0; menu_theme=2; }
			if (Keys[KB_4]) { Keys[KB_4]=0; menu_theme=3; }
			//InitBackImage(menu_back_names[menu_theme]);
			UseBackSurface(screenfull_back);

			FRONTEND_kibble_init();
		}
	}

	if (Keys[KB_END])
	{
		Keys[KB_END]=0;
		MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
		menu_state.selected=menu_state.items-1;
		if (menu_state.mode==FE_MAPSCREEN) mission_selected=mission_count-1;
		while (((menu_data+menu_state.selected)->Type==OT_LABEL)||(((menu_data+menu_state.selected)->Type==OT_BUTTON)&&((menu_data+menu_state.selected)->Choices==(CBYTE*)1))) 
			menu_state.selected--;
	}
	if (Keys[KB_HOME])
	{
		Keys[KB_HOME]=0;
		MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
		menu_state.selected=0;
		if (menu_state.mode==FE_MAPSCREEN) mission_selected=0;
		while (((menu_data+menu_state.selected)->Type==OT_LABEL)||(((menu_data+menu_state.selected)->Type==OT_BUTTON)&&((menu_data+menu_state.selected)->Choices==(CBYTE*)1))) 
			menu_state.selected++;
	}
	if (Keys[KB_UP]||(input&INPUT_MASK_FORWARDS)) {
		Keys[KB_UP]=0;
		MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
		if (menu_state.selected>0) menu_state.selected--;
			else menu_state.selected=menu_state.items-1;
		while (((menu_data+menu_state.selected)->Type==OT_LABEL)||(((menu_data+menu_state.selected)->Type==OT_BUTTON)&&((menu_data+menu_state.selected)->Choices==(CBYTE*)1))) {
			if (menu_state.selected>0) menu_state.selected--;
			else menu_state.selected=menu_state.items-1;
		}
		if ((menu_state.mode==FE_MAPSCREEN)&&mission_selected) mission_selected--;
	}
	if (Keys[KB_DOWN]||(input&INPUT_MASK_BACKWARDS)) {
		Keys[KB_DOWN]=0;
		MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
		if (menu_state.selected<menu_state.items-1) menu_state.selected++;
			else menu_state.selected=0;
		while (((menu_data+menu_state.selected)->Type==OT_LABEL)||(((menu_data+menu_state.selected)->Type==OT_BUTTON)&&((menu_data+menu_state.selected)->Choices==(CBYTE*)1))) {
			if (menu_state.selected<menu_state.items-1) menu_state.selected++;
			else menu_state.selected--;
		}
		if ((menu_state.mode==FE_MAPSCREEN)&&(mission_selected<mission_count-1)&&(FRONTEND_ValidMission(mission_selected+1))) mission_selected++;
	}

	if (Keys[KB_ENTER ] ||
		Keys[KB_SPACE ] ||
		Keys[KB_PENTER] ||
		any_button
		)
	{
		Keys[KB_ENTER ] = 0;
		Keys[KB_SPACE ] = 0;
		Keys[KB_PENTER] = 0;
		MenuData *item=menu_data+menu_state.selected;

		if (fade_mode!=2) MFX_play_stereo(0,S_MENU_CLICK_END,MFX_REPLACE);
		FRONTEND_stop_xition();

		if ((menu_state.mode==FE_SAVESCREEN)&&(item->Data!=FE_BACK))
		{
			save_slot=menu_state.selected+1;
//			item->LabelID=X_SAVE_GAME;
/*			menu_mode_queued=FE_SAVE_CONFIRM;
			fade_mode=2;
			FRONTEND_kibble_flurry();
			return 0;*/
		}

		if ((menu_state.mode==FE_SAVESCREEN)&&(item->Data==FE_MAPSCREEN))
		{
			menu_mode_queued=FE_MAPSCREEN;
			menu_state.stackpos=0;
			menu_thrash=FE_MAINMENU;
		}

		if ((menu_state.mode==FE_SAVE_CONFIRM)&&(item->Data==FE_MAPSCREEN)) 
		{
			if ( item->LabelID == X_CANCEL )
			{
				// Cancel was pressed - just exit.
				menu_mode_queued=FE_MAPSCREEN;
				fade_mode=2;
				FRONTEND_kibble_flurry();
				menu_state.stackpos=0;
				menu_thrash=FE_MAINMENU;
				return 0;
			}

			CBYTE ttl[_MAX_PATH];
			FRONTEND_fetch_title_from_id(MISSION_SCRIPT,ttl,mission_launch);
			bool bSuccess = FRONTEND_save_savegame(ttl,save_slot);

			if ( bSuccess )
			{
				menu_mode_queued=FE_MAPSCREEN;
				fade_mode=2;
				FRONTEND_kibble_flurry();
				menu_state.stackpos=0;
				menu_thrash=FE_MAINMENU;
				return 0;
			}
			else
			{
				// Failed. Just buzz and return to the main menu.
				// I hope this "no" sound is a decent one - sound doesn't work yet!
				MFX_play_stereo(0,S_MENU_CLICK_END,MFX_REPLACE);
				menu_mode_queued=FE_MAPSCREEN;
				fade_mode=2;
				FRONTEND_kibble_flurry();
				menu_state.stackpos=0;
				menu_thrash=FE_MAINMENU;
				return 0;
			}
		}
		if (menu_state.mode==FE_MAPSCREEN) {
			if (mission_count>0) {
				menu_mode_queued=100+mission_selected;
				fade_mode=2;
				FRONTEND_kibble_flurry();
			}
			return 0;
		}

		if (menu_state.mode>=100)
		{
			// Starting a mission.
			// Stop any briefing.
			MFX_QUICK_stop();
			return FE_START;
		}
		if ((menu_state.mode==FE_LOADSCREEN)&&(item->Data==FE_SAVE_CONFIRM))
		{
			bool bSuccess = FRONTEND_load_savegame(menu_state.selected+1);
			if ( bSuccess )
			{
				FRONTEND_MissionHierarchy(MISSION_SCRIPT);
				menu_mode_queued=FE_MAPSCREEN;
				fade_mode=2;
				FRONTEND_kibble_flurry();
				return 0;
			}
			else
			{
				// Failed the load. For now, do nothing.
				// For example, can be if they select an "(EMPTY)" slot.
				// I hope this "no" sound is a decent one - sound doesn't work yet!
				MFX_play_stereo(0,S_MENU_CLICK_END,MFX_REPLACE);
				// Quit back to the main menu.
				menu_mode_queued=FE_MAINMENU;
				fade_mode=2;
				FRONTEND_kibble_flurry();
				return 0;
			}
		}
		switch (item->Type) {
		case OT_MULTI:
			// Cycle through all options.
			item->Data = ( item->Data & ~0xff ) | ( ( item->Data & 0xff ) + 1 );
			if ( ( item->Data & 0xff ) >= ( item->Data >> 8 ) )
			{
				item->Data &= ~0xff;
			}
			break;
		case OT_KEYPRESS:
			grabbing_key=1;
			LastKey=0;
			break;
		case OT_PADPRESS:
			if ( bCanChangeJoypadButtons )
			{
				grabbing_pad=1;
				//LastKey=0;
				last_input = 0;
			}
			else
			{
				// Can't change button - using a predefined setup.
				// I hope this "no" sound is a decent one - sound doesn't work yet!
				MFX_play_stereo(0,S_MENU_CLICK_END,MFX_REPLACE);
			}
			break;
		case OT_PADMOVE:
			// Enter pad-move mode.
			m_bMovingPanel = TRUE;
			break;
		case OT_BUTTON:
		case OT_BUTTON_L:
			if (menu_state.mode==FE_START) return FE_LOADSCREEN;
#ifdef WANT_AN_EXIT_MENU_ITEM
			//if (item->Data==-1) return -1;
			if (item->Data==FE_NO_REALLY_QUIT) return -1;
#endif
			//if (item->Data==-2) FRONTEND_storedata();
			if (item->Data==FE_BACK) FRONTEND_storedata();
			if (item->Data==FE_START) return FE_LOADSCREEN;
			// temp thingy:
			//if (item->Data==FE_MAPSCREEN) return FE_START;
			if (item->Data==FE_EDITOR) return FE_EDITOR;
			if (item->Data==FE_CREDITS) return FE_CREDITS;
			
			FRONTEND_kibble_flurry();

			//FRONTEND_mode(item->Data);
			menu_mode_queued=item->Data;
			fade_mode=2|((item->Data==FE_BACK)?4:0);
			break;
		case OT_RESET:
			switch(menu_state.mode){
#ifdef WANT_A_KEYBOARD_ITEM
			case FE_CONFIG_INPUT_KB:
				menu_data[0].Data = 203;
				menu_data[1].Data = 205;
				menu_data[2].Data = 200;
				menu_data[3].Data = 208;
				menu_data[4].Data = 44;
				menu_data[5].Data = 45;
				menu_data[6].Data = 46;
				//menu_data[7].Data = 47;
				menu_data[7].Data = 57;
				menu_data[8].Data = 15;
				menu_data[9].Data = 28;
				// gap for label
				menu_data[11].Data = 207;
				menu_data[12].Data = 211;
				menu_data[13].Data= 209;
				menu_data[14].Data= 30;
				break;
#endif
			case FE_CONFIG_INPUT_JP:
#ifdef TARGET_DC
				menu_data[0].Data = 1 | (5<<8);
				// gap for label
				menu_data[2].Data = 9;
				menu_data[3].Data = 10;
				menu_data[4].Data = 7;
				menu_data[5].Data = 1;
				menu_data[6].Data = 3;
				menu_data[7].Data = 0;
				// gap for label
				menu_data[9].Data = 4;
				menu_data[10].Data = 5;
				menu_data[11].Data = 6;
				menu_data[12].Data = 8;
#else
				menu_data[0].Data = 4;
				menu_data[1].Data = 3;
				menu_data[2].Data = 0;
				menu_data[3].Data = 1;
				menu_data[4].Data = 7;
				menu_data[5].Data = 8;
				menu_data[6].Data = 2;
				// gap for label
//				menu_data[7].Data = 99;
				menu_data[8].Data = 6;
				menu_data[9].Data = 9;
				menu_data[10].Data = 10;
				menu_data[11].Data = 5;
#endif
				break;
			}
			break;
		}
	}
	if (Keys[KB_LEFT]||(input&INPUT_MASK_LEFT)) {
		Keys[KB_LEFT]=0;
		MenuData *item=menu_data+menu_state.selected;
		if ((item->Type==OT_SLIDER)&&(item->Data>0))
		{
			item->Data--;
			if (item->Data>0)
			{
				item->Data--;
			}
		}



		if ((menu_state.mode==FE_MAPSCREEN)&&district_selected) {
			scan=district_selected-1;
			while ((scan>0)&&!district_valid[scan]) scan--;
			if (district_valid[scan]) {
				district_selected=scan;
				FRONTEND_MissionList(MISSION_SCRIPT,districts[district_selected][2]);
				MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
			}
		}

		if ((item->Type==OT_MULTI)&&((item->Data&0xff)>0)) {
			item->Data--;
			MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
		}

		if (menu_state.mode==FE_CONFIG_VIDEO) FRONTEND_gamma_update();
		if ((menu_state.mode==FE_CONFIG_AUDIO)&&!menu_state.selected) {
			MFX_play_stereo(1,S_TRAFFIC_CONE,0);
			//MFX_set_gain(1,S_TRAFFIC_CONE,255);
		}

	}
	if (Keys[KB_RIGHT]||(input&INPUT_MASK_RIGHT)) {
		Keys[KB_RIGHT]=0;
		MenuData *item=menu_data+menu_state.selected;
		if ((item->Type==OT_SLIDER)&&(item->Data<255))
		{
			item->Data++;
			if (item->Data<255)
			{
				item->Data++;
			}
		}



		if ((menu_state.mode==FE_MAPSCREEN)&&(district_selected<district_count-1)) {
			scan=district_selected+1;
			while ((scan<district_count-1)&&!district_valid[scan]) scan++;
			if (district_valid[scan]) {
				district_selected=scan;
				FRONTEND_MissionList(MISSION_SCRIPT,districts[district_selected][2]);
				MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
			}
		}

		if ((item->Type==OT_MULTI)&&((item->Data&0xff)<(item->Data>>8)-1)) {
			item->Data++;
			MFX_play_stereo(1,S_MENU_CLICK_START,MFX_REPLACE);
		}

		if (menu_state.mode==FE_CONFIG_VIDEO) FRONTEND_gamma_update();
		if ((menu_state.mode==FE_CONFIG_AUDIO)&&!menu_state.selected) {
			MFX_play_stereo(1,S_TRAFFIC_CONE,0);
			//MFX_set_gain(1,S_TRAFFIC_CONE,255);
		}

	}
	if (Keys[KB_ESC]||(input&INPUT_MASK_CANCEL)) {
		Keys[KB_ESC]=0;
//		if (menu_state.mode>=100)
//			MFX_QUICK_stop();
		if (fade_mode!=6) MFX_play_stereo(1,S_MENU_CLICK_END,MFX_REPLACE);
		if (fade_mode==2) // cancel a transition
		{
			fade_mode=1;
			menu_mode_queued=menu_state.mode;
			return 0;
		}
		//FRONTEND_scr_del();
		if (menu_state.stackpos)
		{
			switch(menu_state.mode)
			{
			case FE_CONFIG_VIDEO: // eidos want ESC to store opts
				FRONTEND_store_video_data();
				break;
			case FE_CONFIG_AUDIO:
				MFX_stop(WEATHER_REF,MFX_WAVE_ALL);
				//MFX_stop(SIREN_REF,MFX_WAVE_ALL);
				MFX_set_volumes(menu_data[0].Data>>1,menu_data[1].Data>>1,menu_data[2].Data>>1);
				break;
			}

			// Store any options settings.
			FRONTEND_storedata();

			menu_mode_queued=FE_BACK;
		}
		else
		{
#ifdef WANT_AN_EXIT_MENU_ITEM
			menu_mode_queued=FE_QUIT;
#else
			menu_mode_queued=FE_MAINMENU;
#endif
		}

		if ((menu_state.mode==FE_SAVESCREEN)&&!menu_state.stackpos) {
			menu_thrash=FE_MAINMENU;
			menu_mode_queued=FE_MAPSCREEN;
		}
		fade_mode=6;

		FRONTEND_kibble_flurry();
	}

	return 0;
}



//--- externally accessible ---


void	FRONTEND_init ( bool bGoToTitleScreen )
{

	static bool bFirstTime = TRUE;


	dwAutoPlayFMVTimeout = timeGetTime() + AUTOPLAY_FMV_DELAY;

	// Stop the music while loading stuff.
	stop_all_fx_and_music();
	//MFX_QUICK_stop();

	// These two are so that when a mission ends with a camera still active, the music
	// stuff actually works properly. Obscure or what?
extern UBYTE EWAY_conv_active;
extern SLONG EWAY_cam_active;
	EWAY_conv_active = FALSE;
	EWAY_cam_active = FALSE;

	TICK_RATIO = (1 << TICK_SHIFT);

	// Set up the current language
	switch ( 0 )
	{
	case 0:
		pcSpeechLanguageDir = "talk2\\";
		break;
	case 1:
		pcSpeechLanguageDir = "talk2_french\\";
		break;
	default:
		ASSERT ( FALSE );
		break;
	}


	// Reset the transition buffer's contents.
	lpFRONTEND_show_xition_LastBlit = NULL;

	CBYTE *str, *lang=ENV_get_value_string("language");

#ifdef VERSION_FRENCH
	// Kludge for the DC converter
	if (!lang) lang="text\\lang_french.txt";
#else
	if (!lang) lang="text\\lang_english.txt";
#endif
	XLAT_load(lang);
	XLAT_init();

	IsEnglish=!stricmp(XLAT_str(X_THIS_LANGUAGE_IS),"English");

	str=menu_choice_yesno;
	strcpy(str,XLAT_str(X_NO));
	str+=strlen(str)+1;
	strcpy(str,XLAT_str(X_YES));

	strcpy(MISSION_SCRIPT,"data\\");
	lang=XLAT_str(X_THIS_LANGUAGE_IS);
	if (strcmp(lang,"English")==0)
		strcat(MISSION_SCRIPT,"urban");
	else
		strcat(MISSION_SCRIPT,lang);
	strcat(MISSION_SCRIPT,".sty");

	//
	// Load the mission script into memory so we don't have
	// to scan a file all the time!
	//

	CacheScriptInMemory(MISSION_SCRIPT);

	MENUFONT_Load("olyfont2.tga",POLY_PAGE_NEWFONT_INVERSE,frontend_fonttable);

void MENUFONT_MergeLower(void);	
	MENUFONT_MergeLower();

	menu_theme = 0;
	
	//InitBackImage(menu_back_names[menu_theme]);
	UseBackSurface(screenfull_back);

	// Er... we call FRONTEND_kibble_init just below. Whatever....
	ZeroMemory(kibble,sizeof(kibble));
	ZeroMemory(&menu_state,sizeof(menu_state));
	if (!complete_point) ZeroMemory(mission_hierarchy,60);
	menu_state.mode=-1;

	FRONTEND_CacheMissionList(MISSION_SCRIPT);

	the_display.lp_D3D_Viewport->Clear(1, &the_display.ViewportRect, D3DCLEAR_ZBUFFER);

	ZeroMemory(menu_choice_scanner,255);
	XLAT_str(X_CAMERA,menu_choice_scanner);
	lang=menu_choice_scanner+strlen(menu_choice_scanner)+1;
	XLAT_str(X_CHARACTER,lang);

	MUSIC_mode(MUSIC_MODE_FRONTEND);

	FRONTEND_scr_new_theme(
		menu_back_names  [menu_theme],
		menu_map_names   [menu_theme],
		menu_brief_names [menu_theme],
		menu_config_names[menu_theme]);

	if ( !bFirstTime )
	{
		UseBackSurface(screenfull_back);
	}

	SLONG fx,amb,mus;
	MFX_get_volumes(&fx,&amb,&mus);
	MUSIC_gain(mus);

	FRONTEND_districts(MISSION_SCRIPT);
	FRONTEND_MissionHierarchy(MISSION_SCRIPT);
	FRONTEND_kibble_init();


//extern bool g_bGoToCreditsPleaseGameHasFinished;

	if ( m_bGoIntoSaveScreen )
	{
		// Just won a mission - going into save game.
		FRONTEND_mode(FE_SAVESCREEN);
		m_bGoIntoSaveScreen = FALSE;
	}
	else
	{
		// Frontend menu.
		FRONTEND_mode(FE_MAINMENU);
	}

	// Stop all the music - about to start it again, properly.
	stop_all_fx_and_music();
	MUSIC_reset();
	MUSIC_mode(0);
	MUSIC_mode_process();
	MUSIC_mode(MUSIC_MODE_FRONTEND);


	if (mission_launch)
	{
		// Just won a mission - going into save game.
		return;
	}

	bFirstTime = FALSE;
}

void	FRONTEND_level_lost() 
{
	mission_launch=previous_mission_launch;
	// Start up the kibble again.
	FRONTEND_kibble_init();
	ZeroMemory(&menu_state,sizeof(menu_state));
	menu_state.mode=-1;
//	FRONTEND_MissionHierarchy(MISSION_SCRIPT);
	FRONTEND_mode(FE_MAINMENU);
}

void	FRONTEND_level_won() 
{

	ASSERT(mission_launch<50);


	// update hierarchy data.
	mission_hierarchy[mission_launch]|=2; // complete

extern bool g_bPunishMePleaseICheatedOnThisLevel;
	if ( !g_bPunishMePleaseICheatedOnThisLevel )
	{
		// time to update roper/darci doohickeys
		if (1)//NET_PERSON(0)->Genus.Person->PersonType==PERSON_DARCI) 
		{
			SLONG	found;
			found=NET_PLAYER(0)->Genus.Player->Constitution - the_game.DarciConstitution;
			ASSERT(found>=0);

			if(found>best_found[mission_launch][0])
			{
				the_game.DarciConstitution+=found-best_found[mission_launch][0];
				best_found[mission_launch][0]=found;
			}

			found=NET_PLAYER(0)->Genus.Player->Strength - the_game.DarciStrength;
			ASSERT(found>=0);

			if(found>best_found[mission_launch][1])
			{
				the_game.DarciStrength+=found-best_found[mission_launch][1];
				best_found[mission_launch][1]=found;
			}

			found=NET_PLAYER(0)->Genus.Player->Stamina - the_game.DarciStamina;
			ASSERT(found>=0);

			if(found>best_found[mission_launch][2])
			{
				the_game.DarciStamina+=found-best_found[mission_launch][2];
				best_found[mission_launch][2]=found;
			}

			found=NET_PLAYER(0)->Genus.Player->Skill - the_game.DarciSkill;
			ASSERT(found>=0);

			if(found>best_found[mission_launch][3])
			{
				the_game.DarciSkill+=found-best_found[mission_launch][3];
				best_found[mission_launch][3]=found;
			}
	/*
			the_game.DarciConstitution=NET_PLAYER(0)->Genus.Player->Constitution;
			the_game.DarciStrength=NET_PLAYER(0)->Genus.Player->Strength;
			the_game.DarciStamina=NET_PLAYER(0)->Genus.Player->Stamina;
			the_game.DarciSkill=NET_PLAYER(0)->Genus.Player->Skill;
	*/
		} 
		else 
		{
	#ifdef TARGET_DC
			// Not used.
			ASSERT ( FALSE );
	#else
			the_game.RoperConstitution=NET_PLAYER(0)->Genus.Player->Constitution;
			the_game.RoperStrength=NET_PLAYER(0)->Genus.Player->Strength;
			the_game.RoperStamina=NET_PLAYER(0)->Genus.Player->Stamina;
			the_game.RoperSkill=NET_PLAYER(0)->Genus.Player->Skill;
	#endif
		}
	}


	// Start up the kibble again.
	FRONTEND_kibble_init();
	ZeroMemory(&menu_state,sizeof(menu_state));
	menu_state.mode=-1;
	if (complete_point<mission_launch) complete_point=mission_launch; 
	//mission_launch++;
	FRONTEND_MissionHierarchy(MISSION_SCRIPT);
/*  hierarchy does both of these now
	InitBackImage(menu_back_names[menu_theme]);
	FRONTEND_kibble_init();*/
	FRONTEND_mode(FE_SAVESCREEN);
	m_bGoIntoSaveScreen = TRUE;
}

void FRONTEND_playambient3d(SLONG channel, SLONG wave_id, SLONG flags, UBYTE height = 0)
{
	SLONG	angle = Random() & 2047;

	SLONG	x = (COS(angle) << 4);
	SLONG	y = 0;
	SLONG	z = (SIN(angle) << 4);

	if (height == 1)	y += (512 + (Random() & 1023)) << 8;

	MFX_play_xyz(channel, wave_id, 0, x, y, z);
//	MFX_set_gain(channel, wave_id,255);
}


void	FRONTEND_sound() {
	static SLONG siren_time=100;
	SLONG wave_id;

	MFX_play_ambient(WEATHER_REF,S_WIND_START,MFX_LOOPED|MFX_QUEUED);
//	MFX_play_ambient(MUSIC_REF,S_TUNE_DRIVING,MFX_LOOPED);
	MFX_set_gain(WEATHER_REF,S_AMBIENCE_END,255);
	//MFX_set_gain(MUSIC_REF,S_TUNE_FRONTEND,menu_data[2].Data>>1);
	MUSIC_gain(menu_data[2].Data>>1);
	MFX_set_volumes(menu_data[0].Data>>1,menu_data[1].Data>>1,menu_data[2].Data>>1);

	// Update the music volume.
//extern void DCLL_stream_volume(float volume);
//	DCLL_stream_volume ( 1.0f );


	siren_time--;
	if(siren_time<0)
	{
		wave_id = S_SIREN_START + (Random() % 4);
		siren_time = 300 + ((Random() & 0xFFFF) >> 5);
		FRONTEND_playambient3d(SIREN_REF, wave_id, 0, 1);
	}
	MFX_set_listener(0,0,0,0,0,0);
	MFX_update();
}


void	FRONTEND_diddle_stats()
{
#ifndef	FINAL
#ifndef TARGET_DC
	SWORD stat_up = ENV_get_value_number("stat_up",		0, "Secret");
	stat_up*=(mission_launch-1);

	the_game.DarciConstitution=stat_up;
	the_game.DarciStrength=stat_up;
	the_game.DarciStamina=stat_up;
	the_game.DarciSkill=stat_up;
	the_game.RoperConstitution=stat_up;
	the_game.RoperStrength=stat_up;
	the_game.RoperStamina=stat_up;
	the_game.RoperSkill=stat_up;
#endif
#endif

}

void	FRONTEND_diddle_music()
{
	TRACE("STARTSCR_mission: %s\n",STARTSCR_mission);
	MUSIC_bodge_code=0;
	if (strstr(STARTSCR_mission,"levels\\fight")||strstr(STARTSCR_mission,"levels\\FTutor"))
		MUSIC_bodge_code=1;
	else
		if (strstr(STARTSCR_mission,"levels\\Assault")) 
			MUSIC_bodge_code=2;
		else
			if (strstr(STARTSCR_mission,"levels\\testdrive")) 
				MUSIC_bodge_code=3;
			else
				if (strstr(STARTSCR_mission,"levels\\Finale1"))
					MUSIC_bodge_code=4;


}

UBYTE this_level_has_the_balrog;
UBYTE this_level_has_bane;
UBYTE	is_semtex=0;

SBYTE	FRONTEND_loop() {
	SBYTE res;

	static SLONG last = 0;
	static SLONG now = 0;

	SLONG millisecs;
	
	now       = GetTickCount();

	if (last < now - 250)
	{
		last = now - 250;
	}

	millisecs = now - last;
	last      = now;

	//
	// How fast should the fade state fade?
	//

	SLONG fade_speed = (millisecs >> 3);

	if (fade_speed < 1)
	{
		fade_speed = 1;
	}

	switch(fade_mode&3)
	{
		case 1:
			if (fade_state<63)
			{
				fade_state += fade_speed;

				if (fade_state > 63)
				{
					fade_state = 63;
				}
			}
			else
			{
				FRONTEND_stop_xition();

				fade_mode = 0;
			}
			break;

		case 2:
			if (fade_state>0)
			{
				fade_state -= fade_speed;

				if (fade_state < 0)
				{
					fade_state = 0;
				}
			}
			else
			{
				FRONTEND_mode(menu_mode_queued);
			}
			break;
	}
	fade_rgb=(((SLONG)fade_state*2)<<24)|0xFFFFFF;

	{
		FRONTEND_kibble_process();
	}

#ifndef TARGET_DC
	PolyPage::DisableAlphaSort();
#endif
	FRONTEND_display();
	if ((menu_state.mode==FE_CONFIG_AUDIO)&&(fade_mode==0))
	{
		FRONTEND_sound();
	}
	else
	{
		MFX_set_listener(0,0,0,0,0,0);
		MFX_update();
	}
#ifndef TARGET_DC
	PolyPage::EnableAlphaSort();
#endif
	res=FRONTEND_input();
	//MUSIC_process();
	// This was commented out - why????
	// Aha - because otherwise, after a briefing, the music starts up over the memstream music.
	MUSIC_mode_process();


	// dodgy hidden keys

#ifndef VERSION_DEMO

#ifndef TARGET_DC
//#ifndef NDEBUG
	if(ControlFlag && ShiftFlag)
	{
		if (Keys[KB_PPLUS])
		{ Keys[KB_PPLUS]=0; complete_point++; FRONTEND_MissionHierarchy(MISSION_SCRIPT); cheating=1; }
		if (Keys[KB_ASTERISK])
		{ Keys[KB_ASTERISK]=0; complete_point=40; FRONTEND_MissionHierarchy(MISSION_SCRIPT); cheating=1; }
	}
//#endif

#else


#ifdef DEBUG
	// Just for Tom's debugging.
	if (
		(the_state.rgbButtons[DI_DC_BUTTON_A])&&
		(the_state.rgbButtons[DI_DC_BUTTON_B])&&
		(the_state.rgbButtons[DI_DC_BUTTON_X])&&
		(the_state.rgbButtons[DI_DC_BUTTON_Y]))
	{
		if (the_state.rgbButtons[DI_DC_BUTTON_RTRIGGER])
		{ complete_point++; FRONTEND_MissionHierarchy(MISSION_SCRIPT); cheating=1; }
		if (the_state.rgbButtons[DI_DC_BUTTON_LTRIGGER])
		{ complete_point=40; FRONTEND_MissionHierarchy(MISSION_SCRIPT); cheating=1; }
	}
#endif

	// Proper harder to find cheat.
extern int g_iCheatNumber;
	if ( g_iCheatNumber == 0x1a1a0001 )
	{
		// Expose all missions.
		g_iCheatNumber = 0;
		if (the_state.rgbButtons[DI_DC_BUTTON_LTRIGGER])
		{ complete_point=40; FRONTEND_MissionHierarchy(MISSION_SCRIPT); cheating=1; }
	}

#endif

#endif


#ifdef WANT_AN_EXIT_MENU_ITEM
	if (res==FE_NO_REALLY_QUIT) return STARTS_EXIT;
#endif
	if (res==FE_EDITOR)			return STARTS_EDITOR;
	if (res==FE_LOADSCREEN)		return STARTS_START;

	if (res==FE_START)
	{
		//
		// Start playing!!!
		//

		struct
		{
			CBYTE *mission;
			SLONG  dontload;
			SLONG  has_balrog;

		} whattoload[] =
		{
			{"testdrive1a.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"FTutor1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"Assault1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"police1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"police2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"Bankbomb1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"police3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"Gangorder2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"police4.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"bball2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"wstores1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"e3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | 0, /* balrog ? */ FALSE},
			{"westcrime1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"carbomb1.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"botanicc.ucm", (1 << PERSON_MIB1) | 0, /* balrog ? */ FALSE},
			{"Semtex.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"AWOL1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"mission2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"park2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"MIB.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"skymiss2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"factory1.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"estate2.ucm", (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"Stealtst1.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"snow2.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"Gordout1.ucm", (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"Baalrog3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ TRUE },
			{"Finale1.ucm", (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"Gangorder1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"FreeCD1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"jung3.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, /* balrog ? */ FALSE},
			{"fight1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"fight2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"testdrive2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"testdrive3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},
			{"Album1.ucm", (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, /* balrog ? */ FALSE},

			{"!"}
		};
		
		//
		// What level are we loading?
		//

		SLONG index_into_the_whattoload_array;

		previous_mission_launch=mission_launch;
		strcpy(STARTSCR_mission,"levels\\");
		strcat(STARTSCR_mission,FRONTEND_MissionFilename(MISSION_SCRIPT,menu_state.mode-100));
//		strcpy(STARTSCR_mission,"c:\\master~1\\italian\\bankbomb1.ucm");

		index_into_the_whattoload_array = -1;

		SLONG i;

		for (i = 0; whattoload[i].mission[0] != '!'; i++)
		{
			if (strcmp(FRONTEND_MissionFilename(MISSION_SCRIPT,menu_state.mode-100), whattoload[i].mission) == 0)
			{
				ASSERT(index_into_the_whattoload_array == -1);

				index_into_the_whattoload_array = i;
			}
		}

		ASSERT(index_into_the_whattoload_array != -1);

		//
		// What should we or shouldn't we load?
		//
		
		ASSERT(WITHIN(index_into_the_whattoload_array, 0, 35));

#ifndef TARGET_DC
		extern ULONG DONT_load;

		this_level_has_the_balrog = whattoload[index_into_the_whattoload_array].has_balrog;
		this_level_has_bane       = (index_into_the_whattoload_array == 27);	// Just in the finale...
		DONT_load                 = whattoload[index_into_the_whattoload_array].dontload;

		//
		// This makes us load all people. Comment it out to load only the
		// people the level needs.
		//

		DONT_load = 0;

		// If doing all levels, don't use DONT_load. The two don't mix.
		ASSERT ( !DONT_load );
		
		//
		// Does this level have violence?
		//
#endif
		if(index_into_the_whattoload_array==20) //semtex  wetback
		{
			is_semtex=1;
		}
		else
		{
			is_semtex=0;
		}

		if (index_into_the_whattoload_array == 31 ||
			index_into_the_whattoload_array == 32 ||
			index_into_the_whattoload_array == 1)
		{
			//
			// No violence.
			//

			VIOLENCE = FALSE;
		}
		else
		{
			//
			// Default violence.
			//

			VIOLENCE = TRUE;
		}

		if (cheating) FRONTEND_diddle_stats();

#ifdef TARGET_DC
		g_iLevelNumber = mission_launch;
#endif

		FRONTEND_diddle_music();
		menu_state.stackpos = 0;
		menu_thrash         = 0;
		return STARTS_START;
	}

	if (res == FE_CREDITS)
	{
		{
			extern void OS_hack(void);
			OS_hack();
			MUSIC_mode(MUSIC_MODE_FRONTEND);
			FRONTEND_kibble_init();
		}
	}

	return 0;
}

#endif




