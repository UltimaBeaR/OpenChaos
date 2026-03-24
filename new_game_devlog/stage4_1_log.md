# Лог Этапа 4.1 — Удаление мёртвого кода

План и правила: [`new_game_planning/stage4.md`](../new_game_planning/stage4.md) → секция 4.1

---

## Шаг 1, итерация 1 — Удаление целых файлов (2026-03-24)

Анализ файлов по названиям + сверка с KB + подтверждение пользователем.
Удалено **25 файлов**, скомпилировано, проверено в рантайме.

### Удалённые файлы

| Группа | Файлы | Причина |
|--------|-------|---------|
| `assets/sound_id.cpp` | 1 | Пустой файл, не в CMakeLists |
| `actors/animals/lead.h`, `lead_globals.h` | 2 | Неиспользуемые хедеры, нет .cpp |
| `world/map/qedit.*` | 4 | Редактор карт, `QEDIT_loop()` нигде не вызывается |
| `actors/items/dike.*` | 4 | Вырезанная физика мотоцикла (BIKE), вызовы только в закомментированном коде |
| `actors/animals/canid.*` | 4 | Собаки — dispatch закомментирован в оригинале, нет внешних ссылок |
| `engine/io/drive.*` | 4 | Функции путей к CD-ROM (`GetTexturePath()` и т.д. — runtime-пути к ресурсам, которые могли указывать на CD). Вызовы в 5 файлах заменены на `".\\"`  (текущая директория) |
| `assets/bink_client.*` | 2 | Bink video. `PlayQuickMovie()` оставлен как пустой stub в display.cpp |
| `engine/net/*` | 4 | Сетевой код (все функции были пустыми стабами). Вызовы убраны, `CNET_configure` заглушен |

### Правки в вызывающем коде

- **drive:** `missions/main.cpp` — убран `LocateCDROM()`. `texture.cpp`, `mfx.cpp`, `eway.cpp`, `frontend.cpp` — runtime-пути к ресурсам (через `GetTexturePath()` и др.) заменены на `".\\"`  (текущая директория, CD-ROM не нужен)
- **bink:** `wind_procs.cpp` — убран `BinkMessage()`. `display.cpp` — `PlayQuickMovie` → пустой stub, убраны FMV defines и `mirror` переменная
- **net:** `game.cpp` — убран `NET_kill()`. `attract.cpp` — убран `NET_init()`. `cnet.cpp` — `CNET_configure()` → stub (return FALSE). `thing.cpp` — убрана сетевая ветка в `do_packets()`
- **canid:** `animal_globals.h` — убран extern `CANID_state_function[]`. `animal_globals.cpp` — убрана запись из `ANIMAL_functions[]`
- **qedit:** `game.cpp` — убраны includes
- **CMakeLists.txt:** убраны все удалённые .cpp + дубликат bat.cpp/bat_globals.cpp (был дважды)

### Проверено но НЕ удалено (оказалось в использовании)

bat (летучая мышь — 7+ файлов ссылаются), balloon (loading screens, cutscenes), truetype (рендеринг текста),
chopper (полноценный вертолёт с AI и рендерингом), snipe (режим прицела), attract (система loading screens),
furn.h (struct Furniture встроена в Thing), startscr (коды действий меню), cloth.h (симуляция ткани в controls.cpp)

---

## Шаг 2 — Compiler warnings (2026-03-24)

Временно добавлены флаги в CMakeLists.txt:
`-Wunused-function`, `-Wunused-variable`, `-Wunused-but-set-variable`, `-Wunreachable-code`

Результат первого прогона: **1454 warnings** (1106 unused-variable, 269 unused-but-set, 47 unreachable, 32 unused-function).

### Шаг 2a — Удаление unused functions

Удалено **34 static функции** (32 из первого прогона + 2 каскадных) из 23 файлов.
После первого прогона удалений обнаружился каскад: `calc_win` (вызывалась только из удалённой `calc_prob`)
и `lerp_long` (вызывалась только из удалённой `lerp_vector`).

Также удалены осиротевшие forward declarations (агенты удалили тела функций, но оставили decls —
`RenderStreamToSurface`, `RenderFileToMMStream`, `FRONTEND_shadowed_text`, `SAVE_open`, `PYRO_draw_blob`).

