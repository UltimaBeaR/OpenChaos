# config.json — девлог и план

Новая система конфига для OpenChaos.
Заменяет `config.ini` (оригинальный движок) и нынешний хардкоденый `env_default_config` в `env.cpp`.

## Статус

✅ **Базовая реализация завершена** (2026-04-27). Работает, но сыровато — нужны доработки перед 1.0.

- [x] Создан `oc_config.h` (API)
- [x] Написан `oc_config.cpp` — реализация (JSON r/w + миграция из INI)
- [x] Обновлён `env.cpp` — делегирует в oc_config
- [x] Обновлён `vcpkg.json` — добавлен nlohmann-json
- [x] Обновлён `CMakeLists.txt` — find_package + link + oc_config.cpp
- [x] Добавлен nlohmann/json в `THIRD_PARTY_LICENSES.md` и release packaging
- [x] Компилируется без ошибок
- [x] Все bool-поля в JSON теперь `true`/`false`; обработка `is_boolean()` в `OC_CONFIG_get_int()`
- [x] Нормализация при загрузке: старые 0/1 → конвертируются в bool, файл перезаписывается
- [x] Кривое значение → сброс на дефолт + сохранение
- [x] Type-preserving set: если поле хранилось как bool — так и сохраняется
- [x] Убраны `video_truecolour`, `Adami_lighting`, `max_frame_rate`, `language` из конфига
- [x] Секции объединены: `render` + `openchaos` → `video`
- [x] `fov_multiplier` → константа `OC_FOV_MULTIPLIER` в `config.h`
- [x] `language` → константа `OC_LANGUAGE_FILE` в `config.h`
- [x] Звуковой слайдер исправлен: `MFX_set_volumes` больше не пишет JSON каждый кадр; запись только при OK/ESC через `FRONTEND_storedata`
- [x] Громкость видеороликов следует за Volume слайдером (fx_volume)
- [x] Убраны Graphics и Joypad из меню Options
- [x] Joypad биндинги захардкоженны; `[Joypad]` секция не мигрируется и не читается
- [x] Отображение клавиш в меню Keyboard — всегда латинские имена (SDL_GetScancodeName + toupper)
- [ ] Обновить stages.md и README.md (публичный)
- [ ] Что-то не так с настройкой language при смене — см. known_issues_and_bugs.md

## Что ещё требует доработки (pending до 1.0)

- **Gamma** — `ge_is_gamma_available` возвращает false, меню яркости не работает
- **Language** — захардкожен английский, полноценная поддержка языков требует своих файлов локализации в `open_chaos/` (см. known_issues_and_bugs.md)
- **Обход ENV** — часть кода до сих пор читает через `ENV_get_value_number` напрямую, не через oc_config; нужно убедиться что все пути сохранения согласованы
- **Документация** — stages.md и public README.md не обновлены

## Технические решения (финальные)

**Файл:** `open_chaos/config.json` (рядом с exe, в папке `open_chaos/`)
**Формат:** чистый JSON, без комментариев
**Парсер:** nlohmann/json via vcpkg (`"nlohmann-json"` в vcpkg.json)
**Структура ключей:** вложенные объекты (`{"audio": {"ambient_volume": 127}}`)

### Поведение при СОЗДАНИИ файла (первый запуск, файла нет)

1. Инициализируем g_config со ВСЕМИ известными ключами и хардкоженными дефолтами
2. Пробуем прочитать config.ini — если значение там есть, переопределяем дефолт
3. Записываем config.json со ВСЕМИ ключами и их текущими значениями

### Поведение при ЗАГРУЗКЕ (файл существует)

- Читаем JSON в g_config
- Нормализуем known bool-поля: если хранились как 0/1 — конвертируем в true/false, перезаписываем файл
- Ключи которые есть в файле → используются из g_config
- Ключи которых нет в файле → берётся хардкоженный дефолт в коде (параметр `def`)
- Кривой тип значения → сброс на def, stderr, сохранение
- Отсутствующие ключи при загрузке **НЕ добавляются** в файл

### Поведение при СОХРАНЕНИИ (ENV_set_value_* через UI игры)

