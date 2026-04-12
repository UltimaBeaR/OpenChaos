# Девлог Этапа 3 — Переход на Clang

## Шаг 1: ClangCL в vcxproj (2026-03-19)

### Что сделано

Переключён `PlatformToolset` с `v145` (MSVC) на `ClangCL` в обоих конфигурациях (Debug и Release).

---

### Ошибки компиляции Clang — исправления

#### 1. `figure.cpp` — invalid token paste в макросе `ALIGNED_STATIC_ARRAY`

**Файл:** `DDEngine/Source/figure.cpp:177`

**Суть:** макрос использовал `def##name` где `def = "static D3DCOLOR*"`. Оператор `##` пытался склеить `*` и `MM_pcFadeTable` → невалидный токен `*MM_pcFadeTable`. MSVC молча игнорировал и разделял токены, Clang строго падал с ошибкой.

**Важно:** в оригинальном коде (`original_game/`) после ALIGNED_STATIC_ARRAY вызовов стояли отдельные объявления переменных в блоке `#if 0` (закомментированы). В новом коде эти объявления убраны — макрос является единственным объявлением.

**Фикс:** `def##name` → `def name` (убрали `##`, оставили пробел — два параметра рядом).

#### 2. `Drive.cpp` — `const char*` → `char*`

**Файл:** `DDLibrary/Source/Drive.cpp:67`

```cpp
// было:
static inline char* GetPath(bool on_cd) { return on_cd ? Path : ".\\"; }
// стало:
static inline char* GetPath(bool on_cd) { return on_cd ? Path : (char*)".\\"; }
```

Строковый литерал `".\\"` — `const char*`. Clang не допускает неявный каст к `char*`.

#### 3. `GDisplay.cpp`, `frontend.cpp` — сравнение `void*` с `int`

`MFFileHandle` — это `void*`. Сравнение `> 0` недопустимо для указателей в Clang.

```cpp
// было:
if (image_file > 0) {
// стало:
if (image_file != nullptr) {
```

Семантика: `FileOpen` возвращает `NULL` при ошибке, поэтому `!= nullptr` эквивалентно.

#### 4. `Controls.cpp` — corrupted символ `£`

**Файл:** `Source/Controls.cpp:185`

Символ `£` в исходнике был закодирован как U+FFFD (replacement character, 3 байта в UTF-8) вместо корректного U+00A3. Это произошло при конвертации кодировки Windows-1252 → UTF-8 где-то в истории.

Clang: `character too large for enclosing character literal type`.

**Фикс:** заменён `'£'` (3 байта) на `(UBYTE)0xa3` (Latin-1 код символа фунта стерлингов, UK keyboard layout Shift+3).

Замена сделана через бинарный поиск/замену PowerShell — Edit tool не мог найти строку из-за несовпадения байт.

#### 5. `Controls.cpp` — ternary возвращает `const char*`

```cpp
// было:
CONSOLE_text(PolyPage::AlphaSortEnabled() ? "Alpha sorting is ON" : "Alpha sorting is OFF");
// стало:
CONSOLE_text((CBYTE*)(PolyPage::AlphaSortEnabled() ? "Alpha sorting is ON" : "Alpha sorting is OFF"));
```

`CONSOLE_text` принимает `CBYTE*` (= `char*`), тернарный оператор возвращает `const char*`.

#### 6. `frontend.cpp` — сужение `-1` до `UBYTE`

```cpp
// было:
{ -1, 0, 0, 0 },  // сентинел конца массива RawMenuData[]
// стало:
{ (UBYTE)-1, 0, 0, 0 },
```

Первое поле `RawMenuData::Menu` — `UBYTE`. Clang C++11 запрещает narrowing conversion без явного каста.

---

### Баги в vcxproj — исправления

#### 1. Мусорные `AdditionalOptions` в Debug

100 строк вида:
```xml
<AdditionalOptions Condition="...Debug..."> /D /NODEFAULTLIB:libcmtd.lib" "  /D /NODEFAULTLIB:libcmtd.lib" "</AdditionalOptions>
```

На каждый `.cpp` файл. Это артефакт какой-то старой операции. `/D` — флаг определения препроцессорного макроса, после него должно быть имя, а не `/NODEFAULTLIB:...`. MSVC молча игнорировал, Clang падал: `macro name must be an identifier`.

`/NODEFAULTLIB:libcmtd.lib` — линкерный флаг, ему место в `<Link>`, не в `<ClCompile>`. Убраны все 100 строк.

#### 2. SDL2.lib и OpenAL32.lib отсутствовали в Debug линкере

В Release конфиге: `SDL2.lib` и `OpenAL32.lib` были в `AdditionalDependencies`.
В Debug конфиге: отсутствовали → 20 ошибок линковки (`undefined symbol: _SDL_FreeWAV`, `_alGenSources` и т.д.).

