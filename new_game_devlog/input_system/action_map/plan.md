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

- [x] В `input_actions.cpp` найти все ~20 чтений через `keybrd_button_use[ID]`
      и `joypad_button_use[ID]`, заменить на прямые scancode/btn-индексы по
      текущим env-дефолтам (left=203, right=205, forward=200, back=208,
      punch=44, kick=45, action=46, run=47, jump=57, start=15, select=28,
      camera=207, cam_left=211, cam_right=209, 1stperson=30; gamepad indices
      из строк 430-440)
- [x] Снести функцию init массивов (`input_actions.cpp:402+`) — блок с
      `joypad_button_use[X] = Y` и `keybrd_button_use[X] = ENV_get_value_number(...)`
- [x] Снести вызов этой функции там где она вызывалась при старте (game.cpp, attract.cpp)
- [x] Снести сами массивы из `input_actions_globals.{h,cpp}`
- [x] Снести enum'ы `JOYPAD_BUTTON_*` и `KEYBRD_BUTTON_*` (`input_actions.h:151+`) + extern init_joypad_config
- [x] В `game_tick.cpp:1679` (использование `keybrd_button_use[JOYPAD_BUTTON_SELECT]`) заменить на `KB_ENTER`
- [x] Сборка прошла. Ручная проверка геймплея — за пользователем

### 1b — снос UI экрана ремапа клавиш

- [x] В `frontend.cpp` найти весь блок экрана keyboard rebind (init из env + сохранение)
- [x] Удалить блок целиком (заполнение + сохранение)
- [x] Удалить пункт «Keyboard» из меню Options + items экрана из `raw_menu_data[]`, оставить коммент-маячок
- [x] Удалить функцию `FRONTEND_DrawKey` (рендер биндинга)
- [x] Удалить screen ID `FE_CONFIG_INPUT_KB` (стал ненужным)
- [x] Доп. снесены ставшие мёртвыми `OT_KEYPRESS`, `OT_RESET` define'ы + их switch-cases в `FRONTEND_input` / `FRONTEND_display_overlay`
- [x] Снесён глобал `grabbing_key` + его обработчик в `FRONTEND_input`
- [x] Сборка прошла. Ручная проверка: Options→Keyboard пункт исчез — за пользователем
- [x] Добавлена запись в GAMEPLAY_CHANGES.md

### 1c — мёртвые остатки

- [x] Снесён `engine/input/joystick_globals.h` (alias `#define the_state gamepad_state`) — никто его не include'ил
- [x] В `raw_menu_data[]` пункта Options→Joystick не было (уже снесён ранее) — коммент-маячок не нужен
- [x] Снесены мёртвые компоненты joystick rebind UI: функция `FRONTEND_DrawPad`, define'ы `FE_CONFIG_INPUT_JP`/`OT_PADPRESS`/`OT_PADMOVE`, глобалы `grabbing_pad`/`m_bMovingPanel`/`bCanChangeJoypadButtons`, switch-case'ы OT_PADPRESS/OT_PADMOVE в FRONTEND_input, case OT_PADPRESS в FRONTEND_display_overlay, блок `if (m_bMovingPanel) { ... }` (рендер перемещаемой HUD-панели), блок `if (grabbing_pad && !last_input) { ... }` и связанная переменная `static last_input`
- [x] Доготка из 1b: снесены ставшие мёртвыми `X_KEYBOARD`/`X_JOYSTICK`/`X_JOYPAD` в `xlat_str.h`
- [x] Сборка прошла

---

## Шаг 3 — Action-map

### 3a — Константы устройств (`input_codes.h`) + переименование `KB_*` → `KKEY_*`

- [x] Создан `new_game/src/game/action_map/input_codes.h` с константами KKEY_*, MBTN_*, MAXIS_*, GBTN_*, DBTN_*, GTRIG_*. Шапка со ссылкой на `rules.md`. GDIR_*/GAXIS_* отложены до 3c (нужны pair (stick, dir) semantics, решится при первом потребителе)
- [x] Глобальный sed-replace `\bKB_<X>\b` → `KKEY_<X>` по всем .cpp и .h в `new_game/src/` (~32 файла). Word boundary защитил `INPUT_CAR_KB_*` (часть имени input mask, не клавиша) — они остались нетронутыми. В string literals KB_* не было — ничего не сломалось
- [x] Старые `KB_*` defines удалены из `engine/input/keyboard.h`. Файл теперь подтягивает KKEY_* через `#include "game/action_map/input_codes.h"` — потребители keyboard.h продолжают компилироваться без правок include
- [x] Сборка прошла — линкер OK, никаких правок логики

### 3b — Пустые заголовки `act_*.h`

- [x] Созданы 6 файлов в `new_game/src/game/action_map/`:
      `act_menu.h`, `act_foot.h`, `act_car.h`, `act_cinematic.h`,
      `act_bangunsnotgames.h`, `act_dev_console.h`
