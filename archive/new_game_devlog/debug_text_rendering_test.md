# Расследование: способы вывода текста на экран

Дата: 2026-04-05

## Цель

Протестировать все доступные функции вывода текста на экран, задокументировать поведение
каждой, пофиксить сломанные, понять ограничения. Итоговая справка → `.claude/skills/text-rendering/SKILL.md`.

## Методика

### Тест 1: кнопки 1-8 из game tick

Временный тестовый код в `game_tick.cpp` — перед блоком `if (!allow_debug_keys) return;`.
Каждая клавиша 1-8 вызывает один метод вывода текста. Методы 3, 4, 5, 8 рисуют
каждый кадр в течение ~3 секунд (180 тиков). Остальные — одноразовый вызов.
Клавиши 1-8 конфликтуют с debug keys, поэтому блок размещён ДО проверки `allow_debug_keys`.

### Тест 2: render-pass вызовы

Тестовые вызовы вставлены в `CONSOLE_draw()` (render pass) и `game.cpp` (перед flip).

### Тест 3: визуальная проверка всех функций одновременно

Все работающие функции выведены одновременно на экран для визуального подтверждения.
В игре и в меню.

---

## Результаты по каждой функции

### 1. CONSOLE_text_at(x, y, delay, fmt, ...)

**Первый тест (кнопка 1):** ❌ ничего не появляется на экране.

**Расследование:** в `CONSOLE_draw()` не было кода отрисовки массива `CONSOLE_mess[]`.
В оригинальном коде (fallen/DDEngine/Source/console.cpp, строки 183-206) рендеринг
`CONSOLE_mess[]` был **закомментирован** разработчиками. Оригинал использовал
`font->DrawString()` — 3D-шрифт (`Font3D`), который не был реализован в финальном коде.
`CONSOLE_text_at()` корректно добавляла текст в очередь `CONSOLE_mess[]`, но рисовать
было нечему.

**Фикс:** добавлен цикл отрисовки `CONSOLE_mess[]` через `FONT2D_DrawString` в
`CONSOLE_draw()` (файл `engine/console/console.cpp`), перед `POLY_frame_draw`.
Логика таймера скопирована из закомментированного оригинала.

**Результат после фикса:** ✅ работает. Белый текст в указанных экранных координатах,
пропадает через `delay` мс. Безопасна для вызова из любого места кода.

**Изменённые файлы:** `engine/console/console.cpp` — добавлен цикл отрисовки CONSOLE_mess[].

---

### 2. CONSOLE_status(text)

**Тест (кнопка 2):** ✅ работает сразу, фиксов не требовала.

Зелёный текст в левом верхнем углу. Тот же шрифт что при вводе команд через F9.

**Нюанс:** сообщение **НЕ пропадает само**. Это by design — внутри просто `strcpy` в
статический буфер `console_status_text`, таймера нет. Остаётся на экране пока не будет
явно очищено через `CONSOLE_status((CBYTE*)"")`.

**Артефакт тестового кода:** в тесте текст пропадал при нажатии кнопок 3/4/5/8 через
3 секунды — т.к. при сбросе таймера теста вызывался `CONSOLE_status("")`. Это связь
через тестовый код, не через саму функцию.

---

### 3. FONT2D_DrawString(chr, x, y, rgb, scale, page, fade)

**Первый тест (кнопка 3, без обёртки):** ❌ текст не виден. Побочный эффект: **тени
персонажей пропадают** + артефакты-силуэты теней на дороге/зебре. Та же root cause
что в `shadow_corruption_investigation.md` — VB slot reuse. Нажатие кнопок 1, 2 —
возвращало тени в нормальное состояние. Тень машины не затрагивалась.

**Фикс теней:** обёртка всех per-frame вызовов кнопок 3/4/5/8 в
`POLY_frame_init(UC_FALSE, UC_FALSE)` / `POLY_frame_draw(UC_FALSE, UC_TRUE)`.
Тени после этого не ломаются.

**Тест после фикса теней:** ❌ текст всё равно не виден. `POLY_frame_draw` рисует текст
на экран, но основной render pass (который идёт после game tick) перерисовывает
весь экран поверх. Текст затирается.

**Тест из render pass (CONSOLE_draw):** ✅ работает. `CONSOLE_status` и `CONSOLE_text_at`
обе используют `FONT2D_DrawString` внутри и рисуют корректно.

**Вывод:** низкоуровневая функция, работает только из render pass. Из game tick — бесполезна
в любом варианте: без обёртки ломает тени, с обёрткой текст затирается.

---

