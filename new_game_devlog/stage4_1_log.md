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
| **3.1** | `ai/`, `effects/`, `platform/` | ~61 | Leaf-модули: мало кто зависит от них. effects/ — высокий потенциал мёртвого кода (визуальные эффекты) |
| **3.2** | `assets/`, `ui/` | ~140 | Презентационный слой: assets/ (ресурсы, текстуры) и ui/ (меню, HUD, катсцены) — высокий потенциал |
| **3.3** | `actors/`, `missions/` | ~121 | Геймплей: actors зависят от engine/world, но мало кто зависит от actors. missions/ — game flow |
| **3.4** | `world/`, `engine/`, `core/` | ~349 | Инфраструктура: многое зависит от этих модулей, удалять осторожнее. core/ — только очевидно мёртвое |

### Что НЕ входит в Шаг 3

- `/* */` закомментированные тела функций → Шаг 4
- Orphaned declarations без определений (появятся после удалений) → Шаг 5
- Эти шаги выполняются после завершения всех итераций Шага 3

