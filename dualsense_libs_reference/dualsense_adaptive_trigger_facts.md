# DualSense adaptive trigger — hard facts

Только проверенные/измеренные факты. Без предположений, без описаний кода,
без истории. Каждый факт — однострочный, с пометкой откуда (`measured`,
`code`, `reverse-engineered`, `community`).

Ссылки на детали:
- Реализация и история отладки — [`weapon_haptic_and_adaptive_trigger.md`](../new_game_devlog/weapon_haptic_and_adaptive_trigger.md)
- Тестовый инструмент — [`new_game/src/engine/input/weapon_feel_test.cpp`](../new_game/src/engine/input/weapon_feel_test.cpp) (живёт в ветке `dualsence_test`, не в main)
- Полный аудит реализации эффектов в либе vs. Nielk1 спека — [`dualsense_lib_pr_notes.md`](dualsense_lib_pr_notes.md) раздел 6.6
- Reverse-engineered packing всех эффектов — [Nielk1 gist (HTML)](https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db) / [raw](https://gist.githubusercontent.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db/raw)
- Независимые подтверждения спеки:
  - [DualSenseX/DSX UDP API example](https://github.com/Paliverse/DualSenseX) — коммерческий продукт, ranges параметров для Weapon/Bow/Galloping идентичны Nielk1
  - [Ohjurot/DualSense-Windows (DS5W)](https://github.com/Ohjurot/DualSense-Windows) — open-source C++ library, читает feedback offsets 0x29/0x2A ровно как мы патчим

## Hardware Weapon25 эффект

- **Click rate limit ≈ 270-300 мс** (measured 2026-04-15).
  Если жать R2 чаще чем раз в ~270-300 мс, hardware Weapon25 моторчик
  не успевает перевзвестись. Клики начинают пропадать. На интервалах
  ниже **~200 мс** клики **не срабатывают вообще**, и сопротивление в
  триггере тоже исчезает — мотор просто не успевает реагировать.
  Источник: `weapon_feel_test` observation mode, тапы в разном темпе с
  визуальным контролем `last gap`.

- **Эффект автоматически "рассинхронизируется" при быстрой стрельбе даже
  без команд от PC** (measured 2026-04-15).
  Один раз отправили `mode=AIM_GUN`, больше ничего не трогаем. При
  быстром тапе клики всё равно начинают пропадать — это **внутренняя
  hardware проблема**, не наш HID race. Это ключевой факт: проблема I1
  (rapid-tap missing clicks) физически не решаема на стороне софта в
  рамках Weapon25.

- **Параметры эффекта Weapon25 — реальная спека** (reverse-engineered,
  source: [Nielk1 gist](https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db)
  + [raw текст](https://gist.githubusercontent.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db/raw)).
  - `startPosition`: **2-7** — точка начала зоны эффекта в 16-битном
    bitmap
  - `endPosition`: `startPosition+1`..**8** — точка окончания зоны
  - `strength`: **0-8** — сила сопротивления/щелчка (в wire идёт как
    `strength-1`)
  Wire format:
  ```
  byte[0] = 0x25
  byte[1] = low  8 bits of ((1 << startPosition) | (1 << endPosition))
  byte[2] = high 8 bits of ((1 << startPosition) | (1 << endPosition))
  byte[3] = strength - 1
  ```

- **`dualsense-multiplatform` оригинальная Weapon25 packing сломана**
  (measured 2026-04-16 через A/B тест).
  Функция `Weapon25()` в `GamepadTrigger.h` пакует параметры так:
  ```
  byte[1] = (StartZone << 4) | (Amplitude & 0x0F)
  byte[2] = Behavior
  byte[3] = Trigger & 0x0F
  ```
  Это **не соответствует** реальной Sony спеке (см. выше). Hardware
  получает паразитный bitmap сконкатенированный из 4-битных полей
  `StartZone` и `Amplitude` — и судя по всему либо игнорирует такой
  вход, либо выдаёт близкое к default-ному поведение независимо от
  параметров. Экспериментально подтверждено: одни и те же тестовые
  значения `(2, 8, 8, 0)` с оригинальной упаковкой дают слабый
  глубокий клик (как "дефолт"), а после фикса упаковки — сильное
  сопротивление по всему ходу курка с максимальной силой. Разница
  воспроизводимая и явная.

- **Есть отдельный эффект `Limited_Weapon` (0x12)** (community RE,
  не проверено эмпирически).
  Другой mode code (`0x12`), не 0x25. Использует raw direct packing
  (без bitmap): `byte[1]=startPosition`, `byte[2]=endPosition`,
  `byte[3]=strength`. Range checks жёстче: `startPosition >= 16`,
  `endPosition` в пределах `[start, start+100]`. В либе
  `dualsense-multiplatform` **вообще не реализован** — нет
  `SetLimited_Weapon12()` и mode handling'а. Пока не использовали
  в OpenChaos, не исследовали, не тестировали. Упомянут для
  полноты картины.

- **Аудит ВСЕХ trigger effect функций в либе против Nielk1 спеки**
  (measured 2026-04-16). Большинство функций с bitmap-packing
  (Bow22, Weapon25, MachineGun26, Machine27) **сломаны аналогично
  Weapon25** — не строят правильный bitmap, пишут raw значения или
  magic-числа. Только `Off`, `Resistance (0x01)`, `GameCube (0x02)`
  реализованы корректно (это raw-packing эффекты). Галопинг частично
  работает (bitmap ок, кодировка feet сломана). Полный аудит —
  [`dualsense_lib_pr_notes.md`](dualsense_lib_pr_notes.md) раздел 6.6
  и [`dualsense_protocol_reference.md`](dualsense_protocol_reference.md)
  раздел 4.1. **Практическое следствие**: нельзя доверять ни одному
  сложному эффекту либы без сверки packing'а против спеки.

- **Trigger feedback status байт существует в input report и работает**
  (measured 2026-04-15 после локального патча либы).
  Layout байта: bits 0-3 = state nybble, bit 4 = effect-active flag.
  Offsets в нормализованном буфере: R2 = `41`, L2 = `42`. Источник
  оффсетов: nondebug/dualsense reverse-engineering. Поведение
  bit 4 (act) проверено эмпирически на DualSense через Bluetooth
  на Windows 11.

  - **bit 4 (act) — работает как ожидалось.** `act=1` ровно в зоне
    сопротивления курка (мотор активен), `act=0` всё остальное время.
    Переход `act: 1 → 0` происходит **в момент физического щелчка**
    (или когда игрок снимает палец с курка не доведя до щелчка).
    Это **прямой сигнал из железа**, можно строить fire-detection
    без угадайки по позиции r2.

  - **Чистая логика детекта щелчка**: rising edge `act 1→0`
    одновременно с r2 idущим вверх (растущим) = щелчок физически
    случился. Если r2 в этот же момент падает — это release, не
    щелчок.

- **state nybble (bits 0-3) ведёт себя НЕ так как описано в Nielk1
  gist** (measured 2026-04-15/16).
  Gist утверждает что значения это `0` = до зоны, `1` = внутри,
  `2` = за зоной. **На реальном железе значения 3-9** в зависимости
  от зоны, а не 0-2.

  После применения patched Weapon25 packing (correct bitmap) видно
  что nybble **меняется пропорционально позиции курка внутри зоны
  эффекта**: при зоне `[2,8]` значения идут ~3 в idle, растут до 8
  при продавливании, 9 после клика. При зоне `[4,6]` значения 5-6
  преимущественно, плюс 9 после клика. То есть это скорее
  position-in-zone индикатор, чем простая 3-state логика из gist'а.

  Наблюдается jitter в idle — значения случайно прыгают между двумя
  соседними, например 2 и 3. Это не меняет общую семантику но
  означает что использовать nybble как точный индикатор позиции
  нельзя без сглаживания. Эмпирические наблюдения (DualSense BT, Windows):

  | Состояние | Значение nybble |
  |-----------|-----------------|
  | Idle, курок отпущен полностью | в основном `5`, иногда `4` |
  | Свободный ход курка до зоны | `4`/`5`, нажатие здесь не влияет |
  | В зоне сопротивления (act=1) | в основном `5`, изредка `4` |
  | Глубже давишь, перед самым щелчком | `5` → `6` |
  | Совсем глубоко перед срывом | иногда мелькает `7` |
  | После щелчка, курок продавлен | всегда `9` |
  | Подъём курка из продавленного | `9` → резко `7` (с тактильным микро-импульсом мотора) → `6` → `5` |
  | `8` | не наблюдается ни разу |

  Из-за того что nybble значения не соответствуют документации, для
  детекта щелчка **используем bit 4 (act flag)**, а не nybble. Nybble
  можно изучить дальше как индикатор более тонкого состояния мотора
  (например, переход в `9` = "только что выстрелил, ещё не
  перевзвёлся"), но пока это нам не нужно.

## Поведение при ревизиях контроллера

- **Стандартный DualSense ведёт себя одинаково на всех ревизиях**
  (community knowledge).
  Sony стандартизировала эффекты адаптивных курков как часть PS5
  платформенного API. Все ревизии CFI-ZCT1/CFI-ZCT1W и т.д. имеют одну
  прошивку курков. Эмпирические замеры (точка щелчка, click rate
  limit) — универсальны, не зависят от конкретного контроллера.
  DualSense Edge — премиум-версия с механически другими курками, но
  HID-эффекты те же.

## HID transport

- **Bluetooth round-trip latency ≈ 7-30 мс** для DualSense (community).
  Не измеряли на нашем конкретном контроллере, общеизвестная
  характеристика BT HID.

- **R2 семплируется раз в game tick (~30 Hz, ~33 мс)** (code).
  Между тиками промежуточные значения не видны. Физическое движение
  курка быстрее одного тика для нас "мгновенное" (0 → 254 за один poll).

## Что **не знаем** и хотелось бы измерить

- Точное значение `r2` в момент физического щелчка (точка start_zone).
  Теперь когда работает `act` bit — можно автоматически: записываем r2
  в момент перехода `act: 1→0` с растущим r2.
- Наблюдается ли motor engagement delay после `mode=AIM_GUN`. План:
  hardware delay test (см. `weapon_haptic_and_adaptive_trigger.md`,
  пока не сделан, возможно перекрывается с rate-limit фактом выше).
- Реальное соответствие наших параметров `Amplitude`/`Behavior`/`Trigger`
  спеке `endPosition`/`strength`. Можно проверить варьируя их по
  одному в test-моде и наблюдая что меняется на ощупь.
- **Cross-platform / cross-transport валидация feedback байт layout —
  ЗАВЕРШЕНА** (2026-04-15). Все 6 комбинаций (3 OS × 2 transports)
  показали идентичное поведение:
  - ✅ DualSense BT + Windows 11 Pro
  - ✅ DualSense USB + Windows 11 Pro
  - ✅ DualSense BT + macOS
  - ✅ DualSense USB + macOS
  - ✅ DualSense BT + Steam Deck (Linux)
  - ✅ DualSense USB + Steam Deck (Linux)
  Оффсеты `41/42` и layout `bit 4 = act, bits 0-3 = state` —
  универсальные. Патч готов к PR в апстрим.
