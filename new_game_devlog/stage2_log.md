# Лог Этапа 2 — Удаление мёртвого кода

Этап начат: 2026-03-15

---

## Порядок итераций — принятые решения

**Editor/, GEdit/, Ledit/, sedit/ — удалять В КОНЦЕ Этапа 2**, после удаления всего остального.
Причина: `GEdit.h` включается в 43+ Source файлов без защиты `#ifdef EDITOR`, плюс аналогичные
include'ы из Editor/, sedit/ в Game.cpp, Building.cpp, Enemy.cpp, Structs.h и др.
Требуется отдельный анализ что из этих заголовков реально используется в runtime — это проще
делать когда весь остальной мёртвый код уже удалён.

---

## Попутные исправления (не удаление мёртвого кода)

---

### Исправление Release конфига — `/MTd` и `/NODEFAULTLIB` костыли (2026-03-15)

Обнаружено при проверке: в Release-сборке показывались отладочные надписи на экране.

**Причина:** В `Fallen.vcxproj` Release конфиг использовал `MultiThreadedDebug` (`/MTd`) вместо
`MultiThreaded` (`/MT`). Это автоматически определяет `_DEBUG`, из-за чего все `#ifdef _DEBUG`
блоки активны в Release. В т.ч. `"music mode %d vol %f %s"` в `panel.cpp:5441`.

**Сопутствующий мусор:** 103 строки `<AdditionalOptions> /D /NODEFAULTLIB:libcmtd.lib" "</AdditionalOptions>`
в Release — костыли, добавленные чтобы подавить конфликт линковки из-за неправильного CRT.

**Правки в `Fallen.vcxproj`:**
- `RuntimeLibrary`: `MultiThreadedDebug` → `MultiThreaded` в Release `ItemDefinitionGroup` (строка 63)
- Удалены 103 `<AdditionalOptions>` с `NODEFAULTLIB` из Release секций (глобальная + per-file)

**Проверено:** Release собирается, `music mode` надпись исчезла.

**Не тронуто:** Debug конфиг (`MultiThreadedDebug` там корректен), Debug `NODEFAULTLIB` строки.

---

### Исправление debug-надписи FARFACET в panel.cpp (2026-03-15)

В `DDEngine/Source/panel.cpp:5408` блок с надписями `FARFACET squares drawn` и `music mode`
был под `#ifndef TARGET_DC` без `#ifdef _DEBUG` → показывался в любой сборке включая Release.

**Правка:** `#ifndef TARGET_DC` → `#if !defined(TARGET_DC) && defined(_DEBUG)`.
Внутренний `#ifdef _DEBUG` вокруг `music mode` удалён как ставший избыточным.

**Проверено:** обе надписи исчезли в Release, билд успешен.

---

## Итерации

---

## Итерация 1 — Удаление MuckyBasic/ и thrust/

**Дата:** 2026-03-15

**Удалено:**
- `new_game/MuckyBasic/` — целиком (standalone папка, не в vcxproj)
- `new_game/thrust/` — целиком (standalone папка, не в vcxproj)
- `#ifndef TARGET_DC / #include "../../MuckyBasic/clip.h" / #endif` из `DDEngine/Source/poly.cpp`
- То же включение из `DDEngine/Source/polyrenderstate.cpp`

**Нюансы:**
- `clip.h` из MuckyBasic включался в `poly.cpp` и `polyrenderstate.cpp` через `#ifndef TARGET_DC` (т.е. активно на PC).
  Но функция `CLIP_do` нигде не вызывается — включение было мёртвым.
- В `Fallen.vcxproj` ни MuckyBasic, ни thrust не упоминаются — в сборку не входили.
- Совпадения слова "MuckyBasic" в `eway.h` и `eway.cpp` — только комментарии `// claude-ai:`, не includes.

---

## Итерация 2 — Удаление Glide Engine

**Дата:** 2026-03-15

**Удалено:**
- `new_game/fallen/Glide Engine/` — целиком (glaeng.cpp, glbuild.cpp, glfigure.cpp, glmesh.cpp, glpoly.cpp, gltexture.cpp)
- `new_game/fallen/Glide Library/` — целиком (glidelib.cpp, glidelib.h)
- `Engine.h`: блок `#elif defined(VERSION_GLIDE)` с пустым `typedef struct {} Camera;`
- `Game.h`: закомментированный блок `/* VERSION_GLIDE ... */` с «PC - Glide specific defines»

