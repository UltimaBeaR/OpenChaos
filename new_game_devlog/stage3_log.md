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

**Шаг 2:** Переход с vcxproj на CMakeLists.txt.
