# Stage 11 — Подготовка к релизу (девлог)

## 2026-04-03: Переименование бинарника, система упаковки, лицензии

### Переименование Fallen → OpenChaos

Бинарник переименован из `Fallen` / `Fallen.exe` в `OpenChaos` / `OpenChaos.exe`.

Изменено:
- `CMakeLists.txt` — `project()`, `add_executable()`, все target-ссылки
- `Makefile` — `EXE_NAME`
- Скиллы: `debugger`, `screenshot`, `asan` — имя процесса в командах
- `CLAUDE.md`, `SETUP.md` — документация
- `stage8.md`, `stage5_1_dualsense.md` — актуальные описания (девлоги не тронуты)

### Лицензии

- Добавлен FFmpeg (LGPL-2.0) в `THIRD_PARTY_LICENSES.md` — ранее отсутствовал
- Решение по LGPL: проект open source (MIT), исходники и инструкция сборки доступны — покрывает требования LGPL (аналогично PPSSPP). Windows — динамическая линковка (DLL), macOS — статическая (.a)
- Исследованы реальные проекты (PPSSPP, OpenMW, VLC, mpv, Citra) — все open source и не заморачиваются с LGPL compliance сверх предоставления исходников

### Система упаковки релизов

Создана инфраструктура для сборки релизных архивов:

**Структура:**
```
release/
  release.mk              — Makefile target для упаковки
  assets/
    windows-x64/README.txt — README для Windows-архива
    macos-arm64/
      README.txt           — README для macOS-архива
      OpenChaos.command    — лаунчер для macOS (даблклик без Terminal-танцев)
  dist/                    — (gitignored) выходные zip-архивы
```

**Команда:** `make release-package VERSION=0.1.0`
- Определяет платформу автоматически
- Проверяет наличие Release-билда
- Очищает `dist/` перед каждой упаковкой
- Копирует бинарник + DLL (Windows) + README + .command (macOS)
- Подставляет версию в README через `sed` (шаблон `{{VERSION}}`)
- Результат: `release/dist/OpenChaos-v0.1.0-<platform>.zip`

**Windows-архив:** `OpenChaos.exe` + DLL (SDL3, OpenAL32, fmt, avcodec/avformat/avutil/swresample/swscale)
**macOS-архив:** `OpenChaos` + `OpenChaos.command` (всё статически слинковано, DLL не нужны)

### macOS: .command лаунчер

Файл `OpenChaos.command` — скрипт который при даблклике:
1. Снимает quarantine-атрибут с бинарника (`xattr -dr com.apple.quarantine`)
2. Делает `chmod +x`
3. Запускает игру

Ограничение: открывает окно Terminal (`.command` файлы так работают). В планах — `.app` bundle.

### Рефакторинг Makefile

Разбит на модули:
- `Makefile` — переменные, build/run/copy-resources + includes
- `make/platform.mk` — определение ОС, архитектуры, имени exe, тулчейна
- `make/configure.mk` — vcpkg detection, configure-opengl/d3d6/asan
- `release/release.mk` — release-package target

Добавлены проверки: `build-*` команды выдают понятную ошибку если не сделан `configure-opengl`.
Добавлены Usage-комментарии над каждой командой.

### README в архивах

Отдельные README для каждой платформы (plain text, `.txt`):
- Инструкция по запуску (Windows: скопировать в Steam-папку; macOS: скопировать ресурсы с Windows)
- Предупреждение про моды/DLL-обёртки (dgVoodoo, ReShade)
- Gatekeeper на macOS — инструкция с right-click → Open
- Legal: торговая марка, правообладатель, дисклеймер
- Ссылки на MIT лицензию и THIRD_PARTY_LICENSES.md

### README.md проекта

Добавлена секция "How to play" со ссылкой на GitHub Releases.

### Версионирование

- Semver: `v0.1.0` — первый публичный релиз
- GitHub Releases с тегами, пометка pre-release пока < 1.0
- Процесс: сборка на каждой машине → `make release-package` → upload через `gh release create` или UI
