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

**Что делать дальше:**

1. Регенерировать dead-list (батчи изменили состояние):
   ```bash
   CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON -DENABLE_ASAN=OFF" make configure
   make build-release > /tmp/dead_code_build.log 2>&1
   grep '^lld-link: Discarded' /tmp/dead_code_build.log | sed 's/^lld-link: Discarded //' > /tmp/discarded.txt
   ```

2. Следующие приоритеты (по количеству dead символов):
   - **`src/buildings/building.cpp` (134 dead)** — огромный transitive кластер, нужна осторожность
   - **`src/buildings/building_globals.cpp` (58 dead)** — связан с building.cpp
   - **`src/engine/graphics/pipeline/aeng.cpp` (41 dead)** — pipeline legacy
   - **`src/things/characters/person.cpp` (36 dead)**
   - **`src/ui/menus/widget.cpp` (34 dead)**
   - Быстрые: файлы с 1-4 dead, у которых все символы leaf (верифицировать grep'ом)

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
