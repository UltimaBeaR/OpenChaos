# Флаги препроцессора — Urban Chaos (original_game)

Полная классификация всех `#define` / `#ifdef` / `#if defined` в кодовой базе.
Цель: понять что нужно для финальной PC-версии, что можно безопасно удалить при переписывании.

---

## 1. Платформенные флаги

| Флаг | Где определяется | Назначение | Нужен для PC |
|------|-----------------|-----------|--------------|
| `TARGET_DC` | Настройки проекта / headers | Dreamcast (Sega) | **НЕТ** |
| `PSX` | collide.h, Command.h, dirt.h, building.h | PlayStation 1 | **НЕТ** |
| `_MF_WINDOWS` | MFStdLib.h, MFHeader.h | Windows — определяется при `!defined(TARGET_DC)` | **ДА** |
| `_MF_DOSX` | MFTypes.h | DOS (legacy) | **НЕТ** |
| `__WATCOMC__` | MFMaths.h, MFUtils.h | Watcom C++ compiler (старый) | **НЕТ** |
| `__DOS__` | MFLbType.h | DOS platform | **НЕТ** |
| `__WINDOWS_386__` | MFLbType.h, collide.cpp | Watcom Windows 386 | **НЕТ** |
| `_WINDOWS` | Проектные файлы | Generic Windows build | **ДА** |
| `_WIN32` | .rc, .dsp файлы | Windows 32-bit | **ДА** |

**Важно:** `TARGET_DC` встречается в базовых библиотеках (MFLib1, MFStdLib, DDEngine) — при удалении нужна аккуратность. `PSX` встречается в физике и структурах данных — тоже осторожно.

---

## 2. Графические флаги

| Флаг | Где определяется | Назначение | Нужен для PC |
|------|-----------------|-----------|--------------|
| `VERSION_D3D` | Fallen.vcxproj (Release + Debug) | DirectX 3D — основной графический API | **ДА** (сейчас) |
| `VERSION_GLIDE` | Engine.h (условие) | 3dfx Glide | **НЕТ** |
| `MF_DD2` | MFLib1/Source/C/Windows/Display.cpp | DirectDraw 2 (обёртка) | **ДА** (сейчас) |
| `TEX_EMBED` | Fallen.vcxproj (Release + Debug) | Встраивание текстур; используется в D3DTexture.cpp, polypage.cpp, figure.cpp | **ДА** |
| `DONT_IGNORE_SHADOWS` и др. | Glide Engine/glaeng.cpp | Графические эффекты Glide | **НЕТ** |
| `WORRY_ABOUT_THIS_LATER` | glpoly.cpp | Отложенная задача в Glide | **НЕТ** |

**Примечание:** Весь код Glide (`/fallen/Glide Engine/`) мёртв — `VERSION_GLIDE` не определяется ни в одной конфигурации.

---

## 3. Флаги фич

| Флаг | Где встречается | Назначение | Нужен для PC |
|------|----------------|-----------|--------------|
| `BIKE` | ~15 файлов: bike.cpp, Controls.cpp, Person.cpp и др. | Мотоцикл — незавершённая фича, в финальной игре отсутствует | **НЕТ** |
| `FAST_EDDIE` | Controls.cpp, vehicle.cpp | Быстрый отладочный режим (`#if defined(FAST_EDDIE) && 0`) | **НЕТ** |
| `USE_PASSWORD` | startscr.cpp (одно место) | Система паролей/читов | **ДА** — перенести концептуально (чит-коды работают, реализация может отличаться) |

**BIKE:** флаг существует и используется широко (~15 файлов), но фича незавершена и не вошла в финальную игру. **Не переносить в новую версию.**

---

## 4. Отладочные флаги

