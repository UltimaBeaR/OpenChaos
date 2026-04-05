# Тестирование способов вывода текста на экран

Дата: 2026-04-05

## Методика

Временный тестовый код в `game_tick.cpp` — перед блоком `if (!allow_debug_keys) return;`.
Каждая клавиша 1-8 вызывает один метод вывода текста. Методы 3, 4, 5, 8 рисуют
каждый кадр в течение ~3 секунд (180 тиков). Остальные — одноразовый вызов.

### Как восстановить тестовый код

В `game_tick.cpp`, перед строкой `if (!allow_debug_keys) return;` в функции
`process_controls()`, вставить блок ниже. Также нужны includes в начале файла:

```cpp
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/text/text.h"
#include "engine/graphics/text/font.h"
```

```cpp
// TEMP: debug-print method verification — each key triggers one method.
// Remove after testing.
{
    static int dbg_print_active = -1;
    static int dbg_print_timer = 0;
    Thing* darci = NET_PERSON(0);

    if (Keys[KB_1]) { Keys[KB_1] = 0; CONSOLE_text_at(300, 200, 3000, "TEST: CONSOLE_text_at"); dbg_print_active = -1; }
    if (Keys[KB_2]) { Keys[KB_2] = 0; CONSOLE_status((CBYTE*)"TEST: CONSOLE_status"); dbg_print_active = -1; }
    if (Keys[KB_3]) { Keys[KB_3] = 0; dbg_print_active = 3; dbg_print_timer = 0; }
    if (Keys[KB_4]) { Keys[KB_4] = 0; dbg_print_active = 4; dbg_print_timer = 0; }
    if (Keys[KB_5]) { Keys[KB_5] = 0; dbg_print_active = 5; dbg_print_timer = 0; }
    if (Keys[KB_6]) { Keys[KB_6] = 0; PANEL_new_text(NULL, 3000, "TEST: PANEL_new_text"); dbg_print_active = -1; }
    if (Keys[KB_7]) { Keys[KB_7] = 0; PANEL_new_info_message((CBYTE*)"TEST: PANEL_new_info_message"); dbg_print_active = -1; }
    if (Keys[KB_8]) { Keys[KB_8] = 0; dbg_print_active = 8; dbg_print_timer = 0; }

    if (dbg_print_active > 0 && dbg_print_timer < 180) {
        dbg_print_timer++;
        if (dbg_print_active == 3) FONT2D_DrawString("TEST: FONT2D_DrawString", 300, 300, 0xffffff, 255);
        if (dbg_print_active == 4) draw_text_at(300, 350, (CBYTE*)"TEST: draw_text_at", 0);
        if (dbg_print_active == 5) FONT_buffer_add(300, 400, 255, 255, 0, 0, "TEST: FONT_buffer_add");
        if (dbg_print_active == 8 && darci)
            AENG_world_text(darci->WorldPos.X >> 8, (darci->WorldPos.Y >> 8) - 300, darci->WorldPos.Z >> 8, 0xff, 0xff, 0x00, 0, "TEST: AENG_world_text");
    } else if (dbg_print_timer >= 180) {
        dbg_print_timer = 0;
        dbg_print_active = -1;
        CONSOLE_status((CBYTE*)"");
    }
}
```

Клавиши 1-8 конфликтуют с debug keys (allow_debug_keys), поэтому блок размещён
ДО проверки `if (!allow_debug_keys)` — так клавиши работают всегда.

## Результаты

### Клавиша 1 — CONSOLE_text_at(300, 200, 3000, text)
**Результат:** ❌ ничего не появляется на экране.
**Побочный эффект:** возвращает тени персонажей в нормальное состояние (если они были
испорчены клавишами 3 или 4).

