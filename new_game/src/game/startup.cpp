// Game entry point (compiled as MF_main via #define main MF_main in MFStdLib.h).
// WinMain (in engine/graphics/graphics_engine/backend_directx6/host.cpp) calls MF_main(argc, argv).
// Startup sequence: set base path → load config → locate CD → read detail levels → run game.
#include "engine/platform/uc_common.h"                                           // H_CREATE_LOG, FileSetBasePath
#include "engine/platform/host.h"                 // SetupHost, ResetHost
#include "engine/graphics/pipeline/aeng.h"                     // AENG_read_detail_levels
#include "engine/io/env.h"                                      // ENV_load
#include "game/game.h"                                      // game

// uc_orig: main (fallen/Source/Main.cpp)
// Application startup. Called via MF_main() (the MFStdLib macro renames main to MF_main
// so that Win32 WinMain can invoke it). Initialises the engine and runs the game loop.
SLONG main(UWORD argc, TCHAR* argv[])
{
    FileSetBasePath("");

    ENV_load("config.ini");

    AENG_read_detail_levels();

    if (SetupHost(H_CREATE_LOG)) {
        game();
    }
    ResetHost();

    return EXIT_SUCCESS;
}
