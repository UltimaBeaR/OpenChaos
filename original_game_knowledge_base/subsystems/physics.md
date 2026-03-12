# Физика и коллизии — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/collide.cpp` — главный файл физики (~11K строк)
- `fallen/Headers/collide.h` — структуры коллизий
- `fallen/Headers/pap.h` — heightmap структуры
- `fallen/Source/maths.cpp` — математика
- `fallen/Source/Vehicle.cpp` — физика транспорта

---

## 1. Система координат

**Базовый тип — GameCoord:**
```c
struct GameCoord {
    SLONG X;  // 32-bit signed
    SLONG Y;  // Высота
    SLONG Z;  // Глубина
};
```

**Важнейшие сдвиги (shifts):**

| Операция | Назначение |
|----------|-----------|
| `>> 8` | Блок → юнит (делит на 256) |
| `>> 16` | Юнит → индекс mapsquare |
| `<< 8` | Юнит → блок (умножает на 256) |
| `<< 3` (PAP_ALT_SHIFT) | Высота → хранение (×8) |
| `>> 3` | Хранение → высота (÷8) |
| `>> TICK_SHIFT` | Скорость → позиция (интеграция по времени) |

**Размеры карты:**
- PAP_SIZE_HI: 128×128 hi-res mapsquares
- PAP_SIZE_LO: 32×32 lo-res блоков
- PAP_SHIFT_HI: 8 бит (256 блоков на hi-res квадрат)
- ELE_SIZE: 256 юнитов = 1 mapsquare

**Важно:** В физике НЕ используются float — всё на 32-bit integer (SLONG). Это обеспечивает детерминизм между платформами.

---

## 2. Структуры данных коллизий

**CollisionVect** (collide.h, line 35-44):
```c
struct CollisionVect {
    SLONG X[2];      // X-координаты начала и конца отрезка
    SWORD Y[2];      // Высота (16-bit)
    SLONG Z[2];      // Z-координаты начала и конца
    UBYTE PrimType;  // Тип примитива (стена, забор и т.д.)
    UBYTE PrimExtra; // Доп. информация о типе
    SWORD Face;      // К какому face относится
};
```
- Массив: `col_vects[MAX_COL_VECT]` = 10 000 записей на PC (1 000 на PSX)
- Каждый — это линейный отрезок-барьер в 3D пространстве
- Память: ~300 KB

**CollisionVectLink** (collide.h, line 29-33):
```c
struct CollisionVectLink {
    UWORD Next;       // Linked list → следующий линк
    UWORD VectIndex;  // Индекс в col_vects[]
};
```
- Массив: `col_vects_links[MAX_COL_VECT_LINK]` = 10 000 (PC) / 4 000 (PSX)
- Привязывает CollisionVect к mapsquare (spatial hash)
- Память: ~40 KB

**WalkLink** (collide.h, line 47-51):
```c
struct WalkLink {
    UWORD Next;  // Linked list → следующая ссылка
    SWORD Face;  // Индекс walkable-поверхности (позитивный = обычная, негативный = крыша)
};
```
- Массив: `walk_links[MAX_WALK_POOL]` = 30 000 (PC) / 10 000 (PSX)
- Карта по mapsquare → список walkable-поверхностей
- Память: ~120 KB

---

## 3. Heightmap — структуры карты высот

**PAP_Hi** (pap.h, line 86-93) — 8 байт на ячейку:
```c
typedef struct {
    UWORD Texture;  // ID текстуры
    UWORD Flags;    // Shadow, reflective, roof флаги
    SBYTE Alt;      // Высота (хранится: actual_height >> 3)
    SBYTE Height;   // Запас/расширение
} PAP_Hi;
// PAP_Hi[128][128] = 131 KB
```
- Реальная высота: `Alt << 3` (range: -1024 .. +1016 юнитов)

**PAP_Lo** (pap.h, line 69-77) — lo-res данные:
```c
typedef struct {
    UWORD MapWho;       // Здание/terrain в этом блоке
    SWORD Walkable;     // >0 = обычная face, <0 = крыша
    UWORD ColVectHead;  // Голова linked list коллизионных векторов
    SBYTE water;        // Высота воды (-127 = нет воды)
    UBYTE Flag;         // Склад (warehouse) и т.д.
} PAP_Lo;
// PAP_Lo[32][32]
```

**MapElement** (Map.h) — 14 байт:
```c
struct MapElement {
    LIGHT_Colour Colour;
    SBYTE        AltNotUsedAnyMore;  // устарело
    SWORD        Walkable;
    THING_INDEX  MapWho;             // Голова linked list объектов в ячейке
};
```

---

## 4. Функции получения высоты

**5 вариантов функций — для разных контекстов:**

1. **`PAP_calc_height_at_point(map_x, map_z)`** — высота в угловой точке mapsquare. Прямой lookup без интерполяции.

