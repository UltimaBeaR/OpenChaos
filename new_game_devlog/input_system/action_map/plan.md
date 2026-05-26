# План работы: action_map

Порядок шагов: сначала уборка параллельных систем (которые ломают семантику
compile-time констант), потом ввод констант. Каждый шаг — отдельный коммит
(коммитит пользователь вручную, агент только готовит изменения).

Прогресс отмечается прямо в этом файле: `[ ]` → `[~]` (в работе) → `[x]` (готово).

---

## Шаг 0 — Документация

- [x] Подпапка `new_game_devlog/input_system/action_map/`
- [x] `README.md`
- [x] `rules.md`
- [x] `plan.md` (этот файл)
- [x] Обновить `new_game_devlog/input_system/README.md` — добавить отметку про
      упрощённую модель `action_map/`

---

## Шаг 2 — Снос gamepad→keyboard bridge

Bridge сейчас синтезирует виртуальные нажатия клавиатуры из gamepad-кнопок в
двух местах. С появлением `ACT_*` это ломает семантику (одна и та же кнопка
читается через KKEY-канал через синтез). После сноса меню читает геймпад
напрямую через `input_btn_just_pressed`.

- [ ] `widget.cpp::FORM_KeyProc` — заменить 8 синтезов (`input_frame_inject_key_press(KB_ENTER/ESC/UP/DOWN/LEFT/RIGHT/...)`)
      на прямое чтение `input_btn_just_pressed(GBTN_*)` и установку соответствующего `key` rezultat'a из FORM_KEY_*
- [ ] `gamemenu.cpp:173` — Start→ESC заменить на прямое чтение Start с тем же эффектом
- [ ] Снести `input_frame_inject_key_press` из `input_frame.h/cpp` если не осталось вызовов

**Готовность:** запуск игры, проверка меню — все формы реагируют на Cross/Circle/Triangle/DPad как раньше.

---

## Шаг 1 — Снос старой rebind-системы

