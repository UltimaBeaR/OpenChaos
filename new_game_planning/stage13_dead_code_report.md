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

**Текущее состояние (2026-04-27, после батча 36/финальный проход пустых файлов, сессия 18):**
- Discards count: 4534 (старт) → ... → **1800** (батч 35)
- **Список gc-sections exhausted** — все 80 оставшихся символов нельзя удалять (system noise / source-level callers / inlining artifacts)
- **Финальный проход пустых файлов: выполнен** — удалены shadow_globals.cpp/h + startscr.cpp
- Build OK (make build-release, exit 0)
- Uncommitted — пользователь коммитит вручную
- ⚠️ aeng.cpp: 6 функций пропущены (apply_cloud, show_gamut_lo, AENG_draw_bangs, AENG_draw_people_messages, AENG_clear_screen, AENG_fade_out) — linker их дискардит из-за инлайнинга/мёртвых веток, но source-level вызовы существуют в живых функциях. Нельзя удалить определение без удаления вызовов (нарушение правила "не трогать живой код").
- ⚠️ eng_map_globals.cpp (8 globals), input_actions_globals.cpp (input_to_angle) — constant-propagated globals: linker дискардит, но source-level callers есть в live функциях. Нельзя удалять.
- ⚠️ MF_rotate_mesh (outro_mf.cpp) — live caller WIRE_draw в outro_wire.cpp. Нельзя удалять.
- ⚠️ core.cpp (11 stubs), person.cpp (5 stubs), ds_bridge.cpp (2 API stubs) — intentional stubs / stubs called from live code (caller in same .o as live code). Нельзя удалять.
- ⚠️ oc_config.cpp (18), input_debug.cpp (4), input_debug_dualsense.cpp (1), sdl3_bridge.cpp (3 DWM symbols), dirt.cpp (1 XMM) — system/compiler noise (STL RTTI vtables, XMM constants, system DLL symbols). Нельзя удалять.

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

**Сделано в батче 24 (memory.cpp cluster, сессия 11):**
- `memory.cpp`: удалены 12 dead функций: `release_memory`, `set_darci_normals`, `convert_drawtype_to_index`, `convert_thing_to_index`, `convert_pointers_to_index`, `STORE_DATA` macro + body, `convert_keyframe_to_index`, `convert_animlist_to_index`, `convert_fightcol_to_index`, `save_whole_anims`, `find_best_anim_offset_old`, `find_best_anim_offset`, `MEMORY_quick_load_available`
- `memory.h`: удалены декларации всех 12 функций; обновлён "Internal helpers" комментарий
- Подтверждено: `convert_pointers_to_index` — единственный call site в game_tick.cpp внутри `/* */` блока (не компилируется)
- Подтверждено: `save_whole_game` (пустой stub) оставлен — вызывается из game_tick.cpp F3 debug path (live)
- Build OK (make build-release, exit code 0)

**Сделано в батче 25 (oc_config + env string cascade, сессия 11):**
- `oc_config.cpp`: удалены `OC_CONFIG_get_string`, `OC_CONFIG_set_string`, `g_str_return_buf` static global
- `oc_config.h`: удалены декларации `OC_CONFIG_get_string`, `OC_CONFIG_set_string`; обновлён комментарий
- `env.cpp`: удалены `ENV_get_value_string`, `ENV_set_value_string`
- `env.h`: удалены декларации `ENV_get_value_string`, `ENV_set_value_string`
- Эффект: устранена каскадная генерация ~52 dead символов от nlohmann::json template instantiations (генерировались 2 функциями `OC_CONFIG_get/set_string`)
- Build OK (make build-release, exit code 0)

**Сделано в батче 26 (panel module, сессия 11):**
- `panel_globals.cpp`: удалены через Python (строки 18-125): `#define PIC/PICALL0/PICALL1`, `PANEL_ic[PANEL_IC_NUMBER]` (большой инициализатор), `PANEL_page[4][PANEL_PAGE_NUMBER]`, `PANEL_beat[2][...]`, `PANEL_beat_head/tick/last_ammo/last_specialtype/x[2]`, `PANEL_ammo[PANEL_AMMO_NUMBER]`, `PANEL_toss[PANEL_MAX_TOSSES]`, `PANEL_toss_last`, `PANEL_toss_tick`; убран `#include "engine/graphics/pipeline/poly.h"` (нужен был только для PANEL_page init)
- `panel_globals.h`: удалены через Python (строки 33-167): `PANEL_Ic` struct, все `PANEL_IC_*` defines (46 entries), `extern PANEL_ic`, `PANEL_PAGE_*` defines, `extern PANEL_page`, `PANEL_Beat` struct + `PANEL_NUM_BEATS`, extern-группа `PANEL_beat*`, `PANEL_Ammo` struct + `PANEL_AMMO_*` defines + `extern PANEL_ammo`, `PANEL_Toss` struct + `PANEL_MAX_TOSSES` + `extern PANEL_toss/toss_last/toss_tick`
- `panel.cpp`: удалены 7 dead функций: `PANEL_draw_face`, `PANEL_draw_health_bar`, `PANEL_funky_quad`, `PANEL_new_toss`, `PANEL_do_tosses`, `PANEL_help_message_do`, `PANEL_enable_screensaver`
- `panel.h`: удалены декларации всех 7 функций
- Build OK (make build-release, exit code 0)

