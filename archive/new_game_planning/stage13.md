# Stage 13 — Functionality improvements

**Goal:** functional and visual improvements on top of a stable base (after the 1.0 release).

Former Stages 6, 9, 10.

---

## Possible improvements

1. Increasing the map size: remove the hardcoded small size, possibly map streaming
2. Gameplay improvements: improved car handling, improved target locking, etc.
3. Visual improvements (without replacing assets): dynamic lighting and shadows, post-processing (bloom, color grading, SSAO), anti-aliasing, improved texture filtering, improved particles and effects, improved fog/atmosphere
4. [Testing infrastructure](stage13_testing_infrastructure.md)
5. [Architecture refactoring](stage13_architecture_refactoring.md)
6. [Full dead-code cleanup](stage13_dead_code_cleanup.md) — cppcheck + linker `--gc-sections`, clean out everything left over from the original and the iterative refactoring
7. [Gamepad](stage13_gamepad.md) (generic + common)
8. [DualSense](stage13_dualsense.md) (DualSense-specific)
9. [Additional content](stage13_custom_content.md) (new maps/missions, editor)
10. [AI mission-creation system](stage13_ai_level_design.md) (MCP + AI workflow)
11. [Global ideas](stage13_future_ideas.md) — radical changes, require a "do we even need this" decision
12. Any other improvements and refinements, including ones suggested by the community

---

## Control improvements

A major rework of the controls (D-pad → stick, roll vs. side jump, WASD defaults, mouse camera control, Tank/Stick-like switch in the options, functional keyboard↔gamepad validation) was **done in 1.0** — see the `known_issues_and_bugs_resolved.md` entry "Controls rework". Here is what did NOT make it into 1.0:

