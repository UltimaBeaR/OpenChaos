# FINDINGS — что удалось установить

Все факты из bisect'а в игре и standalone repro. Основа для fix'а.

## Наблюдаемое поведение (до root cause'а, 2026-04-05..2026-04-21)

- Симптомы: FPS ≈ 1 или полный freeze, вся Windows-система реагирует крайне медленно (~1 событие в 10 секунд), audio обычно играет нормально.
- Частота: ~1 на 5-8 запусков в период ранних проявлений. Невозможно форсировать руками — репро было спонтанным.
- Длительность: от нескольких секунд до минут. Редкие случаи — не проходило без reboot'а даже после закрытия процесса игры и IDE.
- Возможен self-recovery через десятки секунд — минуту (не всегда, зависит от случая).
- Воспроизводится на D3D-сборке PieroZ (pre-наших правок) → оригинальный баг пре-релизного кода MuckyFoot, не внесён нами.
- Не связано с memory corruption: после 6 ASan-фиксов (2026-04-11) hang сохранился.
- Проявляется не только на загрузке миссии: есть случаи на главном меню и на стартовой заставке (Дарси на крыше). Общий знаменатель — тяжёлая инициализация / IO.
- Windows message queue во время фриза настолько отстаёт что OS слипает серии WM_MOUSEMOVE + WM_LBUTTONDOWN в title-bar drag — клик по крестику интерпретируется как перетаскивание окна.
- В task manager **не виден** сильный CPU/GPU/RAM load — это и ввело в заблуждение на ранних этапах (гипотезы про busy-wait в ядре, lock_frame_rate и т.д. — все отброшены после root cause'а).

Hardware-specificity: см. отдельный раздел ниже. Из симптомов это не было очевидно — казалось что баг универсальный.

## Воспроизведение в игре (gameplay)

**Три per-frame call-site `ge_lock_screen` в gameplay flow:**
1. [aeng.cpp:4050](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4050) — ночные звёзды (`SKY_draw_stars`).
2. [aeng.cpp:4951](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4951) — WIBBLE (screen-space рябь воды для луж).
3. [font.cpp:197](../../new_game/src/engine/graphics/text/font.cpp#L197) — `FONT_buffer_draw` (custom CPU-plotted 3D-font).

Остальные call-sites либо dead code (`AENG_lock`/`AENG_unlock`/`AENG_flush`), либо debug-only (screenshot по KB_S, record_video, legacy debug FPS overlay) — не триггерят в обычном gameplay. Подробности в [mechanism.md](mechanism.md).

**Bisect callsite'ов** (поэтапно отключая по одному `ge_lock_screen` callsite в игре и проверяя воспроизводится ли hang на testdrive1a, ночь):

| Конфиг | Поведение |
|--------|-----------|
| все 3 callsite работают (baseline = main) | виснет почти сразу |
| все 3 отключены | не виснет |
| только stars активен | **виснет сразу** — stars достаточен сам |
| только wibble активен | **виснет** — wibble достаточен сам |
| только FONT активен | не тестируется — на testdrive1a `FONT_message_upto=0`, callsite не стреляет в обычном gameplay |

**Stars и wibble — независимые триггеры.** Каждого достаточно. FONT — потенциальный триггер того же класса (одинаковый pattern: readpixels + CPU plot + writeback), на тестовой миссии просто не активен.

## Hardware-specificity

**Воспроизводится не на всех машинах.** У автора на Windows 11 + NVIDIA — надёжно (стартовая миссия testdrive1a, ночь). У второго пользователя с **тем же бинарём** — не воспроизводится. Конкретная связка GPU / driver version / Windows version решает.

**Не OpenGL-specific:** тот же readback pattern через `IDirectDrawSurface7::Lock(DDLOCK_WAIT)` в PieroZ D3D6 сборке — тоже виснет. Root cause — на уровне WDDM scheduler / DWM compositor для любого per-frame sync GPU read.

## Standalone repro

См. [repro_hang_opengl/](repro_hang_opengl/). Pure Win32 + WGL, без зависимостей.

Per-frame pattern: `glClear` → triangle draw → N циклов {`glReadPixels` → Y-flip + scattered CPU writes → `glGenTextures` + `glTexImage2D` → textured fullscreen quad → `glDeleteTextures`} → `SwapBuffers(VSync)`.

**Минимум параметров для hang** (на affected hardware):

| Параметр | Значение | Почему |
|----------|----------|--------|
| Context | OpenGL 4.1 core, MSAA 4× | matches game |
| Окно | 640×480 (readpixels 1.2 MB) | 320×240 (307 KB) недостаточно |
| SwapInterval | 2 (VSync blocking) | **без VSync не триггерит вообще** даже при высокой нагрузке |
| CYCLES per frame | 2 минимум, 8 для быстрого repro | 1 не триггерит даже с COUNT=2000 |
| Triangle draw до readpixels | обязателен | без него driver elides readpixels как no-op на cleared backbuffer |
| CPU work между readpixels и teximage | **scattered** writes 500+ по всему buffer'у | sequential writes / busy-wait / 1-byte mutation — не триггерят |
| Writeback quad (draw) | обязателен в standalone | без него driver elides upload как unused |
| VRAM pressure | не нужен | прогон в игре с временно отключёнными texture ops висел при пустом VRAM |

**Репродуцируемые конфиги:**
- Быстрый (<5 сек): `CYCLES=8`, все остальные параметры baseline.
- Пороговый (~50 сек, маргинальный): `CYCLES=2`.

**Разница с реальной игрой:** в игре достаточно 1 readpixels/frame. В standalone нужно 2-8. Игра нагружает driver дополнительной работой (тысячи draws, state switches, VBO uploads) — это понижает порог триггера.

## Ложные гипотезы (исключены)

За сессию investigation перепробовали и исключили как root cause:

- **Memory pressure от текстур.** Hang воспроизводился при практически пустом VRAM (все texture ops временно выключены).
- **Upload path сам по себе** (glTexImage2D/glBindTexture). Stress-тесты 10000 upload'ов/кадр без readpixels — probability hang не меняется.
- **State setters / uniforms / VBO uploads / binds.** Временное отключение этих подсистем — hang оставался.
- **CPU saturation.** Busy-wait того же объёма времени что CPU plot — не триггерит.
- **Absolute readpixels/sec rate.** `SwapInterval=0` (unlimited FPS) даже с 8 циклами readpixels/frame — hang не триггерится. Значит важен не absolute rate, а "per-Present" count.
- **Attach debugger / PumpEvents / SDL_Delay / SetWindowTitle хаки** — не устраняют (попутные workaround'ы, не root cause).
- **Backpressure GPU queue.** `glFinish` каждый кадр — очередь пустая стабильно, а `SwapBuffers` всё равно 80-160 мс во время throttle.
- **DwmFlush per frame.** `msSwap` падает до 5-17 мс (DWM разбужен), но throttle смещается на pre-swap GL calls — блокировка остаётся.
- **Rain / GL stress mode амплификации.** `glBindTexture` циклом 10000/кадр, `glTexImage2D` re-upload 10000/кадр — probability hang не меняется.

## Ключевые числа

- Normal frame: 0-5 мс.
- Hang frame: 80-200+ мс, DWM roughtly 1 present / 10 сек во время active throttle.
- `glTexImage2D` в deg-state: **300-400 мс per occurrence** — жертва throttle, не создатель.
- Продолжительность throttle: десятки секунд до минут, редко требует reboot.
- Audio (WASAPI MMCSS) работает без throttle всегда — разный scheduling domain.

## FBO / render-to-texture

Не используется в коде вообще. Grep по `glBindFramebuffer / glReadBuffer / glDrawBuffer` — ни одного callera в `new_game/src/`. Значит `glReadPixels` всегда читает из default framebuffer `GL_BACK`.
