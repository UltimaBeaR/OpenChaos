# Переработка управления — девлог

Рабочий лог по большой задаче «Переработка управления» из 1.0-багов.
Скоп и требования: `known_issues_and_bugs/known_issues_and_bugs_resolved.md` →
запись «Переработка управления (геймпад + клавиатура + опции)».

Связанные баги в 1.0 которые **закрываются здесь же** (по гипотезе — общая
природа):
- **Перекат вместо прыжка в сторону после климба.** Игрок только что подтянулся
  и сразу пробует прыгнуть в сторону через стик + ✕ — получает перекат. В issue
  записано как «самопроизвольный side-jump после климба»; «side-jump»
  имеется в виду именно **боковой прыжок-перекат**, формулировки одно и то же.
- **Перекат при недожатом стике + ✕.** Когда амплитуда стика маленькая, ✕
  даёт **перекат** (в issue упомянуто «в странную сторону» — направление
  тоже определяется криво из-за малой амплитуды).
- **Авто-перекаты при «движение + jump»** — нужно убрать целиком, перекаты
  будут только через явный L2-модификатор.

**Гипотеза общей природы:** во всех трёх случаях текущая логика, видимо,
интерпретирует ✕ как «перекат», когда стик в **низком/неоднозначном**
состоянии (после климба стик чаще всего ещё не разогнан с центра; «недожатый»
— то же самое в чистом виде; «движение + jump» — порог не строгий и иногда
проваливается в roll-ветку). После переноса перекатов под явный L2-gate
авто-роллы исчезают как класс — но проверить нужно все три кейса.

Не входит в скоп этого девлога (рядом по теме, чиним отдельно):
- Bluetooth/Hot-plug input sync (R3 на свежеподключённом DualSense,
  стик↔клава полушаг, отключение DualSense → зависание клавы в меню).
  Отдельный класс (HID enumeration / device-switch), лежит в
  `known_issues_and_bugs.md` и решается параллельно.

## Связанные источники

- **[input_system/](input_system/)** — низкоуровневая инфраструктура ввода
  (`input_frame`: edge-detect, sticky press, stick-as-direction, auto-repeat).
  Уже готова. Через её API будем читать L2, стик, ✕ — никаких прямых
  `Keys[]` / `rgbButtons[]`. Continuous-стик: `input_stick_x/y`. L2 как
  digital-модификатор: пороговая проверка на `input_trigger(15)` (DualSense)
  / Xbox LT.
- **[input_system/full_plan_deferred.md](input_system/full_plan_deferred.md)** —
  отложенный план actions / remap UI / контекстных групп / reverse-lookup
  glyph'ов. **В 1.0 не делается**, но идеи (особенно контексты
  on-foot/car и split-vs-combined для use/crawl/sprint) полезны как
  справка. В нашей задаче мы:
  - **НЕ** вводим полноценный action-слой (читаем физический ввод через
    `input_frame` напрямую).
  - **НЕ** делаем remap UI и reverse-lookup glyph'ов.
  - **НЕ** трогаем combined-кнопку use/crawl/sprint (см. issue, пункт «D»).

---

## Финальный дизайн геймпада (1.0)

**L2 — мультитул-модификатор (hold).** Игрок ощущает его как «зажми
для альтернативного поведения». Внутри может быть реализован как hold или
toggle — для UX это hold.

Семантика L2:
- **L2 + стик в бок + ✕** → перекат в эту сторону.
- **L2 + стик назад + ✕** → сальто назад.
- **L2 + стик без ✕** → slow-walk + tanky-controls (см. таблицу).
- L2 не выбирает «режим», в который надо специально входить — просто
  модификатор-аккорд. Tanky-controls — побочный эффект, оставлен
  сознательно для редких случаев «хочу повернуться, а не побежать в бок».

**Стик назад = всегда slow-step назад** (даже без L2, даже на бегу). Нужно
для спуска задом по лестнице и пр.

**В боевом режиме L2 не задействован.** Боевая ветка обходит всю L2-логику
— там своя модель (боевые шажки с ориентиром в противника). Side-jump в бою
уже работает через одиночный ✕ — это **сохраняем как есть**, ничего не
переписываем, только не сломать. Tanky-controls и slow-walk в бою тоже
не активируются.

### Таблица — без L2 (обычный режим)

