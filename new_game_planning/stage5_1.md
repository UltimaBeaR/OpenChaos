# Этап 5.1 — Поддержка геймпадов (Xbox + DualSense)

**Статус: ✅ ЗАВЕРШЁН**

**Что сделано:**
- SDL3 заменил SDL2 (аудио + геймпады), DirectInput удалён
- Gamepad абстракция: `engine/input/gamepad.h/cpp` → SDL3 (Xbox/generic) + Dualsense-Multiplatform (DualSense)
- Маппинг кнопок PS1 Config 0, аналоговый стик с плавной скоростью
- Меню: навигация стиком, Cross/A=confirm, Triangle/Y=back, Start=pause, гистерезис, repeat
- Вибрация PS1-style с затуханием (оба контроллера)
- Hotplug + автодетект устройства ввода (клавиатура ↔ Xbox ↔ DualSense)
- DualSense: LED lightbar (здоровье, сирена, дефолт), adaptive triggers (пистолет, газ/тормоз)
- L2/R2 репрофилированы: газ/тормоз в машине, стрельба (альтернатива Cross)

**Незавершённые доработки** → [Этап 13, геймпад](stage13_gamepad.md) + [DualSense](stage13_dualsense.md)

Техническая документация по API библиотеки → [stage5_1_dualsense.md](stage5_1_dualsense.md)
Девлог → `new_game_devlog/stage5_1_log.md`
