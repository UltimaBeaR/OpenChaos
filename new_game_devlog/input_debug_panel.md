# Панель отладки ввода (Input Debug Panel)

Модальная полноэкранная панель в игре для проверки и диагностики устройств
ввода — клавиатуры, Xbox/generic геймпада, DualSense. Чисто dev-инструмент,
не игровая фича. Лежит в [`new_game/src/engine/debug/input_debug/`](../new_game/src/engine/debug/input_debug/).

## Для чего

- Проверить что клава / геймпад / DualSense подключены и распознаются
- Посмотреть живые значения стиков / триггеров / кнопок — отдельно от игры,
  без помех от геймплея
- Протестировать вибрацию (и Xbox и DualSense, через общий `gamepad_rumble`)
- Протестировать DualSense-специфику: lightbar RGB, player LED полоску
  (5 светодиодов над touchpad), тачпад X/Y с двумя пальцами
- В будущем — тест adaptive trigger effects (отдельный sub-page, сейчас заглушка)

## Как открывается

**F11** (в игре) → полноэкранная панель. Активна до F11 или ESC.

На время активности панели игра полностью изолирована:
- Весь игровой ввод заблокирован (`process_controls`, `get_hardware_input`,
  `process_hardware_level_input_for_player`, `GAMEMENU_process`, камера —
  все гейтятся)
- Gamepad-state скрабится в `gamepad_poll` (стики центрируются, кнопки/триггеры
  в 0) чтобы прямые читатели (pause menu, `fc.cpp` камера) не видели ввод
- Игровые выходы на контроллер (`gamepad_rumble_tick` / `gamepad_led_update` /
  `gamepad_triggers_update`) заблокированы через `!input_debug_is_active()` —
  не смешиваются с тестами панели
- **D-pad → stick эмуляция** в `gamepad.cpp` (обычно переопределяет `lX/lY`
  значениями 0/65535 когда нажата стрелка d-pad) тоже **пропускается пока
  панель открыта** — иначе жмёшь стрелку и точка стика прыгает в угол,
  хотя стик не трогаешь
- При открытии: rumble_stop, led_reset, triggers_off — стартуем с чистого листа
- При закрытии: игра следующим кадром восстанавливает свой LED/триггеры

## Структура экрана

**Сверху 3 таба:** Keyboard / Xbox controller / DualSense controller.
Переключение — `1` / `2` / `3`. Подсветка двухуровневая:
- **Открытый таб** — обведён белой рамкой
- **Активное устройство** (откуда последним пришёл ввод) — имя зелёным
  (`active_input_device` глобал из gamepad.cpp — та же глобалка что игра
  использует)

Страница рендерится всегда, но её данные показываются только когда
устройство этой вкладки = активное. На чужих вкладках виджеты показывают
idle-состояние (стики в центре, кнопки тусклые, списки пустые). Реализовано
через прокси-функции в `input_debug.cpp`:
- `input_debug_read_gamepad_for(device)` → `gamepad_state_raw()` если
  active совпадает, иначе `s_idle_state`
- `input_debug_key_held(sc)` → `Keys[sc]` если active==KB
- `input_debug_read_ds_feedback/_effect_active/_touchpad()` — аналогично

`s_gamepad_state_raw` захватывается в `gamepad_poll` **до** скраба (но
**после** d-pad→stick override в геймплейном режиме — в режиме панели
override пропущен, так что snapshot содержит чистые значения стика).

**Снизу футер:**
`arrows navigate | left/right adjust | Enter activate | 1/2/3 page | TAB DS sub-page | F11 / ESC exit`

## Навигация

Все взаимодействия внутри страницы — стрелками, Enter, TAB (TV-remote style):
- **↑/↓** — курсор между строками меню
- **←/→** — изменить числовое значение выбранной строки
- **Enter** — активировать action-строку
- **TAB** — переключить sub-page текущей страницы (DS/Xbox только)

Edge-detect единый на уровне панели в `input_debug_tick` — пишет в
`s_nav` struct, виджеты читают через `input_debug_nav()`. Один цикл =
одно нажатие = один edge. Никакого double-consume.

Sub-page state (DS, Xbox) сбрасывается на VIEW при каждом открытии
панели — `input_debug_dualsense_reset_state()` и
`input_debug_gamepad_reset_sub()` вызываются из
`isolate_controller_from_game()`.

## Страницы

### Keyboard

