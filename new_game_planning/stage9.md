# Этап 9 — Архитектурный рефакторинг

**Цель:** чистая финальная архитектура.

- Чёткое разделение движок / игра
- Рефакторинг Thing системы
- Улучшение тестового покрытия
- Прочие архитектурные улучшения

## Известные задачи

### Вырезать мёртвый мультиплеерный код

`game/network_state_globals.*` — заглушка (CNET_network_game всегда FALSE, CNET_num_players всегда 1).
~50 мест в коде проверяют эти переменные (`if (CNET_network_game)`, циклы по `CNET_num_players`).
Все эти ветки — мёртвый код: мультиплеер удалён. Вырезать ветки, удалить globals, упростить код.

### Вычистить UC-специфику из `engine/physics/`

`collide.cpp` содержит collision response завязанный на UC-геймплей. Общий collision detection — engine, UC-специфичный response — game-слой.

### Инвертировать зависимость engine → game в цепочке запуска

Сейчас: `main.cpp → engine/platform/host.cpp → game/startup.cpp → game/game.cpp`
Engine напрямую вызывает game (HOST_run → MF_main) — нарушение DAG.

Правильно: game регистрирует свой entry point (callback) в engine во время setup,
engine вызывает его когда готов. Зависимость: game → engine, не наоборот.

### Переписать outro на основном движке

`outro/core/` — самодостаточный мини-движок финальных титров (свой рендерер, матрицы, шрифт, TGA-лоадер, камера). Дубликаты движковых систем. Заменить на вызовы основного engine.

### Унификация input pipeline для меню

**Проблема (обнаружена на Этапе 5.1):**

Сейчас клавиатура и геймпад идут в меню двумя параллельными путями, склеенными через `||`:

```cpp
// frontend.cpp, pause menu, gamemenu, widget — везде такой паттерн:
if (Keys[KB_UP] || (input & INPUT_MASK_FORWARDS)) { ... }
```

**Клавиатура:** `Keys[]` → заполняется напрямую Win32/DirectInput → меню читает без посредников.
Нет debounce — зажатая клавиша срабатывает каждый кадр. Пришлось добавить отдельный `kb_dir_ticker`.

**Геймпад (стик):** `the_state.lX/lY` → каждое меню **само** вычисляет деадзону и гистерезис →
ставит биты `INPUT_MASK_*` → проходит через свой `dir_ticker`. Деадзоны/гистерезис дублируются
между frontend.cpp, gamemenu.cpp, pause.cpp.

**Геймпад (D-Pad):** оси `lX/lY` задаются в `gamepad.cpp` (0 или 65535), проходят тот же путь
что и стик, но без проблем вобла (значения дискретные).

**Проблемы текущего подхода:**
- Два раздельных ticker'а (keyboard + gamepad) с разными параметрами в каждом меню
- Деадзона и гистерезис для стика дублируются в каждом файле меню (frontend, gamemenu, pause)
- Кнопки геймпада частично через `INPUT_MASK_*` (через `get_hardware_input`), частично
  через прямое чтение `the_state.rgbButtons[]` — нет единообразия
- `gamemenu.cpp` инжектит `Keys[KB_ESC/UP/DOWN/ENTER] = 1` для совместимости с keyboard-only
  кодом, что создаёт побочные эффекты (кик Дарси при выходе из паузы — пришлось добавлять
  `gamepad_consume_until_released()`)
- Каждое новое меню требует копипасты всей input логики

**Целевая архитектура:**

```
Все устройства → единая очередь menu actions → меню читает actions
```

Один модуль `menu_input` (или расширение gamepad layer):
- Принимает raw input от всех устройств (keyboard `Keys[]`, gamepad `the_state`, мышь в будущем)
- Деадзона, гистерезис, dominant-axis — в одном месте
- Выдаёт `MenuAction` enum: `MENU_UP`, `MENU_DOWN`, `MENU_LEFT`, `MENU_RIGHT`,
  `MENU_CONFIRM`, `MENU_CANCEL`, `MENU_START`
- Repeat/debounce — один раз, с одинаковыми параметрами для всех устройств
- Consumption при закрытии меню — встроен в систему (не отдельный хак)

Меню просто делает:
```cpp
MenuAction action = menu_input_poll();
if (action == MENU_UP) { ... }
if (action == MENU_CONFIRM) { ... }
```

**Файлы для рефакторинга:**
- `ui/frontend/frontend.cpp` — `FRONTEND_input()` (~100 строк input логики)
- `ui/menus/gamemenu.cpp` — `GAMEMENU_process()` (~60 строк gamepad injection)
- `ui/menus/pause.cpp` — `PAUSE_handler()` (~40 строк)
- `ui/menus/widget.cpp` — `FORM_Process()` (~20 строк)
- `engine/input/gamepad.cpp` — `gamepad_consume_until_released()` (костыль, заменить на встроенный механизм)

### C++20 modules

Конвертация .h/.cpp → C++20 modules. Планировалась как часть Этапа 4, но перенесена сюда —
сначала нужна стабильная архитектура (после замены рендерера и чистки зависимостей).
DAG зависимостей уже построен (`tools/st_4_2_dep_graph.py`), структура папок финализирована.
Требует Clang 20+ и CMake 3.28+ (CXX_MODULE_STD). Стандарт modules один и тот же в C++20 и C++23.

> **Примечание:** задачи по разделению D3D и разбиению `uc_common.h` перенесены в пре-стадию [этапа 7](stage7.md) — они необходимы до замены рендерера.
