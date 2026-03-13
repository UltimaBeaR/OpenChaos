# Физика — детальные подсистемы

**Связанный файл:** [physics.md](physics.md) (ядро: координаты, коллизии, движение, гравитация)

---

## 5c. Вода (PAP_FLAG_WATER)

Вода в Urban Chaos — **просто невидимые коллизионные стены** по периметру.

**Флаги и поля:**
- `PAP_FLAG_WATER` — в `PAP_Hi.Flags` для водяных hi-res ячеек
- `PAP_FLAG_SINK_POINT` — устанавливается на водяной ячейке и трёх соседях; используется системой луж/дождя и az.cpp
- `MAV_SPARE_FLAG_WATER` — в MAV_nav SPARE bits [15:14] для навигации AI
- `PAP_Lo.water` (SBYTE) — высота воды в Lo-res единицах; `PAP_LO_NO_WATER = -127` = нет воды

**Инициализация при загрузке уровня** (elev.cpp ~2044-2083):
1. Все `PAP_Lo[x][z].water = PAP_LO_NO_WATER (-127)`
2. Для каждой `PAP_Hi` с `PAP_FLAG_WATER`: `PAP_2LO(x>>2, z>>2).water = water_level >> 3`
   - `water_level = -0x80 = -128` — **хардкодированная** локальная переменная в `EL_load_map()`
   - Итоговое значение: `-128 >> 3 = -16` (в Lo-res ед.)
3. Объекты (OB) в водяных ячейках переставляются: `oi->y = water_level - prim->miny = -128 - miny`

**Коллизия — активный код** (`stop_movement_between()`, collide.cpp ~10027):
- Если одна ячейка `PAP_FLAG_WATER`, а соседняя — нет → невидимый DFacet (стена) вдоль ребра
- Персонажи/транспорт не могут войти в воду — блокируются невидимой стеной

**`OB_height_fiddle_de_dee()` = МЁРТВЫЙ КОД** — функция обёрнута в `/* */`, никогда не выполняется.

**Нет:** замедления в воде, плавания, утопания, динамического изменения уровня.
**Есть:** огонь гасится в воде (Person.cpp:4414) — PYRO_IMMOLATE сбрасывается при PAP_FLAG_WATER.

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

**Важно:** нельзя просто добавить гравитацию как вектор — нужно воспроизвести state-machine логику падения точно.

### Для транспорта — явная гравитация

```c
// vehicle.cpp, line 104
#define GRAVITY (-(UNITS_PER_METER * 10 * 256) / (TICKS_PER_SECOND * TICKS_PER_SECOND))
// = -(128 * 10 * 256) / (80 * 80) = -51 юнит/тик² (integer division)
// TICKS_PER_SECOND = 80 (20 * TICK_LOOP)
```

При нахождении в воздухе: `VelY += GRAVITY` каждый тик.
При контакте с землёй: VelY демпируется через систему подвески.

---

## 6b. Тайминг и физические константы

```c
// Game.h
#define NORMAL_TICK_TOCK  (1000/15)  // 66.67 мс — базовый тик (15 FPS логики)
#define TICK_SHIFT        8
// TICK_RATIO — динамический: (реальное_мс_кадра << 8) / 66.67
// При норме ≈ 256 (1.0x), при медленном ≤ 512 (2.0x), при быстром ≥ 128 (0.5x)
// Сглаживание: 4-кадровая скользящая средняя (SmoothTicks)
// Slow-motion режим: TICK_RATIO = 32 (~12.5% скорости)
```

```c
// Vehicle.cpp
#define TICK_LOOP        4            // под-итераций подвески за кадр
#define TICKS_PER_SECOND (20*TICK_LOOP)  // = 80 — только для формул констант физики

// Подвеска: count = TICK_LOOP; while (--count) {...} → цикл 3 раза за кадр

// Frame rate cap (Game.cpp, lock_frame_rate()):
// Spin-loop busy-wait: while(GetTickCount() - tick1 < 1000/fps) {}
// fps из config.ini: max_frame_rate=30 (default)
```

**Важно:** движение и физика умножаются на `TICK_RATIO >> TICK_SHIFT` — frame-rate independence. Clamp 0.5x–2.0x.