| Флаг | Где определяется | Назначение | Нужен |
|------|-----------------|-----------|-------|
| `DEBUG` | Проект (Debug конфигурация) | Debug build | **ДА** (для разработки) |
| `_DEBUG` | MSVC predefined | Debug configuration | **ДА** (для разработки) |
| `NDEBUG` | Проект (Release конфигурация) | No debug | **ДА** |
| `_RELEASE` | Проект (Release конфигурация) | Release build | **ДА** |
| `FINAL` | Fallen.vcxproj (Release only) | Финальная сборка, выключает отладочный код | **ДА** (в Release) |
| `HEAP_DEBUGGING_PLEASE_BOB` | StdMem.cpp (закомментирован) | Отладка кучи | **НЕТ** |
| `DEBUG_POOSHIT` | Engine.cpp | Отладочный вывод | **НЕТ** |
| `FACET_REMOVAL_TEST` | facet.cpp (закомментирован) | Визуализация удаляемых полигонов | **НЕТ** |
| `TEST_3DFX` | Engine.cpp, LevelEd.cpp | Тестирование 3dfx | **НЕТ** |

---

## 5. Флаги редактора

| Флаг | Где определяется | Назначение | Нужен |
|------|-----------------|-----------|-------|
| `EDITOR` | GEdit.dsp, psxlib.dsp | Level Editor | **НЕТ** |
| `BUILD_PSX` | GEdit.dsp, PSXENG/psxeng.h | Сборка для PSX | **НЕТ** |
| `DOG_POO`, `POO`, `FUNNY_FANNY`, `OLD_STUFF`, `GUY`, `I_AM_A_LOON`, `DOGPOO`, `_MF_WINDOWS_DOG_POO` | Editor/Source файлы | Внутренние флаги разработчиков | **НЕТ** |

Все папки редакторов (`/fallen/Editor/`, `/fallen/GEdit/`, `/fallen/LeEdit/`, `/fallen/SeEdit/`) можно удалить полностью.

---

## 6. Региональные флаги

| Флаг | Где определяется | Назначение | Нужен |
|------|-----------------|-----------|-------|
| `EIDOS` | password.h | Учётные данные издателя: EXPORT_NAME="Darren Hedges", EXPORT_CO="Eidos UK", EXPORT_PW="twogoldfish". Вероятно DRM/дистрибьюция. | **НЕТ** — полностью убрать |
| `USA`, `GERMANY`, `FRANCE`, `KK`, `SINGAPORE`, `LEADER`, `HALIFAX`, `SYNTHESIS`, `DLMM`, `EEM`, `EUROPE`, `JAPAN` | password.h | Альтернативные региональные издания | **НЕТ** |
| `TDFX` | password.h | Специальная версия для 3dfx Voodoo | **НЕТ** |
| `VERSION_ENGLISH`, `VERSION_GERMAN` и др. | psxeng.h | Языковые версии PSX | **НЕТ** |
| `VERSION_DEMO`, `VERSION_REVIEW`, `VERSION_PAL`, `VERSION_NTSC`, `VERSION_LORES` | psxeng.h | Разновидности PSX версий | **НЕТ** |

---

## 7. Активные флаги-константы (заданы в коде, не в проектных настройках)

Эти флаги определяются прямо в исходниках (`#define FLAG 1`), не в vcxproj. Менять не нужно.

| Флаг | Где определён | Назначение | Нужен |
|------|--------------|-----------|-------|
| `USE_TOMS_ENGINE_PLEASE_BOB` | aeng.h (`= 1`) | D3D-friendly рендерер персонажей | **ДА** |
| `WE_NEED_POLYBUFFERS_PLEASE_BOB` | polypage.h (`= 1` на PC) | Z-сортировка полигонов через буфер | **ДА** |
| `NO_BACKFACE_CULL_PLEASE_BOB` | poly.cpp | Отключить backface culling для двусторонних мешей | **ДА** |
| `LIGHT_COLOURED` | light.h (`= 1`) | RGB-цвет освещения вместо greyscale | **ДА** |
| `USE_D3D_VBUF` | vertexbuffer.h (`= 1`) | D3D vertex buffers ("33% faster") | **ДА** |
| `DRAW_WHOLE_PERSON_AT_ONCE` | figure.cpp (`= 1`) | Рисовать весь персонаж одним MultiMatrix вызовом | **ДА** |
| `ULTRA_COMPRESSED_ANIMATIONS` | anim.h (defined) | Формат матриц анимации; выбор между PC и PSX форматом | **ДА** |
| `ANIMATE` | animate.h (defined) | Базовый флаг включения системы анимаций | **ДА** |
| `ANTIALIAS_BY_HAND` | truetype.cpp (`= 1`) | 2x supersampling антиалиасинг для TrueType шрифтов | **ДА** |
| `POLY_FADEOUT_START` | Night.h / poly.h (`= 0.60F`) | Константа дистанции начала fog fade-out | **ДА** |
| `NEW_FLOOR` | aeng.cpp | Система рендеринга полов с improved lighting | **ДА** |
| `SUPERCRINKLES_ENABLED` | supercrinkle.h (`= 0`) | Dynamic surface deformation; явно выключен для production | **НЕТ** (`= 0`, отключён) |
| `DISABLE_CRINKLES` | Crinkle.cpp (`= 0`) | Отключение старой crinkle системы; явно выключен | **НЕТ** (`= 0`, отключён) |
| `CALC_CAR_CRUMPLE` | mesh.cpp (`= 0`) | Вычислять crumple-зоны машин динамически; явно выключен | **НЕТ** (`= 0`, используется pre-computed таблица) |

