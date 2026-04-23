# Отладочные клавиши (debug hotkeys)

Все dev-only клавиши в игре спрятаны под режим **`bangunsnotgames`**
(флаг `allow_debug_keys` в `game_tick_globals.cpp`). Это сделано
специально: финальная сборка 1.0 не должна показывать игроку
отладочный функционал, даже случайным нажатием.

## Как включить режим

1. В игре нажать **F9** — откроется консольный ввод.
2. Набрать `bangunsnotgames` и Enter.
3. Автоматически на 5 секунд всплывёт легенда всех клавиш.
4. **F1** потом — показать легенду ещё раз (5 сек).
5. Повторный ввод `bangunsnotgames` — выключит режим.

F9 — единственная клавиша которая работает **без** `allow_debug_keys`
(иначе включить режим было бы нечем).

## Текущий список клавиш

Полный список — в коде легенды:
[`new_game/src/engine/debug/debug_help/debug_help.cpp`](../new_game/src/engine/debug/debug_help/debug_help.cpp)
(массив `s_rows`). Он же отображается на F1 в игре — **это источник
истины**. Если добавляешь новую debug-клавишу — обязательно добавь её
и в легенду, иначе пользователь о ней не узнает.

Краткий срез по состоянию на 2026-04-17:

| Клавиша      | Действие                                 |
|--------------|------------------------------------------|
| F1           | показать эту легенду (5 сек)             |
| F3           | load game (Shift+F3 = save, `poo.sav`)   |
| F4           | toggle clouds                            |
| F8           | toggle single-step mode                  |
| F9           | консоль (не гейтится)                    |
| F10          | farfacet debug split view                |
| F11          | input debug panel                        |
| F12          | spawn weapons around player + heal 9999  |
| Shift+F12    | cheat toggle (FPS печатается вверху)     |
| Insert       | step once (в single-step)                |
| Ctrl         | lock debug overlay (force-visible)       |
| Ctrl+L       | toggle outside / ambient lighting        |
| Ctrl+Q       | quit                                     |
| Shift+M      | spawn mine at mouse cursor               |
| /            | toggle stealth debug                     |
| L            | toggle dynamic light at player           |
| T            | warehouse debug (двери, MAV arrows)      |
| V            | show version string (fade-out)           |

## История рефакторинга (2026-04-17)

Все клавиши выше раньше были разбросаны по коду с разным статусом
"гейта". Причёсано в одну итерацию:
- **Удалено:** F2 gamepad debug overlay (функционал переехал в input
  debug panel на F11).
- **Перебиндено:** F11 cloud toggle → F4 (раньше F11 делал две вещи
  сразу — панель + облака — из-за чего облака невозможно было проверить);
  `'` (quote) → F8 (toggle step mode); `,` (comma) → Insert (step once).
  Причина перебинда step-mode клавиш — пунктуация в легенде опакна
  ("что вообще значит `'`?").
- **Загейчено:** F11 panel, Ctrl+Q, Ctrl+L, Shift+F12, V, step-mode
  клавиши — раньше работали без `bangunsnotgames`.
- **Новое:** F1 help legend + 5-сек авто-показ при активации режима.
  Модуль: [`new_game/src/engine/debug/debug_help/`](../new_game/src/engine/debug/debug_help/).

## Незавершённое к 1.0

См. [`known_issues_and_bugs.md`](../new_game_planning/known_issues_and_bugs.md)
раздел "Настройки и конфигурация" — задача "пройтись по всему коду и
убедиться что ничего отладочного не торчит в релизе". Первая итерация
сделана (клавиши выше), но остаётся аудит консольных команд (`win`,
`lose`, `cam`, `tels`, `crinkles` etc.), `ControlFlag && ShiftFlag`
debug-веток внутри process_controls, а также проверка что отладочные
оверлеи (`stealth_debug`, `debug_overlay_locked_on` и т.п.) гасятся
при выходе из debug-режима, а не остаются активными.

## Идеи на будущее (post-1.0)

### Debug-хоткеи для быстрого перебора визуальных параметров

При охоте на баги связанные с разрешением / аспектом / FOV / AA нужно
каждый раз править `config.h` → пересобирать → запускать → смотреть.
Это медленно и ломает "живое" сравнение "до/после".

Хотелось бы в debug-режиме (`bangunsnotgames`) иметь хоткеи для
runtime-переключения таких параметров, чтобы можно было быстро
щёлкать и сравнивать отображение не перезапуская игру. Кандидаты:

- **Разрешение / аспект окна** — циклить по набору пресетов
  (640×480, 1280×720, 1920×1080, 480×1920 portrait, 1920×480 ultra-wide).
  Триггерит полный `SetDisplay` + пересоздание scene FBO + `recompute`
  ui_coords. Упирается в реализацию `SetDisplay` в OpenGL-бэкенде
  (сейчас TODO, см. [`core.cpp:2598`](../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L2598)).
- **`OC_FOV_MULTIPLIER`** — инкремент ±0.1, диапазон 0.5..2.0.
  Требует снять `const` с константы (сделать глобалкой с default из config).
- **`OC_AA_ENABLE` / FXAA** — on/off, чтобы оценить насколько шейдерный
  AA реально помогает на данной сцене / разрешении.
- **`OC_RENDER_SCALE`** — 0.5 / 0.75 / 1.0, влияет на scene FBO size.
- Возможно **`OC_FULLSCREEN`** (windowed / borderless toggle) —
  пригодится для тех же сравнений, но тяжелее в реализации.

**Что это даёт:** регрессия типа "на 1920×480 луна пропадает" сейчас
требует править config.h + build. С этими хоткеями это был бы один
клавишу — мгновенное переключение, визуальное сравнение рядом.
Особенно полезно для багов которые проявляются только на узком наборе
параметров (FOV × аспект × глубина scene).

**Не для 1.0** — это dev-ergo improvement, не blocker релиза. После
релиза, когда фаза активного баг-хантинга продолжится (патчи,
community issues), инвестиция окупится.

## Если добавляешь новую debug-клавишу

1. Проверь что она под `if (allow_debug_keys)` или после
   `if (!allow_debug_keys) return;` в process_controls.
2. Выбери клавишу не пересекающуюся с геймплейной раскладкой
   (особенно F5/F6/F7 = CAM1/2/3 в weapon mode).
3. Добавь строку в `s_rows[]` в
   [`debug_help.cpp`](../new_game/src/engine/debug/debug_help/debug_help.cpp)
   с человекочитаемым описанием.
4. Если клавиша делает что-то неочевидное — поставь короткий
   комментарий `// name of what it does` рядом с handler'ом.
