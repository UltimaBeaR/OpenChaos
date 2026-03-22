# Лог Этапа 4 — Реструктуризация кодовая базы

## Итерация 117 — world/environment/building (Building.cpp чанк 1: helpers, edge scanner → new/) (2026-03-22)

- `fallen/Source/Building.cpp` (9878 строк) — чанк 1 (lines 1-1520) → `new/world/environment/building.cpp` + `building.h` + `building_globals.cpp` + `building_globals.h`.
- `insert_collision_vect` — в пре-релизной кодовой базе stub-заглушка (тело пустое, return 0); сохранено 1:1.
- `add_tri_to_walkable_list` — тело закомментировано ("wrong wrong wrong") в оригинале; сохранено 1:1.
- `INSIDE`/`ON_EDGE`/`OUTSIDE` → `BUILDING_INSIDE`/`BUILDING_ON_EDGE`/`BUILDING_OUTSIDE` (конфликт имён с системными/оконными макросами).
- `end_prim_point` и `end_prim_face4` инициализированы в 0 (оригинал: `MAX_PRIM_POINTS - 2` / `MAX_PRIM_FACES4 - 2`), т.к. это runtime-выражения через `save_table[]`, не линк-тайм константы.
- `texture_xy2[]` — чисто локальная для чанка 2 lookup-таблица, оставлена в `old/Building.cpp`.
- Сигнатура `place_building_at` исправлена в `old/fallen/Headers/building.h` (4-param → 5-param).
- Temporary includes: `game.h`, `fallen/Headers/pap.h`, `fallen/Headers/supermap.h`, `fallen/Headers/memory.h`, `pap.h`, `fallen/Headers/collide.h`, `engine/graphics/pipeline/aeng.h`.
- `old/Building.cpp` оставлен (chunk 2+, lines 1521-9878), redirect-заголовки добавлены в начало.

---

## Итерация 116 — effects/dirt (dirt.cpp → redirect) (2026-03-22)

- `fallen/Source/dirt.cpp` (2828 строк) → `new/effects/dirt.cpp` + `dirt.h` + `dirt_globals.cpp` + `dirt_globals.h`.
- `DIRT_gale_height` и `DIRT_gale` в оригинале в `/* */` (мёртвый код); `DIRT_gale` — добавлена stub-реализация (объявлена в заголовке).
- `DIRT_Tree` тип — локальная struct из dirt.cpp; вынесена в `dirt_globals.h`.
- `TICK_SHIFT_LOWRES` — локальный макрос, оставлен в `dirt.cpp`.
- `DIRT_create_brass` — не был объявлен в оригинальном `dirt.h`; добавлен в `new/effects/dirt.h` (Person.cpp ссылался через `extern`).
- `static int tick` внутри `DIRT_process` — local static, остался на месте (не глобал).
- Temporary includes: `Game.h` (GAME_FLAGS, calc_angle, NET_PERSON), `mav.h` (MAV_SPARE), `prim.h` (PRIM_FLAG_TREE, PRIM_OBJ_CAN), `sample.h` (S_KICK_CAN), `Sound.h` (world_type), `poly.h` (POLY_PAGE_EXPLODE*), `animate.h` (SUB_OBJECT_*).

---

## Итерация 115 — world/environment/prim (Prim.cpp → redirect) (2026-03-22)

- `fallen/Source/Prim.cpp` (2595 строк) → `new/world/environment/prim.cpp` + `prim.h` + `prim_globals.cpp` + `prim_globals.h`.
- `slide_around_box` не была объявлена в `collide.h` (только в `collide.cpp`) — добавлена декларация.
- `prim_info` уже был extern-объявлен в `missions/memory_globals.h` (pre-emptive) — определение в `prim_globals.cpp`; комментарий в `memory_globals.cpp` обновлён.
- `dont_slide` параметр в `slide_along_prim` присутствует в сигнатуре, но НЕ используется в теле оригинала (dead feature). Сохранён 1:1.
- `actors/` includes (`thing.h`, `vehicle.h`, `person.h`) — cross-layer deps (world/ → actors/), помечены `// Temporary:`.
- Локальные макросы `SLIDE_EDGE_HEIGHT`, `FURN_UNDERNEATH_BOT/TOP`, `JUST_IN_CASE`, `USED_POINT/FACE3/FACE4` — в `prim.cpp` (скопированы из тел функций оригинала, не публичные).
- `old/fallen/Source/Prim.cpp` → redirect-заглушка.

---

## Итерация 114 — world/environment/ns (ns.cpp чанк 2: walls, curves, cache, queries → old/ns.cpp redirect) (2026-03-22)

- `fallen/Source/ns.cpp` завершён: lines 1179–2373 → `new/world/environment/ns.cpp`.
- Мигрированы: `NS_cache_create_wallstrip`, `NS_cache_create_extra_bit_left/right`, `NS_cache_create_walls`, `NS_cache_create_curve_sewer`, `NS_cache_create_curve_top`, `NS_cache_create_curves`, `NS_cache_create_falls`, `NS_cache_create_grates`, `NS_cache_create`, `NS_cache_destroy`, `NS_cache_fini`, `NS_calc_height_at`, `NS_calc_splash_height_at`, `NS_slide_along`, `NS_inside`, `NS_there_is_a_los`, `NS_get_unused_st`, `NS_add_ladder`, `NS_add_prim`.
- `NS_slide_along` использует `slide_around_sausage` (new/engine/physics/collide.h) — добавлен include + добавлена декларация в collide.h.
- `NS_there_is_a_los` — цикл без инкремента x/y/z в теле (баг оригинала, сохранён 1:1).
- `fall` в `NS_cache_create_grates` — объявлен, не используется (баг оригинала, сохранён 1:1).
- `old/fallen/Source/ns.cpp` → redirect-заглушка (3 строки).

---

## Итерация 113 — world/environment/ns (ns.cpp чанк 1: init, floors, wallstrip helpers) (2026-03-22)

- `fallen/Source/ns.cpp` (3544 строк) начинает миграцию. Чанк 1: lines 1–1178 → `new/world/environment/ns.cpp` + `ns.h` + `ns_globals.cpp` + `ns_globals.h`.
- `ns.h` (старый) → redirect-stub. Старый `ns.cpp` заменён chunk 2+ с includes на new/.
- Внутренние макросы (`NS_HI_CURVE_*`, `NS_TEXTURE_*`, `NS_NORM_*`, `NS_MAX_SCRATCH_*`) вынесены в `ns_globals.h` — shared между chunk 1 (new/) и chunk 2+ (old/).
- `NS_Slight` — тип, определённый только в старом cpp; вынесен в `ns_globals.h` (нужен extern для `NS_slight[]`).
- `NS_los_fail_x/y/z` — дублировались: удалены из old/ns.cpp (были на lines 2187–2189 оставшегося файла), определения в `ns_globals.cpp`.
- `NS_TEXTURE_NUMBER` — в ns_globals.h (shared constant); локальный `#define` в ns_globals.cpp удалён.
- Временные includes: `game.h` в `new/ns.cpp` и `old/ns.cpp` (ASSERT, WITHIN, NULL, rand, memset).

---

## Итерация 112 — missions/game (Game.cpp → redirect) (2026-03-22)

- `fallen/Source/Game.cpp` (1531 строк) → `new/missions/game.cpp` + `game.h` + `game_globals.cpp` + `game_globals.h`.
- `the_game` и `VIOLENCE` — определены в `game_globals.cpp`; уже объявлены как extern в `old/fallen/Headers/Game.h`.
- `playback_file`/`verifier_file` — определены в `actors/core/thing_globals.cpp` (мигрированы ранее с неверным origin `Thing.cpp` вместо верного).
- `player_pos` — дублировать нельзя; уже определён в `actors/core/player_globals.cpp`.
- `tick_ratios/wptr/number/sum` — в оригинале file-scope statics Game.cpp, перенесены в `game_globals` по правилу.
- `plan_view_shot` (многопараметровая версия из Game.cpp) — extern-декларация в game.cpp; Controls.cpp имеет другую (no-param) версию.
- `ShowBackImage/InitBackImage/ResetBackImage` — объявлены в `engine/graphics/graphics_api/gd_display.h`.
- `overlay.cpp`: исправлен неверный uc_orig для `draw_map_screen` (`interfac.cpp` → `Game.cpp`).
- `playcuts.cpp`: исправлены uc_orig для `hardware_input_continue` и `lock_frame_rate` (`playcuts.cpp` → `Game.cpp`).
- `thing.cpp`: исправлен uc_orig для `SmoothTicks` (`Headers/Game.h` → `Source/Game.cpp`).
- `old/fallen/Source/Game.cpp` заменён redirect-заглушкой (3 строки).

---

## Итерация 111 — engine/physics/collide (чанк 5b: финальный, old/collide.cpp → redirect) (2026-03-22)

- `fallen/Source/collide.cpp` чанк 5b: lines 7481–9155 → `new/engine/physics/collide.cpp` + `collide_globals.cpp/h`.
- Lines 7480–7479 (find_intersected_colvect в `/* */`) — мёртвый код, пропущен.
- `box_circle_early_out` — в оригинале только forward declaration без тела; добавлена stub-реализация в new/.
- `collide_box_with_line` — баг оригинала сохранён 1:1: в последнем `COL_CLIP_ZL` блоке `WITHIN(iz, ...)` вместо `WITHIN(ix, ...)`.
- `COLLIDE_fastnav` — новая глобальная переменная (тип `COLLIDE_Fastnavrow*`); тип и макрос `COLLIDE_can_i_fastnav` остались в `fallen/Headers/collide.h` (дублировать в `collide_globals.h` нельзя).
- Новые Temporary includes: `engine/graphics/pipeline/aeng.h` (AENG_world_line), `world/environment/build2.h` (add_facet_to_map), `world/map/pap.h` (PAP_calc_map_height_at).
- `old/fallen/Source/collide.cpp` заменён redirect-заглушкой (3 строки).

---

## Итерация 110 — engine/physics/collide (чанк 5a: LOS functions) (2026-03-22)

- `fallen/Source/collide.cpp` чанк 5a: lines 5960–7213 → `new/engine/physics/collide.cpp` + `collide_globals.cpp/h`.
- Lines 7215–7475 (old `there_is_a_los` в `/* */`) — мёртвый код, пропущен.
- Мигрированы: `save_stack`, `los_done[]`, `los_wptr`, `los_v_*`, `last_mav_square_x/z`, `last_mav_dx/dz`, `los_failure_x/y/z/dfacet` → `collide_globals.cpp/h`.
- Функции: `start_checking_against_a_new_vector`, `check_vector_against_mapsquare`, `check_vector_against_mapsquare_objects`, `there_is_a_los_things`, `there_is_a_los`, `there_is_a_los_mav`, `there_is_a_los_car`.
- `save_stack` — оригинал anonymous struct static, сделан `SaveStack` (named) + extern в `_globals` (нужна extern-linkage, имя не конфликтует).
- `there_is_a_los_mav` и `there_is_a_los_car` ранее ошибочно были в entity_map с неверными файлами (bat.cpp, Vehicle.cpp) — пересозданы с правильным путём `new/engine/physics/collide.h`.
- Новые Temporary includes: `ai/mav_globals.h` (MAV_opt), `ui/camera/fc.h` (FC_explosion, для chunk 5b), `actors/animals/bat.h` (BAT_apply_hit, для chunk 5b).

---

## Итерация 109 — engine/physics/collide (чанк 4: move_thing_quick/collide_against_objects/collide_against_things/drop_on_heads/move_thing) (2026-03-22)

- `fallen/Source/collide.cpp` чанк 4: lines 4514–5957 → `new/engine/physics/collide.cpp`.
- Lines 4366–4512 (`slide_along_redges` v2) — в `/* */`, мёртвый код, пропущен.
- Новые временные includes: `prim.h` (slide_along_prim, prim_get_collision_model, get_prim_info, PRIM_OBJ_SPIKE, PRIM_COLLIDE_*, FACE_FLAG_*), `world/map/ob.h` + `ob_globals.h` (OB_find, OB_avoid, OB_ob, OB_ob_upto), `actors/vehicles/vehicle.h`, `ai/pcom.h` + `combat.h`, `world/navigation/inside2.h`, `actors/items/barrel.h`, `effects/mist.h`, `fallen/Headers/dirt.h`.
- Глобальный `x1, my_y1, z1, x2, y2, z2` из old collide.cpp (line 5273–5274) — стали локальными переменными в `move_thing` (так же работает, т.к. используются только в move_thing).
- `col_with_things[]` + `MAX_COL_WITH` → collide_globals; `FALL_OFF_FLAG_*` → collide_globals.h (нужны снаружи).
- `collide_with_circle` — forward decl в collide.cpp, определение в old/collide.cpp (chunk 6+).

---

## Итерация 108 — engine/physics/collide (чанк 3: slide_along/cross_door/bump_person/slide_along_edges/redges) (2026-03-22)

- `fallen/Source/collide.cpp` чанк 3: lines 3174–4370 → `new/engine/physics/collide.cpp`.
- Новые временные includes: `darci_globals.h` (`just_started_falling_off_backwards`), `world/map/supermap.h` (`SUPERMAP_counter_increase`/`SUPERMAP_counter`), `world/map/supermap_globals.h` (`next_dfacet`), `assets/anim_globals.h` (`next_prim_face4`).
- `slide_along_old` (lines 2392–3168 в оригинале) — уже в `/*...*/`, мёртвый код, пропущен.

---

## Итерация 107 — engine/physics/collide (чанк 2: face/ladder/plant_feet/slide globals) (2026-03-22)

- `fallen/Source/collide.cpp` чанк 2: lines 1581–3174 → `new/engine/physics/collide.cpp` + `collide_globals.cpp/h`.
- Функции: `find_face_near_y`, `find_alt_for_this_pos`, `correct_pos_for_ladder`, `ok_to_mount_ladder`, `mount_ladder`, `set_person_climb_down_onto_ladder`, `find_nearby_ladder_colvect_radius`, `find_nearby_ladder_colvect`, `set_feet_to_y`, `height_above_anything`, `plant_feet`, `get_person_radius`, `get_person_radius2`, `get_fence_height`, `step_back_along_vect`.
- Глобалы → `collide_globals`: `already[]`, `max_facet_find`, `last_slide_colvect`, `last_slide_dist`, `actual_sliding`, `slide_door`, `slide_ladder`, `slide_into_warehouse`, `slide_outof_warehouse`, `slid_along_fence`, `fence_colvect`.
- Макросы `EXTRA_RADIUS`, `VEC_SHIFT`, `VEC_LENGTH` перенесены в `collide.cpp`; `MAX_ALREADY` — в `collide_globals.h` (во избежание circular include через collide.h→collide_globals.h).
- Макросы `EXTRA_RADIUS/VEC_SHIFT/VEC_LENGTH/MAX_ALREADY` сохранены в `old/collide.cpp` вне `#if 0` — нужны `slide_along()` в следующих чанках.
- `MAVHEIGHT` — в `fallen/Headers/mav.h`, не `pap.h`; `calc_height_on_face` — макрос-алиас в `world/navigation/walkable.h`.
- `slide_along_old` (lines 2392–3168) — уже в `/*...*/`, мёртвый код, не мигрировался.
- `bump_someone` — в `/*...*/`, помечен obsolete самим автором, не мигрировался.

---

## Итерация 106 — engine/physics/collide (чанк 1: геометрические хелперы) (2026-03-22)

