# config.json — девлог и план

Новая система конфига для OpenChaos.
Заменяет `config.ini` (оригинальный движок) и нынешний хардкоденый `env_default_config` в `env.cpp`.

## Статус

🔧 **В РАБОТЕ** (начата реализация 2026-04-27)

- [x] Создан `oc_config.h` (API)
- [x] Написать `oc_config.cpp` — реализация (JSON r/w + миграция из INI)
- [x] Обновить `env.cpp` — делегировать в oc_config
- [x] Обновить `vcpkg.json` — добавить nlohmann-json
- [x] Обновить `CMakeLists.txt` — find_package + link + oc_config.cpp
- [x] Обновить все `.cpp` использующие `config.h`: core.cpp, aeng.cpp, poly.cpp, composition.cpp, crt_effect.cpp, host.cpp
- [x] Добавить nlohmann/json в `THIRD_PARTY_LICENSES.md` и в release packaging
- [ ] **Убедиться что компилируется** (make configure + make build-debug — нужен vcpkg download nlohmann)
- [ ] **УДАЛИТЬ `src/config.h`** — сейчас он опустошён (только комментарий), но нужно удалить файл целиком. Все `#include "config.h"` уже убраны.
- [ ] **True/false вместо 0/1** — в `oc_config.cpp` все булевые значения (fullscreen, vsync, aa_enable, crt_enable, scanner_follows, detail_*, play_movie и т.д.) хранятся как JSON числа 0/1. Нужно переделать на нативные JSON bool (true/false). Изменения в `oc_config.cpp`: (1) в `build_defaults_and_migrate()` заменить `1`/`0` на `true`/`false` для всех bool-полей; (2) в `OC_CONFIG_get_int()` добавить обработку `is_boolean()` → `jt->get<bool>() ? 1 : 0`; (3) в `OC_CONFIG_get_float()` — аналогично если нужно. Числовые (не bool) остаются числами: draw_distance, windowed_width/height, max_frame_rate, volume (0-127), scan codes клавиатуры, fov_multiplier, render_scale.
- [ ] **Убрать `video_truecolour` из конфига** — это legacy D3D6 настройка (16/32-bit color depth), не нужна в OpenGL. Убрать из `build_defaults_and_migrate()` в oc_config.cpp (из секции "render"), и из миграции.
- [ ] **Звуковой слайдер — проверить поведение:** сейчас `ENV_set_value_number` вызывается из `mfx.cpp` (~строки 732-734) при изменении громкости, что означает немедленное сохранение в config.json при каждом движении слайдера. **Нужно проверить** когда именно вызывается эта функция — при каждом тике слайдера или только при нажатии OK в меню. Если при каждом тике — это может быть много записей на диск. В оригинальной игре запись тоже происходила при каждом вызове ENV_set_value_number (запись в файл). Сравнить с оригинальным поведением и решить нужна ли буферизация.
- [ ] Убрать из меню игры: Graphics, Joypad (оставить только Options, Sound, Keyboard) — `frontend.cpp`
- [ ] Убрать из кода всё связанное с `[Joypad]` через ENV в `input_actions.cpp` и `frontend.cpp`
- [ ] Обновить документацию (stages.md, README.md)

## Технические решения (финальные)

**Файл:** `openchaos/config.json` (рядом с exe, в папке `openchaos/`)
**Формат:** чистый JSON, без комментариев
**Парсер:** nlohmann/json via vcpkg (`"nlohmann-json"` в vcpkg.json)
**Структура ключей:** вложенные объекты (`{"audio": {"ambient_volume": 127}}`)

### Поведение при СОЗДАНИИ файла (первый запуск, файла нет)

1. Инициализируем g_config со ВСЕМИ известными ключами и хардкоженными дефолтами
2. Пробуем прочитать config.ini — если значение там есть, переопределяем дефолт
3. Записываем config.json со ВСЕМИ ключами и их текущими значениями

### Поведение при ЗАГРУЗКЕ (файл существует)

- Читаем JSON в g_config
- Ключи которые есть в файле → используются из g_config
- Ключи которых нет в файле → берётся хардкоженный дефолт в коде (параметр `def`)
- Отсутствующие ключи при загрузке **НЕ добавляются** в файл

