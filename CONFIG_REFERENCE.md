# Config Reference — OpenChaos

OpenChaos stores its settings in its own JSON config file, `OpenChaos.config.json` (replacing the original game's `config.ini`, except for the language option, which still stays in `config.ini`). This document describes where it lives and what every key does.

Everything adjustable in the **in-game options menu** is written to this file. The file also exposes a number of **extra settings that are not available in the in-game UI** — to change those, edit the JSON directly.

---

## Where it's stored

The config lives in your OS per-user data folder:

- **Windows:** `%APPDATA%\OpenChaos\OpenChaos.config.json` (e.g. `C:\Users\<you>\AppData\Roaming\OpenChaos\OpenChaos.config.json`)
- **macOS:** `~/Library/Application Support/OpenChaos/OpenChaos.config.json`
- **Linux:** `~/.local/share/OpenChaos/OpenChaos.config.json`

The same folder also holds your saves. The game folder itself is treated as read-only.

---

## How it works

- The file is plain JSON, grouped into sections (`audio`, `video`, `game`, …), pretty-printed.
- On first run (or if the file is missing/corrupt) it is written from built-in defaults.
- If a key is missing, the game falls back to its default for that run (it won't necessarily re-add it).
- Values are clamped to a valid range — out-of-range hand-edits are trimmed, not rejected. A value of the wrong type is reset to its default.
- Section names are case-insensitive; keys are case-sensitive.
- Edit the file while the game is **closed** — the game may overwrite it on exit.

**What's in the in-game menu vs config-only:**

- In the menu: the three **audio** volumes (Sound menu) and **`scanner_follows`** (Options menu).
- Written automatically by the game (not menu toggles, but remembered): `windowed_width`, `windowed_height`, `windowed_maximized`.
- Everything else is **config-file-only** — hand-edit the JSON to change it.

---

## audio

Volumes are a fraction from `0.0` (silent) to `1.0` (full).

| Key | Type | Default | What it does | In menu? |
|-----|------|---------|--------------|----------|
| `ambient_volume` | float (0–1) | `1.0` | Ambient / world sound level. | Yes |
| `music_volume` | float (0–1) | `1.0` | Music level. | Yes |
| `fx_volume` | float (0–1) | `1.0` | Sound-effects level (also controls cutscene/movie volume). | Yes |

## video

Display and rendering settings. All config-file-only unless noted.

### Display

| Key | Type | Default | What it does |
|-----|------|---------|--------------|
| `fullscreen` | bool | `true` | Fullscreen vs windowed mode. |
| `windowed_width` | int | `640` | Window width in pixels (windowed mode). Updated automatically when you resize the window. |
| `windowed_height` | int | `480` | Window height in pixels (windowed mode). Updated automatically when you resize the window. |
| `windowed_maximized` | bool | `false` | Start maximized in windowed mode. Updated automatically when you maximize/restore. |
| `vsync` | bool | `true` | Vertical sync (caps tearing to the display refresh). |
| `fps_cap` | int | `300` | Maximum render FPS. `0` (or less) = **unlimited**; values below `30` are raised to `30`. |
| `render_scale` | float (0.25–1.0) | `1.0` | Internal render resolution scale. `1.0` = native; lower values render smaller and upscale (faster on weak GPUs). |
| `antialiasing` | bool | `true` | Post-process anti-aliasing. |
| `crt_effect` | bool | `false` | CRT-style scanline post-process filter. |

### Detail toggles

Each turns a piece of visual detail on/off. All are `bool`, default `true`.

| Key | What it does |
|-----|--------------|
| `detail_shadows` | Character / object shadows. |
| `detail_puddles` | Wet-ground puddle reflections. |
| `detail_dirt` | Ground dirt / grime layer. |
| `detail_mist` | Atmospheric mist / fog. |
| `detail_rain` | Rain particles. |
| `detail_skyline` | Distant skyline rendering. |
| `detail_crinkles` | Terrain surface "crinkle" detail. |
| `detail_stars` | Night-sky stars. |
| `detail_moon_reflection` | Moon reflection on water. |
| `detail_people_reflection` | Pedestrian / character reflections. |
| `detail_filter` | Bilinear texture filtering. |
| `detail_perspective` | Perspective-correct texture mapping. |

## game

| Key | Type | Default | What it does | In menu? |
|-----|------|---------|--------------|----------|
| `scanner_follows` | bool | `false` | Radar orientation: `true` = radar rotates with the character's facing; `false` = rotates with the camera. | Yes |

## movie

| Key | Type | Default | What it does |
|-----|------|---------|--------------|
| `play_movie` | bool | `true` | Whether intro / cutscene videos play. |

## mouse

Mouse camera-look settings.

| Key | Type | Default | What it does |
|-----|------|---------|--------------|
| `camera_orbit_sensitivity` | float (0–1) | `0.4` | Mouse camera-rotation speed (low = slow, high = fast). |
| `camera_orbit_invert_y` | bool | `false` | Invert vertical mouse-look. |

## gamepad

Gamepad stick settings.

| Key | Type | Default | What it does |
|-----|------|---------|--------------|
| `gameplay_stick_deadzone` | float (0–1) | `0.25` | Movement / aim stick deadzone, as a fraction of full deflection. |
| `menu_stick_deadzone` | float (0–1) | `0.25` | Menu-navigation stick threshold (stops controller drift from auto-scrolling menus). |
| `camera_orbit_sensitivity` | float (0–1) | `0.4` | Gamepad camera-rotation speed. |
| `camera_orbit_invert_y` | bool | `false` | Invert vertical gamepad-look. |

---

## Language

The localization language is **not** part of this file — it is still read from the original game's `config.ini`, as in the retail game.
