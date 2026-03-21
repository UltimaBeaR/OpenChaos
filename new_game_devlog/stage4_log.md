# Лог Этапа 4 — Реструктуризация кодовой базы

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
