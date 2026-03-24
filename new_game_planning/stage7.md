# Этап 7 — Новый рендерер

**Цель:** заменить DirectX рендерер на OpenGL/Vulkan через абстракцию.

## Пре-стадия: архитектурный рефакторинг графики

**Необходимо сделать ДО замены рендерера.** Без этого нельзя заменить D3D — вызовы размазаны по всему коду.

### Разбить `engine/platform/uc_common.h` (бывший MFStdLib.h)

Umbrella-заголовок, 145 файлов его включают. Содержит три разных вещи:
1. Windows/D3D includes + platform defines — остаётся в platform
2. Display/WorkScreen API — должно уйти в graphics
3. Подтягивание keyboard, mouse, math, memory, file — umbrella-convenience, каждый потребитель должен включать только то что ему нужно

Разбить на части → обновить 145 файлов-потребителей, каждому подобрав конкретные includes.

### Разделение рендер-логики и D3D в `engine/graphics/`

Сейчас D3D вызовы перемешаны с логикой в pipeline/geometry/resources:
- `graphics_api/` — GDisplay, DDManager, host, work_screen, vertex_buffer (чистый D3D)
- `pipeline/` — poly, bucket, polypage (D3D вперемешку с рендер-логикой)
- `geometry/` — facet, mesh, figure (D3D вперемешку с геометрической логикой)
- `resources/` — d3d_texture, font (D3D текстуры и шрифты)

Целевое разделение:
- `pipeline/` — **ЧТО рисовать и в каком порядке**: bucket sort, z-сортировка, polypage батчинг. Логика рендер-пайплайна, не привязанная к конкретному API.
- `geometry/` — **КАКУЮ геометрию строить**: facet строит стены зданий, mesh строит объекты, cone строит конусы света.
- `resources/` — управление ресурсами: часть останется (логика), часть уйдёт в graphics_engine (d3d_texture и т.д.)

Задача: вырезать D3D вызовы из функций в pipeline/geometry/resources. Функции разрезаются на логическую часть (остаётся) и D3D часть (уходит за абстракцию).

### Мутации стейта

- Перенос мутаций стейта из рендерера в update фазу
  (конкретные места: `drawxtra.cpp`, `PYRO_draw_armageddon()`)

## Основная стадия: замена рендерера

### План

1. Определить абстракцию в `engine/graphics/graphics_api/` (интерфейсы: submit vertices, bind texture, draw)
2. Создать `engine/graphics/graphics_engine/` — реализация абстракции под D3D, `graphics_api/` вложена внутрь
3. Подключить pipeline/geometry/resources к абстракции (после пре-стадии D3D уже вырезан)
4. Добавить OpenGL реализацию в `graphics_engine/`

### Итоговая структура `engine/graphics/`:

```
engine/graphics/
├── graphics_engine/       — D3D/OpenGL реализации, инкапсулированы, наружу не торчат
│   └── graphics_api/      — абстракция (интерфейсы), используется только внутри engine
├── pipeline/              — рендер-логика, работает только через абстракцию
├── geometry/              — геометрическая логика
└── resources/             — текстуры, шрифты
```

**Выбор API:** OpenGL 4.1 vs Vulkan+MoltenVK — решить перед стартом этапа.
Подробно → [`tech_and_architecture.md`](tech_and_architecture.md)

**Критерий:** тестовая инфраструктура из этапа 6 проходит без регрессий.