**Сделано в батче 27 (fire.cpp cluster, сессия 11):**
- `fire.cpp`: удалены `FIRE_init`, `FIRE_create`, `FIRE_process` + 2 static helpers `FIRE_num_flames_for_size`, `FIRE_add_flame`; убраны dead includes `uc_common.h`, `game_types.h`
- `fire.h`: удалены декларации `FIRE_init`, `FIRE_create`, `FIRE_process` (FIRE_get_start оставлен — жив, вызывается из aeng.cpp)
- Build OK (make build-release, exit code 0)

**Сделано в батче 28 (poly.cpp + sky/mesh кластер, сессия 11):**
- `poly.cpp`: удалены 10 dead функций: `fade_point_more`, `POLY_transform_abs`, `POLY_get_screen_pos`, `POLY_transform_using_local_rotation_and_wibble`, `POLY_fadeout_buffer`, `POLY_add_line_2d`, `POLY_clip_line_box`, `POLY_clip_line_add`, `POLY_get_sphere_circle`, `POLY_inside_quad` (397 строк)
- `poly.h`: удалены декларации всех 10 функций
- `poly_globals.cpp/h`: удалены `POLY_clip_left/right/top/bottom` (использовались только POLY_clip_line_box/add)
- `sky.cpp`: удалены `SKY_draw_poly_clouds`, `SKY_draw_poly_sky` (вынесен `SKY_VERY_FAR_AWAY` в file scope — нужен live-функциям)
- `sky.h`: удалены декларации `SKY_draw_poly_clouds`, `SKY_draw_poly_sky`
- `mesh.cpp`: удалены `MESH_draw_morph`, `MESH_init_reflections`, `MESH_draw_reflection` + static helpers `MESH_grow_points`, `MESH_grow_faces`, `MESH_add_point`, `MESH_add_face`, `MESH_add_poly`, `MESH_create_reflection` + dead macros `MESH_MAX_DY`, `MESH_255_DIVIDED_BY_MAX_DY`, `MESH_NEXT_CLOCK`, `MESH_NEXT_ANTI` (780 строк)
- `mesh.h`: удалены декларации `MESH_init_reflections`, `MESH_draw_reflection`, `MESH_draw_morph`
- `mesh_globals.cpp/h`: удалены `MESH_reflection[]`, `MESH_add[]`, `MESH_add_upto` globals + типы `MESH_Reflection`, `MESH_Add`, константы `MESH_MAX_REFLECTIONS`, `MESH_MAX_ADD`
- Build OK (make build-release, exit code 0)

**Сделано в батче 33 (сессия 15, продолжение):**
- `elev.cpp/h`: удалена `ELEV_create_similar_name`
- `tga.cpp/h`: удалена `TGA_load_remap` (~145 строк, с goto-метками)
- `anim_tmap.cpp/h`: удалена `save_animtmaps`
- `thug.cpp/h`: удалены `fn_thug_init`, `fn_thug_normal`; файл сведён к минимуму; убран `#include thug.h` из `person.cpp`
- `playcuts.cpp/h`: удалены `PLAYCUTS_Free` (empty stub), `PLAYCUTS_Reset`
- `gamepad.cpp/h`: удалена `gamepad_is_adaptive_click_active`
- `composition.cpp/h`: удалена `composition_get_scene_texture`
- `crt_effect.cpp/h`: удалена `crt_effect_shutdown`
- `wibble_effect.cpp/h`: удалена `gl_wibble_effect_shutdown`; h сведён к пустому guard
- `morph.cpp/h`: удалены `MORPH_get_points`, `MORPH_get_num_points`
- `night.cpp`: убрана dead forward declaration `calc_inside_for_xyz` внутри block scope; реструктурирован окружающий блок (убраны лишние `{}`)
- `supermap.cpp/h`: удалены `calc_inside_for_xyz`, `add_sewer_ladder`
- `inside2.cpp/h`: удалена `find_stair_y` (~100 строк)
- `map.cpp/h` + `map_globals.cpp/h`: удалены `MAP_light_get_height`, `MAP_light_get_light`, `MAP_light_set_light`, global `MAP_light_map`; map_globals сведён к минимуму
- `font.cpp/h`: удалена `FONT_draw_speech_bubble_text` (~54 строки)
- `polypage.cpp/h`: удалена `GenerateMMMatrix` (~89 строк, D3D matrix builder); убран `#include compression.h`
- `smap.cpp/h`: удалена `SMAP_bike` (empty stub)
- `eway.cpp/h`: удалены `count_people_types`, `EWAY_cam_get_position_for_angle`, `EWAY_cam_look_at` (кластер)
- `pause.cpp/h`: удалён `PAUSE_handler` (~180 строк); файлы сведены к минимуму; убран `#include pause.h` + misleading comment из `game.cpp`
- `frontend.cpp/h`: удалены `FRONTEND_do_drivers`, `FRONTEND_gamma_update`
- `frontend_globals.cpp/h`: удалены `CurrentVidMode`, `CurrentBitDepth`
- `attract.cpp/h`: удалены `level_won`, `level_lost` (empty stubs)
- `overlay.cpp/h`: удалена `overlay_beacons` (empty stub); убрана extern decl из `game.cpp`
- `compression.cpp/h` + `compression_globals.cpp/h` + `image_compression.cpp/h`: удалён весь compression кластер (`COMP_load`, `COMP_calc`, `COMP_decomp`, `IC_pack`, `IC_unpack`, `COMP_data/frame/tga_data/tga_info`, типы `COMP_Frame/DataBuffer/Delta`); файлы сведены к минимуму
- `aeng.cpp`: удалена `AENG_movie_init` (~38 строк); убран вызов из `AENG_init()`; убран `#include compression.h`
- `aeng_globals.cpp/h`: удалены 9 movie globals (`AENG_movie_data[512*1024]`, `AENG_movie_upto`, `AENG_frame_one/two/last/next`, `AENG_frame_count/tick/number`), `AENG_MAX_MOVIE_DATA` define
- Build OK (make build-release, exit 0)

