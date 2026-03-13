# PSX Controller Layout — Urban Chaos

**Источник:** `fallen/psxlib/Source/GHost.cpp`, `fallen/Source/Controls.cpp`, `fallen/Source/interfac.cpp`

Документация PSX-управления как база для геймпад-поддержки в новой игре.

---

## Маппинг PAD-констант → физические кнопки

Стандартные константы PSX SDK (`libpad.h`):

| Константа | Физическая кнопка |
|-----------|------------------|
| `PAD_LU`  | D-Pad Up         |
| `PAD_LD`  | D-Pad Down       |
| `PAD_LL`  | D-Pad Left       |
| `PAD_LR`  | D-Pad Right      |
| `PAD_RU`  | Triangle (▲)     |
| `PAD_RL`  | Square (■)       |
| `PAD_RR`  | Circle (●)       |
| `PAD_RD`  | Cross / X (✕)    |
| `PAD_FLT` | L1               |
| `PAD_FLB` | L2               |
| `PAD_FRT` | R1               |
| `PAD_FRB` | R2               |
| `PAD_START` | Start           |
| `PAD_SEL`   | Select          |

---

## Конфигурации кнопок (4 варианта)

GHost.cpp определяет 4 конфига — игрок выбирает в меню (`pad_config`).

### Config 0 (по умолчанию) — `pad_cfg0`

| Кнопка | Действие (INPUT_MASK_*) |
|--------|------------------------|
| D-Up (PAD_LU) | RUN (бег вперёд) |
| D-Left (PAD_LL) | LEFT |
| D-Right (PAD_LR) | RIGHT |
| D-Down (PAD_LD) | BACKWARDS |
| Triangle (PAD_RU) | KICK |
| Square (PAD_RL) | PUNCH |
| Circle (PAD_RR) | ACTION |
| Cross (PAD_RD) | JUMP |
| L1 (PAD_FLT) | CAMERA (toggle first-person) |
| R1 (PAD_FRT) | STEP_RIGHT (new target / strafe) |
| L2 (PAD_FLB) | CAM_LEFT (debounce) |
| R2 (PAD_FRB) | CAM_RIGHT (debounce) |
| Start | PAUSE (debounce) |
| Select | INVENTORY (debounce) |

### Config 1 — `pad_cfg1`

Отличие от cfg0: **R1 = RUN** (вместо STEP_RIGHT), STEP_RIGHT убран.
Остальные кнопки идентичны cfg0.

### Config 2 — `pad_cfg2`

| Кнопка | Действие |
|--------|---------|
| Triangle | PUNCH |
| Square | KICK |
| Circle | JUMP |
| Cross | ACTION |
| L1 | CAMERA |
| R1 | STEP_RIGHT |
| L2 | CAM_LEFT |
| R2 | CAM_RIGHT |

D-Pad и системные кнопки — как в cfg0.

### Config 3 — `pad_cfg3`

| Кнопка | Действие |
|--------|---------|
| Triangle | PUNCH |
| Square | JUMP |
| Circle | KICK |
| Cross | ACTION |
| L1/R1/L2/R2 | как cfg0 |

---

## Аналоговый режим (DualShock)

`GHost.cpp::Host_VbRoutine()` (вызывается каждый VBlank):

```c
// Тип пада ID=7 → DualShock с аналоговыми стиками
if (PadInfoMode(0,InfoModeCurID,0)==7)
{
    padx = JoystickLeftX(&PAD_Input1) - 128;  // -128..127
    pady = JoystickLeftY(&PAD_Input1) - 128;

    if (padx < -96) PAD_Input1.data.pad &= ~PAD_LL;  // Left stick → D-Pad Left
    else if (padx > 96) PAD_Input1.data.pad &= ~PAD_LR;
    if (pady < -96) PAD_Input1.data.pad &= ~PAD_LU;
    else if (pady > 96) PAD_Input1.data.pad &= ~PAD_LD;
}
```

- **Порог:** ±96 (из ±128) — ~75% отклонения
- Аналог только эмулирует D-Pad (нет плавного движения)
- Правый стик не используется (камера на кнопках L2/R2)

---

## Вибрация (DualShock)

Только для DualShock (ID=7). Управляется через `psx_motor[2]`:

```c
// GHost.cpp ReadInputDevice():
PadSetAct(0x00, psx_motor, 2);     // установить моторы
psx_motor[0] >>= 1;               // малый мотор: делится на 2 каждый тик
psx_motor[1] = (psx_motor[1]*7)>>3;  // большой мотор: *7/8 каждый тик
```

- `psx_motor[0]` — малый мотор (быстрое затухание: ÷2 за тик)
- `psx_motor[1]` — большой мотор (медленное затухание: ×7/8 за тик)
- `vibra_mode` — вкл/выкл через меню (`WAD_VIB_ON/OFF`)
- `g_bEngineVibrations` — отдельный флаг для вибрации от двигателя машины (Dreamcast версия)

---

## PSX-специфичные различия от PC

### Камера (first-person pitch limits)
```c
// interfac.cpp ~7746
#ifdef PSX
    if (look_pitch >  128 && look_pitch <= 1024) {look_pitch =  128;}  // тесней
    if (look_pitch < 1900 && look_pitch >= 1024) {look_pitch = 1900;}
#else  // PC
    if (look_pitch >  256 && look_pitch <= 1024) {look_pitch =  256;}  // шире
    if (look_pitch < 1850 && look_pitch >= 1024) {look_pitch = 1850;}
#endif
```
PC — pitch range чуть шире (±128 vs ±256 относительно горизонта).

### Зона обзора частиц (DIRT_set_focus)
- PSX: радиус `0x800`
- PC: радиус `0xC00` (+50%)

### Мёртвая зона аналога
- PC geимпад: `NOISE_TOLERANCE = 8` (из 128)
- PSX аналог: `96` (из 128) — реализован через собственную проверку в Host_VbRoutine

### Инвентарь
PSX использует отдельные глобалы `PSX_inv_open/focus/count/select/timer` вместо PC-шных `PopupFade/ItemFocus`.
На PSX инвентарь открывается кнопкой Select, листается D-Pad, закрывается любой лицевой кнопкой.

