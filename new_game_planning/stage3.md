# Этап 3 — Переход на Clang ✅ ЗАВЕРШЁН

**Цель:** сменить компилятор с MSVC на Clang.

## Шаг 1 — ClangCL в vcxproj ✅ ЗАВЕРШЁН

- `PlatformToolset` переключён на `ClangCL` в обоих конфигурациях
- Исправлены ошибки компиляции Clang-специфичные:
  - `figure.cpp`: `def##name` → `def name` в макросе `ALIGNED_STATIC_ARRAY` (invalid token paste)
  - `Drive.cpp`: `const char*` → `(char*)` каст для строкового литерала
  - `GDisplay.cpp`, `frontend.cpp`: `MFFileHandle > 0` → `!= nullptr`
  - `Controls.cpp`: corrupted `'£'` (U+FFFD) → `(UBYTE)0xa3`
  - `frontend.cpp`: `{-1,...}` в UBYTE-поле → `{(UBYTE)-1,...}`
- Исправлен баг vcxproj: убраны 100 мусорных строк `/D /NODEFAULTLIB`, добавлены SDL2.lib/OpenAL32.lib в Debug
- Release флаги: `-O2 -fno-inline-functions -fno-strict-aliasing`
  - `-fno-inline-functions`: без него Clang `-O2` ломает рендеринг body parts персонажей
    (инлайнинг `FIGURE_draw_prim_tween_person_only_just_set_matrix` в цикл — причина неизвестна, TODO)
  - `-fno-strict-aliasing`: превентивно, код активно использует pointer type punning

## Шаг 2 — vcxproj → CMakeLists.txt ✅ ЗАВЕРШЁН

vcxproj заменён на CMakeLists.txt + CMake/Ninja сборочная система.

**Критерий:** проект конфигурируется и собирается через CMake/Ninja, Debug и Release работают. ✅