**Нюансы:**
- Обе папки отсутствовали в `Fallen.vcxproj` — в сборку не входили.
- Ни один файл из `Source/` не делал `#include` на glaeng.h/glidelib.h.
- `Map.cpp` содержал только комментарий `// claude-ai:` про Glide — не трогался.
- `VERSION_GLIDE` в `Game.h` уже был закомментирован в `/* */` блоке — был `#if 0`-аналог.
- В `Engine.h` цепочка `#ifdef VERSION_D3D / #elif VERSION_GLIDE / #elif VERSION_PSX` — удалён только Glide-блок; PSX остаётся до своей итерации.
- `light.h`: `LIGHT_get_glide_colour()` — inline-функция внутри `#if LIGHT_COLOURED` блока, вызывалась только из Glide Engine файлов → удалена.

---

## Итерация 3 — Удаление PSX/DC кода

**Дата:** 2026-03-15

**Удалено через coan** (214 файлов, ~22700 строк):
```
tools/coan/coan.exe source -UPSX -UTARGET_DC -UBUILD_PSX --no-transients --filter cpp,c,h,hpp --recurse new_game/fallen/Source new_game/fallen/DDEngine new_game/fallen/DDLibrary new_game/fallen/Headers new_game/fallen/Loader
```
Exit code 19 — норма (предупреждение `--no-transients`).

**Удалено вручную (файлы/папки):**
```
rm -rf new_game/fallen/PSXENG new_game/fallen/psxlib new_game/fallen/psxlib1
rm new_game/fallen/Source/nightpsx.cpp new_game/fallen/Source/Levelpsx.cpp new_game/fallen/Source/dc_credits.cpp new_game/fallen/Source/hm_psx.cpp new_game/fallen/Source/io_psx.cpp new_game/fallen/Source/pausepsx.cpp
```

**Изменено вручную (правки кода — требуют проверки):**
- `Source/qls.cpp`: удалён orphan `#endif` в конце файла. Файл был обёрнут в `#ifdef` который убрали, а `#endif` остался — баг из оригинала (там тоже есть). Без этой правки coan падал с ошибкой "Orphan #endif".
- `Fallen.vcxproj`: удалена строка `<ClCompile Include="Source\nightpsx.cpp">` с одной строкой `AdditionalOptions` внутри.

**Нюансы:**
- `BUILD_PSX` определялся только внутри `#ifdef PSX` в `Game.h` → теперь безусловно не определён. `#ifndef BUILD_PSX` блоки в `memory.cpp`, `Person.cpp` стали активными (верно для PC).
- В `aeng.cpp` остались 3 `#ifdef TARGET_DC` блока (строки 3633, 3776, 3818). Они внутри `#if 0` блока с незакрытым `/* */` комментарием внутри — coan не смог обработать из-за вложенного комментария. Это дважды мёртвый код. Разберём в итерации по `#if 0`.
- Символы `PSX_NOT_REALLY`, `PSX_STERN_REVENGE_BUG_AND_CRAP_DRIVERS`, `PSX_COMPRESS_LIGHT` — другие символы, не `PSX`, coan их не трогал. Это норма.

---

## Итерация 4 — Удаление #if 0 блоков (пачка 1 из ~6)

**Дата:** 2026-03-15

**Удалено файлов целиком (+ из vcxproj):**
- `DDEngine/Source/DCback.cpp`, `DCcredits.cpp`, `DCfont.cpp`, `DCos.cpp` — DC-платформенный рендер, весь файл обёрнут в `#if 0`
- `DDEngine/Headers/DCback.h`, `DCcredits.h`, `DCfont.h`, `DCos.h` — соответствующие заголовки
- `DDEngine/Source/font3d.cpp` — 3D шрифт, весь файл обёрнут в `#if 0`
- `DDEngine/Headers/font3d.h` — заголовок: весь класс `Font3D` внутри `#if 0`, оставлен пустой файл с комментарием `// THIS IS PANTS!`
  - Не удалён полностью: включается в briefing.cpp, pause.cpp, startscr.cpp. Но их использования Font3D — в `/* */` или `#if 0`, активного кода нет.

