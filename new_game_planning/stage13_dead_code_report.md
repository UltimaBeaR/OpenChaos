# Stage 13 — Dead Code Report (рабочий артефакт)

Сгенерирован 2026-04-26 первым проходом `--gc-sections` через [stage13_dead_code_cleanup.md](stage13_dead_code_cleanup.md).
Этот файл — **рабочий артефакт чистки**: список кандидатов, прогресс по итерациям, что осталось.

## ⚠️ ПРАВИЛА ЧИСТКИ (для следующего агента)

**Чистка dead-кода — ТОЛЬКО удаление неиспользуемых символов. НИКАКОГО изменения живого кода.**

- Не дописывать защитные проверки, не рефакторить, не "пока трогаем — давай ещё это поправим"
- Не удалять вызовы функций — даже если они выглядят как no-op
- Если нашёл баг по ходу чистки — записать в `new_game_devlog/` отдельный файл и НЕ ЧИНИТЬ
- Цель: 0 риска что-либо сломать, детерминированная безопасная чистка
- Подробнее: см. ⚠️ блок в [stage13_dead_code_cleanup.md](stage13_dead_code_cleanup.md)

## ⚠️ Не комбинировать DEAD_CODE_REPORT с ASan

`DEAD_CODE_REPORT=ON` + `ENABLE_ASAN=ON` одновременно крэшат игру при старте (FileSetBasePath, MSVC-специфичный конфликт `-ffunction-sections` с ASan literal merging). Каждый по отдельности работает.

Workflow:
- Регенерация dead-list: `CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON" make configure` (ASan OFF — это дефолт)
- Verify под ASan: `make configure-asan` (DEAD_CODE_REPORT сбрасывается в OFF дефолтом)
- НЕ включать оба явно одной командой

## Workflow для продолжения чистки

**Текущее состояние uncommitted в working tree** (на 2026-04-26 после батчей 1+3+4+5+6+7+8):
- Батчи 1-7 закоммичены в "dead code cleanup: p1" и "dead code cleanup: p2"
- Батч 8 — uncommitted
- Discards count (DEAD_CODE_REPORT build): 2947 → 2933 (после батча 8)
- Отфильтрованных dead символов: ~1187 при старте батча 8
- Build OK (с DEAD_CODE_REPORT=ON, без ASan)
- НЕ закоммичено — пользователь хочет коммитить вручную

**Текущее состояние (2026-04-26, после батчей 9-14, сессия 4):**
- Discards count: 2947 → 2933 (батч 8) → 2894 (батчи 9-12) → 2881 (батч 13) → 2873 (батч 14a: fastprim) → 2865 (батч 14b: polypage) → **2863** (батч 14c: ds_bridge)
- Build OK (DEAD_CODE_REPORT=ON, без ASan)
- Uncommitted — пользователь коммитит вручную

**Текущее состояние (2026-04-26, батч 15, сессия 5):**
- Discards count: 2863 → 2725 (батч 15a: building.cpp удалён) → 2697 (батч 15b: build.cpp удалён + prim.cpp -23 функции) → **2639** (батч 15c: building_globals.cpp/h очищены до 4 live символов)
- Dead symbols: 960 → **902**
- Build OK (DEAD_CODE_REPORT=ON, без ASan)
- Uncommitted — пользователь коммитит вручную

**Сделано в батче 9 (leaf globals из live files):**
- `soundenv_globals.cpp/h`: `SOUNDENV_gndctr`
- `message_globals.cpp/h`: `message_count`
- `menufont_globals.cpp/h`: `FONT_TICK`
- `overlay_globals.cpp/h`: `timer_prev`
- `attract_globals.cpp/h`: `current_playback`
- `network_state_globals.cpp/h`: `CNET_i_am_host`, `CNET_connected`
- `supermap_globals.cpp/h`: `next_inside_mem`
- `music_globals.cpp/h`: `last_MFX_QUICK_mode`, `music_volume`
- `game_tick_globals.cpp/h`: `amb_colour` (LIGHT_Colour), `amb_choice_cur`, `controls` (UWORD)
- `player_globals.cpp` + `player.h`: `player_pos`
- `darci_globals.cpp/h`: `air_walking`, `history` (SLONG[10])
- `anim_loader_globals.cpp/h`: `key_frame_count`
- `hm_globals.cpp/h`: `HM_object_upto`
- `special_globals.cpp/h`: `special_di` + `#include "world_objects/dirt.h"` removed

**Сделано в батче 10 — Timer cluster (fully dead, deleted entirely):**
- DELETED: `src/engine/core/timer.cpp` (StartStopwatch, StopStopwatch)
- DELETED: `src/engine/core/timer_globals.cpp/h` (stopwatch_start)
- MODIFIED: `src/engine/core/timer.h` — убраны StartStopwatch/StopStopwatch декларации (BreakStart/BreakEnd оставлены — живые)
- `CMakeLists.txt`: убраны timer.cpp, timer_globals.cpp

**Сделано в батче 10 — SoundEnv cluster (fully dead, deleted entirely):**
- DELETED: `src/engine/audio/soundenv.cpp` (SOUNDENV_precalc stub, SOUNDENV_upload stub)
- DELETED: `src/engine/audio/soundenv.h`
- DELETED: `src/engine/audio/soundenv_globals.cpp` (остался пустым после батча 9)
- DELETED: `src/engine/audio/soundenv_globals.h` (AudioGroundQuad struct, unused)
- `CMakeLists.txt`: убраны soundenv.cpp, soundenv_globals.cpp

