#include "engine/platform/host.h"

// uc_common.h defines: #define main(ac, av) MF_main(ac, av)
// This macro rewrites startup.cpp's main() into MF_main() for the legacy call chain.
// We need a real main() here, so undo the macro.
#undef main

// Application entry point.
//
// Full startup call chain:
//
//   main.cpp: main()
//     → engine/platform/host.cpp: HOST_run()
//         Sets up crash handler, single-instance guard, then calls MF_main().
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
#ifdef _WIN32
// Hybrid-GPU hint (laptops with both an integrated and a discrete GPU). The
// NVIDIA Optimus and AMD switchable-graphics drivers look for these exported
// symbols in the executable: a non-zero value asks the driver to run the app on
// the high-performance DISCRETE GPU. Without them such laptops commonly default
// OpenGL to the weaker integrated GPU. Windows-only — Linux selects the GPU via
// PRIME environment variables, and Apple Silicon has a single GPU. (windows.h
// is not included here, so the types are spelled out: NvOptimusEnablement is a
// DWORD = unsigned long, the AMD flag is an int.)
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char* argv[])
{
    return HOST_run(argc, argv);
}
