# Навигация NPC (Navigation)

## Обзор: две системы

В игре **две параллельные навигационные системы**:

| Система | Файлы | Назначение | Алгоритм |
|---------|-------|-----------|---------|
| **MAV** | `mav.cpp`, `mav.h` | Основная навигация NPC | Greedy best-first на 128×128 сетке (не A* — нет g-cost!) |
| **NAV/Wallhug** | `Nav.cpp`, `Wallhug.cpp` | Вспомогательная/отладочная | Wallhug с оптимизацией |

**MAV — основная система.** NAV/Wallhug используется редко (возможно, только в отладочных случаях).

Дополнительно: `NavSetup.cpp` — настройка, `ns.cpp` — канализация (отдельная сетка, не переносится).

## MAV — основная система навигации

### Данные сетки

```cpp
// Навигационная карта: 128×128 ячеек
extern UWORD *MAV_nav;          // каждая ячейка = индекс в MAV_opt[]
extern SLONG  MAV_nav_pitch;    // = 128

// Макросы доступа (биты упакованы в UWORD):
#define MAV_NAV(x,z)   (MAV_nav[x*128+z] & 1023)   // 10 бит: индекс в MAV_opt
#define MAV_CAR(x,z)   ((MAV_nav[...]>>10) & 15)    // 4 бита: навигация машин
#define MAV_SPARE(x,z) (MAV_nav[...]>>14)           // 2 бита: вода и т.д.

// Таблица вариантов движения
extern MAV_Opt *MAV_opt;        // до 1024 записей
extern SLONG    MAV_opt_upto;

typedef struct { UBYTE opt[4]; } MAV_Opt;
// opt[0]=направление XS, opt[1]=XL, opt[2]=ZS, opt[3]=ZL
// каждый байт — битовое поле MAV_CAPS_*
```

### Действия движения (MAV_Action)

```cpp
typedef struct {
    UBYTE action;   // тип действия (0-7)
    UBYTE dir;      // направление (0-3: XS, XL, ZS, ZL)
    UBYTE dest_x;   // целевая ячейка X
    UBYTE dest_z;   // целевая ячейка Z
} MAV_Action;

#define MAV_ACTION_GOTO       0  // обычная ходьба
#define MAV_ACTION_JUMP       1  // прыжок на 1 блок
#define MAV_ACTION_JUMPPULL   2  // прыжок + подтягивание (1 блок)
#define MAV_ACTION_JUMPPULL2  3  // прыжок + подтягивание (2 блока)
#define MAV_ACTION_PULLUP     4  // подтягивание
#define MAV_ACTION_CLIMB_OVER 5  // перелезть через забор
#define MAV_ACTION_FALL_OFF   6  // упасть вниз
#define MAV_ACTION_LADDER_UP  7  // лестница вверх
```

**Флаги возможностей (MAV_CAPS_*):**
```cpp
#define MAV_CAPS_GOTO       0x01
#define MAV_CAPS_JUMP       0x02
#define MAV_CAPS_JUMPPULL   0x04
#define MAV_CAPS_JUMPPULL2  0x08
#define MAV_CAPS_PULLUP     0x10
#define MAV_CAPS_CLIMB_OVER 0x20
#define MAV_CAPS_FALL_OFF   0x40
#define MAV_CAPS_LADDER_UP  0x80
#define MAV_CAPS_DARCI 0xff     // она умеет всё
```

### Greedy best-first поиск пути (MAV_do)

**Важно: это НЕ классический A\*.** В MAV_do нет накопленного g-cost (стоимости пройденного пути). Используется только эвристика расстояния до цели — т.е. greedy best-first search. Находит путь быстрее A\*, но не гарантирует оптимальный маршрут.

**Входные данные:** стартовая ячейка, целевая ячейка.
**Выходные данные:** первое действие из пути (MAV_Action).
**Горизонт:** 32 ячейки (MAV_LOOKAHEAD = 32).

**Эвристика (MAV_score_pos):**
```cpp
UBYTE MAV_score_pos(x, z) {
    SLONG dist = QDIST2(abs(dest_x-x)<<1, abs(dest_z-z)<<1);
    return min(dist, 255);
}
```