Просто список удерживаемых скан-кодов (hex + decimal). Читается через
`input_debug_key_held`. Заглушка — в будущем сделаем клавиатурную картинку
с подсветкой реально нажимаемых клавиш.

### Xbox controller

Два sub-page, переключение TAB:

#### VIEW (по умолчанию)
Визуализация контроллера с раскладкой кнопок приблизительно под
физический Xbox-совместимый геймпад:
- **LB / RB** — лейблы над LT / RT барами (`y=55`)
- **LT / RT** — вертикальные прогресс-бары с жёлтой заливкой снизу
  (`x=200 / 418`, `y=[70..166]`, `22×96`)
- **Xbox (Guide)** — кнопка в верхнем центре `(cx=320, y=100)`,
  где на DS расположен touchpad
- **Back / Start** — в центре экрана под Xbox кнопкой `(y=240)`,
  симметрично относительно центра `(cx=320)`
- **D-pad ромб** (U/L/R/D) по центру колонки левого стика `(cx=160, cy=235)`
- **Face buttons ромб** (Y/X/B/A; Y top, X left, B right, A bottom)
  по центру колонки правого стика `(cx=480, cy=235)`
- **Left / Right stick** — боксы 96×96 внизу `(cy=330)`, заголовки
  центрированы над боксом, LS/RS индикаторы справа от заголовка
- **X=XXXXX Y=XXXXX** — numeric readout прямо под каждым стиком

Физическая раскладка Xbox (где d-pad и левый стик поменяны местами по
сравнению с DS) сознательно **не** отражается — у Xbox-совместимых
контроллеров раскладки разные (оригинальный Xbox, 360, One, 3rd-party),
рендерим единообразно с DS.

#### TESTS (TAB)
Полноэкранный rumble-виджет (3 строки: low motor / high motor /
fire 200ms pulse). У Xbox нет lightbar и player LEDs, поэтому только
rumble.

### DualSense controller

Три sub-page, переключение TAB (цикл VIEW → TESTS → TRIGGERS → VIEW):

#### VIEW (по умолчанию)
Визуализация под физический DualSense:
- **L1 / R1** — лейблы над L2 / R2 барами `(y=55)`
- **L2 / R2** — вертикальные progress-бары `(x=200 / 418, y=[70..166])`
- **Touchpad** — большой прямоугольник в центре сверху
  `(x=[232..410], y=[80..150])`, между L2 и R2 барами. Зелёная точка
  finger1, пурпурная finger2 (raw hardware 0..1919 × 0..1079).
  Label "Touchpad click" рядом с заголовком — индикатор физического
  клика по тачпаду.
- **Share / Options** — по центру колонки левого / правого стика `(y=100)`,
  на уровне середины touchpad'а (как на реальном контроллере где
  они flanking tачпад сверху)
- **D-pad ромб** (U/L/R/D) по центру левого стика `(cx=160, cy=235)`
- **Face buttons ромб** (Triangle/Square/Circle/Cross) по центру
  правого стика `(cx=480, cy=235)`. Triangle сдвинут +3 px вправо,
  Cross -3 px влево — чтобы визуально выровнять диагональные буквы
  в битмап-шрифте (Triangle шире, без сдвига выглядит левее Cross).
- **PS / Mute** стэкнутые в центре экрана между стиками
  `(cx=320, y=~322/338)` — Mute под PS как на реальном DS
- **Left / Right stick** — боксы внизу `(cy=330)`, L3 / R3 индикаторы
  справа от заголовков
- **X=XXXXX Y=XXXXX** — numeric readout под каждым стиком

#### TESTS (TAB)
Полноэкранное меню для тестирования выходных устройств DS:
- **Rumble test** — low motor / high motor / fire 200ms pulse
- **Lightbar (live)** — R / G / B ручки 0..255 шаг 16, live apply через
  `ds_set_lightbar`
- **Player LEDs (live)** — 3 пары-тогглера (Outer / Inner / Center),
  Enter на строке зажигает/гасит. У hardware DualSense 5 логических
  LED но биты маски (1,5) и (2,4) зеркальные на уровне firmware —
  поэтому 3 пары соответствуют `PlayerLed::{Outer,Inner,Center}`
  константам в libDualsense

#### TRIGGERS (TAB)
Полнофункциональный тестер adaptive trigger эффектов.

**Интерактивные строки (↑↓ курсор, ←→ значения, Enter toggle):**
- `[x] L2 trigger` — независимый тоглер включения левого триггера
- `[x] R2 trigger` — аналогично правого. Оба включённых = bridge hand=2
  (both). Выключенный триггер получает Off, чтобы не оставалось
  остаточного сопротивления