Добавлены в Debug `<Link><AdditionalDependencies>`.

---

### Release флаги — найденные проблемы

#### `-fno-inline-functions` — рендеринг body parts

**Симптом:** в Release сборке тела персонажей рендерятся неправильно: торс на месте, конечности хаотично прыгают вокруг него (в радиусе ~0.5м), позиции меняются каждый кадр.

**Диагностика:**
- Debug (`-O0`): норма
- Release с `-O1`: норма
- Release с `-O2`: баг
- Release с `-O2 -fno-vectorize -fno-slp-vectorize`: баг (векторизация не причина)
- Release с `-O2 -fno-inline-functions`: норма → **причина найдена: агрессивный inlining**

**Предположение о причине:** Clang `-O2` инлайнит `FIGURE_draw_prim_tween_person_only_just_set_matrix` в цикл по 15 body parts в `FIGURE_draw_hierarchical_prim_recurse_individual_cull`. После инлайнинга что-то в оптимизации ломает порядок записи/чтения матриц через глобальный `g_matWorld` (который обновляется через `POLY_set_local_rotation` из `poly.cpp`). Функция записывает матрицу в `MMBodyParts_pMatrix[iMatrixNum]`, а кадровая матрица читается из `g_matWorld`.

**Статус:** TODO — расследовать при работе над рендерером (Этап 5+). Для текущего этапа приемлемо.

**Решение:** `-fno-inline-functions` в Release `AdditionalOptions`. Все остальные `-O2` оптимизации работают.

#### `-fno-strict-aliasing` — превентивно

Код активно использует type punning:
```cpp
unsigned long EVal = 0xe0001000;
pmat->_41 = *(float*)&EVal;  // читаем int-биты как float
```

Clang `-O2` со strict aliasing может оптимизировать такие места некорректно. Флаг отключает это превентивно — подобный паттерн встречается в нескольких местах кодовой базы.

---

### Итог Шага 1

- Debug: 0 ошибок, запускается ✅
- Release: 0 ошибок, запускается, рендеринг корректный ✅
- Предупреждений: ~7400 (Clang строже MSVC — много `-Wwritable-strings`, `-Wshift-op-parentheses`, `-Wunused-variable` и т.д. — не блокируют)

### Следующий шаг

**Шаг 2:** Переход с vcxproj на CMakeLists.txt. ✅ Выполнен ниже.

---

## Шаг 2: vcxproj → CMakeLists.txt (2026-03-19)

### Структура репозитория после шага

```
new_game/
  src/
    fallen/          ← C++ исходники (только код)
    MFStdLib/        ← C++ исходники
  build/             ← gitignored: всё генерируемое
    Debug/
      Fallen.exe, *.dll, text/, config.ini
    Release/
      Fallen.exe, *.dll, text/, config.ini
    CMakeFiles/      ← *.obj и внутренние файлы CMake/Ninja
    build.ninja, cmake_install.cmake, ...
  cmake/
    clang-x86-windows.cmake   ← toolchain: clang-cl x86
  scripts/
    configure.ps1    ← первичная настройка cmake
    build.ps1        ← сборка (полный ребилд или инкремент)
    copy_resources.ps1  ← копирование ресурсов из original_game_resources/
  vcpkg.json         ← манифест зависимостей
  vcpkg_installed/   ← gitignored: SDL2, OpenAL, fmt (как node_modules)
  CMakeLists.txt
```

Удалены из репозитория: `Fallen.vcxproj`, `Fallen.sln`.

Ресурсы игры (`text/`, `config.ini` и всё остальное) живут в `original_game_resources/`
и копируются в `build/Debug/` и `build/Release/` отдельной командой — не при сборке.

### Команды

```
make configure               # первый раз или после изменений в CMakeLists.txt
make build-debug             # полный ребилд Debug
make build-release           # полный ребилд Release
make copy-resources          # скопировать ресурсы в Debug и Release
make copy-resources-debug    # только в Debug
make copy-resources-release  # только в Release
make run-debug
make run-release
```

### Архитектура сборки

**Генератор:** Ninja Multi-Config — один configure для обоих конфигов.

**Toolchain цепочка:**
```
-DCMAKE_TOOLCHAIN_FILE=<vcpkg.cmake>
-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/clang-x86-windows.cmake
```
vcpkg.cmake — основной (управляет пакетами), наш cmake файл грузится вторым через chainload.

**PowerShell и cmake:**
В bash/make окружении ни `powershell`, ни `cmake` не видны по короткому имени.
В Makefile заданы полные пути через переменные `POWERSHELL` и `CMAKE`.