**Алгоритм:**
1. Очистить флаги посещённых ячеек в радиусе горизонта
2. Инициализировать priority queue (256 записей), добавить стартовую позицию
3. Цикл:
   - Взять лучшую позицию из очереди
   - Если длина >= 32 И score лучший из найденных → запомнить путь, возможно вернуть
   - Если позиция == цель → вернуть путь
   - Для каждого из 4 направлений, для каждого действия (GOTO, JUMP, JUMPPULL...):
     - Применить случайное смещение (+0–3, потом +3 для не-GOTO) для естественности
     - Если ячейка не посещена → добавить в очередь
4. При переполнении: до 8 попыток (MAV_MAX_OVERFLOWS) → вернуть лучший частичный путь

**Реконструкция пути:**
- Посещённые ячейки хранят действие и направление прихода: `MAV_flag[128][64]` (4 бита/ячейка), `MAV_dir[128][32]` (2 бита/ячейка)
- `MAV_create_nodelist_from_pos()` — трассировка назад от цели к старту, заполняет `MAV_node[]`
- `MAV_get_first_action_from_nodelist()` — возвращает **первое не-GOTO действие**, или GOTO как можно дальше

**Память:**
- MAV_opt: ≤1024 × 4 байта = 4 КБ
- MAV_nav: 128×128 × 2 байта = 32 КБ
- MAV_flag: 128×64 = 8 КБ
- MAV_dir: 128×32 = 4 КБ

### Предрасчёт MAV (MAV_precalculate)

Вызывается один раз при загрузке уровня. Для каждой ячейки (x,z):

1. **Высота:** `MAVHEIGHT(x,z)` = `PAP_hi[x][z].Height` (SBYTE, 0x40 = 1 блок)
2. **Поиск лестниц** в радиусе: `find_nearby_ladder_colvect_radius()`
3. **Для каждого из 4 направлений:**

   | Разница высот (dh) | Возможные действия |
   |------------------|------------------|
   | 0–2, нет забора | MAV_CAPS_GOTO |
   | 0–2, есть забор | MAV_CAPS_CLIMB_OVER |
   | < 0 (падение), нет забора | MAV_CAPS_FALL_OFF |
   | < 0, есть забор | MAV_CAPS_CLIMB_OVER |
   | 2–4, наверх | MAV_CAPS_PULLUP |
   | если рядом лестница | MAV_CAPS_LADDER_UP |
   | 1–2, нет прямого пути | MAV_CAPS_JUMP |
   | 2–5 | MAV_CAPS_JUMPPULL |
   | > 0 на 2 клетки | MAV_CAPS_JUMPPULL2 |

4. **Особые случаи:**
   - Prim 41 (ступенька): нет пересечения по узким лестницам
   - Скользкие поверхности (наклон > 100 ед.) → навигация отключена
   - Вода: бит MAV_SPARE_FLAG_WATER (текстуры 454, 99456, 99457)

### Динамическое изменение навигации

```cpp
MAV_turn_movement_off(x, z, dir)  // отключить направление (закрытая дверь)
MAV_turn_movement_on(x, z, dir)   // включить (открытая дверь)
MAV_turn_off_whole_square(x, z)   // блокировать всю ячейку
```

Используется системой дверей: дверь открылась → включить движение; закрылась → выключить.

### LOS для навигации

`MAV_can_i_walk(ax,az, bx,bz)` — **path shortcutting**: проверяет, можно ли идти по прямой между двумя ячейками.

**Алгоритм:**
1. `dx = (bx-ax)<<4`, `dz = (bz-az)<<4` — разница в subpixel единицах
2. `dist = QDIST2(|dx|,|dz|)` → нормализует: `dx = dx * (0x4000/dist) >> 8` → длина ~0x40
3. Начинает с центра ячейки (a<<8 + 0x80), шагает по dx/dz каждую итерацию
4. При пересечении границы ячейки (MAV_dmx или MAV_dmz ≠ 0):
   - Проверяет `MAV_CAPS_GOTO` в нужном направлении из MAV_opt[]
   - **Диагональный случай** (оба dmx и dmz ≠ 0): проверяет **ещё 2 угловые ячейки** (corner cells)
