# Reference — удалённые системы wall-vs-camera / wall-vs-ground

Описание того как работали системы коллизии камеры с окружением до удаления (Этап 2 задачи `camera_improvements`). Этот файл — **справочник** для построения новой системы: когда возникнет вопрос «а как старое решало X?», искать здесь.

Источник: [`new_game/src/camera/fc.cpp`](../../new_game/src/camera/fc.cpp) **до** коммита удаления. После удаления — в репо этих систем нет, читать только этот документ или `git show` старых коммитов.

Все системы жили в `fc.cpp`. Камера — это структура `FC_Cam` (см. [`fc.h`](../../new_game/src/camera/fc.h)), массив `FC_cam[FC_MAX_CAMS]`, `FC_MAX_CAMS = 2` (split-screen, реально используется только индекс 0).

---

## Архитектура «want vs actual»

Понять wall-collision без этого нельзя.

`FC_Cam` имеет **две** позиции:
- `fc->x, fc->y, fc->z` — текущая отображаемая позиция камеры (что видит игрок).
- `fc->want_x, fc->want_y, fc->want_z` — целевая позиция куда камера стремится.

В конце `FC_process` идёт smoothing: `fc->x += (want_x - x) >> 2` и т.д. Поэтому большинство wall-collision систем правят `want_*`, а не `*` — изменения проявляются через smoothing.

Углы аналогично: `yaw, pitch, roll` vs `want_yaw, want_pitch, want_roll`.

---

## Toonear режим — общая инфра, использовалась wall-collision и first-person

`FC_Cam` имеет флаг `toonear` (UC_TRUE/FALSE) и saved-state поля `toonear_x/y/z`, `toonear_yaw/pitch/roll`, `toonear_focus_yaw`, `toonear_dist`.

Два режима активации:
1. **First-person look** (через `FC_position_for_lookaround`): `toonear = UC_TRUE`, `toonear_dist = 0x90000` (sentinel). **НЕ удалено — это отдельная фича.**
2. **Wall emergency** (внутри `FC_force_camera_behind`, см. D ниже): когда все альтернативы behind-позиции заблокированы. **Удалено как часть D.**

После удаления — toonear выставляется только режимом (1). Логика `if (!fc->toonear)` в `FC_process` остаётся актуальной для подавления geometry-зависимых вычислений в first-person режиме.

---

## A. 8-шаговый raycast collision push (in `FC_process`)

**Где было:** `FC_process`, блок `if (!fc->toonear) { ... }`, ~145 строк.

**Что делало:** не давало камере пройти сквозь стены, заборы, крышу склада.

### Алгоритм (outdoor ветка)

1. Брало текущую `want_x/y/z` (свдинутую +0x50 по Y).
2. Считало step как `(focus - want) >> 11` — вектор к фокусу, шкалированный.
3. Делало 8 шагов от позиции want к focus, добавляя step каждую итерацию.
4. На каждом шаге:
   - **Floor push (только шаги 0–3):** если `!MAV_inside(x,y,z)` (камера не «внутри проходимой клетки»), но `MAV_inside(x,y-0x100,z)` (точка чуть ниже — внутри) → значит камера под полом → `yforce += 256 - (y & 0xff)` (выталкивает вверх до границы клетки).
   - **Wall push (все 8 шагов):** проверяет соседние ±0x100 точки через `MAV_inside`. Если соседняя точка внутри проходимой клетки → выставляет битовый флаг `FC_PUSH_XS / XL / ZS / ZL` (4 направления XYZ).
   - **Fence detection (только когда `!MAV_inside(x,y,z)` и `y <= ground + 0x240`):** дополнительно проверяет «забор того же уровня высоты» — для каждого из 4 направлений, если соседняя клетка примерно той же высоты (`|delta_height| <= 0x40`) но `MAV_get_caps & MAV_CAPS_GOTO` говорит «туда нельзя пройти», и `there_is_a_los(...)` тоже не пропускает → добавляет соответствующий push flag. LOS флаги: `LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG`.
   - **Применение push:** для каждого flag → накапливает в xforce/zforce смещение (`+= 0x100 - (x & 0xff)` для push в сторону малых X, `-= (x & 0xff)` для больших).

