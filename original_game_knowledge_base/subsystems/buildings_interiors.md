# Здания и интерьеры (Buildings & Interiors)

## Иерархия данных зданий

```
FBuilding  (до 500 зданий)
  └─ FStorey × StoreyCount  (до 2500, linked list via StoreyHead)
       └─ FWall × N  (до 15000, linked list via WallHead)
            └─ FWindow × M  (до 50 типов)
```

При компиляции (загрузка уровня) преобразуется в:
```
DBuilding  (до 1024)
  └─ DFacet × N  (до 16384 граней, диапазон FacetStart-FacetEnd)
```

Файлы: `fallen/Source/Building.cpp`, `fallen/Source/door.cpp`, `fallen/Source/stair.cpp`, `fallen/Headers/building.h`, `fallen/Headers/inside2.h`, `fallen/Headers/supermap.h`

## FBuilding — все поля

```cpp
struct FBuilding {
    UWORD ThingIndex;       // связанный Thing
    UWORD LastDrawn;        // GAME_TURN последнего рисования
    UBYTE Dummy2;           // выравнивание
    UBYTE Foundation;       // тип/высота фундамента
    SWORD OffsetY;          // смещение Y от земли
    UWORD InsideSeed;       // seed процедурной генерации интерьера
    UBYTE MinFoundation;    // минимальная высота фундамента
    UBYTE ExtraFoundation;  // дополнительная высота
    UWORD BuildingFlags;    // флаги состояния здания
    SWORD StoreyHead;       // индекс первого этажа (linked list)
    SWORD Angle;            // угол поворота здания
    UWORD StoreyCount;      // количество этажей
    CBYTE str[20];          // имя/идентификатор
    UBYTE StairSeed;        // seed расположения лестниц
    UBYTE BuildingType;     // тип: 0=HOUSE, 1=WAREHOUSE, 2=OFFICE, ...
    UWORD Walkable;         // проходимая поверхность (флаги)
    UWORD Dummy[4];         // выравнивание
};
```

## FStorey — все поля

```cpp
struct FStorey {
    SWORD DX, DY, DZ;       // размеры/смещение этажа
    UBYTE StoreyType;       // тип (см. STOREY_TYPE_* ниже)
    UBYTE StoreyFlags;      // флаги состояния
    UWORD Height;           // высота этажа в мировых единицах
    SWORD WallHead;         // индекс первой стены (linked list)
    UWORD ExtraFlags;       // дополнительные флаги
    UWORD InsideIDIndex;    // индекс в системе ID комнат
    SWORD Next, Prev;       // двусвязный список этажей
    SWORD Info1;            // дополнительное поле
    SWORD Inside;           // связанный внутренний этаж
    UWORD BuildingHead;     // обратная ссылка на здание
    UWORD InsideStorey;     // индекс в InsideStorey[]
};
```

## Полный список STOREY_TYPE_* (21 тип)

| ID | Имя | Назначение |
|----|-----|-----------|
| 1 | STOREY_TYPE_NORMAL | Обычный этаж со стенами |
| 2 | STOREY_TYPE_ROOF | Крыша |
| 3 | STOREY_TYPE_WALL | Стенная геометрия |
| 4 | STOREY_TYPE_ROOF_QUAD | Четырёхугольная крыша |
| 5 | STOREY_TYPE_FLOOR_POINTS | Точки высоты пола |
| 6 | STOREY_TYPE_FIRE_ESCAPE | Пожарная лестница (внешняя) |
| 7 | STOREY_TYPE_STAIRCASE | Лестница (внешняя) |
| 8 | STOREY_TYPE_SKYLIGHT | Световой фонарь |
| **9** | **STOREY_TYPE_CABLE** | **Система тросов (смерть-слайд)** — есть в финале! |
| 10 | STOREY_TYPE_FENCE | Забор |
| 11 | STOREY_TYPE_FENCE_BRICK | Кирпичный забор |
| 12 | STOREY_TYPE_LADDER | Лестница (вертикальная) |
| 13 | STOREY_TYPE_FENCE_FLAT | Плоский забор |
| 14 | STOREY_TYPE_TRENCH | Траншея/яма |
| 15 | STOREY_TYPE_JUST_COLLISION | Только коллизия (невидимая) |
| 16 | STOREY_TYPE_PARTITION | Внутренняя перегородка |
| 17 | STOREY_TYPE_INSIDE | **Внутренний пол/стена** |
| 18 | STOREY_TYPE_DOOR | Внутренняя дверь |
| 19 | STOREY_TYPE_INSIDE_DOOR | Дверь между интерьерами |
| 20 | STOREY_TYPE_OINSIDE | Другой интерьер (второй проход) |
| **21** | **STOREY_TYPE_OUTSIDE_DOOR** | **Дверь из улицы в интерьер** |

