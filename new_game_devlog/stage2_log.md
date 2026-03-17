# Лог Этапа 2 — Удаление мёртвого кода

Этап начат: 2026-03-15

---

## Статус флагов (пункт 2.5)

Легенда: ✅ удалён | ⬜ не обработан | 🚫 решено не удалять | ➖ не было в коде

| Флаг | Статус | Итерация / примечание |
|------|--------|-----------------------|
| `BIKE` | ✅ | итерация 16 |
| `FAST_EDDIE` | ✅ | итерация 17 |
| `USE_PASSWORD` | ➖ | исчез при удалении редакторов (итерация 11) |
| `SEWERS` | ✅ | итерация 20 |
| `TRUETYPE` | ✅ | итерация 21 |
| `OLD_CAM`/`CAM_OLD` | ✅ | итерация 19 |
| `DOG_POO` | ✅ | итерация 18 |
| `EIDOS` + региональные | ✅ | итерация 22 |
| `_MF_DOSX`, `__WATCOMC__`, `__DOS__`, `__WINDOWS_386__` | ✅ | итерация 23 |
| Glide-флаги (`DONT_IGNORE_*`, `WORRY_ABOUT_THIS_LATER`) | ➖ | удалены вместе с Glide Engine/ (итерация 2) |
| Отключённые оптимизации (`*_PLEASE_BOB` и др.) | ✅ | итерация 24 (`SUPERCRINKLES_ENABLED` — итерация 25) |
| Мёртвый геймплей (`DARCI_HITS_COPS`, `WE_WANT_WIND` и др.) | ✅ | итерация 26 |
| Старые алгоритмы PSXENG (`OLD_FACET_CLIP` и др.) | ✅ | итерация 27 |
| PSX/DC debug (`PSX_COMPRESS_LIGHT`, `DODGYPSXIFY` и др.) | ✅ | итерация 28 |
| Debug-флаги (`DEBUG_POOSHIT`, `HEAP_DEBUGGING_PLEASE_BOB` и др.) | ✅ | итерация 29 (`HIGH_REZ_PEOPLE_PLEASE_BOB` — пропущен, разобраться позже) |
| Debug визуализации (`WE_WANT_TO_DRAW_*` и др.) | ✅ | итерация 30 |
| Developer joke flags (`GONNA_FIREBOMB_YOUR_ASS` и др.) | ✅ | итерация 31 |
| `FACET_REMOVAL_TEST` | ✅ | итерация 33 (решено: Release-only кодовая база) |
| `SUPERCRINKLES_ENABLED` | ✅ | итерация 25 |
| `_DEBUG`, `DEBUG`, `NDEBUG`, `FINAL`, `_RELEASE` (все debug/release блоки) | ✅ | итерация 33 |

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

---

## Итерация 12 — Удаление папок редакторов (Editor/, GEdit/, LeEdit/, SeEdit/)

**Дата:** 2026-03-16

**Удалены папки целиком:**
- `new_game/fallen/Editor/`
- `new_game/fallen/GEdit/`
- `new_game/fallen/LeEdit/`
- `new_game/fallen/SeEdit/`

**Из Fallen.vcxproj удалено:**
- 135 строк: все `ClCompile` для `Editor\Source\*.cpp`, `GEdit\Source\*.cpp`
- Все `ClInclude` для `Editor\Headers\*`, `GEdit\Headers\*`
- 6 строк: `ClCompile`/`ClInclude` для `Ledit\Source\*.cpp`, `Ledit\Headers\*.h`
- `AdditionalIncludeDirectories`: удалены `..\fallen\ledit\headers;`, `..\fallen\sedit\headers;`, `..\fallen\gedit\headers;`

**Хедеры перенесены в Headers/ (нужны рантайму):**
- `Editor/Headers/Anim.h` → `Headers/anim.h` (нужен везде через Structs.h)
- `Editor/Headers/Prim.h` → `Headers/prim.h` (нужен shape.cpp, animal.cpp, vehicle.cpp)
- `Editor/Headers/prim_draw.h` → `Headers/prim_draw.h` (нужен interact.cpp: `rotate_obj()`)
- `Editor/Headers/map.h` → `Headers/edmap.h` (переименован; нужен collide.cpp, building.cpp: `EDIT_MAP_WIDTH/DEPTH`, `DepthStrip`)
- `Editor/Headers/outline.h` → `Headers/outline.h` (нужен building.cpp)
- `Editor/Source/outline.cpp` → `Source/outline.cpp` (нужен рантайму: `do_storeys_overlap()` → `build_roof_grid()` → `process_building()`)
- `ledit/headers/ed.h` → `Headers/ed.h` (нужен night.cpp: `ED_Light` для загрузки .lgt)
- `Editor/Headers/Thing.h` → `Headers/mapthing.h` (частичная копия: `MapThingPSX` + `MAP_THING_TYPE_*` для io.cpp, save_type==18)

Все перемещённые файлы помечены `// uc_orig_file:`, записаны в `new_game_planning/entity_mapping.md`.

**Исправления includes в рантаймовых файлах:**
- `Structs.h`: `../Editor/Headers/Anim.h` → `anim.h`
- `DDEngine/Source/shape.cpp`: `../editor/headers/prim.h` → `../headers/prim.h`
- `DDEngine/Source/facet.cpp`: include `../sedit/headers/es.h` удалён (ES_ символы не используются)
- `Source/Main.cpp`: удалены includes `sedit.h`, `ledit.h`, `GEdit.h` (нулевое использование)
- `Source/io.cpp`: `../editor/headers/thing.h` → `../headers/mapthing.h`
- `Source/Enemy.cpp`, `Roper.cpp`: удалён `../Editor/Headers/Thing.h` (нулевое использование)
- `Source/elev.cpp`, `Game.cpp`: удалены `es.h`, `../sedit/headers/es.h`, `../editor/headers/extra.h`
- `Source/supermap.cpp`: удалён `Editor.hpp` include + `extern SLONG editor_texture_set;`
- `Source/building.cpp`: удалены `Editor.hpp`, `outline.h` (→ теперь `../headers/outline.h`); добавлены `../headers/edmap.h`, `../headers/outline.h`
- `Source/interact.cpp`: удалён `../editor/headers/map.h`; `../editor/headers/prim_draw.h` → `../headers/prim_draw.h`
- `Source/collide.cpp`: `../editor/headers/map.h` → `../headers/edmap.h`
- `Source/animal.cpp`, `vehicle.cpp`: `../Editor/Headers/prim.h` → `../headers/prim.h`
- `Source/night.cpp`: `../ledit/headers/ed.h` → `../headers/ed.h`

**Переменные в building.cpp:**
- Удалено в предыдущей сессии и восстановлено: `edit_map[128][128]`, `tex_map[128][128]`, `edit_map_roof_height[128][128]` — нужны рантайму в `build_easy_roof()` → `build_roof_grid()` → `process_building()`
- `struct EditInfo edit_info` — **удалена без восстановления**: `edit_info.HideMap` всегда 0 в рантайме (editor-only bitmask, никогда не выставляется runtime-кодом). 3 мёртвые проверки удалены из `build_easy_roof()` и `append_wall_prim()`.

