# Загрузка уровня (Level Loading Pipeline)

## Обзор

Загрузка уровня — многоступенчатый процесс, инициируемый из `game_init()` (Game.cpp:546). Полный путь: миссионный файл → карта → освещение → эвент-вэйпойнты → создание игрока и NPC.

Основные файлы: `fallen/Source/elev.cpp` (главная логика), `fallen/Source/io.cpp` (загрузка карты), `fallen/Source/eway.cpp` (эвент-вэйпойнты).

## Форматы файлов уровня

| Расширение | Назначение | Загрузчик |
|-----------|-----------|----------|
| `.ucm` | Миссия: метаданные + эвент-вэйпойнты (содержит пути к остальным файлам) | `ELEV_load_name()` |
| `.iam` | Карта: PAP-сетка + объекты + анимированные примы | `load_game_map()` |
| `.lgt` | Освещение: дневное/ночное, динамические источники | `NIGHT_load_ed_file()` |
| `.txt` | Диалоги горожан (citsez/sewer) | `EWAY_load_fake_wander_text()` |
| `.prm` | Меши примов (загружаются по мере необходимости) | `load_prim_object()` |

## Полная последовательность загрузки

### Шаг 1: `ELEV_load_name(fname)` (elev.cpp:2717)

- Открывает `.ucm` файл
- Читает 4 внутренних имени файлов: `fname_map`, `fname_lighting`, `fname_citsez`, `fname_level`
- Передаёт управление `ELEV_game_init()`

### Шаг 2: `ELEV_game_init()` — инициализация подсистем (elev.cpp:2160)

Вызовы в порядке:
```
FC_init()           // камера
BIKE_init()         // велосипеды
BARREL_init()       // бочки
BALLOON_init()      // шары
NIGHT_init()        // система освещения
OB_init()           // объекты на карте (фонари и т.д.)
TRIP_init()         // tripwires
FOG_init()          // туман
MIST_init()         // дымка
PUDDLE_init()       // лужи
DRIP_init()         // капли
GLITTER_init()      // частицы блёсток
POW_init()          // взрывы
SPARK_init()        // искры
load_animtmaps()    // текстурные карты анимаций
```

### Шаг 3: `load_game_map(fname_map)` — загрузка карты (io.cpp:617)

Последовательность:
1. Открыть `.iam` файл
2. **Читать PAP_Hi** (128×128 = 16384 ячейки, 6 байт каждая): высота, флаги коллизий, проходимость
3. **Читать rooftop texture mapping** (PSX-специфично)
4. **Сбросить PAP_Lo mapwho** данные (32×32)
5. **Загрузить анимированные примы**: читать count, для каждого — LoadGameThing → `load_anim_prim_object()` + `create_anim_prim()`
6. **Загрузить OB (статические объекты)**: `OB_ob_upto`, `OB_ob[]` (фонари, урны и т.д.), `OB_mapwho[][]`
7. **Загрузить super map** (низкое разрешение)
8. **Загрузить нужные примы**: `OB_load_needed_prims()`, `load_needed_anim_prims()` — читает `.prm` файлы
9. **Читать texture_set** → `TEXTURE_choose_set(0-21)` (определяет набор текстур города)
10. **Читать PSX texture remapping** (если version >= 25)

### Шаг 4: Постобработка карты (elev.cpp:2312–2357)

```
calc_prim_info()              // границы мешей
build_quick_city()            // пространственное ускорение (mapwho)
init_animals()                // модели животных
DIRT_init()                   // листья/грязь вокруг деревьев
OB_convert_dustbins_to_barrels()
ROAD_sink()                   // обработка бордюров дорог
ROAD_wander_calc()            // маршруты блуждания NPC
WAND_init()                   // зоны блуждания
WARE_init()                   // данные складов/крыш
MAV_precalculate()            // навигационный граф MAV (тяжёлая операция!)
BUILD_car_facets()            // коллизии для машин
SHADOW_do()                   // предрасчёт теней
COLLIDE_calc_fastnav_bits()   // навигационная сетка коллизий
COLLIDE_find_seethrough_fences() // прозрачные заборы
clear_all_wmove_flags()
InitGrenades()
SOUNDENV_precalc()            // аудио-полигоны (A3D)
SOUND_SewerPrecalc()          // аудио для канализации
```

**MAV_precalculate()** — самая тяжёлая операция: для каждой из 128×128 ячеек вычисляет доступные направления движения (ходьба, прыжок, подтягивание, лестница, падение), результат кэшируется в `MAV_opt[]` и `MAV_nav[]`.

### Шаг 5: Загрузка освещения

```cpp
if (!NIGHT_load_ed_file(fname_lighting))
    NIGHT_ambient(255,255,255, 110,-148,-177);  // дефолт (дневной свет)

NIGHT_generate_walkable_lighting();  // освещение на проходимых поверхностях

if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME))
    GAME_FLAGS |= GF_RAINING;  // ночью всегда идёт дождь
```