- [x] В каждом шапка со ссылкой на `rules.md` и конкретный подшаг 3c.* в `plan.md`
- [x] Include guards (`GAME_ACTION_MAP_ACT_<NAME>_H`)
- [x] Каждый файл уже `#include` подтягивает `input_codes.h` — потребителям достаточно include только `act_<context>.h`
- [x] Пока без констант — заполняются в 3c

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

- [x] **3c.1 — cinematic** (outro, playcuts, video skip). Пилотный контекст. Заполнен `act_cinematic.h`: VIDEO_SKIP (3 KKEY + wildcard GBTN-loop), OUTRO_SKIP (3 KKEY + 2 GBTN), OUTRO_QUIT (1 KKEY + 1 GBTN Start, добавленный взамен будущего сноса bridge), PLAYCUT_SKIP (1 KKEY), GENERIC_SKIP (6 KKEY + 2 GBTN Start/Triangle для hardware_input_continue, тоже взамен bridge). Заменены чтения в `video_player.cpp`, `outro_main.cpp`, `outro/core/outro_os.cpp`, `missions/playcuts.cpp`, `game.cpp::hardware_input_continue`. Сборка прошла. Cycle skip-on-any-gamepad-button в video_player оставлен как wildcard с комментом — не выносится в одну константу.
- [x] **3c.2 — dev console** (ввод текста внутри открытой консоли). Заполнен `act_dev_console.h`: SUBMIT_KKEY (ENTER), CANCEL_KKEY (ESC). Letter keys / Backspace остаются через wildcard `input_last_key()` + `InkeyToAscii[]` — это API "любая клавиша → ASCII", не отдельные биндинги; задокументировано в шапке файла. Заменены чтения в `game_tick.cpp::process_controls` (is_inputing блок). F9 (открытие консоли из gameplay) НЕ тронут — пойдёт в 3c.5/3c.6.
- [x] **3c.3 — bangunsnotgames** (отладочные клавиши после ввода кода). Заполнен `act_bangunsnotgames.h`: 21 константа (F1 legend, F2 CRT, F8 single-step, F10 farfacet, F11 input-debug-panel, Shift+F12 cheat overlay, INS step-once, LCtrl overlay-lock, Ctrl+Q quit, Shift+Q speed-boost, Ctrl+L outside-cam, S screenshot, N cycle-model, B skeleton-overlay, `]` cycle-camera-person, `;` slow-mo, V version-overlay, 1/2/3/4 menu-theme). Заменены чтения в 9 файлах: `debug_help.cpp`, `aeng.cpp`, `game.cpp::special_keys/game_loop`, `game_tick.cpp::process_controls`, `person.cpp`, `thing.cpp`, `frontend.cpp::FRONTEND_input`, `panel.cpp`. ControlFlag/ShiftFlag-модификаторы оставлены как side-channel при call site (не часть action-константы). **НЕ покрыто** в этом подшаге (другие gate, не allow_debug_keys): `playback_game_keys` (GS_PLAYBACK) и `check_debug_timing_keys` (`#if OC_DEBUG_PHYSICS_TIMING`) — пойдут в другие подшаги или останутся как есть.
- [x] **3c.4 — menu** (frontend, pause, won/lost, attract, input debug panel). Заполнен `act_menu.h`: nav UP/DOWN/LEFT/RIGHT (KKEY + GBTN_DPAD_*), confirm (3 KKEY ENTER/SPACE/PENTER + GBTN_SOUTH), cancel (KKEY ESC + GBTN_NORTH), toggle-pause (KKEY ESC + GBTN_START), page-jump HOME/END, form-cycle TAB, attract-quit Q, frontend-cheats Numpad+/* . В `act_bangunsnotgames.h` дополнено: panel close (ESC), page switch (1/2/3), cycle subview (TAB). **Расширен `FORM_KeyProc(SLONG kb_code, SLONG gp_btn_idx = -1)`** в widget.cpp — принимает дополнительный GBTN. **Снесены bridge-блоки** в `widget.cpp::FORM_Process` (6 синтезов JUMP/CANCEL/FORWARDS/BACKWARDS/LEFT/RIGHT — `key = N` оставлен для path methods->Char) и `gamemenu.cpp:170-200` (3 синтеза Start/Triangle/Cross). Pause-toggle handler читает теперь три источника: KKEY_ESC + GBTN_START всегда + GBTN_NORTH когда меню открыто. Confirm handler читает 3 KKEY + GBTN_SOUTH. Заменены чтения в widget.cpp, gamemenu.cpp, frontend.cpp, attract.cpp, input_debug.cpp. **После 3c.4** `input_frame_inject_key_press` нигде не вызывается, только декларация (input_frame.h:95) и definition (input_frame.cpp:241) — финальный снос в шаге 2.
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
