# План работы: action_map

Порядок шагов: сначала снос параллельной rebind-системы (она ломает семантику
compile-time констант), затем основная action-map работа по контекстам. Bridge
gamepad→keyboard удаляется **в конце** как финальная чистка после того как все
контексты получили прямые параллельные чтения gamepad-кнопок.

Каждый шаг — отдельный коммит (коммитит пользователь вручную, агент только
готовит изменения).

Прогресс отмечается прямо в этом файле: `[ ]` → `[~]` (в работе) → `[x]` (готово).

## История пересмотров плана

- **2026-05-26 v1:** изначально шли в порядке Шаг 2 → 1 → 3. Bridge оценивался
  как «2 места» (widget.cpp + gamemenu.cpp:173). При вдумчивом чтении кода
  обнаружено что bridge через `input_frame_inject_key_press(KB_ESC/ENTER/UP/DOWN/...)`
  невидимо обслуживает **всех потребителей** KB_ESC/ENTER/UP/DOWN/LEFT/RIGHT по
  всей игре — video skip, outro/playcuts skip, dev console, debug panel, frontend
  ESC handlers. Снести bridge без замены = сломать gamepad во всех этих местах.
  План пересмотрен: bridge удаляется в самом конце, после того как шаг 3 для
  каждого контекста добавил параллельное чтение GBTN рядом с KKEY.

---

## Шаг 0 — Документация

- [x] Подпапка `new_game_devlog/input_system/action_map/`
- [x] `README.md`
- [x] `rules.md`
- [x] `plan.md` (этот файл)
- [x] Обновить `new_game_devlog/input_system/README.md` — добавить отметку про
      упрощённую модель `action_map/`

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
- [ ] Заменить во всём коде `KB_<X>` на `KKEY_<X>` (глобальный механический replace).
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

**Правило для контекстов где раньше работал bridge** (menu, cinematic, dev console,
frontend ESC и т.п.): рядом с `input_key_*(ACT_*_KKEY)` добавляем
`input_btn_*(ACT_*_GBTN)` (или соответствующий GDIR/GAXIS). Bridge продолжает
работать пока шаг 3 не закончен — это не двойное срабатывание, потому что:
- Bridge инжектит KB-пресс при gp нажатии — реальный KB остаётся не нажат.
- Наше новое чтение GBTN видит то же gp-нажатие.
- KKEY-чтение видит и реальный KB и инжектированный — но gp-нажатие не вызывает
  реального KB. Так что KKEY-чтение по gp срабатывает только через инжект, а
  GBTN-чтение — напрямую. Это один и тот же сигнал, просто два пути в input_frame.

**3c.4 (menu)** — пограничный: bridge удаляется именно в этом подшаге, потому
что вся логика синтеза сосредоточена в `widget.cpp::FORM_Process` и
`gamemenu.cpp:170-200`. После того как меню начнёт читать gamepad напрямую
(через расширение `FORM_KeyProc` чтобы принимать `(kb_code, gp_btn)`) — bridge
не нужен **для меню**, но всё ещё работает для остальных контекстов (через те
же синтезы, пока их не уберём в шаге 2).

Подшаги:

- [ ] **3c.1 — cinematic** (outro, playcuts, video skip). Малый объём, простой контекст — пилот. Добавить параллельные GBTN-чтения рядом с KKEY (Cross/A для confirm-skip, любая кнопка для general skip).
- [ ] **3c.2 — dev console** (ввод текста внутри открытой консоли — Enter/Backspace/ESC/буквы). Геймпадная навигация не нужна (это text-input), но KKEY-константы заводим.
- [ ] **3c.3 — bangunsnotgames** (F1/F2/F8/F10/F11/Ctrl+* отладочные клавиши). Геймпада нет — только KKEY.
- [ ] **3c.4 — menu** (frontend, pause, won/lost — навигация, confirm/cancel/start). **Здесь же**: расширить `FORM_KeyProc(SLONG kb_code, SLONG gp_btn_idx = -1)`, заменить bridge-блок в `widget.cpp` на сохранение только `key = N` (для path methods->Char), снести bridge-блок в `gamemenu.cpp:170-200`, добавить прямые `input_btn_just_pressed` в pause-toggle handler. Bridge в widget.cpp/gamemenu.cpp удаляется. Остальные потребители (video, outro, dev console) пока продолжают видеть синтез — функция `input_frame_inject_key_press` ещё используется их инжектами? Нет, инжекты были только в widget.cpp и gamemenu.cpp. После 3c.4 в коде остаётся **только определение** функции в `input_frame.cpp` без вызовов.
- [ ] **3c.5 — foot** (геймплей на ногах: движение, атака, прыжок, камера, инвентарь, ESC-открыть-пауза, F9-открыть-консоль). Параллельные GBTN-чтения добавляются где раньше bridge помогал (ESC handler).
- [ ] **3c.6 — car** (в машине: акселерация, поворот, сирена, выход, ESC, F9). То же.
- [ ] **Финальный аудит** — `grep` по `input_key_*\|input_btn_*\|input_stick_*\|input_trigger\|input_mouse_consume_rel` по всему `new_game/src/`. Каждое чтение должно идти через `ACT_*` константу. Исключения (если останутся) — задокументировать в `rules.md`.

---

## Шаг 2 — Финальная чистка bridge

После 3c.4 функция `input_frame_inject_key_press` уже без вызовов (синтезы были
только в widget.cpp и gamemenu.cpp, оба места переписаны на прямые чтения в
3c.4). Этот шаг — мелкая зачистка.

- [ ] `grep` по `input_frame_inject_key_press` — убедиться что 0 совпадений в коде
- [ ] Удалить декларацию `void input_frame_inject_key_press(SLONG kb_code)` из `input_frame.h` (~строка 95)
- [ ] Удалить реализацию из `input_frame.cpp` (~строка 241)
- [ ] Привести в порядок комментарии в `input_frame.h/cpp` — убрать упоминания «synthetic press» / «bridge» / «gamepad→keyboard» там где они описывали этот канал. Оставить упоминания только в архивных файлах devlog.
- [ ] Сборка + общая проверка

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