Каскадный эффект: после двух прогонов unused-function warnings = 0.

Удалённые функции по файлам:

| Файл | Функции |
|------|---------|
| `actors/core/interact.cpp` | `valid_grab_angle` |
| `assets/texture.cpp` | `TEXTURE_DC_pack_load_page` (Dreamcast) |
| `assets/xlat_str.cpp` | `mbcs_dec_char`, `mbcs_strncpy` |
| `effects/pyro.cpp` | `lerp_vector`, `lerp_long` (каскад), `PYRO_draw_blob` |
| `effects/tracks.cpp` | `RShift8` |
| `engine/audio/soundenv.cpp` | `SOUNDENV_CB_fn` (typedef), `SOUNDENV_ClearRange`, `SOUNDENV_Quadify`, `cback` (цепочка) |
| `engine/graphics/graphics_api/display.cpp` | `RenderStreamToSurface`, `RenderFileToMMStream` (DirectShow) |
| `engine/graphics/graphics_api/host.cpp` | `DDLibThread` |
| `engine/graphics/pipeline/aeng.cpp` | `add_debug_line` |
| `engine/graphics/resources/menufont.cpp` | `Bloo` |
| `engine/graphics/resources/truetype.cpp` | `ShowDebug` (fwd decl), `ShowTextures` |
| `engine/lighting/crinkle.cpp` | `DontDoAnythingWithThis` |
| `engine/lighting/smap.cpp` | `SMAP_prim_points` |
| `engine/physics/hm.cpp` | `qdist`, `HM_height_at` |
| `missions/save.cpp` | `SAVE_open` |
| `ui/attract.cpp` | `printf2d` |
| `ui/cutscenes/playcuts.cpp` | `PLAYCUTS_Free_Chan` |
| `ui/frontend.cpp` | `FRONTEND_shadowed_text` |
| `world/environment/building.cpp` | `calc_win` (каскад), `calc_prob` |
| `world/environment/door.cpp` | `DOOR_knock_over_people` |
| `world/environment/stair.cpp` | `STAIR_grand`, `STAIR_check_edge` |
| `world/map/ob.cpp` | `OB_height_fiddle_de_dee` |
| `world/map/pap.cpp` | `PAP_assert_if_off_map_lo`, `PAP_assert_if_off_map_hi` |
| `world/map/qmap.cpp` | `QMAP_get_style_texture` |

### Шаг 2b — Удаление unreachable code

Удалён мёртвый код из 42 мест в ~30 файлах. Категории:

- **Code after return** (22): функции со стабами `ASSERT(0); return;` и мёртвым телом после return —
  удалены тела, стабы оставлены. Файлы: animal.cpp, vehicle.cpp, texture.cpp, host.cpp, aeng.cpp,
  console.cpp, crinkle.cpp, night.cpp, collide.cpp, eway.cpp, outro_os.cpp, building.cpp
- **if(0)/if(1) dead branches** (16): удалены мёртвые ветки. Файлы: person.cpp, vehicle.cpp,
  figure.cpp, facet.cpp, mesh.cpp, aeng.cpp, collide.cpp, eway.cpp, fc.cpp, interfac.cpp, building.cpp
- **Dead case/break** (2): person.cpp (copy-paste артефакт), elev.cpp (дубликат return)
- **Unreachable error blocks** (2): elev.cpp

Оставлено **5 warnings** (осознанно):
- `outro_mf.cpp:218,334` — код после ASSERT(0), документирует инвариант
- `building.cpp:1962` — декларации в switch перед первым case (false positive)
- `async_file.cpp:160` — for-loop всегда return на первой итерации (структурный паттерн)

### Шаг 2c — Удаление unused variables

Массовая чистка: **~1400 неиспользуемых переменных** удалены из ~80 файлов.

Паттерны:
- Переменные объявленные но не используемые (C-стиль: всё объявлялось наверху функции)
- set-but-not-used: переменные вычисленные но нигде не прочитанные. Где RHS имел side effects
  (D3D вызовы, FileRead, и т.п.) — вызов сохранён как standalone expression
- Каскадные удаления: удаление diff-loop в texture.cpp после удаления diff_r/g/b