---

## 8. Прочие флаги

| Флаг | Назначение | Нужен |
|------|-----------|-------|
| `USE_A3D` | A3D audio (Aureal 3D) — определён в psxeng.h, только PSX | **НЕТ** |
| `UNICODE` | Unicode в диалогах Windows (userdlg.cpp) | **ДА** |
| `__cplusplus` | Защита extern "C" в wave.h | **ДА** |
| `_CRT_SECURE_NO_WARNINGS` | Подавление предупреждений MSVC о небезопасных CRT функциях | не нужен при переписывании |
| `WINDOWS_IGNORE_PACKING_MISMATCH` | Подавление предупреждений о выравнивании (Debug) | не нужен при переписывании |
| `APSTUDIO_INVOKED` | Защита от редактора ресурсов VS | не нужен при переписывании |
| `MARKS_PRIVATE_VERSION` | fc.cpp — если определён, отключает gun-out режим камеры | **НЕТ** (не определяется в финале) |
| `HIGH_REZ_PEOPLE_PLEASE_BOB` | figure.cpp — закомментирован ("Now enabled only on a cheat!") | **НЕТ** |
| `OLD_CAM` / `CAM_OLD` | Остатки старой камеры (cam.cpp удалён); встречается в #ifdef блоках | **НЕТ** |
| `NO_SERVER` / `SERVER` | Отключённый multiplayer режим; нигде не defined | **НЕТ** |
| `SEWERS` | Флаг системы канализаций (cut feature по Mike Diskett) | **НЕТ** |
| `TRUETYPE` | Незавершённая фича TrueType шрифтов; закомментирован (// #define TRUETYPE) | **НЕТ** |
| `USE_W_FOG_PLEASE_BOB` | W-based fog вместо Z-based; код есть но нигде не defined | **НЕТ** |
| `QUICK_FACET` | Быстрый путь рисования простых фасетов; нигде не defined | **НЕТ** |
| `DO_SUPERFACETS_PLEASE_BOB` | Кэш superfacet; закомментирован ("Can't do these for release") | **НЕТ** |
| `BODGE_MY_PANELS_PLEASE_BOB` | Hack глубины панелей в D3D; нигде не defined | **НЕТ** |
| `BOGUS_TGAS_PLEASE_BOB` | Fast загрузка TGA без сжатия; нигде не defined | **НЕТ** |
| `NO_MORE_BALLOONS` / `NO_MORE_BALLOONS_NOW` | Отключение balloon системы; нигде не defined | **НЕТ** |
| `WE_WANT_WIND` | Вырезанная wind система для эффектов частиц | **НЕТ** |
| `WE_WANT_MANUAL_WIND_ASWELL` | Ручной контроль wind (внутри WE_WANT_WIND) | **НЕТ** |
| `WE_WANT_SHITTY_PANTS_WIND` | Wind для snow уровня; нигде не defined | **НЕТ** |
| `WE_WANT_TO_DRAW_THESE_FACET_LINES` | Debug визуализация граней фасетов | **НЕТ** |
| `WE_WANT_TO_DRAW_THE_TEXTURE_SHADOW_PAGE` | Debug визуализация shadow texture page | **НЕТ** |
| `PSX_COMPRESS_LIGHT` | Сжатый формат освещения для PSX (Night.h) | **НЕТ** |
| `DODGYPSXIFY` | Хак PSX диапазона звука в SOUND_Range() | **НЕТ** |
| `PSX_SIMULATION` | Симуляция PSX поведения на PC | **НЕТ** |
| `VERSION_PSX` | PSX-специфичная структура Camera и логика | **НЕТ** |
| `VERSION_DEMO` | Demo-режим: блокировка миссий, ограниченное меню | **НЕТ** |
| `VERSION_ENGLISH` / `VERSION_GERMAN` / `VERSION_FRENCH` / `VERSION_ITALIAN` / `VERSION_JAPAN` / `VERSION_SPANISH` / `VERSION_KANJI` / `VERSION_KOREA` | Языковые версии PSX (psxeng.h) | **НЕТ** |
| `VERSION_CD` / `VERSION_RCD` / `VERSION_DCD` / `VERSION_DPC` / `VERSION_RPC` / `VERSION_RSC` / `VERSION_RIC` / `VERSION_USA` / `VERSION_NTSC` / `VERSION_PAL` / `VERSION_LORES` / `VERSION_REVIEW` | Варианты PSX дистрибутива | **НЕТ** |
| `MAD_AM_I` / `ARGH` / `MIKE` / `MIKE_INFO` / `DTRACE` / `werrr` | Developer debug флаги разработчиков | **НЕТ** |
| `GONNA_FIREBOMB_YOUR_ASS` / `POO_SHIT_GOD_DAMN` / `OLD_DOG_POO_OF_A_SYSTEM_OR_IS_IT` / `WHAT_THE_FUCK_IS_THIS_DOING_HERE` / `OLDSHIT` / `ONE_DAY` / `GOTTA_DO_A_BETTA_JOB` / `DONE_ON_PC_NOW` / `WERE_GOING_TO_STUPIDLY_STICK_THE_FINAL_BANE_INSIDE` / `WEVE_REPLACED_THE_HEARTBEAT_WITH_A_SCANNER` | Dev-комментарии-флаги (joke names, placeholders) | **НЕТ** |
| `A3D_SOUND` | A3D звук (устаревшая Aureal API, PSX-связан с USE_A3D) | **НЕТ** |
| `DEBUG_POLY` / `DEBUG_SPAN` / `DRAW_THIS_DEBUG_STUFF` / `MESH_SHOW_MOUSE_POINT` / `SHOW_ME_FIGURE_DEBUGGING_PLEASE_BOB` / `SHOW_ME_SUPERFACET_DEBUGGING_PLEASE_BOB` | Debug-визуализации рендера; нигде не defined | **НЕТ** |
| `FASTPRIM_PERFORMANCE` / `STRIP_STATS` / `SUPERFACET_PERFORMANCE` | Perf-профилирование рендера; нигде не defined | **НЕТ** |
| `FEEDBACK` | Debug feedback в flamengine; закомментирован | **НЕТ** |
| `OLD_FACET_CLIP` / `OLD_FLIP` / `OLD_POO` / `OLD_SPLIT` | Старые альтернативные алгоритмы рендера; все мёртвые | **НЕТ** |
| `MIKES_UNUSED_AUTOMATIC_FLOOR_TEXTURE_GROUPER` | Явно неиспользуемый floor texture grouper (название говорит само) | **НЕТ** |
| `UNUSED` / `NOT_USED` / `UNUSED_WIRECUTTERS` / `UNUSED_WIRE_CUTTERS` | Явно помеченный мёртвый код | **НЕТ** |
| `DONT_WORRY_ABOUT_INSIDES_FOR_NOW` | Временный skip процедурных интерьеров; нигде не defined | **НЕТ** |
| `DOWNSAMPLE_PLEASE_BOB_AMOUNT` | Даунсэмплинг текстур; нигде не defined | **НЕТ** |
| `DRAW_BLACK_FACETS` / `DRAW_FLOOR_FURTHER` | Debug-рендер фасетов/пола; нигде не defined | **НЕТ** |
| `BACK_CULL_MAGIC` | Магический backface cull hack; нигде не defined | **НЕТ** |
| `NO_CLIPPING_TO_THE_SIDES_PLEASE_BOB` | Отключение бокового клиппинга; нигде не defined | **НЕТ** |
| `NO_TRANSFORM` | Пропуск трансформаций; нигде не defined | **НЕТ** |
| `SEARCHING` | Debug режим поиска пути; нигде не defined | **НЕТ** |
| `SWIZZLE` | DC-специфичный swizzle формат текстур | **НЕТ** |
| `THIS_IS_INCLUDED_FROM_SW` | Guard для sw.cpp include path | **НЕТ** |
| `TOPMAP_BACK` | Debug topmap background; нигде не defined | **НЕТ** |
| `VERIFY` | Debug проверка данных; нигде не defined | **НЕТ** |
| `WE_WANT_A_WHITE_SHADOW` / `WE_WANT_TO_DARKEN_PEOPLE_IN_SHADOW_ABRUPTLY` / `WE_WANT_TO_TEST_THE_WORLD_LINE_DRAW_BY_DRAWING_THE_COLVECTS` / `WHEN_DO_I_WANT_TO_TWO_PASS` | Debug/эксперименты со светом и тенями | **НЕТ** |
| `DARCI_HITS_COPS` | Геймплей-флаг Darci vs копы; нигде не defined | **НЕТ** |
| `CUNNING_SORT` / `GOOD_SORT` | Альтернативные алгоритмы Z-сортировки; нигде не defined | **НЕТ** |
| `ENABLE_REMAPPING` | Ремаппинг управления; нигде не defined | **НЕТ** |
| `MAKE_THEM_FACE_THE_CAMERA` | Sprite billboarding; нигде не defined | **НЕТ** |
| `LIMIT_TOTAL_PYRO_SPRITES_PLEASE_BOB` | Ограничение pyro спрайтов; нигде не defined | **НЕТ** |
| `LOOK_FOR_START_NOT_JUST_ANY_BUTTON` | PSX: ждать START вместо любой кнопки | **НЕТ** |
| `FS_ISO9600` / `FS_ISO9660` | PSX: CD-ROM filesystem flags | **НЕТ** |
| `_DEBUG_POO` | Debug флаг (отдельный от DEBUG_POOSHIT) | **НЕТ** |
| `BREAKTIMER` | Break timer debug; нигде не defined | **НЕТ** |
| `AUTO_SELECT` | Авто-выбор устройства ввода; нигде не defined | **НЕТ** |

---

## 9. Не feature flags (не нуждаются в отдельной документации)

Эти символы найдены coan-ом в `#ifdef` выражениях, но они не являются настраиваемыми флагами сборки:

| Символ | Почему не flag |
|--------|---------------|
| `PI`, `MAX3`, `MIN3`, `MATRIX_CALC`, `FOUR` | Math константы/макросы |
| `_MSC_VER`, `__MSC_VER` | MSVC compiler version detection |
| `LEVEL_LOST`, `LEVEL_WON` | Return code defines (enum-like значения) |
| `STATE_DEF` | Guard против двойного typedef |
| `VERSION` | Строковая/числовая константа версии |
| `APSTUDIO_READONLY_SYMBOLS` | VS resource editor артефакт |
| `FILTERING_ON` | Transient D3D render state (define/undef внутри файла) |
| `ALPHA_ADD`, `ALPHA_BLEND`, `ALPHA_BLEND_NOT_DOUBLE_LIGHTING`, `ALPHA_MODE`, `ALPHA_TEST` | Transient D3D render states |
| `NORMAL` | Math/geometry enum значение |
| `_mfx_h_`, `_sound_id_h_`, `_SewerTab_HPP_` | Нестандартные include guards |

---

## 11. Требуют уточнения

Эти флаги найдены в оригинальной игре, но их статус неочевиден:

| Флаг | Где встречается | Вопрос |
|------|----------------|--------|
| `INSIDES_EXIST` / `SEWERS_EXIST` | building.cpp, ns.cpp | Флаги существования процедурных интерьеров/канализаций — активны в данных уровней или нет? |
| `FILE_PC` | io.cpp и др. | PC-специфичная загрузка файлов — активен или мёртв? |
| `DONT_IGNORE_SHADOWS` / `DONT_IGNORE_PUDDLES` / `DONT_IGNORE_REFLECTIONS` / `DONT_IGNORE_STARS` / `DONT_IGNORE_FANCY_STUFF` | DDEngine | Флаги качества рендера — заданы где-то или всегда undefined? |
| `COLLIDE_GAME` | collide.h/cpp | Режим коллизий — что переключает? |

---

## Подтверждённые конфигурации (Fallen.vcxproj)

### Release build:
```
NDEBUG;_RELEASE;WIN32;_WINDOWS;VERSION_D3D;TEX_EMBED;FINAL
```

### Debug build:
```
_WINDOWS;WIN32;_DEBUG;DEBUG;VERSION_D3D;TEX_EMBED;WINDOWS_IGNORE_PACKING_MISMATCH
```

**Важно:** `TEX_EMBED` активен в **обеих** конфигурациях. `FINAL` — только в Release.

---

## Итог: что удаляется, что остаётся

### Безопасно удалить при переписывании:
- Вся папка `/fallen/Glide Engine/` — код Glide мёртв ✅ (удалено в Этапе 2)
- Все редакторы: `/fallen/Editor/`, `/fallen/GEdit/`, `/fallen/LeEdit/`, `/fallen/SeEdit/` ✅ (удалено в Этапе 2)
- Папка `/fallen/PSXENG/` — код PSX ✅ (удалено в Этапе 2)
- Флаги: `TARGET_DC`, `PSX`, `VERSION_GLIDE`, `EDITOR`, `BUILD_PSX` ✅ (удалено в Этапе 2)
- Флаги фич (Этап 2, пункт 2.5): `BIKE`, `FAST_EDDIE`, `EIDOS`/региональные, `USE_A3D`, `__WINDOWS_386__`, `FACET_REMOVAL_TEST`, `HIGH_REZ_PEOPLE_PLEASE_BOB`, `MARKS_PRIVATE_VERSION`, `OLD_CAM`/`CAM_OLD`, `NO_SERVER`/`SERVER`, `SEWERS`, `TRUETYPE`, `USE_W_FOG_PLEASE_BOB`, `QUICK_FACET`, `DO_SUPERFACETS_PLEASE_BOB`, `BODGE_MY_PANELS_PLEASE_BOB`, `BOGUS_TGAS_PLEASE_BOB`, `NO_MORE_BALLOONS`/`NO_MORE_BALLOONS_NOW`, `WE_WANT_WIND` и родственные, `PSX_COMPRESS_LIGHT`, `DODGYPSXIFY`, `PSX_SIMULATION`, `VERSION_PSX`, `VERSION_DEMO`, все `VERSION_*` PSX, `HEAP_DEBUGGING_PLEASE_BOB`, `DEBUG_POOSHIT`, `TEST_3DFX`, `FAST_EDDIE`
- Developer joke flags: `MAD_AM_I`, `ARGH`, `MIKE`, `MIKE_INFO`, `DTRACE`, `werrr`, `GONNA_FIREBOMB_YOUR_ASS` и аналогичные

### Требует отдельного изучения:
- ~~`USE_PASSWORD`~~ — решено: чит-коды переносятся (реализация может отличаться)

### Аккуратно обработать при удалении:
- `TARGET_DC` — проникает в базовые библиотеки MFLib1, MFStdLib
- `PSX` — встречается в физике (collide.h) и структурах данных (building.h, dirt.h, Command.h)

### Активные в финальном билде:
- `VERSION_D3D`, `MF_DD2`, `TEX_EMBED` — графический стек
- `FINAL`, `DEBUG`, `_DEBUG`, `NDEBUG`, `_RELEASE` — конфигурации сборки
- `_MF_WINDOWS`, `_WINDOWS`, `_WIN32` — платформенные детекты