**Удалено блоков #if 0 в коде:**
- `DDEngine/Headers/panel.h` — 2 неиспользуемые функции (`PANEL_draw_angelic_status`, `PANEL_draw_press_button`)
- `DDEngine/Source/aeng.cpp` — 10 блоков:
  - `ANNOYINGSCRIBBLECHECK` (`#if 0 / #else`) — оставлен `#define ANNOYINGSCRIBBLECHECK` из else-ветки
  - `AENG_set_draw_distance` — тело функции (функция осталась пустой заглушкой)
  - Создание movie (`Create the movie`) — большой блок ~100 строк
  - `gamut_lo` clamp блок — альтернативный алгоритм
  - `gamut_lo` edge cases — дополнительные проверки диапазонов
  - Doubly-dead блок `#if 0` + `/* */` с `DIRT_INFO_TYPE_*` (тот самый из итерации 3 с TARGET_DC внутри)
  - Bike wheel texture rotation (`#if 0 / #else`) — оставлен актуальный алгоритм с `SIN/COS`
  - Пустой `#if 0 / #endif`
  - Crinkle rendering (не реализован, "code rotation first")
  - `ANIMAL_draw` (x2) + `CurDrawDistance` setter
- `DDEngine/Source/console.cpp` — TCP/IP консоль (~215 строк, под `#if 0 / #ifndef FINAL`)
- `DDEngine/Source/drawxtra.cpp` — 4 блока:
  - Pyro steps (`#if 0 / #else`) — оставлен Throttle вариант
  - Wave particle angle (`#if 0 / #else`) — оставлен `iAngle` вариант
  - Armageddon ring calc (`#if 0 / #else`) — оставлен упрощённый вариант ("What the fuck? Bonkers, mate.")
  - `ANIMAL_draw` функция целиком
- `DDEngine/Source/facet.cpp` — 2 блока: clip check и flashing pink debug
- `DDEngine/Source/farfacet.cpp` — 3 блока: DC renderstate setters, draw-every-farfacet override, draw distance check (`#if 0 / #else`)
- `DDEngine/Source/figure.cpp` — 2 блока: MM lighting table vars, negative light branch