2. **`PAP_calc_height_at(x, z)`** — интерполированная высота. Билинейная интерполяция между 4 угловыми точками. Input: block-level координаты.

3. **`PAP_calc_height_at_thing(thing*, x, z)`** — с учётом контекста Thing. Проверяет находится ли Thing внутри здания, перенаправляет в `WARE_calc_height_at()` или `NS_calc_height_at()`. Для улицы — вызывает `PAP_calc_height_at()`.

4. **`PAP_calc_map_height_at(x, z)`** — максимум из terrain и зданий.

5. **`PAP_calc_height_noroads(x, z)`** — высота без учёта бордюров. Ищет максимальную высоту в окрестности. Используется для игнорирования curbs.

**Логика plant_feet() — как определяется на чём стоит персонаж:**
1. Вычислить позицию ступней в мировых координатах
2. `find_face_for_this_pos()` — найти walkable-поверхность
3. Если face не найден → `PAP_calc_height_at_thing()`
4. Сравнить высоту поверхности с Y-позицией персонажа
5. Если разница > 30 юнитов → падение начинается
6. Если разница > 60 юнитов на face → большое падение (переход в STATE_FALLING)

---

## 5. Движение и коллизии

### move_thing() — основное движение
```c
ULONG move_thing(SLONG dx, SLONG dy, SLONG dz, Thing *p_thing)
```
Полноценное движение с коллизиями для CLASS_PERSON.

**Поток выполнения:**
1. Извлечь начальную позицию из `p_thing->WorldPos`
2. Вычислить целевую позицию: x2 = x1 + dx, etc.
3. Проверить границы карты (128×128), отвергнуть выход за пределы
4. `collide_against_things()` — коллизии с другими персонажами
5. `collide_against_objects()` — коллизии с мебелью/объектами
6. Если на face (walkable платформе):
   - Вызвать `slide_along()` с `extra_wall_height = -0x50`
   - Радиус зависит от состояния (HUG_WALL уменьшает)
   - Флаги: CRAWL, CARRYING, JUMPING
7. `slide_along_edges()` / `slide_along_redges()` — скольжение по рёбрам
8. Проверка перехода с одной face на другую: `find_face_for_this_pos()`
9. Если нет face → `FALL_OFF_FLAG_TRUE`, персонаж падает
10. `move_thing_on_map()` — обновить позицию

### move_thing_quick() — быстрое движение
```c
ULONG move_thing_quick(SLONG dx, SLONG dy, SLONG dz, Thing *p_thing)
{
    new_position = p_thing->WorldPos + delta;
    move_thing_on_map(p_thing, &new_position);
    return 0;
}
```
- **Без коллизий**, без скольжения
- Телепортация с обновлением карты
- Используется для ragdoll, knockback, катсцен

### slide_along() — алгоритм скольжения
```c
SLONG slide_along(
    SLONG x1, SLONG y1, SLONG z1,     // Начальная позиция
    SLONG *x2, SLONG *y2, SLONG *z2,  // Целевая позиция (изменяется in-place!)
    SLONG extra_wall_height,           // Корректировка высоты стены
    SLONG radius,                      // Радиус коллизии
    ULONG flags)                       // CRAWL, CARRYING, JUMPING
```

**Глобальные переменные (sic!):**
```c
SLONG actual_sliding = FALSE;   // Было ли столкновение?
SLONG last_slide_colvect;       // Последняя задетая стена
SLONG fence_colvect = 0;        // Коллизия с забором
```

**Алгоритм:**
1. Вычислить bounding box движения
2. Перебрать все mapsquares в bounding box
3. Для каждого mapsquare — проверить все linked CollisionVect
4. При обнаружении коллизии — спроецировать движение вдоль нормали стены
5. `actual_sliding = TRUE`
6. Вернуть количество коллизий (0 = нет, >0 = есть)

**ВАЖНО:** Нет отскока (restitution = 0). Только чистое скольжение вдоль стены. Нет трения при скольжении.

**Типы тестов коллизий:**
- `check_vect_circle()` — сфера против линейного отрезка
- `slide_around_box()` — движущийся box против отрезка (транспорт)
- `slide_around_sausage()` — capsule против отрезка (цилиндрические объекты)
- `mount_ladder()` — специальный кейс для лестниц

```c
#define DONT_INTERSECT 0
#define DO_INTERSECT   1
#define COLLINEAR      2
```

---

## 6. Гравитация

### Для персонажей — гравитация НЕ физическая, она animation-driven

**Не существует** явного вектора гравитации применяемого к персонажам!