### Поведение при СОХРАНЕНИИ (ENV_set_value_* через UI игры)

- Обновляется только конкретный ключ в g_config
- Если подраздела нет в g_config — создаётся автоматически
- Другие ключи/разделы **НЕ трогаются**
- Записывается вся g_config как есть (только то что было загружено + что изменили)

Это значит: если юзер вручную удалил ключ из config.json, он не вернётся сам по себе. Вернётся только если юзер явно изменит эту настройку через UI.

Поведение реализуется естественно через nlohmann: g_config содержит только то что было в файле, `g_config[section][key] = val` создаёт раздел если нужно, `dump(4)` пишет только то что в памяти.

## Структура JSON (полная)

```json
{
  "audio": {
    "ambient_volume": 127,
    "music_volume": 127,
    "fx_volume": 127
  },
  "keyboard": {
    "keyboard_left": 203, "keyboard_right": 205,
    "keyboard_forward": 200, "keyboard_back": 208,
    "keyboard_punch": 44, "keyboard_kick": 45,
    "keyboard_action": 46, "keyboard_run": 47,
    "keyboard_jump": 57, "keyboard_start": 15,
    "keyboard_select": 28, "keyboard_camera": 207,
    "keyboard_cam_left": 211, "keyboard_cam_right": 209,
    "keyboard_1stperson": 30
  },
  "render": {
    "detail_shadows": 1, "detail_puddles": 1, "detail_dirt": 1,
    "detail_mist": 1, "detail_rain": 1, "detail_skyline": 1,
    "detail_crinkles": 1, "detail_stars": 1,
    "detail_moon_reflection": 1, "detail_people_reflection": 1,
    "detail_filter": 1, "detail_perspective": 1,
    "Adami_lighting": 1, "video_truecolour": 1,
    "draw_distance": 22, "max_frame_rate": 30
  },
  "game": {
    "scanner_follows": 1,
    "language": "text\\lang_english.txt"
  },
  "movie": {
    "play_movie": 1
  },
  "openchaos": {
    "fullscreen": 0,
    "windowed_width": 640,
    "windowed_height": 480,
    "vsync": 1,
    "render_scale": 1.0,
    "aa_enable": 1,
    "crt_enable": 1,
    "fov_multiplier": 1.0
  }
}
```

**Примечание по `fullscreen`:** дефолт `0` (windowed) — это текущее поведение из `config.h`. При релизе 1.0 сменить на `1` (согласно stage12.md).

## Что переносим из config.ini (при миграции на первый запуск)

| Секция config.ini | Ключи | Переносим? | Примечание |
|-------------------|-------|------------|------------|
| `[Audio]` | ambient_volume, music_volume, fx_volume | ✅ | Читается и записывается через меню Sound |
| `[Keyboard]` | все 15 биндингов | ✅ | DirectInput scan codes, совместимы |
| `[Joypad]` | все биндинги | ❌ | Несовместимые legacy индексы. Геймпад теперь хардкод в коде. |
| `[Render]` | detail_* + draw_distance + прочее | ✅ | Только в конфиге, НЕ в UI игры |
| `[Game]` | scanner_follows, language | ✅ | scanner_follows — в меню Options |
| `[Game]` | iamapsx | ❌ | PS1 platform flag |
| `[Video]` | Mode, BitDepth | ❌ | Legacy D3D6 |
| `[Gamma]` | BlackPoint, WhitePoint | ❌ | Legacy D3D6 gamma |
| `[Movie]` | play_movie | ✅ | |
| `[LocalInstall]` | textures/sfx/speech/movies | ❌ | Install flags |
| `[TextureClumps]` | enable_clumps | ❌ | |
| `[Secret]` | clumps | ❌ | Dev flag (make_all_clumps) |
| **Новые** (openchaos секция) | fullscreen, windowed_width, windowed_height, vsync, render_scale, aa_enable, crt_enable, fov_multiplier | — | Только хардкод-дефолты, config.ini не знает об этих |

## Что убираем из UI игры

Меню Options сейчас: **Options / Graphics / Sound / Keyboard / Joypad**

- **Options** — оставляем (там scanner_follows и другие игровые настройки)
- **Graphics** — убираем из меню (detail toggles теперь только в config.json вручную)
- **Sound** — оставляем ✅
- **Keyboard** — оставляем ✅
- **Joypad** — убираем полностью ✅

