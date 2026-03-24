#include <windows.h>
#include "engine/platform/host.h"

// Win32 application entry point.
//
// Full startup call chain:
//
//   main.cpp: WinMain()
//     → engine/platform/host.cpp: HOST_run()
//         Stores WinMain params in globals, creates a named event to prevent
//         multiple instances ("UrbanChaosExclusionZone"), then calls MF_main().
//
//       → game/startup.cpp: MF_main()
//           MF_main is NOT declared anywhere explicitly — it exists because
//           uc_common.h (line 82) has: #define main(ac, av) MF_main(ac, av)
//           So startup.cpp's "SLONG main(UWORD argc, TCHAR* argv[])" is silently
//           rewritten by the preprocessor into MF_main(). HOST_run() calls MF_main()
//           and the linker resolves the symbol to startup.cpp.
//
//         → game/game.cpp: game()
//             The actual game loop — attract mode, mission select, gameplay, repeat.
//
// Architecture issue (recorded in stage9.md):
//   engine/platform/host.cpp depends on game/ — it #includes "game/game_types.h"
//   and calls MF_main() which is defined in game/startup.cpp. This is an inverted
//   dependency (engine → game). Should be fixed via callback registration: game
//   registers its entry point with engine during setup, engine calls it when ready.
//
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPTSTR lpszArgs, int iWinMode)
{
    return HOST_run(hThisInst, hPrevInst, lpszArgs, iWinMode);
}
