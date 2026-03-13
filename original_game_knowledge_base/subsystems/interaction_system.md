# Система взаимодействия — Grab / Ladder / Cable (interact.cpp)

**Файл:** `fallen/Source/interact.cpp` (~900+ строк)
**Связанные:** physics.md (коллизии), controls.md (zipwire), characters.md (анимации), buildings_interiors.md (DFacet)

---

## 1. find_grab_face() — главная функция поиска поверхностей для хватания

**Сигнатура:**
```c
SLONG find_grab_face(x, y, z, radius, dy, angle,
    *grab_x, *grab_y, *grab_z, *grab_angle,
    ignore_building, trench, *type, person)
```

**Двухпроходный поиск:**
- Pass 0: hi-res (roof faces, hidden roofs) — для передвижения по крышам
- Pass 1: lo-res (walkable floor faces) — для обычного terrain

**Алгоритм для каждого walkable face:**
1. Проверка 4 рёбер квада
2. Y-диапазон: ±256 + dy от игрока
3. Угол к поверхности: ±200° от направления игрока
4. Расстояние до ребра: < radius
5. Не слишком близко к земле (иначе нет смысла хватать)
6. Не проходит через fence (`does_fence_lie_along_line()`)

**Возвращает:** face index + grab position (x,y,z) + grab angle; или `type`:
- type=0 — обычная поверхность
- type=1 — cable (трос/zipwire)
- type=2 — ladder (лестница)

**Канализация (PC-only):** `find_grab_face_in_sewers()` — упрощённый вариант, проверяет 4 соседних mapsquare, возвращает GRAB_SEWERS

---

## 2. Cable/Zipwire физика

**Параметры троса закодированы в полях DFacet:**
```c
SWORD angle_step1 = (SWORD)p_facet->StyleIndex;  // кривизна первого сегмента
SWORD angle_step2 = (SWORD)p_facet->Building;     // кривизна второго сегмента
UBYTE count       = p_facet->Height;               // количество шагов
```
Это **переиспользование полей** DFacet — StyleIndex и Building содержат параметры провисания троса.

**find_cable_y_along():** рассчитывает высоту троса в точке:
- Два сегмента (от краёв к середине)
- Косинусная кривая провисания (dip)
- Линейная интерполяция Y между конечными точками

**check_grab_cable_facet():**
- Находит ближайшую точку на отрезке троса
- Рассчитывает провисание (cable dip)
- Если в пределах radius + вертикальный допуск → grab OK

**Связь с controls.md:** После grab → `FLAG_PERSON_ON_CABLE`, sub-states для скольжения/траверса

---

## 3. Ladder (лестница) механика

**check_grab_ladder_facet():**
- Проверяет расстояние до центральной линии лестницы
- Высотный диапазон: `bottom ≤ Y ≤ top + 64`
- Возвращает **-1** если игрок ВЫШЕ лестницы (ещё падает → не хвататься преждевременно)
- Возвращает **1** если валидная позиция для хватания

**mount_ladder()** (в collide.cpp): подробно в physics.md секция 5b

**ПРЕ-РЕЛИЗ баг:** `mount_ladder()` из interfac.cpp закомментирован → игрок не может залезть снизу. AI (pcom.cpp:12549) может.

---

## 4. Позиционирование костей анимации

`calc_sub_objects_position()` (3 варианта):
- Вычисляет мировую позицию кости (рука/нога) во время анимации
- Берёт tween value (0-255), текущий/следующий кадр
- Интерполирует offset между кадрами
- Умножает на масштаб персонажа → вращает FMATRIX
- Захардкоженные смещения: `FOOT_HEIGHT`, `HAND_HEIGHT`

**Используется для:** позиционирования рук при хватании, ног при приземлении.

Кости: `SUB_OBJECT_LEFT_HAND`, `RIGHT_HAND`, `LEFT_FOOT`, `RIGHT_FOOT`

---

## 5. Вспомогательные функции

| Функция | Описание |
|---------|----------|
| `calc_angle(dx, dz)` | 2048-градусный угол из вектора направления |
| `angle_diff(a1, a2)` | Знаковая разница углов |
| `valid_grab_angle()` | **ВСЕГДА возвращает 1** (валидация отключена) |
| `does_fence_lie_along_line()` | Проверяет не проходит ли grab через забор |