- `fallen/Source/collide.cpp` (9183 строк) начинает миграцию. Чанк 1: lines 107–1510 → `new/engine/physics/collide.cpp` + `collide.h` + `collide_globals.cpp` + `collide_globals.h`.
- Мигрированы: `two4_line_intersection`, `clear_all_col_info`, `get_height_along_vect`, `get_height_along_facet`, `check_big_point_triangle_col`, `point_in_quad_old`, `dist_to_line`, `which_side`, `calc_along_vect`, `signed_dist_to_line_with_normal`, `signed_dist_to_line_with_normal_mark`, `nearest_point_on_line`, `nearest_point_on_line_and_dist`, `nearest_point_on_line_and_dist_calc_y`, `distance_to_line`, `nearest_point_on_line_and_dist_and_along`, `calc_height_at`, `collision_storey`, `highlight_face`, `highlight_rface`, `highlight_quad`, `vect_intersect_wall`. Макросы: `BLOCK_SIZE`, `SAME_SIGNS`, `TSHIFT`.
- Глобальные переменные: `dprod`, `cprod`, `global_on`, `next_col_vect`, `next_col_vect_link`, `walk_links[]`, `next_walk_link` → `collide_globals.cpp/h`.
- `which_side` — в оригинале `SLONG cprod;` локальная переменная (shadowing global). Сохранено с комментарием.
- Фикс сборки: `fallen/Headers/collide.h` использует `THING_INDEX` без включения `Game.h`; добавлен `#include "Game.h"` перед `collide.h` в `collide_globals.h` и `collide.h`.
- `col_vects[]` и `col_vects_links[]` — в оригинале определены в collide.cpp, в old/ отсутствуют (удалены на Stage 2); объявлены как extern в collide.h — сборка проходит, используются только в оставшихся чанках.

---

## Итерация 105 — engine/physics/hm (Hypermatter physics) (2026-03-22)

- `fallen/Source/hm.cpp` (3119 строк) → `new/engine/physics/hm.cpp` + `hm.h` + `hm_globals.cpp` + `hm_globals.h`.
- Все внутренние типы (HM_Point, HM_Edge, HM_Index, HM_Mesh, HM_Bump, HM_Col, HM_Object, HM_Primgrid) перенесены в `hm.h` — иначе `hm_globals.h` не может объявить `extern HM_Object HM_object[]`.
- Временные includes в hm.cpp: Game.h, Prim.h, maths.h, pap.h, Matrix.h, `missions/memory_globals.h` (prim_objects/prim_points/prim_faces4), `assets/anim_globals.h` (next_prim_object).
- Три forward decl в hm.cpp: `e_draw_3d_line`, `e_draw_3d_line_col_sorted`, `calc_height_at` — не мигрированы, используются внутри.
- Баг оригинала сохранён 1:1: в `HM_collide_all()` внутренний цикл проверяет `HM_object[i].used` вместо `[j]`.
- Фикс сборки: первоначально не было includes для prim_objects, prim_points, prim_faces4 — добавлены через `missions/memory_globals.h`.

---

## Итерация 104 — old/Person.cpp → redirect stub (2026-03-22)

- old/fallen/Source/Person.cpp заменён 3-строчной redirect-заглушкой (паттерн как у Cop.cpp, Anim.cpp и др.).
- Вся функциональность была уже в #if 0 блоках (итерации 92–103); активный код (lines 1–298) содержал только extern-декларации и forward-декларации, которые теперь покрываются через person.h/person_globals.h.

---

## Итерация 103 — actors/characters/person (чанк 12: person_mav_again..push_people_apart) (2026-03-22)

- Person.cpp чанк 12: lines 17272–19165 (финальный). 35 сущностей: person_mav_again, dir_to_angle[], get_dx_dz_for_dir, init_new_mav, fn_person_mavigate_action, fn_person_mavigate, set_person_grappling_hook_pickup, fn_person_grapple, set_person_mav_to_xz, set_person_mav_to_thing, person_is_on_sewer, person_is_on, set_person_can_pickup, set_person_can_release, set_person_special_pickup, fn_person_can, set_person_do_a_simple_anim, set_person_recircle, set_person_circle, set_person_hug_wall_leap_out, set_person_hug_wall_stand, check_near_facet, can_i_hug_wall, move_ok, fn_person_hug_wall, fn_person_circle, fn_person_circle_old, person_get_scale, how_long_is_anim, person_ok_for_conversation, set_person_float_up, set_person_float_down, fn_person_float, set_person_injured, push_people_apart.
- `dir_to_angle[]` (UWORD) → person_globals.cpp/h (глобальный массив).
- `extern SLONG global_on;` объявлен локально прямо перед `check_near_facet` — использовался в этой единственной функции, не вынесен в заголовок.
- `person_globals.cpp`: удалены extern-декларации для всех state-функций Person.cpp (теперь все из чанков 1-12 объявлены в person.h); оставлены только `fn_person_search` и `fn_person_carry`.
- `fn_person_circle`: inline forward decl `void push_into_attack_group_at_angle(...)` внутри тела — скопирована 1:1 из оригинала.
- `set_person_hug_wall_stand`: default arguments (dangle=0, locked=1) в person.h; в .cpp без defaults (стандартный C++).
- Extern-декларации `person_is_on_sewer` и `set_person_do_a_simple_anim` заменены на комментарии в person.cpp (теперь определены в том же файле).
- old/Person.cpp полностью завёрнут в `#if 0` блоки — файл фактически пуст.

---

## Итерация 102 — actors/characters/person (чанк 11: person_new_combat_node..mav_arrived) (2026-03-22)

- Person.cpp чанк 11: lines 15734–17267. 21 сущность: COMBO_ACCURACY, person_new_combat_node, aim_at_victim, fn_person_fighting, fn_person_wait, turn_to_face_thing, turn_to_face_thing_quick, get_pitch_to_thing_quick, set_person_draw_item, set_person_item_away, set_face_pos, set_face_thing, turn_towards_thing, fn_person_stand_up, fn_person_fight, set_person_goto_xz, fn_person_goto, process_person_goto_xz, fn_person_navigate, init_person_command, mav_arrived.
- Globals: `near_facet` (UWORD) и `kick_or_punch` (SLONG) перенесены в person_globals. Оба были в active zone old/Person.cpp — удалены явно (комментарии).
- Новые временные includes в person.cpp: `ai/pcom.h` (PCOM_set_person_move_punch/kick/arrest, PCOM_call_cop_to_arrest_me, PCOM_attack_happened), `ui/hud/overlay.h` (track_enemy).
- Extern `continue_blocking` (interfac.cpp) добавлен явно — не объявлен ни в одном мигрированном заголовке.
- Redeclaration `Thing* p_target` внутри `case SUB_STATE_PUNCH:` в fn_person_fighting — скопирована 1:1 из оригинала (баг оригинала, компилируется через расширение GCC/MSVC).
- `goto skip_animate` внутри fn_person_fighting — скопирован 1:1 (прыжок через инициализацию, оригинальный стиль).
- Сборка: fix — `aim_at_victim` forward decl с default argument в active zone old/Person.cpp конфликтовала с объявлением в person.h (redefinition of default argument). Удалена.

---

## Итерация 101 — actors/characters/person (чанк 10: set_person_ko_recoil..fn_person_gun) (2026-03-22)

