# Этап 12 — Linux/Steam Deck: дев-лог

## Сессия 2026-04-10 — Первый нативный Linux билд

### Среда разработки (Steam Deck)

VS Code работает как Flatpak (`com.visualstudio.code`, Freedesktop SDK 25.08).
Flatpak имеет `features=devel` и `filesystems=host` — расширения SDK доступны.

**Clang 20.1.8:**
- Установлен через Flatpak SDK расширение: `flatpak install flathub org.freedesktop.Sdk.Extension.llvm20//25.08`
- Файлы расширения в OSTree store недоступны напрямую из Flatpak-сандбокса
- Решение: скопировать бинари и либы из расширения + создать wrapper-скрипты:
  ```
  ~/.local/bin/clang      — wrapper (LD_LIBRARY_PATH + exec)
  ~/.local/bin/clang++    — wrapper (LD_LIBRARY_PATH + exec)
  ~/.local/bin/llvm20/    — скопированные бинари (clang, clang++, clang-20, lld)
  ~/.local/bin/llvm20/lib/ — libclang-cpp.so.20.1, libLLVM-20.so, libLLVM.so.20.1
  ~/.local/bin/lib/clang/20 — симлинк на ~/.local/bin/llvm20/lib/clang/20 (resource dir)
  ```
- `~/.local/bin` добавлен в PATH через Makefile (только для Linux)

**vcpkg:**
- Склонирован в `~/vcpkg` с `--depth=1`
- Для baseline-коммита из `vcpkg.json` нужен отдельный fetch:
  ```bash
  cd ~/vcpkg && git fetch --depth=1 origin 7d1d86a0786d928fd81c9897ba089422716a0755
  ```
- Bootstrap: `~/vcpkg/bootstrap-vcpkg.sh -disableMetrics`
- Автодетект в `configure.mk`: ищет `~/vcpkg/scripts/buildsystems/vcpkg.cmake`

**Зависимости (vcpkg, x64-linux трипл):** SDL3, OpenAL, FFmpeg — все собрались через GCC 15.2
(vcpkg использует системный компилятор `/usr/bin/c++` для x64-linux трипла, игра — Clang 20)

### Что было сделано

1. **`new_game/cmake/clang-x64-linux.cmake`** — создан (аналог Windows/macOS toolchain)
2. **`make/configure.mk`:**
   - Linux vcpkg-детект (auto: `~/vcpkg`)
   - ASan оставлен только на Windows (`configure-asan` → ошибка на не-Windows)
   - `-DCMAKE_MAKE_PROGRAM=$(shell which ninja)` — ninja не находится cmake без явного указания
   - Подсказка для Linux при отсутствии vcpkg
3. **`Makefile`:** `export PATH := $(HOME)/.local/bin:$(PATH)` для Linux
4. **`src/engine/core/macros.h`:** убраны `min`/`max` макросы
   - Конфликтовали с `std::min`/`std::max` в libstdc++ (`stl_algobase.h`)
   - На Windows: MSVC stdlib защищена `push_macro`, libstdc++ — нет
   - 3 использования в `mfx.cpp` заменены на `std::min`/`std::max`
5. **`src/engine/platform/host.cpp`:** добавлен backtrace (`<execinfo.h>`) в Linux crash handler

### Результат сборки

- `make configure` — OK (Clang 20.1.8 подхватился, все зависимости установлены за ~16 мин)
- `make build-debug` — OK (`[316/316] Linking CXX executable Debug/OpenChaos`)
- Игра запускается: OpenGL 4.6 Core (Mesa 25.3.5, radeonsi/vangogh), OpenAL OK

### Известные проблемы (после первого запуска) — РЕШЕНЫ

1. ~~**Чёрный фон в главном меню**~~ — case-sensitive file I/O. Файлы ресурсов `TITLE LEAVES1.TGA` (uppercase), код ищет `title leaves1.tga` (lowercase). **Исправлено:** `fopen_ci()`.
2. ~~**Краш при запуске миссии**~~ — та же причина: файлы ресурсов не находились. **Исправлено:** `fopen_ci()`.

## Сессия 2026-04-11 — Case-insensitive file I/O + документация

### Case-insensitive file I/O

Реализована функция `fopen_ci()` в `engine/io/file.cpp` (объявлена в `file.h`):
- На Linux/macOS: если обычный `fopen` не нашёл файл, `resolve_path_ci()` рекурсивно резолвит **каждый** компонент пути через `opendir`/`readdir`/`strcasecmp`
- На Windows: прямой `fopen` (FS case-insensitive)

Все `fopen` вызовы в проекте заменены на `fopen_ci`:
- Файловый движок: `FileOpen`, `FileExists`, `FileCreate`, `FileDelete`, `MF_Fopen`
- Outro: `outro_imp.cpp`, `outro_tga.cpp`
- TGA запись: `tga.cpp`
- Config: `env.cpp`
- `stat()` в `frontend.cpp` → `fstat(fileno(file))` по открытому handle

Результат: меню, загрузка миссий, геймплей, outro — всё работает на Steam Deck.

### Документация

- `new_game/SETUP.md` — добавлена секция Linux (clang, vcpkg, системные библиотеки, Steam Deck)
- `README.md` — Linux добавлен в таблицу платформ
- `release/assets/linux-x64/` — README.txt + OpenChaos.sh лаунчер
- `release/release.mk` — копирование `.sh` скриптов для Linux
- `known_issues_and_bugs.md` — секция Linux-специфичные проблемы
- `stage8.md` — пункт 16a ✅, таблица верификации обновлена
- `stages.md` — Linux добавлен в статус этапа 8
