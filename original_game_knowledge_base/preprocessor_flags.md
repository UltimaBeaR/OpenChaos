# Флаги препроцессора — Urban Chaos (original_game)

Полная классификация `#define` / `#ifdef` / `#if defined` в кодовой базе оригинальной игры.

---

## Конфигурации сборки (Fallen.vcxproj)

### Release build:
```
NDEBUG;_RELEASE;WIN32;_WINDOWS;VERSION_D3D;TEX_EMBED;FINAL
```

### Debug build:
```
_WINDOWS;WIN32;_DEBUG;DEBUG;VERSION_D3D;TEX_EMBED;WINDOWS_IGNORE_PACKING_MISMATCH
```

`TEX_EMBED` активен в **обеих** конфигурациях. `FINAL` — только в Release.

---

## 1. Платформенные флаги

| Флаг | Где определяется | Назначение | Масштаб | Активен (PC) |
|------|-----------------|-----------|---------|--------------|
| `TARGET_DC` | Настройки проекта | Dreamcast (Sega). Полный порт: file I/O, рендеринг, ввод, ограничения памяти | ~1054 вхождений, ~165 файлов | нет |
| `PSX` | Проект/headers | PlayStation 1. Полный порт с отдельным рендерером (GTE, fixed-point math) | ~1739 вхождений, ~178 файлов | нет |
| `_MF_WINDOWS` | MFStdLib.h; автоматически при `!TARGET_DC` | Windows platform layer: HANDLE для файлов, DirectDraw глобалы | ~36 файлов | **да** |
| `_MF_DOSX` | MFTypes.h (при `__DOS__`) | DOS extended mode: BOOL typedef, DOS file includes | 6 файлов | нет (мёртв) |
| `__WATCOMC__` | Compiler predefined | Watcom C++: abs() fix, Root() asm, warning suppress | 8 файлов | нет (компилятор устарел) |
| `__DOS__` | Compiler predefined | DOS: устанавливает `_MF_DOSX`, calling conventions для MIDAS audio | 3 файла | нет (мёртв) |
| `__WINDOWS_386__` | Compiler predefined | Watcom Windows: устанавливает `_MF_WINDOWS`, MUL64/DIV64 asm | 5 файлов | нет (компилятор устарел) |
| `_WINDOWS` | Проектные файлы, .rc | Generic Windows; в основном ресурсные файлы | ~38 файлов | **да** |
| `_WIN32` | .rc, .dsp, MFStdLib.h | Windows 32-bit; автоматически форсится в MFStdLib.h | ~12 файлов | **да** |

**Цепочка:** `__WINDOWS_386__` / `WIN32` → `_MF_WINDOWS` → `_WIN32` (форсится в MFStdLib.h).

**TARGET_DC** (1054 вхождения) — полноценный порт:
- File I/O: `_wfopen()` вместо `fopen()` с workaround утечки памяти (StdFile.cpp)
- Командная строка: симуляция argv[1] (MFHost.cpp) — DC не поддерживает аргументы
- Палитра: пропуск DirectDraw palette init (Palette.cpp)
- Сеть: `dplayx.lib` вместо стандартных dplay/d3drm (MFLbType.h)
- Клавиатура: AltFlag=0 (StdKeybd.h) — нет alt на контроллере
- Цензура: Game.cpp — французская версия: насилие разрешено на DC, запрещено на PC

**PSX** (1739 вхождений) — параллельный рендерер, не адаптация PC:
- Отдельная папка PSXENG/ с полным рендерером на GTE (hardware transform)
- Fixed-point math vs float: SinTable, AtanTable с 2048-angle системой
- Отдельные реализации фигур: bone-based skinning для GTE vs quaternion на PC
- MAX_NUMBER_OF_CHUNKS = 5 (PSX) vs больше (PC) — ограничение памяти
- Отдельный level loader (Levelpsx.cpp)

---

## 2. Графические флаги