5. Возвращает TRUE если достигли bx/bz; при FALSE → `MAV_last_mx/mz` = последняя успешная ячейка

**Только GOTO**: проверяет исключительно `MAV_CAPS_GOTO`. Прыжки/climb/pull-up — только cell-by-cell.
**Используется в**: `MAV_get_first_action_from_nodelist()` для оптимизации пути (пропуска промежуточных waypoints).

Два варианта LOS по высоте:
- `MAV_height_los_fast()` — только MAV_height карта
- `MAV_height_los_slow()` — также `WARE_inside()` для складских интерьеров

### Навигация на крышах

Крыши — **обычные MAV-ячейки с высоким MAVHEIGHT**. Специального режима нет.

`MAV_calc_height_array()` строит MAVHEIGHT grid:
- Обычная ячейка: `MAVHEIGHT = PAP_calc_height_at(center) / 0x40` (terrain height)
- Ячейка с `PAP_FLAG_HIDDEN` И roof walkable face → `MAVHEIGHT = roof_face.Y / 0x40` (высота крыши здания)
- Ячейка с `PAP_FLAG_HIDDEN` БЕЗ roof face → `MAVHEIGHT = -127` → затем `= 127 + PAP_FLAG_NOGO` (недоступно)
- WAREHOUSE тип → дополнительно устанавливает бит 15 в PAP_Hi.Texture для warehouse nav

MAV_precalculate строит рёбра между roof-ячейками так же как между ground-ячейками (dh check, LOS, climb/jump).
**Скайлайты** убираются из навигации: prim 226 (skylight) → удаляет 2 roof-face записи; prim 227 (large skylight) → 2×3 блок.

## NAV/Wallhug — вспомогательная система

### NAV_Waypoint (8 байт)

```cpp
typedef struct {
    UWORD x;     // X координата
    UWORD z;     // Z координата
    UBYTE flag;  // NAV_WAYPOINT_FLAG_LAST (1<<0), NAV_WAYPOINT_FLAG_PULLUP (1<<1)
    UBYTE length; // оставшиеся вэйпойнты
    UWORD next;   // индекс следующего
} NAV_Waypoint;
```

### Алгоритм Wallhug (Wallhug.cpp, 644 строки)

Работает на 2D сетке 128×128 (те же размеры что MAV). Координаты UBYTE (0-127). 4 кардинальных направления, без диагоналей. `wallhug_path` содержит до 252 вэйпойнтов (`wallhug_waypoint` = {UBYTE x, UBYTE y}).

**Основной алгоритм (wallhug_continue_trivial):**

1. **Прямая линия:** Bresenham от start к end
2. **При встрече стены:** записать текущую позицию как waypoint, запустить двух huggers:
   - hugger[0]: handed=-1 (left-hand rule), начальный поворот +1 от направления стены
   - hugger[1]: handed=+1 (right-hand rule), начальный поворот -1 от направления стены
3. **wallhug_hugstep():** каждый шаг hugger'а:
   - Стена впереди → повернуть в сторону противоположную руке (`-handed`), записать waypoint
   - Нет стены впереди → шагнуть вперёд; если нет стены со стороны руки → повернуть к руке (`+handed`)
4. **Условия отпускания стены** (все 4 должны выполниться):
   - Цель не за плоскостью hugged wall
   - Hugger смотрит в сторону цели (цель не за спиной)
   - Ближе к цели чем точка начала обхода (по обеим осям, не дальше end)
   - Не на стартовой позиции
5. **Победитель:** первый hugger с `dirn==WALLHUG_DONE`; его путь добавляется в основной path
6. **Провал:** оба hugger.dirn==WALLHUG_FAILED_DIRN, или huggers встретились (face-to-face в соседних ячейках), или достигнут max_count

