# PR prep — Trigger feedback status reading for `Dualsense-Multiplatform`

Этот файл содержит **всю информацию необходимую чтобы подготовить
pull request** в апстрим библиотеки
[`rafaelvaloto/Dualsense-Multiplatform`](https://github.com/rafaelvaloto/Dualsense-Multiplatform).

Цель PR: добавить чтение **adaptive trigger feedback status** байт из
DualSense input report и экспонировать их через `FInputContext`.
Сейчас либа эти байты вообще игнорирует, хотя контроллер их
постоянно отправляет. Эти байты позволяют **точно** определять с
PC-стороны когда курок физически щёлкнул в активном trigger-эффекте,
без эмпирической калибровки по позиции триггера.

Документ написан так чтобы агент с чистым контекстом (или человек) мог
взять его и сразу подготовить PR без необходимости разбираться в нашей
кодовой базе или поднимать предысторию.

---

## 1. Контекст и мотивация

### 1.1 Что такое DualSense adaptive trigger feedback status

DualSense поддерживает несколько режимов **adaptive trigger** эффектов
(Weapon, Feedback, Vibration, и т.п.) которые программируются через
output report. Каждый эффект задаёт зону на ходе курка где мотор
активен (создаёт сопротивление, щелчок, вибрацию).

Контроллер **в каждом input report** сообщает 1 байт на курок с
**текущим состоянием курка относительно активного эффекта**. Это
позволяет PC-стороне знать например что сейчас курок прошёл через
точку щелчка Weapon-эффекта — без необходимости угадывать момент по
аналоговой позиции триггера (что неточно из-за hardware variability и
poll rate).

### 1.2 Почему это важно

Без feedback status байт игре приходится **гадать** когда щёлкнул
курок: либо ставить порог по аналоговой позиции (неточно), либо
полагаться на user input timing (тоже неточно). Никакая эмпирика
не сравнима с прямым сигналом из железа, который контроллер и так
шлёт постоянно — нужно только прочитать.

### 1.3 Текущее состояние библиотеки

Файл `Source/Public/GImplementations/Utils/GamepadInput.h`,
функция `FGamepadInput::DualSenseRaw(const unsigned char* HIDInput,
FInputContext* Input)` — основной парсер input report для DualSense.
Читает байты 0x00..0x09 (sticks, triggers, кнопки) и 0x34/0x35
(battery, charging). Байты 41/42 (где живут feedback status для
R2/L2) **не читаются вообще**.

Структура `FInputContext`
(`Source/Public/GCore/Types/Structs/Context/InputContext.h`)
не содержит полей для feedback status — нужно добавить.

---

## 2. Layout байта (reverse-engineered)

### 2.1 Источники

- **Основной**: [`nondebug/dualsense`](https://github.com/nondebug/dualsense) — GitHub repo с полным реверс-инжинирингом DualSense HID protocol. В коде `dualsense-explorer.html` есть таблица оффсетов input report, включая `r2feedback` / `l2feedback`.
- **Дополнительно**: [Game Controller Collective Wiki — DualSense Data Structures](https://controllers.fandom.com/wiki/Sony_DualSense/Data_Structures) — community-документация, упоминает `TriggerLeftStatus` / `TriggerRightStatus` поля.
- **Изначальный pointer**: [Nielk1 trigger effect factories gist](https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db) — упоминает status nybble в контексте Weapon (0x25) эффекта (но описание там устаревшее, см. п. 3.2).

### 2.2 Точные оффсеты

**После strip Report ID и BT extra header** (то есть в нормализованном
буфере, как раз тот вид что получает `DualSenseRaw` после
`&Context->Buffer[Padding]`):

| Поле | Offset (decimal) | Offset (hex) |
|------|------------------|--------------|
| R2 trigger feedback byte | **41** | 0x29 |
| L2 trigger feedback byte | **42** | 0x2A |

Сама библиотека уже корректно обрабатывает USB vs Bluetooth разницу
через `Padding` параметр в `FDualSenseLibrary::UpdateInput`:
```cpp
const size_t Padding =
    Context->ConnectionType == EDSDeviceConnection::Bluetooth ? 2 : 1;
DualSenseRaw(&Context->Buffer[Padding], InputToFill);
```
То есть внутри `DualSenseRaw` мы можем использовать единые offsets
для обоих transports — `41` и `42`.

### 2.3 Layout одного байта

```
Bit:   7 6 5 4 3 2 1 0
       ─ ─ ─ ─ ───────
       │ │ │ │    └── state nybble (bits 0-3)
       │ │ │ └────── effect-active flag (bit 4)
       │ │ └──────── reserved (?)
       │ └────────── reserved (?)
       └──────────── reserved (?)
```

- **bit 4 (effect-active flag)** — `1` если курок находится в зоне
  активного эффекта (мотор работает физически), `0` если нет.
- **bits 0-3 (state nybble)** — внутреннее состояние мотора, см. п. 3.2
  ниже про эмпирическое поведение.
- bits 5-7 — назначение неизвестно, не наблюдали изменений.

---

## 3. Эмпирические измерения (OpenChaos)

После применения локального патча мы провели тестирование на одном
устройстве (см. п. 5 про то что ещё надо проверить).

### 3.1 Bit 4 (effect-active flag) — работает как ожидалось

**Тестируемая конфигурация**: DualSense (стандартный, не Edge),
Bluetooth transport, Windows 11 Pro, эффект `Weapon (0x25)` с
параметрами `start_zone=7, amplitude=2`.

**Результаты**:

- `act=1` ровно в зоне сопротивления курка — там где мотор физически
  активен и игрок чувствует усилие.
- `act=0` когда курок:
  - полностью отпущен (свободный ход до зоны),
  - находится перед зоной сопротивления,
  - продавлен полностью за зону (после щелчка).
- Переход `act: 1 → 0` происходит **в момент физического щелчка**
  (если игрок продавил через всю зону) или когда игрок снимает палец
  с курка (если не продавил).
- Переход `act: 0 → 1` происходит когда курок входит в зону
  сопротивления.

**Применение**: rising edge `act 1→0` синхронизированный с растущим
аналоговым значением курка (`trigger_right`) = **щелчок физически
произошёл**. Это идеальный сигнал для game-side fire detection.

### 3.2 State nybble (bits 0-3) — поведение НЕ соответствует Nielk1 gist

Изначально community-документация (Nielk1 gist) описывала state nybble
как 3-state индикатор: `0` = до зоны, `1` = внутри, `2` = после.

**На реальном железе значения 4-9**, а не 0-2. Это явно более сложный
state machine моторчика. Полная таблица наблюдений:

| Состояние курка | Значение nybble |
|-----------------|-----------------|
| Idle, курок отпущен полностью | в основном `5`, иногда `4` |
| Свободный ход курка до зоны сопротивления | `4`/`5`, нажатие здесь не влияет на значение |
| Внутри зоны сопротивления (act=1) | в основном `5`, изредка `4` |
| Глубже давишь, перед самым щелчком | `5` → `6` |
| Совсем глубоко перед срывом | иногда мелькает `7` |
| После щелчка, курок продавлен полностью | всегда `9` |
| Подъём курка из продавленного | `9` → резко `7` (с тактильным микро-импульсом) → `6` → `5` |
| `8` | не наблюдается ни разу |

**Гипотеза о смысле**: похоже это внутренний state machine ARM/disarm
моторчика, не простая 3-state логика. Например, переход в `9` может
означать "только что сработал щелчок, мотор разряжен, ждёт rearm".
Точная семантика **не задокументирована нигде**, требует дальнейшего
RE.

**Для PR**: мы экспозим nybble как есть (`uint8_t`), с пометкой в
комментарии что значения hardware-specific и community-документация
устарела. Не пытаемся интерпретировать.

---

## 4. Предлагаемый патч

Все правки **только в двух файлах** библиотеки:

### 4.1 `Source/Public/GCore/Types/Structs/Context/InputContext.h`

В struct `FInputContext` добавить 4 новых поля:

```cpp
// Adaptive trigger feedback status. Reported by the controller for
// active effects of type Off (0x05), Feedback (0x21), Weapon (0x25),
// and Vibration/MachineGun (0x26). For other effect modes the values
// are not specified.
//
// EffectActive: true while the trigger is physically inside the
//               active effect's resistance zone (i.e. the motor is
//               engaged). Rising edge from true to false correlates
//               with the click moment for Weapon effects (when the
//               analog trigger value is increasing) or with release
//               (when decreasing).
//
// State: lower-nybble state value reported by the controller. Internal
//        motor state machine — observed values are 4..9 on standard
//        DualSense BT firmware, with semantics documented empirically
//        in OpenChaos devlog (see references below). Earlier
//        community documentation describing this as a 3-state 0/1/2
//        nybble appears to be outdated.
std::uint8_t LeftTriggerFeedbackState  = 0;
std::uint8_t RightTriggerFeedbackState = 0;
bool         bLeftTriggerEffectActive  = false;
bool         bRightTriggerEffectActive = false;
```

Поля размещаем рядом с другими trigger-related полями
(`bLeftTriggerThreshold` / `bRightTriggerThreshold`).

### 4.2 `Source/Public/GImplementations/Utils/GamepadInput.h`

В функции `FGamepadInput::DualSenseRaw(const unsigned char* HIDInput,
FInputContext* Input)`, перед заключительной серией присвоений
`Input->...`, добавить:

```cpp
// Adaptive trigger feedback status. The library normalizes USB vs
// Bluetooth report layouts upstream by stripping the Report ID byte
// and the BT-only extra header, so the same byte offsets apply for
// both transports here.
//
// Source: nondebug/dualsense reverse-engineering
// (https://github.com/nondebug/dualsense).
//
// Layout per byte:
//   bit 4    = effect-active flag (1 = trigger inside active effect
//              zone, motor engaged; 0 = outside zone or no effect).
//   bits 0-3 = state nybble — internal motor state machine value.
//              Empirically 4..9 on standard DualSense, semantics
//              are not fully documented.
//   bits 5-7 = unknown / reserved.
const unsigned char R2Fb = HIDInput[41];
const unsigned char L2Fb = HIDInput[42];
Input->RightTriggerFeedbackState = R2Fb & 0x0F;
Input->LeftTriggerFeedbackState  = L2Fb & 0x0F;
Input->bRightTriggerEffectActive = (R2Fb & 0x10) != 0;
Input->bLeftTriggerEffectActive  = (L2Fb & 0x10) != 0;
```

### 4.3 Что НЕ менять

- ❌ Не трогать `DualShockRaw` — DualShock 4 имеет другой layout, не
  верифицировали.
- ❌ Не трогать output report / trigger effect setters — патч только
  про чтение feedback.
- ❌ Не вводить новые public API методы — поля в `FInputContext`
  достаточно, пользователи либы и так читают эту структуру.

---

## 5. Кросс-платформенная валидация перед PR

**ВАЖНО**: эмпирические измерения сделаны только на **одной**
конфигурации:

- ✅ DualSense (стандартный, не Edge)
- ✅ Bluetooth transport
- ✅ Windows 11 Pro
- ✅ Один конкретный физический контроллер

Перед отправкой PR в апстрим **критически важно** проверить что
оффсеты `41/42` и layout `bit 4 = act, bits 0-3 = state` работают
так же на:

| Конфигурация | Статус |
|--------------|--------|
| DualSense BT + Windows | ✅ проверено (act работает корректно, nybble значения 4-9) |
| DualSense **USB** + Windows | ❌ **не проверено** |
| DualSense BT + **macOS** | ❌ **не проверено** |
| DualSense USB + macOS | ❌ **не проверено** |
| DualSense USB + **Steam Deck** (Linux) | ❌ **не проверено** |
| DualSense BT + Steam Deck | ❌ **не проверено** |

Для каждой непроверенной конфигурации использовать тестовый инструмент
[`weapon_feel_test.cpp`](../new_game/src/engine/input/weapon_feel_test.cpp)
в OpenChaos: запустить игру, взять пистолет в руки, нажать `\` для
входа в режим теста, наблюдать значения `fb=N act=0/1` в правом
верхнем углу экрана и подтвердить что:

1. `act` корректно идёт в `1` в зоне сопротивления курка.
2. `act` идёт в `0` в момент щелчка (или при release).
3. Значения `fb` (state nybble) лежат в наблюдаемом диапазоне 4-9.
4. Никаких странных артефактов типа фиксированных нулей или мусора.

Если на каком-то transport/OS оффсеты другие — это нужно описать в PR
и адаптировать код (возможно через дополнительный padding-aware
индекс или конструкцию вида `if (Bluetooth) ... else ...`).

---

## 6. Связанные ссылки и материалы

### 6.1 Reverse engineering источники

- [`nondebug/dualsense`](https://github.com/nondebug/dualsense) — основная карта HID layout DualSense input report. Здесь взяты offsets 41/42.
- [Game Controller Collective Wiki — DualSense Data Structures](https://controllers.fandom.com/wiki/Sony_DualSense/Data_Structures) — community wiki с описанием полей `TriggerRightStatus`/`TriggerLeftStatus`.
- [Nielk1 trigger effect factories gist](https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db) — описание trigger effect modes и (устаревшее) описание status nybble как 0/1/2.
- [SensePost DualSense Reverse Engineering blog](https://sensepost.com/blog/2020/dualsense-reverse-engineering/) — оригинальная RE статья.
- [`dogtopus/894da226d73afb3bdd195df41b3a26aa`](https://gist.github.com/dogtopus/894da226d73afb3bdd195df41b3a26aa) — DualSense HID descriptor.

### 6.2 Внутренние документы OpenChaos

- [`dualsense_protocol_reference.md`](dualsense_protocol_reference.md) — общая справка по DualSense protocol для проекта.
- [`dualsense_adaptive_trigger_facts.md`](dualsense_adaptive_trigger_facts.md) — наши собственные эмпирические факты про hardware (rate limit, click point, и т.п.), включая полную таблицу feedback nybble значений.
- [`weapon_haptic_and_adaptive_trigger.md`](weapon_haptic_and_adaptive_trigger.md) — история отладки adaptive trigger в OpenChaos, объясняет зачем вообще понадобилось feedback status (boundary detection issue).
- [`new_game/src/engine/input/weapon_feel_test.cpp`](../new_game/src/engine/input/weapon_feel_test.cpp) — наш тестовый инструмент для проверки patched behavior. Используется для cross-platform валидации.

### 6.3 Контекст в либе

- [`Source/Public/GCore/Types/Structs/Context/InputContext.h`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Public/GCore/Types/Structs/Context/InputContext.h) — где добавляем поля.
- [`Source/Public/GImplementations/Utils/GamepadInput.h`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Public/GImplementations/Utils/GamepadInput.h) — где добавляем чтение байт.
- [`Source/Private/GImplementations/Libraries/DualSense/DualSenseLibrary.cpp`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Private/GImplementations/Libraries/DualSense/DualSenseLibrary.cpp) — `FDualSenseLibrary::UpdateInput`, показывает USB vs BT padding handling.

---

## 7. Чеклист для подготовки PR

- [ ] Завершить cross-platform валидацию (п. 5)
- [ ] Зафоркать [`rafaelvaloto/Dualsense-Multiplatform`](https://github.com/rafaelvaloto/Dualsense-Multiplatform) на чистый репо (НЕ из вендоренной копии в OpenChaos)
- [ ] Применить правки из п. 4.1 и п. 4.2 на актуальный main (последний известный коммит — `b8a76fff41cbd5909f7f71b57b6f3cea88b52bfa` от 2026-02-16, либа без релизов)
- [ ] Проверить что код компилируется чисто (нет warnings) на основной target платформе либы
- [ ] Написать PR description со ссылками на источники реверс-инжиниринга и краткое описание зачем эти поля нужны
- [ ] Указать в PR что фича валидирована эмпирически — приложить таблицу из п. 5 с галочками
- [ ] Упомянуть что layout state nybble не соответствует устаревшей community-документации (Nielk1 gist), и что мы оставляем сырое значение для пользователей либы
- [ ] Если автор апстрима попросит — приложить пример использования (rising edge `act 1→0` для click detection)

---

## 8. Что ждёт после merge PR

Когда PR замержат и наша вендоренная копия обновится до новой версии
либы — можно будет:

1. **Удалить локальные правки** из вендоренной копии (они станут
   частью апстрима).
2. **Использовать `act` bit для fire detection** в `weapon_feel.cpp` —
   заменить эмпирическое угадывание момента щелчка по позиции r2 на
   прямой сигнал из железа. Это решит "клик есть, выстрела нет" и
   "выстрел есть, клика нет" коренным образом.
3. **Возможно отдать контрибут на документацию state nybble** —
   например в Nielk1 gist отметить что описание устарело и приложить
   нашу эмпирическую таблицу.