**Итог:** 3342 удалений, 3 вставки (контент из #else веток), 18 файлов.

**Нюансы:**
- coan для `#if 0` не подходит — делаем руками.
- `#if 0 / #else / #endif` блоки: сохранялось содержимое `#else` ветки, удалялись директивы и мёртвая ветка.
- Оставшиеся `#if 0` блоки (~140 штук) — следующие пачки.

---

## Итерация 5 — Удаление #if 0 блоков (пачка 2 — figure.cpp)

**Дата:** 2026-03-15

**Удалено блоков #if 0 в коде:**
- `DDEngine/Source/figure.cpp` — 33 блока (все оставшиеся в файле):
  - `/* */` блок 732-830 (содержал внутри `#if 0/#else/#endif` с D3D lighting — дважды мёртвый)
  - lod_mul calc (перенесён в callers)
  - FIGURE_find_and_clean_prim_queue_item call
  - Bounding sphere `#if 0/#else/#endif` → оставлен correct fix с `pfBoundingSphereRadius`
  - 2× Strip indices (4-triangle и 5-triangle варианты) — оба заменены на List indices
  - Normal blend `#if 0/#elif 0/#elif 0/#else/#endif` → оставлен `(4 * vNormToEdge) - vAvNorm`
  - Старый memcpy вертексов/индексов в permanent буферы
  - MM data setup (ALIGNED_STATIC_ARRAY) — 2 места
  - 3× letterbox viewport `#if 0/#else/#endif` → оставлен hack с `g_dw3DStuffHeight/g_dw3DStuffY` (все 3 заменены сразу через replace_all)
  - 2× DrawIndexedPrimitive vs DrawIndPrimMM `#if 0/#else/#endif` → оставлен DrawIndPrimMM (replace_all)
  - "Can we have 4x polys for free?" — 3× DrawIndexedPrimitive
  - 2× цвет грани (quad и tri) — мёртвые пути через ENGINE_palette
  - TLVertex vs LVertex quad `#if 0/#else/#endif` → оставлен LVertex
  - TLVertex vs LVertex tri `#if 0/#else/#endif` → оставлен LVertex
  - 2× MM cleanup (null ptrs) → пустые if-блоки
  - FIGURE_TPO_init_3d_object с/без slot → оставлен без slot
  - FIGURE_TPO_finish_3d_object без/с slot 1 → оставлен с slot 1
  - Near-plane culling (не реализован)
  - Alpha/clipped path todo
  - 2× muzzle flash draw_prim_tween → оставлен person_only вариант
  - 2× weapon draw_prim_tween → оставлен person_only вариант
  - 2× body part draw_prim_tween → оставлен person_only вариант
  - Recurse child setup (старый стиль) → оставлен новый с pDHPR1Inc
  - ALIGNED_STATIC_ARRAY setup (второй экземпляр)
  - MM NULL cleanup (второй экземпляр)
  - imatrix assignment (старый integer-matrix путь)

**Итог:** ~33 блока, ~192 строки удалено, ~0 вставок (все #else контент уже был в файле как active).
После удаления в figure.cpp: 0 блоков `^#if 0`, в проекте осталось 102.

**Нюансы:**
- Блок 3538-3573 — Python script первоначально определял неверно (3538-3557) из-за `#ifdef` внутри `#else`. Исправлено вручную.
- replace_all использован для трёх идентичных letterbox блоков и двух DrawIndPrimMM блоков.
- Финальные 11 блоков (с табами) обработаны Python one-liner из-за несовместимости с Edit tool.

---

## Итерация 6 — Удаление #if 0 блоков (пачка 3 — DDEngine файлы)

**Дата:** 2026-03-16

**Удалено блоков #if 0 в коде (27 блоков, все через Python remove_lines/keep_else):**

- `DDEngine/Source/font2d.cpp` — 1 блок: dead rgb==0 coordinate adjust
- `DDEngine/Source/mesh.cpp` — 2 блока: FASTPRIM_draw shortcut; colour-code crumple zones debug
- `DDEngine/Source/panel.cpp` — 5 блоков:
  - `PANEL_draw_angelic_status` + `PANEL_draw_press_button` функции (177-226)
  - `PANEL_draw_compass_angle` + `PANEL_draw_compass` функции (692-796)
  - `PANEL_do_heartbeat` функция (1100-1363)
  - fog table debug extern + sprintf (4211-4242, внутри `#ifdef DEBUG`)
  - пустой «FINAL BUILD DONE! HOORAY!» (4383-4387)
- `DDEngine/Source/poly.cpp` — 12 блоков (11 удалений — один блок содержал вложенный):
  - `#if 0/#else/#endif` обёртка вокруг `POLY_perspective(pt)` → оставлен полный inline transform
  - `POLY_add_poly` функция (1404-1567)
  - `POLY_add_triangle_slow` + `POLY_add_quad_slow` функции (2089-2215, содержал вложенный блок 2135)
  - KB_F8 условие перед `POLY_add_quad_fast` (2220-2226)
  - KB_F8 условие перед `POLY_add_triangle_fast` (2234-2240)
  - 4× мёртвые `POLY_setclip` вызовы (2429, 2549, 2606, 2664)
  - `POLY_add_shared_start/point/tri` функции (2856-2941)
  - `POLY_frame_draw_focused` функция (3494-3646)
- `DDEngine/Source/polypage.cpp` — 1 блок: `#if 0/#else/#endif` letterbox hack (ещё одна копия) → оставлен `g_dw3DStuffHeight`
- `DDEngine/Source/polyrenderstate.cpp` — 3 блока:
  - `#if 0/#else/#endif` frame-wait vs `RenderStates_OK` check → оставлен простой `if (RenderStates_OK) return;`
  - DC kludge `else` блок перед active code (179-188)
  - RS.Validate debug loop (1504-1512)
- `DDEngine/Source/ray.cpp` — 1 блок: весь файл (строки 9-1418) был в `#if 0 // Intel C compiler barfs`.
  После удаления: файл содержит только 8 `#include` строк. RAY_ функции нигде не вызываются.
- `DDEngine/Source/sky.cpp` — 2 блока: «man on moon» draw (546-588); «is player looking at moon» check (592-632)

**Итог:** 27 блоков, проект: 119 → 92 `#if 0`.

**Нюансы:**
- Все блоки обработаны одним Python скриптом (remove_lines + keep_else), файлы за один проход.
- poly.cpp блок 2135-2148 был внутри блока 2089-2215 → исчез автоматически при удалении внешнего.
- poly.cpp 2220-2226 и 2234-2240: мёртвый код заканчивался на `else`, связанном с активным блоком после `#endif`. Удалены только строки `#if 0` до `#endif` включительно — активный `{...}` остался.
- `#if 1/#else/#endif` внутри poly.cpp (строки 605-646) не трогали — это `#if 1`, не `#if 0`.

