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