### 4. draw_text_at(x, y, message, font_id)

**Первый тест (кнопка 4, без обёртки):** ⚠️ текст рисовался **поверх теней персонажей**
(не на экране в указанных координатах, а прямо на проекции тени на земле). Текст мелкий,
трудно прочесть. Через пару секунд пропадал, тени нормализовались. Тень машины
не затрагивалась.

**Тест после обёртки POLY_frame_init/draw:** ❌ текст не виден (затирается рендером).

**Тест из render pass (CONSOLE_draw):** ❌ текст не виден.

**Расследование:** добавлен stderr лог в `draw_text_at`. Результат:
`page=1416 font_id=0 font=000001BAA5D63190` — шрифт загружен, `ge_get_font` возвращает
валидный указатель. Функция проходит дальше и добавляет квады через
`POLY_add_quad(quad, POLY_PAGE_TEXT, ...)`. Но текст не рендерится.

Причина: `POLY_PAGE_TEXT` рендерится только в основном POLY frame, который создаётся
в `AENG_draw()`. Тесты из `CONSOLE_draw` и game tick создавали **отдельный** POLY frame
через `POLY_frame_init/draw` — квады попадали в него, он рисовался раньше основного
рендера и затирался.

**Тест из основного POLY frame (AENG_draw):** ✅ работает! Вставлен вызов
`draw_text_at(300, 100, "TEST: draw_text_at in AENG", 0)` перед `POLY_frame_draw(UC_TRUE, UC_TRUE)`
в `AENG_draw` (~строка 6819). Белый текст виден.

Требует предварительную установку `text_fudge = UC_FALSE; text_colour = 0x00ffffff;`.

Используется в реальном коде: `draw_centre_text_at` (text.cpp:211) → `draw_text_at`,
вызывается из `process_bullet_points` (game.cpp:542) для текста вступительной катсцены миссии.
"Press Anything To Play" (poly.cpp:2070) — закомментирован в текущем коде.

**Вывод:** функция рабочая, но только из основного POLY frame в `AENG_draw`.
Для отладки непригодна — требует вставку кода внутрь рендер-пайплайна.

---

### 5. FONT_buffer_add(x, y, r, g, b, shadow, fmt, ...)

**Первый тест (кнопка 5, без обёртки):** ❌ ничего не видно.
**Тест после обёртки POLY_frame_init/draw:** ❌ ничего не видно.

**Расследование:** `FONT_buffer_add` только кладёт текст в буфер. Рисует `FONT_buffer_draw()`
в `game.cpp`. `FONT_buffer_draw` использует `ge_lock_screen` → `FONT_draw_coloured_char`
(пиксельный рендер).

**Проблема:** `FONT_buffer_draw()` вызывалась ТОЛЬКО при `ControlFlag && allow_debug_keys`
(`game.cpp:622`). Без debug overlay буфер никогда не флашился.

**Фикс:** `FONT_buffer_draw()` вынесена из-под условия `ControlFlag && allow_debug_keys`.
Вызывается единственный раз в одном месте (`game.cpp`), поэтому условие было бессмысленным.
Если буфер пуст (`FONT_message_upto == 0`) — ранний return, ноль оверхеда.

**Тест после фикса (прямой вызов в game.cpp перед FONT_buffer_draw):** ✅ работает.
Жёлтый пиксельный текст с тенью виден на экране.

**Изменённые файлы:** `game/game.cpp` — `FONT_buffer_draw()` вынесена из-под условия,
добавлен `#include "engine/graphics/text/font.h"`.

---

### 6. PANEL_new_text(who, delay, fmt, ...)

**Тест (кнопка 6):** ✅ работает сразу. Радио-сообщение с иконкой рации и звуком.

**Тест с инкрементирующимся текстом:** каждое нажатие генерировало уникальный текст
("TEST: PANEL_new_text 1", "...2", "...3" и т.д.).

**Результат:** при разном тексте сообщения отображаются **стеком** — до 3 видно
одновременно. Каждое со своим таймером. Звук воспроизводится при каждом вызове сразу.

При одинаковом тексте от того же `who` — сбрасывается таймер, новый слот не создаётся,
звук НЕ воспроизводится повторно.

Определяется в коде: `PANEL_new_text` (panel.cpp) проверяет `strcmp(pt->text, message)`
для каждого активного слота с тем же `who`. Совпадение → перезапись delay, return.
Несовпадение → новый слот через `PANEL_text_tail`.

---

### 7. PANEL_new_info_message(fmt, ...)