### Шаг 6: `ELEV_load_level(fname_level)` — загрузка эвент-вэйпойнтов (elev.cpp:193)

1. Сброс: `GAME_TURN = 0`, `SAVE_VALID = 0`, `EWAY_init()`
2. Чтение метаданных миссии: версия, имена, crime rate, количество фейковых горожан
3. Создание фейковых горожан: `PCOM_create_person()` для блуждающих гражданских
4. **Цикл загрузки эвент-вэйпойнтов** (до MAX_EVENTPOINTS = 512):
   - Для каждого: 14 байт заголовка + 58 байт данных
   - Конвертация в `EWAY_Way`: условие (EWAY_Conddef), действие (EWAY_Do), длительность (EWAY_Stay)
5. Обработка уровня воды: флаги PAP_FLAG_WATER, PAP_FLAG_SINK_POINT
6. Данные зон: PAP_FLAG_ZONE1/2/3/4

### Шаг 7: Финальная обработка (elev.cpp:2413–2591)

```
calc_prim_info()           // пересчёт после загрузки всех примов
calc_prim_normals()        // нормали к граням (PC)
find_anim_prim_bboxes()    // bounding boxes

TEXTURE_load_needed(...)   // загрузка текстур по требованию

EWAY_process()             // КЛЮЧЕВОЙ ВЫЗОВ — создаёт игрока и NPC!

ELEV_game_init_common()    // wav-список, фоновое аудио

EWAY_load_fake_wander_text(fname_citsez)   // диалоги горожан
OB_make_all_the_switches_be_at_the_proper_height()
OB_add_walkable_faces()    // проходимые поверхности → коллизии
calc_slide_edges()         // рёбра для хвата (карниз, захват)
```

### Шаг 8: Настройка камеры (elev.cpp:2478–2541)

```cpp
for (i = 0; i < NO_PLAYERS; i++) {
    if (NET_PERSON(i)) {
        FC_look_at(i, THING_NUMBER(NET_PERSON(i)));
        FC_setup_initial_camera(i);
    }
}
```

### Шаг 9: Спецэффекты и финал (elev.cpp:2559–2591)

```
// Проигрыш boombox звуков
if (GAME_FLAGS & GF_RAINING) PUDDLE_precalculate()
insert_collision_facets()  // коллизии у воды/канализации
```

После этого управление возвращается в `game_init()` → запускается `game_loop()`. Игрок может двигаться.

## Создание игрока и NPC

**Через `EWAY_process()`** — обрабатывает все активные эвент-вэйпойнты:

- `WPT_CREATE_PLAYER` → `EWAY_create_player()` → `create_player(type, x, y, z, player_id)`
  - Создаёт Thing типа Person (не Player!) в позиции вэйпойнта
- `WPT_CREATE_ENEMIES` → `EWAY_create_enemy()` → `PCOM_create_person(person_type, pcom_ai, pcom_bent, pcom_move, pcom_has, ..., x, y, z, flags2)`

Нет альтернативной системы `.lev` (она Legacy/неиспользуемая).

## Структура EventPoint (миссионный файл)

```
Version (4 bytes)
BriefName, LightMapName, MapName, MissionName, CitSezMapName (_MAX_PATH bytes each)
MapIndex (2), Used (2), Free (2)
CrimeRate (1), Padding (1)
EventPoint array[MAX_EVENTPOINTS]:
  - 14 bytes header (флаги, тип вэйпойнта, тип триггера, X/Y/Z, Radius, Color, Group)
  - 58 bytes data (Data[5], EPRef, EPRefBool, и др.)
ZoneData[128][128] (optional)
```

## Порядок загрузки ресурсов

1. `.ucm` — извлечь имена файлов
2. `.iam` — PAP + объекты + анимированные примы
3. `.prm` — по требованию при загрузке `.iam`
4. `.lgt` — освещение
5. `.txt` — диалоги NPC
6. Эвент-вэйпойнты — из `.ucm` (часть 2)
7. Текстуры (`.tga`/`.txc`) — по требованию во время `TEXTURE_load_needed()`

## Кеш-система (Cache.cpp)

- 128-элементный кеш для динамических данных
- `CACHE_create(key, data, num_bytes)` — сохранить
- `CACHE_invalidate()` — освободить память
- `CACHE_flag/unflag` — пометить что сохранять при смене уровня

## Важно для переноса

- `MAV_precalculate()` — тяжёлая оффлайн-операция; при переносе либо запускать при загрузке, либо кешировать на диск
- Игрок создаётся через тот же `PCOM_create_person()` что и NPC, просто с другим `player_id`
- `GAME_TURN = 0` сбрасывается при каждой загрузке уровня — важно для таймеров
- Ночью всегда дождь (хардкод в движке) — намеренное дизайнерское решение
