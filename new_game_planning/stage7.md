# Этап 7 — Новый рендерер

**Цель:** заменить DirectDraw/D3D6 рендерер на OpenGL через минимальную абстракцию.

**Подход:** итеративно вытянуть все D3D вызовы из игрового кода в модуль `graphics_engine`,
затем написать OpenGL реализацию того же контракта. Подробно → [stage7_log.md](../new_game_devlog/stage7_log.md)

**Выбранный графический API:** OpenGL (версия уточняется, предварительно 3.3–4.1).

---

## Архитектура

```
engine/graphics/graphics_engine/
├── graphics_engine.h               — контракт + render state cache (единственное что видит игровой код)
├── graphics_engine.cpp             — реализация API-agnostic частей (GERenderState)
└── backend_directx6/                            — D3D6 бэкенд (29 файлов)
    ├── graphics_engine_d3d.cpp     — реализация ge_* контракта
    ├── display.cpp/gd_display.h    — DDraw display management
    ├── d3d_texture.cpp/h           — D3D texture class
    ├── dd_manager.cpp/h            — DDraw driver/device enumeration
    ├── vertex_buffer.cpp/h         — D3D vertex buffer pool
    ├── polypage.cpp                — PolyPage::Render/DrawSinglePoly (D3D draw calls)
    ├── texture.cpp                 — texture loading (D3DTexture methods)
    ├── text.cpp                    — bitmap text renderer (D3D VB + textures)
    ├── truetype.cpp/globals        — TrueType text renderer (DDraw surfaces)
    ├── work_screen.cpp/h           — offscreen DDraw work surface
    └── *_globals.cpp/h             — global state for each module
```

- CMake-флаг выбирает какой .cpp компилировать (compile-time switch)
- Никакого runtime dispatch, никаких виртуальных интерфейсов
- Для сравнения: переключил флаг → пересобрал → сравнил

### Принципы контракта

1. Абстракции в терминах "что нужно игре", не "что умеет D3D"
2. Непрозрачные хэндлы (TextureHandle) вместо API-специфичных указателей
3. Свои типы (Vertex, BlendMode, DepthMode) вместо D3D типов
4. Не проектировать "на вырост" — только то что реально вызывается

Подробно с таблицей хороших/плохих абстракций → [stage7_log.md](../new_game_devlog/stage7_log.md)

---

## План работы

### Шаг 1 — Отключить outro ✅
Закомментировать outro в CMakeLists.txt. Сосредоточиться на основной игре.

### Шаг 2 — Выделить graphics_engine (D3D) для основной игры ✅
Итеративно вытаскивать D3D вызовы из игрового кода в graphics_engine.
Цель: D3D хедеры включаются ТОЛЬКО в backend_directx6/ папке.
**Результат:** полный аудит 9 категорий D3D кода — 0 утечек вне backend_directx6/.
Весь игровой код видит только `graphics_engine.h` с чистыми типами.
Подробно → [stage7_log.md](../new_game_devlog/stage7_log.md)

### Шаг 2.5 — Доработки бэкенда (до outro)

**A. Убрать игровую логику из бэкенда ✅:**
- `texture.cpp` — вернулся в assets/, 12 новых ge_* обёрток для D3DTexture
- `display.cpp` — убраны panel/input/poly includes, заменены на callback-механизм
- `polypage.cpp` — AENG_total_polys_drawn заменён на callback
- Повторный аудит 28 файлов: 0 игровой логики в бэкенде

**B. Проверка заменяемости на OpenGL:**
Пройтись по каждому .cpp в бэкенде и оценить: можно ли реализовать на OpenGL,
нужны ли изменения в ge_* контракте. (Ещё не сделано.)

**C. Визуальные регрессии:**
После переноса кода в backend_directx6/ появились визуальные проблемы при запуске.
Найти и исправить ошибки, допущенные при переносе.

### Шаг 3 — Включить outro, выделить graphics_engine для него
Раскомментировать outro, вытянуть его D3D вызовы в тот же контракт.
Проверка: всё работает как раньше.

### Шаг 4 — OpenGL реализация для основной игры
Отключить outro. Написать graphics_engine_opengl.cpp.
Переключить CMake на OpenGL. Запустить, проверить, фиксить.

### Шаг 5 — OpenGL реализация для outro
Включить outro. Дописать OpenGL для outro-специфичных функций.

### Шаг 6 — Финализация
Удалить D3D бэкенд. Убрать DirectX зависимости.

---

## Инвентаризация (выполнена)

- 44 файла с D3D/DDraw (6.6% кодовой базы), ~1600 ссылок
- DrawPrimitive: 29 вызовов (компактно)
- SetRenderState: 802+ вхождений (доминирует, но через макросы)
- Две независимые D3D-системы: основная игра + outro
- Три формата вершин: TLVERTEX, LVERTEX, VERTEX
- Подробно → [stage7_log.md](../new_game_devlog/stage7_log.md)

---

**Правила переноса (обязательно читать!):** → [stage7_renderer_rules.md](stage7_renderer_rules.md)

**Критерий завершения:** игра и outro работают на OpenGL. Визуально идентично D3D версии.
DirectX зависимости удалены.
