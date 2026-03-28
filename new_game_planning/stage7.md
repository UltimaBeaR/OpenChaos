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

**B. Проверка заменяемости на OpenGL ✅:**
Контракт `graphics_engine.h` не требует изменений — opaque handles, API-agnostic типы.
Все 9 .cpp файлов бэкенда заменяемы. Блокеров нет.
Сложные файлы: display.cpp (lifecycle), truetype.cpp (DDraw surfaces), work_screen.cpp (pixel access).
Лёгкие файлы: vertex_buffer, polypage, text — уже абстрагированы.
Для modern GL (3.3+) понадобится шейдерный слой (D3D6 = fixed-function).
Рекомендуемый порядок: vertex_buffer → polypage → graphics_engine_opengl → display → textures → truetype.
Контракт менять заранее не нужно — он реализуем на OpenGL как есть.
Потенциальные улучшения (GETextureBlend → шейдеры, vertex padding, fog/specular) —
сделать по месту при написании OpenGL бэкенда, когда будет ясно что реально нужно.

**C. Визуальные регрессии (3 бага):**

**C1 — Viewport/scaling ✅ ИСПРАВЛЕН (проверено):**
Callbacks регистрировались после OpenDisplay → SetScaling уходил в null.
Перенесено до OpenDisplay. Ошибка подтверждена по stage_5_1_done.

**C3 — Alpha/color key ✅ ИСПРАВЛЕН (обрезка ок, плавность краёв — ждёт сравнения со старой версией):**
3 ошибки переноса render_state.cpp → graphics_engine.cpp:
- `ge_set_color_key_enabled` — не была создана, MAYBE_FLUSH был no-op `{}`
- `ge_set_alpha_test_enabled` — аналогично
- `ge_set_alpha_ref(0x07)` + `ge_set_alpha_func(Greater)` — не переносились из InitScene
Все подтверждены diff'ом с stage_5_1_done.

**C2 — Шрифты в меню при первом запуске ❌ НЕ ИСПРАВЛЕН:**
Анализ: в attract.cpp при переносе потеряны ~43 строки прямых D3D вызовов
(SetRenderState + SetTextureStageState), заменены на ~11 ge_* вызовов.
Ключевая потеря — 10+ вызовов `dev->SetTextureStageState()`:
- `D3DTSS_COLOROP = D3DTOP_MODULATE`, COLORARG1/2, ALPHAOP, ALPHAARG1/2 и др.
- Эти TSS инициализировали texture pipeline перед первым рендером
- Без них текстуры шрифтов не applied (TSS в неизвестном состоянии после создания девайса)
- `ge_set_texture_blend(Modulate)` ставит legacy `D3DRENDERSTATE_TEXTUREMAPBLEND`,
  а attract.cpp использовал TSS (immediate mode) — два разных механизма
- Решение: нужно восстановить TSS initialization в бэкенде (Display::Create или ge_init),
  чтобы texture pipeline был в known state до первого рендера
- Файлы для сравнения:
  - Старый: `git show stage_5_1_done:new_game/src/ui/frontend/attract.cpp` (строки 66-115)
  - Текущий: `new_game/src/ui/frontend/attract.cpp` (строки 66-82)

**C4 — Листья на земле рисуются странно ❌ НЕ ПРОАНАЛИЗИРОВАН:**
Проваливаются через пол вблизи, плоскость видна над горизонтом вдали.
Возможно матрицы/проекция. Нужен анализ.

**C5 — Негатив (фото-негатив эффект) при загрузке уровня ❓ НУЖНА ПРОВЕРКА:**
На пару кадров сразу после загрузки уровня появляется эффект негативного фото
в определённом месте, потом пропадает. Пользователь не уверен что это было до переноса.
Нужно сравнить со старой версией.

**C6 — Края прозрачности (резкие vs гладкие) ❓ НУЖНА ПРОВЕРКА:**
Пикселизированные вырезы vs гладкие в старой версии. Ждёт сравнения.

**Правило:** каждый фикс валидируется по `stage_5_1_done`. Подробнее → `stage7_renderer_rules.md` §4.

### После фикса всех багов C2-C6 — ПОЛНАЯ СВЕРКА

Тотальная построчная сверка всех изменений Stage 7 (от `stage_5_1_done` до текущего).
Цель: убедиться что при переносе D3D кода в абстракцию не было допущено ошибок
в логике отрисовки, которые ещё не проявились визуально.

**Подготовка:**
- Попросить пользователя создать ветку с засквашенным коммитом всех изменений Stage 7
  (без документации — только код), чтобы видеть один чистый diff
- Или: `git diff stage_5_1_done..HEAD -- new_game/src/` с фильтром по .cpp/.h файлам
- Пройти каждый hunk: понять суть, проверить не изменилась ли логика D3D отрисовки
- Особое внимание: render states, texture stage states, vertex format, draw calls,
  порядок инициализации, значения по умолчанию

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
