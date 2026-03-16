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

### Конвертация исходников в UTF-8 (2026-03-16)

Все исходники `new_game/` были в Windows-1252 (игровые файлы) и CP850 (редакторские файлы из `Editor/`).
Конвертированы в UTF-8 для совместимости с современным тулингом.

**Затронуто (коммит `a4076fb` + fix `3404bed`):**
- `DDEngine/Source/Font.cpp`, `font2d.cpp`, `menufont.cpp`
- `Source/widget.cpp`
- `outro/credits.cpp`, `outroFont.cpp`
- `Editor/Source/Edit.cpp`, `FileReq.cpp`
- `Fallen.sln`

**Нюанс:** строковые литералы с non-ASCII байтами потребовали замены на escape-последовательности, чтобы бинарное значение не изменилось после смены кодировки файла:
- `font2d.cpp`: `"\xa3..."` → `"\"" "\xa3" "..."` (байт 0xA3 = знак фунта `£` в Windows-1252, используется как индекс глифа в рендерере шрифта)
- аналогичные правки в `Edit.cpp`, `credits.cpp`, `outroFont.cpp`

**Важно для портирования:** игровой рендерер шрифта использует байт строки напрямую как индекс в таблицу глифов — кодировка Windows-1252. `.txt` файлы с текстом игры (`lang_*.txt`) **не конвертировались** — они читаются как сырые байты и должны оставаться в Windows-1252. Подробнее → `new_game_planning/porting_notes.md` (раздел "Текстовая кодировка игры").

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

---

## Итерация 7 — Удаление #if 0 блоков (пачка 4 — остатки DDEngine + DDLibrary + Source)

**Дата:** 2026-03-16

**Удалено блоков #if 0 (30 блоков, Python apply_removes):**

- `DDEngine/Source/superfacet.cpp` — 9 блоков:
  - `#if0/#else/#endif` полосовые vs списковые индексы (687-714) → оставлен list-вариант
  - `#if0/#else/#endif` max_indices strips vs lists (1227-1233) → оставлен `* 6 / 4`
  - dead SFACET_init branch (1290-1331)
  - 2× dead old paths (1401-1408, 1422-1431)
  - `#if0/#else/#endif` старый D3D SetRenderState vs новый (1494-1505) → оставлен `the_display.SetRenderState`
  - 2× dead paths (1687-1699, 1731-1747)
  - `#if0/#else/#endif` старый MM setup vs `lpvVertices` (1955-1969) → оставлен новый
- `DDEngine/Source/texture.cpp` — 7 блоков: все simple (1277, 1395, 1452, 1481, 1571, 1623-1672, 1813-1826)
- `DDLibrary/Headers/FFManager.h` — 1 блок (10-31)
- `DDLibrary/Source/FFManager.cpp` — 1 блок (8-84)
- `DDLibrary/Source/GHost.cpp` — 1 блок (395-414)
- `DDLibrary/Source/GKeyboard.cpp` — 1 блок `#if0/#else/#endif` (126-140) → оставлен key_turn/Keys путь
- `Headers/inside2.h` — 1 блок (79-85)
- `Headers/wmove.h` — 1 блок (57-59)
- `Source/Attract.cpp` — 3 блока: мёртвый debug (531-556); `#if0/#else/#endif` PSX vs DC mimes (683-763) → оставлен DC вариант; большой мёртвый блок (828-1033)
- `Source/Combat.cpp` — 1 блок (1565-1646)
- `Source/Controls.cpp` — 2 блока: большой блок (1436-1650, содержал вложенный 1553-1610); маленький (1961-1976)
- `Source/Cop.cpp` — 1 блок (140-251)

**Итог:** 30 блоков, проект: 92 → 58 `#if 0` (не считая vcpkg_installed).

**Нюансы:**
- Controls.cpp 1553-1610 был вложен внутри 1436-1650 → удалился автоматически.
- Attract.cpp содержал нестандартные символы (не UTF-8, не cp1251) → читался/записывался в binary mode чтобы сохранить байты.
- Первая попытка записи через `encoding='latin-1'` текст-режим повредила файл (частичная запись из-за ошибки кодировки в первом скрипте) → восстановлен через `git checkout`, затем обработан в binary mode.


---

## Iteration 8 - #if 0 blocks (batch 5 - Source files)

