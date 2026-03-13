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

    // Текущая позиция (реальная, что рендерится)
    SLONG x, y, z;

    // Желаемая позиция (сглаживается к реальной)
    SLONG want_x, want_y, want_z;

    // Скорость перемещения
    SLONG dx, dy, dz;

    // Углы текущие
    SLONG yaw, pitch, roll;

    // Желаемые углы
    SLONG want_yaw, want_pitch, want_roll;

    // Параметры рендера
    SLONG lens;             // zoom фактор (hex fixed-point, 0x28000*CAM_MORE_IN)
    SLONG toonear;          // TRUE = камера слишком близко к стене (clip mode)
    SLONG rotate;           // орбитальное вращение (от кнопок rotate)
    SLONG nobehind;         // счётчик "не за спиной" (активен при rotate)
    SLONG lookabove;        // вертикальный сдвиг точки взгляда (0xa000 в норме)

    // Дистанция и высота камеры
    SLONG cam_dist;
    SLONG cam_height;

    // Для smooth collision avoidance (toonear state)
    SLONG toonear_dist;
    SLONG toonear_x, toonear_y, toonear_z;
    SLONG toonear_yaw, toonear_pitch, toonear_roll;
    SLONG toonear_focus_yaw;
    SLONG smooth_transition;

    UBYTE shake;            // тряска от взрывов (decays каждый кадр)
} FC_Cam;

FC_Cam FC_cam[FC_MAX_CAMS];   // глобальный массив
```

---

## 3. Режимы камеры (FC_change_camera_type, fc.cpp строки 233–262)

| Тип | cam_dist (PC) | cam_height | Описание |
|-----|--------------|------------|----------|
| 0 (default) | `0x280 * 0.75F` ≈ 480 | `0x16000` | Стандартный от третьего лица |
| 1 | `0x280` = 640 | `0x20000` | Дальше и выше |
| 2 | `0x380` = 896 | `0x25000` | Ещё дальше |
| 3 | `0x300` = 768 | `0x8000` | Низкий угол (кинематографичный) |

**PSX type 0:** cam_dist = 0x300, cam_height = 0x18000 (дальше, чем PC).

**CAM_MORE_IN = 0.75F** (fc.cpp строка 37) — PC камера на 25% ближе к игроку, чем PSX версия.

---

## 4. FC_alter_for_pos — модификаторы offset_dist и offset_height

Вызывается каждый кадр перед FC_calc_focus(). Возвращает два значения:
- `offset_dist` — множитель к cam_dist (256 = 1.0x)
- `offset_height` — добавка к высоте камеры

```
Режим с оружием (gun-out, активен в финале):
    → dheight=0, ddist=200    // меньше дистанция, ниже высота

В машине (InCar, типы 0/4/5/6/8):
    → dheight=0, ddist=356    // одинаково для всех машин

Движение (STATE != STATE_IDLE):
    → dheight=0, ddist=256    // стандарт при движении (=1.0x)

Стояние (STATE_IDLE):
    → высотный анализ по MAVHEIGHT направления камеры
    → но: код обнаружения обрыва (`&& 0` в строке 180) = МЁРТВЫЙ КОД
    → реально всегда: dheight=0, ddist=256
```

---

## 5. FC_calc_focus — вычисление точки взгляда

### 5a. focus_yaw (направление "за кем следит")

**Пешком:**
- Базовое значение: `Draw.Tweened->Angle` (направление персонажа)
- `ACTION_SIDE_STEP / ACTION_SIT_BENCH / ACTION_HUG_WALL`: yaw += 1024 (смотреть в лицо)
- `SUB_STATE_ENTERING_VEHICLE`:
  - `ANIM_ENTER_TAXI`: yaw -= 712 (садится слева)
  - иначе: yaw += 712 (садится справа)
- `STATE_DANGLING` (кабель, смерть-слайд, traverse): динамически — камера выбирает сторону, смотрит сбоку (±550 от текущего yaw)
- `GF_SIDE_ON_COMBAT`: yaw -= 512 (вид сбоку в бою)

**В машине:**
- yaw_car = Vehicle->Angle (+ WheelAngle коррекция при движении)
- Если Velocity < 0 (задний ход): yaw_car += 1024 (разворот)

### 5b. focus_x/y/z (позиция точки взгляда)

Выбирается через `FC_get_person_body_part_target()` по Action:
- `CAM_AT_HEAD (1)` — позиция `SUB_OBJECT_HEAD` (отдельная субмодель)
- `CAM_AT_FEET (3)` — среднее между SUB_OBJECT_LEFT_FOOT и RIGHT_FOOT (по Y)
- `CAM_AT_WORLD_POS (2)` — просто WorldPos

Специальные случаи:
- `SUB_STATE_HUG_WALL_LOOK_L / LOOK_R`: focus смещается по углу (peek из-за угла), `InsideRoom` = расстояние смещения

---

## 6. FC_focus_above — высота камеры над фокусом

Базовое значение = `cam_height`.

Корректировки (вычитаются из `lower`, применяются как `above -= lower`):

| Ситуация | Эффект |
|----------|--------|
| Gun-out | lower = 0xa000 (камера ниже) |
| InCar type 4 (грузовик?) | lower = -0x3000*CAM_MORE_IN (камера выше — смотрит вверх) |
| InCar types 0/5/6/8 | lower = -0x1000*CAM_MORE_IN (немного выше) |
| InCar default | lower = 0xa000*CAM_MORE_IN (ниже) |
| InsideIndex != 0 (здание) | cam_height * 1.5 (выше — обзор интерьера) |
| Splitscreen (FC_cam[1].focus) | above -= 0x4000 (ниже, т.к. меньше экрана) |

---

## 7. FC_process — главный алгоритм обновления

Вызывается каждый кадр из `game_loop()` (только при `should_i_process_game()`).

### Шаги:

```
1. FC_alter_for_pos()       → offset_height, offset_dist