- `Effect: < Name >` — цикл по 12 эффектам (←/→): Off / Simple_Feedback /
  Simple_Weapon / Simple_Vibration / Feedback / Limited_Feedback /
  Weapon / Limited_Weapon / Vibration / Bow / Galloping / Machine
- Параметрические строки специфичные для текущего эффекта (1..6 строк),
  каждый эффект хранит свои значения независимо — переключился на
  другой эффект и обратно, параметры сохранились

**Read-only индикаторы (низ страницы, колонками для L2 и R2):**
- `position` — аналоговое значение триггера 0..255
- `fb state` — low nibble feedback байта (0..9)
- `act` — чекбокс effect-active бита (bit 4 feedback байта)
- `slot` — dump 10 байт текущего trigger effect slot'а (hex) в двух
  строках по 5 байт — видно что именно мы запаковали и послали

**Live apply** через bridge: при входе на sub-page пушится текущее
состояние, дальше только на frames с реальным изменением любого поля
(детект через dirty-флаг в param/toggle/effect row handlers). При
выходе из TRIGGERS (TAB) — `ds_trigger_off(2) + ds_update_output` чтобы
триггеры не гудели на VIEW / TESTS страницах.

**Бридж расширен** под тестер: добавлены `ds_trigger_galloping`,
`ds_trigger_simple_weapon`, `ds_trigger_simple_vibration`,
`ds_trigger_limited_feedback`, `ds_trigger_limited_weapon` +
`ds_trigger_bow_full` / `ds_trigger_machine_full` (существующие
упрощённые `ds_trigger_bow` / `ds_trigger_machine` оставлены для
игрового кода). libDualsense получил 5 новых factory функций, packing
портирован 1:1 из duaLib (MIT, `THIRD_PARTY_LICENSES.md` уже покрывает).

## Координатное пространство

Панель целиком в **логических 640×480** координатах:
- `AENG_draw_rect` нативно масштабируется пайплайном
- `FONT_buffer_add` использует **литеральные пиксели окна** — поэтому в
  панели все вызовы обёрнуты в `input_debug_text(logical_x, logical_y, ...)`
  которая скейлит `* RealDisplayWidth / 640` перед `FONT_buffer_add`

Подробности про этот mismatch — в скилле `.claude/skills/text-rendering/SKILL.md`
в секции "Mixing text and HUD rects". Универсальный фикс (скейлинг внутри
самой FONT_draw_coloured_char) записан в `known_issues_and_bugs.md` как
post-1.0 рефакторинг.

**Layer / z-order** — в `AENG_draw_rect` работает **инвертированно** от
того что можно подумать: **lower layer = closer to camera (in front)**.
Корень — цепочка преобразований `AENG_draw_rect → PolyPoint2D::SetSC →
tl_vert.glsl` даёт `ndc_z = layer * 0.0002 - 1`, GL_LESS depth test.
Константы в `input_debug.h`:
- `INPUT_DEBUG_LAYER_ACCENT = 10` — точки / заливки (поверх всего)
- `INPUT_DEBUG_LAYER_CONTENT = 11` — фоновые прямоугольники виджетов
- `INPUT_DEBUG_LAYER_BACKDROP = 12` — полноэкранный dimmer

Skill `hud-rendering` обновлён чтобы отражать эту инверсию.

## Рендер в HUD pipeline

Всё рисуется одним `PANEL_start()` / `PANEL_finish()` батчем в
`input_debug_render()` — это обязательная обёртка иначе геометрия не
флашится в кадре и попадает в shadow page через VB slot reuse (баг
описан в [shadow_corruption_investigation.md](shadow_corruption_investigation.md)).

Вызов `input_debug_render()` идёт сразу после `OVERLAY_handle()` в
game.cpp — то же место где рисуется health bar HUD.

## Архитектура файлов

```
new_game/src/engine/debug/input_debug/
  input_debug.h              — public API, layer / layout константы
  input_debug.cpp            — lifecycle, tick, render, page dispatch,
                               shared widgets (stick/trigger/button/text)
  input_debug_keyboard.cpp   — Keyboard page
  input_debug_gamepad.cpp    — Xbox page (VIEW + TESTS sub-pages)
  input_debug_dualsense.cpp  — DualSense page (VIEW + TESTS + TRIGGERS)
```