**Как это работает:**
1. `plant_feet()` определяет высоту под персонажем
2. При разнице > 60 юнитов → переход в `STATE_FALLING`
3. В состоянии падения: анимационный handler уменьшает DY каждый фрейм (~-20/фрейм)
4. **DY** хранится в `Thing` как `SWORD` (16-bit signed)
5. При приземлении: детектируется через `height_above_anything()`

**Это важно при портировании:** нельзя просто добавить гравитацию как вектор — нужно воспроизвести state-machine логику падения точно.

### Для транспорта — явная гравитация

```c
// vehicle.cpp, line 104
#define GRAVITY (-(UNITS_PER_METER * 10 * 256) / (TICKS_PER_SECOND * TICKS_PER_SECOND))
// = -(128 * 10 * 256) / (80 * 80) = -5120 юнит/тик²
// TICKS_PER_SECOND = 80 (20 * TICK_LOOP)
```

При нахождении в воздухе: `VelY += GRAVITY` каждый тик.
При контакте с землёй: VelY демпируется через систему подвески.

---

## 7. Физика транспорта

**Скорости** (vehicle.h):
```c
SLONG VelX;  // продольная скорость
SLONG VelY;  // вертикальная скорость
SLONG VelZ;  // поперечная скорость
SLONG VelR;  // угловая скорость (рыскание/yaw)
```

**Подвеска** (vehicle.h):
```c
typedef struct {
    UWORD Compression;  // Сжатие пружины
    UWORD Length;       // Длина подвески
} Suspension;

Suspension Spring[4];  // По одной на каждое колесо
SLONG DY[4];           // Вертикальное смещение каждого колеса
```

**Константы подвески:**
- `MIN_COMPRESSION = 13 << 8 = 3328`
- `MAX_COMPRESSION = 115 << 8 = 29440`

**Параметры двигателей:**
```c
// FwdAccel, BkAccel, SoftBrake, HardBrake
#define ENGINE_LGV  17, 10, 4, 8   // Лёгкий грузовик
#define ENGINE_CAR  21, 10, 4, 8   // Обычная машина
#define ENGINE_PIG  25, 15, 5, 10  // Полицейская (быстрее)
#define ENGINE_AMB  25, 10, 5, 8   // Скорая помощь
```

**Ограничения скорости:**
```c
#define VEH_SPEED_LIMIT    750  // ~35 mph
#define VEH_REVERSE_SPEED  300  // Задний ход
```

**Расположение колёс (базы):**
- VAN: ±120, -150±165
- CAR: ±85, -120±160

---

## 8. Лимиты массивов (PC vs PSX)

| Массив | PC | PSX |
|--------|----|-----|
| MAX_COL_VECT_LINK | 10 000 | 4 000 |
| MAX_COL_VECT | 10 000 | 1 000 |
| MAX_WALK_POOL | 30 000 | 10 000 |
| RWMOVE_MAX_FACES | 192 | 192 |

---

## 9. Известные баги и хаки в коде

1. **Координатный хак** (collide.cpp, line ~2753):
   Комментарий: *"shift is here to mimic code in predict_collision_with_face()"*
   — ручная подгонка координат для совпадения двух функций.

2. **Fall-through-roof** (move_thing, line ~7061):
   Активный workaround: *"Get rid of that nasty fall-through-the-roof bug"*
   — персонаж мог падать сквозь крышу.

3. **Несколько методов интерполяции высоты** — разные функции для одной задачи,
   указывает на edge-cases в terrain system.

4. **DeltaVelocity** в Thing.h (line 112-115) — поле существует, использование неясно.
   Возможно незавершённая физическая система.

5. **SWORD для скорости** — 16-bit ограничение (-32K..+32K). При высоких скоростях возможно clipping.

---

## 10. Углы и вращение

- Угловые значения: 0–2047 (11-bit, 2048 = 360°)
- Нет float-углов в физике

---

## 11. Что важно воспроизвести 1:1 при портировании

**Критично (влияет на геймплей):**
1. Точные пороги переходов: 30 юнитов (начало падения), 60 юнитов (большое падение)
2. Алгоритм slide_along — точное скольжение без отскока
3. Система face detection — `find_face_for_this_pos()`
4. Физика подвески транспорта — точные формулы сжатия пружин
5. Гравитация транспорта: -5120 юнит/тик²
6. Анимационно-управляемое падение персонажей (не физическая гравитация!)
7. Все координатные сдвиги (>> 8, >> 16 и т.д.)

**Можно переосмыслить:**
- Linked-list spatial hash → можно заменить на современную структуру (quadtree/octree)
  **НО** — при этом важно сохранить точное поведение collision detection

**Решение по float:**
- Оригинал — чисто integer (SLONG)
- В новой версии: при использовании float — обязательно `-mfpmath=sse -msse2` для строгой 32-bit IEEE 754 точности
- Рассмотреть: сохранить integer-физику для детерминизма и точного воспроизведения
