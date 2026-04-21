# workarounds — варианты обхода (на основе понимания причин)

Все варианты следуют из [FINDINGS.md](FINDINGS.md) (необходимые условия триггера) и [hypothesis.md](hypothesis.md) (механизм на уровне драйвера/WDDM). Ни один не является "production решением" — это trade-offs с разными потерями.

Напоминание: для триггера hang'а необходимы **все** условия одновременно — per-frame `glReadPixels`, scattered CPU writes в readback-buffer, writeback через `glTexImage2D` + fullscreen quad, VSync-blocked `SwapBuffers`, 2+ циклов per frame (в игре). Снять любое одно → hang исчезает.

## 1. Убрать readback-эффекты полностью (GPU-native замена)

Переделать stars / wibble / FONT_buffer_draw на шейдерный рендер без `ge_lock_screen`. Убирает сразу: readpixels, scattered writes, writeback-quad.

- **Плюсы:** устраняет root cause полностью, портируемо (не зависит от GPU/driver), визуально эффекты сохраняются.
- **Минусы:** значительная работа, нужна shader infrastructure для post-process (wibble).
- **Детали реализации: не скоуп этого исследования** — решает fix-агент.

## 2. Убрать VSync (`SwapInterval=0`)

Снять blocking Present. Согласно standalone repro, hang не триггерится без VSync даже при 8 циклах readpixels+writeback/кадр.

- **Плюсы:** минимум кода (одна строчка), визуальные эффекты сохраняются, readback остаётся как есть.
- **Минусы:** screen tearing (нет sync с монитором), FPS может уйти в сотни — нужен soft frame cap (`lock_frame_rate`) чтобы не жечь CPU/GPU вхолостую.
- **Риски:** проверено только на авторском железе. На других GPU/драйверах driver может throttle'ить даже без blocking Present (зависит от конкретной policy). На Steam Deck / Wayland / Vulkan compositor — поведение не проверено, возможно ещё и не нужен (там другой compositor design).
- **Не решает** если кто-то позже снова поставит `SwapInterval=1/2` — bug вернётся.

## 3. Убрать эффекты без замены

Просто не вызывать `ge_lock_screen` в проблемных callsite'ах (gate'ать условно, возвращать early).

- **Плюсы:** нулевая работа, гарантированно убирает hang.
- **Минусы:** визуальные потери — нет звёзд ночью, нет ряби на лужах, debug-шрифт не рисуется. Для релиза — потеря visual content.
- **Приемлемо:** как emergency escape hatch (hotfix на билд до production fix'а).

## 4. PBO async readpixels (НЕ работает как fix)

Использовать Pixel Buffer Object для async read — `glReadPixels` в PBO, на следующем кадре CPU читает из уже готового PBO. Распространённый совет в OpenGL best-practices.

- **Минусы:** не устраняет все компоненты — writeback через `glTexImage2D` + fullscreen quad + scattered CPU writes на PBO-mapped память остаются. Driver может считать PBO async read тем же "sync-read-pattern" для throttle policy. Не проверено на нашей репре; умозрительно — **скорее всего недостаточно**.
- **Вывод:** workaround'ом не считать. Для полного fix'а нужен вариант 1.

## 5. Render-to-texture + read из FBO (не проверено, не рекомендуется)

Рисовать сцену в FBO, `glReadPixels` из FBO вместо default framebuffer.

- **Минусы:** всё равно sync GPU read, всё равно writeback, scattered writes — условия триггера те же. FBO не даёт преимущества в этом scenario.
- **Вывод:** не workaround.

## Сравнение

| Вариант | Работа | Визуал сохранён | Надёжность | Готовность |
|---------|--------|-----------------|-----------|-----------|
| 1. Шейдерная замена | большая | да | полная | для релиза |
| 2. Убрать VSync | минимум | да | зависит от hardware | для временного обхода |
| 3. Убрать эффекты | минимум | нет | полная | для hotfix'а |
| 4. PBO async | средняя | да | недостаточно | не workaround |
| 5. FBO RTT | средняя | да | не работает | не workaround |

## Рекомендация

**Вариант 1** для релиза. Варианты 2 или 3 — для временного обхода если нужно разблокировать тестирование до fix'а.
