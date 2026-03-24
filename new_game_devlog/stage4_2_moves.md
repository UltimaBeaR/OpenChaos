# Этап 4.2 — Таблица перемещений

Текущий путь → новый путь (относительно `src/`).
Обновляется по мере принятия решений о перемещениях.

| Текущий путь | Новый путь | Причина |
|---|---|---|
| `core/*` | `engine/core/*` | engine — самодостаточный движок-комбайн, core его фундамент |
| `platform/*` | `engine/platform/*` | платформенная абстракция — часть движка |
| `engine/graphics/graphics_api/wind_procs.*` | `engine/platform/wind_procs.*` | Win32 WndProc — platform, не graphics |
| `actors/*` | `things/*` | Thing — оригинальное название системы в коде, точнее чем "actors" |
| `engine/platform/platform.h` | `engine/platform/uc_common.h` | Не платформенная абстракция, а легаси umbrella (бывший MFStdLib.h) |
| `engine/lighting/*` | `engine/graphics/lighting/*` | Чисто визуальная подсистема — к графике |