**Нюансы:**
- `build_easy_roof()` / `get_storey_outline()` / `do_storeys_overlap()` — изначально считались editor-only, оказались рантаймовыми: путь `process_building()` → `build_roof()` → `build_roof_grid()` → `build_easy_roof()` / `do_storeys_overlap()`.
- `outline.cpp` исторически жил в `Editor/Source/` но нужен рантайму → перемещён.
- `MapThingPSX` из `Thing.h` нужен в `io.cpp` для загрузки PSX-формата сохранений (save_type == 18) — не мёртвый код.
- `DarkCity.h` (включался в Thing.h) — пустой файл, убран при создании `mapthing.h`.

**Результат:** компиляция 0 ошибок, 293 предупреждения (те же что были до итерации).

---

## Попутные изменения (Этап 2, 2026-03-16) — система отслеживания перемещений

Введена система `uc_orig` для отслеживания связи нового кода с оригиналом:
- `// uc_orig_file: fallen/путь` — в начало файла, когда весь файл перемещён/скопирован
- `// uc_orig: OldName (fallen/файл.cpp)` — рядом с объявлением, когда перемещена отдельная сущность

Таблица соответствий: `new_game_planning/entity_mapping.md` (заведена досрочно, план был Этап 4.3).
Правило добавлено в `CLAUDE.md` и `stages.md`.

---

## Итерация 13 — Пункт 2: Удаление посторонних файлов

**Дата:** 2026-03-16

**Удалено из `new_game/fallen/`:**
- `AutoRun/` — CD-autorun приложение для розничного диска (отдельный VS-проект)
- `SoundConverter/` — standalone утилита конвертации звука
- `UnInst/` — Windows-деинсталлятор для розничного CD
- `dxinstall/` — инсталлятор DirectX для розничного CD
- `server/` — содержал только `extract.bat` (скрипт для извлечения ресурсов из сети MuckyFoot)
- `GEdit.dsp` — проектный файл MSVC6 удалённого редактора GEdit (остаток итерации 12)
- `Fallen.dsp` — проектный файл MSVC6, заменён `Fallen.vcxproj`
- `UpgradeLog.htm` — лог конвертации из MSVC6 в современный VS
- 11 `.bat` файлов: `cdsrc.bat`, `clumps.bat`, `compress.bat`, `copysrc.bat`, `doneclumps.bat`, `english.bat`, `englishsrc.bat`, `french.bat`, `frenchsrc.bat`, `german.bat`, `germansrc.bat` — скрипты сборки локализованных дистрибутивов, ссылаются на несуществующие сети MuckyFoot

---

## Итерация 14 — Пункт 2: Удаление MFLib1, редакторских ресурсов, мусора

**Дата:** 2026-03-16

**Удалено:**
- `new_game/MFLib1/` — исходники библиотеки MuckyFoot (не компилировалась, не линковалась)
- `new_game/fallen/Source/GEdit.cpp` — главный файл редактора GEdit, не в vcxproj
- `new_game/fallen/Loader/` — PSX-only загрузчик языковых версий (PSX libs, не в vcxproj)
- `new_game/fallen/outro/outro.dsp` — старый проектный файл MSVC6
- `new_game/fallen/DDLibrary/DDlib.rc` — 2200 строк редакторских ресурсов (меню, диалоги, тулбары)
- `new_game/fallen/DDLibrary/Headers/resource.h` — все IDD_/IDR_/IDI_ символы для редактора
- 20 BMP/ICO/CUR из `DDLibrary/` — тулбарные иконки редактора

**Правки в vcxproj:**
- Удалены CustomBuild/Image записи на удалённые BMP/ICO/CUR файлы
- Удалена ResourceCompile запись для DDlib.rc
- Удалена ClInclude запись для resource.h
- Удалена запись `<Text Include="notes.txt" />` (файл не существовал)
- Удалён путь `c:\mflib1\libs` из AdditionalLibraryDirectories

**Правки кода (для компиляции без DDlib.rc):**
- `DDLibrary/Headers/DDLib.h`: удалён `#include "resource.h"`
- `DDLibrary/Source/MFX.cpp`: удалён `#include "resource.h"`
- `DDLibrary/Source/GHost.cpp`: `LoadIcon(..., IDI_ICON2)` → `NULL`; `LoadAccelerators(..., IDR_MAIN_ACCELERATOR)` → `NULL`

**Результат:** 0 ошибок, 293 предупреждения (прежнее количество).

**Оставлено:**

- `text/` — рантайм-ресурсы (lang_*.txt загружаются через XLAT_load из frontend.cpp)
- `config.ini` — загружается через ENV_load("config.ini") в Main.cpp
- `vcpkg.json`, `vcpkg_installed/` — зависимости сборки
- `Fallen.sln`, `Fallen.vcxproj`, `Fallen.vcxproj.filters` — система сборки
- `Debug/`, `Release/` — не в git, локальные артефакты сборки (не трогались)

---

## Итерация 15 — Чистка Fallen.vcxproj и Fallen.sln

**Дата:** 2026-03-16

**Fallen.vcxproj — удалено:**
- `AdditionalIncludeDirectories`: пути `..\fallen\ledit\headers`, `..\fallen\sedit\headers`, `..\fallen\gedit\headers` — папки удалены в итерации 12
- `AdditionalLibraryDirectories`: все захардкоженные старые пути (`c:\fallen\bink`, `c:\miles`, `c:\fallen\miles`, `C:\qsound\QM411SDK\qmdx`) — пути на машинах MuckyFoot 1999 года
- `<Midl>` секции в обоих конфигах — MIDL для COM/IDL, у игры нет IDL файлов, leftover от апгрейда MSVC6
- `<Bscmake>` секции + `<BrowseInformation>true` в Release — генератор .bsc файлов для старого VS source browser
- Пустые `<PostBuildEvent>` блоки в обоих конфигах
- `<SccProjectName>` / `<SccLocalPath>` — пустые теги source control, артефакт MSVC6

**Fallen.sln — исправлено:**
- Убраны фантомные конфиги `Debug|x64` и `Release|x64` (добавлены автоматически при конвертации из MSVC6, маппились на Win32 и не строили ничего своего)
- Оставлены только `Debug|Win32` и `Release|Win32` с корректными Build.0 записями

**Результат:** 0 ошибок, 293 предупреждения.

---

## Итерация 16 — Удаление #ifdef BIKE (пункт 2.5)

**Дата:** 2026-03-17

**Команда:**
```
tools/coan/coan.exe source -UBIKE --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine Headers DDLibrary
```
Exit code 2 — предупреждения (норма).