Ключевые public функции:
- `input_debug_toggle()` / `_open()` / `_close()` / `_is_active()` — lifecycle
- `input_debug_tick()` / `_render()` — вызываются из game loop
- `input_debug_dualsense_toggle_sub()` / `input_debug_gamepad_toggle_sub()` —
  TAB handler, вызывается из tick
- `input_debug_dualsense_reset_state()` / `input_debug_gamepad_reset_sub()` —
  сброс sub-page и состояния виджетов при open/close

## Известные открытые задачи и баги

### Баги

**Не все DualSense-кнопки переключают `active_input_device` на DualSense.**
Репро: активное устройство клавиатура (последним был ввод с клавы),
жму кнопки на DS — на вкладке DualSense должны "зажигаться" лейблы, но
некоторые кнопки не перещёлкивают active в DUALSENSE и потому не видны
как нажатые. Подтверждено как минимум:
- **Touchpad click** (физическое нажатие на сенсорную панель) — не переключает
- **Mute** — не переключает
- **Пальцы по тачпаду** (скольжение без клика) — не переключает
  (возможно и не должно — но стоит обсудить)
- Вероятно другие кнопки вне индексов 0..16 тоже

Корень: `gamepad_poll` в [gamepad.cpp](../new_game/src/engine/input/gamepad.cpp)
сканирует активность DS так: `for (int i = 0; i < 17; i++) if (gamepad_state.rgbButtons[i]) ...`.
Но `rgbButtons[17] = touchpad_click` и `rgbButtons[18] = mute` — оба вне
цикла. Плюс finger-on-touchpad вообще не в `rgbButtons`, он в
`s_input.touchpad_finger_1_down` (только в DS InputState).

Фикс: расширить цикл до 19 (`< 19`) для touchpad click + mute. Для
finger-on-touchpad отдельно: добавить проверку
`ds_get_input().touchpad_finger_1_down || ..._2_down` как триггер активности.

**DualSense не детектится в панели после hotplug.**
Сценарий: Xbox был активен → Xbox отключён / DualSense подключён по
Bluetooth → открываю F11 панель → переключаюсь на DS таб → жму кнопки
на DS → на странице ничего не светится. Закрываю F11 — в геймплее DS
работает (перс бегает). Открываю F11 снова → на DS странице по-прежнему
ничего не реагирует, пока не нажму стрелку на клавиатуре, потом DS
начинает отображаться нормально.

Гипотеза: `s_is_dualsense` флаг не успевает переключиться внутри
`gamepad_poll` пока панель активна. Возможно потому что hotplug
detection проходит только через `ds_poll_registry` раз в секунду, и
момент первой активации DS пропускается. Или `active_input_device`
остаётся = KEYBOARD после пресса `3` для смены таба, и визуальный
прокси `input_debug_read_gamepad_for(DS)` возвращает idle state.

**Асимметрия stick dot (не баг, а hw drift).**
Пользователь замечал что при максимальном отклонении стика вправо-вверх
и влево-вниз красная точка оказывается на разном расстоянии от угла
бокса. Это реальный stick drift контроллера (raw значения в покое ≠ 128
на 8-битном HID байте). Фиксится либо deadzone'ом (нельзя — панель
показывает RAW значения специально), либо добавлением отдельного
маркера "реальный нулевой центр" (точка другого цвета). Не приоритет.

### Дизайн на будущее

- **Keyboard страница — картинка клавиатуры** с подсветкой нажатых
  клавиш, вместо голого списка скан-кодов.
- **Убрать d-pad → stick эмуляцию вообще** (а не только скипать при
  активной панели) — поднимался вопрос, что d-pad и stick это разные
  схемы управления (tank controls vs аналог), смешивать их некорректно.
  Решение отложено — нужно проверить не сломает ли это управление
  в игре, где кто-то возможно играет на d-pad.

## История реализации

- **Iteration 1** — каркас панели, F11 вход/выход, полупрозрачный чёрный
  backdrop, блокировка ввода
- **Iteration 2** — 3 страницы, навигация 1/2/3, базовые виджеты
  (stick/trigger/button)
- **Iteration 3** — arrow-nav вместо хоткеев, rumble test, feedback/act
  индикаторы, табы с двойной подсветкой, фикс координатного mismatch'а
  для текста
- **Iteration 4** — touchpad XY парсинг, touchpad viz, lightbar test,
  player LED test. Попутно нашли и пофиксили что libDualsense не шлёт
  BT init packet → LED не применялся на Bluetooth (детали в
  [`dualsense_libs_reference/own_dualsense_lib_plan.md`](../dualsense_libs_reference/own_dualsense_lib_plan.md))