| Флаг | Определение | Назначение | Активен (PC) |
|------|------------|-----------|--------------|
| `VERSION_D3D` | vcxproj (оба конфига) | Основной рендерер: структура Camera, include aeng.h | **да** |
| `VERSION_GLIDE` | Game.h (закомментирован) | 3dfx Glide рендерер. Отдельная папка `/Glide Engine/` | нет (мёртв) |
| `MF_DD2` | Display.cpp (`#define MF_DD2`) | DirectDraw 2 интерфейс vs DirectDraw 1 | **да** |
| `TEX_EMBED` | vcxproj (оба конфига) | Встроенные текстуры: `TEXTURE_get_D3DTexture()` — прямой указатель на D3DTexture | **да** |

**DONT_IGNORE_* флаги** (SHADOWS, PUDDLES, REFLECTIONS, STARS, FANCY_STUFF): находятся **только** в `Glide Engine/glaeng.cpp`, нигде не определены. Вырезанные визуальные фичи Glide-рендерера (тени ~337 строк, звёзды, отражения, лужи, частицы). Для D3D рендерера не существуют.

**ALPHA_* как система code generation:** `ALPHA_MODE`, `ALPHA_ADD`, `ALPHA_BLEND`, `ALPHA_TEST` — не feature flags, а система **множественного включения** tom.cpp:

```c
// sw.cpp — три включения tom.cpp с разными значениями ALPHA_MODE:
SW_draw_span_masked(...)  → #define ALPHA_MODE ALPHA_TEST;  #include "tom.cpp"
SW_draw_span_alpha(...)   → #define ALPHA_MODE ALPHA_BLEND; #include "tom.cpp"
SW_draw_span_additive(...)→ #define ALPHA_MODE ALPHA_ADD;   #include "tom.cpp"
```

tom.cpp включается **3 раза**, генерируя 3 версии функции рисования. Compile-time полиморфизм.
`ALPHA_BLEND_NOT_DOUBLE_LIGHTING = 1` — контроль яркости альфа: `shl ecx,23` vs `shl ecx,23-1`.

---

## 3. Флаги фич

| Флаг | Масштаб | Назначение | Определён? | Активен (PC) |
|------|---------|-----------|-----------|--------------|
| `BIKE` | 453 вхождения, 38 файлов | Полная система мотоцикла: физика, рендеринг, управление, анимации | нет — не в vcxproj, нет `#define` | нет |
| `FAST_EDDIE` | 8 вхождений, 3 файла | Dev-mode: checkerboard culling (KB_1), 2x speed (KB_T), debug keys в release | нет — не в vcxproj | нет |
| `USE_PASSWORD` | 3 вхождения, 1 файл | Гейт стартового экрана: mode=0 (пароль) vs mode=1 (пропуск) | нет — не в vcxproj → mode=1 | нет |
| `SEWERS` | ~111 вхождений, ~24 файла | Редактор канализаций + рендеринг в editor mode. AENG_draw_sewer_editor() | нет — не в vcxproj | нет |
| `TRUETYPE` | 9 вхождений, 4 файла | TrueType шрифты vs bitmap. Закомментирован в truetype.h | нет | нет |
| `NO_SERVER` | noserver.h: `= 1` (всегда) | Standalone режим. Переключает файловые пути (относительные vs серверные) | **да** — всегда 1 | **да** (структурный) |
| `OLD_CAM` / `CAM_OLD` | ~16 вхождений, 7 файлов | Мёртвая камера из cam.cpp (заменена на fc.cpp — Final Camera) | нет | нет |

**BIKE** (453 вхождения, 38 файлов): код полный и функциональный:
- Структура BIKE_Bike: подвеска (wheel_y_front/back), руление, acceleration, brake
- Визуал: tyre marks, body part transforms. Лимит: `BIKE_MAX_BIKES = 2`
- Состояния: PARKED → MOUNTING → DRIVING → DISMOUNTING
- Ключевые файлы: bike.cpp (~1000 строк физики), Person.cpp, Controls.cpp, aeng.cpp (145+ строк рендеринга), collide.cpp, fc.cpp (камера при езде)
- **Не в vcxproj, нигде нет `#define BIKE`.** Код никогда не компилируется.