**Удалено через coan (20 файлов, ~3323 строк):**
- `DDEngine/Source/aeng.cpp` — 209 строк (рендеринг мотоцикла)
- `DDEngine/Source/smap.cpp` — 176 строк
- `Headers/Thing.h` — 3 строки
- `Source/Controls.cpp` — 27 строк
- `Source/Game.cpp` — 17 строк
- `Source/Person.cpp` — 124 строки
- `Source/Vehicle.cpp` — 36 строк
- `Source/collide.cpp` — 21 строка
- `Source/dirt.cpp` — 9 строк
- `Source/elev.cpp` — 3 строки
- `Source/eway.cpp` — 12 строк
- `Source/interfac.cpp` — 101 строка
- `Source/io.cpp` — 6 строк
- `Source/memory.cpp` — 15 строк
- `Source/ob.cpp` — 12 строк
- `Source/overlay.cpp` — 27 строк
- `Source/pcom.cpp` — 637 строк (AI BIKER тип + все bike-related AI)
- `Source/psystem.cpp` — 9 строк
- `Headers/bike.h` — 172 строки (оставлен только include guard — удалён вручную)
- `Source/bike.cpp` — 1707 строк (оставлен только комментарий — удалён вручную)

**Удалено вручную:**
- `new_game/fallen/Source/bike.cpp` — целиком
- `new_game/fallen/Headers/bike.h` — целиком
- `Fallen.vcxproj`: `<ClCompile Include="Source\bike.cpp">` + `<ClInclude Include="Headers\bike.h" />`

**Риски потери кода:** Мотоцикл не вошёл в финальную игру (ни PC, ни PS1) — подтверждено KB (`cut_features.md`). Код полный и функциональный, но намеренно выключен на этапе разработки. Восстанавливать не планируется — в Этапе 10 контроллер xbox реализуется через существующее PS1-поведение, не через BIKE.

**Нюансы:**
- `BIKE` нигде не был определён — код никогда не компилировался; удаление не затрагивает активную логику
- `PCOM_AI_BIKER (11)` — тип AI мотоциклиста — удалён из pcom.cpp (637 строк); enum CLASS_BIKE (17) остался в Thing.h т.к. он не под `#ifdef BIKE`
- `game.h:90` и `memory.cpp:9` включали `bike.h` без `#ifdef BIKE` — coan их не тронул (убирает только блоки, а не standalone `#include`). Удалены вручную.
- `Fallen.vcxproj.filters` содержал запись на `Headers\bike.h` — удалена.

**Результат:** 0 ошибок, 293 предупреждения.

---

## Итерация 17 — Удаление #ifdef FAST_EDDIE (пункт 2.5)

**Дата:** 2026-03-17

**Команда:**
```
tools/coan/coan.exe source -UFAST_EDDIE --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine Headers DDLibrary
```
Exit code 19 — норма.

**Удалено через coan (3 файла, 14 строк):**
- `DDEngine/Source/aeng.cpp` — KB_1 checkerboard culling (шахматная отсечка рендера)
- `Source/Controls.cpp` — `allow_debug_keys = 1` ветка для FAST_EDDIE; схлопнулась в `#ifndef NDEBUG` / `else`
- `Source/Vehicle.cpp` — KB_T Batman-ускорение (`accel <<= 1`); `#if !defined(FAST_EDDIE) || !defined(_DEBUG)` → безусловный блок урона (логика не изменилась)

**Риски потери кода:** QA-build флаг — дев-инструментарий, в финальной игре не нужен. Debug keys в Release — возможность включить через консоль `"bangunsnotgames"` без флага сохранена. Batman-ускорение (`KB_T`) и checkerboard culling (`KB_1`) — отладочные читы, не часть геймплея.

**Нюансы:**
- `FAST_EDDIE` — QA-build флаг, нигде не определён; весь код под ним был мёртвым
- `Vehicle.cpp:2385`: `#if !defined(FAST_EDDIE) || !defined(_DEBUG)` всегда было TRUE (FAST_EDDIE не определён), блок уже был активен. После удаления — безусловный, поведение то же самое.

---

## Итерация 18 — Удаление #ifdef DOG_POO (пункт 2.5)

**Дата:** 2026-03-17

**Команда:**
```
tools/coan/coan.exe source -UDOG_POO --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine Headers DDLibrary
```
Exit code 19 — норма.

**Удалено через coan (4 файла, 1360 строк):**
- `DDEngine/Source/aeng.cpp` — 461 строка (debug-переменные и блоки визуализации коллизий)
- `Source/collide.cpp` — 668 строк: статические пулы `col_vects[10000]` (~300 KB) + `col_vects_links[10000]` (~40 KB) + функции `add_collision_to_single_cell`, `do_move_collide`, `collide_box`
- `Source/Darci.cpp` — 40 строк: функция `setup_person_for_jump_grab`
- `Source/spark.cpp` — 191 строка: 2 блока поиска позиций по `col_vects`

**Риски потери кода:** `DOG_POO` — флаг старой экспериментальной коллизионной системы, заменённой активной. В финальной игре не используется, нигде не определялся. Функции `do_move_collide`, `collide_box` — альтернативные реализации коллизий, заменены активным кодом в `collide.cpp`. Потери функциональности нет.

**Нюансы:**
- `cam.cpp` (весь под `#ifdef DOG_POO`) удалён ещё в итерации 10 — там остатков не было
- `OLD_DOG_POO_OF_A_SYSTEM_OR_IS_IT` в `building.cpp` — другой флаг (developer joke), не тронут
- Комментарии `// claude-ai:` в `fc.h` и `fc.cpp` упоминают `DOG_POO` — это информационные комментарии, не код, не трогались

**Результат:** 0 ошибок, Debug: 131 предупреждение, Release: 293 предупреждения.

---

## Итерация 19 — Удаление #ifdef OLD_CAM (пункт 2.5)

**Дата:** 2026-03-17

**Команда:**
```
tools/coan/coan.exe source -UOLD_CAM --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine Headers DDLibrary
```
Exit code 19 — норма. `CAM_OLD` в коде не встречался — отдельной команды не нужно.

**Удалено через coan (4 файла, 80 строк):**
- `Source/Controls.cpp` — 45 строк (7 блоков вызовов старой камеры)
- `Source/elev.cpp` — 16 строк
- `Source/eway.cpp` — 3 строки
- `Source/memory.cpp` — 16 строк

**Риски потери кода:** Старая камера заменена на `fc.cpp` (Final Camera) до финального релиза. Обе версии игры (PC и PS1) используют только FC-систему. Возврата к старой камере не планируется.

**Нюансы:**
- `cam.cpp` удалён ещё в итерации 10; здесь удалены только оставшиеся вызовы `CAM_*` функций в активных файлах
- `cam.h` по-прежнему включается в нескольких файлах — все вызовы уже под `#ifdef OLD_CAM`, линкерных проблем нет

**Результат:** 0 ошибок.

---

## Итерация 20 — Удаление #ifdef SEWERS (пункт 2.5)

**Дата:** 2026-03-17

**Команда:**
```
tools/coan/coan.exe source -USEWERS --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine Headers DDLibrary
```
Exit code 19 — норма.

**Удалено через coan (1 файл, 348 строк):**
- `DDEngine/Source/aeng.cpp` — функция `AENG_draw_sewer_editor()` целиком

**Риски потери кода:** `SEWERS` — исключительно редакторский флаг для отрисовки сетки канализаций в level editor. Сама система канализаций (`sewer.cpp/h`) компилируется без флага и не тронута. Редактор уровней в этом проекте не воссоздаётся.