**Тест (кнопка 7):** ✅ работает сразу. Выезжающее сообщение в левом нижнем углу,
автоматически пропадает через ~2 секунды. Каждое нажатие — новое сообщение.

---

### 8. AENG_world_text(x, y, z, red, blue, green, shadow, fmt, ...)

**Первый тест (кнопка 8, из game tick):** ❌ ничего не видно.
**Тест после обёртки POLY_frame_init/draw:** ❌ ничего не видно.

**Расследование render pass (тест 2):**

Вставлен безусловный вызов `AENG_world_text` в render pass в `aeng.cpp`.
Первая попытка — в indoor блоке (`AENG_draw_warehouse`, ~строка 7672). Результат: ❌.
Добавлен stderr лог в `AENG_world_text` — **ноль вывода**. Функция не вызывалась.

**Root cause:** в `aeng.cpp` есть **два** цикла рендеринга персонажей:
- `AENG_draw_city` (~строка 6130) — **outdoor** уровни
- `AENG_draw_warehouse` (~строка 7630) — **indoor** уровни

Тест стоял в indoor блоке, а тестовая миссия 1 — outdoor. Код не достигался.

Вторая попытка — тест добавлен в outdoor блок (`AENG_draw_city`, ~строка 6179).
Добавлен stderr лог: `clip=0x10 valid=1` — `POLY_transform` работает, `IsValid()` true.

**Результат:** ✅ работает. Текст виден над персонажами.

**Особенности позиционирования:** координаты `(x, y, z)` — мировые. `y` — высота (вверх).
Ноги персонажа = `WorldPos.Y >> 8`. Для текста примерно над головой: `+ 0x40` ... `+ 0x50`.
Debug overlay текст использует `+ 0x60`. Из-за перспективной проекции текст смещается
относительно визуальной модели при увеличении расстояния — это нормально, inherent limitation.

**⚠️ Порядок цветов — RBG, НЕ RGB!** Параметры функции: `red, blue, green`.
Внутри `AENG_world_text` передаёт в `FONT_buffer_add` как `red, green, blue` (переставляет).
Пример: чтобы получить жёлтый (R=255, G=255, B=0), нужно передать `0xff, 0x00, 0xff`
(red=ff, blue=00, green=ff). Обнаружено при тестировании — передавали `0xff, 0x00, 0xff`
ожидая розовый, получили жёлтый.

---

### FONT_draw(x, y, fmt, ...) и FONT_draw_coloured_text(x, y, r, g, b, fmt, ...)

**Первоначальная оценка:** не тестировалось, предполагалось что `ge_lock_screen()` возвращает
NULL в OpenGL бэкенде.

**Расследование:** `ge_lock_screen()` в OpenGL бэкенде **реализован**. Читает backbuffer
через `glReadPixels` в CPU буфер. `ge_unlock_screen()` заливает модифицированный буфер
обратно как fullscreen quad.

**Тест из CONSOLE_draw (render pass):** ✅ `FONT_draw` — виден жёлтый текст с красной тенью.
**Тест из game.cpp (перед flip):** ✅ виден.
**Тест из attract.cpp (меню, перед AENG_flip):** ✅ виден.

`FONT_draw` — пиксельный рендер (5×7 bitmap), через `ge_plot_pixel`. Не зависит от poly pages,
не вызывает shadow corruption. Требует `ge_lock_screen()` / `ge_unlock_screen()` обёртку.

`FONT_draw_coloured_text` — тот же механизм, но с пользовательским цветом (r, g, b) и без тени.

**В меню работает** — единственный подтверждённый способ вывести debug текст в главном меню.

---

## Shadow corruption от низкоуровневых функций

Все функции добавляющие геометрию в poly pages (FONT2D_DrawString, draw_text_at) — при вызове
из game tick (до `POLY_frame_init` в render pass) вызывают shadow corruption: тени персонажей
пропадают, появляются артефакты-силуэты теней на дороге. Root cause та же что в
`shadow_corruption_investigation.md` — VB slot reuse.

Обёртка `POLY_frame_init/POLY_frame_draw` вокруг вызовов из game tick предотвращает corruption
теней, но текст всё равно не виден (затирается основным рендером).

`FONT_buffer_add` и `AENG_world_text` при вызове из game tick тоже добавляли геометрию
в poly pages и теоретически могли вызвать corruption, но с обёрткой это тоже исправлялось.

**Game-tick-safe функции** (CONSOLE_text_at, CONSOLE_status, MSG_add, PANEL_*, FONT_buffer_add)
НЕ вызывают shadow corruption — они используют очереди/буферы, отрисовка происходит
в правильной фазе рендер-пайплайна.