**SEWERS vs SEWERS_EXIST:** важное различие:
- `SEWERS` — флаг для **кода редактора** (AENG_draw_sewer_editor(), SeEdit). Не defined в game build.
- `SEWERS_EXIST` — флаг в **PSXENG** для условного рисования лестниц канализации (facet.cpp). PSX-only.
- Сама система канализаций (sewer.cpp/h) **компилируется без флага** в основном билде.

**NO_SERVER:** **структурный флаг**, всегда = 1 через noserver.h. Управляет путями к ресурсам (relative vs server-based), не мультиплеером.

---

## 4. Отладочные флаги

| Флаг | Где определяется | Назначение | Активен (PC) |
|------|-----------------|-----------|--------------|
| `DEBUG` | vcxproj (Debug) | Game-level debug: оверлеи, статистика, отладочный вывод | Debug only |
| `_DEBUG` | MSVC predefined (Debug) | Compiler-level: ASSERT, memory tracking, debug symbols | Debug only |
| `NDEBUG` | vcxproj (Release) | Отключает ASSERT и debug output | Release only |
| `_RELEASE` | vcxproj (Release) | Явная метка release build | Release only |
| `FINAL` | vcxproj (Release only) | Самый строгий: отключает debug keys, console, debug rendering | Release only |
| `FACET_REMOVAL_TEST` | build2.cpp (`#ifdef _DEBUG`) | Авто-включается в _DEBUG. Рисует невидимые фасеты красным, F для переключения | Debug only |
| `HEAP_DEBUGGING_PLEASE_BOB` | StdMem.cpp (закомментирован) | Heap profiling: обёртка аллокаций метаданными | нет |
| `DEBUG_POOSHIT` | Editor/Engine.cpp | Debug визуализация зданий в редакторе; 1 вхождение | нет |
| `TEST_3DFX` | Editor/Engine.cpp, LevelEd.cpp | Тест 3dfx в редакторе; внутри #ifdef DOGPOO (никогда не defined) | нет |

**Иерархия debug:**
```
Release:  NDEBUG + _RELEASE + FINAL → всё отключено
Debug:    _DEBUG + DEBUG → ASSERT, debug output, debug keys, FACET_REMOVAL_TEST,
                            MESH_SHOW_MOUSE_POINT, SHOW_ME_FIGURE_DEBUGGING_PLEASE_BOB
```

**⚠️ Баг в vcxproj:** Release конфиг использует `MultiThreadedDebug` (`/MTd`) вместо `MultiThreaded` (`/MT`) — это автоматически определяет `_DEBUG`, из-за чего ВСЕ `#ifdef _DEBUG` блоки активны в Release. Отладочные надписи ("music mode %d vol %f %s" в panel.cpp) показываются в Release. 103 строки `/NODEFAULTLIB:libcmtd.lib` — костыли для подавления конфликта линковки из-за этой ошибки. Артефакт кривой конвертации `.dsp` → `.vcxproj`.

**FINAL** — самый строгий уровень. Гейтит: debug key overrides (Crinkle.cpp, figure.cpp), TCP/IP console, debug rendering modes.

---

## 5. Флаги редактора

| Флаг | Файлы | Назначение |
|------|-------|-----------|
| `EDITOR` | ~30 файлов | Level Editor. Глубоко интегрирован: Building.cpp, collide.cpp, Anim.cpp, poly.cpp |
| `BUILD_PSX` | ~17 файлов | PSX build дифференциация. Определяется ТОЛЬКО внутри `#ifdef PSX` в Game.h → на PC никогда не определён. `#ifndef BUILD_PSX` блоки (memory.cpp, Person.cpp) всегда активны на PC |
| `DOG_POO` | 7 файлов | Мёртвая камера cam.cpp (весь файл в `#ifdef DOG_POO`) + debug коллизий |
| `POO` | 9 файлов | Debug визуализация граней зданий (линии) |
| `FUNNY_FANNY` | 2 файла | Код зданий в редакторе |
| `OLD_STUFF`, `GUY`, `I_AM_A_LOON`, `DOGPOO`, `_MF_WINDOWS_DOG_POO` | 1-5 файлов | Личные debug/joke флаги разработчиков |

Ни один из них не определён в game build.

---

## 6. Региональные флаги

