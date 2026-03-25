# Z-buffer mismatch: персонажи рисуются поверх стен и машин (OC-ZBUF-MISMATCH)

## Симптомы

- Персонажи видны сквозь стены (рисуются впереди стенок)
- Персонажи всегда отрисовываются поверх машин независимо от глубины
- Баг присутствует в пре-релизной кодовой базе, в финальной Steam PC и PS1 версиях его **нет**
- Запись в `prerelease_fixes.md`: "Порядок отрисовки персонаж/машина"

## Корень проблемы

Игра использует **два пути растеризации**, которые пишут в один z-буфер с **несовместимыми z-значениями**:

### Путь 1: Software POLY (стены, машины, объекты)

```
POLY_transform_using_local_rotation → POLY_perspective → POLY_add_quad → POLY_frame_draw → AddFan
```

Z-значение в `POLY_perspective` ([poly.cpp:352](../new_game/src/engine/graphics/pipeline/poly.cpp)):
```cpp
pt->Z = POLY_ZCLIP_PLANE / pt->z;   // Z = P / view_z  (обратное z, 1/z)
```

Затем в `AddFan` ([polypage.cpp:196](../new_game/src/engine/graphics/pipeline/polypage.cpp)):
```cpp
pv[ii].SetSC(..., 1.0F - pts[ii]->Z);   // z_buf = 1 - P/view_z
```

**Итого:** `z_buf = 1 - POLY_ZCLIP_PLANE / view_z` ← **корректно** (совпадает с проекционной матрицей)

### Путь 2: Hardware DrawIndPrimMM (персонажи)

Персонажи (15-part hierarchical skeleton) рисуются через `DrawIndPrimMM` —
кастомную реализацию D3D MultiMatrix batching.

В `DrawIndPrimMM` ([polypage.cpp:611-615](../new_game/src/engine/graphics/pipeline/polypage.cpp)):
```cpp
pTLVert[i].dvSZ = vertex.x * matrix._14 + vertex.y * matrix._24
                 + vertex.z * matrix._34 + matrix._44;   // = W clip = view_z
pTLVert[i].dvRHW = 1.0f / pTLVert[i].dvSZ;              // = 1/view_z (OK)
pTLVert[i].dvSZ *= (POLY_ZCLIP_PLANE);                   // = view_z * P  (НЕПРАВИЛЬНО!)
```

**Итого:** `z_buf = view_z × POLY_ZCLIP_PLANE` ← **НЕПРАВИЛЬНО** (линейное z вместо обратного)

### Проекционная матрица

Проекционная матрица (`g_matProjection`, [poly.cpp:282-297](../new_game/src/engine/graphics/pipeline/poly.cpp)):
```
|-1   0   0   0|
| 0   1   0   0|
| 0   0   1   1|     P = POLY_ZCLIP_PLANE = 1/64
| 0   0  -P   0|
```

После перспективного деления: `z_ndc = (view_z - P) / view_z = 1 - P/view_z`

Правильное z-buffer значение = **`1 - P/view_z`**. Путь POLY это делает. DrawIndPrimMM — нет.

### Числовой пример

Объект на view_z = 5 (P = 1/64 = 0.015625):

| Путь | Формула | z_buf |
|------|---------|-------|
| POLY (стена) | 1 - 0.015625/5 | **0.9969** |
| DrawIndPrimMM (персонаж) | 5 × 0.015625 | **0.0781** |
| Правильное значение | 1 - 0.015625/5 | **0.9969** |

Персонаж получает z=0.078, стена z=0.997. С `D3DCMP_LESSEQUAL`:
стена пытается записать 0.997 поверх 0.078 → **z-тест провален** → стена НЕ перекрывает персонажа.

Диапазоны z-значений для типичных игровых дистанций (view_z ∈ [1, 30]):
- **DrawIndPrimMM:** 0.016 – 0.469 (линейное, малые значения)
- **POLY path:** 0.984 – 0.999 (обратное, большие значения)
- Диапазоны **не пересекаются** → z-тест между путями всегда даёт неправильный результат

## Архитектура рендер-пайплайна

### Порядок отрисовки в aeng.cpp