**Обёртки:**
- `wallhug_trivial(path)` — инициализирует path.length=0 и вызывает continue_trivial с WALLHUG_MAX_COUNT
- `wallhug_tricky(path)` — trivial + wallhug_cleanup (оптимизация)

**Постобработка (wallhug_cleanup):**

1. `line_of_sight_cleanup(path, first_wp)`: для каждого waypoint проверить LOS к waypoint+1..+MAX_LOOKAHEAD(4); если есть → пропустить промежуточные. Повторять пока есть удаления.
2. Для каждой пары соседних waypoints (c1, c1+1): навигировать между ними через `wallhug_trivial()`. Если новый путь НЕ содержит c1 → заменить c1 более коротким путём. После замены → goto line_of_sight_stuff (повторить LOS).
3. Safety limit: max 10 итераций верхнего цикла.

**line_of_sight():** Bresenham от start к end, проверяя WALLHUG_WALL_IN_WAY на каждом шаге.

**Константы:**
- WALLHUG_MAX_COUNT = 20000 (макс. шагов)
- WALLHUG_MAX_PTS = 252 (макс. вэйпойнтов в пути)
- MAX_LOOKAHEAD = 4 (для LOS-оптимизации)
- Направления: NORTH=0, EAST=1, SOUTH=2, WEST=3
- WALLHUG_FAILED = 0x70000000 (маркер провала)
- WALLHUG_WALL_IN_WAY(x,y,dir) → NAV_wall_in_way() (определён в wallhug.h)

## Какую систему выбирает NPC?

По контексту персонажа:
- **На улице** → `MAV_do()` (внешняя 2D-навигация)
- **Внутри склада** → `WARE_mav_*()` (отдельный граф для каждого этажа)
- **Внутри здания (INSIDE2)** → `INSIDE2_mav_*()` (см. ниже — в пре-релизе почти не работает!)
- **В канализации** → отдельная навигация ns.cpp (не переносится)

Поле в Person: `MAV_Action MA` — текущее действие; `MAV_Action pcom_move_ma` — командованное действие.

## Навигация складов (WARE_mav_*)

Склады (`FLAG_PERSON_WAREHOUSE`) имеют **отдельную nav-сетку** (`WARE_nav[]`). Паттерн везде одинаков — **swap указателя MAV_nav**:

```
MAV_nav = &WARE_nav[ww->nav]   // подменяем глобальный nav на склад
MAV_nav_pitch = ww->nav_pitch
MAV_do(...)                    // запускаем стандартный поиск
MAV_nav = old_mav              // восстанавливаем глобальный nav
```

Три функции:

| Функция | Что делает |
|---------|-----------|
| `WARE_mav_enter(p, ware, caps)` | НЕ меняет MAV_nav. Ищет ближайшую дверь (Manhattan) и вызывает глобальный MAV_do() к `door[i].out_x/z` (точка снаружи). Если уже у двери (dist==0) → возвращает GOTO к `door[i].in_x/z` |
| `WARE_mav_inside(p, dest_x, dest_z, caps)` | Swap MAV_nav + перевод в локальные coords (вычитает `ww->minx/minz`) → MAV_do() → обратный перевод + restore |
| `WARE_mav_exit(p, caps)` | Swap MAV_nav. Ищет ближайшую дверь **с проверкой высоты**: `\|height_out - height_in\| < 0x80` (чтобы не выходить через дверь другого этажа!) → MAV_do() внутри склада к `door[i].in_x/z` |

Предрасчёт: `MAV_precalculate_warehouse_nav(ware)` вызывается при инициализации склада (`ware.cpp:431`). Правила навигации внутри склада чуть проще — нет `both_ground` проверки, max dh = 1 всегда.

## Навигация INSIDE2-зданий (INSIDE2_mav_*) — ПРЕ-РЕЛИЗ БАГИ

**⚠️ В пре-релизе навигация внутри INSIDE2-зданий ПОЧТИ НЕ РАБОТАЕТ.**