Все определяются в `password.h`. Система DRM/дистрибуции для издателей. **Активен в исходниках:** только `EIDOS` (Eidos Interactive UK). Остальные закомментированы.

Каждый флаг устанавливает `EXPORT_NAME`, `EXPORT_CO`, `EXPORT_PW` (ROT13-закодированные) + `MAGIC_KEY` (4-байтовый ключ дешифрования):
```c
#define EXPORT_NAME  "Ecqv`h'@lnli~"     // Darren Hedges (ROT13)
#define EXPORT_CO    "Dkgkv&RC"           // Eidos UK
#define EXPORT_PW    "uulcjjcn`yc"        // twogoldfish
#define MAGIC_KEY    99, 98, 97, 96
```
`MAGIC_ARRAY = { 1, 2, 3, 42, MAGIC_KEY }` — 8-байтовый массив для дешифрования.

Список: `EIDOS`, `USA`, `GERMANY`, `FRANCE`, `KK` (Japan), `SINGAPORE`, `LEADER` (Italy), `HALIFAX`, `SYNTHESIS`, `DLMM`, `EEM`, `EUROPE`, `JAPAN`, `TDFX` (3dfx Voodoo version).

**VERSION_* PSX:** цепочка определений в psxeng.h. Каждая версия диска (VERSION_DPC, VERSION_RCD и т.д.) устанавливает язык, видеорежим (PAL/NTSC), файловую систему.

---

## 7. Активные флаги-константы (в исходниках, не в vcxproj)

| Флаг | Определение | Значение | Назначение |
|------|------------|---------|-----------|
| `USE_TOMS_ENGINE_PLEASE_BOB` | aeng.h, Prim.h | `1` (PC и DC) | Core D3D рендерер: матрицы, viewport, z-buffer, render states |
| `WE_NEED_POLYBUFFERS_PLEASE_BOB` | polypage.h | `1` (PC), `0` (DC) | Z-сортировка полигонов через буфер. DC: аппаратная |
| `LIGHT_COLOURED` | light.h | `1` | RGB освещение (3 байта) vs greyscale (1 байт) |
| `USE_D3D_VBUF` | vertexbuffer.h | `1` (PC), `0` (DC) | D3D vertex buffers. "33% faster" |
| `DRAW_WHOLE_PERSON_AT_ONCE` | figure.cpp | `1` | Batch-рендеринг 15 частей тела одним D3D MultiMatrix вызовом |
| `ANTIALIAS_BY_HAND` | truetype.cpp | `1` | 2x supersampling для TrueType шрифтов |
| `POLY_FADEOUT_START` | poly.h, Night.h | `0.60F` (PC) | Дистанция начала fog fade-out |
| `SUPERCRINKLES_ENABLED` | supercrinkle.h | `0` | Dynamic surface deformation. Отключён, stubbed out |
| `CALC_CAR_CRUMPLE` | mesh.cpp | `0` | Динамический расчёт crumple-зон. Отключён, используется таблица (~190 строк) |
| `DISABLE_CRINKLES` | Crinkle.cpp | DC=`1`, PC=`0` | DC: отключены (RAM). **PC: crinkles включены** (до 8192 точек) |
| `NO_BACKFACE_CULL_PLEASE_BOB` | poly.cpp | `0` (всегда) | Мёртвый эксперимент: определён но **никогда не проверяется** через #if |
| `ULTRA_COMPRESSED_ANIMATIONS` | Anim.h | PSX only | Сжатый формат матриц анимации. На PC не активен |
| `NEW_FLOOR` | aeng.cpp | DC only | Оптимизированный рендеринг полов. На PC закомментирован |

**Цепочка горячего пути рендеринга на PC:**
1. `USE_TOMS_ENGINE_PLEASE_BOB` (=1) → матрицы проекции, viewport, z-buffer
2. `DRAW_WHOLE_PERSON_AT_ONCE` (=1) → batch 15 body parts в один D3D MultiMatrix call
3. `WE_NEED_POLYBUFFERS_PLEASE_BOB` (=1) → Z-сортировка для transparency
4. `USE_D3D_VBUF` (=1) → D3D vertex buffers (+33% fps)

**DRAW_WHOLE_PERSON_AT_ONCE** — использует D3D MultiMatrix extension (DrawIndPrimMM). Комментарий в коде: "NOT portable — relies on D3D MultiMatrix extension".

---

## 8. DC-специфичные флаги

Флаги, **определённые только внутри #ifdef TARGET_DC**:

| Флаг | Значение (DC) | Значение (PC) | Назначение |
|------|-------------|--------------|-----------|
| `BODGE_MY_PANELS_PLEASE_BOB` | `defined` | не определён | Z-depth hack для панелей (DEPTH_BODGE_START=0.95f) |
| `NO_CLIPPING_TO_THE_SIDES_PLEASE_BOB` | `1` | `0` | DC: hardware clip; PC: software clip. Обе ветви рабочие |
| `LIMIT_TOTAL_PYRO_SPRITES_PLEASE_BOB` | `500` | не определён | Лимит спрайтов взрывов. PC: "Turned off. You madmen :-)" |
| `ENABLE_REMAPPING` | `1` | `0` | Remapping кнопок контроллера (DIManager.h) |
| `LOOK_FOR_START_NOT_JUST_ANY_BUTTON` | `1` | не применим | Требование Sega: ждать START (DIManager.cpp) |

---

## 9. Не feature flags

Символы в `#ifdef` которые не являются настраиваемыми флагами сборки:

