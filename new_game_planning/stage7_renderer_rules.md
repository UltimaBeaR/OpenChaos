# Этап 7 — Правила переноса графического API

> **Примечание (2026-04-08):** D3D6 бэкенд удалён из кодовой базы. OpenGL 4.1 — единственный бэкенд.
> Правила ниже — исторический референс процесса переноса.

## 3 главных правила

### 1. Весь D3D/DirectDraw код — ТОЛЬКО в папке backend_directx6/

Путь: `new_game/src/engine/graphics/graphics_engine/backend_directx6/`

**Ни один файл вне этой папки не должен:**
- Включать `<d3d.h>`, `<ddraw.h>`, `<d3dtypes.h>`
- Включать любой хедер из `engine/graphics/graphics_engine/backend_directx6/`
- Использовать D3D/DDraw типы: `LPDIRECT3D*`, `LPDIRECTDRAW*`, `D3D*`, `DD*`, `HRESULT` (D3D)
- Обращаться к `the_display` или любым D3D глобалам напрямую
- Содержать D3D константы: `D3DRENDERSTATE_*`, `D3DFVF_*`, `D3DPT_*`, `DDBLT_*` и т.д.

**Исключение:** `outro/` — разбирается отдельно (outro отключен).

Если файл вне backend_directx6/ нуждается в функциональности D3D — он вызывает `ge_*` функцию из `graphics_engine.h`.

### 2. Никакой игровой логики в backend_directx6/

Папка `backend_directx6/` содержит **только**:
- Реализацию ge_* контракта для D3D6
- D3D-специфичные классы (Display, D3DTexture, VertexBufferPool, DDDriverManager)
- Код рендеринга который целиком D3D-специфичен (polypage Render/DrawSinglePoly)

**НЕ должно быть в backend_directx6/:**
- AI, физика, игровая логика, управление сущностями
- Загрузка уровней, миссий, скриптов
- UI/меню логика (кроме display mode enumeration)

Если файл содержит СМЕСЬ game logic + D3D — его нужно РАЗРЕЗАТЬ:
- Game logic остаётся на месте, вызывает ge_*
- D3D реализация уходит в backend_directx6/

### 3. Контракт ge_* должен быть реализуем на OpenGL

Каждая функция в `graphics_engine.h` должна иметь осмысленную реализацию и для D3D, и для OpenGL.

**Нельзя:**
- `ge_get_d3d_device()` — D3D-специфичное API
- `ge_lock_ddraw_surface()` — DDraw-специфичная операция
- Передавать D3D типы через ge_* интерфейс

**Можно:**
- `ge_lock_screen()` — абстракция framebuffer access (D3D: Lock surface, OpenGL: glReadPixels/PBO)
- `ge_create_screen_surface(pixels)` — абстракция offscreen surface (D3D: CreateSurface, OpenGL: texture/FBO)
- `ge_draw_indexed_primitive(type, verts, indices)` — абстракция draw call

**Opaque handles:** `GETextureHandle`, `GEScreenSurface` — uintptr_t, бэкенд маппит на свои типы.

---

## Как проверять соблюдение правил

```bash
# Правило 1: никаких backend_directx6/ includes вне backend_directx6/ и outro/
grep -rn 'include.*graphics_engine/d3d\|include <d3d\|include <ddraw' \
  new_game/src/ --include='*.cpp' --include='*.h' \
  | grep -v 'graphics_engine/backend_directx6/' | grep -v 'outro/'
# Результат должен быть пустым.

# Правило 1 (дополнительно): никаких D3D типов вне backend_directx6/
grep -rn 'the_display\.\|LPDIRECT3D\|LPDIRECTDRAW\|REALLY_SET_\|D3DRENDERSTATE_' \
  new_game/src/ --include='*.cpp' --include='*.h' \
  | grep -v 'graphics_engine/backend_directx6/' | grep -v 'outro/'
# Результат должен быть пустым.
```

---

## 4. Фиксы визуальных регрессий — валидация по старому коду

**При исправлении любого визуального бага после рефакторинга рендер-движка:**
1. Сначала найти подозрительное место в текущем коде
2. Проверить по старому коду (тег `stage_5_1_done` в git) — как было ДО переноса
3. Убедиться что видна конкретная ошибка переноса (неправильная замена, потерянный вызов, изменённый порядок)
4. Только после этого фиксить — осознанно, а не вслепую

**Не фиксить вслепую по догадкам.** Каждый фикс = найденная ошибка переноса.
**Не добавлять новые ge_* вызовы, новую логику или другие D3D вызовы.**
Только восстанавливать то что было потеряно/искажено при переносе.
Эксперименты для диагностики — можно, но финальный коммит = только подтверждённые ошибки переноса.

Путь к старому коду: `git show stage_5_1_done:new_game/src/<old_path>`
Старая структура бэкенда: `engine/graphics/graphics_api/` (не `backend_directx6/`)

---

## Что читать при начале новой сессии

1. `CLAUDE.md` — общие правила проекта
2. `new_game_planning/stages.md` — текущий этап (8, кросс-платформа; этап 7 — основное сделано)
3. `new_game_planning/stage7.md` — план этапа 7
4. **Этот файл** (`stage7_renderer_rules.md`) — 3 правила переноса
5. `new_game_devlog/stage7_log.md` (конец файла) — текущий прогресс и оставшиеся задачи
6. `engine/graphics/graphics_engine/graphics_engine.h` — текущий ge_* контракт