5. В конце прохода: `fc->want_x += xforce << 4` и аналогично y, z. Множитель `<< 4` — сила push'а.

### Warehouse ветка (когда `fc->focus_in_warehouse`)

Похожа, но проще:
- Использует `WARE_inside(ware_id, x, y, z)` вместо `MAV_inside`.
- `WARE_get_caps(ware_id, x>>8, z>>8, MAV_DIR_*) & MAV_CAPS_GOTO` определяет проходимость стен.
- Floor push: если `WARE_inside(..., y-0x100, ...)` → yforce += (y & 0xff).
- **Roof push (только шаги 0–3):** `roof = MAVHEIGHT(x>>8, z>>8) << 6`. Если y > roof - 0x100 → `yforce -= y & 0xff`. Это «не пускать камеру выше крыши склада».

### Магические числа (важные)

- **8 шагов**: достаточно чтобы покрыть весь путь от камеры к фокусу с разумным sampling.
- **`>> 11` step shift**: переводит world units в шаг = расстояние / ~2048.
- **`+= 0x50` по Y на старте**: small offset вверх перед raycast (видимо чтобы попасть в воздушный объём над полом).
- **`0x240` fence ground threshold**: камера считается «у земли» если y <= ground + 0x240. Только тогда работает fence detection.
- **`0x40` height match for fence**: соседние клетки той же высоты с допуском.
- **`0x100` = 256**: размер одной MAV-клетки в low units (после `>> 8`).
- **`<< 4` push strength**: сила выталкивания (xforce/yforce/zforce были в low units, переводятся в world units).

### Зависимости (внешние API, всё в KB)

- `MAV_inside(x, y, z) -> UBYTE` ([ai/mav.h](../../new_game/src/ai/mav.h)) — «находится ли точка внутри проходимого объёма» (по MAV grid).
- `MAV_get_caps(cx, cz, dir) -> UBYTE` — capability bits клетки в направлении (`MAV_DIR_XS/XL/ZS/ZL`).
- `MAV_CAPS_GOTO` — бит «можно пройти».
- `MAVHEIGHT(cx, cz)` — высота крыши/потолка (для warehouse).
- `WARE_inside(ware_id, x, y, z)`, `WARE_get_caps(...)` ([buildings/ware.h](../../new_game/src/buildings/ware.h)) — аналоги для warehouse.
- `PAP_calc_map_height_at(x, z)` ([map/pap.h](../../new_game/src/map/pap.h)) — высота земли в точке.
- `there_is_a_los(x1, y1, z1, x2, y2, z2, flags) -> SLONG` ([engine/physics/collide.h](../../new_game/src/engine/physics/collide.h)) — line-of-sight тест с флагами.

### Связь с B

После A в одном тике сразу шёл блок B (wall-reactive get-behind boost). Если A что-то выставил в xforce/zforce → B усиливал get-behind. То есть A не только толкал from-wall, но и **сигнализировал** B что был wall hit.

### Почему удалено

Это «жёсткая стенка» — камера не могла пройти сквозь. Новый план — позволить камере свободно проходить, а коллизию решать через приближение к персонажу (new system, Этап 4).

---

## B. Wall-reactive get-behind boost (in `FC_process`, get-behind block)

**Где было:** внутри else-ветки get-behind блока, после default get-behind shift.

**Что делало:** когда A выставил xforce или zforce != 0 (камера в стене) — get-behind становился **сильнее**, чтобы быстрее вытащить камеру из стены к нормальной позиции за персонажем.

### Код (упрощённо до удаления)

