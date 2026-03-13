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

## 7. Прочие флаги

| Флаг | Назначение | Нужен |
|------|-----------|-------|
| `USE_A3D` | A3D audio (Aureal 3D) — определён в psxeng.h, только PSX | **НЕТ** |
| `UNICODE` | Unicode в диалогах Windows (userdlg.cpp) | **ДА** |
| `__cplusplus` | Защита extern "C" в wave.h | **ДА** |
| `_CRT_SECURE_NO_WARNINGS` | Подавление предупреждений MSVC о небезопасных CRT функциях | не нужен при переписывании |
| `WINDOWS_IGNORE_PACKING_MISMATCH` | Подавление предупреждений о выравнивании (Debug) | не нужен при переписывании |
| `APSTUDIO_INVOKED` | Защита от редактора ресурсов VS | не нужен при переписывании |
| `MARKS_PRIVATE_VERSION` | fc.cpp — если определён, отключает gun-out режим камеры | **НЕТ** (НЕ определяется в финале) |
| `USE_TOMS_ENGINE_PLEASE_BOB` | aeng.h — всегда = 1, D3D-friendly рендерер персонажей | **ДА** (всегда активен) |
| `WE_NEED_POLYBUFFERS_PLEASE_BOB` | polypage.h — 1 на PC, 0 на DC. Z-сортировка полигонов | **ДА** (= 1 для PC) |
| `HIGH_REZ_PEOPLE_PLEASE_BOB` | figure.cpp — закомментирован ("Now enabled only on a cheat!") | **НЕТ** |
| `NO_BACKFACE_CULL_PLEASE_BOB` | poly.cpp — отключить backface culling для двусторонних мешей | **ДА** (используется) |

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
- Вся папка `/fallen/Glide Engine/` — код Glide мёртв
- Все редакторы: `/fallen/Editor/`, `/fallen/GEdit/`, `/fallen/LeEdit/`, `/fallen/SeEdit/`
- Папка `/fallen/PSXENG/` — код PSX
- Код под флагами: `TARGET_DC`, `PSX`, `VERSION_GLIDE`, `EDITOR`, `BUILD_PSX`
- Отладочные артефакты: `HEAP_DEBUGGING_PLEASE_BOB`, `DEBUG_POOSHIT`, `FACET_REMOVAL_TEST`, `TEST_3DFX`, `FAST_EDDIE`
- Все региональные флаги включая `EIDOS` (только учётные данные издателя, не геймплей)
- `USE_A3D`, `_MF_DOSX`, `__WATCOMC__`, `__DOS__`, `__WINDOWS_386__`

### Требует отдельного изучения:
- ~~`USE_PASSWORD`~~ — решено: чит-коды переносятся (реализация может отличаться)

### Аккуратно обработать при удалении:
- `TARGET_DC` — проникает в базовые библиотеки MFLib1, MFStdLib
- `PSX` — встречается в физике (collide.h) и структурах данных (building.h, dirt.h, Command.h)

### Активные в финальном билде:
- `VERSION_D3D`, `MF_DD2`, `TEX_EMBED` — графический стек
- `FINAL`, `DEBUG`, `_DEBUG`, `NDEBUG`, `_RELEASE` — конфигурации сборки
- `_MF_WINDOWS`, `_WINDOWS`, `_WIN32` — платформенные детекты
