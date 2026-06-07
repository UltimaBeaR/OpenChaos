// Game entry point (compiled as MF_main via #define main MF_main in MFStdLib.h).
// WinMain (in engine/graphics/graphics_engine/backend_directx6/host.cpp) calls MF_main(argc, argv).
// Startup sequence: set base path → load config → locate CD → read detail levels → run game.
#include "engine/platform/uc_common.h" // H_CREATE_LOG, FileSetBasePath
#include "engine/platform/host.h" // SetupHost, ResetHost
#include "engine/graphics/pipeline/aeng.h" // AENG_read_detail_levels
#include "engine/io/env.h" // ENV_load
#include "engine/io/oc_config.h" // OC_CONFIG_set_persistence
#include "engine/io/user_data.h" // USERDATA_init
#include "game/game.h" // game
#include "game/missing_resources.h" // MISSING_RESOURCES_present

// uc_orig: main (fallen/Source/Main.cpp)
// Application startup. Called via MF_main() (the MFStdLib macro renames main to MF_main
// so that Win32 WinMain can invoke it). Initialises the engine and runs the game loop.
SLONG main(UWORD argc, char* argv[])
{
    // Resolve the per-user writable data folder first — all writes (config,
    // saves, crash log) go there, and reads overlay it on top of the game
    // folder. Must precede any file I/O below. See engine/io/user_data.h.
    USERDATA_init();

    FileSetBasePath("");

    // If the original game data is missing, the game can't run — it will only
    // show the "files not found" screen and exit. Detect that here (before any
    // config is touched) and disable config persistence, so a misplaced exe
    // leaves no OpenChaos.config.json behind. The screen itself is shown later,
    // once the renderer is up (see game_startup).
    if (!MISSING_RESOURCES_present())
        OC_CONFIG_set_persistence(false);

    ENV_load("config.ini");

    AENG_read_detail_levels();

    if (SetupHost(H_CREATE_LOG)) {
        game();
    }
    ResetHost();

    return EXIT_SUCCESS;
}