- **Iteration 5** (2026-04-17) — z-order фикс (точки/заливки теперь
  рисуются). Корень бага: конвенция `layer` в `AENG_draw_rect` инверсна
  тому, что документировал skill `hud-rendering`. Цепочка преобразований
  `AENG_draw_rect → PolyPoint2D::SetSC → tl_vert.glsl` даёт
  `ndc_z = layer * 0.0002 - 1` при GL_LESS, то есть **меньший layer =
  ближе к камере**. Подтверждение: `PANEL_draw_health_bar` рисует bg с
  layer=2 и красную заливку layer=1 — заливка поверх. В панели был
  `LAYER_ACCENT=12` (выше чем фоны) — точки отбрасывались depth-тестом.
  Фикс: `ACCENT=10, CONTENT=11, BACKDROP=12`, вынесено в константы в
  заголовке, skill `hud-rendering` переписан. Layout правки: touchpad
  переехал, feedback act/fb переехали под trigger bar. Backdrop 80% → 90%.
- **Iteration 6** (2026-04-17) — рефакторинг layout'а кнопок DS
  ромбами (face + d-pad) + L1/R1/L3/R3/PS/Mute. Per-stick numeric
  readout. Убраны дубликаты значений L2/R2. Общие layout-константы
  `INPUT_DEBUG_STICK_*` / `_TRIG_*` вынесены в заголовок.
- **Iteration 7** (2026-04-17) — DS разбита на две sub-page (View + Tests)
  через TAB, тесты получили полноэкранный режим.
- **Iteration 10** (2026-04-17) — все чекбоксы панели (L2/R2 enable
  тоглы в TRIGGERS, player LED pairs в TESTS, act индикатор) перешли
  с `[x]/[ ]` на `(x)/( )`. Причина та же что у брекетов кнопок в
  Iteration 8 — битмап-шрифт не имеет глифов для `[` `]` и выдаёт
  мусорные плейсхолдеры. Круглые скобки рендерятся чисто. Изменение
  в `input_debug_draw_checkbox` (общий виджет), `trig_toggle_row`
  и `render_player_led`.
- **Iteration 9** (2026-04-17) — TRIGGERS sub-page реализован. Из
  placeholder'а превращён в полноценный тестер 12 adaptive trigger
  эффектов с полной настройкой параметров, независимыми тогглерами
  L2/R2 и read-only индикаторами (position / fb / act / slot dump).
  Попутно: libDualsense расширена 5 новыми factory функциями
  (Galloping, Simple_Weapon, Simple_Vibration, Limited_Feedback,
  Limited_Weapon) из duaLib, bridge получил соответствующие врапперы
  + `ds_trigger_bow_full` / `ds_trigger_machine_full` (старые
  упрощённые версии оставлены для игрового кода). При выходе из
  sub-page (TAB) триггеры автоматически глохнут.
- **Iteration 8** (2026-04-17) — финальный рефакторинг layout'а под
  физический контроллер:
  - Убраны декорации `[ ]` / `   ` в `input_debug_draw_button` — только
    цвет. Брекеты в битмап-шрифте рендерились мусорными глифами.
  - Stick title центрирован над боксом (было left-aligned).
  - **DS VIEW**: touchpad в центре сверху, L1/R1 над L2/R2, Share/Options
    по центру стик-колонок, D-pad / Face ромбами по центру стиков,
    PS/Mute между стиков, act/fb перенесены в TRIGGERS sub-page.
  - **DS TRIGGERS** sub-page (3-я в TAB цикле) как placeholder для
    adaptive trigger tester.
  - **Xbox страница** переделана в DS-style layout: Y/X/B/A ромб,
    U/L/R/D d-pad, LB/RB над LT/RT, Back/Start в центре экрана под
    Xbox кнопкой, Xbox (Guide) в верхнем центре.
  - **Xbox TESTS** sub-page (TAB) для rumble (без lightbar/LEDs).
  - `D-pad → stick` эмуляция в `gamepad.cpp` пропускается когда
    панель активна — иначе жмёшь d-pad и стик "прыгает".

## Зачем это написано

В GAMEPLAY_CHANGES.md этого быть не должно — там player-facing изменения.
Панель — dev-инструмент. Записал сюда для сохранения контекста между
сессиями: кому-то, кто будет через месяц продолжать эту работу, полезно
знать архитектуру панели, ключевые решения (изоляция, прокси, координатные
пространства) и открытые вопросы.