Оставлено **27 warnings** — все ASSERT-only переменные (используются только внутри ASSERT(),
который в Release сборке раскрывается в пустое выражение). Примеры: `hres`, `res` (D3D error checks),
`dwRead` (file read verification), `wmove_index`, `pFirstStripIndex` (index bounds checks).

**Итог Шага 2:** warning-флаги убраны из CMakeLists.txt. Сборка чистая.
98 файлов изменено, ~3340 строк удалено. Рантайм проверен пользователем.

### Нюансы по процессу

- **Каскадные прогоны обязательны:** после удаления unused functions/variables нужно пересобирать
  и проверять заново — удаление одной сущности делает зависимые тоже мёртвыми. Пример:
  удалили `calc_prob` → `calc_win` стала unused (каскад), удалили `lerp_vector` → `lerp_long` unused.
  Для `-Wunused-function` каскад ловится в пределах одного .cpp (static functions).
  Для non-static — каскад ловится на Шаге 3 (grep по проекту).
- **Шаг 4 (if(0), unreachable code) выполнен досрочно** — в рамках Шага 2 через `-Wunreachable-code`.
  В плане Шаг 4 можно считать в основном закрытым, за исключением `/* */` закомментированных тел.
- **ASSERT-only переменные:** ~27 переменных используются только в ASSERT() — в Release сборке
  ASSERT раскрывается в пустое выражение, создавая ложные warnings. Эти переменные нужны для
  Debug-сборки и не удаляются.
- **Агенты при массовой чистке** иногда: (а) удаляли тела функций но оставляли forward declarations,
  (б) сохраняли build output в корень репы вместо /tmp. Нужна проверка после каждого batch'а.

---

## Шаг 3 — Неиспользуемые сущности (grep по проекту)

### Масштаб (оценка)

Кодовая база после Шагов 1-2: **671 файл** (316 .cpp, 355 .h) в 10 модулях.

