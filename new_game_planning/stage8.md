# Этап 8 — Кросс-платформа

**Цель:** убрать Windows-специфичный код и зависимость от Visual Studio.

## Сборочная система — уход от VS

- Заменить `clang-cl` на standalone `clang++` (сейчас clang-cl — MSVC-совместимый фронтенд, требует VS)
- Убрать `vcvarsall.bat x86` из configure.ps1 — нужен для Windows SDK headers/libs
- Заменить VS-bundled `cmake.exe` на системный CMake
- После этого Visual Studio больше не нужна для сборки

## Код — уход от Windows API

### 1. Оконная система + GL контекст (крупное)
- Win32 `CreateWindowEx`, `WndProc`, message loop (`host.cpp`) → SDL3 window + event loop
- WGL контекст (`gl_context.cpp`) → `SDL_GL_CreateContext` (кросс-платформенный GL из коробки)
- SDL3 уже в проекте (геймпады, этап 5.1), но окно пока Win32

### 2. Input (среднее)
- Клавиатура: Win32 `GetAsyncKeyState` / DirectInput → SDL3 keyboard events
- Геймпад: уже на SDL3 ✅
- `DIManager.cpp` (DirectInput) → полностью заменить на SDL3

### 3. File I/O (механическое)
- `MFFileHandle` (Win32 `HANDLE`) → стандартные `fopen`/`fread` или SDL_IO
- Пути файлов: `\\` разделители → `/` или SDL path functions
- `sprintf("%s\\data\\%s")` → кросс-платформенные пути

### 4. Звук
- OpenAL уже кросс-платформенный ✅
- MSS32 → OpenAL Soft (уже сделано)

### 5. Мелочи
- `_alloca` → `alloca` / кросс-платформенный аналог
- Win32 типы (`DWORD`, `HANDLE`) → stdint (`uint32_t`, etc.)
- `GetTickCount` → SDL_GetTicks или std::chrono
- Registry (`env.cpp`) → ini-файл или SDL preferences
- `WinMain` → SDL3 main (SDL_main)

### Оценка объёма
Самое крупное — пункты 1-2 (окно + GL контекст + input). SDL3 закрывает оба.
Файловый слой — механическая работа. По объёму — несколько сессий, не месяцы.

**Критерий:** компилируется и работает на Linux и macOS без Visual Studio.
