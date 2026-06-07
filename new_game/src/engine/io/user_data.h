#ifndef ENGINE_IO_USER_DATA_H
#define ENGINE_IO_USER_DATA_H

#include <cstddef>

// Per-user writable data folder ("overlay" root).
//
// Everything the game *writes* (config, savegames, crash log, debug dumps) goes
// into a per-user data folder instead of next to the executable — the install
// directory may be read-only (e.g. Program Files, a Steam Deck mount). The
// folder mirrors the game-folder layout exactly: a relative path like
// "saves/slot1.wag" maps to "<userroot>/saves/slot1.wag".
//
// Reads use an overlay model (see engine/io/file.cpp): the user folder is
// checked first, then the game folder — as if the user folder were laid on top
// of the install, then the result read back. This also gives savegame fallback
// for free (ship a build with saves in the game folder; they load until the
// player overwrites them, after which the user-folder copy wins).
//
// Location is SDL_GetPrefPath(...,"OpenChaos"):
//   Windows  %APPDATA%\OpenChaos\
//   macOS    ~/Library/Application Support/OpenChaos/
//   Linux    ~/.local/share/OpenChaos/
// One shared folder for every installed copy of the game.

// Resolve the user data root. Call once at the very start of main(), before any
// file I/O. Safe to call when SDL is not yet initialized.
void USERDATA_init();

// User data root, with a trailing '/' and forward slashes. Empty string if it
// could not be determined (writes then fall back to the working directory, the
// pre-overlay behavior).
const char* USERDATA_root();

// Map a relative path to its absolute location under the user data root,
// creating any missing parent directories. An absolute input path is copied
// through unchanged. If there is no user data root, `rel` is copied unchanged
// (working-dir fallback). Always writes a NUL-terminated string into `out` and
// returns `out`.
char* USERDATA_resolve_write(const char* rel, char* out, size_t out_size);

#endif // ENGINE_IO_USER_DATA_H