**Date:** 2026-03-16

**Removed 27 blocks manually (Edit tool + sed):**

- `Source/Thing.cpp` - 1: TRACE("index %d thing %x gt %d") inside InCar check (958-960)
- `Source/pause.cpp` - 1: `static Font3D font(...)` (34-36)
- `Source/overlay.cpp` - 1: MFX_QUICK_still_playing() debug (912-917)
- `Source/Main.cpp` - 1: Random() test loop (90-99)
- `Source/memory.cpp` - 1: WMOVE_remove(CLASS_VEHICLE) call (1942-1945)
- `Source/dirt.cpp` - 1: old PAP if-chain alt version (559-567)
- `Source/Vehicle.cpp` - 5:
  - #if0/#else/#endif ANNOYINGSCRIBBLECHECK+ScribbleCheck -> kept empty #define (82-98)
  - #if0/#else/#endif old analog steering formula -> kept new version (3323-3341)
  - debug TRACE for skid cos^2 (3705-3727)
  - #if0 else-clause "ultra cheap suspension" (4233-4258)
  - #if0/#else/#endif iterative fast_root -> kept sqrt((double)num) (4334-4369)
- `Source/Person.cpp` - 2:
  - OnFace roof assertions (3684-3696)
  - hanging distance debug with PANEL_new_text (13355-13395)
- `Source/Darci.cpp` - 1: "kick off wall" special moves disabled code (672-699)
- `Source/guns.cpp` - 1: police car targeting logic for Darci/Roper (273-299)
- `Source/elev.cpp` - 1: #if0/#else/#endif SND_BeginAmbient vs ELEV_game_init_common -> kept ELEV_game_init_common (2370-2379)
- `Source/pap.cpp` - 1: old InsideIndex/sewers height branch (168-196)
- `Source/supermap.cpp` - 1: PSX TGA texture copy inside #ifdef EDITOR (3303-3313)
- `Source/startscr.cpp` - 1: draw_a_3d_menu(Font3D&) function (238-286)
- `Source/wmove.cpp` - 1: WMOVE_remove() function (507-544)
- `Source/Env.cpp` - 1: entire ENV_load + ENV_heap/var declarations - whole file content was in #if 0 (9-271)
- `Source/Thug.cpp` - 1: fn_thug_normal body - old waypoint AI (68-152)
- `Source/cloth.cpp` - 1: entire CLOTH_process function body (485-673)
- `Source/inside2.cpp` - 1: entire INSIDE NAVIGATION block with INSIDE2_mav_nav_calc (793-1195)
- `Source/interact.cpp` - 1: trench edge-grab code (856-954)
- `Source/ware.cpp` - 1: warehouse door prim placement (595-697)
- `Source/Game.cpp` - 1: edge_map_warning() function with widgets (1006-1106)

**Result:** 27 blocks, project: 49 -> 22 `#if 0`.

**Remaining (22):** eway.cpp (7), frontend.cpp (5), interfac.cpp (7), pcom.cpp (3).

**Notes:**
- Env.cpp: whole file (except includes) was in one #if 0 block. After deletion: 8 lines of headers only.
- cloth.cpp CLOTH_process: function declared, body in #if 0. After deletion: empty function body remains.
- Thug.cpp fn_thug_normal: whole body in #if 0. After deletion: empty function body remains.
- inside2.cpp: 403 lines of indoor navigation AI - only #if0 block in the file.
- Darci.cpp:672 - NOT police cars (I misread parallel tool results initially). Actually: "kick off wall" jump/kick special moves.


---

## Итерация 9 — Удаление #if 0 блоков (пачка 6 — финальная, Source + outro)

**Дата:** 2026-03-16

**Удалено 30 блоков #if 0:**