## Представление интерьеров

**Трёхуровневая система:**

### Уровень 1: Редактор/генерация (FStorey)
- `FStorey.StoreyType == 17/19/20` → этаж имеет интерьер
- `FStorey.InsideIDIndex` → планировка комнат (RoomID)
- `FStorey.InsideStorey` → индекс в `inside_storeys[]`

### Уровень 2: Runtime (InsideStorey)

```cpp
struct InsideStorey {
    UBYTE MinX, MinZ;    // мин. граница
    UBYTE MaxX, MaxZ;    // макс. граница
    UWORD InsideBlock;   // индекс в inside_block[] данных
    UWORD StairCaseHead; // связанные лестницы
    UWORD TexType;       // стиль текстур интерьера
    UWORD FacetStart;    // первый DFacet этажа
    UWORD FacetEnd;      // последний DFacet
    SWORD StoreyY;       // Y-координата
    UWORD Building;      // родительское здание
    UWORD Dummy[2];
};
```

### Уровень 3: Коллизии/рендеринг (DFacet)

```cpp
struct DFacet {
    UBYTE FacetType;     // тип (STOREY_TYPE_*)
    UBYTE Height;        // высота грани
    UBYTE x[2];          // X-диапазон ячеек
    SWORD Y[2];          // мировые Y (низ, верх)
    UBYTE z[2];          // Z-диапазон ячеек
    UWORD FacetFlags;    // FACET_FLAG_* (см. ниже)
    UWORD StyleIndex;    // стиль/материал текстуры
    UWORD Building;      // родительский DBuilding
    UWORD DStorey;       // ссылка на DStorey
    UBYTE FHeight;       // высота грани
    UBYTE BlockHeight;   // высота блока
    UBYTE Open;          // для двери: степень открытия (0-255)
    UBYTE Dfcache;       // кеш ночного освещения
    UBYTE Shake;         // повреждение забора
    UBYTE CutHole;       // дыра в грани
    UBYTE Counter[2];    // счётчик обработки (double-buffer)
};
```

## Как игра определяет "внутри"

**Метод: фильтрация по FacetType**

1. Рядом с `STOREY_TYPE_OUTSIDE_DOOR` (тип 21) → активируется `FStorey.InsideStorey`
2. Система рендеринга переключается:
   - Снаружи: показывать DFacet с типами 1–16, 21
   - Внутри: показывать DFacet с типами 17, 19, 20
3. `ID_should_i_draw(x, z)` — проверяет видимость ячейки из текущей комнаты через RoomID

**RoomID — планировка комнат:**
```cpp
struct RoomID {
    UBYTE X[16];      // X-координаты комнат (до 16)
    UBYTE Y[16];      // Z-координаты
    UBYTE Flag[16];   // флаги валидности
    UBYTE StairsX[10], StairsY[10];   // позиции лестниц
    UBYTE StairFlags[10];  // FLAG_INSIDE_STAIR_UP | STAIR_DOWN
    UBYTE FloorType;  // стиль интерьера
};
```

## Флаги граней (FACET_FLAG_*)

| Флаг | Бит | Назначение |
|------|-----|-----------|
| FACET_FLAG_OPEN | 1<<12 | Дверь открывается |
| FACET_FLAG_90DEGREE | 1<<13 | Открывается на 90° вместо 180° |
| FACET_FLAG_INSIDE | 1<<3 | Внутренняя грань |
| FACET_FLAG_UNCLIMBABLE | 1<<8 | Нельзя залезть |
| FACET_FLAG_SEETHROUGH | 1<<11 | Прозрачный забор (нельзя пролезть) |
| FACET_FLAG_ELECTRIFIED | 1<<6 | Электрифицированный забор |
| FACET_FLAG_BARB_TOP | 1<<10 | Колючая проволока сверху |
| FACET_FLAG_2SIDED | 1<<7 | Рендер с обеих сторон |
| FACET_FLAG_FENCE_CUT | 1<<15 | Дырка в заборе |
| FACET_FLAG_DLIT | 1<<4 | Динамическое освещение |
| FACET_FLAG_HUG_FLOOR | 1<<5 | Фальшивый забор у пола |
| FACET_FLAG_CABLE | 1<<2 | Система тросов |
| FACET_FLAG_IN_SEWERS | 1<<4 | В канализации |

## Система дверей

**До 4 дверей анимируются одновременно (DOOR_MAX_DOORS = 4).**

