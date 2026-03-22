# Define Collision Audit — Preprocessor Results

**Дата:** 2026-03-22
**Кодовая база:** `new_game/src/` на пре-миграционном состоянии (до переносов Этапа 4)
**Коммит:** `2828b00` (stage 3.5 - kb update)
**Метод:** Временно заменили значения `#define` на маркеры `UCREDEFINED_*`,
прогнали `clang++ -E` на всех 176 .cpp файлах, grep'нули выход на маркеры.

**⚠️ Ограничение аудита:** препроцессор работает по translation units (`.cpp` файлам).
Таблицы ниже содержат только `.cpp` файлы. `.h` файлы в них не попали — их содержимое
раскрывается внутри `.cpp` и учтено в счётчиках, но сами `.h` как отдельные единицы
не перечислены. При замене макросов нужно обрабатывать и `.h` файлы — ориентироваться
по include-цепочкам или по компиляции (undeclared identifier = файл пропущен).

**О путях в таблицах ниже:** проверка проводилась на новой игре до реструктуризации кода,
но пути указаны без `src/` и напрямую мапятся на структуру `original_game/`. Это сделано
намеренно — при замене на `UC_*` в новой игре (где структура уже изменена) нужно будет
находить соответствующие сущности в `new/` по комментариям `// uc_orig:`, которые ссылаются
именно на оригинальные пути.

---

## Какие макросы реально резолвятся через наши `#define`

В `original_game/` нашли **18 пересечений** с стандартом/SDK (полная таблица — в
`stage4_bug_define_collisions.md`, группы A–C). Из них в `new_game/src/` на пре-миграционном
чекауте `#define` реально присутствует только для **8** — остальные 10 уже вычищены при
подготовке кодовой базы (FAILED, SUCCEEDED, ERROR, STRICT, SM_CMONITORS, ZeroMemory,
_MAX_PATH, HIWORD, LOWORD, MAX_PATH).

Из этих 8 только **3** реально используются через наши переопределения:

| Макрос | Строк в выводе препроцессора | Источник `#define` |
|--------|-----------------------------|--------------------|
| `TRUE` | 2049 | `MFStdLib.h` / `Always.h` |
| `FALSE` | 2636 | `MFStdLib.h` / `Always.h` |
| `INFINITY` | 150 | `MFStdLib.h` / `Always.h` |

**Остальные 5 — 0 попаданий** (наш `#define` не срабатывает):

| Макрос | Почему не срабатывает |
|--------|-----------------------|
| `min`, `max` | `#ifndef` guard — `<windows.h>` определяет раньше |
| `EXIT_SUCCESS` | `#ifndef` guard — `<stdlib.h>` определяет раньше |
| `WIN32`, `_WIN32` | `#ifndef` guard — compiler/cmdline определяет раньше |

### Проверка: нет ли mixed resolution внутри файлов

Через `clang++ -E -dD` проверили полную цепочку `#define`/`#undef` для `TRUE`, `FALSE`,
`INFINITY` на нескольких файлах (dirt.cpp, collide.cpp, os.cpp, polyrenderstate.cpp, Main.cpp).
Картина одинаковая:

1. `<windows.h>` → `#define FALSE 0`, `#define TRUE 1`
2. `<math.h>` → `#undef INFINITY` + `#define INFINITY (__builtin_inff())`
3. MSVC math header → `#define INFINITY ((float)(_HUGE_ENUF))`
4. Наш `MFStdLib.h` → `#define TRUE ...`, `#define FALSE ...`
5. Наш `MFStdLib.h` → `#undef INFINITY` + `#define INFINITY ...`

**После наших define — никаких переопределений нет.** Системные хедеры идут первыми,
наш `MFStdLib.h` переопределяет последним и остаётся в силе до конца файла.
Ситуации "часть файла резолвится через наш define, часть через стандарт" не происходит.

Нюанс: `os.cpp` (outro) включает `Always.h` раньше `MFStdLib.h`, поэтому `INFINITY`
переопределяется дважды нашим кодом (сначала `Always.h`, потом `MFStdLib.h`), но итог тот же.

---

## `TRUE` — 2049 строк, по файлам