2. FC_calc_focus()          → focus_x/y/z, focus_yaw, focus_in_warehouse

3. Warehouse transition
   → если changed: EWAY_cam_jumped=10
   → вход:  FC_setup_camera_for_warehouse() — особая позиция камеры
   → выход: FC_setup_initial_camera()       — стандартная позиция

4. lookabove update
   → STATE_DEAD/DYING: lookabove убывает (−0x80/кадр), камера опускается
   → иначе: lookabove = 0xa000 (фикс. смещение вверх)

5. Rotate (орбитальное вращение — от кнопок Rotate Left/Right)
   → rotate decays ±0x80/кадр
   → пока rotate != 0: want_x/z смещаются по орбите + nobehind = 0x2000

6. Toonear cancel check
   → dist = QDIST от want до focus
   → если dist > toonear_dist + 0x4000 И dangle > 200: toonear = FALSE
   → восстановить toonear_* позицию, smooth_transition = TRUE

7. Collision detection (8 шагов от want к focus)
   → Outside:
      - Первые 4 шага: проверка MAV_inside по Y (подтолкнуть вверх)
      - Все 8 шагов: проверка стен (MAV_inside ±X, ±Z) + заборов (MAV_get_caps + LOS)
      - Аккумулируем xforce, yforce, zforce
   → Inside warehouse:
      - Проверка WARE_inside (основа)
      - WARE_get_caps для стен и потолка
   → Применить: want_x/y/z += force << 4

8. Get-behind (только если !toonear)
   → behind = focus + SIN/COS(focus_yaw) * cam_dist*offset_dist
   → dx/dz = behind - want_x/z
   → Скорость сближения:
      - Entering vehicle: >> 3 (быстро)
      - Driving: >> 5 (медленно — плавность)
      - Рядом со стеной (xforce||zforce): >> 6 + >> 5
      - Нормально: >> 3
      - Gun-out добавляет >> 4 дополнительно в любом случае

9. Adjust Y (высота камеры)
   → goto_y = focus_y + FC_focus_above() + offset_height
   → dy = goto_y - want_y; dy >>= 3
   → Cap при нормальных условиях: dy ∈ [-0x0c00, +0x0d00]
   → Cap при большом отставании (dy > 0x10000): dy capped at 0x2000
   → На движущейся платформе (FACE_FLAG_WMOVE): dy <<= 5 (мгновенно)

10. Distance clamp
    → FC_DIST_MIN = FC_DIST_MAX = cam_dist * offset_dist
    → dist < min: push away from focus (want -= normalized_dir * delta)
    → dist > max: pull toward focus (want += normalized_dir * delta)

11. Position smoothing (want → actual)
    → if |delta| > 0x800:
       x += dx >> 2,  y += dy >> 3,  z += dz >> 2
    → иначе: snap (want = actual, smooth_transition = FALSE)

12. FC_look_at_focus() — вычислить yaw/pitch/roll для взгляда на фокус

13. Angle smoothing
    → dyaw/dpitch/droll = want - actual (wrapped to ±1024 units)
    → if |total_delta| > 0x180: actual += delta >> 2
    → иначе: snap

14. Lens
    → fc->lens = 0x28000 * CAM_MORE_IN  (всегда, FOV не меняется)

15. Shake
    → shake_x/y/z = random(shake) - shake/2  →  actual_pos += shake << 7
    → shake -= 1;  shake -= shake >> 3  (exponential decay)
```

---

## 8. Toonear режим (камера за стеной)

Когда камера упирается в стену (`toonear = TRUE`):
- Позиция камеры фиксируется у стены
- `toonear_dist = 0x90000` → first-person mode (камера у головы персонажа)
- Отмена: когда focus ушёл достаточно далеко И повернулся — snap к toonear_* координатам

---

## 9. Shake (тряска от взрывов)

`FC_explosion()` вызывается при взрыве рядом с камерой:
- `fc->shake` устанавливается пропорционально `force`
- Каждый кадр: случайное смещение × 128 юнитов
- Decay: `shake -= 1 + shake >> 3` (экспоненциальный)

---

## 10. FC_can_see_person / FC_can_see_point

Используются для проверки видимости NPC из камеры (AI awareness?).
Реализованы в fc.cpp строки 1964–2090.

---

## 11. FC_allowed_to_rotate (fc.cpp строки 991–1072)

Проверяет, можно ли вращать камеру в данном направлении. Используется перед FC_rotate_left/right().

