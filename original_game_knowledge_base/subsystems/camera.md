# Камера — Urban Chaos

**Ключевые файлы:**
- `fallen/Headers/fc.h` — FC_Cam структура, FC_MAX_CAMS
- `fallen/Source/fc.cpp` — "The final camera?", вся активная логика
- `fallen/Headers/cam.h` — МЁРТВЫЙ КОД (не компилируется, см. ниже)
- `fallen/Source/cam.cpp` — МЁРТВЫЙ КОД (`#ifdef DOG_POO` никогда не определён)

---

## 1. Единственная активная система: FC (Final Camera)

`cam.cpp` **никогда не компилируется** — весь файл обёрнут в `#ifdef DOG_POO` (строка 5), который нигде не определяется.
`Game.cpp` вызывает только `FC_init()` и `FC_process()`.

`FC_MAX_CAMS = 2` (fc.h строка 63) — две камеры для поддержки splitscreen (но кооп не реализован).

---

## 2. Структура FC_Cam (fc.h строки 15–61)

```c
typedef struct {
    // Цель камеры
    Thing *focus;           // NULL = камера не используется
    SLONG  focus_x;
    SLONG  focus_y;
    SLONG  focus_z;
    SLONG  focus_yaw;
    UBYTE  focus_in_warehouse;

    // Текущая позиция
    SLONG x, y, z;

    // Желаемая позиция (сглаживается)
    SLONG want_x, want_y, want_z;

    // Скорость перемещения
    SLONG dx, dy, dz;

    // Углы текущие
    SLONG yaw, pitch, roll;

    // Желаемые углы
    SLONG want_yaw, want_pitch, want_roll;

    // Параметры рендера
    SLONG lens;             // zoom фактор (hex fixed-point, например 0x24000)
    SLONG toonear;          // признак что объект слишком близко
    SLONG rotate;           // вращение
    SLONG nobehind;
    SLONG lookabove;
    UBYTE shake;            // тряска (от взрывов)

    // Дистанция и высота камеры
    SLONG cam_dist;
    SLONG cam_height;

    // Для smooth collision avoidance
    SLONG toonear_dist;
    SLONG toonear_x, toonear_y, toonear_z;
    SLONG toonear_yaw, toonear_pitch, toonear_roll;
    SLONG toonear_focus_yaw;
    SLONG smooth_transition;
} FC_Cam;

FC_Cam FC_cam[FC_MAX_CAMS];   // глобальный массив
```

---

## 3. Режимы камеры (FC_change_camera_type, fc.cpp строки 211–240)

| Тип | cam_dist (PC) | cam_height | Описание |
|-----|--------------|------------|----------|
| 0 (default) | `0x280 * 0.75F` ≈ 480 | `0x16000` | Стандартный от третьего лица |
| 1 | `0x280` = 640 | `0x20000` | Дальше и выше |
| 2 | `0x380` = 896 | `0x25000` | Ещё дальше |
| 3 | `0x300` = 768 | `0x8000` | Низкий угол |

**CAM_MORE_IN = 0.75F** (fc.cpp строка 37) — PC камера на 25% ближе к игроку, чем PSX версия.

**MARKS_MACHINE** — флаг компиляции для версии разработчика:
- Определён → `cam_height = 0x16000` при инициализации
- Не определён → `cam_height = 0x1a000` (выше)

---

## 4. Режим с оружием (gun-out)

```c
// fc.cpp строки 65-71
// #ifndef MARKS_PRIVATE_VERSION — активен в финальной версии
if (person_has_gun_out() && !driving && !biking) {
    *dheight = 0;     // камера не поднимается
    *ddist   = 200;   // абсолютная дополнительная дистанция отдаления
}
```

**MARKS_PRIVATE_VERSION** не определён в финале → gun-out режим активен.

---

## 5. Главный цикл (FC_process, fc.cpp строка ~1140)

Вызывается каждый кадр из `game_loop()`:
1. Проходит по `FC_cam[0..FC_MAX_CAMS-1]`
2. Пропускает камеры с `focus == NULL`
3. `FC_alter_for_pos()` — вычисляет смещение позиции по текущему состоянию
4. `FC_calc_focus()` — определяет точку взгляда
5. Обработка перехода в/из warehouse (здание)
6. Тряска от взрывов (поле `shake`)
7. Collision avoidance: raycast через MAV высоты — камера придвигается при пересечении стен
8. Сглаживание позиции и углов

---

## 6. Что переносить

- Всю FC логику (единственная активная система)
- FC_Cam с теми же полями (или эквивалент)
- Режимы 0-3 с теми же cam_dist / cam_height значениями
- CAM_MORE_IN = 0.75F для PC
- Gun-out смещение (+200 к dist)
- Collision avoidance (raycasting от игрока к позиции камеры)
- cam.cpp / cam.h — **не переносить**, мёртвый код