- **Mouse shooting / aiming** — an optional mode when playing with the keyboard: in addition to rotating the camera with the mouse (that's in 1.0) — also aiming/shooting with the mouse. Standard WASD+mouse as in modern 3rd person action games. → duplicated in [known_issues_and_bugs.md "Controls"](../known_issues_and_bugs/known_issues_and_bugs.md).
- **Steam Input API** — a unified API for all controllers via Steam. Provides button remapping, glyphs. Unclear whether it's available for non-Steam games — needs research. → duplicated in [known_issues_and_bugs.md "Controls"](../known_issues_and_bugs/known_issues_and_bugs.md).

## Control hints and in-game help

The game is non-obvious in its controls — many actions (rolls, back strikes, combos, parkour moves) are hard to discover on your own.

- **Contextual button hints** — extend the idea of the PS1 hints (the dotted NPC aiming lines, see `GAMEPLAY_CHANGES.md`) to the whole game: show hints everywhere it makes sense — in menus, in combat, when interacting with objects, during the tutorial. Button icons depend on the device: DualSense → PlayStation icons (✕, △, ○, □), Xbox/generic → Xbox icons (A, B, X, Y), keyboard → key names. Device-type detection already works (`InputDeviceType`: keyboard / Xbox / DualSense — switches automatically based on the last active input).
- **Visual layout diagram in the options** — an item in the settings that shows the full control layout: which button does what, with a visual image of the controller/keyboard.
- **In-game wiki/reference** — a knowledge base of the game's capabilities, accessible from the pause or main menu: combat moves (running + left + jump = roll, in combat strike + back = back kick, etc.), parkour moves, interactions. Format: short cards with a description and a button layout.

## Mods and third-party content

**Principle:** everything third-party and modified is disabled by default and enabled explicitly by the user. By default the user plays the standard version, close to the original.

- **Third-party mod support** — build a mod-loading system (compatibility with the original won't be achievable).
- **Integrating community mod functionality** — implement some features from existing mods (Maddi Edition and others) directly in the main game's source: first person camera, disabling auto-aim/radar, etc. The mods are made via binary hacks of the original exe and aren't directly compatible with the remake — we only take the ideas. Links to the mods: `new_game_devlog/community_links.md`

## In-game launcher

Decision: **there will be no separate windowed launcher**. Instead — an in-game screen that launches first, before any intro splash. One window for everything, convenient on the Steam Deck.

**Concept:**
- Launches **first** when the exe starts, before any game splashes/intros.
- Its own mini-renderer for the screen: an "OpenChaos" title + some animation (possibly autumn leaves like in the missions — as a thematic reference).
- A short disclaimer about the Urban Chaos IP — that this is an opensource version, makes no claim to the rights, and so on. Concise, no walls of text.
- The **"Launch game"** item is pre-selected — the player presses the cross (DualSense) / Enter (keyboard) and the launcher goes away, the game starts.
- The launcher has an **Exit** — closes the program.
- **Settings** — UI for the options (details later). Some things from the old PC menu may move here (resource folder selection, control settings, experimental features, etc.).
  - **Config hint (preliminary idea, may not do it):** on the settings screen, show the user a short reminder that these same settings can also be changed by hand in the `.json` config (with an approximate path given). In case they set something wrong and want to roll it back, or set a resolution at which the game won't start — so they know where to look for a manual reset/edit.
- **Info** — a menu item with information about the project:
  - **Main author of the rework:** UltimaBeaR.
  - A short description of what this rework is: an open-source modernization of the original Urban Chaos (1999, MuckyFoot), a modern stack (OpenGL, SDL3, x64), a new renderer, DualSense and Xbox support, etc.
  - A link to the project's GitHub repository.
  - Possibly a link/nod to the forks we branched off from (PieroZ and the earlier ones — see [CONTRIBUTORS.md](../CONTRIBUTORS.md)).
  - A short thank-you to MuckyFoot for the original.
- From the actual in-game menu (Exit game) the player returns **back to the launcher**, not to the OS. The launcher is the central hub.

**Architecture:**
- **A separate folder** like `new_game/src/openchaos_launcher_screen/` (or similar) — all of the launcher's code lives there. **Does not use the game's code** — only the render-engine abstraction (so as not to duplicate the GL/Vulkan layer).
- Its own "mini-engine" for rendering/layout inside the launcher.
- The resource format for the launcher is still open: either our own (for flexibility), or we reuse the original game's formats (for simplicity). We'll decide during implementation.

**Custom resources — the `open_chaos/` folder:**
- Next to the exe there's an `open_chaos/` folder — **all** custom game resources that don't belong to the original Urban Chaos go there:
  - Launcher resources (animations, fonts, logos)
  - In the future — mod resources, third-party content (see the "Mods and third-party content" section)
- The original Urban Chaos resources stay in the standard game folder (where the game already reads them from now).
- This is a clean separation: "original" vs. "ours and the modders'".

**Out of scope for 1.0** — [stage12.md](stage12.md#L65) records that the launcher is not in 1.0.

## Configuration and settings

The main part of the settings will move into the in-game launcher's UI (see above).

- **Resource folder selection** — right now the user copies the resources by hand. We need a UI for specifying the path to the Steam folder (the path is saved in the config, not asked again).
- **Control settings** — a new section in the options menu (replacing the removed "Joystick" from 1.0). Details → [stage13_gamepad.md](stage13_gamepad.md)
- **UI for experimental features** — right now the feature flags are hardcoded in `feature_flags.cpp`. Make an "Experimental" / "Extras" tab in the settings to enable them without recompiling. Saved to the config (`openchaos.cfg`). This also includes the original's cut features — bats, multiplayer, and other things currently behind flags. Description of the flags → [`FEATURE_FLAGS.md`](../FEATURE_FLAGS.md).

## Renderer optimizations

**The basic optimizations are done** (draw call batching, GPU transform, texture array for characters, extending the draw distance and fog — see [stage13_optimizations.md](stage13_optimizations.md) for historical context and the applied techniques). We consider the current performance state sufficient for the target hardware (Steam Deck / weak PCs) given the current architecture.

**There's no point in going further within the current codebase.** Further optimizations would require a **serious rework of the game's architecture and the rendering pipeline** (single-threaded → multi-threaded, rewriting the sprite pipeline, batching transparent geometry without losing ordering, etc.) and **possibly a move to Vulkan**. That's too large an undertaking, **nothing is planned on this topic in post-1.0** — left as a "if there's ever a desire to rewrite everything".

## Fog color per level + skybox on bright missions

**Fog color.** Right now the fog color is the same (close to black) regardless
of the level's environment. On open-space levels / daytime missions
this looks out of place — for example on **Target UC** there are bright
mountains visible in the distance, but the fog behind them is black, producing
a "hole into the sky" instead of a natural atmospheric transition. The idea: tie
the fog color to the level (day / night / enclosed space / open / mountains /
city, etc.). Examples of the tones needed (to be picked on the scene):
- Daytime open levels with a bright background (Target UC, mountain views) →
  light-blue or turquoise fog.
- Dark urban nighttime levels (as now) → dark blue / nearly
  black (effectively the current color).
- Twilight / transitional ones — an intermediate shade.

**The question of the mechanism.** Hardcoding the color per mission is fast but
not universal. We'd like a single way to determine "which fog for
which level". Options:
- A field in the level format (if there's room) — `fog_color` RGB.
- Auto-computing from the sky color / lighting — taking the average tone of
  `NIGHT_sky_colour` or something similar.
- A "level type → color" lookup table + a manual override field.

**Where to change it in the code.** `NIGHT_sky_colour` / `AENG_clear_viewport` —
starting points ([aeng.cpp:8422](../new_game/src/engine/graphics/pipeline/aeng.cpp#L8422)
already uses the sky color for clear). The fog color is set somewhere else —
find it via a `fog` / `u_fog_color` grep.

**Skybox on bright levels.** A related problem: if the player on a
daytime mission tilts the camera far up — the sky is rendered **black**
instead of continuing the skybox with a blue sky. It looks like a hole. Need to
figure out:
- Where in the sky-rendering pipeline the black zone appears at a large pitch.
- Is the skybox even initialized on such levels? (It may have
  limited vertical coverage.)
- Either extend the skybox zone, or add a gradient fill for
  the top as a fallback.

Both items are about the visual perception of open space,
discuss them together.

## Debugging and cheats

- **Cheat to load a mission by number** — a cheat or debug mode: a key combination → enter the mission number → load. For debugging — so as not to replay the game just to reach late missions. Note: it seems all missions are accessible through the normal mission map (after the ASan fixes), but we're not 100% sure.
- **Anti-cheat marking** — think about it: if the user activated a cheat, mark it in the save (e.g. "cheats used") and somehow indicate that the playthrough was done with cheats (options: a mark in the UI, a visual change to the character like a clown mask, or something else — to be thought through). The goal is to remove the motivation to use cheats during a normal playthrough. We need to investigate the save format — how to add a flag without breaking compatibility with the old format.

## Quick saves within a mission

The missions are long and difficult — when killed, the player is forced to replay the whole mission, which is frustrating. The inter-mission saves (the existing system) we don't touch.

**Idea:** limited quick saves (checkpoints) within a single mission. Available only within the current mission playthrough — they reset on leaving the mission or starting a new one. There may be limits (number of saves per mission, cooldown, etc.) so as not to turn the game into save-scumming. Design details — to be thought through later.

**⚠️ There's already an in-mission save/load mechanism inherited from the original in the code — check it during implementation:** the functions `MEMORY_quick_save` / `MEMORY_quick_load` (`missions/memory.cpp`, write a full snapshot of the mission state to `data/quicksave.dat`) and the handlers `X_SAVE_GAME` / `X_LOAD_GAME` in `ui/menus/gamemenu.cpp`. **Right now this is dead code** — no menu (the `GAMEMENU_menu` table in `gamemenu_globals.cpp`) references these items, so `quicksave.dat` is never written. The `data/quicksave.dat` path is now wrapped into the user data folder (see the data-folder system implemented in 1.0). When implementing checkpoints: check that these functions work (they haven't been tested), refine and wire them up to the UI if needed — it may be easier to reuse them than to write from scratch.

## Infrastructure and distribution

**A single home for the "distribution/installation/icons" topic.** For 1.0, distribution is fixed in its current form — per-platform zip packages (`release/`) + a Windows icon embedded in the exe (see `stage12.md` Task #8). **All distribution improvements go here, post-1.0.**

### Installers on all platforms (auto-detect the original's folder)

Goal: installation on top of the existing game folder **without manual copying**, with auto-detection of a copy of Urban Chaos. The current `release/` has script stubs, needs work.

- ▸ **Windows:** a full installer (not a zip) with auto-detection of the original's folder — Steam (`steamapps/common/Urban Chaos`, priority if found and it contains resources) and GOG Galaxy (`GOG Games/Urban Chaos` or similar); if not found — a manual selection dialog; if found — offers it as the default; installs the exe+dependencies next to the original's resources. Tools: **NSIS** or **Inno Setup** (both free, can search for folders via the Steam/GOG registry).
- ▸ **macOS:** a `.dmg` with a `.app` bundle (drop into `/Applications`). The binary in `.app/Contents/MacOS/`, the original's resources — alongside or specified on first launch. Built via `hdiutil` or `create-dmg`. **Investigate signing and notarization** (without notarization, Gatekeeper blocks it on modern macOS). **Open question:** the path to the original's resources — a selection dialog on first launch, or fixed next to the `.app`?
- ▸ **Steam Deck / Linux:** installation **without Desktop Mode** (Game Mode doesn't let you place files directly). Options: a bash `.sh` script from Desktop Mode (finds the Urban Chaos steamapps, copies our binary); Flatpak/AppImage. **Check:** is Urban Chaos available on Steam for the Steam Deck (a Windows game via Proton — the license should be visible from SteamOS). The patch works on top of Proton: our Linux-native binary replaces the Windows exe (Proton is then not needed). Check: adding a non-Steam game with a Linux binary as a Launch option without Proton directly from Game Mode.

### Distribution via Steam (own app / mod or beta branch)

**Main motive:** a real Steam game installs **on the Steam Deck directly from Game Mode** like any other → closes the main pain point with installing on the Deck. Two paths:

- **Our own separate Steam app** (own appID, ~$100, Valve review) that **requires Urban Chaos installed from Steam** — on launch it finds the original in the Steam library and takes the assets from there (we don't bundle the assets ourselves — they belong to My Little Planet). **Precedent:** the paid third-party mod [Maddie Modder Mods for Urban Chaos](https://store.steampowered.com/app/3992700/) (app 3992700) already lives on Steam this way — built on this same open code. **Nuances:** (1) the hard "requires the original" requirement Steam only enforces for DLC, and DLC is only released by the owner of the base game — so we do the Urban Chaos presence check **ourselves** in the launcher (no → "install Urban Chaos", we don't start); (2) the "Urban Chaos" trademark belongs to **My Little Planet Ltd (Guy Simmons)** — mentioning it in the store touches the trademark, in practice Valve/Guy tolerate it (Maddie Modder is named exactly that way and is sold), but it's cleaner to have his consent.
- **An official beta branch at the rights holder's** — the cleanest UX (nothing required from us, the assets are already in place), but content into the beta (whether with a password code or not) can only be uploaded by the owner of the appID — runs into Guy Simmons's consent. We can't put up the beta branch/beta code ourselves.

Rights holder contact — `legal/rights_history.md` ([@LittlePlanetGuy](https://x.com/littleplanetguy)).

### Icon on the other platforms

- ✅ **Windows — done in 1.0.** Our own icon (a maple leaf on a black rounded plate) is embedded in the `.exe` and set as the runtime window icon.
  - Master sources in the repo root: `openchaos_icon.png` (the 1024² leaf — the icon is generated from it) and `openchaos_logo.png` (the OPEN CHAOS wordmark — a separate logo asset, not used in the icon).
  - Build resources in `new_game/resources/icon/`: `OpenChaos.ico` (16/24/32/48/64/128/256, **the leaf everywhere** — the wordmark is unreadable at small sizes, and Windows unpredictably substituted versions), `OpenChaos.rc` (embeds the `.ico` into the exe), `window_icon.png` (embedded into the binary, set via `SDL_SetWindowIcon`).
  - Wired into the build: `enable_language(RC)` + `WIN_RESOURCES` in `new_game/CMakeLists.txt`, `CMAKE_RC_COMPILER=llvm-rc` in the toolchain; the PNG is embedded by the same CMake mechanism as the glyphs.
  - ⚠️ The leaf is a silhouette from a Windows system emoji (Segoe UI Emoji). For a clean public release, replace it with our own/CC0: drop in a new `openchaos_icon.png` and regenerate `.ico`/`window_icon.png`.
- ⏸️ **macOS / Linux / Steam Deck — deferred.** The macOS icon requires an `.app` bundle (`.icns` + `Info.plist`); Linux — via `.desktop`. On the Steam Deck: the `.sh` launcher doesn't carry an icon, auto-pickup without manual setup is only possible if you add to Steam a `.desktop` (not a `.sh`) with absolute paths — requires testing on the hardware and generating paths at install time. Makes sense to do together with the installers above. Use the same logo as in the animated version in the project README.

### Other

- **macOS Intel:** the `clang-x64-macos.cmake` toolchain + a Universal Binary (`clang-universal-macos.cmake`) — so far Apple Silicon only. There are probably very few macOS Intel users (Apple moved to ARM back in 2020), not a blocker.
- **CI/CD:** GitHub Actions for automatic builds on all platforms when a tag is pushed. For 1.0 the builds are done manually; automation — after the release.

## References

- `PieroZ/MuckyFoot-UrbanChaos` commit `0f2e69d` — WIP on the bat/bane boss + explosions (bangs). Worth a look when refining the bosses/effects.
