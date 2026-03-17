// Main.cpp
// Guy Simmons, 17th October 1997.

#include "Game.h"
#include "DDLib.h"
#include "renderstate.h"
#include "Drive.h"

#ifdef GUY
#endif

#ifndef EXIT_SUCCESS

// Not defined on PSX
#define EXIT_SUCCESS 1
#endif

#include "Sound.h"
#include "Memory.h"
#include "env.h"

/*
#include "finaleng.h"


void mkt_test(void)
{
        if (FINALENG_init())
        {
                FINALENG_initialise_for_new_level();

                while(SHELL_ACTIVE)
                {
                        if (Keys[KB_Q])
                        {
                                break;
                        }

                        FINALENG_draw();
                }

                FINALENG_fini();
        }
}
*/

//---------------------------------------------------------------


// VerifyDirectX
//
// init DirectDraw and Direct3D and check they're OK

static int numdevices = 0;

static HRESULT CALLBACK D3DEnumDevicesCallback(GUID FAR* lpGuid,
    LPTSTR lpDeviceDescription,
    LPTSTR lpDeviceName,
    LPD3DDEVICEDESC lpD3DHWDeviceDesc,
    LPD3DDEVICEDESC lpD3DHELDeviceDesc,
    LPVOID lpContext)
{
    if (lpD3DHWDeviceDesc->dwFlags) {
        TRACE("HARDWARE Device %s \"%s\"\n", lpDeviceDescription, lpDeviceName);
        numdevices++;
    } else {
        TRACE("SOFTWARE Device %s \"%s\"\n", lpDeviceDescription, lpDeviceName);
        numdevices++; // count these too
    }
    return D3DENUMRET_OK;
}

extern HINSTANCE hGlobalThisInst;

// claude-ai: Точка входа (PC/DC). Цепочка: SetupHost() → game() → ResetHost(). game() содержит весь игровой цикл.
SLONG main(UWORD argc, TCHAR* argv[])
{
    // Does nothing exciting otherwise.
    FileSetBasePath("");

    ENV_load("config.ini");

    LocateCDROM();

    AENG_read_detail_levels(); // get engine defaults

//	ASSERT(the_qs_sound_manager.Init() == ERROR_QMIX_OK);
//	the_qs_sound_manager.Init() == ERROR_QMIX_OK;


    // claude-ai: SetupHost() создаёт окно и инициализирует DirectX/платформу; game() — весь игровой цикл
    if (SetupHost(H_CREATE_LOG)) {
        //		mkt_test();

        game();
    }
    ResetHost();

    return EXIT_SUCCESS;
}

//---------------------------------------------------------------