| Символ | Что это |
|--------|---------|
| `COLLIDE_GAME` | Include guard collide.h |
| `ANIMATE` | Include guard animate.h |
| `SEARCHING` | Локализованная строка "Поиск идёт..." в psxeng.h (нем/фр/исп/ит) |
| `AUTO_SELECT` | Числовая константа (=4) в Loader.cpp |
| `SWIZZLE` | Локальный define (=1) в sw.cpp/tom.cpp для PSX текстур |
| `THIS_IS_INCLUDED_FROM_SW` | Include guard для tom.cpp (включается из sw.cpp) |
| `VERIFY` | Assertion макрос: Debug → ASSERT(x), Release → x |
| `FILTERING_ON` | Закомментирован в Editor/Poly.cpp; bilinear filtering |
| `PI`, `MAX3`, `_MSC_VER`, `LEVEL_LOST`, `STATE_DEF`, `VERSION`, `NORMAL` | Math константы, compiler detection, enum values, guards |
| `_mfx_h_`, `_sound_id_h_`, `_SewerTab_HPP_` | Нестандартные include guards |

---

## 10. Прочие флаги (мёртвый код / debug / не определены)

**Отключённые оптимизации** (закомментированы, но код полностью функционален):
- `DO_SUPERFACETS_PLEASE_BOB` (facet.cpp) — кэш фасетов. Комментарий: "Can't do these for release yet"
- `MIKES_UNUSED_AUTOMATIC_FLOOR_TEXTURE_GROUPER` (aeng.cpp) — 100+ строк группирования текстур пола
- `WE_WANT_WIND` (Controls.cpp:3297-3452) — авто-ветер + `DIRT_gale()`. Три уровня: auto → manual → blizzard. ~155 строк
- `BOGUS_TGAS_PLEASE_BOB` (Tga.cpp) — генератор фейковых 16x16 текстур по хешу имени
- `USE_W_FOG_PLEASE_BOB`, `QUICK_FACET`, `DOWNSAMPLE_PLEASE_BOB_AMOUNT`

**Debug визуализации** (не определены): `WE_WANT_TO_DRAW_THESE_FACET_LINES`, `WE_WANT_TO_DRAW_THE_TEXTURE_SHADOW_PAGE`, `DRAW_BLACK_FACETS`, `DRAW_FLOOR_FURTHER`, `DEBUG_POLY`, `DEBUG_SPAN`, `DRAW_THIS_DEBUG_STUFF`, `STRIP_STATS`, `SUPERFACET_PERFORMANCE`, `FASTPRIM_PERFORMANCE`, `FEEDBACK`, `BREAKTIMER`

**Debug-only** (авто-включаются в Debug): `MESH_SHOW_MOUSE_POINT` (при `!NDEBUG`), `SHOW_ME_FIGURE_DEBUGGING_PLEASE_BOB` (defined в figure.cpp, за `#ifdef DEBUG`), `SHOW_ME_SUPERFACET_DEBUGGING_PLEASE_BOB` (defined в superfacet.cpp)

