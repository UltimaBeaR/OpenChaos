# DualSense доработки (часть Этапа 13)

Всё что касается DualSense-специфичных фич. Базовая поддержка сделана на Этапе 5.1 (LED, adaptive triggers для пистолета, вибрация). Здесь — что осталось.

Техническая документация по API библиотеки → [stage5_1_dualsense.md](stage5_1_dualsense.md)
Общие доработки геймпада (generic + shared) → [stage13_gamepad.md](stage13_gamepad.md)

---

## Синхронизация adaptive trigger с выстрелом
Щелчок триггера не совпадает с моментом выстрела — выстрел происходит раньше. Нужно синхронизировать. См. [known_issues](known_issues_and_bugs.md) — "DualSense adaptive trigger: щелчок не совпадает с выстрелом".

## Weapon-specific feedback (дробаш, автомат)
Сейчас adaptive triggers + вибрация реализованы только для пистолета. Для дробаша и автомата — ничего.

**Дробаш (shotgun):**
- Adaptive trigger R2: тяжёлый клик (rigid resistance → резкий провал при выстреле)
- Вибрация: один сильный короткий удар (оба мотора на максимум, быстрое затухание)
- Ощущение: мощный одиночный выстрел с сильной отдачей

**Автомат (assault rifle):**
- Adaptive trigger R2: вибрация курка при стрельбе (`TriggerEffectType::Vibration` с frequency + amplitude)
- Вибрация: серия коротких импульсов синхронно с очередью
- Возможно audio-to-haptic для натурального ощущения

**Референс:** современные шутеры на PS5 (Returnal, Ratchet & Clank, COD MW2) — у каждого оружия свой профиль триггера и haptics.

## Тачпад (B5)
- API: `EnableTouch()`, читать `TouchPosition`/`TouchRelative`/`TouchFingerCount`
- Варианты: камера, карта, быстрый доступ к инвентарю
- Требует дизайн-решения
- См. [known_issues](known_issues_and_bugs.md) — "B5 — Тачпад".

## Гироскоп (B6)
- API: `EnableMotionSensor()`, читать `Gyroscope`/`Accelerometer`
- Применение: точное прицеливание в first-person mode
- Требует калибровку и UI
- См. [known_issues](known_issues_and_bugs.md) — "B6 — Гироскоп".

## Audio-to-haptic (B7)
- API: `AudioHapticUpdate()` через miniaudio (уже включён в библиотеку)
- Конвертация игровых звуков в тактильную обратную связь
- Приоритетные эффекты: стрельба (отдача), взрывы, удары, двигатель машины (ощущение оборотов), шаги по разным поверхностям
- Требует: поправить CMake path для miniaudio, интегрировать с MFX звуковой системой
- Самая сложная фича
- См. [known_issues](known_issues_and_bugs.md) — "B7 — Audio-to-haptic".

## Вибрация в видеовставках
- `MDEC_vibra[]` — frame-synchronized vibration для intro/endgame
- Требует интеграцию с видеоплеером
- См. [known_issues](known_issues_and_bugs.md) — "Вибрация в видеовставках".

## Вибрация в меню
- Тест-вибрация при включении опции в настройках
- Опция пока не в UI
- См. [known_issues](known_issues_and_bugs.md) — "Вибрация в меню".