---

## 7. Физика транспорта

Подробности: **`vehicles.md`** — подвеска (4 пружины), двигатели, ограничения скорости.
Гравитация транспорта: `GRAVITY = -51 юн/тик²` (integer division) — см. раздел 6 выше.

---

## 8. Лимиты массивов (PC vs PSX)

| Массив | PC | PSX |
|--------|----|-----|
| MAX_COL_VECT_LINK | 10 000 | 4 000 |
| MAX_COL_VECT | 10 000 | 1 000 |
| MAX_WALK_POOL | 30 000 | 10 000 |
| WMOVE_MAX_FACES (=RWMOVE_MAX_FACES) | 192 | 192 |

---

## 9. HyperMatter — spring-lattice физика мебели

**Файл:** `fallen/Source/hm.cpp` (~2000 строк, детально аннотирован)

**Ключевые факты:**
- Собственная разработка MuckyFoot (НЕ Criterion middleware)
- Только для мебели (бочки, ящики, стулья) — не для персонажей и транспорта
- Spring-lattice: 3D решётка масс-точек + пружины, Euler integration, 13 пружин/точку
- `HM_process()`: spring forces → bump forces → position update → cleanup
- **`gy = 0` hardcoded!** Объекты отскакивают от Y=0, НЕ от реального terrain
- Баги: `already_bumped[i]` вместо `[j]` (некоторые HM-HM столкновения пропускаются)
- `HM_stationary(threshold=0.25)` → "усыпляет" объект когда почти неподвижен
- `FURN_hypermatterise()` очищает `FLAG_FURN_DRIVING` при активации HM

---

## 9b. WMOVE — движущиеся ходимые поверхности

**Файл:** `fallen/Source/wmove.cpp`, `fallen/Headers/wmove.h`

Система позволяет персонажам стоять на крышах движущихся машин и платформ.

**Принцип:** для каждого транспорта/платформы создаётся виртуальная walkable face (PrimFace4 с FACE_FLAG_WMOVE), которая движется вместе с объектом.

**Лимит:** `WMOVE_MAX_FACES = 192`

**Количество faces per object** (`WMOVE_get_num_faces()`):
- CLASS_PERSON, CLASS_PLAT: 1
- VAN/AMBULANCE/WILDCATVAN: 1
- JEEP/MEATWAGON: 4
- CAR/TAXI/POLICE/SEDAN: 5

**PSX ограничение:** на PSX только фургоны (VAN/WILDCATVAN/AMBULANCE) создают WMOVE faces.

**Жизненный цикл:**
```
WMOVE_create(thing)   // при создании vehicle/platform
WMOVE_process()       // в Thing.cpp каждый кадр: update positions + re-attach to map
WMOVE_remove(class)   // при очистке уровня
```

**WMOVE_relative_pos()** — как персонажи следуют за surface:
- Вычисляет позицию в базисе **прошлого** положения face
- Переводит в **новый** базис → new_x, new_y, new_z
- `dangle` = поворот face за кадр → добавляется к углу персонажа
- Threshold: `|dy| < 0x200` → dy=0 (не скользить вверх/вниз)

---

## 9c. Известные баги и хаки в коде

1. **HM_collide_all bug** (hm.cpp): `already_bumped[i]` вместо `[j]` — некоторые HM-HM столкновения пропускаются. Практически не заметно из-за быстрого "засыпания" объектов.

2. **Координатный хак** (collide.cpp ~2753): *"shift is here to mimic code in predict_collision_with_face()"* — ручная подгонка координат.

3. **Fall-through-roof workaround** (move_thing ~7061): *"Get rid of that nasty fall-through-the-roof bug"* — персонаж мог падать сквозь крышу.

4. **DeltaVelocity** в Thing.h (line 112-115) — поле существует, использование неясно. Возможно незавершённая физическая система.

5. **SWORD для скорости** — 16-bit ограничение (-32K..+32K). При высоких скоростях возможно clipping.

---

## 10. Углы и вращение

- Угловые значения: 0–2047 (11-bit, 2048 = 360°)
- Нет float-углов в физике
