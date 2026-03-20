# Этап 8 — Кросс-платформа

**Цель:** убрать Windows-специфичный код и зависимость от Visual Studio.

## Сборочная система — уход от VS

- Заменить `clang-cl` на standalone `clang++` (сейчас clang-cl — MSVC-совместимый фронтенд, требует VS)
- Убрать `vcvarsall.bat x86` из configure.ps1 — нужен для Windows SDK headers/libs
- Заменить VS-bundled `cmake.exe` на системный CMake
- После этого Visual Studio больше не нужна для сборки

## Код — уход от Windows API

- `WinMain` → SDL3 main
- Windows API вызовы → SDL3 / std::
- `DIManager.cpp` (DirectInput) → SDL3 input
- Звук: MSS32 → OpenAL Soft (или miniaudio)

**Критерий:** компилируется и работает на Linux и macOS без Visual Studio.