- Person.cpp чанк 10: lines 13982–15728. 18 функций + 3 макроса: set_person_ko_recoil, set_person_recoil, fn_person_recoil, find_anim_fall_dir, move_away_from_wall, generate_bonus_item, part_bad, fn_person_dying, init_dead_tween, fn_person_dead, dist_from_a_to_b, player_aim_at_new_person, get_angle_to_target, player_running_aim_gun, twist_darci_body_to_angle, might_i_be_a_villain, am_i_a_thug, calc_dist_benefit_to_gun, highlight_gun_target, fn_person_gun. Макросы: FALL_DIR_STRANGE_LAND, FALL_DIR_LAND_ON_FRONT, FALL_DIR_LAND_ON_BACK.
- Globals: `dead_tween` (DrawTween) и `combo_display` (UBYTE) перенесены в person_globals. `combo_display` была активна в old/Person.cpp (line 15840, вне #if 0 блока) — удалена явно (комментарий).
- Новые externs: `get_pitch_to_thing_quick`, `turn_to_face_thing_quick` (Person.cpp, ещё не мигрированы); `look_pitch` (interfac.cpp).
- Extern declarations `might_i_be_a_villain` и `player_running_aim_gun` в person.cpp (chunk 4/9 forward decls) заменены на комментарии — теперь определены в этом же файле.

---

## Итерация 100 — actors/characters/person (чанк 9: fn_person_dangling..fn_person_moveing) (2026-03-22)

- Person.cpp чанк 9: lines 12330–13977. 11 функций + 1 макрос: fn_person_dangling, set_person_running_stop, should_person_automatically_land_on_fence, FENCE_DA, process_a_vaulting_person, set_person_sit_down, set_person_unsit, person_holding_2handed, person_holding_special, person_holding_bat, get_yomp_anim, fn_person_moveing.
- `person_holding_bat` — в оригинале inline-определение в active zone old/Person.cpp (line 240). Убрано из old/Person.cpp (заменено комментарием), определено в person.cpp (chunk 9).
- `get_yomp_anim` — ранее был `extern` в person.cpp (chunk 8 использовал как forward decl). Extern убран, теперь определена в файле.
- `person_holding_2handed` — ранее был `extern` в person.cpp. Убран, теперь определена.
- Новые externs: `mount_ladder` (collide.cpp), `player_running_aim_gun` (Person.cpp later), `continue_firing` (interfac.cpp), `set_person_do_a_simple_anim` (Person.cpp later), `trickle_velocity_to` (used in SUB_STATE_STOPPING_OT active code).
- `SUB_STATE_STOPPING` дублирующий `break; break;` (баг оригинала) — перенесён с комментарием.
- `SUB_STATE_SLIPPING` / `SUB_STATE_SLIPPING_END` — два `break;` в конце (один после SLIPPING_END fallthrough, один после SLIPPING) — как в оригинале.

---

## Итерация 99 — actors/characters/person (чанк 8: grab_ledge..do_person_on_cable) (2026-03-22)

- Person.cpp чанк 8: lines 10702–12325. 14 функций + 5 макросов: grab_ledge, set_tween_for_dy, set_tween_for_height, over_nogo, fn_person_jumping, position_person_at_ladder_top, position_person_at_ladder_bot, check_limb_pos_on_ladder, check_limb_pos_on_fence, check_limb_pos_on_fence_sideways, fn_person_laddering, fn_person_climbing, set_cable_angle, do_person_on_cable. Макросы PERSON_LIMB_ON_LADDER/TOP_BLOCK/BOT_BLOCK/OFF_TOP/OFF_BOT.
- `do_person_on_cable` ранее объявлена `extern` в person.cpp — замена на `get_yomp_anim` extern (который используется в fn_person_jumping), `do_person_on_cable` теперь определена в файле.
- `check_limb_pos_on_fence_sideways` — forward decl `calc_along_vect` внутри тела функции (как в оригинале), поскольку функция static-локальная в collide.cpp и не объявлена ни в одном заголовке.
- `fn_person_jumping`: дублирующий `break` после `SUB_STATE_FLYING_KICK_FALL` (unreachable) перенесён 1:1 (баг оригинала).

---

## Итерация 98 — actors/characters/person (чанк 7: set_person_pos_for_fence..find_best_cable_angle) (2026-03-22)

- Person.cpp чанк 7: lines 9241–10701. 25 функций: set_person_pos_for_fence, set_person_pos_for_half_step, is_facet_vaultable, is_facet_half_step, set_person_land_on_fence, set_person_kick_off_wall, fight_any_gang_attacker, find_arrestee, find_corpse, perform_arrest, fn_person_search, set_person_random_idle, fn_person_idle, set_person_in_vehicle, set_person_out_of_vehicle, locked_anim_change, locked_anim_change_of_type, locked_anim_change_height_type, set_limb_to_y, locked_next_anim_change, locked_anim_change_end_type, locked_anim_change_end, steep_cable, face_down_cable, find_best_cable_angle.
- `is_facet_vaultable` / `is_facet_half_step` — были `inline` в оригинале; сделаны non-inline, потому что ещё используются в немигрированных частях old/Person.cpp (требуется external linkage). Добавлены в person.h.
- Old/Person.cpp: удалены дублирующие forward decls `locked_anim_change` (с `dangle=0`), `fight_any_gang_attacker`, `set_person_pos_for_fence`, `set_person_pos_for_half_step` — они конфликтовали с person.h (redefinition of default argument / ambiguity). Теперь все берутся из person.h.
- `set_person_in_vehicle` — два switch-блока с присвоением `sample` (S_VAN_START и S_VAN_IDLE) сохранены 1:1, хотя переменная dead (все play-вызовы закомментированы в оригинале).
- PCOM_set_person_ai_flee_person / PCOM_make_driver_run_away — объявлены как forward decl в person.cpp (не в pcom.h, не в person.h), т.к. нужны только для set_person_in_vehicle.

---

## Итерация 97 — actors/characters/person (чанк 6: set_person_uncarry..set_person_pos_for_fence_vault) (2026-03-22)

- Person.cpp чанк 6: lines 7651–9237. 32 функции: set_person_uncarry, set_person_stand_carry, fn_person_carry, set_person_arrest, set_person_croutch, set_person_crawling, set_person_leg_sweep, set_person_punch, set_person_kick_dir, set_person_fight_anim, set_person_kick, set_person_kick_near, set_person_stomp, set_person_position_for_ladder, play_jump_sound, set_person_climb_ladder, set_person_on_ladder, set_person_on_fence, set_person_standing_jump, set_person_standing_jump_forwards, set_person_standing_jump_backwards, set_person_running_jump, set_person_running_jump_lr, traverse_pos, set_person_traverse, set_person_pulling_up, set_person_drop_down, set_person_locked_drop_down, is_wall_good_for_bump_and_turn, am_i_facing_wall, along_middle_of_facet, set_person_pos_for_fence_vault.
- Добавлены Temporary includes: actors/core/interact.h (find_grab_face), fallen/Headers/prim.h (which_side, two4_line_intersection, signed_dist_to_line_with_normal, does_fence_lie_along_line). Добавлен extern: person_is_on_sewer (Person.cpp, ещё не мигрирован).
- `set_person_punch` — unreachable `node = 19` после `break` для BASEBALLBAT — copy-paste баг оригинала, перенесён 1:1 с комментарием.
- `play_jump_sound` — inline с static local `jump_chan`. В person.h не объявляется (файл-локальная).
- `VAULT_DA` macro в `set_person_pos_for_fence_vault` — inline #define без финального #undef (как в оригинале), утекает в следующий чанк — будет решено при миграции chunk 7.
- Forward decl `set_person_running_jump_lr` в person.cpp перед `set_person_running_jump` (вызывает её). Определение в том же файле ниже.

---

## Итерация 96 — actors/characters/person (чанк 5: set_person_idle_uncroutch..set_person_carry) (2026-03-22)

- Person.cpp чанк 5: lines 6144–7647. Функции: set_person_idle_uncroutch, set_person_turn_to_hug_wall, set_person_hug_wall_dir, set_person_hug_wall_look, set_person_idle, set_person_locked_idle_ready, set_person_sidle, set_person_flip, set_person_running, set_person_running_frame, set_person_draw_special, set_person_draw_gun, set_person_gun_away, set_person_step_left, set_person_step_right, set_vehicle_anim, position_person_for_vehicle, set_person_enter_vehicle, add_person_to_passenger_list, remove_person_from_passenger_list, set_person_passenger_in_vehicle, set_person_exit_vehicle, set_anim_walking, set_anim_running, set_anim_idle, set_person_walking, set_person_walk_backwards, set_person_sneaking, set_person_hop_back, find_idle_fight_stance, set_person_fight_idle, set_person_fight_step, set_person_fight_step_forward, set_person_block, set_person_idle_croutch, emergency_uncarry, carry_running, set_person_carry.
- Добавлены Temporary includes: actors/vehicles/vehicle.h (VEH_find_door), actors/vehicles/vehicle_globals.h (sneaky_do_it_for_positioning_a_person_to_do_the_enter_anim). Добавлен extern: continue_moveing (interfac.cpp).
- `set_person_sidle` — тело полностью закомментировано в оригинале (только `return 0`), перенесена с пустым телом.
- `set_person_fight_idle` была объявлена в chunk 4 секции person.h — дубль из chunk 5 убран.
- Forward decl `set_person_stand_carry` в person.cpp (chunk 5 вызывает её раньше определения).

---

## Итерация 95 — actors/characters/person (чанк 4: set_person_aim..drop_all_items) (2026-03-22)

- Person.cpp чанк 4: lines 4742–6140. Функции: set_person_aim, weapon_accuracy_at_dist, VehicleBelongsToMIB, get_shoot_damage, shoot_get_ammo_sound_anim_time, actually_fire_gun, set_person_running_shoot, get_persons_best_weapon_with_ammo, dont_hurt_target_during_cutscene, set_person_shoot, set_person_grapple_windup, set_person_grappling_hook_release, person_has_gun_out, drop_current_gun, drop_all_items.
- Добавлены 6 новых Temporary include в person.cpp: actors/items/guns.h, actors/items/special.h, ai/combat.h, effects/pyro.h, actors/animals/bat.h, actors/items/barrel.h.
- `weapon_accuracy_at_dist` в оригинале `inline` — перенесена как `static inline` (файл-локальная).

---

## Итерация 94 — actors/characters/person (чанк 3: general_process_player..camera_normal) (2026-03-22)

- Person.cpp чанк 3: lines 3202–4739. Функции: general_process_player, person_pick_best_target, general_process_person, check_on_slippy_slope, slope_ahead, person_normal_move_dxdz, person_normal_move, person_normal_move_check, advance_keyframe, retreat_keyframe, move_locked_tween, person_normal_animate_speed, person_normal_animate, person_backwards_animate, camera_shoot, camera_fight, camera_normal.
- `get_person_radius` не объявлена ни в одном заголовке — добавлен `extern` в person.cpp (определение в collide.cpp).
- `calc_sub_objects_position_fix8` — inline extern в старом Person.cpp (line 4425, активная зона) — добавлен дублирующий extern в new/person.cpp.
- `dx/dy/dz` в `person_normal_animate_speed` при Locked: оригинал использует lock_x1 для всех трёх осей (copy-paste баг), перенесён 1:1.
- camera_shoot/camera_fight/camera_normal: все тела в оригинале закомментированы — перенесены как пустые функции (dead code).
- Дублирующие декларации general_process_player/person_pick_best_target/general_process_person в person.h обнаружены и исправлены (были добавлены и в секцию chunk 2, и в секцию chunk 3).

---

## Итерация 93 — actors/characters/person (чанк 2: sweep_feet..do_look_for_enemies) (2026-03-22)

- Person.cpp чанк 2: lines 1769–3193. Функции: sweep_feet, is_there_room_behind_person, get_along_facet, set_person_dead, is_person_guilty, person_on_floor, really_on_floor, is_person_dead, is_person_ko, is_person_ko_and_lay_down, knock_person_down, person_bodge_forward, los_between_heads, oscilate_tinpanum, dist_to_target, dist_to_target_pelvis, is_person_crouching, can_a_see_b, can_i_see_place, set_person_sliding_tackle, set_person_vault, set_person_climb_half, can_i_see_player, do_look_for_enemies.
- `can_a_see_b` уже объявлена в old Person.h с default args — в new person.h добавлена ссылка-комментарий без декларации (избежание `redefinition of default argument`).
- `set_person_mav_to_thing` объявлена только как forward decl в old Person.cpp (line 284, активная зона) — добавлен extern в person.cpp.

---

## Итерация 92 — actors/characters/person (чанк 1: dispatch tables..death_slide) (2026-03-22)

- Person.cpp — самый большой файл (~19K строк), мигрируется чанками. Чанк 1: ~lines 300–1762.
- `people_functions[]`, `generic_people_functions[]`, `PERSON_mode_name[]`, `health[]` → person_globals.cpp (правило: все глобалы в _globals).
- Circular dependency при попытке сделать redirect из old/Person.h: `person.h → thing.h → Game.h → Person.h(old) → person.h`. Решение: new/person.h включает `fallen/Headers/Game.h` (Temporary) вместо thing.h и не переопределяет Person struct — всё берётся из old/Person.h через Game.h.
- old/Person.h не переключён в redirect — останется с оригинальным содержимым до полной миграции Person.cpp.
- `get_fence_bottom`, `get_fence_top`, `MagicFrameCheck` — убран `static`/`static inline`, т.к. используются из немигрированных частей old/Person.cpp.
- `generic_people_functions[]` ссылается на `fn_person_*` из old/Person.cpp — forward declarations в person_globals.cpp.
- `player_dlight` (line 225 old/Person.cpp) и `timer_bored` (line 3195) были вне MIGRATED block — добавлены `#if 0 // MIGRATED` обёртки.
- `ShowAnimNumber` — пустая inline-заглушка (в оригинале тоже пустая debug-функция).

---

## Итерация 91 — actors/vehicles/vehicle (чанки 2+3: collision..wheel query API) (2026-03-22)

- Завершена миграция Vehicle.cpp: чанки 2 (~1540 строк) + 3 (~900 строк) в одной итерации — принудительно, т.к. `siren`, `do_car_input`, `process_car` — static, нужны VEH_driving/CollideCar.
- `VEH_reduce_health` signature исправлена: chunk 1 ошибочно поместил 5-параметровую версию — фактически 3 параметра: `(Thing* p_car, Thing* p_person, SLONG damage)`.
- Forward declarations в новом vehicle.cpp: `siren`, `GetCarPoints`, `init_arctans`, `process_car`, `CollideCar`, `CollideWithKerb`, `do_car_input`.
- `VEH_col[VEH_MAX_COL]`, `VEH_col_upto`, `car_hit_flags`, `vehicle_wheel_pos_vehicle`, `vehicle_wheel_pos_info` → vehicle_globals.cpp/h.
- `VEH_FWD_ACCEL/DECEL/REV_ACCEL/DECEL` — определены внутри `do_car_input` как в оригинале (`#define`/`#undef` внутри функции).
- old/Vehicle.cpp: всё тело файла теперь в `#if 0 // MIGRATED` блоках. Активны только includes, extern declarations (строки 1–147).

---

## Итерация 90 — actors/vehicles/vehicle (чанк 1: VehInfo..draw_car) (2026-03-22)

- Чанк 1 (~1140 строк) из 3 чанков Vehicle.cpp: VehInfo table, statefunctions, helpers матриц, alloc/dealloc, draw_car, VEH_on_road, VEH_add_damage/bounce, init/create.
- `WHEELTIME` и `WHEELRATIO` перемещены в `vehicle.h` (нужны для размера массива `arctan_table[]` в vehicle_globals.h).
- `car_matrix`, `arctan_table`, `arctan_table_ok` определены в `vehicle_globals.cpp` (не в vehicle.cpp) — выполнение правила "все глобалы в _globals".
- `make_car_matrix`, `make_car_matrix_p`, `apply_car_matrix`, `is_driven_by_player`, `there_is_a_los_car`, `should_i_collide_against_this_anim_prim` — non-static (используются из old/ chunks 2+3 через extern).
- `GetCarPoints` — оставлен активным и в old/Vehicle.cpp (static inline, нужен CollideCar и др. в chunks 2+3); в new/vehicle.cpp отдельная `static inline` копия для `VEH_on_road`. Будет удалён из old/ когда chunks 2+3 мигрируют.
- `init_arctans` — определена в new/vehicle.cpp (вызывается из `VEH_init_vehinfo`); тоже нужна в old/ (в `#if 0` блок не помещена — используется steering_wheel в chunks 2+3 через `arctan_table_ok` guard).
- Redirect-заголовок old/fallen/Headers/Vehicle.h: `#pragma once` + include vehicle.h + vehicle_globals.h.
- Добавлены `// Temporary:` includes: `fallen/DDEngine/Headers/font2d.h`, `fallen/Headers/pcom.h`, `fallen/Headers/psystem.h`.

---

## Итерация 89 — ai/pcom (девятый чанк: PCOM_process_person..PCOM_make_driver_run_away) (2026-03-22)

- `on_same_side` — не static в оригинале, но нигде не объявлена в заголовках; помечена `static` в new/ (только внутреннее использование).
- old/pcom.cpp полностью перенесён: всё тело файла теперь в `#if 0 // MIGRATED` блоках. Активны только заголовки, globals и forward declarations в начале файла.

---

## Итерация 88 — ai/pcom (восьмой чанк: PCOM_process_state_change..PCOM_process_movement) (2026-03-22)

- `PCOM_runover_scary_person`, `Noise`, `MAX_NOISE`, `noise_count`, `noises[]`, `PCOM_debug_string` — глобалы → `pcom_globals.cpp/h`; удалены из old/pcom.cpp.
- `PCOM_RUNOVER_*` макросы и `BLOCKED(C)` оставлены активными в old/pcom.cpp (нужны `DriveCar` который ещё не мигрирован); продублированы в new/pcom.cpp.
- `set_person_fight_idle`, `people_allowed_to_hit_each_other` — нет заголовков → добавлены как file-scope `extern` в new/pcom.cpp.
- `fallen/Headers/road.h` — добавлен как `// Temporary:` include (нужен `ROAD_nearest_node`, `ROAD_node_degree`, `ROAD_is_zebra`, `JN_RADIUS_IN`, `NEAR_JUNCTION`, `AT_JUNCTION`).
- Осталось в old/pcom.cpp: ~1590 строк (строки 11092–12666).

---

## Итерация 87 — ai/pcom (седьмой чанк: PCOM_process_findcar..PCOM_find_bodyguard_victim) (2026-03-22)

- `in_right_place_for_car` (из Vehicle.cpp) — добавлен как file-scope `extern` в `new/ai/pcom.cpp` (используется в `PCOM_process_findcar` и `PCOM_process_hitch`).
- `person_drawn_recently` — static helper, не объявлена в заголовке.
- `PCOM_MAX_BENCH_WALK` — file-local `#define` (был inline в оригинале), добавлен перед `PCOM_process_normal`.
- Добавлены `// Temporary:` includes: `actors/core/thing.h` (для `THING_find_nearest`) и `actors/items/special.h` (для `SPECIAL_BOMB`, `SPECIAL_SUBSTATE_*`).
- Осталось в old/pcom.cpp: ~3240 строк (строки 9428–12666).

---

## Итерация 86 — ai/pcom (шестой чанк: PCOM_process_killing..PCOM_process_navtokill) (2026-03-22)

- `timer_bored` и `IsEnglish` — добавлены как file-scope `extern` в `new/ai/pcom.cpp` (ранее объявлялись как inline-extern внутри тел функций в old).
- Добавлены `// Temporary:` includes: `fallen/DDEngine/Headers/aeng.h` (для `AENG_world_line` в `draw_view_line`) и `actors/core/interact.h` (для `calc_sub_objects_position` в `draw_view_line`).
- `PCOM_find_mib_appear_pos` — пустая функция в оригинале (тело не реализовано); перенесена как есть (empty stub).
- `quick_kick = 0` в `PCOM_process_killing` — объявлена но не используется в оригинале; сохранена 1:1.

---

## Итерация 85 — ai/pcom (пятый чанк: PCOM_create_player..PCOM_process_wander) (2026-03-22)

- `arrest_me[100]` + `next_arrest` — глобалы → `pcom_globals.cpp/h`; `MAX_ARREST_ME` макрос туда же.
- `PCOM_can_i_see_person_to_arrest` — полностью в `/* */` в оригинале → dead code, не перенесена; в `pcom.h` декларация сохранена проактивно (была добавлена ранее итерацией 84).
- `extern ULONG timer_bored` — был в блоке `#if 0 MIGRATED (iter 82)` перед `PCOM_do_regen`; после `#if 0` блокировки пропал из active зоны → добавлен заново после `#endif MIGRATED iter 85` для остатка pcom.cpp.
- `fallen/Headers/Vehicle.h` — добавлен как `// Temporary:` include для `reinit_vehicle` и `Vehicle` struct (нужен `PCOM_process_driving_wander`).
- 8 новых функций добавлены в `pcom.h`: `init_arrest`, `do_arrests`, `should_person_regen`, `PCOM_process_driving_still/patrol/wander`, `PCOM_process_patrol`, `PCOM_process_wander`.

---

## Итерация 84 — ai/pcom (третий чанк: AI setters + PCOM_create_person) (2026-03-22)

- `PCOM_summon[4]` — `static` в оригинале нет, глобал → `pcom_globals.cpp` + `pcom_globals.h`; макрос `PCOM_SUMMON_NUM_BODIES` туда же.
- Добавлены includes в `new/ai/pcom.cpp`: `engine/audio/mfx.h`, `engine/audio/sound.h` (SOUND_Range), `assets/sound_id.h` (S_COP_ARREST_START/END), `fallen/Headers/spark.h`, `fallen/Headers/balloon.h`, `fallen/Headers/cnet.h`, `fallen/Headers/overlay.h // Temporary:` (track_enemy — нарушение DAG, но Temporary).
- 6 функций не были объявлены в pcom.h (`PCOM_set_person_ai_knocked_out`, `PCOM_set_person_ai_arrest`, `PCOM_set_person_ai_leavecar`, `PCOM_set_person_ai_investigate`, `PCOM_process_getitem`, `PCOM_process_summon`) — добавлены в `new/ai/pcom.h`.
- `PersonIsMIB` — в оригинале закомментированное тело определения + два extern forward decl; воспроизведено как extern + заблокированный комментарий (мёртвый код).
- `UWORD anim` в `PCOM_set_person_ai_hands_up` — объявлена но не используется (как в оригинале); оставлено для 1:1 (без предупреждений компилятора).

---

## Итерация 83 — ai/pcom (второй чанк: movement state setters + gang attack system) (2026-03-21)

- Перенесены: все `PCOM_set_person_move_*` / `PCOM_set_person_substate_*` (22 функции), `PCOM_renav`, `PCOM_finished_nav`, `PCOM_set_person_move_pause`, `PCOM_set_person_move_animation`, `PCOM_set_person_move_shoot`, `PCOM_set_person_move_circle`.
- Gang attack ring: `check_players_gang`, `count_gang`, `get_any_gang_member`, `get_nearest_gang_member`, `find_target_from_gang`, `remove_from_gang_attack`, `scare_gang_attack`, `reset_gang_attack`, `process_gang_attack`, `push_into_attack_group_at_angle`, `PCOM_new_gang_attack`.
- `check_players_gang` и др. — в оригинале без `static` (внешние файлы: Person.cpp, interfac.cpp, combat.cpp их вызывают). Написаны non-static сразу.
- `gang_angle_priority` — в оригинале `static` в pcom.cpp; по правилам перемещена в `pcom_globals.cpp` + `pcom_globals.h` (extern `UBYTE gang_angle_priority[8]`).
- `dist_to_target` был внутри нового `#if 0 // MIGRATED` блока — добавлен forward decl в верхнюю секцию old/pcom.cpp.
- `PCOM_set_person_move_runaway` не объявлена в pcom.h (используется только внутри pcom.cpp) — добавлен forward decl в old/pcom.cpp для оставшихся вызывающих.
- Добавлен include `ai/combat_globals.h` в new/ai/pcom.cpp (нужен `gang_attacks[]`); добавлен include `fallen/Headers/Person.h // Temporary:`.
- `#if 0 // MIGRATED` охватывает строки 1751–3394 old/pcom.cpp.

---

## Итерация 82 — ai/pcom (первый чанк: globals + утилиты + запросы, PCOM_init..PCOM_alert_my_gang_to_flee) (2026-03-21)

- `old/fallen/Headers/pcom.h` заменён redirect-заглушкой; новый `new/ai/pcom.h` содержит полный публичный API.
- Заголовок `new/ai/pcom.h` содержит объявления ещё не перенесённых функций (из остатка pcom.cpp) — чтобы компиляция всего кода, ссылающегося на pcom.h, не сломалась.
- Макросы `PCOM_MOVE_STATE_*`, `PCOM_MOVE_SUBSTATE_*`, `PCOM_MOVE_SPEED_*`, `PCOM_MOVE_FLAG_*`, `PCOM_EXCAR_*`, `PCOM_ARRIVE_DIST` — file-local в оригинале (определены в pcom.cpp), воспроизведены в `new/ai/pcom.cpp`; оставлены в active-зоне `old/pcom.cpp` (нужны не перенесённым функциям).
- `PCOM_TICKS_PER_TURN`, `PCOM_TICKS_PER_SEC` перемещены в pcom.h перед inline-функциями (иначе PCOM_get_duration не компилируется).
- `PersonIsMIB` — конфликт типов возврата с combat.cpp (был `SLONG`, должен `BOOL`); исправлено в combat.cpp.
- `PCOM_move_state_name` — потребовалось три отдельных `#if 0 // MIGRATED` блока в old/pcom.cpp: строковые массивы до макросов, PCOM_move_state_name между макросами, глобалы+функции после макросов; иначе макросы оказывались внутри #if 0.

---

## Итерация 81 — ai/combat (fallen/Source/Combat.cpp, 2957 строк) (2026-03-21)

- `find_attack_stance` в оригинальном `combat.h` объявлена с 6 параметрами, но реализация (и все вызывающие) использует 7 (плюс `attack_range`). Новый заголовок использует правильную 7-параметровую сигнатуру.
- `found[16]` — file-scope глобал без `static`, использовался как буфер во всех функциях одного TU → по правилам переехал в `_globals`.
- `is_anyone_nearby_alive` — объявлена в старом `combat.h`, но нигде не определена. Оставлена как мёртвое объявление в `new/ai/combat.h` для совместимости.
- `func(void)` — пустая функция-заглушка (только закомментированный printf). Удалена как мёртвый код.
- Зависимости вверх по DAG: `ui/hud/overlay.h` (track_enemy) и `missions/eway.h` (EWAY_get_person, check_eway_talk) — tech debt оригинала, воспроизведён 1:1.
- 3 inline-`#define` внутри функций (COMBAT_HIT_DIST_LEEWAY, STANCE_DIST_LEEWAY, STANCE_DANGLE_IMPORTANCE) — найдены при review без uc_orig; исправлено.

---

## Итерация 80 — missions/eway (четвёртый чанк, финальный: EWAY_set_inactive..EWAY_deduct_time_penalty) (2026-03-21)

- `old/fallen/Source/eway.cpp` заменён redirect-заглушкой. `new/missions/eway.cpp` теперь полный.
- `analogue` — inline-`extern` внутри `EWAY_grab_camera`, как в оригинале; объявлен в `Game.h` → отдельный forward decl не нужен.
- `timer` в `EWAY_deduct_time_penalty` — объявлен но не использован в оригинале (`SLONG timer = ew->ec.arg2`); удалён чтобы избежать предупреждения, логика не затронута.
- `SLONG id` в `EWAY_work_out_which_ones_are_in_warehouses` — аналогично, удалён (unused).
- `people_types` extern — уже в entity mapping (supermap_globals); дублирующую запись не добавляли.

---

## Итерация 79 — missions/eway (третий чанк: EWAY_finish_conversation + EWAY_process_conversation + EWAY_process_emit_steam + EWAY_set_active) (2026-03-21)

- `old/fallen/Source/eway.cpp`: перенесены 4 функции (~1430 строк) через `#if 0 // MIGRATED` блок. Остаток (`EWAY_set_inactive`..`EWAY_deduct_time_penalty`) — следующая итерация.
- Новые Temporary includes в `new/missions/eway.cpp`: `effects/pyro.h`, `fallen/Headers/barrel.h`, `fallen/Headers/plat.h`.
- `find_nice_place_near_person`, `push_people_apart`, `set_stats`, `VEH_reduce_health`, `hit_player`, `PCOM_set_person_ai_normal`, `BARREL_dissapear` — нет в заголовках, воспроизведены как inline-`extern` в теле функции (паттерн оригинала).
- `PANEL_wide_top_person` / `PANEL_wide_bot_person` — нет в заголовках, объявлены inline-`extern` внутри `EWAY_finish_conversation`.
- `EWAY_set_inactive` в `EWAY_DO_GROUP_RESET`: forward decl внутри switch-case (нужен, т.к. `EWAY_set_inactive` определён позже в том же TU — теперь в `new/` он окажется после `EWAY_set_active` в другой итерации; пока объявлен как forward decl внутри тела функции).

---

## Итерация 78 — missions/eway (второй чанк: EWAY_evaluate_condition + EWAY_create_camera + EWAY_process_camera) (2026-03-21)

- `old/fallen/Source/eway.cpp`: перенесены `EWAY_evaluate_condition` (1121 строк), `EWAY_create_camera` (59 строк), `EWAY_process_camera` (479 строк). Остаток (`EWAY_finish_conversation`..`EWAY_deduct_time_penalty`) — в `old/`, итерация следующая.
- `EWAY_evaluate_condition` default arg (`= FALSE`) теперь в `eway.h` (без дефолта в определении). `interfac.cpp` имел дублирующий `extern` с дефолтом → убран дефолт из extern в `interfac.cpp` (дефолт из `eway.h` применяется).
- `fallen/Headers/collide.h` добавлен в includes eway.cpp (`// Temporary:`): нужен `there_is_a_los` для EWAY_COND_A_SEE_B и финальной проверки CONVERSATION.
- `timer_bored` и `person_ok_for_conversation` были extern-объявлены прямо в старом eway.cpp (перед функциями) — при удалении блока пропали → восстановлены как extern в оставшейся части `old/eway.cpp` (всё ещё нужны EWAY_finish_conversation и пр.).
- `PersonIsMIB`, `is_person_dead`, `is_semtex`, `EWAY_set_active` — inline-`extern` forward declarations внутри тела функции, воспроизведены как в оригинале.

---

## Итерация 77 — missions/eway (первый чанк: globals + get_level_word..EWAY_created_last_waypoint) (2026-03-21)

- `old/fallen/Source/eway.cpp` (6503 строки): перенесён первый чанк — все глобальные переменные и функции от `get_level_word` до `EWAY_created_last_waypoint` (~1930 строк). Остаток (`EWAY_evaluate_condition`..`EWAY_deduct_time_penalty`) — в `old/`, итерация следующая.
- Архитектурное решение: типовые определения (`EWAY_Way`, `EWAY_Cond` и т.д.) перенесены в `eway_globals.h`, а не в `eway.h` — потому что глобальные переменные в `eway_globals.h` ссылаются на эти типы. Иначе circular include.
- `EWAY_evaluate_condition` — в оригинале дефолтный аргумент (`= FALSE`) объявлен прямо в `definition` в cpp-файле, не в хедере. Forward declaration в `eway.h` без дефолта — иначе C++ дала бы ошибку "default argument already specified".
- `ob_index` в `EWAY_create_item`: конфликт глобального `ob_index` и локальной переменной с тем же именем → локальная переименована `ob_index_local` (conflict rename).
- `fallen/Headers/drive.h` уже мигрирован → `engine/io/drive.h` (без Temporary).
- `fallen/Headers/aeng.h` находится в `fallen/DDEngine/Headers/aeng.h` — исправлено при компиляции.
- `memory.h` уже redirect-заглушка → include исправлен на `missions/memory.h` напрямую.
- `person_ok_for_conversation` — не объявлена ни в одном заголовке; добавлен inline-`extern` forward declaration.

---

## Итерация 76 — missions/elev (второй чанк: ELEV_game_init_common + ELEV_game_init + остальные) (2026-03-21)

- `old/fallen/Source/elev.cpp` заменён redirect-заглушкой. `new/missions/elev.cpp` теперь полный.
- `SND_BeginAmbient` — объявлена в `engine/audio/sound.h` (уже включён через `fallen/Headers/sound.h`); `extern void SND_BeginAmbient()` внутри `ELEV_game_init_common` оставлен как в оригинале.
- `char junk[1000]` в `ELEV_load_name` — локальная переменная, shadowing глобального `CBYTE junk[2048]` из `elev_globals`; паттерн оригинала, сохранён.
- `MAV_init`, `MAV_precalculate` — через `fallen/Headers/mav.h` (`// Temporary:`).
- `COLLIDE_calc_fastnav_bits`, `COLLIDE_find_seethrough_fences`, `insert_collision_facets` — через `fallen/Headers/collide.h` (`// Temporary:`).
- `STARTSCR_mission` — уже мигрирован в `assets/startscr_globals.h`; включён напрямую (без Temporary).
- `playback_file` — уже в `actors/core/thing_globals.h`; включён напрямую.

---

## Итерация 75 — missions/elev (первый чанк: globals + ELEV_load_level) (2026-03-21)

- `old/fallen/Source/elev.cpp` (2976 строк): перенесён первый чанк — globals + `ELEV_load_level` (~1884 строки). Остальные функции (`ELEV_game_init_common`, `ELEV_game_init`, `ELEV_create_similar_name`, `ELEV_load_name`, `ELEV_load_user`, `reload_level`) остаются в `old/` до следующей итерации.
- `panel.h` находится в `fallen/DDEngine/Headers/` (не в `fallen/Headers/`) — исправлено при компиляции.
- `Vehicle.h` ещё не мигрирован в `new/actors/vehicles/` — использован `fallen/Headers/Vehicle.h` с пометкой `// Temporary:`.
- `people_types[50]` — определён в `Person.cpp`, нет в заголовке; добавлен inline-`extern` forward declaration.
- `vehicle_random[]` — аналогично, только inline-`extern` в `old/elev.cpp`; воспроизведён паттерн.
- `elev.h` изначально содержал `uc_orig` для ещё не мигрированных функций — исправлено: `uc_orig` только на `ELEV_load_level`, остальные объявления без него до момента миграции.

---

## Итерация 74 — actors/animals/bat (второй чанк: BAT_normal + BAT_create + BAT_apply_hit) (2026-03-21)

- `old/fallen/Source/bat.cpp` заменён redirect-заглушкой. `new/actors/animals/bat.cpp` теперь полный.
- `there_is_a_los_mav` — не объявлена ни в одном заголовке (только inline-`extern` в оригинальном `bat.cpp`); воспроизведён паттерн, добавлен `uc_orig`.
- `fallen/Headers/pyro.h` — уже redirect-заглушка (`effects/pyro.h`); include исправлен на прямой.
- `fallen/Headers/collide.h` — `// Temporary:` (содержит `there_is_a_los`, `create_shockwave`).
- `assets/level_loader.h` — `load_anim_prim_object` (нужен для `BAT_create`).
- CMakeLists: `src/old/fallen/Source/bat.cpp` заменён на `src/new/actors/animals/bat.cpp` + `src/new/actors/animals/bat_globals.cpp`.

---

## Итерация 73 — actors/animals/bat (первый чанк: globals + BAT_find_summon_people..BAT_balrog_slide_along) (2026-03-21)

- `old/fallen/Source/bat.cpp` (2908 строк) разбит: первый чанк (1342 строки) → `new/actors/animals/bat.cpp`.
- `BAT_state_name[]` — в оригинале file-scope (не `static`), перенесён в `bat_globals.cpp`.
- `BAT_summon[]` — аналогично, в `bat_globals.cpp`.
- `BAT_ANIM_*` и прочие file-private макросы (`BAT_FLAG_*`, `BAT_STATE_*`, `BAT_SUBSTATE_*`, `BAT_COLLIDE_*`, `BAT_BALROG_WIDTH`, `BAT_TICKS_PER_SECOND`) остаются в `bat.cpp` (не в `.h`).
- `advance_keyframe(DrawTween*)` — не в `darci.h` (там другая сигнатура); используется inline-`extern` forward declaration (паттерн из `animal.cpp`).
- `is_person_dead`, `is_person_ko` — inline-`extern` forward declarations (паттерн из `guns.cpp`/`psystem.cpp`).
- `set_person_float_up` — объявлена в `Person.h`, доступна через `Game.h`.
- `pcom.h` — временный include (нужен `PCOM_AI_NONE`, `PCOM_AI_SUICIDE`), помечен `// Temporary:`.

---

## Итерация 72 — missions/memory (второй чанк: convert_to_pointer + load + MEMORY_quick + save_dreamcast_wad) (2026-03-21)

- `old/fallen/Source/memory.cpp` заменён redirect-заглушкой. `new/missions/memory.cpp` теперь полный.
- `convert_drawtype_to_pointer` / `convert_thing_to_pointer` — в оригинале `static`, но нужны снаружи (`convert_index_to_pointers`). Убран `static`, добавлены в `memory.h`.
- `EWAY_time_accurate/time/tick`, `EWAY_cam_*` (15 переменных) — не объявлены в `eway.h`, были только inline-`extern` в `old/memory.cpp`. Добавлены явные `extern` в `new/missions/memory.cpp`.
- `TEXTURE_set` — объявлен в `new/assets/texture_globals.h`; добавлен `extern SLONG TEXTURE_set` в `memory.cpp`.
- `STORE_DATA_DC` — переименован из `STORE_DATA` (в `save_dreamcast_wad`) чтобы избежать конфликта с `STORE_DATA` из `save_whole_anims` (оба в одном TU). Помечен `--conflict` в entity mapping.
- Include-порядок в `memory.h`: `Game.h` перемещён до `memory_globals.h` (supermap.h внутри memory_globals.h нужен `MFFileHandle`).

---

## Итерация 71 — missions/memory (первый чанк: globals + save_table + convert + save_whole) (2026-03-21)

- Исходный файл: `old/fallen/Source/memory.cpp` (2409 строк). Первый чанк ~670 строк (без `save_table`).
- Созданы: `new/missions/memory.h`, `new/missions/memory.cpp`, `new/missions/memory_globals.h`, `new/missions/memory_globals.cpp`.
- `old/fallen/Headers/memory.h` заменён redirect-заглушкой.
- `EWAY_counter` определён в `eway.cpp` (не в `eway.h`) — добавлен `extern UBYTE* EWAY_counter` в `memory_globals.cpp`.
- **CHECK B fix:** `save_table[]` сначала ошибочно оказался в `memory.cpp`; перемещён в `memory_globals.cpp`. Все сопутствующие includes (ob.h, eway.h, pap.h, ...) перенесены туда же.
- `prim_info` — определён в `Prim.cpp` (не мигрирован); в `memory_globals.h` — только `extern`, определение остаётся в `Prim.cpp` до его миграции.
- `convert_keyframe_to_pointer` и аналоги: в оригинале `static`, но вызываются из `old/memory.cpp` (второй чанк). Убран `static`, добавлены объявления в `memory.h`.
- Второй чанк (`convert_index_to_pointers`, `uncache`, `load_*`, `MEMORY_quick_*`, `save_dreamcast_wad`) остаётся в `old/memory.cpp` до следующей итерации.

---

## Итерация 70 — assets/anim_loader (io.cpp второй чанк: VUE loader + .all/.moj loaders) (2026-03-21)

- `prim_points`, `prim_faces3/4`, `prim_objects`, `prim_multi_objects` объявлены в `memory.h` (не в `prim.h`) — добавлен `// Temporary:` include на `memory.h`.
- `DONT_load` из `world/map/supermap_globals.h` — DAG-нарушение (assets → world); помечено `// Temporary:`.
- `old/fallen/Source/io.cpp` полностью заменён redirect-заглушкой.

---

## Итерация 69 — engine/lighting/night (второй чанк: dfcache_recalc..NIGHT_load_ed_file) (2026-03-21)

- Мигрированы: `NIGHT_dfcache_recalc`, `NIGHT_dfcache_create`, `NIGHT_dfcache_destroy`, `NIGHT_get_light_at`, `NIGHT_find`, `NIGHT_init`, `calc_lighting__for_point`, `NIGHT_generate_roof_walkable`, `NIGHT_generate_walkable_lighting`, `NIGHT_destroy_all_cached_info`, `NIGHT_load_ed_file`.
- `hidden_roof_index[128][128]` — в оригинале определён внутри `night.cpp` без `static`, доступен из `aeng.cpp` через `extern`. Перенесён в `night_globals.cpp`. В `aeng.cpp` убрано inline-`extern` объявление — теперь доступен через `fallen/Headers/night.h` → `night_globals.h`.
- `old/fallen/Source/night.cpp` полностью заменён redirect-заглушкой.

---

## Итерация 68 — engine/lighting/night (первый чанк: globals + ambient + slight + dlight + cache + dfcache_init) (2026-03-21)

- Исходный файл: `old/fallen/Source/night.cpp` (3905 строк). Первый чанк — ~2300 строк.
- Созданы новые файлы: `new/engine/lighting/night.h`, `new/engine/lighting/night.cpp`, `new/engine/lighting/night_globals.h`, `new/engine/lighting/night_globals.cpp`.
- `old/fallen/Headers/Night.h` заменён redirect-заглушкой (2 строки).
- Мигрированы: все types/macros из `Night.h`, все globals, функции `NIGHT_ambient`, `NIGHT_ambient_at_point`, `NIGHT_slight_*`, `NIGHT_light_mapsquare`, `NIGHT_get_facet_info`, `NIGHT_light_prim`, `NIGHT_dlight_*`, `NIGHT_cache_*`, `NIGHT_dfcache_init`.
- **Ошибка компиляции 1:** `NIGHT_FLAG_INSIDE` — file-scope `#define` в оригинальном `night.cpp`, используется как в новом `night.cpp`, так и в оставшемся `old/night.cpp` (в `NIGHT_dfcache_create`). Перемещён в `night.h`.
- **Ошибка линковки:** `hidden_roof_index[128][128]` — в оригинале не `static`, доступен из `aeng.cpp` через `extern`. При переносе ошибочно сделан `static` в `old/night.cpp`. Исправлено на non-static.
- **CHECK B fix:** `NIGHT_llight[NIGHT_MAX_LLIGHTS]` и `NIGHT_llight_upto` — сначала оставлены как `static` в `night.cpp`, но правило требует все globals в `_globals`. Перемещены в `night_globals.cpp`. Тип `NIGHT_Llight` и макрос `NIGHT_MAX_LLIGHTS` перемещены в `night.h`.
- **CHECK A fix:** `NIGHT_Precalc`, `NIGHT_Preblock`, `NIGHT_Point`, `NIGHT_MAX_POINTS` — отсутствовали `uc_orig` комментарии. Добавлены.
- **CHECK A fix:** `NIGHT_specular_enable` extern — отсутствовал `uc_orig`. Добавлен (origin: `Controls.cpp`).
- `NIGHT_slight_init`, `NIGHT_slight_compress`, `NIGHT_dlight_init`, `NIGHT_cache_init`, `NIGHT_dfcache_init`, `NIGHT_dlight_squares_do` — в оригинале не в header, но `NIGHT_init()` (осталась в `old/night.cpp`) вызывает первые 4 из них. Решение: убран `static`, добавлены объявления в `night.h`.
- Сборка [296/296] ОК, 52 предупреждения (все из old/ файлов, не новые).

---

## Итерация 67 — ai/mav (второй чанк: MAV_draw..MAV_turn_car_movement_off) (2026-03-21)

- `TRACE` — в оригинальном `MFStdLib.h` был no-op `#define TRACE`; убран при переносе библиотеки. Добавлен `#ifndef TRACE / #define TRACE(...) / #endif` в `new/ai/mav.cpp`.
- `WARE_ware` / `WARE_nav` не объявлены через `world/environment/ware.h` — нужен явный `#include "world/environment/ware_globals.h"`.
- `MAP_HEIGHT` / `MAP_WIDTH` не объявлены в `mav_globals.h` — добавлен `#include "fallen/Headers/Map.h"` (редирект в `world/map/map.h`).
- Первые реализации `MAV_draw` и `MAV_precalculate_warehouse_nav` были написаны неверно (`DRAW3D_line` не существует; warehouse-функция потеряла ~220 строк логики: лестницы, прыжки, staircase hack). Переписаны строго по оригиналу после чтения `original_game/fallen/Source/mav.cpp`.
- `PQ_Type` + `PQ_HEAP_MAX_SIZE` + `PQ_better` — local definitions перед inline-включением `pq.h/pq.cpp` (шаблонный heap). Паттерн воспроизведён 1:1.
- old/fallen/Source/mav.cpp полностью заменён redirect-заглушкой.

---

## Итерация 66 — ai/mav (первый чанк: MAV_init..MAV_precalculate) (2026-03-21)

- `StoreMavOpts` — в оригинале `static`, но `MAV_precalculate_warehouse_nav` (остаётся в old mav.cpp) её вызывает. Сделана публичной, добавлена в `mav.h`.
- Include-порядок: `Game.h` должен идти первым — он подтягивает `MFStdLib.h` (нужен для `MFFileHandle` в `supermap.h`) и `string.h` (нужен для `strcpy` в `anim.h`). Добавлен этот паттерн как в `new/ai/mav.cpp`, так и в `old/mav.cpp` и `new/ai/mav_globals.h`.
- Ошибочная первая реализация `MAV_calc_height_array` использовала неверные поля структуры (bounding box); переписана точно по оригиналу: `db->Walkable` → `dw->Next` → `dw->StartFace4..EndFace4` → `roof_faces4[j].RX/RZ/Y`.
- Три ошибки компиляции исправлены (undeclared `StoreMavOpts`, undeclared `strcpy` в двух единицах трансляции), сборка [294/294] ОК.

---

## Итерация 65 — actors/items/special (2026-03-21)

- Циклический include: `Game.h` → `Special.h` (redirect) → `special.h` → `Game.h`. Решение: убран `#include "fallen/Headers/Game.h"` из `special.h`, заменён на `#ifndef THING_INDEX` guard + `#include "fallen/Headers/Structs.h"` (паттерн из `barrel.h`). `MemTable` и `save_table` forward-объявлены с `#ifndef FALLEN_HEADERS_GAME_H` guard чтобы не переопределять.
- `find_empty_special` — в оригинале не `static`, используется из `save.cpp` через `extern`. Ошибочно сделана static; исправлено, добавлена в `special.h`.
- `SAVE_TABLE_SPECIAL` перенесён в `special.h` из `Game.h` (нужен для `MAX_SPECIALS` макроса).
- `actors/items/special.cpp` зависит от `effects/` и `ui/hud/` — DAG-нарушения унаследованные из оригинала; помечены `// Temporary:`.

---

## Итерация 64 — assets/texture (2026-03-21)

- `TEXTURE_get_fiddled_position` существует только в Glide-версии (`fallen/Glide Engine/Source/gltexture.cpp`); добавлен stub возвращающий `page` как есть — функция объявлена в `texture.h` и вызывается из `aeng.cpp`, но в D3D-билде не используется по смыслу.
- `TEXTURE_WORLD_DIR` объявлен в `fallen/Headers/io.h` (определён в `io.cpp`, ещё не мигрирован); добавлен `#include "fallen/Headers/io.h"` в `texture.cpp` как временный include.
- DC pack (`TEXTURE_DC_pack[]`, `TEXTURE_DC_pack_load_page`, `TEXTURE_DC_pack_init`) — Dreamcast-специфичный код, `TEXTURE_ENABLE_DC_PACKING=0` на PC; оставлен в файле как static (не в globals) поскольку снаружи не используется.
- `IndividualTextures` — file-internal bool, оставлен static в `texture.cpp`.

---

## Итерация 0 — Подготовка (2026-03-20)

**Создано:**
- `tools/entity_map.py` + `new_game_planning/entity_mapping.json`
- `src/old/` — старый код перемещён: `src/fallen/` → `src/old/fallen/`, `src/MFStdLib/` → `src/old/MFStdLib/`
- `src/new/` — создана целевая иерархия папок (пустая)
- CMakeLists.txt обновлён: пути источников, include directories (добавлены `src/new/` и `src/old/`)

**Компиляция:** Release ✅, Debug ✅

**Анализ DAG (автоматический):**
- `MFStdLib` (StdMem, StdMaths и др.) — чистый core, нет зависимостей на проект ✓
- `Game.h` — мега-хаб: 26 инклудов, включается в 22+ файлах. Это **не блокер** — `new/` файлы пишутся чистыми, без Game.h
- Нарушения слоёв в `old/`: `MFx.h` (аудио) включает `Thing.h`; `poly.cpp` (graphics) включает `Game.h`/`night.h`/`eway.h` — остаются в `old/` как есть, в `new/` будут чистые include-ы
- Порядок миграции из stage4_rules.md **корректен** — начинаем с core/ (математика, память)

---

## Итерация 1 — core/types + core/macros + core/memory (2026-03-20)

- `memory.h` включает `<windef.h>` для BOOL — временная Windows-зависимость до Этапа 8
- No-op ASSERT вызовы из memory.cpp убраны

---

## Итерация 2 — core/math (2026-03-20)

- `math.h` включает `<cmath>` и `<cstdlib>` (для sqrt в Root() и abs в Hypotenuse)

---

## Итерация 3 — engine/io/file (2026-03-20)

- `TraceText` оставлен в `old/StdFile.cpp` — относится к Host section, не к файловому I/O
- `TraceText` нигде не вызывается (мёртвый код), удалится при миграции Host section
- `file.h` включает `<windows.h>` напрямую для HANDLE/BOOL — self-contained до Этапа 8

---

## Итерация 4 — engine/input/keyboard + engine/input/mouse (2026-03-20)

Штатно.

---

## Итерация 5 — engine/io/drive + engine/io/async_file (2026-03-20)

- `drive.h` — только декларации; `Drive.cpp` остаётся в `old/` (зависит от `env.h`)
- `AsyncFile2.cpp` полностью перенесён в `async_file.cpp`, удалён из CMakeLists
- `AsyncFile.h` (старая версия) — редирект на `async_file.h`, как и `AsyncFile2.h`

---

## Итерация 6 — engine/input/keyboard.cpp + engine/input/mouse.cpp (2026-03-20)

- `mouse.cpp` временно включает `DDlib.h` для `hDDLibWindow` (GDisplay ещё не мигрирован)
- `MouseDX`/`MouseDY`/`RecenterMouse` перенесены из `DDlib.h` в `mouse.h`
- ASSERT вызов в `ClearLatchedKeys` убран (no-op макрос)

---

## Фикс — вынос глобалов в `_globals` файлы (2026-03-20)

Правило было в stage4_rules.md с самого начала, но игнорировалось итерации 1–6.
Созданы: `memory_globals`, `math_globals`, `keyboard_globals`, `mouse_globals` (.h + .cpp).
Entity mapping обновлён (33 записи — file path).
Чеклист самопроверки дополнен пунктом B (проверка `_globals`).

---

## Итерация 7 — engine/input/joystick (DIManager) (2026-03-20)

- `ClearPrimaryDevice` не перенесён — мёртвая декларация, нигде не вызывается
- `DIRECTINPUT_VERSION 0x0700` нужен перед `<dinput.h>` — добавлен в joystick_globals.h и joystick.cpp
- MOUSE/KEYBOARD/JOYSTICK макросы убраны из MFStdLib.h (дубликат, теперь в joystick.h через DDlib.h → DIManager.h redirect)

---

## Итерация 8 — core/fixed_math + core/matrix + core/fmatrix (2026-03-20)

- `fmatrix.cpp` временно включает `fallen/Headers/prim.h` — нужны struct Matrix33/CMatrix33/Matrix31/SMatrix31 + CMAT masks
- `MATRIX_find_angles_old` перенесён (неиспользуемая старая версия, но код 1:1)
- `MATRIX_calc_int` перенесён — не объявлен в хедере, но определён в оригинале
- ASSERT вызовы в matrix_transform/matrix_transform_small убраны (no-op)
- `ray.h` (DDEngine) — мёртвый файл (нет реализации, нигде не вызывается), не переносить

---

## Итерация 10 — core/fmatrix типы + core/vector + maths.cpp (2026-03-20)

- Matrix33/CMatrix33/Matrix31/SMatrix31 + SMAT/CMAT макросы перенесены из prim.h в fmatrix.h
- prim.h теперь включает core/fmatrix.h вместо дублирующих определений
- Structs.h теперь включает core/vector.h (SVector/SmallSVector/SVECTOR/GameCoord/TinyXZ)
- matrix_mult33 и rotate_obj перенесены из maths.cpp в core/fmatrix.cpp
- MATHS_seg_intersect перенесён из maths.cpp в core/math.cpp
- maths.cpp удалён (опустел), убран из CMakeLists
- Временные `#include "fallen/Headers/prim.h"` удалены из fmatrix.cpp и quaternion.cpp

---

## Итерация 9 — assets/file_clump + assets/tga + core/quaternion (2026-03-20)

- `tga.h` потребовал `<windows.h>` для `BOOL` в сигнатуре `TGA_load`
- `quaternion.cpp` агент использовал `<windef.h>` без `_WIN32` — заменено на `<windows.h>`
- `quaternion.cpp` временно включает `fallen/Headers/prim.h` (Matrix33/CMatrix33 + CMAT masks)
- tga_width/tga_height/TGA_header — non-static глобалы → вынесены в `assets/tga_globals.h/.cpp`
- Первая итерация с использованием subagent'ов (tga + quaternion параллельно)

---

## Итерация 11 — assets/image_compression + assets/compression + engine/lighting/ngamut + engine/graphics/graphics_api/work_screen (2026-03-20)

- `work_screen_globals.h` использовала `<DDLib.h>` → циклический include (`DDLib.h` → `GWorkScreen.h` → `work_screen.h` → `work_screen_globals.h`); исправлено заменой на `"core/types.h"` (MFRect и SLONG там определены)
- Все file-private глобалы в compression.cpp и image_compression.cpp помечены `static` (не экспортируются)
- `NGAMUT_add_square` — `static inline` в ngamut.cpp
- Баг `MAX3` в NGamut.cpp (несовпадающие скобки: `MAX(MAX(a,b,c))`) перенесён 1:1; MAX3 нигде не используется в реальном коде
- Фикс после итерации: `COMP_tga_data`, `COMP_tga_info`, `COMP_data`, `COMP_frame` (comp.cpp) и `test` (ic.cpp) были ошибочно помечены `static` — в оригинале они не были static; исправлено через вынос в `compression_globals` и `image_compression_globals`
- `COMP_data` — анонимный struct в оригинале; для `extern`-декларации в globals.h потребовалась именованная обёртка `COMP_DataBuffer` (определена в compression.h); `COMP_MAX_DATA` перенесён из compression.cpp в compression.h (нужен для типа)

---

## Итерация 12 — engine/graphics/geometry/aa + core/timer + assets/bink_client + engine/io/drive + engine/graphics/pipeline/message (2026-03-20)

- `Gamut.cpp` удалён как мёртвый код — `build_gamut_table`/`draw_gamut`/`gamut_ele_pool`/`gamut_ele_ptr` нигде не вызываются
- `timer.cpp`: агент изменил ULONG → LONGLONG в `stopwatch_start`/`GetFineTimerFreq`/`GetFineTimerValue` — откатано вручную до оригинального поведения с усечением до 32 бит (`.u.LowPart`)
- `BreakTimer.h` содержал no-op макросы (`BreakTime(X)` и др.) используемые в `aeng.cpp` — перенесены в `core/timer.h`
- `MSG_add` в старом `Message.h` был закомментирован, но функция реально используется. В новом `message.h` добавлена активная декларация с `char*` (= CBYTE*) — иначе линкер не находит символ.
- `drive.cpp` включает `env.h` через temporary include — `env.h` использует CBYTE/SLONG, поэтому перед ним добавлен `#include "core/types.h"`
- `bink_client.h` forward-декларирует `struct IDirectDrawSurface` — не включает `<ddraw.h>` чтобы не тянуть весь COM стек в header

---

## Итерация 13 — engine/audio/mfx (2026-03-20)

- `MFX.h` включал `thing.h` → `Game.h`, давая всем включателям неявный доступ к `the_game`, `MAX_THINGS` и т.д. После замены на redirect к `engine/audio/mfx.h` (который только forward-декларирует `struct Thing;`) сломались `GHost.cpp` и `music.cpp` — добавлены явные `#include "game.h"` в обоих файлах
- `alDevice` и `alContext` в оригинале не-static, но не объявлены ни в одном заголовке → помечены `static` в новом файле как file-private
- `SetLinearScale` и `SetPower` объявлены без `static` в оригинале, но нигде не используются снаружи → помечены `static` в новом файле

---

## Итерация 14 — engine/graphics/resources/d3d_texture + engine/graphics/graphics_api/wind_procs + host (2026-03-21)

- `wind_procs.h` временно включает `DDManager.h` (через `<windows.h>/<ddraw.h>/<d3d.h>` prefix) — `ChangeDDInfo` использует `DDDriverInfo/D3DDeviceInfo/DDModeInfo`
- `d3d_texture.cpp` требовал `#include <MFStdLib.h>` для `ASSERT/MFnew/MFdelete` (оригинал шёл через `DDLib.h`)
- `wind_procs.cpp` требовал явного `#include "assets/bink_client.h"` для `BinkMessage` (оригинал шёл через `DDLib.h`)
- `host_globals.h` — `UWORD` тянется через Windows SDK (winsmcrd.h → typedef WORD UWORD); добавлен `core/types.h` для явности
- `quaternion_globals.h` — предыдущий баг: `BOOL` не было определено при компиляции в изоляции; исправлено добавлением `<windows.h>`
- `drive.cpp` — предыдущий баг: `core/types.h` не был добавлен перед `env.h` (хотя итерация 12 планировала это); исправлено
- `GHost.cpp`, `WindProcs.cpp`, `D3DTexture.cpp` — удалены из CMakeLists, старые хедеры стали редиректами

---

## Итерация 15 — engine/graphics/pipeline/render_state + bucket + vertex_buffer (2026-03-21)

- `vertex_buffer.h` потребовал `<MFStdLib.h>` (для `ASSERT` в inline `GetPtr()`) и `<stdio.h>` (для `FILE*` в `DumpInfo`) — без них build падал
- `vertex_buffer.cpp` включал `console.h` и `poly.h` в оригинале — оба не используются в коде файла; в новом не включены
- `RenderState::s_State` — статический член класса, определён в render_state.cpp (не в _globals) — это корректно, static member обязан определяться в .cpp класса
- `vertex_pool[]` в bucket.h — extern объявление (определение в aeng.cpp, ещё не перенесён)

---

## Итерация 16 — engine/graphics/pipeline/polypoint + polypage (2026-03-21)

- `polypage.cpp` требовал `core/matrix.h` для `MATRIX_MUL` (в `GenerateMMMatrixFromStandardD3DOnes`) — в оригинале шёл через `matrix.h`
- Инлайн `DRAWN_PP` макрос (`#define DRAWN_PP ppDrawn`) не перенесён — это был локальный алиас переменной `ppDrawn` в той же функции, замена прямая
- `FanAlloc` и `SetTexOffset(UBYTE)` — dead declarations без реализации, не перенесены
- `ppLastPolyPageSetup` — объявлен в оригинале но нигде не используется, не перенесён
- `PolyPage::s_AlphaSort` и другие статические члены — определены в `polypage.cpp` (не в `_globals`), аналогично прецеденту `RenderState::s_State` из итерации 15
- `polypage.h` включает `poly.h` (temporary) — `POLY_Point` ещё не мигрирован

---

## Итерация 17 — engine/graphics/resources/font (2026-03-21)

- `font.cpp` включает `<MFStdLib.h>` перед `GDisplay.h` — MFStdLib тянет `<windows.h>/<ddraw.h>/<d3d.h>`, без этого DDManager.h не компилируется
- Bit-pattern макросы (`_____` … `xxxxx`) вынесены в `font_globals.cpp` вместе с данными шрифта — они используются только для инициализации таблиц

---

## Итерация 18 — engine/graphics/pipeline/poly.h + dd_manager.h + gd_display.h (header-only) (2026-03-21)

- Header-only миграция: все три `.cpp` реализации остаются в `old/` (зависят от `game.h`/`night.h`/`eway.h`)
- `poly.h` не имел своих `#include` — стал чистым заголовком с единственной зависимостью `core/types.h`
- `dd_manager.h` самодостаточен: только `<windows.h>/<ddraw.h>/<d3d.h>` + `core/types.h`
- `gd_display.h` включает `dd_manager.h` + `d3d_texture.h` — оба уже в `new/`
- `wind_procs.h` освобождён от temporary `#include "fallen/DDLibrary/Headers/DDManager.h"` — теперь напрямую `dd_manager.h`
- Разблокирует миграцию файлов зависящих от `poly.h` (crinkle, facet, mesh, console, Text, aeng и др.)

---

## Итерация 19 — engine/graphics/geometry/sprite + engine/graphics/resources/menufont (2026-03-21)

- `MENUFONT_DrawFlanged` и `MENUFONT_DrawFutzed` не перенесены — мёртвый код: ASSERT в `MENUFONT_Draw` подтверждает, что эти флаги никогда не передаются
- `MENUFONT_MergeLower` отсутствовал в оригинальном `menufont.h`, но вызывается снаружи (`frontend.cpp` l.4106–4107 через forward declaration) — добавлен в `menufont.h`
- `MENUFONT_fadein_char` — file-private, помечен `static`
- `PANEL_draw_quad` — forward declaration в `menufont.cpp` (panel.cpp ещё не мигрирован, зависит от `game.h`)

---

## Итерация 21 — assets/xlat_str + assets/sound_id (2026-03-21)

- `previous_byte` (static file-scope) перемещён в `xlat_str_globals` по требованию правила — в `xlat_str.cpp` глобальных переменных не осталось
- `sound_id.cpp` (new/) — near-empty stub; единственный смысловой файл `sound_id_globals.cpp`

---

## Итерация 20 — world/map/qmap (header+globals) + pipeline/qeng + pipeline/text (2026-03-21)

- `qmap.h` — только header+globals; `qmap.cpp` остаётся в `old/` (включает `game.h`); глобалы вынесены из `qmap.cpp` в `qmap_globals.cpp`
- `qmap.h` содержит дублирующие `extern`-декларации с `qmap_globals.h` — допустимо в C++, определение одно в `qmap_globals.cpp`
- `text.cpp` — временные `extern D3DTexture TEXTURE_texture[]` и `extern SLONG TEXTURE_page_font` вместо include `texture.h` (цепочка `texture.h → crinkle.h → aeng.h` не поддаётся temporary include из new/)
- `qeng.cpp` полностью мигрирован, old/qeng.cpp удалён; `Text.cpp` полностью мигрирован, old/Text.cpp удалён

---

## Итерация 22 — engine/graphics/resources/font2d + console (2026-03-21)

- `font2d_data` (временный буфер TGA для инициализации атласа) — хранится как `void*` в `_globals`, чтобы не тащить `assets/tga.h` в слой `engine/graphics`. Настоящий тип `TGA_Pixel[256][256]`, локальный typedef `FontAtlasPixels` в `font2d.cpp`, доступ через макрос `FONT2D_DATA`
- `FONT2D_DrawString_NoTrueType` — file-private static, в новом файле тоже static; вызывается только из `FONT2D_DrawString_3d`
- `FONT2D_DrawString_3d` — не объявлена в оригинальном `font2d.h`, но вызывается из `facet.cpp` и `guns.cpp` через `extern`; добавлена в `font2d.h`
- `CONSOLE_status` — не объявлена в оригинальном `console.h`, вызывается из `Controls.cpp` через `extern`; добавлена в `console.h`
- `console_Data`, `console_status_text`, `console_last_tick`, `console_this_tick` — в оригинале были static; вынесены в `_globals` по правилу, переименованы через conflict-конвенцию (префикс `console_`)
- `PANEL_new_text` / `PANEL_draw_quad` / `PANEL_GetNextDepthBodge` — forward declarations в `console.cpp` / `font2d.cpp`; panel.cpp ещё зависит от `game.h`

---

## Итерация 24 — engine/lighting/crinkle + engine/effects/flamengine + header-only: aeng.h, crinkle.h, texture.h, superfacet.h (2026-03-21)

- `aeng.h` → header-only `engine/graphics/pipeline/aeng.h`; содержит `SVector_F` (typedef struct, ранее только в aeng.h); TEXTURE_* функции сохранены в обоих headers (aeng.h и texture.h) как было в оригинале — дублирование корректно, определения одни
- `texture.h` → header-only `assets/texture.h`; `TEXTURE_fix_prim_textures` — оригинальный `texture.h` объявлял `(SLONG flag)`, реализация `()` — исправлено на `void` согласно реализации
- `crinkle.h` → header-only `engine/lighting/crinkle.h`; полная реализация в `crinkle.cpp` — старый `Crinkle.cpp` включал `game.h` но ничего из него не использовал
- `flamengine.cpp` — статические lock/unlock helper'ы `TEXTURE_flame_lock/unlock/update` остались file-private (static); класс `Flamengine::Feedback()` использует `the_display` из `gd_display.h`
- `crinkle.cpp` добавлен `<MFStdLib.h>` для `ASSERT/WITHIN/SATURATE/SWAP_FL/MF_Fopen/MF_Fclose`
- `flamengine.cpp` — заменён `#include "assets/texture.h"` на прямые `extern` декларации (DAG: `engine/` не должен включать `assets/`); паттерн такой же как в `text.cpp` (итерация 20)
- `crinkle.h` включает `assets/file_clump.h` — pre-existing coupling из оригинала (FileClump в публичном API), tech debt
- В entity_mapping: 5 макросов `CRINKLE_MAX_*` и `ZONES` добавлены после review (были пропущены при первоначальном добавлении)

---

## Итерация 23 — core/heap + assets/anim_tmap + engine/animation/morph + engine/net + world/environment/outline (2026-03-21)

- `MORPH_filename` — в оригинале `CBYTE*` (= `char*`), в новом `const char*` (строковые литералы); семантически корректнее, поведение идентично
- `morph.cpp` — sscanf format `%d` vs `SLONG*` (long*): pre-existing warning из оригинала, перенесён 1:1
- `OUTLINE_insert_link` — баг оригинала сохранён 1:1: `if (ol->x == OUTLINE_LINK_TYPE_END)` вместо `ol->type`
- `OUTLINE_intersects` — переменная `mz2` (была в оригинале, но не использовалась) удалена как dead variable; логика не изменена
- `outline.cpp` — no globals, нет `_globals` файлов: все данные heap-allocated, на стеке или локальные
- `engine/net/net.cpp` — полный stub; `NET_get_player_name` возвращает `"Unknown"` как `CBYTE*` (pre-existing warning из оригинала)

---

## Итерация 25 — engine/io/env + engine/graphics/pipeline/wibble + engine/graphics/resources/truetype (2026-03-21)

- `env2.cpp` — мёртвые инклуды `game.h`, `Interfac.h`, `menufont.h` не перенесены (не использовались в теле)
- `wibble.cpp` — 6 файл-скоп глобалов (`mul_y1/2`, `mul_g1/2`, `shift1/2`) объявлены в оригинале но не используются в `WIBBLE_simple` — перенесены в `_globals` по правилу, dead
- `wibble.cpp` — `fallen/Headers/Game.h` включается ПЕРВЫМ перед `gd_display.h`: `Game.h → MFStdLib.h` объявляет `extern SLONG DisplayWidth/DisplayHeight`, а `gd_display.h` переопределяет их как `#define 640/480`; если порядок обратный — синтаксическая ошибка
- `truetype.cpp` — все локальные static-переменные переименованы через `tt_` префикс + переданы через `_globals`; в `.cpp` сделаны `#define`-алиасы обратно на оригинальные имена для читаемости
- `truetype.cpp` — `ShowDebug` — объявлена как static forward declaration, реализации нет ни в оригинале ни в новом файле (dead declaration в пре-релизной версии)

---

## Итерация 27 — effects/spark + effects/ribbon + effects/drip (2026-03-21)

- `spark.h` включает `fallen/Headers/Game.h` для `THING_INDEX` (используется в `SPARK_Pinfo.person`)
- `spark_globals.h` включает `fallen/Headers/Map.h` для `MAP_HEIGHT` (размер массива `SPARK_mapwho`)
- `Ribbons[]` (static file-private в оригинале) → переименована в `ribbon_Ribbons` (conflict rename, флаг `--conflict`)
- `NextRibbon` в `RIBBON_alloc` — static local внутри функции, остаётся в `.cpp` (не file-scope global)
- `ribbon.cpp` включает `DrawXtra.h` — временный include для `RIBBON_draw_ribbon` (drawxtra.cpp не мигрирован)

---

## Итерация 26 — effects/fire + effects/mist + effects/glitter (2026-03-21)

- Первая итерация в новой директории `effects/` (game-level) — создана
- `FIRE_Flame`, `FIRE_Fire`, `MIST_Point`, `MIST_Mist`, `GLITTER_Spark`, `GLITTER_Glitter` — file-private structs, вынесены в `.h` (нужны для `_globals`)
- `fire.cpp`/`mist.cpp` include `game.h` — ожидаемо для game-level effects (GAME_TURN, TICK_INV_RATIO)
- `FIRE_get_next` — unfinished pre-release stub; always returns NULL; перенесён 1:1
- `calc_height_at` — forward declaration в `mist.cpp`, вызов закомментирован в оригинале; перенесён с `uc_orig`
- `static SLONG type_cycle` в `MIST_create` — static local (функциональная), не file-scope глобал; остаётся в .cpp

---

## Итерация 28 — engine/graphics/geometry/oval + world/environment/build + ui/hud/planmap (2026-03-21)

- `LIGHT_building_point` и `LIGHT_amb_colour` определены в оригинальном `build.cpp` (не в light.cpp), `light.h` лишь объявляет их как `extern`; вынесены в `build_globals`
- `build_globals.h` включает `game.h` (temporary) перед `light.h` — `light.h` требует `THING_INDEX`, `GameCoord` которые приходят через `game.h`; без этого `light.h` не компилируется в изоляции
- Первая итерация с созданием директории `ui/hud/` (ранее не существовала)
- `planmap.cpp` — `EDGE_LEFT/TOP/RIGHT/BOTTOM` — file-private `#define`s в `.cpp`, добавлены `uc_orig` + записи в mapping

---

## Итерация 29 — engine/graphics/graphics_api/dd_manager (2026-03-21)

- `DDManager.cpp` включал `game.h` и `env.h` но использовал только `ENV_get_value_number/set` — оба инклуда в `new/` заменены на нужные (`engine/io/env.h`, `core/memory.h`)
- `OS_calculate_mask_and_shift` (из `D3DTexture.cpp`) вызывается через `extern` forward declaration — чтобы не тянуть `resources/` include в `graphics_api/` (DAG нарушение)
- `CanDoAdamiLighting` и `CanDoForsythLighting` инициализируются в конструкторе `= false`; в оригинале они не инициализировались (только `CanDoModulateAlpha`/`CanDoDestInvSourceColour` инициализированы явно) — безопасно
- Закомментированный overload `DDDriverManager::FindDriver(DDCAPS*, ...)` (~110 строк) не перенесён — полностью мёртвый код в `/* */` блоке

---

## Итерация 30 — engine/lighting/shadow + effects/fog + actors/core/drawtype (2026-03-21)

- Создана первая директория `actors/core/` — первая итерация с `actors/` слоем
- `drawtype.h` новый: `MAX_DRAW_TWEENS`/`MAX_DRAW_MESHES` оставлены как макросы через `save_table` (как в оригинале); разрешаются при использовании когда `Game.h` уже включён
- `count_draw_tween` не перенесён — мёртвый код, нигде не вызывается
- `drawtype.h` содержит temporary `#include "fallen/Headers/cache.h"` — `CACHE_Index` нужен для `DrawMesh`, cache не мигрирован
- `fog.cpp` — `calc_height_at` — forward declaration присутствует как в оригинале; фактически не вызывается (вместо него `PAP_calc_height_at`)
- `shadow_1`/`shadow_2` в `SHADOW_do` — declared-but-unused переменные из оригинала, перенесены 1:1
- `shadow.cpp` не включает `fallen/Headers/shadow.h` (сам является реализацией; включение было бы циклическим через redirect)

---
## Итерация 31 — world/map/pap + world/navigation/walkable (2026-03-21)

- `pap.cpp` включает `Game.h` + `mav.h` + `ware.h` (temporary) — нужны `GAME_FLAGS/GF_NO_FLOOR`, `MAVHEIGHT`, `WARE_calc_height_at`, `FLAG_PERSON_WAREHOUSE`
- `PAP_assert_if_off_map_lo/hi` — static (в оригинале non-static, но нигде не объявлены в хедерах и не вызываются снаружи)
- `inside2.h` и `ns.h` из оригинального pap.cpp — не используются в перенесённых функциях, не включены
- `gh_vx/vy/vz` — в оригинале file-scope non-static; вынесены в `walkable_globals` по правилу
- `PAP_FLATTISH_SAMPLES` — `#define` внутри тела функции, не file-scope; маппинг не нужен

---

## Итерация 32 — engine/audio/music + engine/audio/soundenv + effects/tracks + world/map/map (2026-03-21)

- `tracks.h` НЕ включает `game.h` — циклический include: `game.h` → `tracks.h` redirect → `effects/tracks.h` → `game.h`. Решение: `struct Thing;` forward declaration, `UWORD` вместо `THING_INDEX` для `Track.thing` (аналогично pap.h)
- `tracks.cpp` — `game.h` идёт первым include'ом, иначе типы не резолвятся. `sound.h` исключён — цепочка `sound.h` → `Structs.h` → `anim.h` вызывала ошибки `strcpy`/`rand` undeclared; `world_type` берётся через `extern`, `WORLD_TYPE_SNOW` переопределён локально
- `soundenv.cpp` — `<MFStdLib.h>` идёт первым (нужен `BOOL`); `AudioGroundQuad` определён в `soundenv_globals.h` (не был в оригинале — там просто массив int'ов без типа)
- `map.h` включает `building.h` (для `RMAX_PRIM_POINTS`) и guard `#ifndef THING_INDEX` перед `#include "light.h"` — `light.h` требует оба типа без `game.h`
- `MAP_light_map` определён в `map.cpp` (non-trivial initializer); `map_globals.cpp` — пустой stub для соответствия паттерну `_globals`
- `MapElement.MapWho` — `UWORD` вместо `THING_INDEX` (аналогично Track.thing — избегаем game.h зависимости в map.h)

---

## Итерация 33 — world/environment/door + engine/physics/sm (2026-03-21)

- `DOOR_find` и `DOOR_knock_over_people` — не объявлены в `door.h`, нигде не вызываются снаружи → помечены `static`
- `sm.cpp` (`engine/physics/`) включает `world/map/pap.h` — нарушение DAG (`engine/` не должен зависеть от `world/`); pre-existing coupling из оригинала, перенесён 1:1; tech debt
- Приватные structs `SM_Sphere`, `SM_Link`, `SM_Object` из `sm.cpp` вынесены в `sm_globals.h` (нужны для объявления extern-массивов)

---

## Итерация 35 — engine/graphics/geometry/cone + farfacet (2026-03-21)

- `cone.cpp` включает `fallen/Headers/memory.h` напрямую — `fallen/Headers/Game.h` не тянет его транзитивно (Memory.h давно вынесен из цепочки); `facet_links`/`dfacets` объявлены только там
- `CONE_interpolate_colour` и `CONE_insert_points` — file-private `static`, не объявлены в хедере; в новом файле тоже `static`
- Макросы `CONE_MAX_POLY_POINTS`, `CONE_SWAP_POINT_INFO`, `CONE_MAX_DONE`, `FARFACET_MAX_STRIP_LENGTH`, `MY_PROJ_MATRIX_SCALE` — внутри тел функций, маппинг не нужен
- `farfacet.cpp` включает `fallen/Headers/memory.h` по той же причине

---

## Итерация 34 — engine/effects/psystem (2026-03-21)

- `PARTICLE_Draw` — реализован в `drawxtra.cpp` (DDEngine), не в psystem.cpp; остался в redirect `PSystem.h` как самостоятельная декларация
- `set_person_dead` — в forward declaration использовал `BOOL b1, BOOL b2` вместо `SLONG behind, SLONG height`; компиляция прошла но линкер не нашёл символ; исправлено до `SLONG`
- `engine/effects/psystem.cpp` включает `world/map/pap.h` — DAG нарушение (`engine/` → `world/`), аналогично sm.cpp из итерации 33; pre-existing coupling из оригинала, перенесён 1:1; tech debt

---

## Итерация 36 — engine/graphics/pipeline/sky (2026-03-21)

- `sky.cpp` включает `fallen/Headers/Game.h` первым — нужен `GAME_TURN` (для wibble лунного отражения); временная зависимость на `game.h`, порядок инклудов критичен (та же причина что в итерации 25: game.h → MFStdLib.h объявляет extern DisplayWidth/Height, а gd_display.h перебивает их макросами)
- `SKY_draw_poly_sky_old` не объявлен в оригинальном `sky.h`, но вызывается из `aeng.cpp` через extern — добавлен в новый `sky.h`
- `cam.h` включался в оригинале, но ни один CAM_* символ не используется — не перенесён

---

## Итерация 37 — engine/graphics/geometry/fastprim (2026-03-21)

- В оригинале блок `kludge_shrink` дублирован (строки 904–946 повторяются дважды с повторным `extern UBYTE kludge_shrink;`) — перенесён 1:1 как баг оригинала
- `assets/texture.h` включён для `FACE_PAGE_OFFSET` — DAG нарушение (`engine/` → `assets/`), помечено `// Temporary:`, pre-existing coupling из оригинала
- `Night.h` — имя с заглавной буквой на диске; исправлен регистр в include после первоначальной опечатки
- `FASTPRIM_find_texture_from_page` — статическая вспомогательная функция в оригинале не объявлена нигде, объявлена в новом `.cpp` без `static` (нужна linkage для возможного использования снаружи в будущем — как в оригинале не была static)

---

## Итерация 39 — world/navigation/inside2 + world/environment/build2 (2026-03-21)

- `stair_teleport_bodge` — объявлена в оригинальном `inside2.h`, но реализация полностью закомментирована и нигде не вызывается → не перенесена (мёртвый код)
- `find_stair_in` — файл-приватный хелпер, не объявлен в оригинальном хедере → `static` в новом `.cpp`
- `set_nogo_pap_flags` в `build2.cpp` — тело функции полностью закомментировано блоком `/* ... */` в оригинале → не перенесена (мёртвый код)
- `calc_ladder_ends` сделана публичной (не `static`): вызывается из `Building.cpp` через extern-declaration — статическая линковка её не найдёт; добавлена в `build2.h`
- Компиляция: SVector incomplete type в `fastprim.cpp` при включении `building.h` через `memory.h` — причина: новый redirect `inside2.h` не тянет `structs.h` транзитивно; фикс: добавлен `#include "structs.h"` в `old/fallen/Headers/memory.h` перед `building.h`
- PSX-специфика в оригинальном `supermap.cpp` (TIM-файлы, палитры) — отложена; `calc_inside_for_xyz` тоже в `supermap.cpp` и зависит от `find_inside_room` — кандидат на следующую итерацию
- Entity mapping: 29 новых записей (публичные функции, приватные static хелперы, structs, макросы, globals)

---

## Итерация 38 — engine/graphics/pipeline/poly_render + engine/lighting/smap (2026-03-21)

- `RenderStates_OK` — `static bool` в оригинале, оставлен в `poly_render.cpp` (не в _globals): единственное исключение из правила globals — это implementation state, не покидает файл, так сохраняет исходную инкапсуляцию
- `polyrenderstate.cpp` не имел собственного заголовка; все публичные функции уже объявлены в `poly.h` — создан минимальный `poly_render.h` только как redirect на `poly.h`
- `SMAP_bike` объявлена в оригинальном `smap.h`, но никогда не была реализована — перенесена как пустая заглушка для ABI-совместимости
- `SMAP_get_world_pos` — только forward declaration в оригинальном `smap.cpp` без тела — не перенесена (мёртвый код)
- `smap.h` включает `engine/graphics/pipeline/aeng.h` для `SVector_F` в публичном API — DAG нарушение `engine/lighting` → `engine/graphics`, помечено `// Temporary:`

---

## Итерация 40 — actors/core/state + switch + hierarchy + player + actors/characters/cop + roper + thug + snipe + assets/startscr (2026-03-21)

- `switch_functions`, `cop_states`, `roper_states`, `thug_states` — исходно внутри `.cpp` файлов (switch как `static`, остальные как non-static); по правилу ALL globals → `_globals`; созданы `switch_globals`, `cop_globals`, `roper_globals`, `thug_globals` файлы
- `darci_states` — не перенесён (Darci.cpp ещё не мигрирован); объявлен через `extern StateFunction darci_states[]` в `player_globals.cpp` с пометкой
- Redirect-заголовки `old/Cop.h`, `old/Roper.h` дополнены `#include "*_globals.h"` — `Person.cpp` использует `cop_states`/`roper_states`
- `snipe.h` (new): SNIPE_* extern-переменные убраны из публичного хедера → только в `snipe_globals.h`; redirect `old/snipe.h` дополнен `#include "snipe_globals.h"`
- `startscr.h` (new) содержала только extern `STARTSCR_mission` — убрана (variables only in _globals); `.h` остался как пустой placeholder для будущего содержимого
- `fn_cop_fight` — объявлена как `extern` forward decl и одновременно определена в `cop.cpp` → дублирование убрано
- Ошибки компиляции при циклических include через `Game.h` → `Player.h`/`Switch.h`: решено по паттерну из предыдущих итераций (`#ifndef THING_INDEX + Structs.h` вместо `Game.h` в `.h` файлах)

---
## Итерация 41 — actors/characters/darci + actors/animals/animal + actors/vehicles/chopper (2026-03-21)

- `chopper.h` требует `<string.h>` перед `Structs.h` (аналогично animal.h) — `Structs.h` → `anim.h` использует `strcpy`; также зависит от `actors/core/drawtype.h` для типа `DrawMesh`
- `CHOPPER_fn_init`/`CHOPPER_fn_normal` — `memory.cpp` ссылается напрямую → сделаны public; объявлены в `chopper.h`
- `CHOPPER_AVOID_SPEED` — `#define` внутри тела функции `CHOPPER_home` (1:1 с оригиналом)
- `darci_states[]`, `ANIMAL_functions[]`, `CHOPPER_functions[]`/`CIVILIAN_state_function[]` — все перемещены в соответствующие `_globals.cpp`
- `person_slide_inside` — возвращает `SLONG` (не `void`): тип взят из `inside2.h`
- Строки 1026–1167 оригинального `Darci.cpp` — большой `/* */`-комментарий, не мигрировался

---

## Итерация 42 — actors/items/balloon + grenade + dike (2026-03-21)

- Создана директория `actors/items/` (первая итерация с items-слоем)
- `GrenadeArray` (статический в оригинале) и `global_g` — вынесены в `grenade_globals`; `Grenade` struct определён в `grenade_globals.h` (нужен для extern объявления массива)
- `ProcessGrenade` — file-private static, не объявлен в `grenade.h`; `if (gp->dy > 0x400)` оригинала содержал только закомментированные строки → упрощён до `if (gp->dy <= 0x400)` без изменения поведения
- `overdist` в `dike.cpp` — dead variable в оригинале (объявлен, не используется) → не перенесён
- `DIKE_get_grip` — file-private static, не объявлена в оригинальном `dike.h`
- `BALLOON_get_attached_point` — file-private static, не объявлена в оригинальном `balloon.h`

---

## Итерация 43 — actors/items/hook + effects/pow (2026-03-21)

- `HOOK_reel()` — объявлена в оригинальном `hook.h`, реализации нет нигде (мёртвая декларация пре-релиза); сохранена в `hook.h` для ABI-совместимости, не реализована
- `check_pows()` — non-static (оригинал объявляет через local forward decl внутри `POW_init`); перенесена как non-static; `POW_insert_sprite` — non-static в оригинале, помечена `static` как file-private
- `pow.h` включает `<MFStdLib.h>` — нужен `ASSERT` для inline `POW_create`
- `effects/pow.h` включает `world/map/pap.h` (Temporary) — для `PAP_SIZE_LO` в `extern UBYTE POW_mapwho[]`; pre-existing coupling из оригинала
- `sizeof(POW_mapwho)` заменено на `PAP_SIZE_LO * sizeof(POW_mapwho[0])` — extern incomplete array не допускает sizeof

---

## Итерация 45 — actors/items/barrel + actors/animals/canid (2026-03-21)

- `BARREL_position_on_hands` / `BARREL_throw`: объявлены в оригинальном `barrel.h`, но реализации в `/* */` блоке в `barrel.cpp`; вызовы из `Person.cpp` тоже в `/* */`. Оставлены как объявления без реализации — ABI совместимость, линкер не жалуется.
- `BARREL_convert_stationary_to_moving` / `BARREL_convert_moving_to_stationary`: не были в оригинальном `barrel.h`, но вызываются из `memory.cpp` и `eway.cpp` → сделаны публичными (не static).
- `CANID_fn_init` / `CANID_fn_normal`: добавлены в `canid.h` (в оригинале не объявлялись) — нужны для таблицы `CANID_state_function[]` в `canid_globals.cpp`.
- `CANID_fn_normal`: substate switch полностью закомментирован в оригинале (`/* */`) — сохранён 1:1 как закомментированный блок.
- Код «CODE GRAVEYARD» (fly/perch/flee/walk поведения, ~400 строк в `/* */`) — не мигрирован.
- `barrel.cpp`: `#include <string.h>` добавлен перед `fallen/Headers/Structs.h` — `anim.h` использует `strcpy` без включения `<string.h>`.
- `canid_globals.cpp`: добавлен `#include "statedef.h"` — `STATE_INIT`/`STATE_NORMAL` определены там, не в `state.h`.

---

## Итерация 46 — world/environment/puddle + tripwire + world/navigation/wand (2026-03-21)

- `PUDDLE_texture` / `PUDDLE_ripple` — non-static в оригинале, но нигде не используются снаружи; оставлены `static` в `puddle.cpp` (аналогично `alDevice`/`alContext` из итерации 13)
- `WAND_square_for_person` — не объявлена в оригинальном `wand.h`, file-private static хелпер
- `WAND_find_good_start_point` / `WAND_find_good_start_point_near` / `WAND_find_good_start_point_for_car` — не в оригинальном `wand.h`, но вызываются снаружи через extern forward declarations; добавлены в `wand.h`
- `MapElement* me` в `PUDDLE_init` — declared-but-unused, из оригинала, перенесён 1:1

---

## Итерация 44 — world/environment/id.h (header-only) + stair + plat (2026-03-21)

- `id.h` → header-only `world/environment/id.h`; нет зависимостей кроме `core/types.h`; `id.cpp` отсутствует в `old/` (удалён на этапе 2)
- `stair.cpp`: `STAIR_FLAG_UP/DOWN` из `building.h` — не включать `building.h` напрямую (требует `Structs.h` для `SVector`); скопированы как `#ifndef STAIR_FLAG_UP` блок с `// Temporary:` комментарием; `DIV64` из `core/fixed_math.h` — добавлен include
- `plat.cpp` включает `game.h` + unmigrated headers через короткие пути (DDEngine/Headers в include path); `PLAT_plat`/`PLAT_plat_upto` удалены из `plat.h` (дублировались с `plat_globals.h`)
- `STAIR_srand/grand/rand/set_bit/get_bit/get_bit_from_square/check_edge/add_to_storey/is_door` — static file-private helpers; `STAIR_EDGE` — file-private macro

---

## Итерация 47 — actors/core/thing (2026-03-21)

- `nearest_class` (объявлена в оригинальном `thing.h`, тело в `/* */` в Thing.cpp) — не перенесена; была удалена при миграции `switch.cpp` (Switch.cpp вызывал её, но новый switch.cpp обходится без неё)
- `thing.cpp` включает `world/map/pap.h` — DAG нарушение (`actors/` → `world/`); pre-existing coupling из оригинала (map management функции используют PAP_2LO); помечено `// Temporary:`
- `thing_globals.h` дублирует объявление `THING_array`/`THING_ARRAY_SIZE` с `thing.h` — оба файла включаются через redirect; дублирование безвредно (одинаковые значения/extern)
- `GAMEMENU_is_paused` — в оригинальном `gamemenu.h` возвращает `SLONG`, не `BOOL`; исправлено после первой ошибки компиляции
- `slow_mo` — в оригинале `static` file-scope; перенесён в `_globals` по правилу

---

## Итерация 48 — world/map/road (2026-03-21)

- `road.cpp` включает `actors/core/thing.h` — DAG нарушение (`world/` → `actors/`); pre-existing coupling из оригинала (game.h → Thing.h → THING_find_nearest); помечено `// Temporary:`
- `ROAD_find_node/connect/disconnect/intersect/split/add/is_middle` — file-private; в оригинале не были static; сделаны static как file-private helpers
- `page` и `MapElement* me` в `ROAD_sink` — declared-but-unused переменные из оригинала; перенесены 1:1

---

## Итерация 49 — world/environment/ware + ui/pause (2026-03-21)

- `WARE_enter`/`WARE_exit` полностью закомментированы в оригинале (`/* */`), но вызываются из `collide.cpp` → написаны минимальные стабы (WARE_in + NIGHT_destroy_all_cached_info); ambient light save/restore не перенесён — было за `/* */`
- `WARE_Door` — анонимный nested struct из оригинала вынесен в именованный `struct WARE_Door` для extern-декларации; layout идентичен
- `WARE_bounding_box` — в оригинале non-static, но вызывается только из `ware.cpp`; сделана `static`
- `ware_globals.h` добавлен `<string.h>` первым include — `mav.h` → `structs.h` → `anim.h` использует `strcpy`; та же причина что в barrel.cpp (итерация 45)
- `door_x/y/z/door_angle/dy` в `WARE_init` — declared-but-unused переменные из оригинала, перенесены 1:1

---

## Итерация 50 — ui/menus/gamemenu + missions/save + actors/core/effect + actors/characters/enemy (2026-03-21)

- `Effect.cpp`/`Enemy.cpp` — мёртвый код (init_effect/init_enemy нигде не вызываются снаружи); перенесены 1:1 с пометкой в комментарии
- `GAMEMENU_menu[]` + `GAMEMENU_Menu` typedef — перемещены из `gamemenu.cpp` в `gamemenu_globals`; в `gamemenu_globals.cpp` добавлен `#include "assets/xlat_str.h"` для констант X_*
- `SAVE_out_data`/`LOAD_in_data` — в оригинале non-static, но нигде не используются снаружи → помечены `static` в новом файле
- `save.cpp` — двойной `SAVE_handle = LOAD_open()` в `LOAD_ingame` перенесён 1:1 (баг оригинала)
- `save.cpp` — unreachable код после `return(1)` в `SAVE_person` (wandering civ/driver ветки) перенесён 1:1 с комментарием

---

## Итерация 51 — ui/hud/overlay + ui/menus/cnet (2026-03-21)

- `pq.cpp` пропущен — это include-only файл (шаблон для A*), `#include "pq.cpp"` прямо в `mav.cpp`; будет перенесён вместе с mav
- `timer_prev` в оригинале был `static` file-scope → вынесен в `overlay_globals` по правилу (не-static там)
- Фикс check B при ревью: `help_text`, `help_xlat`, `panel_enemy`, `panel_gun_sight`, `track_count`, `timer_prev` изначально были оставлены в `overlay.cpp` — перемещены в `overlay_globals`
- `FONT_draw` в cnet.cpp: декларирован в `engine/graphics/pipeline/aeng.h` (тип `SLONG FONT_draw(SLONG x, SLONG y, CBYTE* text, ...)`)

---

## Итерация 52 — effects/pyro + actors/items/guns (2026-03-21)

- `MAX_COL_WITH` переименован в `PYRO_MAX_COL_WITH` — конфликт: file-scope macro столкнулся с локальным массивом того же имени в `PYRO_blast_radius`; conflict-rename по правилу
- `PYRO_blast_radius` использует local `#define MAX_COL_WITH 16` для stack array внутри функции — не file-scope, не переносится в `_globals`
- `normalise_val256` — static file-private; независимая копия также существует в `Vehicle.cpp` (не мигрирована), обе 1:1 с оригиналом
- `free_pyro` — в оригинальном `pyro.h` закомментирована; в новом `pyro.h` тоже не объявлена (implementation detail); вызывается только изнутри `pyro.cpp`
- `PYRO_init_state` — объявлена в оригинальном `pyro.h`, тело никогда не реализовано; перенесена как dead ABI declaration
- `col_with[]` — shared global (используется в `Person.cpp` через `extern`); в `_globals`
- State function arrays + `PYRO_functions` — обнаружены check B при ревью; перемещены из `pyro.cpp` в `pyro_globals`
- `guns.cpp`: dead code `calc_target_score` + `find_target` (old gun targeting в `/* */` в оригинале) — не перенесён

---

## Итерация 53 — actors/items/projectile + engine/audio/sound + ui/attract (2026-03-21)

- `wtype` (`static WorldType wtype`) — в оригинале file-private в `sound.cpp`; вынесен в `sound_globals` по правилу; `WorldType` enum тоже в `sound_globals.h`
- `world_type` init в `sound_globals.cpp` = литерал `1` (не `WORLD_TYPE_CITY_POP`) — circular include: `sound_globals.cpp` не может включать `sound.h`
- `process_weather` вызывает `PAP_calc_map_height_at` → `sound.cpp` включает `world/map/pap.h`; отмечено `// Temporary:` (DAG нарушение `engine/audio/` → `world/map/`)
- `SOUND_SewerPrecalc` / `SewerSoundProcess` — тела закомментированы в оригинале; перенесены как пустые стабы
- `demo_text` — в оригинале `static CBYTE demo_text[]` в `Attract.cpp`; вынесен в `attract_globals` по правилу
- `attract.cpp` include fix: оригинал использовал относительный путь `"..\Headers\ddlib.h"` — не существует в иерархии `src/old/`; заменён на `"fallen/DDLibrary/Headers/DDlib.h"`
- `playbacks[]` в attract_globals.cpp — строки с `\\` (Windows path сепаратор); оставлены 1:1, это data assets

---

## Итерация 54 — actors/core/interact + assets/anim_globals (2026-03-21)

- `AnimPrimBbox` typedef и `MAX_GAME_CHUNKS`/`MAX_ANIM_CHUNKS` были только в оригинальном `interact.h` — перенесены в `assets/anim_globals.h`
- `DFacet` хранит XZ координаты как байты: `p_facet->x[0] << 8` (не `->X[0]`) — критично для `check_grab_cable_facet` и `get_cable_along`
- `check_grab_cable_facet` использует `nearest_point_on_line_and_dist_and_along` (с `along`), не укороченную версию без `along`
- `check_grab_ladder_facet` вызывает `correct_pos_for_ladder()` для расчёта угла хвата
- `calc_angle`, `angle_diff`, `valid_grab_angle`, `get_cable_along` — file-private хелперы, `static`
- `find_grab_face_in_sewers` использует локальные `best_*` переменные вместо глобальных — в оригинале тоже локальные; globals используются только в `find_grab_face`

---

## Итерация 55 — world/navigation/wmove + world/map/qedit + ui/cutscenes/playcuts (2026-03-21)

- `wmove.cpp` включает `fallen/Headers/Vehicle.h` (Temporary) — `get_vehicle_body_offset/prim` не перенесены; `fallen/Headers/memory.h + prim.h` — `prim_points/prim_faces4/prim_objects/next_prim_*` и типы PrimPoint/PrimFace4/PrimObject/PrimInfo
- `WMOVE_get_pos` и `WMOVE_get_num_faces` — file-private static (не объявлены в оригинальном wmove.h, нигде не вызываются снаружи)
- `wmove.cpp` включает `statedef.h` для `STATE_DEAD` (по прецеденту из pyro.cpp)
- `playcuts.cpp` включает `fallen/Headers/fc.h` (Temporary) — `FC_cam` не мигрирован; `fallen/DDEngine/Headers/DrawXtra.h` (Temporary) — `DRAW2D_Box` не мигрирован; `fallen/Headers/dirt.h` (Temporary) — dirt.cpp не мигрирован
- `playcuts.h` включает `<MFStdLib.h>` для `MFFileHandle`; `playcuts_globals.h` включает `<windows.h>` для `BOOL`
- `playcuts.cpp` включает `engine/audio/sound.h` для `MUSIC_REF`
- `PLAYCUTS_Free_Chan` — no-op (комментарии с причиной); `PLAYCUTS_Free` — no-op пул
- `qedit.cpp` — все функции static (file-private); зависит от `engine/graphics/pipeline/qeng.h` (DAG: world → engine — допустимо)

---

## Итерация 56 — assets/anim (2026-03-21)

- Build fix: include order bug — `assets/anim.h` included `fallen/Headers/anim.h` before `assets/anim_globals.h`, causing `UBYTE`/`SLONG` unknown in `Prim.h` (which uses them at line 231 before its own `core/fmatrix.h` include at line 265). Fixed by swapping order in `anim.h`: globals first (brings in MFStdLib → core/types.h), then fallen/Headers/anim.h.
- `convert_anim`, `load_anim_system`, `append_anim_system`, `darci_normal_count`, `playing_combat_tutorial`, `playing_level` — forward-declared as extern (defined in io.cpp, not yet migrated)
- `setup_anim_stuff` — not declared in anim.h (private helper, called only from io.cpp which uses the redirect)

---

## Итерация 57 — ui/menus/widget (2026-03-21)

- Method tables (`BUTTON_Methods` etc.) moved to `widget_globals.cpp`; all method implementations made public (declared in `widget.h`) since they're referenced by the tables in a separate translation unit
- `interfac.h` included temporarily for `get_hardware_input`, `INPUT_TYPE_JOY`, `INPUT_MASK_*`
- `DrawXtra.h` included temporarily for `DRAW2D_Box/Tri/Sprite` (drawxtra.cpp not yet migrated)
- `InkeyToAscii`/`InkeyToAsciiShift` — extern forward declarations (defined in Controls.cpp, not yet migrated)
- `hDDLibWindow` — local `extern` inside `FORM_Process` as in the original

---

## Итерация 58 — engine/graphics/geometry/superfacet (2026-03-21)

- `FacetRows`/`FacetDiffY` declared as `extern` (defined in `facet.cpp`, not yet migrated)
- `facet_rand`/`set_facet_seed`/`texture_quad` also extern from `facet.cpp`
- `SUPERFACET_build_call`: `foundation` uninitialized when `df->FHeight == 0` — pre-existing bug in original (no else clause at line 615 of original); preserved 1:1
- `SUPERFACET_create_points`: `foundation` has proper `else { foundation = 0; }` — confirmed from git history

---

## Итерация 59 — engine/graphics/geometry/shape (2026-03-21)

- `SHAPE_colour_mult` и `SHAPE_tripwire_uvs` и `SHADOW_cylindrical_shadow` — file-private static (нигде не объявлены в оригинальном `shape.h`, не используются снаружи)
- `Game.h` включается первым — тот же паттерн что итерация 25 и 36: `MFStdLib.h` объявляет `extern DisplayWidth/Height`, `gd_display.h` переопределяет их как `#define`; обратный порядок — синтаксическая ошибка
- `SHAPE_balloon_colour` — non-static global (используется в `mesh.cpp` через `extern`); вынесен в `shape_globals`
- `balloon_colours[4]` — static local внутри `SHAPE_draw_balloon`, не file-scope; остаётся в `.cpp`
- `Night.h` находится в `fallen/Headers/`, не в `DDEngine/Headers/` (как в оригинале)

---

## Итерация 60 — world/map/ob + world/map/supermap (2026-03-21)

- `envmap_specials` — не static: вызывается из `elev.cpp` и `Game.cpp`; добавлена декларация в `ob.h`
- `make_all_clumps` — dev tool (итерируется по всем уровням, создаёт texture clumps, exit(0)); не удалялась — вызывается из `game_startup`; зависит от `make_texture_clumps` (defined in Game.cpp, not yet migrated) через forward decl
- `next_dbuilding`/`next_dfacet`/`next_dstyle` и др. (9 переменных) — объявлены `extern` в `fallen/Headers/supermap.h`, были определены в `old/supermap.cpp`; перенесены в `supermap_globals.cpp`
- `fallen/Headers/supermap.h` — **не редиректится**: содержит определения структур (DFacet, DBuilding, DWalkable, DStorey) которые нужны и в `old/` коде; `new/world/map/supermap.h` включает его
- `create_super_dbuilding` — editor function, нигде не вызывается; не мигрировалась (мёртвый код)
- `levels[]` — изначально static в supermap.cpp, нарушение правила globals; исправлено — перенесено в `supermap_globals.cpp`; struct `Levels` + extern — в `supermap_globals.h`
- `supermap_globals.cpp` включает `fallen/Headers/Game.h` (Temporary) для PERSON_* констант в `levels[]`

---

## Итерация 61 — ui/camera/fc (2026-03-21)

- `fc.h` включает `fallen/Headers/Game.h` (Temporary) — `FC_Cam.focus` = `Thing*`, требует полного типа; `FC_process` использует CLASS_*, FLAG_PERSON_*, STATE_*, ACTION_*, GF_*
- `fc.cpp` дополнительно включает `fallen/Headers/animate.h` — ACTION_* и SUB_OBJECT_* не транзитивно доступны через `Game.h` (не включён в цепочку `Game.h → Structs.h → anim.h`)
- `EWAY_grab_camera` уже объявлена в `eway.h` с типом `SLONG`; extern-декларация с `void` убрана — конфликт типа
- `UBYTE cap` в `FC_alter_for_pos` — dead variable из оригинала, перенесена 1:1
- `font2d.h` не включён — в оригинале был, но использовался только в закомментированном блоке `/* */`
- Новая директория `ui/camera/` создана

---

## Итерация 62 — ui/hud/eng_map (2026-03-21)

- `MAP_pulse_init` и `MAP_beacon_init` — в оригинале не были static; изначально сделаны static, но `elev.cpp` вызывает их через extern → исправлено на non-static; добавлены в `eng_map.h`
- `DisplayWidth`/`DisplayHeight` — нужен `gd_display.h` после `game.h` чтобы заменить `extern SLONG` на `#define 640/480`; без него линкер не мог найти символы (та же схема что sky.cpp итерации 36)
- `eng_map_globals.h` включает `fallen/Headers/memory.h` — требует `<string.h>` (→ anim.h → strcpy) и `<MFStdLib.h>` (→ supermap.h → MFFileHandle) перед ним
- `MAP_init` — объявлена в оригинальном `map.h`, но нигде не реализована и не вызывается → мёртвая декларация, не перенесена
- `FC_cam` — объявлен в `fc_globals.h`, не в `fc.h`; добавлен include `fc_globals.h`

---

## Итерация 63 — world/map/qmap (2026-03-21)

- `QMAP_compress_all`, `QMAP_make_room_at_the_end_of_the_all_array`, `QMAP_compress_prim_array`, `QMAP_get_style_texture`, `QMAP_create_cube` — file-private в оригинале (нет декларации в `qmap.h`, нигде не вызываются снаружи) → помечены `static`
- `qmap.cpp` не включает `game.h` — `MSG_add` идёт через `engine/graphics/pipeline/message.h`, `MemAlloc`/`MemFree` через `core/memory.h`; macros `ASSERT`/`WITHIN`/`SATURATE`/`SWAP`/`MAX`/`MIN` — через `<MFStdLib.h>` (→ Always.h)

---