```cpp
struct DOOR_Door { UWORD facet; };  // NULL = не используется
```

**Открытие двери:**
1. `DOOR_find(world_x, world_z)` — поиск ближайшей двери (STOREY_TYPE_OUTSIDE_DOOR)
2. Установить `FACET_FLAG_OPEN` на DFacet
3. Добавить в `DOOR_door[]` очередь анимации
4. Включить MAV-навигацию: `MAV_turn_movement_on()` для соседних ячеек

**Анимация (`DOOR_process()` — вызывается каждый кадр):**
- Без `FACET_FLAG_90DEGREE`: `Open` 0 → 255 (180°), шаг +4/кадр
- С `FACET_FLAG_90DEGREE`: `Open` 0 → 128 (90°), шаг +4/кадр
- При достижении максимума: снять FACET_FLAG_OPEN, дверь "открыта"

**Закрытие:** обратно уменьшать `Open`, при 0 → `MAV_turn_movement_off()`.

## Система лестниц

**Автоматическое размещение.** Структура:
```cpp
struct STAIR_Stair {
    UBYTE flag;     // STAIR_FLAG_UP | STAIR_FLAG_DOWN
    UBYTE up;       // целевой InsideStorey при движении вверх
    UBYTE down;     // при движении вниз
    UBYTE x1,z1, x2,z2;  // занимает 2 соседние ячейки сетки
};
// До 32 лестниц, до 3 на этаж (STAIR_MAX_PER_STOREY)
```

**Алгоритм размещения (STAIR_calculate) — детали:**
1. Перемешать порядок этажей (Fisher-Yates на STAIR_MAX_STOREYS)
2. Для каждой пары этажей с `abs(height_i - height_j) == 1` (смежные):
   - AND bitset ячеек двух этажей → общие допустимые позиции
   - Попытаться переиспользовать существующую лестницу на каком-либо этаже
   - Если не нашли — перебрать все пары (x1,z1)→(x2,z2) в 4 направлениях
3. **Scoring** новой позиции:
   - Base: `STAIR_rand() & 0xffff` (случайность всегда)
   - `outside==2` (оба квадрата у внешней стены): `+0x10000`
   - `outside==1` (один квадрат у стены): `-0x10000`
   - `outside==0` (внутри): `+0` (нейтрально)
   - Координата совпадает с краем bounding box: `+0x5000` (до ×4)
   - Напротив "opposite wall" hint: `+0xbabe` за каждый квадрат
   - Блокирует внешнюю дверь: `-INFINITY`
4. `best_score >= 0` → создать пару лестниц на обоих этажах (STAIR_FLAG_UP + STAIR_FLAG_DOWN)

**Геометрия лестниц:** тип STOREY_TYPE_STAIRCASE, физика интерполирует Y между значениями `DFacet.Y[2]`.

## Система дорог (Road)

**Граф узлов на основе текстур:**
- Текстуры 323–356 (PC) = дорога; 333–334 = зебра
- 4 типа: TARMAC(0), GRASS(1), DIRT(2), SLIPPERY(3)
- До 256 узлов (ROAD_MAX_NODES), до 4 связей на узел

**Построение:** `ROAD_wander_calc()` сканирует карту, находит центры дорог (радиус 5 ячеек), строит граф прямых сегментов.

**Навигация машин:**
1. `ROAD_find(wx,wz, &n1, &n2)` — найти ближайший сегмент
2. `ROAD_get_dest(n1, n2, ...)` — позиция в своей полосе (ROAD_LANE от центра)
3. На перекрёстке `ROAD_whereto_now()` — случайный выбор (вперёд: score 20+rand, назад: score 10)
4. Спаун машин у краёв карты: `ROAD_find_me_somewhere_to_appear()` (расстояние > 0x1000 от игрока)

**Бордюры:** `ROAD_sink()` снижает крайние вершины дорог, создавая перепад.

## Процедурная генерация фасадов

Стены с `FLAG_WALL_AUTO_WINDOWS`:
- Окна расставляются автоматически по формуле: `count = dist / (BLOCK_SIZE * 4)`
- Паттерн: стена → окно → стена → окно...
- 5 текстурных фрагментов: LEFT, MIDDLE, RIGHT, MIDDLE1, MIDDLE2

**Нет повреждения зданий!** `Shake` в DFacet — только для заборов. `CutHole` — прорезание дыры в заборе. Здания неуязвимы.

## Лимиты

