# Этап 4.2 — Таблица перемещений

Текущий путь → новый путь (относительно `src/`).
Обновляется по мере принятия решений о перемещениях.

| Текущий путь | Новый путь | Причина |
|---|---|---|
| `core/*` | `engine/core/*` | engine — самодостаточный движок-комбайн, core его фундамент |
| `platform/*` | `engine/platform/*` | платформенная абстракция — часть движка |
| `engine/graphics/graphics_api/wind_procs.*` | `engine/platform/wind_procs.*` | Win32 WndProc — platform, не graphics |
| `actors/*` | `things/*` | Thing — оригинальное название системы в коде, точнее чем "actors" |
| `engine/platform/platform.h` | `engine/platform/uc_common.h` | Не платформенная абстракция, а легаси umbrella (бывший MFStdLib.h) |
| `engine/lighting/*` | `engine/graphics/lighting/*` | Чисто визуальная подсистема — к графике |
| `engine/graphics/geometry/bloom.*` | `engine/graphics/postprocess/bloom.*` | Постобработка, не геометрия |
| `engine/graphics/pipeline/wibble.*` | `engine/graphics/postprocess/wibble.*` | Постобработка, не геометрия |
| `engine/graphics/pipeline/text.*` | `engine/graphics/text/text.*` | Текстовый рендерер, к шрифтам |
| `engine/graphics/resources/font*` | `engine/graphics/text/font*` | Шрифты → text/ |
| `engine/graphics/resources/menufont*` | `engine/graphics/text/menufont*` | Шрифты → text/ |
| `engine/graphics/resources/truetype*` | `engine/graphics/text/truetype*` | Шрифты → text/ |
| `engine/graphics/resources/console.*` | `engine/console/console.*` | Дебаг-подсистема движка, не графика |
| `engine/graphics/pipeline/message.*` | `engine/console/message.*` | Дебаг текстовый вывод |
| `engine/graphics/pipeline/sky.*` | `engine/graphics/geometry/sky.*` | Создаёт визуальный вывод |
| `engine/graphics/pipeline/vertex_buffer.*` | `engine/graphics/graphics_api/vertex_buffer.*` | Чистый D3D |
| `engine/graphics/pipeline/render_state.*` | `engine/graphics/graphics_api/render_state.*` | Чистый D3D |
| `engine/graphics/resources/d3d_texture.*` | `engine/graphics/graphics_api/d3d_texture.*` | Чистый D3D |
| `engine/graphics/graphics_api/host.*` | `engine/platform/host.*` | Создание окна, не graphics API |
| `engine/animation/figure.*` | `engine/graphics/geometry/figure.*` | D3D рендерер мешей, не анимация |
| `missions/main.cpp` | `game/startup.cpp` | Точка входа игры, не миссия |
| `missions/game.*` | `game/game.*` | Game loop, не миссия |
| `missions/game_types.h` | `game/game_types.h` | Game struct, не миссия |
| `missions/game_globals.*` | `game/game_globals.*` | Globals для game |
| `ui/controls.*` | `game/game_tick.*` | Per-frame game dispatcher (переименован) |
| `ui/interfac.*` | `game/input_actions.*` | Input→action mapping (переименован) |
| `ui/cutscenes/playcuts.*` | `missions/playcuts.*` | Кат-сцены вызываются из EWAY |
| `ui/cutscenes/outro/* (движок)` | `outro/core/*` | Движок титров |
| `ui/cutscenes/outro/* (контент)` | `outro/*` | Контент титров |
| `world/environment/edmap.h` | `world/map/map_constants.h` | Map constants, не editor (переименован) |
| `effects/*` | `effects/{weather,combat,environment}/*` | Группировка эффектов по типу |
| `ui/menus/cnet.*` | **удалён** | Сетевая заглушка. CNET_* globals → game/network_state_globals |
| `world/map/*` | `map/*` | Top-level модуль |
| `world/level_pools.h` | `map/level_pools.h` | Пулы геометрии, часть карты |
| `world/navigation/*` | `navigation/*` | Top-level модуль |
| `world/environment/` (здания) | `buildings/*` | Геометрия зданий |
| `world/environment/` (объекты) | `world_objects/*` | Мировые объекты |
| `world/environment/ns.*` | `map/sewers.*` | Канализация = слой карты, переименование |
| `ui/camera/*` | `camera/*` | Камера — не UI |
| `assets/` (загрузчики) | `assets/formats/*` | Парсеры форматов в подпапку |
| `missions/elev.*` | `assets/formats/elev.*` | Загрузчик .ucm |
| `assets/compression.*` | `engine/compression/*` | Универсальные алгоритмы |
| `assets/image_compression.*` | `engine/compression/*` | Универсальные алгоритмы |
| `world_objects/ware.*` | `buildings/ware.*` | Внутренние помещения |
| `effects/environment/dirt.*` | `world_objects/dirt.*` | Не эффект, интерактивная подсистема |
| `ui/frontend.*` | `ui/frontend/frontend.*` | Подпапка для фронтенда |
| `ui/attract.*` | `ui/frontend/attract.*` | Связан с frontend |
| `ui/pause.*` | `ui/menus/pause.*` | Рядом с gamemenu |
| `ai/combat.*` | `combat/combat.*` | Общая боевая система, не только AI |
| `things/items/guns.*` | `shooting/guns.*` | Система стрельбы, не item |
| `things/items/projectile.*` | `shooting/projectile.*` | Снаряды, часть стрельбы |
| `assets/startscr.*` | `ui/frontend/startscr.*` | Данные фронтенда, не ресурс |