**configure.ps1:** перед вызовом cmake запускает `vcvarsall.bat x86` чтобы получить
пути к Windows SDK headers/libs — без этого clang-cl не найдёт `windows.h`, `d3d9.h` и т.д.

**build.ps1:** перед cmake --build добавляет PowerShell в PATH процесса — нужно для
post-build шагов, которые копируют DLL через cmake -E.

**vcpkg DLL копирование:** `VCPKG_APPLOCAL_DEPS` отключён (его `applocal.ps1` требует
`powershell.exe` через cmd.exe без полного пути — падает в этой среде).
DLL копируются вручную через `add_custom_command(POST_BUILD)` в CMakeLists.txt.

**copy_resources.ps1:** использует `robocopy` (полный путь через `$env:SystemRoot\System32\`)
с исключением git-файлов (`.gitkeep`, `.gitignore`, `.gitattributes`, `.git/`).

### Известные потенциальные проблемы

- `strmbase.lib` / `amstrmid.lib` / `quartz.lib` — унаследованные из vcxproj зависимости DirectShow.
  `strmbase.lib` не входит в современные Windows SDK. Если линковка упадёт — удалить из
  `target_link_libraries`.

### Нюансы при реализации

#### git mv не работал на fallen/
`git mv new_game/fallen` падал с Permission denied — что-то держало handle на папку (не был запущен процесс, причина неизвестна). Решение: `git ls-files | cp` + `git rm --cached` + `git add`. Git корректно детектировал rename.

#### CMake 4.2: `$Config` в именах Ninja-правил
CMake 4.2 генерирует `rule CXX_COMPILER__Fallen_$Config` в rules.ninja. Ninja не поддерживает
переменные в именах правил — падает при парсинге. Политика `CMP0172 OLD` не помогла.
Решение: **Ninja Multi-Config** generator — в нём этой проблемы нет.

### Статус

- Конфигурация: **✅** (`make configure`)
- Сборка Debug: **✅** (`make build-debug`)
- Сборка Release: **✅** (`make build-release`)
- Копирование ресурсов: **✅** (`make copy-resources`)
- Запуск: не проверялся

---

## Доработки после Шага 2 (2026-03-19)

### Debug DLL копирование — SDL2d.dll, fmtd.dll

Debug-сборка падала при запуске: требовала `SDL2d.dll`, а в папке лежала только релизная `SDL2.dll`.

vcpkg кладёт debug DLL в отдельную папку с другими именами:
- `vcpkg_installed/x86-windows/bin/` — `SDL2.dll`, `OpenAL32.dll`, `fmt.dll` (Release)
- `vcpkg_installed/x86-windows/debug/bin/` — `SDL2d.dll`, `OpenAL32.dll`, `fmtd.dll` (Debug)

**Фикс:** в `CMakeLists.txt` разделены post-build команды копирования по конфигурациям через `CONFIGURATIONS Release` / `CONFIGURATIONS Debug`. OpenAL32.dll имеет одинаковое имя в обоих конфигах, но берётся из разных папок.

---

## Шаг 3: Документация (2026-03-19)

Созданы и обновлены пользовательские README/гайды под новую сборочную систему:

- `new_game/SETUP.md` — полный гайд по первичной настройке: Visual Studio 2026, компоненты (clang-cl, CMake tools, vcpkg), шаги configure → copy-resources → build → run, таблица всех make-команд
- `original_game/SETUP.md` — гайд по сборке оригинальной игры: VS2026 + MSVC, vcpkg integrate install, сборка Fallen.sln
- `README.md` (корень) — убраны устаревшие пути (`fallen/Debug/` → `build/Debug/`), секция Setup заменена ссылками на SETUP.md, добавлены `configure` и `copy-resources` в таблицу команд
- `new_game/README.md` — исправлены пути, Quick start дополнен шагами `configure` и `copy-resources`
- `original_game/README.md` — переработан: добавлен нормальный заголовок с историей форков, оригинальный текст Mike Diskett сохранён ниже

---

## Итог Этапа 3 (2026-03-19)

Этап 3 полностью завершён. Итоговое состояние:

- Компилятор: **clang-cl x86** (через Visual Studio 2026)
- Сборочная система: **CMake + Ninja Multi-Config** (заменил vcxproj/MSBuild)
- Зависимости: **vcpkg manifest mode** (SDL2, OpenAL, fmt — устанавливаются автоматически при configure)
- Debug и Release: оба собираются и запускаются ✅

**Важная заметка на будущее (Этап 8):** текущая сборка всё ещё требует Visual Studio — для vcvarsall.bat (Windows SDK), для clang-cl и cmake (пути захардкожены в скриптах). Цель Этапа 8 — убрать эту зависимость: перейти на standalone Clang + CMake без VS, что откроет путь к кросс-платформенной сборке.
