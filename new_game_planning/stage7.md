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

**C3 — Alpha/color key ✅ ИСПРАВЛЕН (проверено, совпадает со старой версией):**
3 ошибки переноса render_state.cpp → graphics_engine.cpp:
- `ge_set_color_key_enabled` — не была создана, MAYBE_FLUSH был no-op `{}`
- `ge_set_alpha_test_enabled` — аналогично
- `ge_set_alpha_ref(0x07)` + `ge_set_alpha_func(Greater)` — не переносились из InitScene
Все подтверждены diff'ом с stage_5_1_done.

**C2 — Шрифты/листики/декорации в меню при первом запуске ✅ ИСПРАВЛЕН (проверено):**
Корневая причина: `ge_set_viewport` использовал NDC clip values (-1, h/w, 2, 2*h/w),
а старый код (attract.cpp, FRONTEND_display) ставил screen-space clip values (0, 0, 640, 480).
dgVoodoo (D3D6→D3D11 wrapper) не рендерил пиксели с неправильными clip values.
Полигоны добавлялись корректно (78 font polys, 12 leaf polys), DrawIndexedPrimitiveVB
возвращал S_OK, но пиксели не появлялись на экране.
Фикс: `ge_set_viewport` теперь использует screen-space clip values (как в stage_5_1_done).
Подтверждено diff'ом с `stage_5_1_done:attract.cpp` и `stage_5_1_done:frontend.cpp`.

**C4 — Листья на земле ✅ ИСПРАВЛЕН (проверено):**
Корневая причина та же что и C2: `POLY_camera_set` в старом коде вызывал
`SetViewport2` напрямую с NDC clip values (-1, 1, 2, 2), но после переноса
стал вызывать `ge_set_viewport` который форсировал screen-space clip values.
3D рендеринг получал неправильный clip volume → листья (и возможно другая
геометрия) не проходили clipping.
Фикс: добавлена `ge_set_viewport_3d` с явными clip параметрами.
`POLY_camera_set` вызывает `ge_set_viewport_3d(-1, 1, 2, 2)` — как в stage_5_1_done.
2D код (attract/frontend) продолжает использовать `ge_set_viewport` со screen-space clips.

**C5 — Загрузочный экран просвечивает через первые кадры ✅ ИСПРАВЛЕН (проверено):**
Back buffer содержал остатки загрузочного экрана на первом кадре — аддитивное
небо (One, One) выявляло их. Точная ошибка переноса не найдена (все D3D вызовы 1:1
идентичны — подтверждено ревью + прямой подстановкой), но clear работает только после
EndScene основной render-сцены. Фикс: `ge_clear(true, false)` между EndScene основного
рендера и `POLY_frame_draw_odd()` (`aeng.cpp`), только на первом кадре.
Подробности → [stage7_c5_investigation.md](../new_game_devlog/stage7_c5_investigation.md)

**C6 — Края прозрачности ✅ НЕ БАГ (проверено):**
Пикселизированные вырезы — так же выглядят в старой версии. Показалось.

**Правило:** каждый фикс валидируется по `stage_5_1_done`. Подробнее → `stage7_renderer_rules.md` §4.

**Сводка багов:** C1 ✅, C2 ✅, C3 ✅, C4 ✅, C5 ✅, C6 ✅ не баг. **Все баги исправлены.**

### После фикса всех багов — ПОЛНАЯ СВЕРКА

Тотальная построчная сверка всех изменений Stage 7.
Цель: убедиться что вынос D3D кода в graphics_engine — **чистый рефакторинг**
с нулевыми изменениями в рантайм-поведении DirectX вызовов.

**Ветка:** `TEMP_FOR_REVIEW` — squashed коммит всех изменений Stage 7 (только код, без документации).
Один коммит = один чистый diff для ревью.

**Что проверять:**

1. **3 правила из `stage7_renderer_rules.md`:**
   - Правило 1: Никакого D3D/DDraw кода вне `backend_directx6/` (и `outro/`)
   - Правило 2: Никакой игровой логики внутри `backend_directx6/`
   - Правило 3: Контракт ge_* реализуем на OpenGL

2. **Рантайм-эквивалентность DirectX вызовов (главное):**
   - Все вызовы DirectX в новой версии (внутри движка) должны 1:1 матчиться
     с тем что было ДО выноса в движок
   - Порядок вызовов D3D API не изменён
   - Ничего не забыто (пропущенные вызовы)
   - Ничего лишнего не добавлено
   - Значения параметров не изменены
   - Это должен быть **чистый рефакторинг** — zero runtime changes

3. **Построчная проверка каждого hunk:**
   - Каждое изменение: понять что было → что стало → совпадает ли поведение
   - Render states, texture stage states, vertex formats, draw calls
   - Порядок инициализации, значения по умолчанию

**Процесс:**
- Идти по диффу строчка за строчкой
- Промежуточные результаты писать в `new_game_devlog/stage7_review_results.md`
- Ошибки/несхождения — фиксировать, не прерывая ревью
- Сомнения/вопросы — фиксировать, не прерывая ревью
- По итогу — полный отчёт: что найдено, что требует фикса, что вызывает вопросы
- Если нужны изменения в абстракции — тоже записать, это допустимо

### Шаг 2.6 — Убрать дублирование DisplayWidth/DisplayHeight ✅

`DisplayWidth`/`DisplayHeight` (константы 640/480) были определены в 9 местах из-за
исторического конфликта между MFStdLib (`extern SLONG`) и GDisplay.h (`#define`).
Сведено к одному `#define` в `uc_common.h`. Убраны: extern-объявление, переменные
из `display_globals.cpp`, 7 локальных `#define` (sky, frontend, game_tick, eng_map,
sprite, wibble, gd_display.h). Outro не затронуто — там используется только
`RealDisplayWidth`/`RealDisplayHeight`.

### Шаг 3 — Включить outro, выделить graphics_engine для него
Раскомментировать outro, вытянуть его D3D вызовы в тот же контракт.
Проверка: всё работает как раньше.

### Шаг 4 — OpenGL реализация для основной игры
Отключить outro. Написать graphics_engine_opengl.cpp.
Переключить CMake на OpenGL. Запустить, проверить, фиксить.

**⚠️ Заметки из ревью Stage 7 — учесть при реализации:**
- **TriangleFan:** `GEPrimitiveType::TriangleFan` нет в OpenGL 3.3 core profile. Сейчас 0 вызовов — удалить из enum, или конвертировать в triangle list в бэкенде.
- **Color key:** `ge_set_color_key_enabled` — DDraw-концепция, в OpenGL нет. Конвертировать color key → alpha при загрузке текстур, а рантайм toggle сделать no-op.
- **Perspective correction:** `ge_set_perspective_correction` — на современных GPU всегда perspective-correct. Сделать no-op.
- **16-bit текстуры:** `ge_lock_texture_pixels` возвращает `uint16_t**`. OpenGL бэкенд скорее всего 32-bit (RGBA8). Нужно менять интерфейс или конвертировать.
- **Windows API:** `ge_to_gdi()`/`ge_from_gdi()` — переименовать в `ge_release_display()`/`ge_reacquire_display()`. `ge_update_display_rect(void* hwnd)` — абстрагировать platform handle.
- **Capability queries:** `ge_supports_dest_inv_src_color()`, `ge_supports_modulate_alpha()`, `ge_is_hardware()` — всегда true на современном железе. Захардкодить или убрать, упростив code paths.

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