- Обновляется только конкретный ключ в g_config
- Если поле ранее хранилось как bool — сохраняется как bool (type-preserving)
- Если подраздела нет в g_config — создаётся автоматически
- Другие ключи/разделы **НЕ трогаются**
- Записывается вся g_config как есть

### config.h

`src/config.h` **не удалён** — переиспользован для compile-time констант которые не даём менять через UI:
- `OC_FOV_MULTIPLIER` — масштаб FOV (1.0 = оригинал)
- `OC_LANGUAGE_FILE` — путь к языковому файлу (захардкожен английский до реализации полной локализации)

## Структура JSON (финальная)

```json
{
  "audio": {
    "ambient_volume": 127,
    "music_volume": 127,
    "fx_volume": 127
  },
  "keyboard": {
    "left": 203, "right": 205,
    "forward": 200, "back": 208,
    "punch": 44, "kick": 45,
    "action": 46, "run": 47,
    "jump": 57, "start": 15,
    "select": 28, "camera": 207,
    "cam_left": 211, "cam_right": 209,
    "1stperson": 30
  },
  "video": {
    "detail_shadows": true, "detail_puddles": true, "detail_dirt": true,
    "detail_mist": true, "detail_rain": true, "detail_skyline": true,
    "detail_crinkles": true, "detail_stars": true,
    "detail_moon_reflection": true, "detail_people_reflection": true,
    "detail_filter": true, "detail_perspective": true,
    "fullscreen": true,
    "windowed_maximized": false,
    "windowed_width": 640,
    "windowed_height": 480,
    "vsync": true,
    "render_scale": 1.0,
    "antialiasing": true,
    "crt_effect": true
  },
  "game": {
    "scanner_follows": true
  },
  "movie": {
    "play_movie": true
  }
}
```

**Примечание по `fullscreen`:** дефолт `true` (fullscreen). Уже установлен для релиза 1.0 (согласно stage12.md).

## Что переносим из config.ini (при миграции на первый запуск)

| Секция config.ini | Ключи | Переносим? | Примечание |
|-------------------|-------|------------|------------|
| `[Audio]` | ambient_volume, music_volume, fx_volume | ✅ | Читается и записывается через меню Sound |
| `[Keyboard]` | все 15 биндингов | ✅ | DirectInput scan codes, совместимы |
| `[Joypad]` | все биндинги | ❌ | Несовместимые legacy индексы. Геймпад теперь хардкод в коде. |
| `[Render]` | detail_* | ✅ | Только в конфиге, НЕ в UI игры (секция video). draw_distance убран — AENG_set_draw_distance no-op. |
| `[Game]` | scanner_follows | ✅ | В меню Options |
| `[Game]` | language | ❌ | Захардкожен в config.h как OC_LANGUAGE_FILE |
| `[Game]` | iamapsx | ❌ | PS1 platform flag |
| `[Video]` | Mode, BitDepth | ❌ | Legacy D3D6 |
| `[Gamma]` | BlackPoint, WhitePoint | ❌ | Legacy D3D6 gamma |
| `[Movie]` | play_movie | ✅ | |
| `[LocalInstall]` | textures/sfx/speech/movies | ❌ | Install flags |

## Меню Options (финальное)

**Options / Sound / Keyboard** — Graphics и Joypad убраны.

- **Options** — scanner_follows и другие игровые настройки
- **Graphics** — убрано из меню; detail toggles только в config.json вручную
- **Sound** — оставлено ✅
- **Keyboard** — оставлено ✅
- **Joypad** — убрано полностью ✅

## Связанные файлы

- [oc_config.h](../../new_game/src/engine/io/oc_config.h) — публичный API
- [oc_config.cpp](../../new_game/src/engine/io/oc_config.cpp) — реализация
- [config.h](../../new_game/src/config.h) — compile-time константы (OC_FOV_MULTIPLIER, OC_LANGUAGE_FILE)
- [env.cpp](../../new_game/src/engine/io/env.cpp) — делегирует в oc_config
- [frontend.cpp](../../new_game/src/ui/frontend/frontend.cpp) — UI меню Options
- [input_actions.cpp](../../new_game/src/game/input_actions.cpp) — чтение биндингов
- [stage12.md](../../new_game_planning/stage12.md) — конфиг в критериях релиза 1.0
