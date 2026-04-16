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
  (5 светодиодов над touchpad), индикаторы feedback/act от adaptive triggers,
  тачпад X/Y с двумя пальцами
- В будущем — тест adaptive trigger effects (effect cycle + tunable params)

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

`s_gamepad_state_raw` захватывается в `gamepad_poll` **до** скраба —
поэтому скраб не мешает панели видеть реальный ввод.

**Снизу футер** — строка напоминает клавиши:
`arrows navigate | left/right adjust | Enter activate | 1/2/3 switch page | F11/ESC exit`

## Навигация

Все взаимодействия внутри страницы — стрелками и Enter (TV-remote style),
никаких хоткеев-на-каждую-функцию:
- **↑/↓** — курсор между строками меню
- **←/→** — изменить числовое значение выбранной строки
- **Enter** — активировать action-строку

Edge-detect единый на уровне панели в `input_debug_tick` — пишет в
`s_nav` struct, виджеты читают через `input_debug_nav()`. Один цикл =
одно нажатие = один edge. Никакого double-consume.

Курсор — per-page:
- KB страница: курсора нет (только просмотр)
- Xbox страница: 3 строки (rumble test)
- DualSense страница: 9 строк (rumble 3 + lightbar 3 + player LED 3)

Страница вычисляет `total_rows` и хендлит up/down. В виджет передаётся
`local_cursor = page_cursor - widget_offset`; если в диапазоне виджета —
виджет считает себя focused.

## Страницы

### Keyboard
Просто список удерживаемых скан-кодов (hex + decimal). Читается через
`input_debug_key_held`. Заглушка — в будущем сделаем клавиатурную картинку
с подсветкой реально нажимаемых клавиш.

### Xbox controller
- Два стика: визуальный бокс 96×96 с крестом и красной точкой по отклонению
- LT/RT бары 22×96 с жёлтой заливкой снизу пропорционально значению
- Все кнопки (A/B/X/Y/LB/RB/Back/Start/LS/RS/DPad ↑↓←→) как лейблы
  "[A]" зелёный нажато / " A " серый отпущено
- Raw числа внизу (lX/lY/rX/rY/LT/RT) для верификации визуализации
- **Rumble test** (3 строки): low motor / high motor / fire 200ms pulse

### DualSense controller
Всё что у Xbox + DualSense-специфика:
- PS-подписи кнопок (Cross/Circle/Square/Triangle, L1/R1, Share/Options/PS/Mute)
- Индикаторы **act + fb** справа от каждого trigger-бара — state из HID
  feedback байта (0x29 для R2, 0x2A для L2)
- **Touchpad XY** — прямоугольник 160×64 (грубо масштаб 1920×1080 ÷ 12).
  Зелёная точка finger1, пурпурная finger2. Текст ниже с координатами.
  Парсинг в `libDualsense/ds_input.cpp` (fingers 1 и 2 по offsets 0x20 / 0x24)
- **Rumble test** — тот же общий виджет что на Xbox (routes через
  `gamepad_rumble` → `ds_set_vibration`)
- **Lightbar (live)** — три ручки R/G/B, 0..255 шаг 16. Live apply через
  `ds_set_lightbar` при любом изменении — подсветка меняется сразу
- **Player LEDs (live)** — три пары-тогглера: Outer / Inner / Center.
  Enter на строке зажигает/гасит. У hardware DualSense 5 логических LED но
  биты маски (1,5) и (2,4) зеркальные на уровне firmware — поэтому логических
  групп всего 3, соответствуют `PlayerLed::{Outer,Inner,Center}` константам
  в libDualsense

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

## Рендер в HUD pipeline

Всё рисуется одним `PANEL_start()` / `PANEL_finish()` батчем в
`input_debug_render()` — это обязательная обёртка иначе геометрия не
флашится в кадре и попадает в shadow page через VB slot reuse (баг
описан в [shadow_corruption_investigation.md](shadow_corruption_investigation.md)).

Вызов `input_debug_render()` идёт сразу после `OVERLAY_handle()` в
game.cpp — то же место где рисуется health bar HUD.

## Известные открытые задачи и баги

### Баги (надо чинить)

**Точки стиков / заливка триггеров / точки тачпада не рисуются.**
Визуально видны только **фоновые прямоугольники** виджетов (тёмно-серые
backgrounds боксов стиков, outline трбar'ов, рамка тачпад-бокса) и
**текст** рядом с ними. А вот:
- Красная точка 4×4 на стиках (deflection dot) — не появляется
- Жёлтая заливка снизу trigger-бара пропорционально значению — не появляется
- Крестик-перекрестие через центр stick-бокса — не видно
- Зелёная/пурпурная точка пальца на touchpad-визуализации — не появляется

Что работает: фоновые rect'ы есть, значит AENG_draw_rect доходит до
пайплайна. Что не работает: rect'ы с более высоким layer value (`LAYER_ACCENT`=12
для точки/заливки vs `LAYER_CONTENT`=11 для фона). Гипотеза: z-order
неправильно работает внутри одного POLY batch / одной страницы — возможно
higher layer не даёт "в-front" эффект как ожидалось. Или rect очень
маленький (4×4) и теряется в каком-то culling'е.

Проверить:
- Перевернуть layer ordering (backdrop на 12, точка на 10) — посмотреть
  инвертируется ли
- Нарисовать тест-точку без layer-фокусов, просто на `POLY_PAGE_COLOUR`
  без отличий от фона — если всё равно не видно, проблема не в layer
- Проверить что высота/ширина rect'а > 0 после логического→пиксельного
  скейла (если масштабирование даёт 0×0 — rect невидим)

**Не все DualSense-кнопки переключают `active_input_device` на DualSense.**
Репро (2026-04-17): активное устройство клавиатура (последним был ввод с
клавы), жму кнопки на DS — на вкладке DualSense должны "зажигаться"
лейблы, но некоторые кнопки не перещёлкивают active в DUALSENSE и потому
не видны как нажатые. Подтверждено как минимум:
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
finger-on-touchpad отдельно: добавить проверку `ds_get_input().touchpad_finger_1_down
|| ..._2_down` как триггер активности — если это желаемое поведение
(стоит обсудить).

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

### Дизайн на будущее

- **Layout кнопок "как на реальном геймпаде"** — сейчас просто сетка 4×N
  лейблов. Должно визуально повторять реальное расположение кнопок DS5 /
  Xbox (face buttons в форме ромба, shoulders сверху, triggers под ними,
  D-pad слева от стика, тачпад над face buttons для DS и т.д.)
- **Adaptive trigger effect tester (DualSense)** — селектор эффекта
  (Off/Weapon/Feedback/Bow/Vibration/Machine/Simple_Feedback) + tunable
  параметры с arrow-nav. Живое применение через `ds_trigger_*` функции
  bridge'а. Тестировать act bit и feedback state в панели без зависимости
  от игрового weapon_ready state'а
- **Keyboard страница — картинка клавиатуры** с подсветкой нажатых
  клавиш, вместо голого списка скан-кодов

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

## Зачем это написано

В GAMEPLAY_CHANGES.md этого быть не должно — там player-facing изменения.
Панель — dev-инструмент. Записал сюда для сохранения контекста между
сессиями: кому-то, кто будет через месяц продолжать эту работу, полезно
знать архитектуру панели, ключевые решения (изоляция, прокси, координатные
пространства) и открытые вопросы.