```cpp
if (xforce || zforce) {
    // Stronger get-behind when near walls.
    fc->want_x += dx >> 6;   // small contribution
    fc->want_z += dz >> 6;
    fc->want_x += dx >> 5;   // bigger contribution
    fc->want_z += dz >> 5;
    if (gun_out && !driving) {
        fc->want_x += dx >> 4;   // even bigger if gun-out
        fc->want_z += dz >> 4;
    }
} else {
    fc->want_x += dx >> 3;   // normal contribution
    fc->want_z += dz >> 3;
    if (gun_out && !driving) {
        fc->want_x += dx >> 4;
        fc->want_z += dz >> 4;
    }
}
```

Где `dx = behind_x - fc->want_x`, `dz = behind_z - fc->want_z`. То есть тянет к behind-позиции.

### Замечание

В «stronger» ветке сумма shift'ов = `>> 6` + `>> 5` ≈ `>> 4.4`, что **меньше** чем `>> 3` в normal. Хотя называется stronger — на деле она **слабее** по силе одного тика, но действует **каждый тик** пока камера в стене (тогда как normal — действует **всегда**). За несколько тиков stronger догоняет/перегоняет normal. Логика не интуитивна, видимо MuckyFoot тюнили чтобы избежать резких прыжков.

### Связь с A

Этот блок **зависел от A** — если A удалить, xforce/zforce всегда 0, всегда идёт else-ветка. Поэтому B удалена синхронно с A.

---

## C. `FC_allowed_to_rotate` + блокировка L1/R1

**Где было:** функция `FC_allowed_to_rotate(FC_Cam* fc, SLONG rotate_dir)`, вызов в `FC_rotate_left` / `FC_rotate_right`.

**Что делало:** L1/R1 inputs (orbit camera) проверяли, не приведёт ли поворот в геометрию. Если приведёт — поворот игнорировался. Это и есть «камера блокируется стенами и перестаёт вращаться по L1/R1».

### Алгоритм

1. Считало `dx, dz` от focus к want (направление от персонажа к камере).
2. Поворачивало этот вектор на 90° (left или right) — получало точку куда камера попадёт после поворота: `mx, my, mz`.
3. Делало 4 проверки в углах квадрата ±`FC_ROTATE_NEAR` (0x40) вокруг этой точки:
   - Outdoor: если **любая** угловая точка `MAV_inside(cx,cy,cz)` (внутри клетки = в препятствии для outdoor смысла, см. note ниже) — return UC_FALSE.
   - Warehouse: если **любая** угловая точка `!MAV_inside(cx,cy,cz)` — return UC_FALSE.
4. Если все 4 ok → return UC_TRUE → rotate выставляется в `fc->rotate = ±0x600`.

### Note про инверсию MAV_inside

В outdoor `MAV_inside == UC_TRUE` означает что точка **внутри непроходимого объёма** (стена, забор). В warehouse наоборот — `MAV_inside == UC_TRUE` означает «внутри валидной клетки склада», а UC_FALSE = вышли за пределы. Поэтому проверки инвертированы.

### Магические числа

- `FC_ROTATE_NEAR = 0x40` — радиус квадрата вокруг ожидаемой позиции. Маленькое значение — допускает близкие к стене повороты.
- `0x600` — сила rotate (выставляется в `fc->rotate`, дальше FC_process применяет это для постепенного поворота). Decay в FC_process — `0x80 * TICK_RATIO >> TICK_SHIFT` каждый тик.

### Почему удалено

L1/R1 теперь всегда работает, камера может развернуться куда угодно (геометрия игнорируется).

---

## D. `FC_force_camera_behind` — wall-aware behind positioning

**Где было:** функция `FC_force_camera_behind(SLONG cam)`. Вызывается извне:
- `game_tick.cpp` (×2) — geneval game logic
- `input_actions.cpp` (×5) — нажатие «get behind» action (геймпад, клавиатура)
- `missions/memory.cpp` (×1)
- Внутри `FC_setup_initial_camera` (удалено в E)