Параллельная рантайм-таблица переназначения клавиш делает невозможным считать
константу источником истины. Сносим её целиком (env-load, UI экран, массивы,
enum'ы). UI восстановим позже когда придёт runtime-ремап (вне 1.0).

### 1a — снос геймплейных массивов и init из env

- [ ] В `input_actions.cpp` найти все ~20 чтений через `keybrd_button_use[ID]`
      и `joypad_button_use[ID]`, заменить на прямые scancode/btn-индексы по
      текущим env-дефолтам (left=203, right=205, forward=200, back=208,
      punch=44, kick=45, action=46, run=47, jump=57, start=15, select=28,
      camera=207, cam_left=211, cam_right=209, 1stperson=30; gamepad indices
      из строк 430-440)
- [ ] Снести функцию init массивов (`input_actions.cpp:402+`) — блок с
      `joypad_button_use[X] = Y` и `keybrd_button_use[X] = ENV_get_value_number(...)`
- [ ] Снести вызов этой функции там где она вызывалась при старте
- [ ] Снести сами массивы из `input_actions_globals.{h,cpp}`
- [ ] Снести enum'ы `JOYPAD_BUTTON_*` и `KEYBRD_BUTTON_*` (`input_actions.h:151+`)
- [ ] В `game_tick.cpp:1675-1679` (использование `keybrd_button_use[JOYPAD_BUTTON_SELECT]`) заменить на прямой scancode
- [ ] Сборка + ручная проверка на ногах: движение / атака / прыжок / экшен / first-person view

### 1b — снос UI экрана ремапа клавиш

- [ ] В `frontend.cpp` найти весь блок экрана keyboard rebind:
      `menu_data[0..14].Data = ENV_get_value_number(...)` (строки ~1936-1949)
      и parallel `ENV_set_value_number(...)` при выходе (~2307-2331)
- [ ] Удалить блок целиком (заполнение + сохранение)
- [ ] Удалить пункт «Keyboard» из меню Options — оставить **один** коммент-маячок:
      `// Здесь раньше был пункт ремапа клавиш + одноимённый sub-screen; снесено в действиях action_map. См. new_game_devlog/input_system/action_map/plan.md шаг 1b. История — git log.`
- [ ] Удалить функцию рендера биндинга на экране (`frontend.cpp:653` — «Draws the name of the keyboard key assigned to a menu action»)
- [ ] Удалить screen ID / enum value которым этот экран адресовался
- [ ] Сборка + проверка: меню Options открывается, пункта keyboard нет, остальные пункты работают

### 1c — мёртвые остатки

- [ ] Снести `engine/input/joystick_globals.h` (alias `#define the_state gamepad_state`)
      и любые includes этого файла
- [ ] Поискать joystick UI экран остатки в `frontend.cpp` — если есть пункт Options→Joystick или подобное, снести так же как keyboard (с коммент-маячком на одну строку)
- [ ] Сборка + общая проверка что ничего не отвалилось

---

## Шаг 3 — Action-map

### 3a — Константы устройств (`input_codes.h`) + переименование `KB_*` → `KKEY_*`

- [ ] Создать `new_game/src/game/action_map/input_codes.h` со всеми типами
      констант (KKEY_*, MBTN_*, MAXIS_*, GBTN_*, DBTN_*, GDIR_*, GAXIS_*, GTRIG_*).
      Значения KKEY_* взять из текущего `engine/input/keyboard.h`.
- [ ] Заменить во всём коде `KB_<X>` на `KKEY_<X>` (грубо: глобальный
      механический replace).
- [ ] Удалить старые `KB_*` определения из `engine/input/keyboard.h`
- [ ] Сборка — должна пройти без правок логики
- [ ] Шапка `input_codes.h` со ссылкой на `rules.md`

### 3b — Пустые заголовки `act_*.h`

- [ ] Создать 6 файлов в `new_game/src/game/action_map/`:
      `act_menu.h`, `act_foot.h`, `act_car.h`, `act_cinematic.h`,
      `act_bangunsnotgames.h`, `act_dev_console.h`
- [ ] В каждом шапка со ссылкой на `rules.md` и `plan.md`
- [ ] Include guards
- [ ] Пока без констант — заполняются в 3c

### 3c — Вынос всех мест чтения ввода в `ACT_*` константы

Идём по контекстам, каждый — отдельный мини-коммит. После каждого — проверка
что соответствующий режим игры работает.

- [ ] **3c.1 — cinematic** (outro, playcuts, video skip). Малый объём, простой контекст — пилот.
- [ ] **3c.2 — dev console** (ввод текста внутри открытой консоли — Enter/Backspace/ESC/буквы)
- [ ] **3c.3 — bangunsnotgames** (F1/F2/F8/F10/F11/Ctrl+* отладочные клавиши)
- [ ] **3c.4 — menu** (frontend, pause, won/lost — навигация, confirm/cancel/start)
- [ ] **3c.5 — foot** (геймплей на ногах: движение, атака, прыжок, камера, инвентарь, ESC-открыть-пауза, F9-открыть-консоль)
- [ ] **3c.6 — car** (в машине: акселерация, поворот, сирена, выход, ESC, F9)
- [ ] **Финальный аудит** — `grep` по `input_key_*\|input_btn_*\|input_stick_*\|input_trigger\|input_mouse_consume_rel` по всему `new_game/src/`. Каждое чтение должно идти через `ACT_*` константу. Исключения (если останутся) — задокументировать в `rules.md`.

---

## После 3 — что НЕ делается в этой работе

- Runtime-resolver / ремап-UI / кросс-контекстный стек (это `full_plan_deferred.md`)
- Reverse-lookup glyph'ов для подсказок UI
- Расширение mouse API (drag, double-click, scroll)
- DualSense touchpad finger tracking
- Гироскоп / motion sensor

Эти расширения становятся возможными благодаря шагу 3 — каждое чтение в коде
будет привязано к именованной константе, которую можно подменить runtime-слоем
без правки точек вызова.