| Стик | без ✕ | + ✕ |
|------|-------|-----|
| нейтраль | стой | вертикальный прыжок на месте |
| вперёд | бег вперёд | дальний прыжок вперёд (как сейчас) |
| влево / вправо | бег вбок | прыжок вбок |
| назад | slow-step назад | **НЕ** back-jump (см. фикс бага #2) — обычная директивная логика |
| назад + бок | (бок доминирует, бег вбок) | обычный прыжок в эту сторону |

### Таблица — L2 зажат

| Стик | без ✕ | + ✕ |
|------|-------|-----|
| нейтраль | стой | вертикальный прыжок |
| вперёд | slow-walk вперёд | обычный прыжок вперёд (= сейчас «slow + jump») |
| влево / вправо only | **поворот L/R** (tanky) | боковой перекат |
| назад | slow-walk назад | **сальто назад** |
| вперёд + бок | slow-walk вперёд с одновременным доворотом в эту сторону (= d-pad вперёд+бок, но медленно) | как «вперёд + ✕» (директивный прыжок вперёд с уклоном) |
| назад + бок | slow-walk назад с доворотом | **по доминанте**: ближе к боку → side-roll; ближе к назад → сальто назад |

### Баг-фиксы (в скоупе этапа)

1. **Climb-exit + side + ✕ → перекат.** Убрать roll-ветку у climb-exit,
   оставить дефолтный directional jump.
2. **«Slow walk + side + ✕ → перекат», «slow walk + back + ✕ → back-jump».**
   Автоматически уходят, потому что:
   - Перекаты теперь только через `L2 + side + ✕`.
   - Сальто назад теперь только через `L2 + back + ✕`.
   - Без L2 эти комбо неоткуда триггерить — проверим что не осталось
     альтернативного пути в roll/back-jump.
3. **Side-roll «не со всех позиций»** (текущее tanky-controls + rotation-hold).
   Найти лишний предикат в action-tree.

### Зацепки по реализации

- В коде раньше **уже была кнопка переключения на медленную хотьбу**
  (история репо). Найти её реализацию при старте — это и есть фундамент
  L2-state, просто перевешиваемый под другой trigger.
- L2 у перса свободна (машина — отдельный input context); перепроверить
  что нигде не пересекается.
- Xbox LT — аналоговый, нужен порог для digital-семантики модификатора.
  Выбрать значение по аналогии с порогом R2/L2 у DualSense (тот же `input_trigger`).
- Возможно нужна гистерезис у L2-порога чтобы режим не дребезжал на границе.
- D-pad движение **не дублируем**. После реализации он свободен и
  перепрофилируется отдельно (вероятно — смена оружия и т.п.).

### Sanity-сценарии для теста

- Рукопашка — L2 ничего не меняет; ✕ работает как раньше.
- Side-roll из бега (L2 + бок + ✕ когда уже движемся).
- Side-roll стоя на месте (L2 + бок + ✕ от нейтрали).
- Прижатие к стене (◯ hold) + L2 — нужно ли там вообще L2 что-то менять?
- Спуск по лестнице задом (стик назад без L2 vs с L2).
- Climb-exit во все стороны.
- Стик в каждую из 8 направлений + ✕ с L2 и без L2 (16 кейсов матрицы).

### Подзадачи реализации

- [x] Найти где триггерятся перекаты и back-jump в person/action.
- [x] Найти старую кнопку «toggle slow walk» — её state-машина — фундамент.
- [x] Перепроверить что L2 у перса свободна.
- [x] Прокинуть L2 как modifier (с порогом + гистерезис) — Phase A.0
      (биндинг к `m_bForceWalk` + `player_relative`).
- [x] Убрать старые авто-триггеры перекатов/back-jump на стике
      (source-aware split, D-pad/клава пока сохраняют legacy).
- [x] Починить climb-exit (баг #1) — для стика. D-pad/клава пока legacy.
- [ ] Связать L2-modifier с roll / back-flip на стике (Phase A.1).
- [ ] Унифицировать D-pad/клава под L2-схему (Phase A.1, конечный этап).
- [ ] Сохранить бой нетронутым — explicit гард что combat-ветка
      обходит L2-логику (проверить после A.1).
- [ ] Починить side-roll «не со всех позиций» (баг tanky-controls).
- [ ] Пройти 16-кейсовую матрицу + lab-тесты на лестнице, climb, бою.

## Этап A2 — Уход от D-pad для движения

После A проверить что весь функционал движения доступен со стика.
D-pad перепрофилируется отдельно (вероятно — смена оружия).

## Этап A3 — Кувырок vs прыжок в сторону (action tree priorities)

При беге + резком повороте стика + ✕ персонаж делает кувырок (flip)
вместо прыжка в сторону. Особенно мешает паркуру. Пересмотреть пороги
в action tree чтобы прыжок в сторону срабатывал предсказуемо, а кувырок
— только при явном намерении.

## Этап B — Клавиатура + мышь

Делается **после** того как геймпадная схема выше готова и оттестирована.
Принцип — **переиспользуем тот же L2-tactical стейт-машинно**, просто
кнопки другие и вместо стика — WASD.

- Дефолты — **WASD**.
- **Мышь = камера** (как правый стик). Без стрельбы/прицеливания мышью
  (это post-1.0, см. «Mouse shooting»).
- Движение **в world-space** (stick-like), не tank-controls.
- Открытое решение: «назад + бок + jump» с L2-эквивалентом — по доминанте
  невозможно (на клаве direction дискретный). Скорее всего — **прыжок назад**
  (см. ответ пользователя по дизайну). Решим при реализации Этапа B.
- Какой клавишей будет L2-аналог — выберем при реализации (вероятно
  Shift/Alt). Должно ощущаться как hold-модификатор.

## Этап C — Опции

- Переключалка **Tank controls ↔ Stick-like** — общая для клавы и
  геймпада, применяется сразу к обоим.
- Решение по полноценной кастомной системе настройки клавиш отложено —
  см. Задачу #7 в `new_game_planning/stage12.md` (вариант (1)-(4)).
  В этом девлоге **не** проектируем кастомизацию — только дефолты и
  Tank/Stick-like toggle.

## Этап D — Финальная валидация (перед смоук-тестом)

Не часть смоук-теста, а отдельный чек:
- Клава покрывает все возможности геймпада и наоборот.
- DualSense и Xbox функционально неотличимы.
- Геймпад-чит (комбинации стиков/кнопок) — продублирован для клавы
  другим механизмом (последовательность клавиш / комбо).

---

## Что НЕ делаем в 1.0

(из issue, для памяти):
- Развязка кнопки «Action» на отдельные действия (multi-action остаётся).
- Стрельба/прицеливание мышью.
- Steam Input API.
- Контекстные подсказки кнопок по игре.

---

## Находки и решения

### Триггеры авто-перекатов и авто-backflip (2026-05-24)

Перекаты (side-roll) и backflip триггерились в **трёх местах** независимо
от амплитуды стика — только по digital-флагу `INPUT_MASK_LEFT/RIGHT/BACKWARDS`
(порог в `get_hardware_input` — раскладка раньше 4096 raw от центра):

1. **`action_idle[]`** entries 0-1 — прямой путь idle + JUMP + LEFT/RIGHT
   → `ACTION_FLIP_LEFT/RIGHT` (минуя STANDING_JUMP).
2. **`action_standing_jump[]`** entries 0-1 — mid-jump переключение, любой
   side-tap во время прыжка переводил в roll.
3. **`case ACTION_STANDING_JUMP:` body** (input_actions.cpp:2413-2431) —
   обе ветки `should_i_jump` true/false вызывали `set_person_flip()`
   на LEFT/RIGHT. Backflip — тоже отсюда: BACKWARDS + `should_person_backflip`
   → `set_person_standing_jump_backwards`.

Гипотеза «низкая амплитуда стика → roll» оказалась неточной: roll
триггерился **всегда** на digital-флаг, независимо от амплитуды и состояния.

`should_i_jump()` — это **не** про скорость или климб. Проверяет 4 точки
вокруг персонажа в warehouse-карте: если мапятся в разные тайлы (стоишь
в дверном проёме склада) → false. То есть это редкий гейт; в основном
true. Climb-exit попадает в false-branch потому что 4 точки спанят край
warehouse-тайла.

### Орфанная инфраструктура slow-walk и tanky-controls

Готовое к переиспользованию под L2:

- `m_bForceWalk` (`game/input_actions_globals.h:24`) — глобальный флаг
  slow-walk. Читается в `process_analogue_movement()`. Не пишется никем.
- `player_relative` (`game/input_actions_globals.h:10`) — глобальный флаг
  tank-controls (повороты вместо strafe). Читается в анимационном/движенческом
  коде. Инициализируется 0, не пишется.
- `Person.Mode` enum: RUN/WALK/SNEAK/FIGHT/SPRINT — уже переходит между
  состояниями через `process_analogue_movement` по velocity и `m_bForceWalk`.
- Старая кнопка `JOYPAD_BUTTON_MOVE = 31` — toggle slow walk, помечена
  `unbound; walk mode removed`. Историческое название отвязанного слота.

Под L2 будем писать `m_bForceWalk` и `player_relative` — никакой новой
state-машины делать не надо.

### L2 свободна на ногах (проверено)

L2 (физическая кнопка index 15) используется только в:
- Тормоз машины (`apply_button_input_car`, vehicle.cpp:2414).
- Старый kick-эвалуатор (kick result intentionally discarded —
  комментарий «kick moved from L2 to R1»).
- Cheat-комбо `Select + L1 + L2 + D-pad`.

Комментарий «L2 is the ACTION button on foot now» — **stale**. Реальная
привязка ACTION идёт на физическую кнопку index 1 (◯/B) через
`joypad_button_use[JOYPAD_BUTTON_ACTION] = 1`.

---

## Открытые вопросы

*Заполняется по ходу.*

### Дебажные функции с мышью при включённом capture

Под `bangunsnotgames` две отладочные клавиши читают позицию курсора в окне:
- **Shift+F12** — спавн мины в точке курсора (`game_tick.cpp:2391`)
- **KB_G** — телепорт Дарси в точку курсора (`game_tick.cpp:2476`)

После того как заработает mouse capture (курсор прячется в режиме игры),
точку курсора пользователю не видно → этими дебаг-функциями станет невозможно
пользоваться. Сама капча не блокирует чтение `MouseX/Y` (последняя
позиция до захвата всё равно сохраняется), но кликнуть «в нужную точку»
без видимого курсора не выйдет.

Варианты решения (когда дойдут руки, не блокирует основную задачу):
- (a) рисовать debug-crosshair только когда дебаг-клавиша зажата
- (b) временно отжимать capture пока зажата debug-клавиша
- (c) использовать центр экрана как точку без участия курсора вообще
- (d) toggle debug-режима который освобождает курсор для дебаг-сессии

Сейчас не делается — фичи под debug-гейтом, не релизные.

---

## Связанные коммиты

- **2026-05-24** — Phase A.0: биндинг L2 как on-foot модификатор
  (`new_game/src/game/input_actions.cpp`, в `get_hardware_input`).

  **Финальный дизайн — sticky регим-машина.** Изначально пробовали
  per-tick классификацию по углу стика, но это вызвало баг: при плавном
  переводе стика из forward-зоны в side-зону во время хотьбы анимация
  ломалась (walk anim играл без advance кадров, мелкое движение). При
  pure-back инициации стика перс разворачивался в камеру и шёл к ней
  передом, а не пятился. Причина — режим переключался прямо во время
  движения, движок не успевал корректно среагировать.

  **Решение:** регим определяется **один раз** в момент когда стик
  выходит из нейтрали при зажатом L2, дальше регим залипает до
  возврата стика в нейтраль или отпуска L2.

  | Регим | Триггер на выходе из нейтрали | Что делает |
  |-------|-------------------------------|-----------|
  | NONE | стик в deadzone или L2 не зажат | ничего |
  | FORWARD | `dy_c < 0` (любая forward-компонента) | `m_bForceWalk=TRUE`, `player_relative=0` → camera-relative slow walk. Поворот стика → перс рулится и идёт в новую сторону (как в беге) |
  | BACKWARD | `dy_c > 0` (любая back-компонента) | `m_bForceWalk=TRUE`, `player_relative=0` (camera-relative); force-entry walk_back из `player_turn_left_right_analogue` минуя tank-mode threshold. Перс сходится к facing-opposite-of-stick (а не крутится непрерывно), затем пятится в направлении стика |
  | ROTATE | pure-side зона (`|dy| < |dx|/2`, конус ~27°) | `analogue=0` + чистка analog-битов + чистка `INPUT_MASK_FORWARDS/BACKWARDS/MOVE` → digital handler → кручение в место без translation |

  **BACKWARD: история итераций.**

  *Попытка 1 — `player_relative=1` + переиспользование tank-mode auto-entry.*
  `set_person_walk_backwards` вызывается из `player_turn_left_right_analogue`
  когда `player_relative==1` и `|dangle| > 624`. Это сработало для входа,
  но exit threshold (600) выкидывал перса из walk_back при прокрутке
  стика — игрок крутил стик из back в forward через круг и движок
  переключался на forward running.

  *Попытка 2 — `player_relative=1` + skip-exit (`l2_regime` файл-скоуп,
  гейт в `player_turn_left_right_analogue` около line 1633).* Это
  починило bug #2 (стик forward → forward running), но осталось bug #1:
  перс при tank-mode непрерывно крутится на ходу. Причина —
  `target_angle = stick_angle + char.Angle` в tank-mode СЛЕДУЕТ за
  char.Angle: angular delta между стиком и facing остаётся константной,
  перс крутится бесконечно, никогда не сходясь.

  *Попытка 3 — `player_relative=0` (camera-relative) + явный force-entry/exit
  walk_back.* В camera-relative target_angle в world space → перс сходится
  за пару тиков на «facing opposite of stick», дальше идёт ровно жопой
  туда куда стик. Auto-entry tank-mode не работает с `player_relative=0`
  (он внутри `if (player_relative)` блока), поэтому force-entry добавлен в
  `player_turn_left_right_analogue` в самом начале: если `l2_regime ==
  BACKWARD` и не в walk_back — вызвать `set_person_walk_backwards`.
  Соответственно force-exit когда регим уходит из BACKWARD. Skip-exit
  логика из попытки 2 оставлена как defensive (мёртвый код если
  `player_relative=0` всегда, но не вредит).

  *Попытка 3.1 — расширение force-entry со STATE_IDLE до STATE_IDLE ||
  STATE_MOVEING.* Первая версия гейтила только на STATE_IDLE, что
  пропускало переходное состояние — игрок отпускал стик, движок начинал
  замедлять перса, но State ещё был MOVEING; игрок жал L2 + back в этот
  момент → force-entry не срабатывал → camera-relative analog вёл перса
  лицом к стику. Расширение на MOVEING покрывает этот кейс.
  `set_person_walk_backwards` безопасно вызывается из MOVEING (он сам
  ставит State+SubState).

  *Попытка 3.2 — STATE_GUN + STATE_HIT_RECOIL добавлены.* С оружием
  наголо (пистолет/AK/дробаш) перс сидит в STATE_GUN. Force-entry не
  срабатывал → BACKWARD регим устанавливался, но walk_back не
  начинался → `process_analogue_movement` фолил в default → `set_person_
  running` → camera-relative analog с back-стиком = поворот лицом к
  камере (= визуально «прокручивающаяся анимация») + бег к камере. На
  клавиатуре проблемы нет: digital BACKWARDS handler в
  `apply_button_input` явно перечисляет `case STATE_GUN:
  set_person_walk_backwards`. Сматчил с keyboard'ом: добавил STATE_GUN
  и STATE_HIT_RECOIL в force-entry condition.

  **ROTATE: analog rotation speed** (доработка после тестов):
  `player_turn_left_right` (digital handler) уже имеет analog ветку при
  непустых packed analog битах (line ~1970: `wTurn = (wJoyX * wMaxTurn)
  >> 7`). В ROTATE регим теперь **не чистим** analog биты 18-31 —
  digital handler автоматически использует analog скорость
  пропорционально отклонению стика. На максимуме отклонения — текущая
  скорость кручения. На полу-отклонении — половина. Clear-ы только для
  INPUT_MASK_FORWARDS/BACKWARDS/MOVE остаются (защита от случайного
  walk-back при диагональном дрейфе).

  **Почему `player_relative=1` НЕ используется** (хотя сначала так и
  пробовали): этот флаг даёт analog-tank где velocity всё равно ставится
  от амплитуды стика — character крутится и движется одновременно
  (curving path). Это не то что нужно: для pure-side нужно «крутится не
  двигаясь», как D-pad arrow alone. Поэтому tank-rotation теперь
  приходит через digital re-routing, а не через флаг.

  **Инфраструктура:**
  - Сброс `m_bForceWalk = UC_FALSE`, `player_relative = 0` в начале
    функции на каждом тике — гарантирует что отпускание L2 / отключение
    геймпада / клавиатурный путь моментально снимают modifier.
    `player_relative` всё равно никем не пишется в `1`, но reset
    оставлен для defense-in-depth (флаг — orphan-глобал).
  - В геймпад-ветке: `input_trigger_raw(15)` (L2/LT, 0..255), гистерезис
    engage > 89 (~35 %) / release < 51 (~20 %), `static bool
    l2_engaged`. Гистерезис автоматически отжимается при возврате 0
    (геймпад отвалился).
  - Очистка analog-битов 18-31 в pure-side случае: тот же трюк
    `input &= 0x0003FFFF` что в keyboard-branch — без него
    `player_turn_left_right` видит мусор в upper bits и интерпретирует
    центрованный стик как analog-вход (см. комментарий в keyboard
    handler).

  **Что ощущается:**
  - Зажал L2 + стик в бок → перс крутится в место, не идёт.
  - Зажал L2 + стик вперёд → медленный шаг вперёд.
  - Зажал L2 + стик forward-side → медленный шаг по диагонали (camera-
    relative, как при «недожатии» стика).
  - Отпустил L2 → возвращается обычный бег.

  Перекаты и back-jump через L2 — в A.1.

  **Согласованность зон стика с A.1:** угловая классификация стика под L2
  (pure-side cone ±27° от горизонтали, пороги по `NOISE_TOLERANCE` и
  ratio 1:2 между minor/major осью) — это **тот же критерий** что будет
  использоваться в A.1 для выбора tactical-действия по ✕:
  - pure-side зона (что даёт rotation здесь) → side-roll в A.1
  - pure-forward зона → forward jump
  - pure-back зона → backflip
  - back-side диагональ → по доминанте оси (см. финальный дизайн выше)
  - forward-side диагональ → forward jump в направлении стика
    (через snap, как сейчас)
  Пороги выбраны один раз и должны быть консистентны — иначе игрок
  получит «крутится тут / не катится при ✕» на одном и том же отклонении
  стика. В A.1 либо переиспользуем те же выражения inline, либо вынесем
  в helper.

- **2026-05-24** — баг-фикс авто-перекатов и авто-backflip
  (`new_game/src/game/input_actions.cpp`), source-aware вариант:
  - Удалены entries 0-1 из `action_idle[]` — priority-маршрут к
    ACTION_FLIP_LEFT/RIGHT мимо STANDING_JUMP больше не нужен.
  - Удалены entries 0-1 из `action_standing_jump[]` — mid-jump side-tap
    → roll switch убран для всех источников (мелкая потеря оригинального
    поведения, цена унификации).
  - В `case ACTION_STANDING_JUMP:` маршрутизация теперь по `analogue`
    глобалу:
    - **analogue == 1 (стик):** FORWARDS / LEFT / RIGHT / BACKWARDS →
      `set_person_standing_jump_forwards` (прыжок в facing-направление,
      тот же путь что running-jump). Перед вызовом — **снап Angle перса
      под направление стика** (`Arctan(-dx, dz) + camera_angle`, та же
      формула что в `process_analogue_movement`). Снап нужен потому что
      в переходных состояниях (например, сразу после climb-up) перс
      ещё не повёрнут под стик — анимация подтягивания блокирует
      аналоговую ротацию. Без снапа running-jump бросал бы перса в
      старое facing-направление, а не туда куда жмёт стик. В running /
      slow-walk снап — no-op (перс уже выровнен по стику). Стик в
      нейтрале → vertical jump. Никаких авто-roll/backflip.
    - **analogue == 0 (D-pad / клавиатурные стрелки):** старое поведение
      сохранено — LEFT/RIGHT → roll, BACKWARDS → backflip, climb-exit
      `should_i_jump=false` тоже даёт roll (как было).
  - Источник истины `analogue`: глобал в
    `game/input_actions_globals.cpp:25`, выставляется в `get_hardware_input`
    (1 при использовании стика, 0 при D-pad / клавиатурных стрелках).
  - Результат: на стике — без авто-roll/backflip, climb-exit без roll.
    На D-pad/клаве — поведение как было. Унификация всех источников под
    L2-схему придёт в Phase A.1.
  - Бой не тронут (`ACTION_FIGHT_KICK` в action_standing_jump остался).
  - `set_person_flip`, `set_person_standing_jump_backwards`,
    `should_person_backflip`, switch-case'ы `ACTION_FLIP_LEFT/RIGHT` —
    **оставлены живыми** (переиспользуем под L2).

  ⚠️ **Регрессия первой версии правки** (зафиксировано в той же
  сессии): первая попытка убрала LEFT/RIGHT/BACKWARDS-ветки безусловно
  → сломала D-pad/клавиатуру (на них тоже пропали roll/backflip).
  Источники D-pad/keyboard сливаются с стиком в один `INPUT_MASK_*`
  набор через `get_hardware_input`, дискриминатор — отдельный глобал
  `analogue`. Урок: при изменениях в action handler всегда проверять
  что вход — это композитная маска от всех источников, и при
  source-specific поведении нужен явный гейт.

- **2026-05-24** — L2 just-engaged window для tactical actions из running
  (`new_game/src/game/input_actions.cpp`):
  - Проблема: бег вперёд + быстрое L2 + стик вправо + ✕ — игрок хочет
    перекат, но регим классифицируется как FORWARD (force-FORWARD при
    running_or_walking, чтобы плавно перейти бег → шаг). ✕ → forward
    jump, не перекат.
  - Конфликт: ранее уже добавили force-FORWARD при беге чтобы избежать
    обратной проблемы (бег вбок + L2 → резко ROTATE и стоп). Теперь
    оба требования нужно совместить.
  - Решение: file-scope счётчик `g_l2_engage_window_ticks` (~6 тиков ≈
    300ms) стартует на L2 not-held → held transition в `get_hardware_input`.
    В `ACTION_STANDING_JUMP` body: пока счётчик > 0, ✕ обрабатывается
    напрямую по зоне стика, минуя регим. Pure-side → roll. Pure-back →
    backflip (с тем же snap'ом что в BACKWARD branch). Pure-forward /
    диагональ / нейтраль → fall through на regime-based логику.
  - Регим классификация **не тронута** — окно влияет ТОЛЬКО на ✕. Так
    что running + L2 без ✕ всё равно даёт FORWARD регим (плавный шаг),
    а running + L2 + быстрый ✕ + side даёт перекат.
  - g_l2_rotate_locked не выставляется в window-path — lock семантика
    предназначена для ROTATE регим (явное намерение «крутиться»), не
    для быстрого run-roll.
  - **Detection permissive**: пер первой версии было `side_only` (LEFT/RIGHT
    set + НЕТ FWD/BACK). При диагональном стике (typical при беге с
    forward inertia) не срабатывало. Расслабил до `has_side` — любая
    боковая компонента даёт roll, даже с диагональной forward/back
    примесью. Player pressed L2 + side intent — он получает roll.
    Dominant-axis нюанс для back+side остаётся в explicit ROTATE/BACKWARD
    регимах, не в окне.
  - **Sprint покрытие**: при обычном беге L2 → m_bForceWalk=true →
    `process_analogue_movement` переходит RUNNING → WALKING на следующий
    тик. WALKING-state action_walk даёт STANDING_JUMP на JUMP → мой
    override срабатывает. При СПРИНТЕ (`Mode == PERSON_MODE_SPRINT`,
    зажатый ◯) переход подавлен — есть `if (Mode != SPRINT)` гейт в
    process_analogue_movement. Char остаётся в RUNNING → action_run даёт
    RUNNING_JUMP (не STANDING_JUMP) → override не доходил. Фикс: тот же
    window-override добавлен в `case ACTION_RUNNING_JUMP` body перед
    дефолтным `set_person_running_jump`.

- **2026-05-24** — Snap facing перед backflip из BACKWARD регим
  (`new_game/src/game/input_actions.cpp`):
  - Проблема: backflip из BACKWARD улетал в неправильном направлении —
    стик ровно назад давал backflip back-left, стик back-left давал
    backflip ровно назад. Со смещением.
  - Причина 1: `set_person_standing_jump_backwards` не меняет `char.Angle`
    — backflip летит исходя из текущего facing. В момент срабатывания
    из BACKWARD регим char только что вошёл в walk_back (force-entry),
    а ротация под stick-target (capped 192/tick) ещё не сошлась.
  - Причина 2 (главная — обнаружена во второй итерации): `person_normal_move`
    (используется walk_back) и `projectile_move_thing` (используется в
    SUB_STATE_STANDING_JUMP_BACKWARDS handler) имеют **противоположные**
    конвенции: первый `dx = -(SIN * Velocity)`, второй `dx = (SIN * Velocity)`.
    Walk_back добавляет +1024 к target чтобы negated формула двигала перса
    к стику. Если использовать тот же +1024 для backflip — un-negated
    формула двигает в противоположную сторону.
  - Третья итерация (финал): «без +1024» дало противоположный
    результат — backflip летит в обратную от стика сторону. Перепроверил
    Arctan и projectile_move_thing формулы более внимательно:
    `projectile_move_thing` с Velocity = -32 двигает в
    `(-SIN(angle), -COS(angle))` = **180° от facing**. Для движения
    к стику facing должен быть 180° **от** стика — это ТА ЖЕ формула
    что walk_back использует через +1024 offset. Так что снап должен
    включать +1024 (как изначально и было).
  - Также: во время backflip полёта (SubState=STANDING_JUMP_BACKWARDS),
    `player_turn_left_right_analogue` продолжает крутить char.Angle к
    target. Без согласованного offset target расходится с моим snap и
    capped rotation (TURN_CAP_JUMP=24/tick × ~10 тиков backflip = 240
    единиц) криво загибает траекторию (этот эффект и был «back-and-left»
    в первой итерации). Фикс: добавил `SUB_STATE_STANDING_JUMP_BACKWARDS`
    в условие +1024 offset в player_turn_left_right_analogue, чтобы
    target rotation = my snap = не было дрейфа во время полёта.

- **2026-05-24** — Убран cooldown переката + failed-сообщение
  (`new_game/src/things/characters/person.cpp`,
  `new_game/src/combat/combat_cooldown.{h,cpp}`):
  - С новой L2-схемой rotation lock (✕ в ROTATE → lock до возврата
    стика в нейтраль) спам переката естественно ограничен — нет смысла
    в отдельном cooldown'е. Игрок физически не может зажарить два
    переката подряд без явного жеста (отпустить стик, опять зажать,
    нажать ✕).
  - Удалено: cooldown-блок из `set_person_flip` (включая
    `PANEL_new_info_message(XLAT_str(X_FAILED))` который добавлялся
    специально для информирования об отказе).
  - Удалено: `COOLDOWN_ROLL` из enum `CombatCooldown` +
    `COOLDOWN_ROLL_SECONDS` define + соответствующая запись в
    `s_cd_turns[]` array.
  - Другие cooldown'ы (slide/arrest/grapple/block) не тронуты.

  *Историческая запись для контекста* — раньше тут стоял cooldown
  чтобы перекат нельзя было заспамить, и `PANEL_new_info_message`
  показывал «failed» при отказе. Теперь оба удалены.

- **2026-05-24** — Убран ROTATE lock
  (`new_game/src/game/input_actions.cpp`):
  - Lock добавлялся специально для случая «✕ в ROTATE → нет переката
    из-за cooldown → перс продолжает крутиться → игрок не понимает
    почему ничего не произошло». После удаления cooldown'а перекат
    всегда срабатывает → lock больше не нужен.
  - Удалено: `g_l2_rotate_locked` global, set'ы lock'а в ROTATE branch
    body, lock suppression блок в get_hardware_input (NONE regime +
    очистка movement+JUMP битов), JUMP-set гейт `!g_l2_rotate_locked`.
  - После переката игрок продолжает удерживать стик в бок → перс
    остаётся в ROTATE → крутится. Это естественно: если игрок не хочет
    крутиться после переката, отпускает стик. Спам переката тоже
    естественно ограничен длительностью flip-анимации (~0.5с).

- **2026-05-24** — ROTATE lock после ✕ (УДАЛЁН, см. выше)
  (`new_game/src/game/input_actions.cpp`):
  - Проблема: игрок крутится (ROTATE) и жмёт ✕ с намерением сделать
    перекат. Если кулдаун проката заблокировал действие — перс
    продолжает крутиться, ничего не произошло визуально, кнопка
    «съедена впустую». Даже если перекат сработал — игрок продолжает
    держать стик в бок, и кручение сразу возобновляется после анимации
    переката. Обескураживает.
  - Решение: ✕ в ROTATE регим выставляет `g_l2_rotate_locked = true`
    (независимо от того сработал ли реально перекат). Lock блокирует
    новую классификацию ROTATE + подавляет stick-движение
    (INPUT_MASK_FORWARDS/BACKWARDS/LEFT/RIGHT/MOVE очищены, packed
    analog биты тоже). Перс стоит на месте. Lock снимается когда стик
    возвращается в нейтраль (или L2 отпущен).
  - Чтобы заново начать крутиться: вернуть стик в нейтраль, опять
    толкнуть в бок → новая классификация → ROTATE.
  - PUNCH/KICK биты не тронуты — игрок может ударить пока в locked
    состоянии. JUMP тоже чистится в locked, иначе последующие ✕
    нажатия проваливались в standing-jump fallback (без movement битов
    → vertical jump на месте, при иных условиях → forward jump). Это
    воспринималось как «L2 ещё что-то делает» — lock должен полностью
    игнорировать ✕ до возврата стика в нейтраль.
  - **Подводный камень**: первая попытка чистила INPUT_MASK_JUMP в L2-блоке,
    но JUMP set из `input_btn_held(JOYPAD_BUTTON_JUMP)` идёт **после**
    L2-блока в `get_hardware_input` (line ~3654). Чистка молча
    перетиралась. Финальный фикс — гейтнуть установку JUMP на
    `!g_l2_rotate_locked` прямо в месте set'а.

- **2026-05-24** — Walk_back: стрельба полностью заблокирована
  (`new_game/src/game/input_actions.cpp`):
  - Баг: при движении назад (L2 BACKWARD регим или клавиатурная стрелка)
    стрельба бажила по разному: дробаш 1 выстрел потом нихуя, AK
    вибрация без выстрелов, пистолет без анимации. Конфликт
    state/anim между walk_back и shoot.
  - Пробовали разные варианты «exit walk_back → shoot нормально»:
    set_person_idle, set_person_aim, lock-machine с supression
    движения. Все давали остаточные глюки анимации (frozen pose с
    fire-effect, single shot потом stuck, etc).
  - Финальное решение — **полный блок стрельбы** во время walk_back.
    Игрок отпускает стик назад чтобы стрелять.
    - `set_player_shoot`: early return если SubState == WALKING_BACKWARDS.
      Покрывает три call site'а (ACTION_SHOOT body в apply_button_input,
      PUNCH-with-gun-out path в do_an_action, aim-mode first-person
      shoot в apply_button_input_first_person).
    - `case ACTION_FIGHT_PUNCH`: action_walk_back маппит PUNCH →
      ACTION_FIGHT_PUNCH, который зовёт `set_person_shoot` напрямую,
      минуя set_player_shoot. Гейт повторён здесь.
  - NPC не затронуты — у них отдельный код стрельбы (pcom.cpp).
  - **Trigger-effect/vibration тоже отключена** (`game.cpp:1245+`):
    `non_firing_state` определялся только по `State`, а walk_back — это
    SubState внутри STATE_MOVEING, поэтому не покрывался. Добавил
    отдельный chek `if (SubState == SUB_STATE_WALKING_BACKWARDS)
    non_firing_state = true`. Это отключает trigger effect (Weapon25
    click / Machine pulse) при walk_back — игрок не чувствует rumble
    при пустом нажатии R2 пока пятится.

- **2026-05-24** — Унификация stick "pure axis" cone + dangling pre-filter
  (`new_game/src/game/input_actions.cpp`):
  - Вынес ratio 1:2 (~27° от dominant axis) в file-scope константу
    `STICK_AXIS_DOMINANCE_RATIO = 2`. Используется в:
    - L2 ROTATE classification (pure-side → ROTATE).
    - L2 forward↔back zone tracker (PURE_FORWARD / PURE_BACK / SIDE_ISH).
    - Hug-wall handler (pure-side → шаг вдоль / иначе → break out).
      Раньше был 45° threshold (`|dx|>|dz|`), теперь 27° — должно
      чувствоваться более «строго» (диагональ выводит из hug,
      раньше оставалась).
    - Dangling pre-filter (см. ниже).
  - **Dangling pre-filter** (`STATE_DANGLING`): когда перс висит
    перед climb-up, action_dangling даёт приоритет PULL_UP на
    INPUT_MASK_FORWARDS. Любое лёгкое отклонение стика вверх →
    PULL_UP срабатывает, side traverse невозможен. Pre-filter в
    `apply_button_input` (до action selection) определяет dominant
    axis стика по тому же cone:
    - pure-vertical (|dy| > ratio*|dx|) → оставляем FWD/BACK, чистим
      LEFT/RIGHT.
    - else (диагональ или pure-side past deadzone) → оставляем
      LEFT/RIGHT, чистим FWD/BACK/MOVE.
    Гейтнуто на stick_active (|dx_c|>8192 || |dy_c|>8192) чтобы не
    ломать клавиатурный ввод когда стик в нейтрали.

- **2026-05-24** — L2: резкий FORWARD↔BACKWARD переключает регим
  (`new_game/src/game/input_actions.cpp`):
  - Раньше FORWARD/BACKWARD регимы были полностью sticky — стик
    forward→back без возврата в нейтраль оставлял FORWARD, перс
    разворачивался лицом к камере и шёл к ней (camera-relative
    анализ). Не интуитивно — игрок ожидает что резкое переключение
    стика сменит направление движения.
  - **Финальная реализация — отслеживание зоны стика.** Каждый тик
    определяется зона: NEUTRAL (deadzone), PURE_FORWARD
    (|dy|>2*|dx| и dy<0), PURE_BACK (|dy|>2*|dx| и dy>0), SIDE_ISH
    (всё остальное past deadzone — включая чистую сторону и
    диагонали). `static prev_stick_zone` хранит последнюю
    не-нейтральную зону. Если cur=PURE_BACK и prev=PURE_FORWARD →
    re-classify FORWARD→BACKWARD. И наоборот. NEUTRAL не обновляет
    prev (deadzone transit не перетирает направление).
  - При rolling через side: первый же не-pure-forward тик
    (диагональ или pure-side) переключает prev в SIDE_ISH. Дальше
    когда стик доходит до pure-back, prev=SIDE_ISH → не re-classify.
    Rolling всё ещё даёт rotation как и раньше.
  - При прямом slam: stick идёт через deadzone (NEUTRAL не обновляет
    prev) или резко перескакивает на pure-back. prev остаётся
    PURE_FORWARD → re-classify срабатывает.
  - Раньше пробовали single-tick dy sign-flip с обоими тиками past
    NOISE_TOLERANCE. Это срабатывало через раз (slam может занять
    2-3 тика, в транзите тик может быть в deadzone → sign-flip не
    детектировался) и давало ложные срабатывания на rolling (быстрый
    rolling мог за один тик пройти от forward до back, скипнув side).
    Зона-based подход устраняет оба эти класса бага.

- **2026-05-24** — Aim-mode (L1 hold) — левый стик полностью зеркалит правый
  (`new_game/src/game/input_actions.cpp`, `apply_button_input_first_person`):
  - В первое-лицо/aim-режиме (L1 hold = камера сблизка из-за головы)
    yaw + pitch раньше шли только с правого стика. Левый стик
    использовался для движения. Игроку хотелось управлять обзором с
    того же стика на котором ходит.
  - Добавлено: чтение `input_stick_x_axis_raw/y_axis_raw(INPUT_STICK_LEFT)`
    с теми же формулами и порогами что у правого стика
    (`STICK_YAW_MAX = 32`, `STICK_PITCH_MAX = 13` per frame at full
    deflection). X → yaw, Y → pitch. _raw — чтобы D-pad не мирился в
    стик (consistent с общим on-foot отключением D-pad).
  - Движение левым стиком (через INPUT_MASK_MOVE) сохраняется — левый
    стик одновременно и двигает, и управляет обзором.
  - **Приоритет правого стика per-axis**: при отклонении обоих стиков
    их вклад не складывается. Берётся правый если он past deadzone на
    оси, иначе левый. Per-axis (отдельно X и Y) позволяет крутить
    одним стиком а смотреть вверх-вниз другим, если игрок хочет.

- **2026-05-24** — D-pad больше не движет персонажа + hug-wall работает на стике
  (`new_game/src/game/input_actions.cpp`):
  - **D-pad на контроллере отключен** для on-foot движения. В
    `get_hardware_input` поменял `input_stick_x_axis` →
    `input_stick_x_axis_raw` (PRE-D-Pad-override) для X и Y. D-pad
    больше не миррорится в stick → не выставляет
    INPUT_MASK_LEFT/RIGHT/FORWARDS/BACKWARDS. Условие
    `!input_dpad_active()` тоже убрано (всегда analogue=1 при
    подключенном геймпаде). D-pad кнопки сами по себе всё ещё читаются
    через `input_btn_held(11..14)` для меню. Клавиатурные стрелки не
    тронуты — keyboard branch использует input_key_held как раньше.
  - **Hug-wall + стик** теперь дифференцированно: бок-доминантный
    стик (|dx| > |dz|) → шаг вдоль стенки (через action_hug_wall ⇒
    ACTION_HUG_LEFT/RIGHT). Forward/back-доминантный стик → break out
    (прямой вызов `set_person_running`) — матчит поведение D-pad/стрелок
    где вперёд/назад выводят из hug.
    До: на любой стик `process_analogue_movement` валился в default →
    `set_person_running` → отрыв на любом отклонении (включая чистый
    бок) → шаг вдоль стенки на стике был невозможен.
    HUG_WALL_TURN substate (анимация поворота-в-стенку) сохранена с
    безусловным return — её не прерываем ни в каком случае.

    **Подводный камень**: первая итерация фолбилась через case-fallthrough
    к default-ветке, но fallthrough попадал в `case STATE_CARRY` с
    гейтом `if (SubState != STAND_CARRY) return;` — hug-wall SubState
    никогда не STAND_CARRY → return → break-out не срабатывал. Финальная
    версия зовёт `set_person_running` прямо в HUG_WALL case.

- **2026-05-24** — Phase A.1: перекаты и сальто назад через L2-регим
  (`new_game/src/game/input_actions.cpp`, в `case ACTION_STANDING_JUMP`):
  - L2 ROTATE регим + ✕ → side roll в сторону стика (`set_person_flip`).
    Раньше уже работало случайно через legacy D-pad ветку (analogue=0
    + INPUT_MASK_LEFT/RIGHT → flip), но теперь explicit гейт по
    `l2_regime == L2_REGIME_ROTATE` — чище и не сломается если
    input-transformations изменятся.
  - L2 BACKWARD регим + ✕ → сальто назад
    (`set_person_standing_jump_backwards`). `should_person_backflip`
    защищает от опасных мест (крыша склада, отсутствие LOS) — при
    блокировке падает в vertical jump.
  - L2 FORWARD регим + ✕ → не требует спец-обработки. Если перс
    бежит — action_run даёт ACTION_RUNNING_JUMP (отдельный путь,
    прыжок в facing). Если шагает — STANDING_JUMP body попадает в
    snap-and-forward ветку (стик-направление выровнено с facing).
  - Перекаты/backflip активируются **только** при явном L2-регим.
    Бег вперёд + перетащил стик к камере + ✕ → перс в FORWARD-регим,
    прыжок в facing (вперёд), не walk-back и не backflip. То же при
    переходе из бега в шаг через L2 — пока перс был в running, регим
    FORWARD, прыжок в facing.

- **2026-05-24** — ROTATE регим перестал быть полностью sticky
  (`new_game/src/game/input_actions.cpp`):
  - Проблема: в ROTATE региме (стик в pure-side cone, перс крутится в
    место) при переводе стика в forward/back направление регим оставался
    ROTATE до возврата в нейтраль. Перс продолжал крутиться вместо того
    чтобы пойти по стику.
  - Решение: ROTATE теперь sticky только пока стик в pure-side cone.
    При выходе стика из cone → re-classify в **FORWARD** (camera-relative
    хождение). Намеренно всегда FORWARD, даже при стике назад — потому
    что переход «из кручения в любую сторону через диагональ» это
    «передумал и пошёл», а не «решил пятиться». BACKWARD из ROTATE не
    переключается — для входа в walk-back игроку нужно вернуть стик в
    нейтраль и пушнуть назад заново.
  - FORWARD и BACKWARD остаются полностью sticky (без переклассификации
    до возврата в нейтраль).

- **2026-05-24** — L2-регим классификация учитывает текущее движение
  (`new_game/src/game/input_actions.cpp`):
  - Проблема: если перс бежал не вперёд по миру (в бок или к камере) и
    игрок нажимал L2 во время бега, регим классифицировался по camera-
    relative стику — стик в бок давал ROTATE (перс резко переходил в
    кручение на месте), стик к камере давал BACKWARD (перс резко
    переходил в walk-back). Не плавно.
  - Решение: при классификации регима проверяем не только стик, но и
    состояние перса. Если уже в `STATE_MOVEING + SUB_STATE_RUNNING/WALKING`
    при моменте L2-engage → FORWARD регим всегда. ROTATE/BACKWARD —
    только при старте из не-двигающихся состояний (IDLE и т.п.).
  - Реализация: file-scope `g_player_thing_is_running_or_walking`
    обновляется в `apply_button_input` (там есть p_person), читается в
    `get_hardware_input` при классификации. 1 тик лаг (классификация
    использует состояние с предыдущего тика), но это и хорошо — флаг
    отражает «было ли движение прямо перед L2-нажатием».
  - WALKING_BACKWARDS специально не включён — игрок только что вышедший
    из walk-back и сразу пушащий back+L2 должен пере-войти в BACKWARD
    по стику, не быть форсирован в FORWARD.

- **2026-05-24** — Полная скорость running независимо от отклонения стика
  (`new_game/src/game/input_actions.cpp`):
  - Прошлая правка убрала переход running → walking при слабом стике, но
    скорость running всё ещё скейлилась со стика
    (`Velocity = (velocity * 60) >> 8`) — частичный стик давал медленный
    бег, что вместе с убранным шагом дезориентировало (анимация бега, но
    темп низкий).
  - Фикс: в RUNNING case `process_analogue_movement` для Darcy (не ROPER)
    Velocity теперь константа 42 (= max from old formula at full diagonal
    stick) при любом отклонении за deadzone. Любой стик past deadzone →
    full-speed running. ROPER оставлен с прежним скейлингом.
  - L2 slow walk не тронут — там скорость по-прежнему скейлится со стика
    в WALKING case (slow walk это и есть переменная медленная ходьба).

- **2026-05-24** — Убрана медленная хотьба по слабому отклонению стика
  (`new_game/src/game/input_actions.cpp`, `process_analogue_movement`):
  - До: переход RUNNING → WALKING в `SUB_STATE_RUNNING` срабатывал на
    `(Velocity < 20 || m_bForceWalk)`. Без L2 слабо отклонённый стик давал
    низкую velocity → перс автоматически переходил в шаг.
  - После: оба перехода (running↔walking) гейтятся **только** на
    `m_bForceWalk`. Без L2 перс остаётся в running на любом отклонении
    стика. Velocity по-прежнему скейлится со стика — медленный стик даёт
    медленный бег (но никогда не шаг). С L2 — переход в walking
    мгновенный, при отпускании L2 — мгновенный выход из walking
    (раньше требовался Velocity > 24, при слабом стике мог застрять в
    walking).
  - Шаг теперь — исключительно L2-режим.

- **2026-05-24** — Post-climb jump suppression
  (`new_game/src/game/input_actions.cpp` + `new_game/src/things/characters/person.cpp`):
  - После завершения climb-up анимации (pull_up) при удерживаемом ✕ перс
    прыгал в неправильную сторону: анимация заканчивалась, тут же
    срабатывал `ACTION_STANDING_JUMP`, перс был лицом в сторону climb,
    стик ещё не успел повернуть facing — directional jump уходил вперёд
    вместо стика. Snap-логика помогала только при сильно отклонённом
    стике (digital flag set), при слабом отклонении не срабатывала.
  - Решение: глобальный счётчик `g_post_climb_jump_block_ticks`
    устанавливается в `person.cpp` в `fn_person_moveing` →
    `case SUB_STATE_PULL_UP` сразу после `set_person_idle` (когда
    pull_up завершён) на 8 тиков. В `apply_button_input` (вверху)
    чистится `INPUT_MASK_JUMP` пока счётчик > 0, потом декремент.
  - Эффект: спам ✕ во время climb → JUMP блокируется ~400ms (8 тиков
    × 50ms на 20 Hz физике). За это время `process_analogue_movement`
    перевёл перса в STATE_MOVEING, и `player_turn_left_right_analogue`
    повернул facing к стику. Когда счётчик доходит до 0, JUMP проходит
    → action_run выбирает `ACTION_RUNNING_JUMP` → `set_person_running_jump`
    прыгает в facing-направление, которое теперь совпадает со стиком.
  - Для игрока: ✕ удерживался — прыжок «срабатывает чуть позже» в
    правильную сторону. Не воспринимается как потеря инпута, т.к.
    задержка < 0.5s.
  - Edge-cases:
    - Если игрок отпускает ✕ во время блока — JUMP не сработает вообще
      (нет sticky catch-up для этого случая, что нормально — пользователь
      просто отжал кнопку).
    - Если игрок не зажимает стик после climb — перс остаётся в IDLE,
      `ACTION_STANDING_JUMP` (которое фолбэк через `set_person_standing_jump`
      = vertical) сработает после разблокировки. Нормально.

## TODO к концу всей переработки управления

- [x] Добавить запись в `GAMEPLAY_CHANGES.md` единым блоком когда полный
  цикл управления приземлится (баг-фикс + L2 + клавиатура + опции).
  **Сделано** — секция «## Управление» в `GAMEPLAY_CHANGES.md` покрывает
  баг-фиксы, L2 (перекаты/медленный шаг), L1 (зум/ход назад), клавиатуру,
  смену оружия, UI-cancel (Circle), плюс пункт меню «Mechanics» и убранную
  настройку управления. Промежуточные коммиты туда не идут — там
  кумулятивный «vs original» взгляд.
