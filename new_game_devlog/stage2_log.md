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

**Инструмент:** coan с флагами `-UPSX -UTARGET_DC -UBUILD_PSX`

**Команда:**
```
tools/coan/coan.exe source -UPSX -UTARGET_DC -UBUILD_PSX --no-transients --filter cpp,c,h,hpp --recurse new_game/fallen/Source new_game/fallen/DDEngine new_game/fallen/DDLibrary new_game/fallen/Headers new_game/fallen/Loader
```

**Удалено:**
- coan обработал 214 файлов, удалил ~22700 строк PSX/DC кода
- Папки `PSXENG/`, `psxlib/`, `psxlib1/` — удалены целиком
- PSX-файлы из Source/: `nightpsx.cpp`, `Levelpsx.cpp`, `dc_credits.cpp`, `hm_psx.cpp`, `io_psx.cpp`, `pausepsx.cpp`
- `nightpsx.cpp` убран из `Fallen.vcxproj`

**Попутные правки:**
- `qls.cpp`: удалён orphan `#endif` в конце файла (баг из оригинала — там тоже есть). Файл был обёрнут в `#ifdef` который убрали, а `#endif` остался. Без этой правки coan падал с ошибкой "Orphan #endif".

**Нюансы:**
- `BUILD_PSX` определялся только внутри `#ifdef PSX` в `Game.h` → теперь безусловно не определён. `#ifndef BUILD_PSX` блоки в `memory.cpp`, `Person.cpp` стали активными (верно для PC).
- В `aeng.cpp` остались 3 `#ifdef TARGET_DC` блока (строки 3633, 3776, 3818). Они внутри `#if 0` блока с незакрытым `/* */` комментарием внутри — coan не смог обработать из-за вложенного комментария. Это дважды мёртвый код. Разберём в итерации по `#if 0`.
- Символы `PSX_NOT_REALLY`, `PSX_STERN_REVENGE_BUG_AND_CRAP_DRIVERS`, `PSX_COMPRESS_LIGHT` — другие символы, не `PSX`, coan их не трогал. Это норма.