```
1. Terrain, reflections, drips → POLY_frame_draw(FALSE,FALSE)  [aeng.cpp:4848]
2. Puddles, facets (building walls/floors) → accumulate in POLY buckets
   // НЕТ промежуточного flush — закомментирован и в оригинале [aeng.cpp:6033-6035]
3. Object loop [aeng.cpp:6037-6326]:
   - DT_ROT_MULTI → FIGURE_draw()        — персонажи (hardware DrawIndPrimMM)
   - DT_ANIM_PRIM → ANIM_obj_draw()      — анимированные объекты (bikes и т.д.)
   - DT_MESH      → MESH_draw_poly()      — статичные меши (software POLY_add)
   - DT_VEHICLE   → draw_car()            — машины (MESH_draw_poly → software POLY_add)
   // НЕТ промежуточного flush между объектами [aeng.cpp:6331-6332]
4. Shadows, effects, messages
5. POLY_frame_draw(TRUE,TRUE)  — ФИНАЛЬНЫЙ flush всех POLY buckets [aeng.cpp:6807]
```

Персонажи рисуются **мгновенно** (hardware → framebuffer + z-buffer) в шаге 3.
Всё остальное (стены, машины, меши) **накапливается** в POLY buckets и рисуется позже в шаге 5.
При корректных z-значениях это не проблема — z-буфер обеспечивает правильный порядок.
С **несовместимыми** z-значениями (баг) — персонажи всегда «выигрывают» z-тест.

### Два пути в FIGURE_draw_prim_tween

```
FIGURE_draw_prim_tween(prim, ...)
├── if prim ∈ [261..263]  → muzzle flash (software POLY path, POLY_add)
└── else                  → regular body part
    ├── if MM_bLightTableAlreadySetUp → hardware MM path (DrawIndPrimMM)
    │   ├── if !NeedsSorting && alpha==255 && not near-clipped → DrawIndPrimMM
    │   └── else → "currently unimplemented" (skip!)
    └── else → build lighting table lazily, then same hardware MM path
```

`FIGURE_draw` вызывается для CLASS_PERSON из aeng.cpp. Она вызывает:
- `FIGURE_draw_hierarchical_prim_recurse` для 15-part persons → batched DrawIndPrimMM
- `FIGURE_draw_prim_tween` loop для остальных

`ANIM_obj_draw` (для CLASS_BIKE/vehicles) → вызывает `FIGURE_draw_prim_tween` с
`MM_bLightTableAlreadySetUp = FALSE`. Пре-релиз всё равно строит lighting lazily и
использует hardware MM path.

### Отвергнутая гипотеза: render state leakage

`RenderState::SetChanged()` использует diff-tracking через статический `s_State`:
- Каждый `PolyPage` хранит своё желаемое состояние (RS)
- `SetChanged()` сравнивает с `s_State` и выставляет только отличающиеся стейты
- `s_State` обновляется после каждого `SetChanged()`

Figure код вызывает `pa->RS.SetRenderState(CULLMODE/ALPHABLENDENABLE/TEXTUREMAPBLEND)` +
`pa->RS.SetChanged()` перед `DrawIndPrimMM`. Это корректно обновляет `s_State`.
Последующий `POLY_frame_draw` видит правильный `s_State` и применяет только дельты.
Z-buffer стейты (ZEnable, ZWriteEnable, ZFunc) figure код НЕ трогает.

`NeedsSorting()` = `!ZWriteEnable` — возвращает true для прозрачных поверхностей
(ZWriteEnable=FALSE), которые нужно сортировать back-to-front.

### Почему envmap потребовал отдельный vertex transform

Envmap блок из оригинала читает позиции вершин из `POLY_buffer[]` (заполняется
software vertex transform в `POLY_transform_using_local_rotation`). В пре-релизе
для обычных body parts `POLY_buffer` **не заполняется** — код идёт сразу в hardware
MM path. Поэтому при восстановлении envmap пришлось добавить отдельный проход
`POLY_transform_using_local_rotation` специально для вершин envmap-граней CLASS_VEHICLE.

## Баг присутствует и в оригинальном коде

[original_game/fallen/DDEngine/Source/polypage.cpp:1071-1075](../original_game/fallen/DDEngine/Source/polypage.cpp) —
идентичная формула `dvSZ *= POLY_ZCLIP_PLANE`. MuckyFoot исправили это **после** заморозки
пре-релизного кода (в финальных билдах PC и PS1 бага нет).