### Клавиша 2 — CONSOLE_status(text)
**Результат:** ✅ работает. Зелёный текст "TEST: CONSOLE_status" появляется в левом
верхнем углу (тот же шрифт что при вводе команд через F9). Сообщение НЕ пропадает
само — остаётся пока не будет перезаписано. Если после этого нажать 3 (FONT2D) и
подождать 3 секунды — зелёный текст пропадает (таймер теста вызывает
`CONSOLE_status("")` при сбросе).
**Побочный эффект:** также возвращает тени в нормальное состояние.

### Клавиша 3 — FONT2D_DrawString("TEST: FONT2D_DrawString", 300, 300, 0xffffff, 255)
**Результат:** ❌ текст НЕ появляется на экране.
**Побочный эффект:** ТЕНИ ПЕРСОНАЖЕЙ ПРОПАДАЮТ. Тени становятся невидимыми на всё время
пока FONT2D рисует (~3 сек). При повторном нажатии 3 — тени не возвращаются.
Нажатие клавиш 1, 2 или 4 — возвращает тени в нормальное состояние.
На тень машины НЕ действует — только тени персонажей.

### Клавиша 4 — draw_text_at(300, 350, text, 0)
**Результат:** ⚠️ частично работает, но рисует НЕ ТАМ. Белый текст появляется
ПОВЕРХ ТЕНЕЙ ПЕРСОНАЖЕЙ (не на экране в указанных координатах, а прямо на проекции
тени на земле). Текст трудно прочесть т.к. он мелкий и на тени. Через пару секунд
текст пропадает и тени возвращаются в нормальное состояние.
Если нажать 3 во время отображения — тень пропадает вместе с текстом.
Если нажать 1 или 2 — тени нормализуются, текст уходит.
На тень машины НЕ действует.

### Клавиша 5 — FONT_buffer_add(300, 400, 255, 255, 0, 0, text)
**Результат:** ❌ ничего не появляется на экране.
**Побочный эффект:** возвращает тени в нормальное состояние и убирает текст с теней
(если были артефакты от 3 или 4).

### Клавиша 6 — PANEL_new_text(NULL, 3000, text)
**Результат:** ✅ работает. Появляется радио-сообщение внизу экрана с иконкой рации
и звуковым сигналом. По поведению идентично CONSOLE_text — тот же механизм отображения.
Пока сообщение на экране — повторные нажатия ничего не делают и звук не воспроизводится.
Когда сообщение пропадает и нажать снова — появляется заново со звуком.

### Клавиша 7 — PANEL_new_info_message(text)
**Результат:** ✅ работает. Выезжающее сообщение в левом нижнем углу. Автоматически
пропадает через ~2 секунды.

### Клавиша 8 — AENG_world_text(x, y-300, z, 0xff, 0xff, 0x00, 0, text)
**Результат:** ❌ ничего не появляется на экране (тест без debug overlay).
Внутри использует FONT_buffer_add() который тоже не работает — скорее всего
не заработает и с debug overlay.
**Побочный эффект:** возвращает тени в нормальное состояние.
**Не проверено:** с включённым debug overlay (F9 → bangunsnotgames → Ctrl).

### Общее наблюдение по теням

Проблема с тенями связана с render state leak в OpenGL рендерере. Те же артефакты
видны когда враг целится огнестрельным оружием — серые пунктирные линии прицела
рисуются, и в этот момент тень персонажа отображается серым прямоугольником того же
цвета что пунктиры. Когда пунктиры пропадают — тень возвращается в норму.

Баг записан в `known_issues_and_bugs.md` ("OpenGL: тень глючит когда враг целится").
Вероятно одна и та же root cause — render state утекает между poly pages.

FONT2D_DrawString и draw_text_at ломают тени по той же причине — они меняют
текстуру/blend mode и это утекает в следующий poly page (тени).

## FONT_draw — не проверено

FONT_draw() / FONT_draw_coloured_text() требует ge_lock_screen()/ge_unlock_screen().
Не включён в тест. В OpenGL бэкенде ge_lock_screen() возвращает NULL (screen buffer
не реализован) — скорее всего не работает.