`INSIDE2_mav_nav_calc(inside)` — строит 2D nav-сетку здания:
- Размер: до 32×32 ячеек (`INSIDE2_MAX_NAV = 1024`), хранится в `INSIDE2_mav_nav[UWORD]`
- Хранит прямой 4-битный bitmask (0-15): биты 0-3 = можно ли идти в каждое из 4 направлений
- Работает через первые 16 записей `MAV_opt[]` которые гарантированно соответствуют bitmask 0-15
- **БАГ:** цикл по Z использует `is->MinX..MaxX` вместо `is->MinZ..MaxZ` → nav-сетка заполнена неправильно!

`INSIDE2_setup_mav()` / `INSIDE2_restore_mav()` — swap MAV_nav на INSIDE2_mav_nav (аналогично складам).

| Функция | Статус |
|---------|--------|
| `INSIDE2_mav_enter(p, inside, caps)` | ✅ Работает — глобальный MAV_do() к ближайшей двери (балл: Manhattan + dY>>5) |
| `INSIDE2_mav_inside(p, inside, x, z)` | ❌ `ASSERT(0)` — полная заглушка! NPC не умеет ходить внутри здания |
| `INSIDE2_mav_stair(p, inside, new_inside)` | ❌ Баг: `> 16` вместо `>> 16` → возвращает текущую позицию (GOTO на месте) |
| `INSIDE2_mav_exit(p, inside)` | ❌ Баг: `> 16` вместо `>> 16` → то же самое |

**Вывод:** в пре-релизе NPC может подойти к зданию (`mav_enter`), но зайдя внутрь — застрянет. Вероятно, в финальной версии эти функции были дописаны.

## Динамическое обновление MAV при изменении уровня

**Только двери — точечные обновления:**
```cpp
// door.cpp: при открытии двери
MAV_turn_movement_on(mx, mz, left);   // включить проход в направлении left
MAV_turn_movement_on(mx, mz, right);  // и в противоположном

// door.cpp: при закрытии двери
MAV_turn_movement_off(mx, mz, left);
MAV_turn_movement_off(mx, mz, right);
```

Поведение функций:
- `MAV_turn_movement_off(mx, mz, dir)` — очищает **только** бит `MAV_CAPS_GOTO` у данного направления (остальные caps — JUMP, PULLUP и т.д. — не трогает)
- `MAV_turn_movement_on(mx, mz, dir)` — **заменяет** весь набор caps данного направления на `MAV_CAPS_GOTO` (только ходьба, всё остальное теряется!)

**Взрывы стен и другие разрушения НЕ обновляют MAV** — `MAV_precalculate()` не вызывается во время игры. Только при загрузке уровня и через отладочный хоткей Shift+J в дебаг-сборке.

Склады: `MAV_precalculate_warehouse_nav()` вызывается при инициализации склада, не во время игры.

## Избегание толпы

**Отдельной системы нет.** Реализовано через:
- `InWay` поле — если кто-то блокирует путь, NPC пытается обойти
- Bash-in-way логика: если блокирующий — цель, атаковать

## Масштаб координат

- 1 ячейка MAV = 256 мировых единиц (сдвиг на 8 бит)
- Высота: 0x40 = 1 блок = 64 единицы SBYTE-шкалы
- Карта 128×128 ячеек = 32768×32768 мировых единиц

## Важно для переноса

- MAV — неявный граф (не хранит рёбра явно, вычисляет из MAV_opt на лету)
- MAV ищет только 32 шага вперёд (greedy BFS, не A*), затем возвращает лучший частичный путь — NPC вызывает MAV_do() повторно
- Случайное смещение при выборе действий (+0–3) намеренно добавляет непредсказуемость маршрутов
- Предрасчёт MAV_precalculate() выполняется каждый раз при загрузке уровня (~серьёзная нагрузка)
- INSIDE2 навигация: в финале нужно дописать mav_inside/stair/exit с нуля (заглушки в пре-релизе)
- Склады: паттерн MAV_nav swap (save → redirect → use MAV_do → restore) — применять для всех спецсред
- Взрывы стен: в финале решить нужна ли динамическая перестройка MAV (в оригинале — нет)