**Мёртвый геймплей**: `DARCI_HITS_COPS` (используется как `#if` без `#define` → всегда 0), `NO_MORE_BALLOONS`/`NO_MORE_BALLOONS_NOW`, `WE_WANT_WIND`/`WE_WANT_MANUAL_WIND_ASWELL`/`WE_WANT_SHITTY_PANTS_WIND`, `UNUSED_WIRECUTTERS`/`UNUSED_WIRE_CUTTERS`

**Старые алгоритмы** (PSXENG, заменены): `OLD_FACET_CLIP`, `OLD_FLIP`, `OLD_POO`, `OLD_SPLIT`, `CUNNING_SORT`, `GOOD_SORT`, `BACK_CULL_MAGIC`

**PSX/DC crossplatform debug**: `PSX_COMPRESS_LIGHT`, `DODGYPSXIFY`, `PSX_SIMULATION`, `FILE_PC` (DC: PC vs CD-ROM file serving), `_DEBUG_POO` (math debug в Quaternion.cpp)

**Developer debug**: `MARKS_PRIVATE_VERSION` (отключает gun-out камеру в fc.cpp), `HIGH_REZ_PEOPLE_PLEASE_BOB` (4x вершин + cheat 0x10f1a7e "inflate"), `MAKE_THEM_FACE_THE_CAMERA` (billboard спрайтов), `MAD_AM_I`, `ARGH`, `MIKE`, `DTRACE`

**Developer joke flags** (в основном в PSXENG, стиль Mike Diskett):
- `GONNA_FIREBOMB_YOUR_ASS` — DrawXtra.cpp: рисование спрайта firebomb
- `WHAT_THE_FUCK_IS_THIS_DOING_HERE` — poly.cpp: старая POLY_transform_old()
- `OLDSHIT` — engine.cpp: старая check_prim_ptr_ni()
- `WERE_GOING_TO_STUPIDLY_STICK_THE_FINAL_BANE_INSIDE` — engine.cpp: glow-эффект босса Bane
- `WEVE_REPLACED_THE_HEARTBEAT_WITH_A_SCANNER` — panel.cpp: замена heartbeat на scanner UI
- `GOTTA_DO_A_BETTA_JOB`, `ONE_DAY`, `DONE_ON_PC_NOW`, `POO_SHIT_GOD_DAMN`, `OLD_DOG_POO_OF_A_SYSTEM_OR_IS_IT`

**Прочие**: `USE_A3D` (определён в Sound.h, `#define USE_A3D`; Aureal 3D audio — компания обанкротилась в 2000), `A3D_SOUND` (A3DManager.cpp, условная инициализация/очистка A3D), `DONT_WORRY_ABOUT_INSIDES_FOR_NOW` (Glide Engine stub), `NO_TRANSFORM` (Editor), `TOPMAP_BACK` (не найден в коде), `WHEN_DO_I_WANT_TO_TWO_PASS` (PSXENG), `WE_WANT_A_WHITE_SHADOW`, `WE_WANT_TO_DARKEN_PEOPLE_IN_SHADOW_ABRUPTLY`

---

## 11. Уточнения по неочевидным флагам

| Флаг | Что это |
|------|---------|
| `COLLIDE_GAME` | Include guard collide.h. Не функциональный флаг. |
| `INSIDES_EXIST` | PSX engine (PSXENG/SOURCE/engine.cpp). Условное включение inside2.h и функций draw_insides(), set_inside_texture(). |
| `SEWERS_EXIST` | PSX engine (PSXENG/SOURCE/facet.cpp). Условное рисование FACET_draw_ns_ladder(). |
| `FILE_PC` | DC development flag (Main.cpp, Drive.cpp). На DC: `FILE_PC` → загрузка с PC (`\\PC\\Fallen\\`), иначе с CD-ROM. |
| `DONT_IGNORE_*` | Только Glide Engine (glaeng.cpp). Нигде не определены. Вырезанные визуальные фичи Glide-рендерера. |