**Что делало:** мгновенно ставило `want_x/y/z` в позицию «за персонажем». Если эта позиция была заблокирована — пробовало альтернативы. Если все альтернативы заблокированы — переходило в emergency first-person mode (toonear) с сохранением state.

### Алгоритм (до удаления wall-aware части)

1. **Compute ideal behind:** `gox = focus_x + SIN(focus_yaw) * cam_dist >> 8`, аналогично для goz; `goy = focus_y + FC_focus_above(fc)`.
2. **fc->toonear = UC_FALSE.**
3. **LOS check от фокуса к ideal behind:** `MAV_height_los_slow(in_warehouse, focus_x, focus_y + 0x10000, focus_z, gox, goy, goz)`.
4. **Если LOS пройден** → переход к шагу 7 (через метку `found_place:`).
5. **Если LOS не пройден:**
   - Сохраняет позицию сбоя в `abort_x/y/z` из глобалов `MAV_height_los_fail_x/y/z`.
   - Пробует 4 alternative angles: `dangle ∈ {+100, -100, +200, -200}` (yaw units, 2048 = 360°).
   - Для каждого угла пересчитывает gox/goz и снова делает `MAV_height_los_slow`. Если пройден → `goto found_place`.
6. **Если все 4 fail (emergency):**
   - gox/goy/goz = abort точка минус 0x6000 по Y.
   - **Сохраняет полное pre-state в toonear_* поля:** позицию (`toonear_x/y/z`), углы (`toonear_yaw/pitch/roll`), focus_yaw (`toonear_focus_yaw`), и считает `toonear_dist = QDIST2(|gox-focus_x|, |goz-focus_z|)`.
   - `fc->toonear = UC_TRUE` → активирует first-person mode.
   - Потом в `FC_process` блок «toonear cancel» (lines 1025–1057 до удаления) **выходит** из этого режима когда камера повернулась достаточно (`|dangle| > 200`).
7. **found_place:** `fc->want_x/y/z = gox/goy/goz`.

### Что осталось после удаления

Только шаги 1, 2, 7. Просто: посчитал behind, выставил want. Без LOS, без альтернатив, без emergency toonear.

### Магические числа

- `0x10000`: вертикальное смещение от focus_y для LOS check (~1 unit вверх от фокуса).
- `±100, ±200`: альтернативные углы (в yaw units; 100 ≈ 17.6°, 200 ≈ 35.2°).
- `0x6000`: смещение вниз от abort точки для emergency позиции.
- `QDIST2`: приближенное вычисление 2D дистанции `|x| + |z|` или подобное.

### Зависимости

- `MAV_height_los_slow(warehouse, x1, y1, z1, x2, y2, z2) -> SLONG` — LOS с возвратом точки сбоя в глобалы.
- `MAV_height_los_fail_x/y/z` — глобалы, заполняются `MAV_height_los_slow` при fail.
- `FC_focus_above(fc)`, `FC_calc_focus(fc)` — utility функции (не удалены).
- `SIN`, `COS` — fixed-point trig.

### Почему удалена только часть

Функция вызывается извне как «get camera behind» action — это **базовая фича**, не collision. Wall-aware часть была надстройка. После удаления — функция стала чисто «поставь камеру за персонажем», без оглядки на геометрию.

### Замечание про emergency toonear

После удаления D, emergency toonear больше не выставляется. Block «toonear cancel» в `FC_process` остаётся (он не удалён), но он теперь активируется только из `FC_position_for_lookaround` (first-person look). Логика «выход из first-person когда angle diverges > 200» работает для обоих случаев, так что cancel-blok всё ещё нужен.

---

## E. `FC_setup_initial_camera` — initial LOS check + fallback