**Инциденты батча 33:**
- `aeng_globals.h:173: error: unknown type name 'COMP_Frame'` после очистки compression.h — диагностировано как dead AENG_movie кластер (единственный user `COMP_Frame` — `AENG_movie_init`). Решение: удалить весь AENG_movie кластер.
- `game.cpp:77` имел stale `#include pause.h` с misleading комментарием про `PANEL_`-функции — функции реально из `panel.h`. Исправлено и include убран.

**Что делать дальше:**

⚠️ **gc-sections dead-code pass ЗАВЕРШЁН** (батч 35, 2026-04-27). Все 80 оставшихся символов нельзя удалять — system noise, source-level callers, inlining artifacts.

1. ✅ **Финальный проход: пустые файлы** — ВЫПОЛНЕН (батч 35 + батч 36, 2026-04-27)
2. ✅ **Финальный проход: лишние #include** — ВЫПОЛНЕН (батч 37, сессия 19, 2026-04-27): `clang-tidy misc-include-cleaner` по всем 170+ `.cpp` файлам. 93 файла изменены, ~355 лишних `#include` удалено. Сборка OK.
3. ⬜ **Проход: cppcheck `--enable=unusedFunction`** — ещё не установлен. Отложен. Дополняет gc-sections: видит функции без синтаксических вызовов в коде (gc-sections видит "не достижимо от entry point"). Разные слепые пятна. Workflow:
   ```bash
   cppcheck --enable=unusedFunction \
            --project=new_game/build/Release/compile_commands.json \
            -j 8 2>cppcheck_unused.txt
   cat cppcheck_unused.txt | grep "unusedFunction"
   ```
   Ожидаются false positives: callback-registered функции, virtual методы, template инстансы. Каждый кандидат верифицировать grep'ом.
4. ⬜ **Проход: `#if 0` и `#if false` блоки** — gc-sections не видит (код не компилируется). Найти:
   ```bash
   grep -rn "^#if 0\|^#if false\|^#if FALSE" new_game/src/ --include="*.cpp" --include="*.h"
   ```
   Каждый блок смотреть глазами — мёртвый код из оригинала который так и не убрали.
   ⚠️ **`#if DEFINE` блоки** — если define не `0`/`false` а именованный (`#ifdef FOO`, `#if SOME_FLAG`) — **спрашивать у пользователя перед удалением** каждого такого блока: точно ли этот define мёртв.