## Затронутые вызовы DrawIndPrimMM

```
polypage.cpp:562     — определение функции (баг здесь, строка 615)
figure.cpp:2147      — FIGURE_draw_prim_tween (software fallback, non-hierarchical)
figure.cpp:2757      — FIGURE_draw_hierarchical_prim_recurse (основной путь персонажей)
figure.cpp:4366      — FIGURE_draw_prim_tween_person_only
fastprim.cpp:868     — FastPrim батчинг (cached mesh)
aeng.cpp:3219        — отрисовка полов (mm_draw_floor)
```

Персонажи — 3 из 5 вызовов. Но полы (aeng.cpp) и FastPrim тоже затронуты.

## Фикс PieroZ (коммит a344769) — обходной путь

PieroZ перевёл персонажей с hardware DrawIndPrimMM на software POLY_add путь.
Поскольку и стены и персонажи теперь идут через POLY pipeline с одинаковой z-формулой
(`1 - P/view_z`), z-тест работает корректно.

**Что сделано:**
- `FIGURE_draw_prim_tween`: заменён hardware MM path на software vertex transform + POLY_add
- `FIGURE_draw_hierarchical_prim_recurse`: вызовы `FIGURE_draw_prim_tween_person_only_just_set_matrix` заменены на `FIGURE_draw_prim_tween`
- Удалена функция `FIGURE_draw_prim_tween_person_only_just_set_matrix` (matrix-only step для MM batch)
- Восстановлен блок environment mapping для `CLASS_VEHICLE` (из оригинала, отсутствовал в пре-релизе)
- Добавлен `SetTempTransparent()` при `FIGURE_alpha != 255`

**Оценка:**
- ✅ Фиксит симптом — персонажи больше не рисуются поверх стен/машин
- ❌ Не фиксит корень — DrawIndPrimMM по-прежнему генерирует неправильные z-значения
- ❌ fastprim.cpp и aeng.cpp (полы) по-прежнему используют сломанный DrawIndPrimMM
- ❌ Теряется hardware MultiMatrix батчинг (потеря производительности, на современных машинах несущественна)

## Правильный фикс

Исправить z-формулу в `DrawIndPrimMM` ([polypage.cpp:615](../new_game/src/engine/graphics/pipeline/polypage.cpp)):

```cpp
// Было (баг):
pTLVert[i].dvSZ *= (POLY_ZCLIP_PLANE);

// Правильно:
pTLVert[i].dvSZ = 1.0f - POLY_ZCLIP_PLANE / pTLVert[i].dvSZ;
```

Одна строка. Фиксит корневую проблему для **всех** 5 вызовов, сохраняет hardware path.

PieroZ также восстанавливал envmap блок для `CLASS_VEHICLE` в figure.cpp.
Но это **мёртвый код**: CLASS_VEHICLE рисуется через `draw_car()` → `MESH_draw_poly()`,
а не через `FIGURE_draw_prim_tween`. Envmap в mesh.cpp уже работает (стёкла, хромированные бамперы).
CLASS_BIKE в кодовой базе есть, но байков нет ни на одном уровне.

## Статус

- [x] Применить фикс z-формулы в DrawIndPrimMM (`polypage.cpp:615`)
- [n/a] ~~Envmap для CLASS_VEHICLE~~ — мёртвый код в figure.cpp: CLASS_VEHICLE рисуется через `MESH_draw_poly`, envmap в mesh.cpp уже работает. Байков нет в игре.
- [n/a] ~~SetTempTransparent для FIGURE_alpha~~ — не нужен: `FIGURE_alpha` всегда 255 (присвоения закомментированы в `aeng.cpp:6135,6137`). Включение first-person fade — отдельная фича, не часть этого бага.
- [x] Проверить визуально: персонажи корректно перекрываются стенами и машинами
- [x] Проверить: полы (aeng.cpp `draw_quick_floor` → `draw_i_prim`) корректно отрисовываются
- [n/a] ~~FastPrim~~ — `FASTPRIM_draw` нигде не вызывается в текущем коде (мёртвый path)