## Что убираем из кода (Joypad ENV)

- `frontend.cpp` — убрать блок с `ENV_set_value_number("joypad_*", ...)` (~строки 2452–2463)
- `input_actions.cpp` — убрать `ENV_get_value_number` для `joypad_*` в `init_joypad_config()`, оставить только хардкоденые SDL3 индексы (они уже там есть)
- `env_default_config` в `env.cpp` — полностью удалить (заменяется на oc_config)

## config.h → УДАЛИТЬ

Все настройки из `src/config.h` переехали в секцию `"openchaos"` config.json.
Файл config.h удалить, убрать `#include "config.h"` из всех файлов, добавить `#include "engine/io/oc_config.h"`.

**Файлы где нужно менять include:**
- `src/engine/platform/host.cpp` — OC_WINDOWED_WIDTH, OC_WINDOWED_HEIGHT, OC_FULLSCREEN
- `src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp` — OC_RENDER_SCALE, OC_WINDOWED_*, OC_FULLSCREEN, OC_VSYNC
- `src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp` — OC_AA_ENABLE
- `src/engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.cpp` — OC_CRT_ENABLE (global init)
- `src/engine/graphics/pipeline/aeng.cpp` — OC_FOV_MULTIPLIER (2 места)
- `src/engine/graphics/pipeline/poly.cpp` — OC_FOV_MULTIPLIER

**Замены:**
- `OC_FULLSCREEN` → `OC_CONFIG_get_int("openchaos", "fullscreen", 0) != 0`
- `OC_WINDOWED_WIDTH` → `OC_CONFIG_get_int("openchaos", "windowed_width", 640)`
- `OC_WINDOWED_HEIGHT` → `OC_CONFIG_get_int("openchaos", "windowed_height", 480)`
- `OC_VSYNC` → `OC_CONFIG_get_int("openchaos", "vsync", 1) != 0`
- `OC_RENDER_SCALE` → `OC_CONFIG_get_float("openchaos", "render_scale", 1.0f)`
- `OC_FOV_MULTIPLIER` → `OC_CONFIG_get_float("openchaos", "fov_multiplier", 1.0f)`
- `OC_AA_ENABLE` → cached static bool в composition, init в `composition_init()`
- `OC_CRT_ENABLE` в global init → `bool g_crt_enabled = true;` + `crt_effect_init()` вызывается из `composition_init()`

## Файлы которые нужно создать/изменить

### Новые файлы
- `new_game/src/engine/io/oc_config.h` ✅ (создан)
- `new_game/src/engine/io/oc_config.cpp` — реализация

### Изменяемые файлы
- `new_game/vcpkg.json` — добавить `"nlohmann-json"`
- `new_game/CMakeLists.txt` — find_package + link + source
- `new_game/src/engine/io/env.cpp` — делегировать в oc_config, удалить env_default_config
- `new_game/src/config.h` — удалить / опустошить
- `new_game/src/engine/platform/host.cpp`
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp`
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp`
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.h` — добавить `void crt_effect_init();`
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.cpp`
- `new_game/src/engine/graphics/pipeline/aeng.cpp`
- `new_game/src/engine/graphics/pipeline/poly.cpp`
- `new_game/src/ui/frontend/frontend.cpp` — убрать Graphics и Joypad из меню, убрать joypad ENV_set блоки
- `new_game/src/game/input_actions.cpp` — убрать ENV_get для joypad в init_joypad_config()
- `THIRD_PARTY_LICENSES.md` — добавить nlohmann/json
- `release/release.mk` — добавить THIRD_PARTY_LICENSES.md в пакет

## Связанные файлы

- [env.cpp](../../new_game/src/engine/io/env.cpp) — текущий хардкоденый конфиг
- [config.h](../../new_game/src/config.h) — текущие OC_ defines (удаляем)
- [frontend.cpp](../../new_game/src/ui/frontend/frontend.cpp) — UI меню Options
- [input_actions.cpp](../../new_game/src/game/input_actions.cpp) — чтение биндингов
- [stage12.md](../../new_game_planning/stage12.md) — конфиг в критериях релиза 1.0
