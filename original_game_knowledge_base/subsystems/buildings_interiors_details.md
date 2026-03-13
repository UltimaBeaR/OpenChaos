# Здания и интерьеры — детали (id.cpp, WARE)

**Связанный файл:** [buildings_interiors.md](buildings_interiors.md) — структуры, двери, лестницы, дороги

---

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

---

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

---

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
