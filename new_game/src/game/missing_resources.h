#ifndef GAME_MISSING_RESOURCES_H
#define GAME_MISSING_RESOURCES_H

// Startup guard for a misplaced executable.
//
// OpenChaos ships only the new engine — it reuses the original Urban Chaos
// game data, so the exe must live in the original game's install folder. If a
// user runs it from the wrong place, none of the resources load. Instead of a
// silent crash (or an OS message box, which is awkward on Steam Deck / a
// fullscreen window), we draw a self-contained "files not found" screen using
// an embedded bitmap font — no external resource is touched — and exit cleanly.

// True if the essential original-game files are present where the game reads
// them (via the FileExists path, i.e. relative to the game's base path).
bool MISSING_RESOURCES_present(void);

// Show the "files not found" screen and exit the process cleanly. Never returns.
// Requires the renderer (ge_init) to be initialised; touches no game resources.
[[noreturn]] void MISSING_RESOURCES_show_and_exit(void);

#endif // GAME_MISSING_RESOURCES_H