Экспортируемые сущности (примерно):
- ~2100 extern переменных в хедерах
- ~2600 деклараций функций в хедерах
- ~4500 макросов (#define)
- ~530 типов (struct/enum/typedef)

Компилятор (Шаг 2) покрыл только **static** функции и локальные переменные.
Шаг 3 ищет мёртвый код **между** файлами: non-static функции, extern-глобалки, типы, макросы.

Ожидаемый улов: **500-800 неиспользуемых сущностей**.

### Метод

Для каждого модуля:
1. Извлечь все экспортируемые сущности из .h файлов (функции, extern переменные, типы, макросы)
2. Grep каждой сущности по всему `src/` — если нигде не используется кроме определения → кандидат
3. Safety checks: проверить function pointers, dispatch tables, макросы, conditional compilation
4. Показать пользователю список → подтверждение
5. Удалить определения (.cpp) + декларации (.h) + пустые файлы + CMakeLists.txt
6. Билд (`make build-release`)
7. Каскадная проверка: новые unused сущности после удалений?

### План итераций

Порядок: от менее связанных (leaf) модулей к более связанным (core).
Рантайм-проверка + коммит — между итерациями.

| Итерация | Модули | Файлов | Почему в этом порядке |
|----------|--------|--------|----------------------|
| **3.1** ✅ | `ai/`, `effects/`, `platform/` | ~61 | Leaf-модули. Удалено 22 сущности (1 файл целиком) |
| **3.2** ✅ | `assets/`, `ui/` | ~140 | Презентационный слой. Удалено 33 сущности (2 файла целиком) |
| **3.3** ✅ | `actors/`, `missions/` | ~121 | Геймплей. Удалено 20 сущностей (4 файла целиком). ~75 internal-only в missions/ → Шаг 5 |
| **3.4** ✅ | `world/`, `engine/`, `core/` | ~349 | Инфраструктура. Удалено ~35 сущностей (6 хедеров целиком), ~1249 строк |

### Итерация 3.1 — ai/, effects/, platform/ (2026-03-24)

Удалено 22 сущности из 7 файлов. Модули оказались довольно чистыми.

- **cloth.h** — удалён целиком (15 сущностей). Нет реализации (cloth.cpp не существует),
  все вызовы в controls.cpp внутри `/* */` комментария
- **FightTree** struct → заменён подробным комментарием над `fight_tree[][]` (по просьбе пользователя)
- **FIRE_get_next()** — stub (всегда NULL), нигде не вызывается. Связанная `FIRE_get_start()`
  вызывается из `aeng.cpp` но iterator pattern не завершён (get_next никогда не вызывается) —
  каскадная чистка get_start + 4 глобалок оставлена на будущее
- **platform.h** — 5 мёртвых сущностей: `FadeDisplay` (нет реализации), `FLAGS_USE_3DFX` (3DFX),
  `SHELL_NAME`, `NoError`, `SHELL_CHANGED` (дубликат из gd_display.h)

### Итерация 3.2 — assets/, ui/ (2026-03-24)

Удалено 33 сущности из 22 файлов. 2 файла удалены целиком.

**assets/ — 11 сущностей:**

- **anim_loader.h/.cpp** — 7 функций: `invert_mult`, `find_matching_face`, `normalise_max_matrix`,
  `create_kline_bottle`, `save_insert_a_multi_prim`, `save_insert_game_chunk`, `save_anim_system`.
  Все стабы или хелперы без внешних вызовов. Также удалены 5 осиротевших extern деклараций
  (`write_a_prim`, `add_point`, `create_a_quad`, `build_prim_object`, `save_prim_asc`),
  forward declaration `struct PrimPoint`, `#include <math.h>`
- **level_loader.h/.cpp** — 2 функции: `revert_to_prim_status` (вызов только в закомментированном коде),
  `find_colour` (нигде не вызывается). `load_needed_anim_prims` — убрана из .h, сделана `static`
  (вызывается только внутри level_loader.cpp)
- **image_compression_globals.h/.cpp** — удалены целиком: содержали только `TGA_Pixel test[256*256]`
  (debug массив ~256KB). Каскадно удалена `IC_test()` из image_compression.h/.cpp (единственный
  потребитель `test[]`, сама нигде не вызывается). Убран `#include "image_compression_globals.h"`

**ui/ — 22 сущности:**

- **cam.h** — 9 orphaned деклараций без реализации: `CAM_look_at_thing`, `CAM_set_behind_up`,
  `CAM_set_pos`, `CAM_set_angle`, `CAM_get_dangle`, `CAM_set_dpos`, `CAM_get_dpos`,
  `CAM_rotate_left`, `CAM_rotate_right`
- **interfac.h/.cpp** — 3 сущности: `player_apply_move_analgue` (декларация без реализации),
  `remove_action_used`, `get_analogue_dxdz`
- **panel.h/.cpp** — 4 функции: `PANEL_crap_text`, `PANEL_draw_number` (7-segment renderer),
  `BodgePageIntoAddAlpha`, `BodgePageIntoSub`
- **overlay.h/.cpp** — 2 функции: `arrow_object`, `arrow_pos`. Убран `#include "assets/xlat_str.h"`
- **attract.h/.cpp** — 1 функция: `any_button_pressed` (stub). Убраны `#include "engine/input/joystick.h"`,
  `joystick_globals.h`, `extern DIJOYSTATE the_state`
- **fc.h/.cpp** — 1 функция: `FC_unkill_player_cam`
- **eng_map.h/.cpp** — 1 функция: `MAP_draw_onscreen_beacons`. Убраны `#include "ui/camera/fc.h"`,
  `fc_globals.h`
- **CMakeLists.txt** — убрана строка `image_compression_globals.cpp`

---

### Итерация 3.3 — actors/, missions/ (2026-03-24)

Удалено 20 сущностей из 20 файлов. 4 файла удалены целиком.

**actors/ — 14 сущностей:**

- **effect.h/.cpp** — удалены целиком: `init_effect` + внутренняя `process_effect` (прототип 1997)
- **enemy.h/.cpp** — удалены целиком: `init_enemy` (прототип 1997, не вызывается)
- **animal.h/.cpp** — 1 функция: `ANIMAL_register` (stub return NULL). Убраны осиротевшие
  `struct GameKeyFrameChunk`, extern `load_anim_system`
- **darci.h/.cpp** — 2 функции: `fn_darci_normal` (пустой handler), `show_walkable` (debug stub)
- **person.h/.cpp** — 9 MAV-стабов (все `ASSERT(0)`): `person_mav_again`, `get_dx_dz_for_dir`,
  `init_new_mav`, `fn_person_mavigate_action`, `fn_person_mavigate`, `process_person_goto_xz`,
  `fn_person_navigate`, `init_person_command`, `mav_arrived`.
  В `person_globals.cpp` записи STATE_NAVIGATING и STATE_MAVIGATING заменены на `{ 0, NULL }`.
  В `cop.cpp` убраны extern-декларации `fn_person_navigate`, `fn_person_mavigate`
- **CMakeLists.txt** — убраны `effect.cpp`, `enemy.cpp`

**missions/ — 6 сущностей:**

- **game.h/.cpp** — 2 функции: `GAME_map_draw_old` (заменена на `GAME_map_draw`),
  `demo_timeout` (no-op, TIMEOUT_DEMO=0). Также удалены макросы `TAB_MAP_MIN_X/Z`, `TAB_MAP_SIZE`
  и два вызова `demo_timeout()` в game_loop
- **memory.h/.cpp** — 1 функция: `save_dreamcast_wad` (~230 строк, Dreamcast only)
- **memory_globals.h/.cpp** — 1 переменная: `psx_remap` (PSX only) + запись из save_table
- **eway.h** — 2 макроса: `EWAY_CONV_TALK_A`, `EWAY_CONV_TALK_B`

**Примечание:** missions/ содержит ещё ~75 сущностей которые используются только внутри своего .cpp
но экспортируются через .h — это тема Шага 5 (orphaned declarations), не Шага 3.

### Итерация 3.4 — world/, engine/, core/ (2026-03-24)

Удалено ~35 сущностей из 28 файлов. 6 хедеров удалены целиком, ~1249 строк.

**world/ — 6 stub-хедеров без реализации (удалены целиком):**

- **bang.h** — `BANG_get_next`, `BANG_Info`, `BANG_BIG` (эффекты взрывов, нет .cpp)
- **water.h** — 10+ функций `WATER_*` (водная система, нет .cpp)
- **sewer.h** — `SEWER_save/load/colvects_*/get_water` (канализация, нет .cpp).
  `SEWER_PAGE_NUMBER` использовался в `aeng_globals.cpp` — заменён на литерал `4`
- **enter.h** — `ENTER_can_i/leave/setup/get_extents` (вход в здания, нет .cpp)
- **nav.h** — `NAV_*` (навигация, нет .cpp)
- **az.h** — `AZ_create_lines/init`, `AZ_line[]`, `AZ_line_upto` (action zones, нет .cpp)
- Удалены includes из: elev.cpp (3), eway.cpp (1), game.cpp (2)

**engine/ — 9 мёртвых функций:**

- **figure.h/.cpp** — 4 функции: `mandom` (legacy PRNG), `local_set_seed` (баг seed),
  `ANIM_obj_draw_warped` (cloaking эффект), `get_sort_z_bodge` (SW renderer).
  Убран `#include "ai/mav.h"`
- **aeng_globals.h/.cpp** — `AENG_movie_update` (stub, return immediately)
- **message.h/.cpp** — `MSG_clear`
- **qeng.h/.cpp** — `QENG_init`, `QENG_fini` (неиспользуемый alt pipeline)
- **superfacet.h/.cpp** — `SUPERFACET_draw`. Убраны 3 осиротевших include

**core/ — ~20 сущностей:**

- **math.h** — `Hypotenuse` (inline, не вызывается), `PROP`/`PROPTABLE_SIZE` макросы, `#include <cstdlib>`
- **vector.h** — 3 struct'а: `SmallSVector`, `SVECTOR`, `TinyXZ`
- **types.h** — 2 typedef: `MAPCO8`, `MAPCO24`
- **macros.h** — 3 макроса-дубликата: `sgn`, `swap`, `in_range`
- **fmatrix.h** — 4 shift-константы: `SMAT_SHIFT0/1/2/D`
- **math_globals.h/.cpp** — `Proportions[]` (514 строк) удалён.
  `SinTableF[]`/`CosTableF` оставлены (используются через `SIN_F`/`COS_F` в engine_types.h)
- **timer.h** — `BreakStart/End/Facets/Frame` оставлены (вызываются в game.cpp, aeng.cpp)

---

### Что НЕ входит в Шаг 3

- `/* */` закомментированные тела функций → Шаг 4
- Orphaned declarations без определений (появятся после удалений) → Шаг 5
- Эти шаги выполняются после завершения всех итераций Шага 3