**Нюансы:**
- `SEWERS_EXIST` (PSX-only) удалён ещё в итерации 3 вместе с PSXENG
- `sewer.cpp/h` не тронуты — компилируются без флага, рантайм-код

**Результат:** 0 ошибок.

---

## Итерация 21 — Удаление #ifdef TRUETYPE (пункт 2.5)

**Дата:** 2026-03-17

**Команда:**
```
tools/coan/coan.exe source -UTRUETYPE --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine Headers DDLibrary
```
Exit code 19 — норма.

**Удалено через coan (3 файла, 52 строки):**
- `DDEngine/Source/font2d.cpp` — 42 строки (5 альтернативных TrueType-путей рендеринга)
- `DDEngine/Source/texture.cpp` — 6 строк (`TT_Init()` / `TT_Term()`)
- `Source/frontend.cpp` — 4 строки (`GetTextHeightTT()`, активный `y += 20` сохранён из `#else`)

**Риски:** TrueType шрифты в финальную игру не вошли — ни PC, ни PS1. Финальная игра работает на bitmap-шрифтами, активный код сохранён. Функционал не нужен.

**Результат:** 0 ошибок.

---

## Итерация 22 — Удаление региональных/DRM флагов (пункт 2.5)

**Дата:** 2026-03-17

**Удалено вручную:**
- `Headers/password.h` — целиком (220 строк)

**Что было внутри:** `#define EIDOS` (активный) + 13 закомментированных региональных флагов (`USA`, `GERMANY`, `FRANCE`, `KK`, `SINGAPORE`, `LEADER`, `HALIFAX`, `SYNTHESIS`, `DLMM`, `EEM`, `EUROPE`, `JAPAN`, `TDFX`). Каждый задавал ROT13-закодированные `EXPORT_NAME`, `EXPORT_CO`, `EXPORT_PW` дистрибьютора и `MAGIC_KEY`. Внизу файла — большой `/* */` блок с незакодированными именами (Darren Hedges, Nicholas Earl и др.).

**Риски:** DRM конкретных розничных дистрибьюторов Eidos 1999 года не нужен в open-source фан-порте. Файл нигде не включался — на компиляцию и работу игры не влиял. Функционал не нужен.

**Результат:** 0 ошибок.

---

## Итерация 23 — Удаление мёртвых компиляторов/платформ: `__WATCOMC__`, `__DOS__`, `__WINDOWS_386__`, `_MF_DOSX` (пункт 2.5)

**Дата:** 2026-03-17

**Команда:**
```
tools/coan/coan.exe source -U__WATCOMC__ -U__DOS__ -U__WINDOWS_386__ -U_MF_DOSX --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine DDLibrary Headers outro
```
Exit code 0 — норма.

**Удалено через coan (2 файла, 24 строки):**
- `Headers/inline.h` — 13 строк: `__WINDOWS_386__` блок с Watcom asm-реализациями `MUL64`/`DIV64` (inline C++ версии сохранены)
- `outro/midasdll.h` — 11 строк: `__DOS__` блок calling conventions + `__WATCOMC__` warning pragma suppressions

**Риски:** Совместимость с Watcom C++ компилятором и DOS-платформой. В финальную игру не вошли (финал компилируется MSVC). Не нужны.

**Нюансы:**
- `__WATCOMC__`, `__DOS__`, `__WINDOWS_386__` — predefined компилятором (Watcom), в исходниках нигде не определялись; coan обрабатывает как undefined
- `_MF_DOSX` — определялся только внутри `#ifdef __DOS__` в MFTypes.h (удалён вместе с __DOS__)
- MFStdLib/ уже отсутствует в new_game (удалена ранее), coan не нашёл — норма

**Результат:** 0 ошибок. Debug: 131 предупреждение, Release: 293 предупреждения.

---

## Итерация 24 — Удаление отключённых оптимизаций (пункт 2.5)

**Дата:** 2026-03-17

**Команда coan:**
```
coan source -UUSE_W_FOG_PLEASE_BOB -UQUICK_FACET -UDO_SUPERFACETS_PLEASE_BOB -UMIKES_UNUSED_AUTOMATIC_FLOOR_TEXTURE_GROUPER -UBOGUS_TGAS_PLEASE_BOB -UDOWNSAMPLE_PLEASE_BOB_AMOUNT --no-transients --replace DDEngine/Headers/poly.h DDEngine/Source/facet.cpp DDEngine/Source/aeng.cpp DDLibrary/Source/Tga.cpp
```
Exit code 19 — норма.

**Удалено через coan (4 файла):**
- `DDEngine/Headers/poly.h` — `#ifdef USE_W_FOG_PLEASE_BOB` ветка (~4 строки), оставлен активный `#else` код fog
- `DDEngine/Source/facet.cpp` — `#ifdef QUICK_FACET` блок (3 строки); `#ifdef DO_SUPERFACETS_PLEASE_BOB` блок (~32 строки)
- `DDEngine/Source/aeng.cpp` — `#ifdef MIKES_UNUSED_AUTOMATIC_FLOOR_TEXTURE_GROUPER` блок (~346 строк: группировка текстур пола)
- `DDLibrary/Source/Tga.cpp` — `#ifdef BOGUS_TGAS_PLEASE_BOB` блок (31 строка); `#ifdef DOWNSAMPLE_PLEASE_BOB_AMOUNT` блок (34 строки)

