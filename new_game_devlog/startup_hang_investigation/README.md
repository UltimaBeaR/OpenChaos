# Startup-hang investigation

System-wide lag при загрузке миссии / во время геймплея (блокер 1.0). Игра превращает весь desktop в ~1 кадр/сек, audio играет нормально, mouse/клавиатура почти не реагируют. Длится от десятков секунд до минут, редко требует reboot.

## TL;DR

**Root cause:** per-frame `glReadPixels` в трёх gameplay path'ах игры (звёзды ночью, WIBBLE рябь воды, FONT_buffer_draw) → CPU modify → writeback через glTexImage2D + fullscreen quad. Этот sync-read pattern при VSync-blocked `SwapBuffers` triggers WDDM/driver throttling нашего процесса.

**Hardware-specific:** воспроизводится не на всех машинах. У автора на Windows 11 + NVIDIA триггерится надёжно; у другого пользователя с тем же билдом — нет.

**Не OpenGL-specific:** тот же pattern через `IDirectDrawSurface7::Lock` в PieroZ D3D6 сборке — тоже виснет. Root cause на уровне WDDM scheduler / DWM compositor для любого per-frame GPU sync read.

**Текущий статус в main:** `ge_lock_screen` / `ge_unlock_screen` эмитят `glReadPixels` + writeback безусловно, hang триггерится в обычной игре на affected hardware.

**Возможные варианты обхода** (trade-offs) — [workarounds.md](workarounds.md).

## Навигация

| Файл | Что там |
|------|---------|
| [FINDINGS.md](FINDINGS.md) | Что установлено из bisect'а: симптомы, callsite'ы-триггеры, параметры минимального repro, ложные гипотезы. |
| [mechanism.md](mechanism.md) | Что делает код `ge_lock_screen` / `ge_unlock_screen` в main, где вызывается, dead API. |
| [hypothesis.md](hypothesis.md) | Предполагаемые причины почему именно этот pattern триггерит и почему hardware-specific. |
| [workarounds.md](workarounds.md) | Возможные варианты обхода с trade-offs, основанные на понимании причин. |
| [repro_hang_opengl/](repro_hang_opengl/) | Минимальный standalone repro (pure Win32 + WGL, без зависимостей). |

## Что ещё в проекте связано

- **Карточка бага:** [known_issues_and_bugs.md](../../new_game_planning/known_issues_and_bugs.md) секция «Крэши» → "Тормоза при загрузке миссии".