- Source/eway.cpp — 7 блоков: ANNOYINGSCRIBBLECHECK#if0/#else (оставлен пустой), PSX COND_TRUE конвертация, cam accel TICK_RATIO x2 (#if0/#else), debug PANEL_new_text с вейпойнт-номером x3 (#if0/#else)
- Source/frontend.cpp — 5 блоков: STARTS_* vs #include startscr.h (#if0/#else), best_score -INFINITY vs 1000 (#if0/#else), order > vs < best_score (#if0/#else), dead menu_state.base calc, dead item->Data=31
- Source/interfac.cpp — 7 блоков: dead backflip-on-lifts, dead anim_prim_switches, SUB_STATE_SIDLE ASSERT vs handling (#if0/#else), STATE_SEARCH ASSERT vs handling (#if0/#else), DC analog vs PC counter (#if0/#else), simple vs damped steering (#if0/#else), dead PSX_SetShock
- Source/pcom.cpp — 3 блока: dead AENG_world_line debug, BLOCKED macro debug vs пустой (#if0/#else), dead PCOM_RUNOVER_TURN_LEFT/RIGHT
- outro/Tga.h — 1: объявление TGA_save
- outro/outroTga.cpp — 1: реализация TGA_save + TGA_header массив
- outro/outroMatrix.cpp — 1: весь файл кроме MATRIX_scale (MATRIX_calc, MATRIX_calc_arb, MATRIX_vector, MATRIX_skew, MATRIX_3x3mul, MATRIX_find_angles, MATRIX_construct и др.)
- outro/outroMain.cpp — 1: debug FONT_draw Fin/? текст
- outro/os.cpp — 4: тело OS_calculate_mask_and_shift, OS_message_handler callback, OS_Mode typedef + OS_mode_init + OS_mydemo_setup_mode_combo, WinMain для outro

**Итог:** 30 блоков, проект: 22 → **0** #if 0.

**Нюансы:**
- os.cpp (CRLF) — Edit tool не смог сопоставить строки → удаление через Python rb по номерам строк.
- outroMatrix.cpp и outroTga.cpp: частичная замена нарушила файлы → восстановлены через Write/двойной Edit.

- MFStdLib/Headers/MFStdLib.h — 1: объявления GetInputDevice/ReadInputDevice (старый DirectInput API) — удалены. (Обнаружен после основной итерации)

---

## Итерация 10 — Удаление cam.cpp и Command.cpp

**Дата:** 2026-03-16

**Удалено:**
- `Source/cam.cpp` — полностью под `#ifdef DOG_POO` (никогда не компилировался). Не в vcxproj.
- `Source/Command.cpp` — legacy AI-командная система, заменена PCOM в финале. Не в vcxproj.

**Нюансы:**
- `cam.h` включается в Attract.cpp, Controls.cpp, elev.cpp, eway.cpp, Game.cpp, interfac.cpp, Person.cpp — но все CAM_ функции в активном коде находятся внутри `#ifdef OLD_CAM` или `/* */` комментариев. Никаких линкерных проблем нет и не было.
- `Command.h` включается только из нескомпилированных файлов (Level.cpp) или закомментировано (Cop.cpp, Furn.cpp, Person.cpp, Thug.cpp). Активный код не использует.
- `Source/Level.cpp` — также не в vcxproj, Level.h не включается ни одним скомпилированным файлом. Помечен для удаления в конце Этапа 2 (пункт 2: файлы без включателей + не в vcxproj).
- Vcxproj не менялся — ни один из файлов там не числился.


---

## Итерация 11 — Удаление #ifdef EDITOR блоков (coan)

**Дата:** 2026-03-16

**Команда:**
```
tools/coan/coan.exe source -UEDITOR --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine DDLibrary Headers Loader
```
Exit code 19 — норма.

**Изменено 22 файла, ~7086 строк удалено. Ключевые нюансы:**
- aeng.cpp: -1089 строк. Большой #ifdef EDITOR блок (1085 стр) editor-only функции AENG_draw_ns, AENG_draw_water; малый #if/#else: оставлен UBYTE fade = 0xff.
- startscr.cpp: -1822 стр. Файл был #ifndef EDITOR (runtime) / #else (editor 1822 стр). coan правильно оставил runtime.
- supermap.cpp: -2332 стр (13 EDITOR блоков).
- Building.cpp: -605 стр (18 блоков).
- Controls.cpp: -2 стр: #ifndef EDITOR вокруг ASCII-таблиц (правильно убрать).
- GEdit.h по-прежнему включается в runtime-файлах как extern объявления — не используется в runtime после удаления EDITOR блоков. Уберётся при удалении GEdit/ папки.
- Setup-файлы в Source/ (дубликаты GEdit/Source/) будут удалены вместе с GEdit/ папкой.
- Vcxproj не менялся.