| Кол-во | Файл |
|--------|------|
| 126 | `fallen/DDEngine/Source/polyrenderstate.cpp` |
| 89 | `fallen/DDEngine/Source/aeng.cpp` |
| 62 | `fallen/Source/collide.cpp` |
| 60 | `fallen/Source/pcom.cpp` |
| 48 | `fallen/Source/interfac.cpp` |
| 47 | `fallen/DDEngine/Source/panel.cpp` |
| 45 | `fallen/Source/frontend.cpp` |
| 44 | `fallen/Source/eway.cpp` |
| 43 | `fallen/DDEngine/Source/figure.cpp` |
| 43 | `fallen/DDEngine/Source/engineMap.cpp` |
| 39 | `fallen/Source/Person.cpp` |
| 39 | `fallen/DDLibrary/Source/DDManager.cpp` |
| 36 | `fallen/outro/os.cpp` |
| 36 | `fallen/DDEngine/Source/poly.cpp` |
| 33 | `fallen/Source/Controls.cpp` |
| 33 | `fallen/Source/Attract.cpp` |
| 33 | `fallen/DDEngine/Source/shape.cpp` |
| 29 | `fallen/Source/Game.cpp` |
| 29 | `fallen/DDEngine/Source/drawxtra.cpp` |
| 27 | `fallen/DDEngine/Source/sky.cpp` |
| 27 | `fallen/DDEngine/Source/flamengine.cpp` |
| 26 | `fallen/Source/elev.cpp` |
| 26 | `fallen/Source/Vehicle.cpp` |
| 26 | `fallen/DDEngine/Source/texture.cpp` |
| 26 | `fallen/DDEngine/Source/renderstate.cpp` |
| 25 | `fallen/DDEngine/Source/fastprim.cpp` |
| 24 | `fallen/DDLibrary/Source/GHost.cpp` |
| 24 | `fallen/DDLibrary/Source/DIManager.cpp` |
| 24 | `fallen/DDLibrary/Source/D3DTexture.cpp` |
| 24 | `fallen/DDEngine/Source/truetype.cpp` |
| 24 | `fallen/DDEngine/Source/superfacet.cpp` |
| 24 | `fallen/DDEngine/Source/polypage.cpp` |
| 24 | `fallen/DDEngine/Source/farfacet.cpp` |
| 24 | `fallen/DDEngine/Source/Text.cpp` |
| 23 | `fallen/Source/dirt.cpp` |
| 23 | `fallen/DDLibrary/Source/WindProcs.cpp` |
| 23 | `fallen/DDEngine/Source/font2d.cpp` |
| 22 | `fallen/Source/pause.cpp` |
| 22 | `fallen/Source/overlay.cpp` |
| 22 | `fallen/DDLibrary/Source/GKeyboard.cpp` |
| 22 | `fallen/DDLibrary/Source/GDisplay.cpp` |
| 22 | `fallen/DDEngine/Source/qeng.cpp` |
| 22 | `fallen/DDEngine/Source/oval.cpp` |
| 21 | `fallen/Source/Main.cpp` |
| 21 | `fallen/DDLibrary/Source/net.cpp` |
| 21 | `fallen/DDLibrary/Source/GWorkScreen.cpp` |
| 21 | `fallen/DDLibrary/Source/GMouse.cpp` |
| 21 | `fallen/DDLibrary/Source/FileClump.cpp` |
| 21 | `fallen/DDLibrary/Source/Drive.cpp` |
| 21 | `fallen/DDLibrary/Source/BinkClient.cpp` |
| 21 | `fallen/DDLibrary/Source/AsyncFile2.cpp` |
| 21 | `fallen/DDEngine/Source/wibble.cpp` |
| 21 | `fallen/DDEngine/Source/vertexbuffer.cpp` |
| 21 | `fallen/DDEngine/Source/planmap.cpp` |
| 21 | `fallen/DDEngine/Source/console.cpp` |
| 21 | `fallen/DDEngine/Source/Gamut.cpp` |
| 21 | `fallen/DDEngine/Source/Font.cpp` |
| 21 | `fallen/DDEngine/Source/Bucket.cpp` |
| 18 | `fallen/Source/widget.cpp` |
| 17 | `fallen/DDEngine/Source/facet.cpp` |
| 15 | `fallen/Source/road.cpp` |
| 12 | `fallen/Source/mav.cpp` |
| 10 | `fallen/Source/outline.cpp` |
| 8 | `fallen/Source/fc.cpp` |
| 8 | `fallen/Source/Special.cpp` |
| 7 | `fallen/Source/ware.cpp` |
| 7 | `fallen/Source/save.cpp` |
| 7 | `fallen/Source/ns.cpp` |
| 6 | `fallen/outro/imp.cpp` |
| 6 | `fallen/Source/night.cpp` |
| 6 | `fallen/Source/hm.cpp` |
| 6 | `fallen/DDEngine/Source/menufont.cpp` |
| 6 | `fallen/DDEngine/Source/Crinkle.cpp` |
| 5 | `fallen/Source/ob.cpp` |
| 5 | `fallen/Source/Combat.cpp` |
| 5 | `fallen/DDEngine/Source/Quaternion.cpp` |
| 4 | `fallen/Source/wmove.cpp` |
| 4 | `fallen/Source/cnet.cpp` |
| 4 | `fallen/Source/barrel.cpp` |
| 4 | `fallen/Source/Prim.cpp` |
| 4 | `fallen/DDEngine/Source/sprite.cpp` |
| 4 | `fallen/DDEngine/Source/comp.cpp` |
| 3 | `fallen/outro/credits.cpp` |
| 3 | `fallen/Source/wand.cpp` |
| 3 | `fallen/Source/pap.cpp` |
| 3 | `fallen/Source/Thing.cpp` |
| 3 | `fallen/Source/Darci.cpp` |
| 3 | `fallen/DDEngine/Source/mesh.cpp` |
| 3 | `fallen/DDEngine/Source/cone.cpp` |
| 2 | `fallen/outro/outroTga.cpp` |
| 2 | `fallen/outro/outroFont.cpp` |
| 2 | `fallen/Source/stair.cpp` |
| 2 | `fallen/Source/spark.cpp` |
| 2 | `fallen/Source/qmap.cpp` |
| 2 | `fallen/Source/puddle.cpp` |
| 2 | `fallen/Source/plat.cpp` |
| 2 | `fallen/Source/music.cpp` |
| 2 | `fallen/Source/memory.cpp` |
| 2 | `fallen/Source/io.cpp` |
| 2 | `fallen/Source/guns.cpp` |
| 2 | `fallen/Source/grenade.cpp` |
| 2 | `fallen/Source/bat.cpp` |
| 2 | `fallen/DDLibrary/Source/Tga.cpp` |
| 1 | `fallen/outro/wire.cpp` |
| 1 | `fallen/outro/mf.cpp` |
| 1 | `fallen/outro/back.cpp` |
| 1 | `fallen/Source/supermap.cpp` |
| 1 | `fallen/Source/snipe.cpp` |
| 1 | `fallen/Source/sm.cpp` |
| 1 | `fallen/Source/qedit.cpp` |
| 1 | `fallen/Source/pyro.cpp` |
| 1 | `fallen/Source/psystem.cpp` |
| 1 | `fallen/Source/maths.cpp` |
| 1 | `fallen/Source/gamemenu.cpp` |
| 1 | `fallen/Source/door.cpp` |
| 1 | `fallen/DDLibrary/Source/MFX.cpp` |
| 1 | `fallen/DDEngine/Source/smap.cpp` |
| 1 | `fallen/DDEngine/Source/ic.cpp` |
| 1 | `MFStdLib/Source/StdLib/StdMem.cpp` |
| 1 | `MFStdLib/Source/StdLib/StdFile.cpp` |