**Сделано в батче 11 — Bucket cluster (fully dead, deleted entirely):**
- DELETED: `src/engine/graphics/pipeline/bucket.cpp` (init_buckets)
- DELETED: `src/engine/graphics/pipeline/bucket.h`
- DELETED: `src/engine/graphics/pipeline/bucket_globals.cpp` (e_bucket_pool, e_buckets, e_end_buckets, bucket_lists)
- DELETED: `src/engine/graphics/pipeline/bucket_globals.h`
- `CMakeLists.txt`: убраны bucket.cpp, bucket_globals.cpp

**Сделано в батче 12 — dead functions из live files:**
- `mav.cpp/h`: удалены `MAV_turn_off_square`, `MAV_height_los_fast`
- `ware.cpp/h`: удалены `WARE_enter`, `WARE_exit` (их вызовы в collide.cpp закомментированы в `/* */`)
- `mfx.cpp/h`: удалены `MFX_set_queue_gain`, `MFX_get_wave`
- `image_compression.cpp/h`: **восстановлены** после ошибочного удаления — `IC_pack`, `IC_unpack`, `IC_Packet` живы (используются в compression.cpp). `IC_pack`/`IC_unpack` discarded linker-ом потому что callers в compression.cpp — мёртвые функции, но сам файл должен существовать для компиляции.
- `morph.cpp/h`: **восстановлены** `MORPH_get_points`, `MORPH_get_num_points` — используются в mesh.cpp
- `tracks.cpp/h`: **восстановлена** `TRACKS_Reset` — используется в save.cpp. `TRACKS_Draw` (stub) удалена верно — её вызов в aeng.cpp закомментирован.

**Уроки батча 12:**
- Если linker discards функцию, это не означает что весь её файл можно удалить — нужно проверять используется ли .h и другие символы из .cpp
- Вызовы в `/* */` блоках — функция действительно мёртвая (caller никогда не компилируется)
- **ВСЕГДА** grep-верифицировать: не только саму функцию но и её возвращаемые типы и структуры из того же .h

**Что было сделано в батче 8 (2026-04-26):**

Стратегия: связки (clusters) — leaf функции + их transitive зависимости. Удалялись вместе.

Удалены (батч 8):
- `facet_globals.cpp/h`: `iNumFacets`, `iNumFacetTextures`
- `night_globals.cpp/h`: `NIGHT_walkable[NIGHT_MAX_WALKABLE]`
- `night.h`: `NIGHT_MAX_WALKABLE` define + `NIGHT_WALKABLE_POINT` macro
- `facet.cpp/h`: `FACET_draw_quick`, `FACET_draw_walkable_old`, `FACET_project_crinkled_shadow`
- `crinkle.cpp/h`: `CRINKLE_project` (только caller был мёртвый FACET_project_crinkled_shadow)
- `ngamut.cpp/h`: `NGAMUT_view_square`
- `switch.cpp/h`: `alloc_switch` (caller `switch_functions`)
- `switch_globals.cpp/h`: удалены целиком (только содержали `switch_functions[]`)
- `CMakeLists.txt`: убран `switch_globals.cpp`
- `road.cpp/h`: `ROAD_calc_mapsquare_type`, `ROAD_get_mapsquare_type`, `ROAD_TYPE_*` constants
- `road_globals.cpp/h`: `ROAD_mapsquare_type[]`
- `aeng.cpp`: `AENG_draw_far_facets` (caller мёртвого `FACET_draw_quick`)

Пропущено:
- `env_inifile` — caller `ENV_load` жив, dc_leaf_check.py ошибся в классификации
- `LIGHT_building_point` — caller `BUILD_draw` жив в исходнике (transitive, нужно удалять вместе с `BUILD_draw`)
- `find_anim_fight_height` — вызов в живом `should_i_block` (компилятор оптимизировал, но source-ref есть)

Уроки батча 8:
- Удаление include из .h может сломать транзитивные цепочки (`aeng.h` из `crinkle.h` — восстановлен)
- `--gc-sections` требует разрешения всех символов ДО прунинга → нельзя удалять callee без caller
- dc_leaf_check.py ненадёжен на Windows — проверять grep'ом вручную

**Что было сделано в этой сессии (батчи 6-7):**

Стратегия: leaf globals — глобальные переменные без единого реального caller'а (только definition в .cpp + extern в .h). Верифицировались grep'ом перед удалением.