| Константа | Значение |
|-----------|---------|
| MAX_BUILDINGS | 500 |
| MAX_STOREYS | 2500 |
| MAX_WALLS | 15000 |
| MAX_INSIDE_STOREYS | 1024 |
| MAX_DFACETS | 16384 |
| MAX_DBUILDINGS | 1024 |
| BLOCK_SIZE (1 ячейка сетки) | 64 мировых единицы |
| DOOR_MAX_DOORS | 4 одновременно |
| ROAD_MAX_NODES | 256 |
| STAIR_MAX_STAIRS | 32 |

## Рендеринг интерьеров (id.cpp)

**Файл:** `fallen/Source/id.cpp` (~9300 строк)

`id.cpp` = "interior display" — рендеринг 3D геометрии изнутри зданий.
Работает только когда игрок находится внутри здания (STOREY_TYPE_INSIDE_*).

### Pipeline id.cpp

```
ID_set_outline()                → задать периметр текущего внутреннего пространства
ID_calculate_in_squares()       → определить что внутри/снаружи (flood-fill)
ID_calculate_in_points()        → вычислить вершины геометрии
ID_generate_floorplan()         → мастер-оркестратор: генерация всей интерьерной геометрии
  ├── ID_find_rooms()           → топология комнат
  ├── ID_find_flats()           → плоские поверхности
  ├── ID_create_mapsquare_faces()  → генерация граней (стена/окно/дверь/верх)
  ├── ID_generate_inside_walls() → процедурные внутренние стены
  ├── ID_make_connecting_doors() → двери между комнатами
  ├── ID_assign_room_types()    → назначение типов комнат
  └── ID_score_layout_house_ground() → оценка планировки (дом/склад/офис)
ID_draw()                       → рендеринг через D3D
```

### Runtime функции

```c
ID_change_floor(storey)      // переключить на другой этаж (при смене этажа)
ID_collide_2d(x, z, r)      // 2D коллизия внутри здания (стены комнат)
ID_calc_height_at(x, z)     // высота пола/потолка в точке
ID_wall_colvects_insert()   // добавить коллизионные векторы для текущего интерьера
ID_wall_colvects_remove()   // убрать коллизионные векторы при выходе
```

### Структуры id.cpp

```c
// Точка в интерьере
struct ID_Point {
    SLONG x, z;       // мировые координаты
    UBYTE flags;      // corner, interior, edge
};

// Грань (полигон стены/пола/потолка)
struct ID_Face {
    UBYTE type;       // wall / window / door / floor / ceiling
    UBYTE flags;      // walkable, dlit и т.д.
};

// Ячейка сетки в интерьере
struct ID_Square {
    UBYTE inside;     // TRUE если эта ячейка внутри
    UBYTE room_id;    // к какой комнате относится
};
```

### Архитектурные особенности

- id.cpp генерирует **процедурную** геометрию интерьера — не берёт готовые полигоны из данных
- Геометрия регенерируется при изменении этажа (ID_change_floor)
- `ID_get_face_texture()` — маппинг тип-грани → D3D texture page
- Рендеринг в `ID_draw()` использует D3D5 API

## ID_generate_floorplan() — полный флоу

```
сигнатура: SLONG ID_generate_floorplan(storey_type, stair[], num_stairs, seed, find_good_layout, furnished)
возвращает: seed_used (UWORD) или -1 при ошибке
```

1. Валидация: нет нулевых стен → иначе return -1
2. `ID_calculate_in_squares()` — flood-fill inside/outside ячеек по периметру стен
3. `ID_calculate_in_points()` — определить внутренние точки; fail → return -1
4. Разметить лестницы: `ID_FLOOR(stair[i].x,z)->flag |= ID_FLOOR_FLAG_STAIR`
5. Подсчёт `ID_floor_area` (кол-во inside-ячеек)
6. Поиск лучшей планировки:
   - `find_good_layout==TRUE`: пробует **32 сида** (ID_MAX_FITS), каждый раз:
     - `ID_generate_inside_walls(storey_type)` → расставляет внутренние стены
     - `ID_score_layout(storey_type)` → оценивает; `ID_clear_inside_walls()` → сброс
     - Выбирает seed с `best_score >= 0`; если нет таких → return -1
   - `find_good_layout==FALSE`: использует данный seed напрямую
7. Генерация граней: `ID_add_room_faces(i)` для каждой комнаты (или `ID_add_wall_faces` если `YOU_WANT_THIN_WALLS`)
8. Если `furnished`: `ID_place_furniture()`

**ID_generate_inside_walls()** — расстановка внутренних стен:
- `num_walls = ID_floor_area >> 4 + 1` (≈1 стена на 16 ячеек)
- Apartement: `num_walls = ID_floor_area >> 3 + 2` (более плотно)
- Apartement: фиксированные стартовые точки у лестничных углов → коридор