---

## `FALSE` — 2636 строк, по файлам

| Кол-во | Файл |
|--------|------|
| 177 | `fallen/DDEngine/Source/polyrenderstate.cpp` |
| 97 | `fallen/Source/frontend.cpp` |
| 89 | `fallen/DDEngine/Source/aeng.cpp` |
| 82 | `fallen/Source/eway.cpp` |
| 67 | `fallen/Source/collide.cpp` |
| 60 | `fallen/DDEngine/Source/panel.cpp` |
| 59 | `fallen/Source/Person.cpp` |
| 58 | `fallen/DDEngine/Source/poly.cpp` |
| 53 | `fallen/Source/interfac.cpp` |
| 49 | `fallen/DDEngine/Source/engineMap.cpp` |
| 47 | `fallen/DDEngine/Source/figure.cpp` |
| 46 | `fallen/DDEngine/Source/drawxtra.cpp` |
| 45 | `fallen/Source/pcom.cpp` |
| 44 | `fallen/DDEngine/Source/renderstate.cpp` |
| 43 | `fallen/Source/Game.cpp` |
| 43 | `fallen/DDEngine/Source/texture.cpp` |
| 42 | `fallen/DDLibrary/Source/DDManager.cpp` |
| 42 | `fallen/DDEngine/Source/shape.cpp` |
| 41 | `fallen/Source/Attract.cpp` |
| 38 | `fallen/Source/elev.cpp` |
| 38 | `fallen/Source/Controls.cpp` |
| 31 | `fallen/DDEngine/Source/sky.cpp` |
| 30 | `fallen/Source/pause.cpp` |
| 30 | `fallen/Source/dirt.cpp` |
| 30 | `fallen/Source/Vehicle.cpp` |
| 30 | `fallen/DDEngine/Source/fastprim.cpp` |
| 29 | `fallen/DDEngine/Source/superfacet.cpp` |
| 29 | `fallen/DDEngine/Source/flamengine.cpp` |
| 29 | `fallen/DDEngine/Source/console.cpp` |
| 28 | `fallen/DDLibrary/Source/GHost.cpp` |
| 28 | `fallen/DDEngine/Source/font2d.cpp` |
| 27 | `fallen/Source/road.cpp` |
| 27 | `fallen/Source/overlay.cpp` |
| 27 | `fallen/DDEngine/Source/truetype.cpp` |
| 27 | `fallen/DDEngine/Source/farfacet.cpp` |
| 27 | `fallen/DDEngine/Source/Text.cpp` |
| 25 | `fallen/outro/os.cpp` |
| 25 | `fallen/DDLibrary/Source/GDisplay.cpp` |
| 25 | `fallen/DDEngine/Source/qeng.cpp` |
| 24 | `fallen/DDEngine/Source/polypage.cpp` |
| 24 | `fallen/DDEngine/Source/oval.cpp` |
| 23 | `fallen/DDLibrary/Source/DIManager.cpp` |
| 22 | `fallen/DDEngine/Source/vertexbuffer.cpp` |
| 21 | `fallen/DDLibrary/Source/net.cpp` |
| 20 | `fallen/Source/Main.cpp` |
| 20 | `fallen/DDLibrary/Source/WindProcs.cpp` |
| 20 | `fallen/DDLibrary/Source/D3DTexture.cpp` |
| 19 | `fallen/DDLibrary/Source/GMouse.cpp` |
| 19 | `fallen/DDLibrary/Source/GKeyboard.cpp` |
| 19 | `fallen/DDLibrary/Source/AsyncFile2.cpp` |
| 19 | `fallen/DDEngine/Source/wibble.cpp` |
| 19 | `fallen/DDEngine/Source/planmap.cpp` |
| 19 | `fallen/DDEngine/Source/Gamut.cpp` |
| 19 | `fallen/DDEngine/Source/Bucket.cpp` |
| 18 | `fallen/Source/fc.cpp` |
| 18 | `fallen/DDLibrary/Source/GWorkScreen.cpp` |
| 18 | `fallen/DDLibrary/Source/FileClump.cpp` |
| 18 | `fallen/DDLibrary/Source/Drive.cpp` |
| 18 | `fallen/DDLibrary/Source/BinkClient.cpp` |
| 18 | `fallen/DDEngine/Source/Font.cpp` |
| 17 | `fallen/Source/widget.cpp` |
| 17 | `fallen/Source/qedit.cpp` |
| 16 | `fallen/Source/mav.cpp` |
| 16 | `fallen/Source/bat.cpp` |
| 15 | `fallen/DDEngine/Source/facet.cpp` |
| 13 | `fallen/Source/playcuts.cpp` |
| 13 | `fallen/Source/maths.cpp` |
| 12 | `fallen/Source/hm.cpp` |
| 12 | `fallen/Source/gamemenu.cpp` |
| 12 | `fallen/Source/Combat.cpp` |
| 11 | `fallen/Source/save.cpp` |
| 10 | `fallen/Source/memory.cpp` |
| 10 | `fallen/Source/cnet.cpp` |
| 10 | `fallen/DDEngine/Source/menufont.cpp` |
| 10 | `fallen/DDEngine/Source/cone.cpp` |
| 9 | `fallen/Source/plat.cpp` |
| 9 | `fallen/Source/outline.cpp` |
| 9 | `fallen/Source/ns.cpp` |
| 9 | `fallen/Source/night.cpp` |
| 8 | `fallen/Source/ob.cpp` |
| 8 | `fallen/Source/Special.cpp` |
| 7 | `fallen/outro/imp.cpp` |
| 7 | `fallen/Source/pyro.cpp` |
| 7 | `fallen/Source/psystem.cpp` |
| 7 | `fallen/Source/barrel.cpp` |
| 7 | `fallen/DDEngine/Source/sprite.cpp` |
| 7 | `fallen/DDEngine/Source/mesh.cpp` |
| 6 | `fallen/Source/supermap.cpp` |
| 6 | `fallen/Source/puddle.cpp` |
| 6 | `fallen/Source/grenade.cpp` |
| 6 | `fallen/Source/animal.cpp` |
| 5 | `fallen/Source/tracks.cpp` |
| 5 | `fallen/Source/drip.cpp` |
| 5 | `fallen/DDEngine/Source/comp.cpp` |
| 5 | `fallen/DDEngine/Source/build.cpp` |
| 5 | `fallen/DDEngine/Source/Crinkle.cpp` |
| 4 | `fallen/outro/outroTga.cpp` |
| 4 | `fallen/outro/outroFont.cpp` |
| 4 | `fallen/Source/ware.cpp` |
| 4 | `fallen/Source/pap.cpp` |
| 4 | `fallen/Source/io.cpp` |
| 4 | `fallen/Source/hook.cpp` |
| 4 | `fallen/Source/Thing.cpp` |
| 4 | `fallen/Source/Prim.cpp` |
| 4 | `fallen/Source/Darci.cpp` |
| 4 | `fallen/DDLibrary/Source/MFX.cpp` |
| 4 | `fallen/DDEngine/Source/Quaternion.cpp` |
| 3 | `fallen/Source/stair.cpp` |
| 3 | `fallen/Source/spark.cpp` |
| 3 | `fallen/Source/dike.cpp` |
| 3 | `fallen/Source/Sound.cpp` |
| 3 | `MFStdLib/Source/StdLib/StdFile.cpp` |
| 2 | `fallen/outro/mf.cpp` |
| 2 | `fallen/outro/credits.cpp` |
| 2 | `fallen/Source/wand.cpp` |
| 2 | `fallen/Source/snipe.cpp` |
| 2 | `fallen/Source/qmap.cpp` |
| 2 | `fallen/Source/pow.cpp` |
| 2 | `fallen/Source/music.cpp` |
| 2 | `fallen/Source/door.cpp` |
| 2 | `fallen/DDEngine/Source/smap.cpp` |
| 1 | `fallen/outro/wire.cpp` |
| 1 | `fallen/outro/outroMain.cpp` |
| 1 | `fallen/outro/cam.cpp` |
| 1 | `fallen/outro/back.cpp` |
| 1 | `fallen/Source/wmove.cpp` |
| 1 | `fallen/Source/walkable.cpp` |
| 1 | `fallen/Source/trip.cpp` |
| 1 | `fallen/Source/soundenv.cpp` |
| 1 | `fallen/Source/sm.cpp` |
| 1 | `fallen/Source/shadow.cpp` |
| 1 | `fallen/Source/ribbon.cpp` |
| 1 | `fallen/Source/morph.cpp` |
| 1 | `fallen/Source/mist.cpp` |
| 1 | `fallen/Source/interact.cpp` |
| 1 | `fallen/Source/inside2.cpp` |
| 1 | `fallen/Source/heap.cpp` |
| 1 | `fallen/Source/guns.cpp` |
| 1 | `fallen/Source/glitter.cpp` |
| 1 | `fallen/Source/fog.cpp` |
| 1 | `fallen/Source/fire.cpp` |
| 1 | `fallen/Source/env2.cpp` |
| 1 | `fallen/Source/drawtype.cpp` |
| 1 | `fallen/Source/chopper.cpp` |
| 1 | `fallen/Source/canid.cpp` |
| 1 | `fallen/Source/build2.cpp` |
| 1 | `fallen/Source/balloon.cpp` |
| 1 | `fallen/Source/animtmap.cpp` |
| 1 | `fallen/Source/Thug.cpp` |
| 1 | `fallen/Source/Switch.cpp` |
| 1 | `fallen/Source/State.cpp` |
| 1 | `fallen/Source/Roper.cpp` |
| 1 | `fallen/Source/Player.cpp` |
| 1 | `fallen/Source/Pjectile.cpp` |
| 1 | `fallen/Source/Map.cpp` |
| 1 | `fallen/Source/Hierarchy.cpp` |
| 1 | `fallen/Source/FMatrix.cpp` |
| 1 | `fallen/Source/Enemy.cpp` |
| 1 | `fallen/Source/Effect.cpp` |
| 1 | `fallen/Source/Cop.cpp` |
| 1 | `fallen/Source/Building.cpp` |
| 1 | `fallen/Source/Anim.cpp` |
| 1 | `fallen/DDEngine/Source/ic.cpp` |
| 1 | `MFStdLib/Source/StdLib/StdMem.cpp` |

