# 100 — Keyboard key rebinding

- Status: 🔵 open
- Area: input
- Tags: feature, gh-issue
- Created: 2026-10-27
- Related: GitHub issue [UltimaBeaR/OpenChaos#9](https://github.com/UltimaBeaR/OpenChaos/issues/9) (UserGit-2021,
  questions); gamepad side — "Gamepad settings in the UI" task; full controls UI idea — "Custom options menu" task

## Description
A player asks how to rebind keyboard keys. Today it's impossible: custom key rebinding was removed in 1.0, the
keyboard bindings are hardcoded in the action map (`act_*.h`), and the old `[Keyboard]` section of `config.ini` is
deliberately not migrated (`oc_config.cpp`). Add rebinding — at least via `OpenChaos.config.json`, later in a UI.

## Important notes
Issue #9 also asks where the settings are stored and whether EAX reverb works — answers need no code:
- Settings location (checked 2026-10-27): `OpenChaos.config.json` (`engine/io/oc_config.h`) in the per-user data folder from
  `SDL_GetPrefPath` (`engine/io/user_data.h`): Windows `%APPDATA%\OpenChaos\`, macOS
  `~/Library/Application Support/OpenChaos/`, Linux `~/.local/share/OpenChaos/`; the config is written with all keys on the first run
  (`oc_config.cpp`, except the "game files not found" startup path, which writes nothing).
- Reverb → #101 (audio goes through OpenAL Soft, no reverb; matches the original PC code).