---

## Фиксы внесённые в код

| Файл | Изменение | Причина |
|------|-----------|---------|
| `engine/console/console.cpp` | Добавлен цикл отрисовки `CONSOLE_mess[]` через `FONT2D_DrawString` в `CONSOLE_draw()` | `CONSOLE_text_at` не рисовала — рендеринг был закомментирован в оригинале |
| `game/game.cpp` | `FONT_buffer_draw()` вынесена из-под `ControlFlag && allow_debug_keys` | Буфер не флашился без debug overlay, `FONT_buffer_add` и `AENG_world_text` не работали |
| `game/game.cpp` | Добавлен `#include "engine/graphics/text/font.h"` | Нужен для `FONT_buffer_draw` declaration |

---

## Меню (attract mode)

Главное меню (`game_attract_mode` в `attract.cpp`) имеет свой render loop:
`FRONTEND_loop()` → `AENG_flip()`. Не вызывает `CONSOLE_draw()`, `FONT_buffer_draw()`,
`process_controls()`, или game loop в `game.cpp`.

**Работает в меню:** `FONT_draw` / `FONT_draw_coloured_text` с ручной вставкой
`ge_lock_screen/unlock` перед `AENG_flip()` в `attract.cpp`.

**Не работает в меню:** все остальные функции — их render path не выполняется.

**Меню паузы** — overlay поверх игрового рендера, тот же pipeline что в игре.
Все игровые текстовые функции работают в меню паузы.

---

## Как восстановить тестовый код кнопок 1-8

В `game_tick.cpp`, перед строкой `if (!allow_debug_keys) return;` в функции
`process_controls()`, вставить блок ниже. Также нужны includes:

```cpp
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/text/text.h"
#include "engine/graphics/text/font.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/console/message.h"
```

```cpp
// TEMP: debug-print method verification — each key triggers one method.
{
    static int dbg_print_active = -1;
    static int dbg_print_timer = 0;
    Thing* darci = NET_PERSON(0);

    if (Keys[KB_1]) { Keys[KB_1] = 0; CONSOLE_text_at(300, 200, 3000, "TEST: CONSOLE_text_at"); dbg_print_active = -1; }
    if (Keys[KB_2]) { Keys[KB_2] = 0; CONSOLE_status((CBYTE*)"TEST: CONSOLE_status"); dbg_print_active = -1; }
    if (Keys[KB_3]) { Keys[KB_3] = 0; dbg_print_active = 3; dbg_print_timer = 0; }
    if (Keys[KB_4]) { Keys[KB_4] = 0; dbg_print_active = 4; dbg_print_timer = 0; }
    if (Keys[KB_5]) { Keys[KB_5] = 0; dbg_print_active = 5; dbg_print_timer = 0; }
    if (Keys[KB_6]) { Keys[KB_6] = 0; static int dbg_panel_cnt = 0; dbg_panel_cnt++; PANEL_new_text(NULL, 3000, "TEST: PANEL_new_text %d", dbg_panel_cnt); dbg_print_active = -1; }
    if (Keys[KB_7]) { Keys[KB_7] = 0; PANEL_new_info_message((CBYTE*)"TEST: PANEL_new_info_message"); dbg_print_active = -1; }
    if (Keys[KB_8]) { Keys[KB_8] = 0; dbg_print_active = 8; dbg_print_timer = 0; }

    if (dbg_print_active > 0 && dbg_print_timer < 180) {
        dbg_print_timer++;
        POLY_frame_init(UC_FALSE, UC_FALSE);
        if (dbg_print_active == 3) FONT2D_DrawString("TEST: FONT2D_DrawString", 300, 300, 0xffffff, 256, POLY_PAGE_FONT2D);
        if (dbg_print_active == 4) draw_text_at(300, 350, (CBYTE*)"TEST: draw_text_at", 0);
        if (dbg_print_active == 5) FONT_buffer_add(300, 400, 255, 255, 0, 0, "TEST: FONT_buffer_add");
        if (dbg_print_active == 8 && darci)
            AENG_world_text(darci->WorldPos.X >> 8, (darci->WorldPos.Y >> 8) - 300, darci->WorldPos.Z >> 8, 0xff, 0xff, 0x00, 0, "TEST: AENG_world_text");
        POLY_frame_draw(UC_FALSE, UC_TRUE);
    } else if (dbg_print_timer >= 180) {
        dbg_print_timer = 0;
        dbg_print_active = -1;
        CONSOLE_status((CBYTE*)"");
    }
}
```
