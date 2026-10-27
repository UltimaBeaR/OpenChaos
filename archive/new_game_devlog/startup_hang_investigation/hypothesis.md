# hypothesis — предполагаемые причины

Это **не подтверждённые** механизмы — исключительно reasonable guesses основанные на поведении bisect'а и external research. Для production fix'а их знать не обязательно (fix — просто убрать readback), но полезно для понимания контекста.

## Что точно известно (см. [FINDINGS.md](FINDINGS.md))

- Необходимые условия триггера все сразу:
  1. Per-frame `glReadPixels` из default framebuffer (sync GPU→CPU).
  2. Scattered CPU writes в readback-буфер (sequential — не триггерит).
  3. `glTexImage2D` upload модифицированного буфера.
  4. Draw fullscreen quad с этой текстурой (без draw — driver elides upload).
  5. VSync-blocked `SwapBuffers`.
  6. Минимум 2 readpixels+upload+quad циклов per frame (один — не триггерит).

Один любой элемент удалить → hang исчезает. Почему именно эта комбинация — ниже.

## Предполагаемая цепочка механизмов

### 1. NVIDIA "Threaded Optimization" fallback

NVIDIA driver по умолчанию offload'ит GL команды в worker thread (async submission). При детекте repeated implicit synchronization (каждый `glReadPixels` = implicit flush + GPU wait) — driver **отключает threaded optimization для процесса** и переходит в strict sync mode. NVIDIA Developer Forums / Khronos Community документируют это поведение, включая warning `"Pixel-path performance warning: Pixel transfer is synchronized with 3D rendering"`.

**Следствие:** после этого момента каждый GL call блокирует main thread. Включая `SwapBuffers` — отсюда 100-160мс стало вместо 0-5мс.

### 2. WDDM scheduler quantum throttling

Windows Display Driver Model держит GPU scheduler в kernel (Dxgkrnl.sys). Когда процесс создаёт sync chokepoint (каждый frame ждёт GPU finish), scheduler **понижает GPU quantum priority** процесса, чтобы DWM compositor и другие GPU clients получали приоритет. Это не TDR (2-сек threshold не достигается), а скорее quota-based downgrade.

**Следствие:** даже если driver отменил threaded optimization, WDDM всё равно даёт процессу всё меньше GPU time — поэтому throttling усугубляется со временем, а не self-recover'ится быстро.

### 3. DWM compositor contention

Modern Windows (Vista+) композитит все окна через DWM. Primary surface больше не direct-to-screen — `SwapBuffers` становится Present через DWM очередь. При active VSync игра ждёт DWM готовности, но DWM тоже scheduled через WDDM. Сочетание "процесс деградирован + Present blocked на compositor" создаёт feedback loop: DWM не даёт рано commit наш кадр, WDDM ещё уменьшает приоритет, DWM даёт ещё меньше.

**Следствие:** весь desktop начинает лагать (DWM замедлен), не только наше окно. Отсюда "system-wide lag".

### 4. Почему VSync-blocked SwapBuffers необходим

Без VSync (`SwapInterval=0`) `SwapBuffers` возвращает немедленно, процесс не блокируется, WDDM не видит "waiting for Present" состояние. В этом режиме ни NVIDIA driver, ни WDDM не применяют throttle policy — только при blocking Present процесс становится "scheduling victim".

### 5. Почему scattered CPU writes важны (а sequential нет)

Гипотеза: после `glReadPixels` buffer в CPU memory. Перед `glTexImage2D` его нужно скопировать в staging/VRAM. Driver делает DMA copy из system RAM через PCIe.

- **Sequential writes** на x86-64 идут через write combining buffer (WCB), coalesce'ятся в burst writes прямо в RAM, CPU cache не трогает эти lines как dirty.
- **Scattered writes** в random addresses не проходят WCB — оставляют dirty cache lines в L1/L2 по всему 1.2 MB buffer'у.

При DMA copy: cache coherency protocol (MESI/MESIF) должен снапнуть каждую cache line в диапазоне, проверить dirty ли. Scattered pattern → много dirty lines → много snoop latency → CPU stalls на каждом upload.

Это добавляет ~микросекунды к каждому `glTexImage2D`, плюс создаёт busy GPU queue. Кумулятивно за несколько циклов — достаточно чтобы WDDM quota policy среагировала.

**Слабое место гипотезы:** это объяснение для amount CPU-GPU overhead per cycle, но не обязательно единственный фактор. Возможны другие причины (например driver может делать какие-то cache flush intrinsics для текстур после modification, и scattered vs sequential их по-разному affects).

### 6. Почему 2+ циклов per frame

Возможно WDDM detect'ит "multiple sync reads per Present()" как sign'у процесса-sync-heavy и triggers throttle. Один sync read per frame не превышает порог; два+ — превышает.

Альтернатива: кумулятивная нагрузка scattered-writes + upload'ов достигает критической точки в одном presentation interval. В игре 1 readpixels/frame тоже триггерит, но там есть "фон" нагрузки (множество draws, state changes) — фактическая CPU/driver load equiv ~multiple readpixels в standalone.

## Почему hardware-specific

Voor combinations GPU/driver/OS имеют разные scheduling policies:

- **NVIDIA driver versions** — threaded optimization fallback mechanism менялся от версии к версии. Новые могут быть чувствительнее или наоборот умнее.
- **Intel / AMD драйверы** — разные threaded submission implementations, разные policies для sync reads. Возможно вообще не применяют такой downgrade.
- **Windows 10 vs 11** — WDDM evolution (1.0 → 3.x), DWM scheduling изменился.
- **Monitor refresh + G-Sync / FreeSync** — влияет на DWM vsync cadence и Present queue behavior.
- **Hybrid GPU setups (laptop, Optimus/PRIME)** — дополнительная проксирование через integrated GPU, другое поведение.

Тот факт что у автора воспроизводится, а у второго пользователя с **тем же бинарём** — нет, говорит что триггер находится на границе какого-то порога, сдвинутого в worst-case на authoring hardware.

## Почему в оригинале (1999-2000) это не было проблемой

- Target железо: Win98/ME/2000/XP без DWM compositor. DirectDraw Lock возвращал прямой pointer в video memory — real в microseconds.
- GPU были слабые в vertex processing — CPU pixel plot через VRAM Lock реально был быстрее чем GPU primitive draw (для тысяч одиночных пикселей).
- Shader'ов не было. Post-process effects (wibble) можно было делать только через Lock/modify/Unlock.

Финальные release'ы Urban Chaos (PC, PS1) ran on target hardware где этот pattern был idiomatic и дешёвый. Windows Vista+ + WDDM перевели DirectDraw Lock в emulation через GDI (медленно + sync), и пока никто не запускает ту сборку на современном Windows — проблема невидима.

Наш port сохранил rendering design, но теперь он на modern WDDM — вот hang и всплыл.

## Практические следствия

- Условия триггера (`glReadPixels` + CPU scattered writes + writeback quad + VSync) все вместе — специфичный legacy pattern, который на современных драйверах попадает в sync-read throttling policies.
- Любое GPU-native rendering (shaders, persistent textures, без readback) избегает условий триггера by construction.
- Варианты обхода и trade-offs — [workarounds.md](workarounds.md).