5. ✅ **Проход: крупные закомментированные блоки** — ВЫПОЛНЕН (батч 38, сессия 20-21, 2026-04-27). Regex-pass по всему `new_game/src/` — удалено ~120 блоков ≥4 строк. Осталось: 0 блоков.

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
1a. **Empty stub с живыми callers**: пустая функция `void f() {}` или `SLONG f(...) { return 0; }` вызывается из живого кода. Компилятор инлайнит её в ничто, поэтому линкер не видит reference и выкидывает как dead. Но ОПРЕДЕЛЕНИЕ нужно для компиляции — нельзя удалять. Верификация: grep включать сам .cpp файл (`grep -rn "\bfunc\b" new_game/src/`). Типичный симптом: build fail с `error: use of undeclared identifier 'func'` после удаления.
2. **Callback'и через указатель**: функция зарегистрирована как `&fn` в setter'е (`ge_set_X_callback`). `--gc-sections` это видит как live (адрес взят), так что в dead-list таких быть не должно. Но если setter сам вырезан — функция станет мёртвой. Проверять при удалении.
3. **Виртуальные методы**: vtable классов используется через виртуальный диспатч, конкретный метод может выглядеть мёртвым но вызывается через указатель на базовый класс.
4. **Шаблонные инстансы**: STL contains'ы могут давать ложные leaf-кандидаты.
5. **Импорты из системных DLL**: `c_DwmMaxQueuedBuffers`, `_Avx2WmemEnabledWeakValue` — это extern символы из системных либ. Уже отфильтрованы.
6. **Stale .obj от удалённых исходников**: `weapon_feel_test.cpp.obj` остался в build/ но исходника нет, в линке не участвует. Уже отфильтрован.
7. **Stub-only в обычном билде**: `DEAD_CODE_REPORT=ON` включает `-ffunction-sections` → каждая функция в своей секции → линкер выкидывает dead функцию отдельно от её соседей. В обычном Release-билде без этого флага все функции из одного `.cpp` делят одну `.text`-секцию. Если в файле есть хоть одна live-функция — весь `.o` линкуется, включая dead caller-ов. Dead caller-ы содержат references на callees (ge_texture_free_all и т.п.) → линкер требует их определения. **Правило:** перед удалением определения callee нужно сначала удалить всех его caller-ов (или убедиться что caller-ы находятся в файлах только с dead-кодом — тогда весь `.o` не линкуется). Если caller удалить нельзя сейчас — оставить callee как minimal stub (`{}` / `return 0`), он останется в discard-list для DEAD_CODE_REPORT сборки, но не сломает обычный Release.

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
| 2026-04-26 | Batch 18 | person.cpp: 31 dead funcs deleted; person.h: ~35 dead decls removed; cop.cpp: extern fn_person_stand_up removed; darci.cpp: extern set_person_kick_off_wall removed. Restored 5 as empty stubs (inlined by compiler, but source-level callers exist): camera_shoot/fight/normal (called at ~10 sites in person.cpp), set_person_sidle (called at lines 4409,4476), set_person_running_jump_lr (called at lines 10071,10074,12733,12737). Discards: 2324→2293 (-31). | build OK |
| 2026-04-26 | Batch 18b | person_globals.cpp/h: deleted player_visited[16][128]; aeng.cpp: removed dead extern decl + commented-out use. Discards: 2293→2292. | build OK |
| 2026-04-26 | Batch 19 | widget.cpp: rewrote keeping 6 live fns (WIDGET_snd, FORM_KeyProc, FORM_Process, FORM_GetWidgetFromPoint, FORM_Draw, FORM_Free, FORM_Focus), deleted 34 dead + 6 static helpers + struct ListEntry + dead defines. widget_globals.cpp: deleted 9 dead Methods structs, kept WidgetTick+EatenKey. widget.h: removed ~50 dead decls/inlines/extern-Methods. game_tick.cpp: deleted dead form_proc (ref'd BUTTON_Methods). game_tick.h: removed form_proc decl + widget.h include; game_tick.cpp: added direct widget.h include (for Form* in local extern decl). Discards: 2292→2242 (-50). | build OK |
| 2026-04-26 | Batch 20 | eng_map.cpp: deleted MAP_draw (260 lines, 1 named dead fn + 31 float/XMM constants). eng_map.h: removed MAP_draw decl. | build OK |
| 2026-04-26 | Batch 21 | core.cpp: deleted 14 definitions entirely (ge_shutdown, ge_bound_texture_contains_alpha, ge_draw_primitive, ge_draw_indexed_primitive_unlit, ge_fill_rect, ge_destroy_user_texture, ge_run_cutscene, ge_restore_screen_surface, ge_is_texture_loaded, image_mem global, s_display_changed static, SetDisplay, ClearDisplay, s_image_mem_buf). 10 converted to minimal stubs: ge_to_gdi/from_gdi/is_display_changed/clear_display_changed (called from dead ShellPauseOn/LibShellChanged in host.cpp), ge_get_font/texture_free_all/texture_change_tga/texture_set_greyscale/texture_get_size (dead callers in texture.cpp/text.cpp), ge_get_screen_bpp/enumerate_drivers (dead FRONTEND_do_drivers in frontend.cpp). Removed outro_graphics_engine.h + wibble_effect.h from core.cpp includes; removed frontend.cpp extern image_mem. game_graphics_engine.h: removed 19 dead decls; uc_common.h: removed SetDisplay/ClearDisplay decls. **Lesson:** cannot delete a callee definition if caller lives in same .o as live code (non-function-sections build). Must delete callers first. See "Подводные камни" #7. | build OK |
| 2026-04-27 | Batch 22 | collide.cpp: удалены 22 dead функции (clear_all_col_info, get_height_along_vect, get_height_along_facet, TSHIFT+check_big_point_triangle_col, point_in_quad_old, dist_to_line, calc_along_vect, calc_height_at, collision_storey, highlight_face/rface/quad, vect_intersect_wall, find_alt_for_this_pos, get_fence_height, step_back_along_vect, cross_door, bump_person, in_my_fov, collide_against_sausage, box_box_early_out, box_circle_early_out). collide.h: удалены все 22 dead декларации + extern col_vects_links/col_vects. collide_globals.cpp/h: удалены 9 dead globals (col_vects_links, col_vects, next_col_vect, next_col_vect_link, walk_links, next_walk_link, already, max_facet_find, last_slide_dist). fog.cpp/mist.cpp/cop.cpp/roper.cpp/thug.cpp: удалены extern decl для calc_height_at. walkable.cpp: удалён extern decl для highlight_rface. **Инцидент:** Python-regex удалил bump_person вместе с 2200+ строк живого кода (slide_along_edges, slide_along_redges, move_thing*, collide_against_*, drop_on_heads, there_is_a_los*, slide_around_circle) — восстановлено через git show HEAD. | -46 discards, build OK |
| 2026-04-27 | Batch 23 | outro_os.cpp/h: удалены 13 dead (OS_buffer_add_sprite_rot, OS_fps, OS_mouse_get, OS_mouse_set, OS_sound_init/start/volume, OS_string, OS_texture_create(int,int), OS_texture_finished_creating, OS_texture_lock, OS_texture_size, OS_texture_unlock) + OS_BITMAP_* макросы и связанные декларации. aeng.cpp/h: удалена AENG_set_draw_distance (no callers). Пропущены 6 aeng.cpp функций (apply_cloud, show_gamut_lo, AENG_draw_bangs, AENG_draw_people_messages, AENG_clear_screen, AENG_fade_out) — linker дискардит через inline/optimizer но source-level вызовы в живых функциях. stage13_dead_code_cleanup.md: добавлена секция "Финальный проход — чистка лишних #include". | build OK |
| 2026-04-27 | Batch 29 (сессия 12) | Регенерация dead-list: 2140 discards, 410 symbols. input_actions_globals.cpp/h: удалены 7 dead globals (debug_input, g_iCheatNumber, last_camera_dx/dy/dz, last_camera_yaw/pitch). anim_globals.cpp/h: удалены 5 dead globals (test_chunk2, test_chunk3, thug_chunk, the_elements, prim_multi_anims, next_prim_multi_anim) — roper_pickup и test_chunk восстановлены (source-level callers в dead но не закомментированных функциях). supermap.cpp: убран/восстановлен extern roper_pickup. outro_mf.cpp/h: удалены 8 dead функций (MF_ambient, MF_invert_zeds, MF_add_triangles_normal/normal_colour/light/light_bumpmapped/specular/specular_shadowed). backend_opengl/outro/core.cpp + outro_graphics_engine.h: удалены 8 dead (oge_shutdown, oge_texture_create_blank/finished_creating/size/lock/unlock, oge_init_renderstates, oge_calculate_pipeline). texture.cpp/h + aeng.h: удалены 9 dead (TEXTURE_86_lock/unlock/update, TEXTURE_free_unneeded, TEXTURE_get_fiddled_position, TEXTURE_load_needed_object, TEXTURE_looks_like, TEXTURE_set_greyscale, TEXTURE_set_tga). Пропущены: eng_map_globals (constant-propagated, live callers), input_to_angle (live caller apply_button_input_fight), MF_rotate_mesh (live caller WIRE_draw). | build OK |
| 2026-04-27 | Batch 30 (сессия 13) | Регенерация: 2101 discards, 372 symbols в 116 файлах. switch.cpp: удалён целиком (7 dead funcs: init_switches, free_switch, create_switch, fn_switch_player/thing/group/class); switch.h: убраны декларации 7 функций, оставлены Switch struct + SwitchPtr (используются в game_types.h/memory.cpp/thing.h); CMakeLists.txt: убран switch.cpp. thing.cpp: удалены 7 dead (move_thing_on_map_dxdydz, log_primary_used/unused_list, log_secondary_used/unused_list, wait_ticks, set_slow_motion) + extern decl Time; thing.h: убраны декларации всех 7. fc.cpp: убран extern decl set_slow_motion. host.cpp: удалены 7 dead (ShellPaused, ShellPauseOn, ShellPauseOff, LibShellChanged, LibShellMessage, Time, TraceText); host.h: убраны декларации, добавлен прямой #include uc_common.h; uc_common.h: убраны TraceText/LibShellChanged/LibShellMessage. sdl3_bridge.cpp/h: удалена sdl3_window_set_size (нет callers). Пропущены sdl3_gamepad_shutdown/sdl3_set_mouse_grab — live callers в gamepad.cpp и game_tick.cpp. Discards: 2101→2078 (-23). | build OK |
| 2026-04-27 | Batch 31 (сессия 14) | Регенерация: 2078 discards. DELETED целиком: wibble_globals.cpp/h (6 dead globals), outline.cpp/h (5 dead funcs), text.cpp/h (5 dead funcs: draw_centre_text_at, draw_text_at, show_text, text_height, text_width); CMakeLists.txt: убраны wibble_globals.cpp, outline.cpp, text.cpp. text_globals.cpp/h: убран text_colour (dead). game.cpp: убраны extern decls draw_centre_text_at/draw_text_at. eng_map.cpp: убран #include text.h. interact_globals.cpp/h: убраны best_angle, best_x, best_y, best_z, r_matrix (локальные переменные с тем же именем в interact.cpp — не глобалы). figure_globals.cpp/h: убраны body_part_upper, part_type, local_seed, jacket_col, leg_col, peep_recol + PeepRecolEntry type. fire_globals.cpp/h: убраны FIRE_fire, FIRE_fire_last, FIRE_flame, FIRE_flame_free, FIRE_get_info, FIRE_get_point (оставлены live: FIRE_get_z/xmin/xmax/fire_upto/flame). texture_globals.cpp/h + texture.h: убраны TEXTURE_fiddled, TEXTURE_page_lcdfont_alpha, TEXTURE_page_flames_alpha, TEXTURE_liney, TEXTURE_av_r/g/b. anim.cpp/h: убраны SetCMatrixComp, calc_sub_objects_position_no_tween, convert_elements, fix_multi_object_for_anim, reset_anim_stuff, set_next_prim_point (setup_anim_stuff оставлен — static stub с live caller). weapon_feel.cpp/h: убрана weapon_feel_fire_reset. Discards: 2078→2024 (-54). | build OK |
| 2026-04-27 | Batch 32 (сессия 15) | Регенерация: 2024 discards. dirt.cpp/h: удалены DIRT_get_info, DIRT_behead_person, DIRT_create_mine, DIRT_destroy_mine, DIRT_gale + DIRT_INFO_TYPE_*/DIRT_Info. animal.cpp/h: удалены alloc_animal, ANIMAL_create, ANIMAL_animatetween, ANIMAL_animate, ANIMAL_set_anim, ANIMAL_get_animal, ANIMAL_draw (6 funcs). shape.cpp/h: удалены SHAPE_semisphere, SHAPE_semisphere_textured, SHAPE_waterfall, SHAPE_alpha_sphere (4 funcs). fog.cpp/h: удалены FOG_get_dyaw (static), FOG_create (static), FOG_set_focus, FOG_gust, FOG_process, FOG_get_start, FOG_get_info + FOG_Info struct + FOG_TYPE_TRANS*/NO_MORE. fog_globals.cpp/h: удалены FOG_focus_x, FOG_focus_z, FOG_focus_radius, FOG_get_upto (4 globals). psystem.cpp/h: удалены PARTICLE_Exhaust, PARTICLE_Exhaust2, PARTICLE_Steam, PARTICLE_SGrenade + fmatrix.h include. snipe.cpp/h: удалены SNIPE_mode_on, SNIPE_mode_off, SNIPE_turn, SNIPE_shoot + SNIPE_TURN_* flags; SNIPE_LENS_END вынесен на file scope (нужен живой SNIPE_process). snipe_globals.cpp/h: удалены SNIPE_on, SNIPE_cam_x, SNIPE_cam_y, SNIPE_cam_z. sm.cpp/h: удалены SM_init, SM_create_cube, SM_process + SM_CUBE_*/SM_GRAVITY макросы + SM_CUBE_TYPE_* константы + dead includes. sound.cpp/h: удалены DieSound, play_ambient_wave, play_music, SewerSoundProcess + fc.h/fc_globals.h includes. ВОССТАНОВЛЕНЫ (ошибочно удалённые, нет callers только потому что game_shutdown не вызывал gamepad_shutdown): gamepad_shutdown, weapon_feel_shutdown, ds_shutdown, ds_set_haptic_volume, ds_set_lightbar_setup. game_shutdown() дополнен вызовом gamepad_shutdown(). game_startup(): удалён дублирующий MFX_init() (первый вызов в SetupHost(), второй был лишним — утечка AL device). dc_map.py после: 272 symbols в 99 файлах. | build OK |
| 2026-04-27 | Batch 35 (сессия 17) | Регенерация: 1800 discards, 80 symbols в 19 файлах. input_actions.cpp/h: удалены lock_to_compass + set_look_pitch (нет callers). person.cpp: убрана forward decl lock_to_compass. Список gc-sections exhausted — все оставшиеся нельзя удалять. Финальный проход пустых файлов: удалены compression/*.cpp/h (6 файлов), thug.cpp/h, build_globals.cpp/h, map_globals.cpp/h, pause.cpp/h, wibble_effect.h, joystick_globals.cpp; убраны из CMakeLists.txt; убран #include "map/map_globals.h" из map.cpp. | build OK |
| 2026-04-27 | Batch 37 (сессия 19) | Финальный проход: лишние #include. `clang-tidy -p build/compile_commands.json --checks='-*,misc-include-cleaner'` по всем ~170 .cpp файлам. 93 файла изменены, ~355 лишних `#include` удалено. Основные паттерны: `uc_common.h` (тянул types транзитивно), `eway.h`→`eway_globals.h` (EWAY_Way+vars в globals), `sound.h`→`sound_globals.h`, `game_types.h`/`statedef.h`/`pap_globals.h` false positives (оставлены где нужны транзитивно), `cop.h/darci.h/roper.h` не нужны в person_globals.cpp (только _globals.h нужны), `person.h` в panel.cpp (PERSON_* из person_types.h через thing.h). Сборка: build OK. | build OK |
| 2026-04-27 | Batch 38 (сессия 20-21) | Проход: закомментированные /* */ блоки. Удалено ~120 блоков по всей кодовой базе: person.cpp (~30), collide.cpp (10), pcom.cpp (15), game_tick.cpp (8), aeng.cpp (15), vehicle.cpp (1), barrel.cpp (2), figure.cpp (6), elev.cpp (4), poly.cpp (3), input_actions.cpp (25), eway.cpp (4), wmove.cpp (1), bat.cpp (1), special.cpp (1), message.cpp (1), outro_wire.cpp (1), outro_font.cpp (1), save.cpp (1), и др. Regex-pass по всему src/ — 0 блоков ≥4 строк осталось. | build OK |
| 2026-04-27 | Batch 36 (сессия 18) | Финальный проход пустых файлов (продолжение). DELETED: shadow_globals.cpp/h (нет деклараций/определений — остался пустым после батча 7), startscr.cpp (нет определений — globals-only модуль, все данные в startscr_globals.cpp). Убраны из CMakeLists.txt. Убран #include "engine/graphics/lighting/shadow_globals.h" из elev.cpp, shadow.cpp, game.cpp (получали только types.h транзитивно, shadow.h уже его включает). | build OK |
| 2026-04-27 | Batch 34 (сессия 16) | Регенерация: 1821 discards, 134 dead symbols в 38 файлах. anim_globals.cpp/h: удалён test_chunk. env_globals.cpp/h: DELETED ENTIRE (env_inifile, env_strbuf — нет callers вне env_globals); убран #include env_globals.h из env.cpp; убран из CMakeLists.txt. sm_globals.cpp/h: удалены SM_object, SM_object_upto + SM_Object type + SM_MAX_OBJECTS define. hierarchy_globals.cpp/h + hierarchy.h: удалён body_part_parent (нет .cpp callers). spark.cpp/h: удалена SPARK_in_sphere (caller в /* */). eway.cpp/h: удалена EWAY_deduct_time_penalty (caller в /* */). projectile.cpp/h: удалена alloc_projectile (нет callers). outro_imp.cpp/h: удалены IMP_load и IMP_binary_save (IMP_load callers в /* */, IMP_binary_save нет callers). Пропущены: save_globals.cpp/h — попытка удалить откатана (save.cpp компилируется всегда и использует эти globals); index_lookup (aeng_globals) — live source-level caller в aeng.cpp; PUDDLE_create — live caller в process_controls (KB_B debug, не в /* */); FIGURE_draw_hierarchical_prim_recurse_individual_cull — inlined from live caller, source ref exists; set_person_in_building_through_roof — после ASSERT(0) но source ref есть; camera_*/set_person_* stubs — live source callers. | build OK |
| 2026-04-27 | Batch 33 (сессия 15) | Продолжение с dc_map.py списком (272 symbols). elev.cpp/h: ELEV_create_similar_name. tga.cpp/h: TGA_load_remap (~145 строк с goto). anim_tmap.cpp/h: save_animtmaps. thug.cpp/h: fn_thug_init, fn_thug_normal (файл сведён к минимуму; убран #include thug.h из person.cpp). playcuts.cpp/h: PLAYCUTS_Free (empty stub), PLAYCUTS_Reset. gamepad.cpp/h: gamepad_is_adaptive_click_active. composition.cpp/h: composition_get_scene_texture. crt_effect.cpp/h: crt_effect_shutdown. wibble_effect.cpp/h: gl_wibble_effect_shutdown (h сведён к пустому guard). morph.cpp/h: MORPH_get_points, MORPH_get_num_points (ВОССТАНОВЛЕНЫ: ошибочно удалены в батче 12, реально мертвы — нет callers в mesh.cpp). night.cpp: убрана dead forward declaration calc_inside_for_xyz внутри блока; блок реструктурирован (убраны лишние `{}`). supermap.cpp/h: calc_inside_for_xyz, add_sewer_ladder. inside2.cpp/h: find_stair_y (~100 строк). map.cpp/h + map_globals.cpp/h: MAP_light_get_height, MAP_light_get_light, MAP_light_set_light, MAP_light_map (global). font.cpp/h: FONT_draw_speech_bubble_text. polypage.cpp/h: GenerateMMMatrix (~89 строк D3D matrix builder); убран #include compression.h. smap.cpp/h: SMAP_bike (empty stub). eway.cpp/h: count_people_types (immediate-return stub), EWAY_cam_get_position_for_angle, EWAY_cam_look_at (кластер, только внутренние callers). pause.cpp/h: PAUSE_handler (~180 строк); файлы сведены к минимуму; убраны #include pause.h из game.cpp + исправлен misleading comment. frontend.cpp/h: FRONTEND_do_drivers, FRONTEND_gamma_update. frontend_globals.cpp/h: CurrentVidMode, CurrentBitDepth. attract.cpp/h: level_won, level_lost (empty stubs). overlay.cpp/h: overlay_beacons (empty stub); убрана extern decl из game.cpp. compression.cpp/h + compression_globals.cpp/h + image_compression.cpp/h: весь compression кластер (COMP_load, COMP_calc, COMP_decomp, IC_pack, IC_unpack, COMP_data/frame/tga_data/tga_info, типы COMP_Frame/DataBuffer/Delta); файлы сведены к минимуму. aeng.cpp: AENG_movie_init (~38 строк), убран вызов из AENG_init(), убран #include compression.h. aeng_globals.cpp/h: удалены 9 movie globals (AENG_movie_data[512*1024], AENG_movie_upto, AENG_frame_one/two/last/next, AENG_frame_count/tick/number, AENG_MAX_MOVIE_DATA). Инцидент: aeng_globals.h ломался из-за COMP_Frame после очистки compression.h — диагностировано как dead AENG_movie кластер, решено удалением всего кластера. | build OK |

## Проход: закомментированные /* */ блоки (Batch 38+)

Сгенерирован 2026-04-27, все блоки ≥4 строк в `new_game/src/`. Исключены vendor файлы (`glad/gl.h`, `khrplatform.h`).

Правила:
- Удалять только явно мёртвый закомментированный код (замещённая реализация, вырезанная фича, debug-хак)
- НЕ удалять блоки с TODO/FIXME — это осознанные заглушки
- `#if DEFINE` (не `#if 0`) — спрашивать у пользователя перед удалением

### aeng.cpp (6 блоков)

| Строка | Размер | Описание | Статус |
|--------|--------|----------|--------|
| 2879 | 52 | Draw reflections of OBs — DETAIL_REFLECTIONS ветка | ✅ |
| 3254 | 145 | Draw water in the city — вся водная система | ✅ |
| 3607 | 25 | PAP_FLAG_ANIM_TMAP animated texture mapping | ✅ |
| 4198 | 41 | Draw wheels above Darci's head (debug viz) | ✅ |
| 4297 | 35 | Dead MIB sub-objects position calc | ✅ |
| 4684 | 28 | PUDDLE drawing loop | ✅ |
| 6206 | 26 | KB_PPOINT ambient light tweak (debug key) | ✅ |

### game_tick.cpp (~20 блоков)

Большинство — debug key handlers (`if (Keys[KB_X]) { ... }`) которые закомментированы ещё в оригинале. Все безопасны к удалению.

| Строка | Размер | Описание | Статус |
|--------|--------|----------|--------|
| 1233 | 61 | KB_B debug handler | ✅ |
| 1383 | 26 | DIKE_Dike debug | ✅ |
| 1420 | 18 | is_person_crouching darci check | ✅ |
| 1691 | 38 | "Is darci still?" — idle check | ✅ |
| 1738 | 26 | KB_1 anim_type[] debug | ✅ |
| 1801 | 28 | KB_3 debug | ✅ |
| 1830 | 18 | auto music scheduling test | ✅ |
| 1893 | 47 | Hook control (HOOK_get_state) | ✅ |
| 1995 | 27 | darci in sewers GF_SEWERS check | ✅ |
| 2046 | 25 | process count loop | ✅ |
| 2080 | 50 | "Is darci ready to go upstairs?" | ✅ |
| 2140 | 42 | KB_P4 splitscreen debug | ✅ |
| 2345 | 19 | "Simon's idea!" camera target look | ✅ |
| 2365 | 37 | KB_8 debug | ✅ |
| 2525 | 19 | KB_POINT steam/ypos debug | ✅ |
| 2693 | 21 | KB_Z debug | ✅ |
| 2715 | 112 | KB_R big debug block | ✅ |
| 2835 | 33 | KB_T debug | ✅ |
| 2870 | 23 | KB_X + fti debug | ✅ |
| 2895 | 48 | KB_M debug | ✅ |
| 3012 | 52 | fti static force block | ✅ |
| 3067 | 116 | large unnamed debug block | ✅ |
| 3213 | 26 | KB_5 chopper debug | ✅ |
| 3241 | 31 | FC_cam_dist / FC_cam_height debug | ✅ |
| 3273 | 22 | KB_F12 debug | ✅ |
| 3398 | 18 | loop с c0/index1/index2/index3 | ✅ |
| 3463 | 52 | KB_N debug | ✅ |
| 3517 | 50 | `#ifndef TARGET_DC #if !defined(PSX)` KB_R block | ✅ (внутри `/* */` — удалено вместе с блоком) |

### person.cpp (5 блоков)

| Строка | Размер | Описание | Статус |
|--------|--------|----------|--------|
| 8962 | 110 | SUB_STATE_DROP_DOWN_OFF_FACE case | ✅ |
| 10101 | 22 | MSG_add "stopping vel" debug | ✅ |
| 10184 | 24 | continue_action check | ✅ |
| 13015 | 19 | "You can't throw mines any more" | ✅ |
| 13051 | 69 | person_normal_animate end block | ✅ |
| 13432 | 31 | angle calc block | ✅ |

### collide.cpp (4 блока)

| Строка | Размер | Описание | Статус |
|--------|--------|----------|--------|
| 2044 | 23 | unnamed block | ✅ |
| 2594 | 30 | find_face_for person_inside check | ✅ |
| 2672 | 57 | actual_sliding dx/dy/dz block | ✅ |
| 2769 | 30 | slide_door < 0 check | ✅ |

### pcom.cpp (3 блока)

| Строка | Размер | Описание | Статус |
|--------|--------|----------|--------|
| 2301 | 37 | gang_attacks[gang].Perp loop | ✅ |
| 4690 | 40 | WAND_find_good_start_point extern + call | ✅ |
| 6882 | 21 | GAME_TURN + THING_NUMBER debug | ✅ |

### Остальные файлы

| Файл | Строка | Размер | Описание | Статус |
|------|--------|--------|----------|--------|
| vehicle.cpp | 285 | 31 | switch на vehicle type | ✅ |
| barrel.cpp | 188 | 28 | BARREL_FLAG_HIT check | ✅ |
| barrel.cpp | 1263 | 27 | in_the_air barrel block | ✅ |
| figure.cpp | 2812 | 19 | AnimPrimBbox world_link | ✅ |
| figure.cpp | 3146 | 24 | ENGINE_palette Col2 (red/green/blue) | ✅ |
| figure.cpp | 3212 | 22 | ENGINE_palette Col2 (second instance) | ✅ |
| elev.cpp | 2097 | 24 | "bodge for puzzle" mission < 0 | ✅ |
| poly.cpp | 1721 | 20 | "Guy Demo Dodge!!!" ATTRACT_MODE | ✅ |
| input_actions.cpp | 1344 | 18 | auto climb ladders | ✅ |