---

## `INFINITY` — 150 строк, по файлам

| Кол-во | Файл |
|--------|------|
| 17 | `fallen/Source/Prim.cpp` |
| 9 | `fallen/Source/ob.cpp` |
| 9 | `fallen/DDEngine/Source/NGamut.cpp` |
| 8 | `fallen/DDEngine/Source/aeng.cpp` |
| 7 | `fallen/Source/pcom.cpp` |
| 6 | `fallen/outro/imp.cpp` |
| 6 | `fallen/Source/hm.cpp` |
| 6 | `fallen/Source/collide.cpp` |
| 6 | `fallen/DDEngine/Source/smap.cpp` |
| 6 | `fallen/DDEngine/Source/Crinkle.cpp` |
| 5 | `fallen/Source/dirt.cpp` |
| 5 | `fallen/Source/Combat.cpp` |
| 4 | `fallen/Source/ware.cpp` |
| 4 | `fallen/Source/barrel.cpp` |
| 4 | `fallen/DDEngine/Source/mesh.cpp` |
| 4 | `fallen/DDEngine/Source/figure.cpp` |
| 4 | `fallen/DDEngine/Source/farfacet.cpp` |
| 4 | `fallen/DDEngine/Source/aa.cpp` |
| 3 | `fallen/Source/wand.cpp` |
| 3 | `fallen/Source/pap.cpp` |
| 3 | `fallen/Source/eway.cpp` |
| 3 | `fallen/DDEngine/Source/ic.cpp` |
| 3 | `fallen/DDEngine/Source/facet.cpp` |
| 2 | `fallen/Source/stair.cpp` |
| 2 | `fallen/Source/plat.cpp` |
| 2 | `fallen/Source/Person.cpp` |
| 2 | `fallen/DDEngine/Source/engineMap.cpp` |
| 2 | `fallen/DDEngine/Source/comp.cpp` |
| 1 | `fallen/Source/supermap.cpp` |
| 1 | `fallen/Source/road.cpp` |
| 1 | `fallen/Source/mav.cpp` |
| 1 | `fallen/Source/fc.cpp` |
| 1 | `fallen/Source/elev.cpp` |
| 1 | `fallen/Source/door.cpp` |
| 1 | `fallen/Source/bat.cpp` |
| 1 | `fallen/Source/Controls.cpp` |
| 1 | `fallen/DDEngine/Source/panel.cpp` |
| 1 | `fallen/DDEngine/Source/font2d.cpp` |
| 1 | `fallen/DDEngine/Source/Message.cpp` |
