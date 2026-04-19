# DualSense доработки (часть Этапа 13)

Всё что касается DualSense-специфичных фич. Базовая поддержка сделана на Этапе 5.1 (LED, adaptive triggers для пистолета, вибрация). Здесь — что осталось.

Техническая документация по API библиотеки → [stage5_1_dualsense.md](stage5_1_dualsense.md)
Общие доработки геймпада (generic + shared) → [stage13_gamepad.md](stage13_gamepad.md)

---

## ~~Синхронизация adaptive trigger с выстрелом~~ ✅ Готово
Реализовано через act-bit детект. См. [`known_issues_and_bugs_resolved.md`](known_issues_and_bugs_resolved.md) — "DualSense adaptive trigger: щелчок не совпадает с выстрелом".

## ~~Weapon-specific feedback (дробовик, автомат)~~ ✅ Готово (2026-04-19)

Реализовано для всех трёх основных оружий. Фактическая конфигурация — в [`weapon_feel.cpp`](../new_game/src/engine/input/weapon_feel.cpp):

- **Пистолет:** Weapon25 клик зоны 4-6, strength 5. Envelope на slow-моторе DualSense.
- **Автомат (AK47):** Machine (0x27) двуамплитудный пульс (start=4, end=9, ampA=7, ampB=3, freq=8, period=80) — непрерывный rattle на всём диапазоне. Envelope на slow-моторе DualSense + **на Xbox per-motor routing 75/100** (оба мотора, low чуть слабее для более "layered" ощущения).
- **Дробовик (shotgun):** Weapon25 клик (зоны 3-6, strength 7 — тяжелее пистолетного) как baseline + **post-shot Vibration burst** — после выстрела на триггер включается `ds_trigger_vibration` (position=0, freq=60Hz) с линейно затухающей амплитудой 8→0 за 0.25с. Эмулирует отдачу в курке. Инфраструктура: `TriggerEffectType::Vibration` + поля `post_shot_vibration_*` в профиле + `weapon_feel_tick_trigger_vibration()` API + override в `gamepad_triggers_update`.

Подробности реализации — в архиве решённых багов.

**Референс:** современные шутеры на PS5 (Returnal, Ratchet & Clank, COD MW2) — у каждого оружия свой профиль триггера и haptics.

## Тачпад (B5)
- API: `EnableTouch()`, читать `TouchPosition`/`TouchRelative`/`TouchFingerCount`
- Варианты: камера, карта, быстрый доступ к инвентарю
- Требует дизайн-решения

## Гироскоп (B6)
- API: `EnableMotionSensor()`, читать `Gyroscope`/`Accelerometer`
- Применение: точное прицеливание в first-person mode
- Требует калибровку и UI

## Audio-to-haptic (B7)

Геймплейная идея: конвертация игровых звуков в тактильную обратную
связь на контроллере.

- Приоритетные эффекты: стрельба (отдача), взрывы, удары, двигатель
  машины (ощущение оборотов), шаги по разным поверхностям.
- Для 1.0 идём **упрощённым путём** — envelope реального WAV'а
  модулирует обычные rumble-моторы (низкочастотный + высокочастотный).
  Не требует нового пути в libDualsense.
- **Полноценный audio-haptic** (PCM через HID 0x32 прямо в DSP
  контроллера — как Astro's Playroom делает) — отдельная задача,
  упирается в нереализованный transport в libDualsense. Когда он
  появится, игровую сторону (какие звуки пускать, как их
  нормализовывать) пилим здесь. См. план либы:
  [`own_dualsense_lib_plan.md §13.2`](../new_game_devlog/dualsense_libs_reference/own_dualsense_lib_plan.md).

## Вибрация в видеовставках
- `MDEC_vibra[]` — frame-synchronized vibration для intro/endgame
- Требует интеграцию с видеоплеером

## Вибрация в меню
- Тест-вибрация при включении опции в настройках
- Опция пока не в UI

## libDualsense follow-up (post-1.0 доработки либы)

Весь список отложенных задач по самой libDualsense собран в плане
библиотеки:
[`new_game_devlog/dualsense_libs_reference/own_dualsense_lib_plan.md`](../new_game_devlog/dualsense_libs_reference/own_dualsense_lib_plan.md).

Кратко что там:
- **§12 Convenience layer** — ввести чёткое разделение на raw API
  (internal, байты/HID units — текущее состояние) и convenience API
  (public, проценты / normalized float / physical units / enum-ы).
  Сейчас в либе **пёстрая картина**: часть полей уже convenience
  (sticks/triggers/battery в `DS_InputState`, enum-ы для
  MuteLed/AudioRoute), часть всё ещё raw (rumble 0..255, LED brightness
  0..2 с перевёрнутой шкалой, IMU int16 без применённой калибровки,
  audio volumes 0..255). Таблица inconsistency — в §12.1 плана.
  Заодно переделка in-game тестера на friendly-форматы (сейчас
  показывает сырые байты для диагностики wire-багов — это было
  оправдано на этапе разработки, но мешает нормальному использованию).
  См. §12.5 плана.
- **§13.1 USB Audio Class PCM output** (обычный звук в спикер/jack
  контроллера) — **не делаем, только документация**. Работает на USB
  «сам собой» как обычная звуковая карта ОС, но нам не нужно.
- **§13.2 Audio haptics через HID 0x32** — PCM-вибрация (как Astro's
  Playroom: звук шагов по снегу «чувствуется» ладонями). Единственный
  способ доставить пользовательский аудиоконтент на **BT-контроллер**
  (USB Audio Class по BT недоступен). Требует reverse-engineer
  Sony-undocumented wire format'а. Это то же самое что «Audio-to-haptic
  (B7)» выше в этом же документе — там высокоуровневое описание фичи
  со стороны игры, а в §13.2 плана либы — как именно реализовать
  API внутри библиотеки. Связаны.