**ID_find_rooms()** — BFS flood-fill по комнатам:
- Queue размером 64 элемента (QUEUE_SIZE)
- Распространение в 4 стороны, остановка если флаг `ID_FLOOR_FLAG_WALL_XL/XS/ZL/ZS`
- Каждая связная компонента → отдельный room ID (1-based)

**ID_score_layout_house_ground()** scoring:
- Per room: ratio = max(dx,dz)/min(dx,dz) × 256; цель = 414 (золотое сечение)
- Score += `(150 + (414 - ratio)) * 10` (отрицательный для сильно вытянутых!)
- `ID_ROOM_FLAG_RECTANGULAR` → +1000
- Комната доступна только из одной другой → `score = score * 3 / 4`
- Лестница в туалете → `score -= score/2`

**ID_score_layout_warehouse()** scoring (другая логика):
- biggest_room_area << 8 (приоритет большому помещению)
- + room_count << 10 (много комнат тоже хорошо)
- Слишком вытянутые комнаты → штраф

**ID_assign_room_types()** — bubble sort по размеру, затем:
- HOUSE/OFFICE: biggest→LOUNGE; lobby→LOBBY; по размеру: LOO(1-й), KITCHEN(2-й), DINING(3-й), LOUNGE(4-й+)
- WAREHOUSE: biggest→WAREHOUSE; lobby(с дверью)→LOBBY; LOO(1-й), OFFICE(середина), MEETING(biggest среди остатков, если ≥3)

## WARE_init() — структура складской системы

**WARE_Ware** (max 32 складов, `WARE_MAX_WARES=32`):
```cpp
struct WARE_Ware {
    struct { UBYTE out_x, out_z, in_x, in_z; } door[4]; // внешняя/внутренняя сторона двери (mapsquare)
    UBYTE door_upto;      // кол-во дверей (0-4)
    UBYTE minx, minz, maxx, maxz;  // bounding box (UBYTE, inclusive)
    UBYTE nav_pitch;      // = bz2-bz1 (шаг по Z для nav/height индексации)
    UWORD nav;            // индекс в WARE_nav[] (UWORD per cell, MAV-данные)
    UWORD building;       // DBuilding index
    UWORD height;         // индекс в WARE_height[] (SBYTE per cell, MAVHEIGHT)
    UWORD rooftex;        // индекс в WARE_rooftex[] (UWORD per roof_face4)
    UBYTE ambience;       // ambient звук внутри склада
};
```

**Пулы (allocator-style):**
- `WARE_nav[4096]` — nav-данные всех складов подряд
- `WARE_height[8192]` — высоты ячеек
- `WARE_rooftex[4096]` — текстуры крыши (из roof_faces4[])

**Инициализация (WARE_init()):**
1. Загрузить `.map` файл (меняет расширение `.iam` → `.map`): rooftop textures
2. `MAV_calc_height_array(TRUE)` — высоты без крыш складов (для определения OBs inside)
3. Для каждого `BUILDING_TYPE_WAREHOUSE`:
   - `WARE_bounding_box()` → minx/z/maxx/z
   - PAP_Hi.HIDDEN squares → `PAP_LO_FLAG_WAREHOUSE` (на PAP_Lo >>2)
   - Двери = DFacets с `STOREY_TYPE_DOOR`: out = center + perpendicular, in = center - perpendicular
   - Аллоцировать `nav_memory = (bx2-bx1)*(bz2-bz1)` слотов в WARE_nav + WARE_height
   - `MAV_precalculate_warehouse_nav()` — вычислить MAV внутри склада
   - Заполнить WARE_height из `MAVHEIGHT(x,z)` для каждой ячейки
   - Скопировать rooftop textures из `roof_faces4[]` в `WARE_rooftex[]`
4. OBs с `y+maxy < MAVHEIGHT*64-0x20` → `OB_FLAG_WAREHOUSE` (= объект внутри)
5. `MAV_calc_height_array(FALSE)` — восстановить высоты с крышами
6. **Размещение door prims закомментировано** (`#if 0`)

---

## Важно для переноса

- Интерьеры — **не отдельная геометрия**, а те же DFacets с типами 17/19/20. Рендер переключается фильтрацией.
- Двери анимируются через `Open` поле (0–255), синхронизированы с MAV-навигацией.
- STOREY_TYPE_CABLE (тип 9) — тросы/death-slide — есть в финальной игре, переносить.
- Нет движущихся платформ в геймплее (platformSetup.cpp = только редактор).
- Дороги определяются текстурами, а не явными данными — при переносе нужна та же логика определения типа поверхности.
