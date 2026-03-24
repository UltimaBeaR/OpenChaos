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
| `engine/graphics/geometry/bloom.*` | `engine/graphics/postprocess/bloom.*` | Постобработка, не геометрия |
| `engine/graphics/pipeline/wibble.*` | `engine/graphics/postprocess/wibble.*` | Постобработка, не геометрия |
| `engine/graphics/pipeline/text.*` | `engine/graphics/text/text.*` | Текстовый рендерер, к шрифтам |
| `engine/graphics/resources/font*` | `engine/graphics/text/font*` | Шрифты → text/ |
| `engine/graphics/resources/menufont*` | `engine/graphics/text/menufont*` | Шрифты → text/ |
| `engine/graphics/resources/truetype*` | `engine/graphics/text/truetype*` | Шрифты → text/ |
| `engine/graphics/resources/console.*` | `engine/console/console.*` | Дебаг-подсистема движка, не графика |
| `engine/graphics/pipeline/message.*` | `engine/console/message.*` | Дебаг текстовый вывод |
| `engine/graphics/pipeline/sky.*` | `engine/graphics/geometry/sky.*` | Создаёт визуальный вывод |
| `engine/graphics/pipeline/vertex_buffer.*` | `engine/graphics/graphics_api/vertex_buffer.*` | Чистый D3D |
| `engine/graphics/pipeline/render_state.*` | `engine/graphics/graphics_api/render_state.*` | Чистый D3D |
| `engine/graphics/resources/d3d_texture.*` | `engine/graphics/graphics_api/d3d_texture.*` | Чистый D3D |
| `engine/graphics/graphics_api/host.*` | `engine/platform/host.*` | Создание окна, не graphics API |