**Где было:** функция `FC_setup_initial_camera(SLONG cam)`. Вызывается:
- При старте миссии ([eway.cpp](../../new_game/src/missions/eway.cpp))
- При level load ([elev.cpp](../../new_game/src/assets/formats/elev.cpp))
- При warehouse exit (внутри `FC_process`, lines 925–938)
- При смене персонажа ([game_tick.cpp](../../new_game/src/game/game_tick.cpp) ×3 для cam=1, split-screen)

**Что делало:** ставило камеру в default позицию «перед персонажем» (`dx = -SIN(focus_yaw)`, шаг 3 раза). Перед этим проверяло LOS от focus к default позиции. Если LOS не пройден — вызывало `FC_force_camera_behind(cam)` как fallback.

### Алгоритм

1. `FC_calc_focus(fc)`.
2. `dx = -SIN(focus_yaw)`, `dz = -COS(focus_yaw)`. Это вектор **перед** персонажем (минус, потому что в игре yaw считается «куда смотрит», и behind = +SIN/COS, перед = -SIN/COS).
3. **LOS check:** `there_is_a_los(focus_x, focus_y+0xa000, focus_z, focus_x+dx+dx+dx, focus_x+0xa000, focus_z+dz+dz+dz, LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)`.
   - **Баг в оригинале:** второй аргумент к destination — `focus_x + 0xa000` вместо `focus_y + 0xa000`. Видимо опечатка MuckyFoot. Это означает что LOS check считал с подменой Y координаты на X (плюс constant). После удаления — баг ушёл вместе с системой.
4. **Если LOS пройден:** ставит default front-position (`focus_x + dx*3, focus_y + 0xa000, focus_z + dz*3`).
5. **Если не пройден:** fallback `FC_force_camera_behind(cam)`, копирует want в actual.

После удаления — всегда шаг 4 (default front-position).

### Магические числа

- `0xa000`: вертикальное смещение от focus_y (initial camera height).
- `+ dx + dx + dx`: 3 шага по SIN/COS (приблизительно 3 × 65536 ≈ ~3 world units).
- `LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG`: игнорировать прозрачные заборы при LOS.

### Почему удалено

LOS check защищал от установки камеры внутри стены на спавне. Теперь — камера может стоять внутри стены без последствий (геометрия игнорируется).

---

## F. (НЕ удалено) GF_NO_FLOOR Y floor clamp

**Где живёт (после удаления A–E):** `FC_process`, Y-tracking блок, около строки ~890 (после удалений сместилось).

**Что делает:** `if (GAME_FLAGS & GF_NO_FLOOR) { if (goto_y < 0x28000) goto_y = 0x28000; }` — на уровнях без пола (флаг `GF_NO_FLOOR`) не даёт целевой Y опуститься ниже `0x28000`. Это safety против ухода камеры под уровень.

**Почему не удалено:** это не wall-vs-camera, это floor safety. Пользователь сказал «не нашёл ситуаций где мешает; меньше удаляем — меньше риска». Оставлено как есть.

---

## Что осталось НЕ тронутым и почему

### Связано с коллизией, но НЕ wall-vs-camera

- **`FC_can_see_point(cam, x, y, z) -> SLONG`** — fast LOS reject от позиции камеры до точки, через 5 шагов с `MAV_inside`. Это **gameplay-LOS** для запросов «виден ли объект из камеры». Не управляет камерой.
- **`FC_can_see_person(cam, Thing*) -> SLONG`** — проверка видимости персонажа (head + torso + 4 shoulder points). Также gameplay-LOS, не камеру управляет.

Эти функции остаются в `fc.cpp`. Они используют `MAV_inside` (подгружается через `buildings/ware.h` → `ai/mav.h` транзитивно).

### First-person mode