**Удалено вручную:**
- `DDEngine/Source/poly.cpp:106` — `#define NO_BACKFACE_CULL_PLEASE_BOB 0` (определён но никогда не проверялся через #if)
- `DDEngine/Source/facet.cpp` — `// #define QUICK_FACET 1` (закомментированный define)

**Оставлено:**
- `SUPERCRINKLES_ENABLED` — отложено до выяснения; удалён в итерации 25.

**Результат:** 0 ошибок. Debug: 131 предупреждение, Release: 293 предупреждения.

---

## Итерация 25 — Удаление SUPERCRINKLES_ENABLED (пункт 2.5)

**Дата:** 2026-03-17

**Метод:** вручную (не через coan — флаг определялся как `#define SUPERCRINKLES_ENABLED 0` + `#if SUPERCRINKLES_ENABLED`, а не через `#ifdef`; coan в принципе умеет такое, но в этот раз удалено вручную).

**Удалено файлов целиком:**
- `DDEngine/Headers/supercrinkle.h` — весь файл (47 строк)
- `DDEngine/Source/supercrinkle.cpp` — весь файл (~1055 строк)
- Обе записи удалены из `Fallen.vcxproj`

**Удалено из кода:**
- `facet.cpp`: `#include "supercrinkle.h"` + мёртвый `else`-блок (~50 строк) в ветке `if (AENG_detail_crinkles && ...)` — суперкринкл вызывался только когда `AENG_detail_crinkles` был false
- `superfacet.cpp`: `#include "supercrinkle.h"` + `#define SUPERFACET_CALL_FLAG_CRINKLED` + поле `crinkle` в `SUPERFACET_Call` + два больших `if (CRINKLED) { ... } else { рабочий код }` блока (~200 строк удалено, рабочий `else`-код сохранён) + упрощена логика группировки вызовов
- `Game.cpp`: `#include "..\ddengine\headers\supercrinkle.h"`

**Почему удалён:** Пользователь подтвердил — обычный bump mapping (crinkles) работает (хотя и глючит, баг залогирован в `prerelease_fixes.md`). Supercrinkles — улучшенная версия, выключена, в финальную игру не вошла. Восстанавливать не нужно: оригинал в `original_game/` есть.

**Нюансы:**
- `SUPERCRINKLE_draw()` в `superfacet.cpp` вызывался только внутри `if (sc->flag & SUPERFACET_CALL_FLAG_CRINKLED)` — флаг никогда не выставлялся (т.к. `SUPERCRINKLE_IS_CRINKLED` всегда возвращал FALSE). Оба блока были полностью мёртвыми.
- При ручном удалении `else`-блока в `facet.cpp` была допущена ошибка: удалена лишняя закрывающая `}` от `if (AENG_detail_crinkles...)`. Исправлено добавлением `}` обратно.
- ⚠️ **Нарушение процесса:** флаг удалён без предварительного чтения `preprocessor_flags.md` и `DENSE_SUMMARY.md`, без использования coan, без записи в лог до работы. В следующий раз — строго по регламенту.

**Результат:** 0 ошибок, Debug + Release собираются чисто.


---

## Итерация 26 — Удаление мёртвого геймплея (пункт 2.5)

**Дата:** 2026-03-17

**Команда coan:**
```
coan source -UDARCI_HITS_COPS -UNO_MORE_BALLOONS -UNO_MORE_BALLOONS_NOW -UWE_WANT_WIND -UWE_WANT_MANUAL_WIND_ASWELL -UWE_WANT_SHITTY_PANTS_WIND -UUNUSED_WIRECUTTERS -UUNUSED_WIRE_CUTTERS --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine Headers DDLibrary
```
Exit code 19 — норма.

**Удалено через coan + 1 правка вручную:**
- `Source/Combat.cpp` — 3 блока `#if DARCI_HITS_COPS` + 1 блок внутри `/* */` (удалён вручную)
- `Source/pcom.cpp` — 3 блока `#if DARCI_HITS_COPS`
- `Source/Person.cpp` — 1 блок `#if DARCI_HITS_COPS` + 1 блок `#ifdef UNUSED_WIRECUTTERS`
- `Source/Controls.cpp` — блок `#if WE_WANT_WIND` (~90 строк) с вложенными `WE_WANT_MANUAL_WIND_ASWELL` и `WE_WANT_SHITTY_PANTS_WIND`
- `Source/collide.cpp` — 5 блоков `#ifdef UNUSED_WIRECUTTERS`
- `DDEngine/Source/aeng.cpp` — 2 блока: `#if NO_MORE_BALLOONS` и `#if NO_MORE_BALLOONS_NOW`
- `DDEngine/Source/facet.cpp` — 3 блока `#ifdef UNUSED_WIRECUTTERS` + 1 блок `#ifdef UNUSED_WIRE_CUTTERS`

**Нюансы:**
- `DARCI_HITS_COPS` используется как `#if` (не `#ifdef`) — coan обрабатывает корректно как нулевое значение
- `WE_WANT_MANUAL_WIND_ASWELL` и `WE_WANT_SHITTY_PANTS_WIND` вложены внутрь `WE_WANT_WIND` — удалились автоматически
- `DIRT_gale()` уже была в `/* */` в dirt.cpp — `WE_WANT_WIND` не линковался бы даже при включении
- `DIRT_gust()` (разлёт листьев от движения персонажей/машин) — активный код, не тронут
- `/* */` блок в Combat.cpp с `#if DARCI_HITS_COPS` внутри — coan не трогает комментарии, удалён вручную

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 292 предупреждения.

---

## Итерация 27 — Старые алгоритмы PSXENG / `OLD_POO` (2026-03-17)

**Флаги:** `OLD_FACET_CLIP`, `OLD_FLIP`, `OLD_POO`, `OLD_SPLIT`, `CUNNING_SORT`, `GOOD_SORT`, `BACK_CULL_MAGIC`

**Результат:**
- `OLD_FACET_CLIP`, `OLD_FLIP`, `OLD_SPLIT`, `CUNNING_SORT`, `GOOD_SORT`, `BACK_CULL_MAGIC` — ➖ отсутствовали в new_game (были только в PSXENG, удалённом ранее)
- `OLD_POO` — ✅ удалён: 1 вхождение в `Source/overlay.cpp`, тело функции `track_enemy()` (~50 строк)

**Что удалено:** старая HUD-система отображения врагов — портреты лиц (6 спрайтов по PersonType) + HP-полоска + полоска скилла в углу экрана (y=450). Заменена `PANEL_draw_local_health()` — floating HP bar над головой врага в 3D. Финальная игра использует новую систему (подтверждено по видео).

**Нюансы:**
- `OVERLAY_draw_tracked_enemies()` — тоже мёртвый код (нигде не вызывается), но без флага; оставлена на следующий проход (п.3 — удаление мёртвых сущностей)
- `track_enemy()` вызывается из Combat.cpp/pcom.cpp/Person.cpp — функция-стаб сохранена

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 292 предупреждения.

---

## Итерация 28 — PSX/DC debug флаги (2026-03-17)

**Флаги:** `PSX_COMPRESS_LIGHT`, `DODGYPSXIFY`, `PSX_SIMULATION`, `FILE_PC`, `_DEBUG_POO`, `USE_A3D`, `A3D_SOUND`

**Результат:**
- `PSX_SIMULATION`, `FILE_PC` — ➖ отсутствовали в new_game
- `PSX_COMPRESS_LIGHT` (Night.h) — ✅ удалён: выбор между packed `UWORD Col` (PSX) vs `UBYTE red,green,blue` (PC). Активная PC-ветка сохранена.
- `DODGYPSXIFY` (Sound.h) — ✅ удалён: мёртвая переменная `dodgy_psx_mode` и ветка в `SOUND_Range()`
- `_DEBUG_POO` (Quaternion.cpp) — ✅ удалён: проверки ортогональности матриц + ASSERT
- `A3D_SOUND` (A3DManager.cpp) — ✅ удалён: `#ifndef A3D_SOUND → return` (A3D init) и `#ifdef A3D_SOUND` (CleanUp). Init остался с безусловным return — A3D не инициализируется.
- `USE_A3D` — ✅ удалён: `#define USE_A3D` из Sound.h убран вручную, декларация `A3D_Check_Init()` удалена. Активные блоки (elev.cpp include soundenv.h + `SOUNDENV_precalc()`) удалены через coan. Мёртвые блоки (briefing.cpp, Person.cpp, soundenv.cpp, Main.cpp — все были закомментированы) удалены. Game.cpp и Sound.cpp — были в `/* */`, coan не трогал.

**Нюансы:**
- `SOUNDENV_precalc()` больше не вызывается (была под `#ifdef USE_A3D` в elev.cpp). A3D технология мертва (Aureal обанкротилась в 2000), функция не нужна.
- soundenv.h теперь не включается в elev.cpp — но soundenv.cpp остаётся в проекте (может быть нужен в будущем при портировании реверба)

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 292 предупреждения.

---

## Итерация 29 — Debug-флаги (2026-03-17)

**Флаги:** `DEBUG_POOSHIT`, `TEST_3DFX`, `HEAP_DEBUGGING_PLEASE_BOB`, `MARKS_PRIVATE_VERSION`, `_DEBUG_POO`, `BREAKTIMER`
(`HIGH_REZ_PEOPLE_PLEASE_BOB` — пропущен намеренно, разобраться позже)

**Результат:**
- `DEBUG_POOSHIT`, `TEST_3DFX` — ➖ уже не было в коде (удалены вместе с редакторами)
- `_DEBUG_POO` — ➖ уже не было в коде (удалён в итерации 28)
- `BREAKTIMER` — ✅ удалён: BreakTimer.h упрощён (оставлены no-op макросы), из BreakTimer.cpp удалён весь блок с реализацией (~170 строк), из Game.cpp убран wrapper `#ifndef BREAKTIMER`
- `MARKS_PRIVATE_VERSION` — ✅ удалён: fc.cpp — 2 `#ifndef MARKS_PRIVATE_VERSION` / `#endif` убраны, тело сохранено (gun-out камера всегда активна — верное поведение)
- `HEAP_DEBUGGING_PLEASE_BOB` — ✅ удалён: StdMem.cpp очищен — убраны HeapDebugInfo структура, DumpDebug(), heap tracking в MemAlloc/MemFree/SetupMemory/ResetMemory, альтернативные MFnewTrace/MFdeleteTrace

**Нюанс:** `MARKS_PRIVATE_VERSION` использовал `#ifndef` (не `#ifdef`): тело всегда активно, флаг никогда не определялся. Убраны только обёртки.

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 292 предупреждения.

---

## Итерация 30 — Debug визуализации (2026-03-17)

**Флаги:** `STRIP_STATS`, `DRAW_THIS_DEBUG_STUFF`, `ARGH`, `FEEDBACK`, `FASTPRIM_PERFORMANCE`, `WE_WANT_TO_DRAW_THESE_FACET_LINES`, `SUPERFACET_PERFORMANCE`, `DTRACE` (в мёртвых `#else` ветках)

**Инструмент:** coan с `--no-transients` на 6 файлах + ручные правки для figure.cpp (DTRACE).

**Результат:**
- `STRIP_STATS` — ✅ удалён из aeng.cpp (3 блока), panel.cpp (1 блок)
- `DRAW_THIS_DEBUG_STUFF` — ✅ удалён из aeng.cpp, facet.cpp
- `ARGH` — ✅ удалён из aeng.cpp (~60 строк debug визуализации треугольников)
- `FEEDBACK` — ✅ удалён из flamengine.cpp
- `FASTPRIM_PERFORMANCE` — ✅ удалён из fastprim.cpp (10+ блоков счётчиков)
- `WE_WANT_TO_DRAW_THESE_FACET_LINES` — ✅ удалён из facet.cpp
- `SUPERFACET_PERFORMANCE` — ✅ удалён из superfacet.cpp; флаг был `#define`'ан в самом файле — использовался `--no-transients` для переопределения; убран также coan-inserted error comment
- `DTRACE` — ✅ удалён из figure.cpp: 3 мёртвые `#else` ветки `#if 1` удалены вручную + восстановлены `#endif` для `#if 1`

**Нюанс:** `SUPERFACET_PERFORMANCE` был активно `#define`'ан в superfacet.cpp (не закомментирован). Coan с `--no-transients -USUPERFACET_PERFORMANCE` корректно его удалил, оставив coan-error comment который был убран вручную.

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 292 предупреждения.

---

## Итерация 31 — Developer joke flags (2026-03-17)

**Флаги:** `MIKE`, `MAD_AM_I`, `OLD_DOG_POO_OF_A_SYSTEM_OR_IS_IT`
(остальные — `GONNA_FIREBOMB_YOUR_ASS`, `WHAT_THE_FUCK_IS_THIS_DOING_HERE`, `OLDSHIT` и др. — были только в PSXENG, уже удалены)

**Инструмент:** coan `-UMIKE -UMAD_AM_I -UOLD_DOG_POO_OF_A_SYSTEM_OR_IS_IT --no-transients` на 10 файлах.

**Файлы:** pyro.h, building.cpp, dirt.cpp, elev.cpp, eway.cpp, fc.cpp, Game.cpp, interfac.cpp, overlay.cpp, DIManager.cpp

**Нюанс:** В fc.cpp флаг `MIKE` определял `VERSION_NTSC`. После удаления `#ifdef MIKE` остался `#ifndef MARKS_MACHINE / #define MARKS_MACHINE / #endif` — это корректно: он был ВНЕ блока `#ifdef MIKE` и всегда активен. `MARKS_MACHINE` управляет высотой камеры (0x16000 vs 0x1a000), всегда определён.

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 292 предупреждения.

---

## Пункт 3 — Удаление orphan-файлов (2026-03-18)

Удалены файлы, не упомянутые в `Fallen.vcxproj` и не включаемые ни одним `#include`.

**Удалено 86 файлов:**

- **fallen/Source/ (73 файла):** editor setup-файлы (`*Setup.cpp`), а также мёртвые файлы: `Camera.cpp` (старая камера, заменена fc.cpp), `sewer.cpp` (вырезанная фича SEWERS), `briefing.cpp`, `cutscene.cpp`, `converse.cpp`, `Mission.cpp`, `cloth.cpp`, `water.cpp`, `mesh.cpp`, и др.
- **fallen/outro/ (3 файла):** `checker.cpp`, `lmap.cpp`, `slap.cpp` — не в vcxproj (остальные outro-файлы в vcxproj есть)
- **DDEngine/Source/ (3 файла):** `Attract.cpp` (attract mode, вырезан), `sw.cpp` (software renderer), `Temp.cpp` (временный)
- **DDLibrary/Source/ (4 файла):** `A3DManager.cpp` (экспериментальный Aureal 3D), `AsyncFile.cpp`, `GFile.cpp`, `QSManager.cpp`
- **fallen/Headers/ (4 файла):** `Camera.h`, `EngWind.h`, `nd.h`, `qls.h`

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 293 предупреждения.

---

## Итерация 32 — Инвентаризация и удаление оставшихся флагов (2026-03-18)

### Что удалено

**Инструмент:** coan в двух батчах + ручное удаление комментированных дефайнов. ~55 файлов.

**DC/PSX платформа:**
- `TARGET_DC` — Dreamcast-ветки. Пропущен в ранних итерациях.
- `PSX`, `psx` (lowercase) — PSX-платформа. `psx` — в psystem.cpp.
- `LAZY_LOADING_MEMORY_ON_DC_PLEASE_BOB` — DC-специфичная загрузка памяти.
- `TEST_DC` — DC-тест в memory.cpp (в т.ч. `#if TEST_DC`).
- `DREAMCAST_CHEATS_PLEASE_BOB` — DC читы в interfac.cpp.
- `VERSION_PSX` — PSX версия в eway.cpp.

**Пропущенные региональные (должны были уйти в итерации 22):**
- `VERSION_GERMAN`, `VERSION_FRENCH` — были закомментированы в Game.cpp (`// #define VERSION_GERMAN 1`), но `#ifdef` блоки остались.
- `VERSION_USA` — в memory.cpp.
- `VERSION_NTSC` — 2 блока в fc.cpp. `MIKE` убрали в итерации 31, но сами `#ifdef VERSION_NTSC` оставались.

**Мёртвые платформенные баги:**
- `PSX_NOT_REALLY` (fc.cpp), `PSX_STERN_REVENGE_BUG_AND_CRAP_DRIVERS` (Thing.cpp).
- `__WC32__` (Watcom compiler) — в outro/midasdll.h.

**Мёртвый звук:**
- `USE_A3D` — A3D звук. Пропущен в итерации 28. Блоки в chopper.cpp, Game.cpp, Person.cpp.

**По запросу пользователя (было в "needs discussion"):**
- `VERSION_DEMO` — демо-версия (ограничение уровней, меню). Удалить решили т.к. в финале нет.
- `JAPANESE` — японская локализация в xlat_str.cpp. Удалить решили.
- `DAMAGE_TEXT` — плавающие числа урона в 3D над персонажами. Удалить решили.
- `ALLOW_DANGEROUS_OPTIONS` — dev-опции в frontend. Удалить решили.

**Мёртвые dev/joke флаги (никогда не определены):**
`OBEY_SCRIPT`, `BEHEAD`, `FUNNY_FANNY`, `SOME_POO`, `MATTS_BUGGY_VERSION`, `UNFINISHED`,
`ROPER_EVER_ARRESTS_AGAIN`, `NOT_USED`, `NOT_SETUP_IN_PROJECTILE`,
`WE_WANT_TO_TURN_AND_PUT_OUR_BACK_TO_THE_WALL`, `WE_WANT_TO_APPLY_THE_EQUAL_FORCE_TO_THE_CUBE`,
`UNUSED_WIRECUTTER`, `USE_ACTION_TO_THROW_GRENADE`, `SHOW_ANIM_NUMS`, `AMERICA`, `GUY`,
`FORCE_STUFF_PLEASE_BOB`, `LIMIT_TOTAL_PYRO_SPRITES_PLEASE_BOB`, `ANIM_PRIM` (как флаг), `ANIM_PRIMS`

**Примечание:** `ANIM_PRIM` как значение (`CLASS_ANIM_PRIM`, `DT_ANIM_PRIM`, `TT_ANIM_PRIM` и т.д.) — НЕ затронуто. Убраны только `#ifdef ANIM_PRIM` / `#ifdef ANIM_PRIMS` guard-блоки.

**Ручная чистка:** убраны комментированные `// #define ...` из america.h, demo.h, briefing.h, Game.cpp, xlat_str.cpp, frontend.cpp, drawxtra.cpp.

**Результат:** 0 ошибок, Debug: 130 предупреждений, Release: 293 предупреждения.

---

## Справка: Флаги которые ОСТАВЛЕНЫ (решено не трогать) — 2026-03-18

### Всегда активны (определены in-file, живой код)

| Флаг | Где определён | Что делает |
|------|--------------|-----------|
| `NO_SERVER` | `Headers/noserver.h` | Всегда 1. Отключает сетевой сервер-режим. Убирает ряд условных путей в texture.cpp, font2d.cpp, supermap.cpp и др. |
| `ULTRA_COMPRESSED_ANIMATIONS` | `Headers/anim.h` | Всегда активен. Определяет формат хранения анимационных данных (сжатые vs полные). Критичен для совместимости с ресурсами. |
| `NEW_LEVELS` | `Source/memory.cpp` | Всегда 1. Контролирует формат загрузки уровней в memory.cpp. Нужен для работы с финальными ресурсами. |
| `REMAP_KEYBOARD` | `Source/interfac.cpp` | Всегда 1. Включает поддержку переназначения клавиш. |
| `MARKS_MACHINE` | `Source/fc.cpp` | Всегда определён. Высота камеры 0x16000 (vs 0x1a000). |
| `WANT_AN_EXIT_MENU_ITEM` | `Source/frontend.cpp` | PC-специфичный пункт "Exit" в меню. |
| `WANT_A_KEYBOARD_ITEM` | `Source/frontend.cpp` | Пункт настройки клавиатуры в меню. |
| `WANT_A_START_JOYSTICK_ITEM` | `Source/frontend.cpp` | Пункт джойстика в меню. |
| `ANNOYING_HACK_FOR_SIMON` | `Source/frontend.cpp` | Хак в меню (всегда включён). |
| `ALLOW_JOYPAD_IN_FRONTEND` | `Source/frontend.cpp` | Джойпад в меню (всегда включён). |
| `MUST_DOUBLE_CLICK_FORWARDS_TO_GET_OUT_OF_FIGHT_MODE` | `Source/interfac.cpp` | Поведение выхода из режима драки. |
| `FACETINFO` | `DDEngine/Source/facet.cpp` | Сбор debug-инфо по фасетам (массивы FACETINFO). Всегда включён. |
| `TOMS_TEST_FIXUP_CODE` | `DDEngine/Source/aeng.cpp` | Fixup-код для рендера пола (всегда включён). |
| `MESH_SHOW_MOUSE_POINT` | `DDEngine/Source/mesh.cpp` | Визуализация точки курсора на mesh (всегда включена). |
| `SHOW_ME_SUPERFACET_DEBUGGING_PLEASE_BOB` | `DDEngine/Source/superfacet.cpp` | Debug-инфо superfacet (всегда включено). |

### Намеренно оставлены для разработки

| Флаг | Причина |
|------|---------|
| `FACET_REMOVAL_TEST` | Auto-включается в `_DEBUG`. Полезен при разработке рендерера. |
| `HIGH_REZ_PEOPLE_PLEASE_BOB` | Отложен — требует отдельного глубокого анализа (4x вершин у персонажей, cheat-код `0x10f1a7e`). |

### Требуют разбирательства (пока не трогать)

**`TEX_EMBED` (63 вхождения)**
- Закомментирован в polypage.h (`// #define TEX_EMBED`). Сейчас **НЕ** определён → активна ветка без TEX_EMBED.
- При включении: текстуры встраиваются напрямую в вершинные буферы (embedded mode). Без него — отдельные texture stage.
- Затрагивает: poly.cpp (~30 мест), aeng.cpp (~10), fastprim.cpp, figure.cpp, polypage.cpp, texture.cpp, fastprim.cpp.
- Нельзя просто удалить `-UTEX_EMBED` ветки: нужно понять какая ветка правильная для PC. Требует анализа KB (DDEngine/rendering subsystem).

**`FINAL` (20 вхождений)**
- Нигде не определён. `#ifndef FINAL` ветки АКТИВНЫ (debug-проверки всегда работают).
- Встречается в: crinkle.cpp (1), poly.cpp (1), figure.cpp (1).
- Похоже на "финальный релизный билд" флаг (без assert'ов и debug-кода). Но в финальной игре он не определён судя по коду.
- Вопрос: нужен ли нам этот флаг в будущем (для Этапа 10)?

**`NEW_FLOOR` (8 вхождений)**
- Закомментирован в aeng.cpp (`// #define NEW_FLOOR defined`). Сейчас **НЕ** определён → активна ветка `#ifndef NEW_FLOOR`.
- Альтернативный алгоритм рендера пола в aeng.cpp. Экспериментальный, не вошёл в финал.
- Требует анализа: может пригодиться в будущем или является мёртвым экспериментом.

---

## Попутное исправление — унификация include guards (2026-03-18)

Все хедеры в `new_game/` (исключая `vcpkg_installed/`) приведены к единому соглашению об include guards.

**Правило именования:** полный путь относительно `new_game/`, заглавными буквами, `/` и `.` заменяются на `_`.
Пример: `fallen/Headers/snipe.h` → `FALLEN_HEADERS_SNIPE_H`.
Дополнительно: на закрывающий `#endif` добавлен комментарий `// GUARDNAME`.

**Охват:** 236 файлов в трёх директориях:
- `fallen/Headers/` — 145 файлов
- `fallen/DDEngine/Headers/` + `fallen/DDLibrary/Headers/` — 75 файлов (были сделаны ранее)
- `fallen/outro/` — 16 файлов
- `MFStdLib/Headers/` — 6 файлов (уже были правильными)

**Типичные проблемы, которые были исправлены:**
- Устаревший формат `_NAME_` (подчёркивания вокруг) → заменён на путевой формат
- Несовпадение имени гварда и имени файла (`interfac.h` имел `INTERFACE_H`, `light.h` имел `LIGHTG_H`, `noserver.h` имел `SERVER`)
- `#define GUARD_NAME 1` (с trailing value) → нормализовано без значения
- Файлы без гварда вообще (`grenade.h`, `music.h`, `pq.h`, `widget.h`, `soundenv.h`, `briefing.h` и др.) → добавлен полный `#ifndef`/`#define`/`#endif`
- Пустые файлы (`america.h`, `demo.h`) → добавлен минимальный гвард

**Проверка уникальности:** проверено через `grep -m1 "^#ifndef" | sort | uniq -d` — дубликатов не найдено.


## Попутное исправление — конфликты типов после унификации гвардов (2026-03-18)

После унификации include guards сборка упала с 7 ошибками переопределения типов.
Причина: два файла-пары ранее случайно использовали одинаковое имя гварда (`ENGINE_H` и `_TGA_`),
что скрывало дублирование. После уникальных гвардов оба файла стали компилироваться, и конфликт вылез.

### Пара 1: `fallen/Headers/Engine.h` vs `fallen/DDEngine/Headers/Engine.h`

**Итоговое решение: `fallen/Headers/Engine.h` удалён полностью.**

Первоначально казалось что файл нужен — в нём были объявления игровых функций (`init_3d_engine`,
`game_engine` и т.д.). Но проверка показала: ни один `.cpp` этот файл не включает (в `fallen/Source/`
он закомментирован), и реализаций этих функций в проекте нет вообще. Файл — мёртвый код.

В оригинале он тоже был мёртвым: весь контент был внутри гварда `ENGINE_H`, а `DDEngine/Headers/Engine.h`
всегда включался первым и определял тот же гвард — файл целиком скипался препроцессором.

На типах в `DDEngine/Headers/Engine.h` оставлены `// uc_also_in: fallen/Headers/Engine.h`
с пояснением что именно отличалось в удалённой копии (лишний `CameraRAngle`, отсутствующий `CameraPos`).

### Пара 2: `fallen/outro/Tga.h` vs `fallen/DDLibrary/Headers/Tga.h`

Файлы **не одинаковые** — два разных загрузчика TGA:
- `DDLibrary/Tga.h`: движковый, 6-параметрный `TGA_load`, `TGA_Info` с полем `contains_alpha`, + `TGA_save`, clump management
- `outro/Tga.h`: автономный для outro-модуля, 4-параметрный `TGA_load`, `TGA_Info` с полем `ULONG flag` (бит-флаги)

Конфликт возникал в `outro/os.cpp`: включал оба через цепочку
`os.cpp` → `tga.h` (outro) + `ddlib.h` → `D3DTexture.h` → `"tga.h"` (DDLibrary).

**Решение:** outro-типы переименованы с префиксом `OUTRO_`:
- `TGA_Pixel` → `OUTRO_TGA_Pixel`
- `TGA_Info` → `OUTRO_TGA_Info`
- `TGA_load` → `OUTRO_TGA_load`

Обновлены все 3 файла outro: `outro/Tga.h`, `outro/outroTga.cpp`, `outro/os.cpp`, `outro/outroFont.cpp`.
На типах в `DDLibrary/Tga.h` добавлены `// uc_also_in: fallen/outro/Tga.h` с пояснениями.

### Новый формат комментария: `uc_also_in`

Задокументирован в `new_game_planning/entity_mapping.md`. Используется на канонической версии
сущности, когда её дублирующее определение было удалено или переименовано в другом месте.
Грепается скриптом как `uc_also_in`.

**Результат:** Debug и Release собираются без ошибок.

---

## Итерация 33 — Release-only: удаление всего debug-кода (2026-03-18)

**Решение:** Сфокусировать кодовую базу исключительно на Release-сборке. Debug-код удаляется навсегда.
Мотивация: Release-only кодовая база проще для доработки. Если что-то понадобится — восстановить из `original_game/`.

**Обработанные флаги:**
- `-U_DEBUG -UDEBUG` — Debug-only блоки удалены
- `-DNDEBUG -DFINAL -D_RELEASE` — Release-only блоки раскрыты (обёртки убраны, код остаётся)
- `-UFACET_REMOVAL_TEST -USHOW_ME_FIGURE_DEBUGGING_PLEASE_BOB` — вторичная очистка (флаги стали undefined после удаления _DEBUG/DEBUG)

**Команды coan (в папке `fallen/`):**
```
coan source -U_DEBUG -UDEBUG -DNDEBUG -DFINAL -D_RELEASE --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine DDLibrary Headers outro
coan source -UFACET_REMOVAL_TEST -USHOW_ME_FIGURE_DEBUGGING_PLEASE_BOB --no-transients --filter cpp,c,h,hpp --recurse Source DDEngine DDLibrary Headers outro
```
Та же команда с `-U_DEBUG -UDEBUG -DNDEBUG -DFINAL -D_RELEASE` применена к `../MFStdLib/`.

**Нюансы:**
- В `aeng.cpp:8109` и `DDManager.cpp:2041` `#ifdef _DEBUG`/`#ifdef DEBUG` блоки были внутри `/* */` — coan их не тронул (верно: это уже мёртвый код внутри комментариев)
- В `Controls.cpp`, `frontend.cpp`, `Game.cpp` — только комментарии `// claude-ai:` и `// #ifndef ...` упоминают эти флаги; не код, не тронуты
- Debug: 129 предупреждений (было 130), Release: 293 — ошибок нет

**Результат:** 0 ошибок. Debug: 129 предупреждений, Release: 293 предупреждения.
