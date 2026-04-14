# Stage 12 parity — batch 1 bugfixes (2026-04-14)

Первая партия фиксов из stage12 parity workstream. Контекст и полный процесс отбора →
[`new_game_planning/stage12_parity/`](../new_game_planning/stage12_parity/). Этот файл — описание
того **что именно** поменяно в коде и **почему**, без процессного шума (аудитов,
отвергнутых пунктов, решений о стратегии).

Применено 11 фиксов в 13 файлах + 1 внеплановый smoke-test фикс (ware assert).
Сгруппированы по характеру бага.

**Smoke-test результат (2026-04-14, release+ASan):** прошёл. Прыжки в клубе,
стрельба в Gatecrasher, смена миссий — без крашей и видимых регрессий. Фиксы
готовы к коммиту.

## Crash / iteration bugs

### ENG-18 — `OB_remove` missing return

[ob.cpp:687](../new_game/src/map/ob.cpp#L687) — в цикле поиска prim-face, принадлежащего удаляемому
OB, после `remove_walkable_from_map(i)` не было `return`. Цикл продолжал сравнивать
`f4->ThingIndex == -oi->index` дальше, хотя индекс `oi->index` уже не валиден (OB
переписан последним элементом массива через декремент `om->num`). В худшем случае —
use-after-free на `prim_faces4`, в лучшем — лишняя работа и потенциально ошибочное
повторное удаление. Фикс: добавлен `return` — walkable face у каждого OB ровно
один, после удаления искать больше нечего.

### ANM-15 — `MESH_draw_guts` untextured ASSERT

[mesh.cpp:341](../new_game/src/engine/graphics/geometry/mesh.cpp#L341) — при отрисовке quad без
текстуры (`POLY_PAGE_COLOUR` ветка) стоял `ASSERT(0)`. Защитный assert от "этого не
должно быть" — на практике срабатывал в debug сборках на легитимных мешах
(например флаги без текстурной развёртки). Фикс: убран, ветка рисует quad цветом
как и задумано.

### ANM-03 — `FIGURE_draw` null `ae1`/`ae2` guard

[figure.cpp:2949](../new_game/src/engine/graphics/geometry/figure.cpp#L2949) — после
`ae1 = dt->CurrentFrame->FirstElement; ae2 = dt->NextFrame->FirstElement;`
отсутствовала проверка на null. Остальные три варианта функции (`FIGURE_draw_reflection`,
`FIGURE_draw_*`) уже имели одинаковый guard с `MSG_add` и early return — этот был
пропущен при портировании. Если AnimTween повреждён — null deref при
`&ae1[i]` на первой же итерации. Фикс: добавлен тот же guard что и в соседних функциях.

## Out-of-bounds / overflow

### GFX-19 + GFX-37 — `AENG_upper`/`AENG_lower` resize + mask removal

[aeng_globals.h](../new_game/src/engine/graphics/pipeline/aeng_globals.h),
[aeng_globals.cpp](../new_game/src/engine/graphics/pipeline/aeng_globals.cpp),
[aeng.cpp](../new_game/src/engine/graphics/pipeline/aeng.cpp) — это самый крупный фикс сессии,
двухшаговый и lock-step (один без другого = overflow).

**Баг:** массивы `AENG_upper[MAP_WIDTH/2][MAP_HEIGHT/2]` и
`AENG_lower[MAP_WIDTH/2][MAP_HEIGHT/2]` — это per-vertex lighting/UV для ground tile
grid. Размер 64×64. А карта — 128×128 mapsquares. Во всех 31 точках доступа в
`aeng.cpp` индексация шла через маски:

```c
pp = &AENG_upper[x & 63][z & 63];
quad[0] = &AENG_lower[(x + 0) & 63][(z + 0) & 63];
// ... 29 аналогичных мест
```

Маски `& 63` загоняли индекс в диапазон 0..63, так что доступ всегда валиден — но
семантически это значит что **mapsquare (x, z) и mapsquare (x+64, z) пишут в одну
ячейку массива**. Tile wrap-around. Для carmageddon-style open world это
катастрофа, но Urban Chaos — большие отдельные миссии, и визуально это не
проявлялось (соседние куски мапы в кадр одновременно попадают редко).

**Фикс (lock-step):**

1. Resize `AENG_upper`/`AENG_lower` с `[MAP_WIDTH/2 + MAP_SIZE_TWEAK][...]` до
   `[MAP_WIDTH][MAP_HEIGHT]` — полное разрешение карты.
2. Удалить все 31 `& 63` маски в call-sites через replace_all по шаблонам:
   `[x & 63]` → `[x]`, `[(x + 0) & 63]` → `[x + 0]` и т.д.

Делать по отдельности нельзя: только resize без удаления масок — маски продолжат
обрезать индексы и fix не сработает; только удаление масок без resize — OOB write
за пределы массива.

### GFX-07 — `POLY_shadow` buffer size

[poly.h:310](../new_game/src/engine/graphics/pipeline/poly.h#L310) +
[poly_globals.cpp:17](../new_game/src/engine/graphics/pipeline/poly_globals.cpp#L17) —
массив `POLY_shadow` использовал константу `POLY_SHADOW_SIZE` вместо
`POLY_BUFFER_SIZE`. У нас оба равны 8192, так что runtime-эффекта нет — no-op.
Применил для консистентности с release (в оригинале константы разошлись,
`SHADOW_SIZE` был меньше, что давало overflow при большом количестве теней в кадре).

## Logic / off-by-one

### PRT-50 — `PCOM_add_gang_member` shuffle off-by-one

[pcom.cpp:275](../new_game/src/ai/pcom.cpp#L275) — функция добавляет человека в gang,
вставляя его в правильную позицию массива `PCOM_gang_person[]`. Если точка вставки
не в конец, нужно сдвинуть хвост вправо на одну позицию. Цикл сдвига:

```c
for (i = PCOM_gang_person_upto - 1; i >= pg->index + pg->number; i--) {
    PCOM_gang_person[i + 1] = PCOM_gang_person[i];
}
```

`upto - 1` — это последний **занятый** индекс + 1, т.е. **свободный** слот (куда
будем писать). Но цикл читает `[i]` где `i = upto - 1` — читает ещё не
инициализированный мусор и копирует его в `[upto]`. Затем инкрементирует `upto` —
валидный последний элемент затирается мусором, а предпоследний становится
дубликатом. Один элемент массива теряется, другой дублируется.

Фикс: начинать с `upto - 2` — последнего реально занятого слота.

### AI-04 — Balrog building collision (PAP_FLAG_HIDDEN)

[bat.cpp:934-962](../new_game/src/things/animals/bat.cpp#L934) — в
`BAT_balrog_slide_along` стоял блок проверки 9 соседних тайлов (сам тайл + 8
соседей) на `PAP_FLAG_HIDDEN`:

```c
if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
    // skip — on building
} else {
    if (PAP_2HI(mx + 1, mz).Flags & PAP_FLAG_HIDDEN) collide |= BAT_COLLIDE_XL;
    if (PAP_2HI(mx - 1, mz).Flags & PAP_FLAG_HIDDEN) collide |= BAT_COLLIDE_XS;
    // ... 6 more neighbours
}
```

Помечал Balrog'a как столкнувшегося со стеной если хоть один сосед — здание. Итог:
босс залипал на углах зданий (проверено пользователем в retail, симптом: Balrog
тупо стоит рядом со стеной, не может слезть).

Collision через `mo.opt[MAV_DIR_*]` выше по функции уже корректно покрывает
навигацию между тайлами — этот блок был избыточный и делал ложные срабатывания.
Фикс: удалён полностью.

### VEH-11 — `WMOVE_relative_pos` fixed-point overflow

[wmove.cpp:491+](../new_game/src/navigation/wmove.cpp#L491) — функция пересчитывает
позицию персонажа, стоящего на движущемся wmove-face (крыша едущей машины, лифт,
платформа), после того как face сдвинулся. Алгоритм: выразить старую позицию в
local (a, b) координатах face → применить те же (a, b) веса к новой позиции face.

Баги в pre-release версии:

1. **Overflow в shifts.** Было:
   ```c
   along_a = ((rcrossb << 13) + (1 << 12)) / acrossb;
   along_b = ((-rcrossa << 13) + (1 << 12)) / acrossb;
   ```
   `rcrossb` может быть большим (произведение двух 16-bit), сдвиг `<<13`
   выбивает знаковый бит на крупных faces. Release фикс: `<<12 / <<11` —
   уменьшили разрядность, overflow больше нет.

2. **Y-sum overflow.** Было:
   ```c
   wy = (yo << 8) + (day * align_a + dby * along_b + (1 << 9) >> 5);
   ```
   Два произведения складывались до шифта — опять overflow на больших faces.
   Release: разбить на независимые шифты:
   ```c
   wy = (yo << 8) + (day * along_a >> 4) + (dby * along_b >> 4);
   ```
   Потеря точности на 1 бит vs overflow — overflow хуже.

3. **Аналогично для `now_x/y/z`:** `>>5` на сумме → `>>4` на сумме (после
   исправления shifts константа `(1 << 8)` вместо `(1 << 9)`).

4. **Teleport clamps.** В конце стояло:
   ```c
   if (abs(*now_x - last_x) > 0x10000) *now_x = last_x;
   if (abs(*now_z - last_z) > 0x10000) *now_z = last_z;
   ```
   Это было hack чтобы скрыть overflow — если результат получался безумный,
   откатить на last_x/z (персонаж застывал на месте на одном кадре). После
   исправления shifts overflow не случается, hack больше не нужен. Убран.

Все изменения портированы дословно из release `wmove.cpp`.

### VEH-10 — `WMOVE_remove(class)` новая функция

[wmove.h](../new_game/src/navigation/wmove.h) + [wmove.cpp](../new_game/src/navigation/wmove.cpp) —
добавлена новая функция `WMOVE_remove(UBYTE which_class)`. Bulk-удаляет все
wmove-faces принадлежащие Thing'ам заданного класса, с compacting массива через
`memmove` (сдвигает хвост влево, декрементирует `WMOVE_face_upto`, повторяет для
того же индекса).

Используется при level transition — когда игра выгружает все машины класса
`CLASS_VEHICLE`, их walkable faces должны удалиться синхронно. Без этого — stale
faces в массиве, которые ссылаются на уже невалидные Thing'и, и на следующем тике
`WMOVE_process` получает access violation.

Код функции портирован дословно из release.

## Audio

### AUD-07 — Wind sample reorder

[sound_id_globals.cpp:9-15](../new_game/src/assets/sound_id_globals.cpp#L9) — в массиве
`sound_list[]` имена wav-файлов wind samples были перепутаны:

```c
"f_WIND1.wav",
"f_WIND3.wav",   // должно быть WIND2
"f_WIND4.wav",   // должно быть WIND3
"f_WIND5.wav",   // должно быть WIND4
"f_WIND2.wav",   // должно быть WIND5
"f_WIND6.wav",
"f_WIND7.wav",
```

Sound-ID enum (`S_WIND1`, `S_WIND2`, ...) индексирует массив по позиции. Так что
когда код хочет проиграть `S_WIND2`, система проигрывает `f_WIND3.wav` (на позиции 1),
когда хочет `S_WIND5` — получает `f_WIND2.wav`. Три сэмпла из пяти играют
неправильные файлы. Слышно в любой уличной миссии с ветром.

Фикс: восстановлен нормальный порядок 1/2/3/4/5/6/7.

## AI — защитные ASSERT(0) trip-wires (PRT-23)

В `pcom.cpp` в 5 функциях state-transition стояли `ASSERT(0)` как защита от
"невозможных" комбинаций типа "коп атакует Darci" или "цивил получает ai_state
KILLING". Писались в процессе отладки — "если сюда попали, что-то не так".

Проблема: в реальных миссиях эти комбинации возникают легитимно. Пример: цивил
в клубе Gatecrasher при начале стрельбы получает `PCOM_AI_STATE_*` через каскад
alert-ов. Assert срабатывает → abort() → crash debug build. В release build
asserts были заглушены (define `ASSERT` → пустой), поэтому баг не всплывал на
retail — но у нас ASSERT работает как должен, через `uc_assert_fail` → crash_log.txt → abort.

Убраны следующие asserts (все на проверку `PersonType == DARCI/COP/CIV`):

- [pcom.cpp — `PCOM_alert_my_gang_to_a_fight`](../new_game/src/ai/pcom.cpp) — DARCI+CIV и DARCI+COP в loop
- `PCOM_set_person_ai_arrest` — DARCI target
- `PCOM_set_person_ai_kill_person` — DARCI+COP (и мой temp-workaround с early return для CIV)
- `PCOM_set_person_ai_navtokill_shoot` — DARCI+COP и CIV
- `PCOM_set_person_ai_navtokill` — DARCI+COP
- `PCOM_add_gang_member` — CIV (убран попутно с PRT-50 edit)

`default: ASSERT(0)` в switch-ах (которые ловят невалидный `pcom_move` / `pcom_ai_state`
enum) оставлены — они защищают от повреждённой памяти, не от легитимных состояний.

---

## Smoke-test фикс — `WARE_mav_inside` assert crash

Обнаружено при smoke-тесте сразу после применения основных фиксов. Персонаж
прыгал в клубе Gatecrasher (помечен `FLAG_PERSON_WAREHOUSE`, `Ware != 0`), при
приземлении игра молча закрылась. crash_log.txt:

```
Assertion failed: WARE_in_floorplan(p_person->Genus.Person->Ware, dest_x, dest_z)
File: buildings/ware.cpp  Line: 432
```

**Причина:** в `WARE_mav_inside` стоял защитный assert что destination лежит
внутри floorplan'а warehouse'а. При прыжке игрока на краю лестницы физика на
один кадр выводит WorldPos за границу ware-floorplan (floorplan это дискретный
grid mapsquare'ов, а физика continuous). NPC внутри клуба в тот же тик запускает
pathfinding на игрока, получает его текущую позицию как destination, передаёт
в `WARE_mav_inside` — assert срабатывает, `uc_assert_fail()` → abort.

В retail этот assert скомпилирован в no-op (`-DNDEBUG` эффект, или asserts
просто отключены в финальном билде) — тот же баг существует, но молчит, потому
что `MAV_do` дальше всё равно обрабатывает невалидный индекс как-нибудь и NPC
на кадр двигается странно. Игрок никогда не замечает.

**Фикс:** [ware.cpp:432, 445, 527](../new_game/src/buildings/ware.cpp) — все три
`ASSERT(WARE_in_floorplan(...))` в `WARE_mav_inside` и `WARE_mav_exit` заменены
на graceful fallback: если точка вне floorplan'а, функция возвращает
`MAV_Action { action=GOTO, dest=current_position }` — stand-in-place на один
тик. Pathfinding пересчитается на следующем тике, когда цель (или сам NPC)
вернётся в bounds.

Для линии 445/527 (start_x персонажа) дополнительно нужно было восстановить
swapped `MAV_nav`/`MAV_nav_pitch` перед early return, чтобы не оставить
глобальное состояние в несогласованном виде.

**Бонус-инсайт для PHY-01:** этот крэш подтвердил что `p_person->Genus.Person->Ware`
— это **warehouse instance ID** (ненулевой внутри помещения), а не "weapon"
флаг как я изначально подумал. Это объясняет логику release-фикса PHY-01:
`if (Ware) FIND_FACE_NEAR_BELOW; else FIND_ANYFACE` — внутри warehouse нужен
строгий поиск по конкретному этажу, снаружи — общий. Обновлю `plan.md` #27
когда вернёмся к тому пункту.

---

## Связанный фикс (не из parity workstream)

Во время сессии пользователь проверил два бага в retail Steam PC и подтвердил что
они воспроизводятся идентично — значит оригинальные релизные баги, не регрессии
OpenChaos. Обновлена таблица в
[known_issues_and_bugs.md](../new_game_planning/known_issues_and_bugs.md):

- **NPC на танцполе клуба не находят выход** (Gatecrasher) — pathfinding не видит
  лестницы, NPC бегут в стенку при стрельбе. Статус: `Известный (релизный баг)`.
- **Телепорт при прыжке в конце лестницы** (Gatecrasher) — провал под текстуры и
  "вскарабкивание". Того же класса что "телепорт на крышу при прыжке у стены".
  Статус: `Известный (релизный баг)`.

Оба не обязательны до 1.0.