- **`FC_position_for_lookaround(cam, pitch)`** — ставит over-the-shoulder view, выставляет `toonear = UC_TRUE`, `toonear_dist = 0x90000` (sentinel значение, отличается от любого реального QDIST2). Используется через input_actions (видимо кнопка look around).
- **Toonear cancel block** в `FC_process` — выходит из first-person когда камера повернулась достаточно. Условие: `dist > toonear_dist + 0x4000` и `|dangle| > 200`. Срабатывает только в режиме first-person теперь (раньше — также из emergency wall режима D).

### Camera core (всё ниже не связано с коллизией)

- `FC_calc_focus`, `FC_look_at_focus`, `FC_alter_for_pos`, `FC_focus_above` — utility расчёт focus point и distance/height presets.
- Vehicle platform inheritance (rigid follow of moving vehicle) — `fc->platform_thing`/`platform_last_*` поля и блок в `FC_process` 881–923 (нумерация до удаления A).
- Warehouse transition snap — при пересечении границы склада камера хард-снапается через `FC_setup_camera_for_warehouse` или `FC_setup_initial_camera`, плюс `render_interp_mark_camera_teleport`.
- `lookabove` decay при STATE_DEAD/DYING.
- `nobehind` decay (задержка перед re-applying get-behind после ручного поворота).
- Right stick orbit + height adjustment.
- Get-behind core (без wall-reactive ветки).
- Y tracking с скоростью зависящей от moving platform (`FACE_FLAG_WMOVE`).
- Distance clamp `FC_DIST_MIN`/`MAX` (оба = `cam_dist * offset_dist`, по сути «держи на фиксированной дистанции»).
- Position smoothing `>> 2` (xz), `>> 3` (y).
- Angle smoothing `>> 2`.
- Fixed lens `0x28000 * CAM_MORE_IN`.
- Shake.
- `FC_explosion` — добавляет shake пропорционально дистанции и силе.
- `FC_setup_camera_for_warehouse` — ставит камеру у двери склада на entry. Не collision (просто entry positioning).

---

## Подсказки для построения новой системы (Этап 4)

1. **Не возрождать старые сущности.** Новая система = новые имена, новые поля в `FC_Cam` (или отдельная структура). Старые удалены не зря — они правили `want_*` напрямую и были тесно переплетены с другими частями `FC_process`. Новая логика должна работать **поверх** на отдельных переменных.

2. **Возможные точки переиспользования идей** (но **не кода**):
   - 8-step raycast (A) — концепция «sample вдоль сегмента» полезна для нахождения первой точки видимости / коллизии. Но не как push.
   - 4 alternative angles (D) — концепция «попробовать обходные углы если базовый заблокирован» — может пригодиться для choose-best-position алгоритмов.
   - `there_is_a_los` с `LOS_FLAG_*` — мощный LOS API, есть смысл использовать в новой системе.

3. **Чего избегать:**
   - Прямой правки `fc->want_x/y/z` или `fc->x/y/z` — это **старая логика**. Новая должна писать в свои переменные, а интерполяция (Этап 3 + 4) читает из новых.
   - Битовые маски push-направлений (`FC_PUSH_*`) — низкоуровневый паттерн 1999 года, плохо читается. Если нужны directions — векторы.
   - Магические сдвиги типа `>> 11`, `>> 4`, `>> 8` — старая fixed-point арифметика. Можно использовать но с **именованными константами** (см. CLAUDE.md правило).

4. **Использовать существующее API:**
   - `MAV_inside`, `MAV_height_los_slow`, `there_is_a_los` — продолжают работать, можно дёргать из новой системы.
   - `FC_focus_above`, `FC_calc_focus`, `FC_focus_*` поля — focus point считается старой системой и доступен.
   - `FC_cam[cam].x/y/z` после полного отработки старого `FC_process` — это **готовая позиция камеры от старой системы**. Новая система читает это и считает свою позицию поверх.

5. **Кинематики (cutscenes) не трогать.** `EWAY_grab_camera` подменяет позицию камеры в `FC_can_see_person` и в других местах. Кинематики работают своим pipeline, не через новую систему.
