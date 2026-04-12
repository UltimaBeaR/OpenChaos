# Этап 11 — Подготовка к первому альфа-релизу (v0.1.0)

**Цель:** подготовить проект к первому публичному альфа-релизу.

**Результат:** выпущена alpha v0.1.0 (2026-04-04). Платформы: Windows x64, macOS arm64.

## Лицензии и атрибуция

- ✅ `THIRD_PARTY_LICENSES.md` — собраны все лицензии: SDL3 (zlib), OpenAL Soft (LGPL-2.0),
  FFmpeg (LGPL-2.0), fmt (MIT, transitive dep of OpenAL), Dualsense-Multiplatform (MIT), miniaudio (Public Domain)
- ✅ Проверить что `LICENSE` в корне корректен (наш copyright + оригинальный)
- ✅ Проверить что `original_game/LICENSE` на месте
- ✅ Проверить что vendored библиотеки (`new_game/libs/`) содержат свои LICENSE файлы
- ✅ Убедиться что `README.md` и `CONTRIBUTORS.md` актуальны

## Лицензионные ограничения на линковку

- **OpenAL Soft и FFmpeg (LGPL-2.0):** проект open source (MIT) — исходники и инструкция
  сборки доступны, пользователь может пересобрать с любой версией библиотеки.
  Это покрывает требования LGPL (аналогично PPSSPP и другим open source проектам).
  Windows: динамическая линковка (DLL). macOS: статическая линковка (.a).
- При добавлении новых зависимостей — проверять лицензию на совместимость.

## Сборка и дистрибуция

- Ручная сборка на каждой платформе → `make build-release` + `make release-package VERSION=X.Y.Z`
- Формат: portable zip (бинарник + DLL на Windows, бинарник + .command на macOS)
- Пользователь копирует содержимое архива в папку с игрой из Steam
- Версионирование: semver, теги `vX.Y.Z`, GitHub Releases (pre-release пока < 1.0)
- Структура: `release/` — скрипты упаковки, ассеты (README, .command), `release/dist/` — выходные zip

## Linux

- ✅ Добавлена сборка и пакет для Linux
- ✅ Проверено на стандартном Linux-десктопе
- ✅ Проверено на Steam Deck — играбельно в desktop mode

## Документация

- ✅ Пользовательский README с инструкцией по установке и запуску