Удалены (батч 6):
- `bat_globals.cpp/h`: `BAT_state_name[]`
- `thug_globals.cpp/h`: `thug_states[]` (весь файл стал пустым — сокращён до минимума)
- `darci_globals.cpp/h`: `history_thing[100]`
- `person_globals.cpp/h`: `PERSON_mode_name[]`, `dir_to_angle[]`
- `attract_globals.cpp/h`: `demo_text[]`, `playbacks[]`
- `overlay_globals.cpp/h`: `help_text[]`, `help_xlat[]` (+ убран лишний #include xlat_str.h)
- `combat_globals.cpp/h`: `grapple[]`, `kicks[][]`, `punches[][]`

Удалены (батч 7):
- `pcom_globals.cpp/h`: `gang_angle_priority[]`
- `soundenv_globals.cpp/h`: `SOUNDENV_gndquads[]`
- `hierarchy_globals.cpp/h` + `hierarchy.h`: `body_part_parent_numbers[]`
- `game_tick_globals.cpp/h`: `amb_choice[]` + `#define AMB_NUM_CHOICES`

Пропущено (с обоснованием):
- `find_anim_fight_height` (combat.cpp): компилятор оптимизировал вызов из живого `should_i_block` (return value дисcard), но в исходнике вызов присутствует. Правило #4 — не удалять вызовы из живого кода.

**Сделано в батче 13 (poly.cpp + anim.cpp leaf cluster):**
- `poly.cpp`: удалены `NewTweenVertex2D_X`, `NewTweenVertex2D_Y`, `POLY_clip_against_side_X`, `POLY_clip_against_side_Y` (кластер: clip_side вызывает tween, у cluster нет внешних callers)
- `anim.cpp`: удалены `Anim::Anim()`, `Anim::~Anim()`, `Anim::StartLooping`, `Anim::StopLooping`, `Anim::SetAllTweens`, `Anim::AddKeyFrame`, `Anim::RemoveKeyFrame`, `Character::AddAnim`, `Character::RemoveAnim` (9 методов, нет внешних callers)
- Объявления в `anim_types.h` оставлены (не вызываются → linker не ругается; зачистка хедеров отдельной итерацией)
- Build OK, 2894 → 2881 discards (-13)

**Сделано в батче 14 (сессия 4):**
- `fastprim.cpp/h`: удалены `FASTPRIM_find_texture_from_page`, `FASTPRIM_free_cached_prim`, `FASTPRIM_free_queue_for_call`, `FASTPRIM_create_call`, `FASTPRIM_add_point_to_call`, `FASTPRIM_ensure_room_for_indices`, `FASTPRIM_draw` (7 функций-кластером); почищены include'ы в fastprim.h и fastprim.cpp (−8 discards)
- `polypage.cpp/h`: удалены `AddFan`, `AddWirePoly`, `DrawSinglePoly`, `SortBackFirst`, `MergeSortIteration` + `DoMerge` static template; почищен `#include "poly.h"` из polypage.h (−8 discards)
- `ds_bridge.cpp/h`: удалены `ds_trigger_bow`, `ds_trigger_machine` (legacy simplified wrappers без callers) (−2 discards)

**Сделано в батче 15 (building cluster, сессия 5):**
- `building.cpp`: удалён целиком (135 dead functions, все были dead) (-138 discards)
- `building.h`: очищен до forwarding header (только includes prim_types.h + building_types.h)
- `build.cpp` + `build.h`: удалены целиком (3 dead functions: BUILD_draw, BUILD_draw_inside, LIGHT_get_colour)
- `prim.cpp`: удалены 23 dead functions (delete_prim_points_block, compress_prims, smooth_a_prim, copy_prim_to_end, delete_prim_*, delete_last_prim, delete_a_prim, quick_normal, apply_ambient_light_to_object, get_rotated_point_world_pos, set_anim_prim_anim, toggle_anim_prim_switch_state и др.); убраны dead macros (shadow_calc, in_shadow, in_shadowo)
- `building_globals.cpp`: очищен до 4 live globals (next_roof_bound, textures_xy, dx_textures_xy, textures_flags) (-58 discards)
- `building_globals.h`: очищен до 4 live extern declarations + minimal includes
- CMakeLists.txt: убраны building.cpp, build.cpp

**Сделано в батче 16 (aeng.cpp + HM module, сессия 6):**
- `hm.cpp` + `hm.h`: удалены целиком (все функции мёртвые, все callers в game_tick.cpp в `/* */` блоках)
- `hm_globals.cpp` + `hm_globals.h`: удалены целиком (4 dead globals)
- `CMakeLists.txt`: убраны hm.cpp, hm_globals.cpp
- `attract.cpp`: убрана dead forward declaration `AENG_demo_attract` (3 строки)
- `combat.cpp`: убраны extern decls для `e_draw_3d_line` и `e_draw_3d_line_col` (4 строки)
- `collide.cpp`: убрана extern decl для `e_draw_3d_line` (1 строка)
- `aeng.h`: удалены 14 диапазонов мёртвых деклараций (119 строк) — `AENG_e_draw_3d_line*`, `e_draw_3d_line*` макросы, `AENG_draw_scanner/power/FPS/fade_in`, `AENG_light_draw/waypoint_draw/rad_trigger_draw/groundsquare_draw`, `AENG_set_sky_nighttime/daytime`, `AENG_draw_col_tri`, `AENG_fade_in`, `AENG_get_draw_distance`, `AENG_set_camera_radians` (6-param)
- `aeng_globals.h`: удалены декларации `AENG_calc_gamut_lo_only`, `AENG_do_cached_lighting_old`, `AENG_draw_rain_old`, `AENG_draw_cloth`, `AENG_add_projected_lit_shadow_poly`, `show_gamut_hi`, `cache_a_row`, `add_kerb`, `draw_i_prim`, `general_steam`, `draw_quick_floor`, `AENG_draw_rectr`
- `poly.h`: удалены декларации `calc_global_cloud`, `use_global_cloud`
- `aeng.cpp`: удалено 2112 строк мёртвых функций (9156 → 7044 строк):
  - `calc_global_cloud`, `use_global_cloud`
  - `AENG_get_draw_distance`
  - `AENG_calc_gamut_lo_only`
  - 6-param `AENG_set_camera_radians` (без splitscreen)
  - `AENG_do_cached_lighting_old`
  - `AENG_add_projected_lit_shadow_poly`, `AENG_draw_rain_old`
  - `AENG_draw_cloth`
  - `AENG_set_sky_nighttime`, `AENG_set_sky_daytime`, `AENG_draw_rectr`
  - `AENG_draw_col_tri`
  - `show_gamut_hi`
  - floor cluster: `cache_a_row`, `add_kerb`, `draw_i_prim`, `general_steam`, `draw_quick_floor`
  - `AENG_draw_scanner`, `AENG_draw_power`
  - `AENG_draw_FPS`
  - `AENG_fade_in`
  - `AENG_e_draw_3d_line`, `AENG_e_draw_3d_line_dir`, `AENG_e_draw_3d_line_col`, `AENG_e_draw_3d_line_col_sorted`, `AENG_e_draw_3d_mapwho`, `AENG_e_draw_3d_mapwho_y`, `AENG_demo_attract`
  - `AENG_light_draw`, `AENG_waypoint_draw`, `AENG_rad_trigger_draw`, `AENG_groundsquare_draw`
  - Добавлен `#define AENG_NUM_RAINDROPS 128` на file scope (был внутри удалённой dead функции, но используется в живой `AENG_draw_rain`)
- Build OK (make build-release, exit code 0)
- Uncommitted — пользователь коммитит вручную

**Сделано в батче 17 (QMAP/QENG/stair кластеры + aeng_globals + game.cpp, сессия 7):**
- `qmap.cpp/h` + `qmap_globals.cpp/h`: удалены целиком (ни одного live caller)
- `qeng.cpp/h` + `qeng_globals.cpp/h`: удалены целиком (ни одного live caller)
- `stair.cpp/h` + `stair_globals.cpp/h`: удалены целиком (ни одного live caller)
- CMakeLists.txt: убраны qeng.cpp, qeng_globals.cpp, qmap.cpp, qmap_globals.cpp, stair.cpp, stair_globals.cpp
- `aeng_globals.cpp`: удалены 21 dead global + убраны 2 includes (`polypage.h`, `game_graphics_engine.h`)
- `aeng_globals.h`: удалены декларации + types/constants только для dead globals (`StoreLine`, `MAX_LINES`, `FloorStore`, `GroupInfo`, `MAX_FLOOR_TILES_FOR_STRIPS` и др.)
- `game.cpp`: удалены 4 dead функции (`GAME_map_draw`, `leave_map_form_proc`, `process_bullet_points`, `hardware_input_replay`) + extern decl `plan_view_shot` + dead includes
- `game.h`: удалены декларации dead функций + удалены includes `widget.h`, `widget_globals.h`, `game_globals.h`; добавлен `game_types.h` (вместо game_globals.h — чистый types-only header)
- Побочный эффект: `game.cpp` добавлен `#include "game/game_globals.h"` (прямая зависимость вместо транзитивной через game.h)
- Побочный эффект: `figure.h` добавлены `#include "effects/combat/pyro.h"` и `#include "things/core/drawtype.h"` (были скрытой транзитивной зависимостью через game.h → game_globals.h → thing.h)
- Build OK (make build-release, exit code 0)

**Что делать дальше:**

1. Регенерировать dead-list при начале следующей сессии:
   ```bash
   CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON -DENABLE_ASAN=OFF" make configure
   make build-release 2>&1 | grep "^lld-link: Discarded" | sed 's/^lld-link: Discarded //' > /c/Users/BeaR/AppData/Local/Temp/discarded_filtered.txt
   python /c/Users/BeaR/AppData/Local/Temp/dc_map.py
   python /c/Users/BeaR/AppData/Local/Temp/dc_demangle.py
   ```

2. Следующие приоритеты (актуально после батча 16, **~2639 discards, ~902 dead symbols**; точные числа — регенерировать):
   - **`src/things/characters/person.cpp` (36 dead)**
   - **`src/ui/menus/widget.cpp` (34 dead)** — `MENUFONT_Draw_floats` transitive (caller `BUTTON_Draw` тоже dead)
   - **`src/ui/hud/eng_map.cpp` (32 dead)**
   - `map/qmap_globals.cpp` (29 dead)
   - `engine/graphics/graphics_engine/backend_opengl/game/core.cpp` (24 dead)
   - `map/sewers.cpp` (23 dead) — NS_cache_* + NS_init/NS_precalculate кластер, файл имеет живые функции
   - `engine/physics/collide.cpp` (22 dead)
   - `engine/graphics/pipeline/aeng_globals.cpp` (22 dead)
   - `outro/core/outro_os_globals.cpp` (21 dead)
   - ~~`engine/physics/hm.cpp` (21 dead)~~ — удалён в батче 16
   - `buildings/prim_globals.cpp` (3 dead globals: global_bright, global_flags, global_res)

3. Продолжать стратегию: сначала leaf, потом transitive связками

**Инструменты (если нужно восстановить dc_file_to_dead.json):**

Скрипты лежат в `C:/Users/BeaR/AppData/Local/Temp/`:
- `dc_map.py` — строит file→dead_symbols маппинг
- `dc_demangle.py` — добавляет demangled имена
- `dc_leaf_check.py` — грубая leaf/transitive классификация (есть баги в path matching на Windows — не доверять на 100%, всегда верифицировать grep'ом вручную)

После регенерации: `/tmp/discarded.txt` → `/tmp/discarded_filtered.txt` → `dc_map.py` → `dc_demangle.py`

**Для продолжения с того же места:**

1. Прочитать стек правил выше + правила в `stage13_dead_code_cleanup.md`
2. Учесть предупреждение выше про DEAD_CODE_REPORT + ASan — не комбинировать
3. Если есть закоммиченные изменения — продолжать с уже накопленного состояния (`git diff main` покажет что было сделано). Если нет — статус выше актуален
4. Регенерировать актуальный dead-list (изменилось после удалений):
   ```bash
   CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON -DENABLE_ASAN=OFF" make configure
   make build-release > /tmp/dead_code_build.log 2>&1
   grep '^lld-link: Discarded' /tmp/dead_code_build.log | sed 's/^lld-link: Discarded //' > /tmp/discarded.txt
   ```
   (или использовать существующий список — но он мог устареть после батчей 1+3+4+5)
5. Выбрать файл из таблицы "Top hot zones / Transitive dead" — приоритет `building.cpp` (134 dead символов!), `aeng.cpp` (40+22 globals), `person.cpp` (36), `widget.cpp` (34)
6. Для выбранного файла:
   - `grep "src/<файл>" /tmp/dc/all_dead_per_file.txt` → реальный список dead символов
   - **ВАЖНО:** не использовать списки из памяти/контекста — только grep'ом из файла. В прошлой сессии я галлюцинировал списки (см. "Batch 2 attempt REVERTED")
   - Для каждого dead символа: проверить `grep -rn "\b$symbol\b" new_game/src/` — есть ли callers вне самого файла
   - Если есть call sites вне dead-set — пропустить символ (caller жив)
   - Если call sites только внутри dead функций — удалить связкой
7. Edit/Write для удаления функции (uc_orig comment + signature + body) и декларации в .h
8. После каждых 1-3 файлов — build (без ASan!) для верификации
9. После 4-5 файлов или ~30-50 удалений — обновить отчёт + предложить пользователю коммит

**Что НЕ делать:**
- **НЕ запускать subagent'ов** для самого удаления — в прошлой сессии главный агент галлюцинировал списки символов и передавал их subagent'ам, те "честно" удаляли несуществующие функции и пытались удалять живые. Откатили весь батч. Работать в основном контексте, последовательно файл за файлом.
- **НЕ брать списки символов из памяти/контекста** — ТОЛЬКО через `grep` из реальных файлов на диске (`/tmp/dc/all_dead_per_file.txt`, build log). Это правило #1 для предотвращения галлюцинаций.
- Не комбинировать DEAD_CODE_REPORT + ASan (см. блок выше)
- Не "пока трогаем — давай починим" — записать баг в devlog и идти дальше
- Не менять живой код — только удаление мёртвых символов
- Не "чинить" найденные баги inline — записывать в devlog

## Как сгенерирован

CMake опция `DEAD_CODE_REPORT=ON` в [new_game/CMakeLists.txt](../new_game/CMakeLists.txt) включает:
- `-ffunction-sections -fdata-sections` (компилятор)
- `/OPT:REF /VERBOSE` на Windows (lld-link) либо `--gc-sections --print-gc-sections` на Unix (ld.lld)

Прогон:
```bash
CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON" make configure
make build-release > /tmp/dead_code_build.log 2>&1
```

Затем парсинг: `grep '^lld-link: Discarded' build.log` → пересечение с `llvm-nm --defined-only --extern-only` по всем нашим `.obj` → текстовый grep по `new_game/src/` для разделения на **leaf** vs **transitive**.

## Сводка по последнему прогону

| Метрика | Значение |
|---------|----------|
| Всего "Discarded" линкером (после фильтра констант/строк) | ~1145 |
| **Leaf dead** (никто в исходниках не ссылается) | **81** |
| **Transitive dead** (в исходниках есть текстовые ссылки, но caller'ы тоже мертвы) | 1055 |

**Разница важна:** leaf можно удалять напрямую. Transitive — там есть `if (...)` в исходнике который вызывает функцию, но эта ветка/функция-обёртка тоже в dead-list. Нужно удалять связкой (граф зависимостей), иначе сборка сломается.

## Top hot zones (по количеству)

### Leaf dead (всё что осталось живо после фильтра)

| Файл | Кол-во |
|------|--------|
| src/engine/core/quaternion.cpp | 10 |
| src/map/sewers.cpp | 9 |
| src/engine/graphics/geometry/superfacet.cpp | 9 |
| src/assets/formats/anim.cpp | 9 |
| src/engine/graphics/geometry/fastprim.cpp | 7 |
| src/engine/graphics/ui_coords.cpp | 5 |
| src/engine/graphics/pipeline/polypage.cpp | 5 |
| src/engine/graphics/pipeline/poly.cpp | 4 |
| src/combat/combat.cpp | 4 |
| src/things/vehicles/vehicle.cpp | 3 |
| src/game/input_actions.cpp | 3 |
| src/engine/graphics/graphics_engine/game_graphics_engine.cpp | 3 |
| src/ui/hud/overlay.cpp | 2 |
| src/engine/core/matrix.cpp | 2 |
| src/things/characters/cop.cpp | 1 |
| src/engine/graphics/pipeline/aeng.cpp | 1 |
| src/camera/fc.cpp | 1 |
| src/buildings/building.cpp | 1 |
| src/assets/formats/level_loader.cpp | 1 |
| src/ai/pcom.cpp | 1 |

### Transitive dead (top 30, нужно удалять связкой)

| Файл | Кол-во |
|------|--------|
| src/buildings/building.cpp | 133 |
| src/buildings/building_globals.cpp | 58 |
| src/engine/graphics/pipeline/aeng.cpp | 40 |
| src/things/characters/person.cpp | 36 |
| src/ui/menus/widget.cpp | 34 |
| src/map/qmap_globals.cpp | 29 |
| src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp | 23 |
| src/buildings/prim.cpp | 23 |
| src/engine/physics/collide.cpp | 22 |
| src/engine/graphics/pipeline/aeng_globals.cpp | 22 |
| src/outro/core/outro_os_globals.cpp | 21 |
| src/map/sewers_globals.cpp | 20 |
| src/engine/physics/hm.cpp | 16 |
| src/map/sewers.cpp | 14 |
| src/buildings/stair_globals.cpp | 14 |
| src/ui/hud/planmap_globals.cpp | 13 |
| src/outro/core/outro_os.cpp | 13 |
| src/ui/hud/panel_globals.cpp | 12 |
| src/missions/memory.cpp | 12 |
| src/game/game_globals.cpp | 11 |
| src/engine/graphics/pipeline/poly.cpp | 10 |
| src/ui/menus/widget_globals.cpp | 9 |
| src/outro/core/outro_mf.cpp | 9 |
| src/map/qmap.cpp | 9 |
| src/engine/physics/collide_globals.cpp | 9 |
| src/assets/texture.cpp | 9 |
| src/ui/hud/eng_map_globals.cpp | 8 |
| src/things/core/switch.cpp | 8 |
| src/game/input_actions_globals.cpp | 8 |
| src/engine/graphics/graphics_engine/backend_opengl/outro/core.cpp | 8 |

## Полный leaf-список

```
src/engine/core/matrix.cpp	MATRIX_calc_int
src/engine/core/matrix.cpp	MATRIX_find_angles_old
src/engine/core/quaternion.cpp	QuatMul
src/engine/core/quaternion.cpp	EulerToQuat
src/engine/core/quaternion.cpp	is_unit
src/engine/core/quaternion.cpp	MatrixToQuatInteger
src/engine/core/quaternion.cpp	QuatToMatrixInteger
src/engine/core/quaternion.cpp	BuildACosTable
src/engine/core/quaternion.cpp	acos
src/engine/core/quaternion.cpp	QuatSlerpInteger
src/engine/core/quaternion.cpp	cmat_to_mat
src/engine/core/quaternion.cpp	check_isonormal_integer
src/engine/graphics/pipeline/polypage.cpp	GERenderState::ZLift
src/engine/graphics/pipeline/polypage.cpp	PolyPage::AddWirePoly
src/engine/graphics/pipeline/polypage.cpp	PolyPage::DrawSinglePoly
src/engine/graphics/pipeline/polypage.cpp	PolyPage::SortBackFirst
src/engine/graphics/pipeline/polypage.cpp	PolyPage::MergeSortIteration
src/engine/graphics/pipeline/aeng.cpp	AENG_draw_far_facets
src/engine/graphics/pipeline/poly.cpp	NewTweenVertex2D_X
src/engine/graphics/pipeline/poly.cpp	NewTweenVertex2D_Y
src/engine/graphics/pipeline/poly.cpp	POLY_clip_against_side_X
src/engine/graphics/pipeline/poly.cpp	POLY_clip_against_side_Y
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_find_texture_from_page
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_free_cached_prim
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_free_queue_for_call
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_create_call
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_add_point_to_call
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_ensure_room_for_indices
src/engine/graphics/geometry/fastprim.cpp	GERenderState::IsAlphaBlendEnabled
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_free_end_of_queue
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_convert_texture
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_make_facet_points
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_create_points
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_fill_facet_points
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_build_call
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_create_calls
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_convert_facet
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_redo_lighting
src/engine/graphics/ui_coords.cpp	ui_coords::frame_to_screen
src/engine/graphics/ui_coords.cpp	ui_coords::screen_to_frame
src/engine/graphics/ui_coords.cpp	ui_coords::old_px_to_screen
src/engine/graphics/ui_coords.cpp	ui_coords::screen_to_pixels
src/engine/graphics/ui_coords.cpp	ui_coords::old_px_to_screen_pixels
src/engine/graphics/graphics_engine/game_graphics_engine.cpp	GERenderState::SetTextureFilter
src/engine/graphics/graphics_engine/game_graphics_engine.cpp	GERenderState::SetBlendMode
src/engine/graphics/graphics_engine/game_graphics_engine.cpp	GERenderState::SetTempTransparent
src/assets/formats/anim.cpp	Anim::ctor
src/assets/formats/anim.cpp	Anim::dtor
src/assets/formats/anim.cpp	Anim::StartLooping
src/assets/formats/anim.cpp	Anim::StopLooping
src/assets/formats/anim.cpp	Anim::SetAllTweens
src/assets/formats/anim.cpp	Anim::AddKeyFrame
src/assets/formats/anim.cpp	Anim::RemoveKeyFrame
src/assets/formats/anim.cpp	Character::AddAnim
src/assets/formats/anim.cpp	Character::RemoveAnim
src/assets/formats/level_loader.cpp	_vfscanf_l
src/map/sewers.cpp	NS_cache_create_wallstrip
src/map/sewers.cpp	NS_cache_create_extra_bit_left
src/map/sewers.cpp	NS_cache_create_extra_bit_right
src/map/sewers.cpp	NS_cache_create_walls
src/map/sewers.cpp	NS_cache_create_curve_sewer
src/map/sewers.cpp	NS_cache_create_curve_top
src/map/sewers.cpp	NS_cache_create_curves
src/map/sewers.cpp	NS_cache_create_falls
src/map/sewers.cpp	NS_cache_create_grates
src/buildings/building.cpp	place_building_at
src/things/characters/cop.cpp	fn_cop_fight
src/things/vehicles/vehicle.cpp	VEH_dealloc
src/things/vehicles/vehicle.cpp	free_vehicle
src/things/vehicles/vehicle.cpp	animate_car
src/combat/combat.cpp	find_possible_combat_target
src/combat/combat.cpp	find_hit_value
src/combat/combat.cpp	find_anim_fight_height
src/combat/combat.cpp	show_fight_range
src/ai/pcom.cpp	get_nearest_gang_member
src/camera/fc.cpp	FC_must_move_up_and_around
src/ui/hud/overlay.cpp	show_help_text
src/ui/hud/overlay.cpp	OVERLAY_draw_tracked_enemies
src/game/input_actions.cpp	action_ladder
src/game/input_actions.cpp	action_flip_left
src/game/input_actions.cpp	action_flip_right
```

## Подводные камни (false positives класс ошибок)

1. **Транзитивно мёртвое**: вызов в коде есть, но caller сам мёртв. Линкер выкидывает обоих. Если удалить функцию вслепую — код caller не компилируется. Нужно удалять связкой.
2. **Callback'и через указатель**: функция зарегистрирована как `&fn` в setter'е (`ge_set_X_callback`). `--gc-sections` это видит как live (адрес взят), так что в dead-list таких быть не должно. Но если setter сам вырезан — функция станет мёртвой. Проверять при удалении.
3. **Виртуальные методы**: vtable классов используется через виртуальный диспатч, конкретный метод может выглядеть мёртвым но вызывается через указатель на базовый класс.
4. **Шаблонные инстансы**: STL contains'ы могут давать ложные leaf-кандидаты.
5. **Импорты из системных DLL**: `c_DwmMaxQueuedBuffers`, `_Avx2WmemEnabledWeakValue` — это extern символы из системных либ. Уже отфильтрованы.
6. **Stale .obj от удалённых исходников**: `weapon_feel_test.cpp.obj` остался в build/ но исходника нет, в линке не участвует. Уже отфильтрован.

## Прогресс по итерациям

(заполняется по ходу работы)

### 2026-04-26 — Batch 1: quaternion (integer-path + dead helpers)

**Удалено из `src/engine/core/quaternion.cpp`:**
- Forward decl `QUATERNION_BuildTweenInteger` (наверху файла)
- Float-path dead helpers: `QuatMul`, `EulerToQuat`, `is_unit`
- Весь integer-path:
  - `struct QuatInt`
  - `MatrixToQuatInteger`
  - `QuatToMatrixInteger`
  - `BuildACosTable`
  - `#define DELTA_INT 1638`
  - `QuatSlerpInteger`
  - `cmat_to_mat`
  - `check_isonormal_integer`
  - `QUATERNION_BuildTweenInteger`

**Сохранено в `quaternion.cpp` (single live entry-point + его транзитивные хелперы):**
`CQuaternion::BuildTween` (вызывается из `figure.cpp:1776,3769,4046`) → использует `cmat_to_fmat`, `check_isonormal`, `MatrixToQuat`, `QuatSlerp`, `QuatToMatrix`, `fmat_to_mat`. Все хелперы помечены `static` (file-local), поскольку нет внешних вызовов.

**Удалено из `src/engine/core/quaternion.h`:**
- Декларации `QuatSlerp`, `QuatToMatrix`, `QUATERNION_BuildTweenInteger`
- Класс `FloatMatrix` (перенесён в `quaternion.cpp` — используется только внутри)

**Удалены файлы целиком:**
- `src/engine/core/quaternion_globals.cpp` (содержал только `acos_table` + `acos_table_init` для integer-path)
- `src/engine/core/quaternion_globals.h`

**Прочее:**
- `#include <math.h>` убран из `quaternion.cpp` (clangd: not used directly, символы приходят через `engine/core/math.h`)
- `quaternion_globals.cpp` убран из `new_game/CMakeLists.txt`

**Результат сборки:** OK, exit 0. Линкер discarded count: **4534 → 4510** (-24).

| Дата | Итерация | Файлы/символы | Результат |
|------|----------|---------------|-----------|
| 2026-04-26 | Batch 1 | quaternion (12 функций + 2 globals + 2 файла) | -24 discards, build OK |
| 2026-04-26 | Batch 2 attempt (REVERTED) | parallel SAs с галлюцинированными списками — откат | — |
| 2026-04-26 | Batch 3 | superfacet.cpp (9 dead funcs) + superfacet_globals (2 globals) | -45 discards, build OK |
| 2026-04-26 | Batch 4 | matrix.cpp (5 funcs) + matrix.h + outro_matrix.h + fc.cpp (3 funcs) + fc.h + cop.cpp (fn_cop_fight) + pcom.cpp (4 funcs + extern decl) + pcom.h. Не тронуто: MATRIX_find_angles (есть caller в HM_draw который сам мёртв но не удалён). | -15 discards, build OK |
| 2026-04-26 | Batch 5 | ui_coords (5 funcs + .h decls), game_graphics_engine (3 GERenderState methods), overlay (4 funcs: should_i_add_message, show_help_text, OVERLAY_draw_tracked_enemies, add_damage_value, add_damage_value_thing — пропустил overlay_beacons тк живой caller в game.cpp:519), combat (4 funcs: find_possible_combat_target, find_hit_value, show_fight_range, is_person_under_attack — пропустил find_anim_fight_height тк compiler оптимизировал вызов в живом should_i_block но в исходнике вызов остался), vehicle (4: VEH_dealloc, VEH_get_assignments, animate_car, free_vehicle), input_actions (3 dead arrays: action_ladder/flip_left/flip_right + 2 funcs: get_last_input, allow_input_autorepeat — пропустил continue_action, set_look_pitch, lock_to_compass, pre_process_input тк live callers in person.cpp). | -38 discards, build OK |
| 2026-04-26 | Batch 6 | leaf globals: BAT_state_name, thug_states, history_thing, PERSON_mode_name, dir_to_angle, demo_text, playbacks, help_text, help_xlat, grapple[], kicks[][], punches[][] (12 globals в 8 файлах). Пропущен find_anim_fight_height — вызов из живого should_i_block (compiler оптимизировал, но source-ref остался). | -12 discards, build OK |
| 2026-04-26 | Batch 7 | leaf globals: gang_angle_priority, SOUNDENV_gndquads, body_part_parent_numbers (из hierarchy_globals.cpp + 2 .h), amb_choice + AMB_NUM_CHOICES (game_tick_globals) | -4 discards, build OK |
| 2026-04-26 | Batch 8 | facet cluster (3 funcs + 2 globals + night_globals/h macros), crinkle_project, ngamut_view_square, switch cluster (alloc_switch + switch_functions + switch_globals.cpp/h deleted), road cluster (2 funcs + ROAD_mapsquare_type + ROAD_TYPE_* consts), AENG_draw_far_facets | -14 discards, build OK |
| 2026-04-26 | Batch 9 | qeng.cpp/h + qeng_globals.cpp/h: DELETED ENTIRE MODULE (software rasterizer, unused since OpenGL migration) | build OK |
| 2026-04-26 | Batch 10 | qmap.cpp/h + qmap_globals.cpp/h: DELETED ENTIRE MODULE (software map renderer tied to qeng) | build OK |
| 2026-04-26 | Batch 11 | stair.cpp/h + stair_globals.cpp/h: DELETED ENTIRE MODULE (stair rendering, dead since new renderer) | build OK |
| 2026-04-26 | Batch 12 | aeng_globals.cpp/h: deleted dead globals (Lines[], next_line, global_b, AENG_sewer_page, AENG_gamut_lo_*, AENG_project_lit_*, AENG_sky_type/colour, m_vert_mem_block32, m_indicies, kerb_scale*/du/dv) + dead struct/macro defs (StoreLine, FloorStore, GroupInfo, MAX_LINES, KERB_*) | build OK |
| 2026-04-26 | Batch 13 | poly.cpp cluster (4 funcs: NewTweenVertex2D_X/Y + POLY_clip_against_side_X/Y), anim.cpp Anim+Character methods (9 funcs) | -13 discards, build OK |
| 2026-04-26 | Batch 14a | fastprim.cpp (7 dead funcs удалены кластером: FASTPRIM_draw и 6 хелперов); fastprim.h/cpp include cleanup | -8 discards, build OK |
| 2026-04-26 | Batch 14b | polypage.cpp (AddFan, AddWirePoly, DrawSinglePoly, SortBackFirst, MergeSortIteration, DoMerge); polypage.h decl cleanup + убран #include poly.h | -8 discards, build OK |
| 2026-04-26 | Batch 14c | ds_bridge.cpp/h: ds_trigger_bow, ds_trigger_machine (legacy simplified wrappers) | -2 discards, build OK |
| 2026-04-26 | Batch 15 | game_globals.cpp/h: 11 dead globals (CAM_cur_x/y/z/yaw/pitch/roll, pause_menu, game_paused_highlight, bullet_point, screen_mem, verifier_name) | build OK |
| 2026-04-26 | Batch 16 | planmap.cpp/h + planmap_globals.cpp/h: DELETED ENTIRE MODULE (2 dead funcs + 13 dead globals). anim_loader.cpp: 8 dead funcs cluster (load_frame_numbers, sort_multi_object, set_default_people_types, make_compress_matrix, load_multi_vue, load_anim_mesh, load_key_frame_chunks, load_a_multi_prim) + anim_loader.h decl cleanup. outro_os_globals.cpp/h: 21 dead globals (OS_application_name, OS_frame_is_fullscreen, OS_frame_is_hardware, KEY_inkey, OS_midas_ok, OS_trans_upto, all OS_bitmap_*). Fix: figure.h needed direct includes for Pyro+DrawTween; game.h→game_types.h to fix Thing cascade; game.cpp needed direct #include game_globals.h. | build OK |
| 2026-04-26 | Batch 17 | sewers.cpp: 23 dead funcs removed (NS_init, NS_precalculate, NS_cache_*, NS_add_*, NS_create_wallstrip_point×2, NS_get_unused_st) — kept 5 live (NS_calc_height_at, NS_calc_splash_height_at, NS_slide_along, NS_inside, NS_there_is_a_los). sewers_globals.cpp: 20 dead globals removed — kept NS_hi + NS_los_fail_x/y/z. sewers_globals.h: trimmed to only 4 live externs. Note: attempted full module delete — caught by linker (darci/dirt live callers). | build OK |
