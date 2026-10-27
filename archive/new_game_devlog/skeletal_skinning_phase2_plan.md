# План: Фаза 2 скиннинга — консолидация рендера персонажа + soft rig

Рабочий план **Фазы 2** работ по скиннингу: консолидация всей геометрии
персонажа в один VBO + multi-bone skinning + texture array + авто-риг с
soft-весами + A/B тумблер.

Без правок кода — анализ, фиксация наработок, этапность, точки верификации.
**Документ самодостаточный:** новый агент должен суметь стартовать,
прочитав только этот файл (плюс [skeletal_skinning_plan.md](skeletal_skinning_plan.md)
для контекста Фазы 1).

Связанные документы (не дублировать, только ссылки):

- [skeletal_skinning_plan.md](skeletal_skinning_plan.md) — Фаза 1 (1A–1E).
  **Завершена, закоммичена.** Этот документ — её продолжение.
- [render_batching_plan.md](render_batching_plan.md) — батчинг-план верхнего
  уровня. Эта работа реализует «Этап 2» из него (один draw на персонажа).
- [stage13_future_ideas.md](../new_game_planning/stage13_future_ideas.md)
  §«Скелетные анимации персонажей».

---

## 1. Что Фаза 1 на самом деле сделала (и не сделала)

**Важная коррекция к [skeletal_skinning_plan.md](skeletal_skinning_plan.md):**
там планировался формат вершины «сразу под 4 веса с итерации 1»
(`idx0=bMatIndex, w0=1.0`). **Это было дизайн-намерение, не факт.**
По коду iter-1 шипанулся MVP с **single-bone** форматом:

```c
struct GESkinVertex {        // 44 bytes, см. game_graphics_engine.h:502
    float    x, y, z;        // model (bone-local) space
    float    nx, ny, nz;     // model-space normal
    uint32_t bone;           // ОДНА кость, никаких весов
    uint32_t color, specular;
    float    u, v;
};
```

Это **не недоработка** — для 1D перфа single-bone было достаточно, и это
правильное MVP. Но: «iter-2 как чисто data-only смена весов» **не выйдет.**
Формат вершины надо расширять, шейдер переписывать, конвейер апгрейдить.

**Что iter-1 реально сделал (закоммичено):**
- 1A–1C: трансформ/лайтинг/туман персонажей на GPU.
- 1D: персистентный VBO **на (TomsPrimObject, материал)** — кэшируется по
  модели, не пересоздаётся каждый кадр. Источник: первый раз когда
  `ge_draw_multi_matrix` вызвался для этой связки, в
  [polypage.cpp:460](../new_game/src/engine/graphics/pipeline/polypage.cpp#L460),
  результат складывается в `mm->skin_cache` (на сам meshe указатель
  `GESkinMesh*`). Следующий раз — `mm->skin_cache != NULL` → early-out на
  [polypage.cpp:513](../new_game/src/engine/graphics/pipeline/polypage.cpp#L513),
  только `ge_skin_mesh_draw` с обновлённой палитрой.
- 1E: тени персонажей на GPU.

**Чего iter-1 не сделал:**
- ❌ Один draw на персонажа. Сейчас ~15–30 draw'ов на персонажа
  (по числу body-part×material комбинаций).
- ❌ Multi-bone веса. Один индекс кости на вершину.
- ❌ Bind-pose / skin matrices. Каждый MM-call работает в своей маленькой
  палитре, обычно с одной костью.

---

## 2. Цель Фазы 2

### 2.1. Архитектурный invariant (project-wide)

**⚠️ В ИТОГЕ — ОДИН ПУТЬ СКИННИНГА НА ВСЁ. Никаких rigid-фолбэков
нигде в коде.** Этот пункт перекрывает все остальные. Если возникает
соблазн оставить «старый путь для случая X» — это ошибка планирования,
а не валидный compromise.

Конкретно «один путь» означает:

- **Единый soft skinning АЛГОРИТМ на GPU.** Всё что движется костями
  (тело, тени, отражения; люди и не-люди) использует одну и ту же
  математику: `skin[i] = current[i] × inv_bind[i]`, до 4 костей на
  вершину, блендинг в bind-space через `skin_world_vert.glsl` (или
  его варианты — см. ниже). Никаких CPU-софтверных rasteriser'ов,
  никаких baked-screen палитр, никаких single-bone fixed путей.
- **Vertex format и шейдеры могут отличаться по подсистемам**, и это
  ок. У теней не нужны UV/color/normal — только position + bones +
  weights (легче VBO, меньше attribute traffic). У отражений нужно то
  же что у тела, плюс другая финальная матрица (mirror). Не-люди могут
  использовать тот же vertex format что люди но с trivial весами, либо
  свой урезанный — решить по факту. Главное: **алгоритм soft skinning
  один и тот же на GPU**, vertex format и финальная проекция — детали
  реализации каждой подсистемы.
- **Не-люди (Балрог, летучие мыши, Bane)** идут через тот же soft skin
  алгоритм с trivial весами `(w0=1, w1..w3=0)`. Авто-риг к ним **не
  применяется** (это про человеческую анатомию), но runtime-рендер
  использует ту же skin-математику, что у людей.
- **Тело, тени, отражения** все используют одну и ту же skin-палитру
  на кадр и один и тот же алгоритм деформации. Soft-веса для людей
  автоматически распространяются на тень и отражение — никакой
  отдельной интеграции не требуется.
- **Финал (P2-J):** весь код старого MM/baked path
  (`MMBodyParts_pMatrix`, `skin_vert.glsl`, baked-palette draw API,
  старый vertex format с одной костью на вершину, per-body-part
  shadow draw, CPU-софтверный reflection rasteriser) **полностью
  удаляется**. Если он остался — Фаза 2 НЕ завершена.

### 2.2. Что конкретно должно быть достигнуто к концу Фазы 2

- **Один draw на персонажа** (или ~1 — зависит от того есть ли
  альфа-сорт).
- **Один VBO/IBO на (chunk, MeshID)** содержащий геометрию всех 15
  болванок.
- **Палитра 15 костей** на персонажа, индексы вершин — глобальные 0..14.
- **Multi-bone skinning** в шейдере, до 4 костей/вершину, через
  skin-матрицы `current × inv_bind`.
- **Texture array на (chunk, MeshID)** — все материалы тела как слои
  одной GL-текстуры, per-vertex layer index.
- **Авто-риг** генерит soft-веса для граничных вершин в суставах на
  загрузке/первом использовании. Веса в памяти, не на диск.
- **A/B-тумблер soft/hard rig** (debug-клавиша) для визуальной сверки в
  процессе доводки.
- **Не-люди** через тот же скин с trivial весами (см. §2.1).
- **Тени и отражения** через тот же скин path.
- **Визуально 1:1 с текущим рендером на каждом промежуточном этапе.**

---

## 3. Критические наработки (что выяснили в обсуждении 2026-05-19/20)

**Эти факты нужно знать с первой минуты — не переоткрывать!**

### 3.1. Скелет человека (15 костей, иерархия)

Захардкожен в [pose_composer.cpp:17](../new_game/src/engine/graphics/geometry/pose_composer.cpp#L17),
переиспользуем как есть:

```
0  PELVIS         (root, parent = -1)
1  LEFT_FEMUR     (parent = PELVIS)
2  LEFT_TIBIA     (parent = LEFT_FEMUR)
3  LEFT_FOOT      (parent = LEFT_TIBIA)
4  TORSO          (parent = PELVIS)
5  LEFT_HUMORUS   (parent = TORSO)       ← плечо
6  LEFT_RADIUS    (parent = LEFT_HUMORUS) ← предплечье
7  LEFT_HAND      (parent = LEFT_RADIUS)
8  RIGHT_HUMORUS  (parent = TORSO)       ← плечо
9  RIGHT_RADIUS   (parent = RIGHT_HUMORUS)
10 RIGHT_HAND     (parent = RIGHT_RADIUS)
11 HEAD           (parent = TORSO)
12 RIGHT_FEMUR    (parent = PELVIS)
13 RIGHT_TIBIA    (parent = RIGHT_FEMUR)
14 RIGHT_FOOT     (parent = RIGHT_TIBIA)
```

Сейчас scope (`POSE_PERSON_BONE_COUNT = 15`). Авто-риг работает **только**
для скелетов с `bone_count == 15` и `flat_skeleton == false`. Остальные →
старый rigid путь.

### 3.2. A-поза (нейтральная поза для авто-рига)

**Выбор позы — A-поза 30°**, не T-поза 90°. Проверено в игре, пользователь
подтвердил.

- **30° (а не 45° или 90°):** при 90° (горизонтальные руки) ригид-болванка
  плечевого «наплечника» накладывается на болванку плеча — артефакт. 45°
  немного лучше, 30° оптимально: руки разведены достаточно чтобы кисти не
  были рядом с бёдрами (нет ложных кросс-влияний для авто-рига), но и
  наплечники не залезают на плечо.
- **Ось — Z**, не X и не Y. На X — руки уходят вперёд (не в стороны).
- **Знаки:** `rotate_obj(0, 0, -ANGLE, ...)` для **LEFT_HUMORUS** (кость 5),
  `rotate_obj(0, 0, +ANGLE, ...)` для **RIGHT_HUMORUS** (кость 8).
  Противоположный знак отправляет каждую руку через тело на чужую сторону.
- **UC-углы:** 30° = `2048 × 30 / 360 ≈ 171`.
- Все остальные кости — `identity` локальный поворот (это и есть
  «естественная rest-поза рига», в которой руки висят вдоль тела). Только
  плечи получают ±30°.
- **PELVIS world rot — identity** (фиксированное направление, поза
  воспроизводимая). World position таза — берётся как есть (где Дарси
  сейчас стоит).

**Где это сейчас закодировано (как временный визуальный тест):**
[render_interp.cpp `g_tpose_override_enabled` ветка в render_interp_compute_pose](../new_game/src/engine/graphics/render_interp.cpp).
Делает override per-frame для Дарси (NET_PLAYER(0)→PlayerPerson). Это
**временный механизм для подбора угла** — для авто-рига нужен такой же
расчёт но на этапе **загрузки/сборки меша**, не per-frame.

### 3.3. Локальные позиции костей (bone offsets)

Источник: `dt->TheChunk->AnimKeyFrames[anim].FirstElement[part].OffsetX/Y/Z`.
Per-keyframe, при движении могут немного смещаться. Для bind-позы можно
взять любой нейтральный кадр (idle/stand), или использовать значения из
текущего снапшота (`PoseSnap::bones_curr[i].x/y/z` — уже parent-local в
scale ×256, см. [render_interp.cpp:265](../new_game/src/engine/graphics/render_interp.cpp#L265)).

### 3.4. Известные косметические артефакты A-позы

Это **проявления жёсткого рига**, ожидаемые. Soft rig должен исправить
часть из них, остальное — отдельные кейсы:

- **Кисти у некоторых персонажей висят в воздухе.** Болванка кисти
  отдельная, геометрия не соединена с предплечьем (просто кубик-кисть
  висит на координатах кости). **Soft rig это НЕ исправит** — здесь нет
  общей геометрии, нет шва, нечего блендить. Это либо ручная
  доделка геометрии, либо забить.
- **MIB-плащ приклеен к ногам.** Плащ — отдельная геометрия, прибита к
  одной кости (вероятно торс или таз). Ноги двигаются → плащ остаётся →
  залипает/клиппится. **Soft rig это исправит:** вершины плаща в шовной
  зоне получат веса от ног → плащ деформируется вслед.

### 3.5. Идентификация персонажа

- Игрок (Дарси, Roper) = `NET_PERSON(0)` или
  `NET_PLAYER(0)->Genus.Player->PlayerPerson`.
- Тип персонажа: `Genus.Person->PersonType` (см. [person_types.h](../new_game/src/things/characters/person_types.h)).
- Скелет: `Genus.Person->AnimType` → один из ANIM_TYPE_DARCI / _ROPER / _CIV
  (3 chunk'а в `game_chunk[]`).
- Меш в chunk'е: `Draw.Tweened->MeshID` → индекс в `chunk->MultiObject[]`.

### 3.6. Debug-клавиши уже добавленные

- **K** — temp toggle: A-поза Дарси (визуальный тест A-позы, удалять когда
  авто-риг закончен). Без `allow_debug_keys` гейта.
- **N** — циклить визуальную модель Дарси (15 типов). Под `allow_debug_keys`,
  в F1 легенде. Оставляем как keeper.

---

## 4. Алгоритм авто-рига (зафиксирован в обсуждении)

**Гибрид:** топология выбирает кандидатов, falloff по расстоянию задаёт вес.

### 4.1. Вход (на каждый уникальный (chunk, MeshID))

- Геометрия модели в model space (vertex pos + bone_index из `bMatIndex`).
- Скелетная иерархия (общая для всех людей, см. §3.1).
- Bone offsets (parent-local, см. §3.3).
- A-поза локальные повороты: identity для всех, ±30° Z для плеч (§3.2).

### 4.2. Шаг 1 — bind-pose forward kinematics

Прогнать FK с A-поза локальными поворотами + offsets → 15 мировых
матриц костей в bind-позе. Pelvis world rot = identity, pos = (0,0,0)
(условный origin модели).

Это **bind_palette[15]** — pose-инвариантная константа на (chunk, MeshID).

### 4.3. Шаг 2 — вершины в общем bind-space

Каждая вершина движка авторена в **локальной системе своей кости**
(`vert.x/y/z` это позиция в кости `bMatIndex`). Конвертируем в общий
bind-space:

```
vert_bind = bind_palette[bMatIndex] * vert_local
```

Это даёт «где вершина стояла бы в A-позе с identity-PELVIS» — общая
референс-система. Хранится в VBO как `pos` (одна позиция на вершину).

### 4.4. Шаг 3 — определение шовных вершин

Для каждой вершины кости B с родителем P:

```
joint_world = bind_palette[B].translation  // = origin кости B в A-позе
                                             // (= точка стыка с родителем P)
dist        = |vert_bind - joint_world|     // расстояние до сустава
```

### 4.5. Шаг 4 — вес родителя по затуханию

Параметр на тюнинг: ширина зоны блендинга `band` (доля длины кости B,
например `band = own_bone_length × 0.4`). Формула затухания:

```
t = clamp(dist / band, 0, 1)                // 0 у сустава, 1 на дальнем конце
w_parent = (1 - t) × W_MAX                  // макс блэнд у самого сустава
w_own    = 1 - w_parent
```

- `W_MAX = 0.5` (50/50 на стыке — стартовое значение, подобрать глазами).
- Альтернатива линейному `(1-t)` — `smoothstep(1-t)` для более мягкого
  перехода. Пробовать оба, оценивать глазами.
- Дальше от сустава чем `band` → `t=1` → `w_parent=0` → жёсткая привязка.

### 4.6. Шаг 5 — запись весов

```
vert.bones[0]   = bMatIndex           // своя кость
vert.weights[0] = w_own
vert.bones[1]   = body_part_parent[bMatIndex]  // родитель
vert.weights[1] = w_parent
vert.bones[2]   = vert.bones[3]   = 0
vert.weights[2] = vert.weights[3] = 0
```

PELVIS — корень, родителя нет. Его вершины всегда `w0=1`.

### 4.7. Расширение на дочерние стыки (если понадобится)

Если визуально дистальный стык (например, бедро↔колено: верх голени с
бедром) даст шов — добавить блэнд **со стороны родителя** в обратную
сторону. То есть нижние вершины бедра получают вес от голени. Это шаг
итерации 2.1, пробовать только если 4.5 не закроет шов.

### 4.8. Что НЕ делаем (зафиксировано)

- **Поиск кандидатов по радиусу в пространстве** — отвергнут (риск
  кросс-влияний рука→нога). Берём родителя из иерархии — топология
  гарантирует что влияние не утечёт на неродственную кость.
- **Авто-риг для не-человеков** (балрог, летучие мыши) — не делаем.
- **Сохранение весов на диск** — не делаем. Считаем в памяти при первом
  использовании, кэшируем по (chunk, MeshID). Старт уровня дёшев.

---

## 5. Архитектура движка (что меняется)

### 5.1. Формат вершины

Новый `GESkinVertex` (или новое имя — `GESkinVertexV2`?):

```c
struct GESkinVertex {
    float    pos[3];        // bind-space position (см. §4.3)
    float    normal[3];     // bind-space normal
    uint8_t  bones[4];      // bone indices into 15-bone palette
    uint8_t  weights[4];    // normalised uint8 (0..255 = 0..1)
    uint32_t layer;         // texture array layer index (per-vertex)
    uint32_t specular;      // fog (CPU)
    float    uv[2];
};
```

Размер: 12 + 12 + 4 + 4 + 4 + 4 + 8 = **48 байт** (vs 44 байт сейчас).
Bones/weights — 4×uint8 каждое для компактности. Layer per-vertex — для
texture array.

`color` поле можно убрать (lit-path для персонажей не используется,
unlit-path считает цвет в шейдере).

### 5.2. Bind-pose & skin matrices

На каждый (chunk, MeshID) — кэш `bind_palette[15]` (см. §4.2). Один раз
посчитать, держать в памяти.

Каждый кадр CPU считает `skin_matrix[15] = current_palette[i] × inverse(bind_palette[i])`
и передаёт как uniform массив. GPU считает:

```
world_pos    = Σ_i weights[i] × (skin_matrix[bones[i]] × bind_pos)
world_normal = Σ_i weights[i] × (skin_matrix[bones[i]].rot × bind_normal)
```

Skin-матрицы дешевле инвертировать **в bind-позе один раз** (а не каждый
кадр). На каждый кадр — только умножение `current × precomputed_inverse_bind`.

### 5.3. Texture array per (chunk, MeshID)

При первой сборке меша:

1. Пройти по всем материалам персонажа (по всем body-part'ам этого MeshID).
2. Собрать список уникальных текстур.
3. Найти максимальный размер `(W, H)` среди них.
4. Создать `GL_TEXTURE_2D_ARRAY` размера `W × H × N_layers`.
5. Каждую текстуру: если меньше — паддинг (заливка нулём/повтор), если
   больше — ресайз или собственный путь (можно опционально). Залить как
   слой массива.
6. На каждую вершину записать индекс слоя соответствующего её материалу.

В шейдере: `sampler2DArray`, `texture(tex_array, vec3(uv.x, uv.y, layer))`.

**Mipmap'ы:** `glGenerateMipmap(GL_TEXTURE_2D_ARRAY)` или ручная заливка
по уровням.

**Альфа/прозрачность:** если в массиве есть прозрачные текстуры — все слои
должны рендериться в правильном порядке. Прозрачные части тела (есть ли
такие?) могут потребовать раздельных draw'ов. Открытый вопрос.

### 5.4. Единый VBO/IBO на персонажа

Геометрия 15 болванок склеивается в один буфер. Индексы — глобальные
(0..N-1 по всему буферу). При сборке per-vertex bone index переводится из
**локального** (по body-part'у, как сейчас в `bMatIndex`) в **глобальный**
(0..14 в 15-bone палитре).

Связывание per-body-part в один меш делается на этапе сборки скин-меша
(`mm_flush_skin` или его новый аналог).

### 5.5. Изменения в figure.cpp / polypage.cpp

- `figure.cpp` сейчас рекурсивно по `FIGURE_dhpr_*` обходит болванки и
  вызывает `ge_draw_multi_matrix` на каждую часть тела. Новый путь:
  один вызов в начале (или в начале первой части тела), сборка/драй ВСЕЙ
  модели.
- `polypage.cpp:ge_draw_multi_matrix` сейчас на каждый MM-call отдельно
  кеширует через `mm->skin_cache`. Новый путь: ключ кэша — `(chunk, MeshID)`,
  а не `(TomsPrimObject, material)`.
- Тени персонажей (`SMAP_person_gpu` в smap.cpp) рисуют **per body-part**
  через `s_prim_shadow_mesh[]`. С консолидацией тени тоже консолидируются —
  один draw силуэта на персонажа.

---

## 6. Этапность (с гейтами визуальной сверки)

⚠️ **Каждый этап = сборка + ВИЗУАЛЬНАЯ A/B-сверка пользователем на
контрольных сценах (Urban Shakedown, RTA intro, ночь/туман, ближний план,
склад с тенями).** Без подписания этапа — не идём дальше.

### Этап P2-A: один VBO на персонажа, **single-bone**, 1:1

- Объединить геометрию всех болванок одного `(chunk, MeshID)` в один меш.
- Bone-индекс per-vertex — глобальный 0..14 (из иерархии).
- Палитра 15 матриц на персонажа на кадр.
- **Веса тривиальные: `w0=1` (rigid).** Никакого авто-рига.
- Texture array **НЕ делаем** ещё — оставляем N draws по числу текстур из
  одного VBO. Это позволяет изолировать корректность консолидации.
- Гейт: визуально 1:1 со старым рендером.

### Этап P2-B: texture array / atlas — ⛔ ВЫНЕСЕН В POST-1.0

Изначально планировался как обязательный финальный этап Фазы 2 (перф-апгрейд:
один draw call на персонажа через `GL_TEXTURE_2D_ARRAY`). По решению
2026-05-22 вынесен в [stage13_optimizations.md](../new_game_planning/stage13_optimizations.md)
как post-1.0 оптимизация — не блокирует 1.0, требует серьёзной переделки
вершинного формата + новой кэш-инфраструктуры + runtime-резолва
JACKET/OFFSET флагов. На текущем железе перф персонажей не бутылочное
горло.

**После выноса P2-B Фаза 2 закрыта** (invariant §2.1 «один soft skin path
на ВСЁ» выполнен после P2-J, см. лог §11).

### Этап P2-C: bind-pose & skin matrices, всё ещё **single-bone**

- Посчитать `bind_palette` на (chunk, MeshID).
- Конвертировать вершины в bind-space при сборке меша.
- На GPU: skin-матрицы `current × inverse(bind)`.
- Всё ещё `w0=1` — формально multi-bone готов, но веса тривиальные.
- Гейт: визуально 1:1. **Это самый рисковый этап** — переход между
  координатными системами легко проебать.

### Этап P2-D: формат вершины под 4 веса, **single-bone эффективно**

- Расширить `GESkinVertex` под bones[4] + weights[4] (см. §5.1).
- При сборке писать `bones[0]=своя, weights[0]=255, остальное=0`.
- Шейдер использует все 4, но при таких весах математика идентична single.
- Гейт: визуально 1:1.

### Этап P2-E: авто-риг алгоритм + A/B тумблер

- Реализовать §4 (поиск шовных вершин, расчёт весов).
- Только для 15-bone человеков, только при включённом тумблере.
- Тумблер: новая клавиша (или та же K — на этом этапе K=A-поза удаляется,
  можно переиспользовать).
- Гейт: пользователь визуально сравнивает hard vs soft на разных моделях
  (включая MIB-плащ).

### Этап P2-F: ⛔ удалён

Этот этап изначально планировался под удаление временных debug-тумблеров.
По решению 2026-05-20: **тумблеры живут до самого конца, удаляются
вместе со всем старым кодом на P2-J**. Причина: пока идут переносы
(P2-G/H/I/B), тумблер baked↔world (F) остаётся главным инструментом
визуальной A/B-сверки на каждой подсистеме. Преждевременное удаление
лишает нас гейта на тех этапах. Аналогично hard↔soft тумблер (P2-E)
полезен до самого конца для отладки соседних подсистем.

Буква **F** в идентификаторе сохранена чтобы не сбивать существующие
ссылки на буквенные шаги. Содержание перенесено в P2-J.

### Этап P2-G: не-люди на новый soft skin path

⚠️ **Обязательный по invariant §2.1.** Без этого этапа в коде остаётся
старый код для flat-скелетов.

- Балрог, летучие мыши, Bane, Гаргулья — все не-15-bone скелеты.
- Их скелеты flat (нет parent/child иерархии) — это нормально, новый
  skin path этого не запрещает. У каждой кости свой собственный world
  transform, нет FK-цепочки.
- Bind-палитра у них тривиальная: identity rotation, position = bone
  rest offset из keyframe 0. inv_bind = inverse(bind).
- **Веса вершин — trivial:** `(w0=1.0, w1..w3=0)`, `bones[0] = bMatIndex`
  оригинальной вершины. Авто-риг к ним НЕ применяется (он про
  человеческую анатомию). НО runtime-рендер идёт через **тот же** soft
  skin path что у людей — те же `skin_world_vert.glsl`,
  `ge_skin_world_draw_range`, тот же multi-bone vertex format. Это и
  есть invariant §2.1.
- Гейт: визуально 1:1 со старым flat-rigid рендером.

### Этап P2-H: тени через ТОТ ЖЕ soft skin path

⚠️ **Обязательный по invariant §2.1.**

- Сейчас `SMAP_person_gpu` рисует тени per-body-part через
  `s_prim_shadow_mesh[]`. Каждый персонаж — N draw'ов силуэта со своим
  отдельным shader path.
- **Тени должны идти через ОДИН skin path с телом**, не через
  отдельный shader. Не «свой шейдер силуэта», а **`skin_world_vert.glsl`
  с альтернативной финальной проекцией** (shadow-clip вместо
  camera-clip). Сама skin-палитра, сам bind-space меш, сами веса —
  идентичны телу. То есть когда у людей будут soft веса (P2-E), тень
  тоже автоматически получит soft-деформацию у суставов — не нужно
  ничего дополнительно настраивать.
- Реализация: либо общий шейдер с uniform-флагом «shadow projection»,
  либо два шейдера разделяющих все uniforms кроме финальной матрицы.
  Решить по чистоте кода.
- Один draw силуэта на персонажа (вместо N per-body-part).
- Гейт: визуально 1:1, число draw'ов на тени резко падает, soft-веса
  деформируют тень синхронно с телом.

### Этап P2-I: отражения в лужах через ТОТ ЖЕ soft skin path

⚠️ **Обязательный по invariant §2.1.**

- Сейчас `FIGURE_reflect_*` делает **CPU-скиннинг + mirror + projection**
  в screen-space POLY_Point'ы (`FIGURE_rpoint[]`), а POLY уже GPU-
  растеризует их через `tl_vert.glsl`. То есть «отдельный CPU-путь» — это
  про скиннинг костей и mirror-трансформ, а не про растеризацию. Сам
  пиксельный рендер уже GPU (включая wibble — `wibble_frag.glsl`).
  Цель P2-I — снести именно CPU-скиннинг отражения.
- **Отражения должны идти через ОДИН skin path с телом и тенями.**
  Тот же `skin_world_vert.glsl`, та же skin-палитра, тот же bind-space
  меш, те же веса. Отличие — только в финальной проекционной матрице
  (отражение по плоскости лужи + camera·projection·viewport).
- Поскольку отражение использует тот же skin path — оно **автоматически
  получает soft-веса для людей**, никакой отдельной интеграции с
  авто-ригом не требуется. Отражение деформируется вместе с телом
  идентично.
- Гейт: визуально 1:1, отражения корректны по геометрии и реагируют
  на soft-деформацию.

### Этап P2-B (выполнение): texture array / atlas — ⛔ ВЫНЕСЕН В POST-1.0

См. [stage13_optimizations.md](../new_game_planning/stage13_optimizations.md)
секция «Texture array / atlas для персонажей». Не в скоупе Фазы 2.

### Этап P2-J: удаление ВСЕГО старого кода ✓ ЗАВЕРШЁН (2026-05-22)

Большая часть OLD-инфраструктуры снесена попутно во время P2-G.7 / P2-H /
P2-I (старый шейдер `skin_vert.glsl`, `MM_pMatrix`/`MMBodyParts_pMatrix`,
`ge_draw_multi_matrix`/`ge_skin_mesh_draw`/`ge_draw_skinned`,
`shadow_sil_vert.glsl`, `FIGURE_draw_prim_tween_reflection`, дебаг-тумблеры
K/F/hard-soft и их глобалы). Финальная зачистка 2026-05-22 убрала
последний остаток — диагностический `u_diagnostic_color` uniform. Подробно
см. лог P2-J ниже в §11.

**Историческая цель этапа:** после него в коде остаётся **ТОЛЬКО новый soft
skinning путь на GPU**. Всё остальное старое — нахуй с пляжа (так
зафиксировано пользователем 2026-05-20).

Что удаляется:

**Старый рендер тела:**
- `MMBodyParts_pMatrix`, `MM_pMatrix`, `MMBodyParts_pNormal` и
  связанные globals.
- Старый `skin_vert.glsl` (baked palette shader).
- `ge_skin_mesh_draw`, `ge_skin_mesh_draw_range`, `ge_draw_skinned`
  (старый GL API baked-палитры).
- Старый vertex format (одна кость на вершину) — если он уже стал
  частью унифицированного multi-bone формата, проверить что старая
  форма нигде не дублируется.
- `figure_build_consolidated_skin` (вариант с bone-local позициями) и
  поле `pPrimObj->skin_consolidated` — остаётся только `_world`
  вариант (можно переименовать обратно в `skin_consolidated`, теперь
  единственный).
- `FIGURE_draw_prim_tween_person_only_just_set_matrix` — вся
  baked screen-projection бухгалтерия per-bone.

**Старые тени:**
- Per-prim shadow path в `SMAP_person_gpu` (после P2-H тени уже идут
  через единый skin path).
- Если `shadow_sil_vert.glsl` стал не нужен (P2-H использует общий
  shader с флагом) — удалить и его.

**Старые отражения:**
- CPU reflection rasteriser в `FIGURE_reflect_*` целиком (после P2-I).
- Все вспомогательные структуры reflection software-пути.

**Debug-тумблеры (жили до этого момента):**
- A-поза тумблер (K) и его ветка в `render_interp_compute_pose`.
- Тумблер baked↔world (F) и `g_skin_world_path_enabled`. После
  удаления `g_skin_world_path_enabled` логика в `figure.cpp` упрощается:
  новый путь становится единственным.
- Тумблер hard↔soft (P2-E) и сопутствующая ветка в авто-риг коде.

**Проверка:**
- `grep` по символам выше — не должно остаться dead code.
- Сборка чистая, линковка чистая.
- Гейт: визуально всё работает идентично последнему успешному
  предыдущему гейту (P2-B перед удалением).

---

## 7. Чек-лист рисков 1:1 (что легко проебать)

Эти места нужно пройти бережно — все они уже однажды ломались (см. лог
Фазы 1):

- **Координатные системы:** bind-pose ↔ world ↔ camera. Skin-матрица
  `current × inverse(bind)` — порядок умножения и сторона матриц (row vs
  column major) критичны. Сверять на простой сцене (стоячая Дарси) до
  движения.
- **Per-vertex normal:** в bind-space, трансформ матрицей-ротацией. Если
  в матрицах костей нет неравномерного scale — простой `rot × normal` ок.
  Иначе нужен inverse-transpose (см. [skeletal_skinning_plan.md §9](skeletal_skinning_plan.md)).
- **Z-clip и near-plane** — уже однажды чинили `BUGFIX-OC-ZBUF-MISMATCH`.
  Чистая проекция в шейдере должна повторять; контрольная сцена —
  перекрытия тела со стенами/машинами с близкого ракурса.
- **Туман** — плоский per-character из `g_mm_fog_view_z`, НЕ per-vertex
  (см. [skeletal_skinning_plan.md §6](skeletal_skinning_plan.md)).
- **Лайтинг** — half-Lambert по 64-таблице, `fNormScale = 251`. См.
  [stage12_character_lighting_investigation.md](../new_game_planning/stage12_character_lighting_investigation.md).
- **Прозрачность** — `FIGURE_alpha`, сортировка back-to-front. При
  консолидации в один VBO + texture array прозрачные части тела (если есть)
  могут оказаться в одном draw с непрозрачными → ломается порядок.
  Решить по факту обнаружения.
- **Тени** — путь `SMAP_person_gpu` зависит от per-prim shadow-мешей.
  Консолидация ломает этот контракт; нужен паралельный апдейт shadow
  пути под единый меш (либо оставить shadow на per-body-part пока).
- **Отражения в лужах** — отдельный CPU-путь, `ge_draw_multi_matrix`
  не вызывается. Эта работа его НЕ трогает. Отдельный пункт в roadmap.

---

## 8. Открытые вопросы (решаем по ходу)

- Максимум костей на вершину: 4 хватит? Для humanoid с 2-bone блэндом
  скорее всего да. Резерв на 4 на будущее.
- `W_MAX` (макс блэнд у сустава): 0.5 стартово, может потребоваться 0.4
  или 0.6 в зависимости от того как выглядит шов. Подбираем глазами.
- `band` (ширина зоны блэнда): 0.4 длины кости стартово. Тюнить.
- Линейный vs smoothstep falloff — пробовать оба.
- Прозрачные части тела персонажа в texture array — есть ли вообще? Если
  да — стратегия: разделить на два прохода (opaque pass + alpha pass) с
  переключением между двумя draw'ами на один персонаж.
- Тени: консолидировать одновременно с телом или временно оставить
  per-body-part? Голосую «временно оставить», портировать после P2-E.

---

## 9. Что НЕ делаем в этой Фазе

**⚠️ В этой секции — только то что мы реально пропускаем "навсегда" или
оставляем post-Фаза-2. Перенос подсистем (не-люди, тени, отражения) на
новый skin path — НЕ в этой секции, а ОБЯЗАТЕЛЬНЫЕ этапы P2-G/H/I/J
(см. §6). Их пропускать нельзя — это invariant §2.1.**

- Атрибуция текстур на швах между разными частями тела (стык головы
  и шеи может иметь разрыв текстуры) — отдельная косметика, не часть
  авто-рига. Можно делать или не делать в Фазе 2 — на invariant не влияет.

---

## 10. Процесс работы

- **Worktree**: эта итерация идёт **отдельной веткой/worktree** от main.
- **Гейты**: после каждого этапа (P2-A … P2-J) — сборка + явный
  визуальный gate от пользователя. Без подписания не идём дальше.
- **A/B тумблер soft/hard** на этапе P2-E — должен переключаться в
  рантайме, мгновенный визуальный контраст.
- **План — живой документ.** По ходу работы добавлять лог под §11 (как
  в Фазе 1). Если выяснится что-то критичное — поднимать в §3.

---

## 11. Лог реализации

> Журнал самого свежего сверху. Каждая запись = короткий блок: что
> сделано, что ждёт верификации, что подписано.

### 2026-05-22 — P2-J: финальная зачистка диагностической инфраструктуры ✓

Снесён последний остаток OLD/диагностического кода — `u_diagnostic_color`
uniform + ранний-выход в [common_frag.glsl](../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/common_frag.glsl)
+ 3 GL location'а + 3 setUniform вызова в [core.cpp](../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp).
Это был визуальный пробник P2-E (раскрашивать tl/skin-world/skin-reflect
программы в красный/зелёный/синий для A/B-сверки путей при отладке
soft-rig'а); постоянно выставлен в `(0,0,0,0)` → early-exit никогда не
срабатывал; мёртвый код.

**После этой правки в коде ровно один soft skin path на ВСЁ:**
- ✅ Один скелетный алгоритм (`skin[i] = current[i] × inv_bind[i]`, до
  4 костей/вершину, blend в bind-space) общий для тела + теней +
  отражений людей и не-людей.
- ✅ Vertex format унифицирован: bind-space pos/normal + bones[4] +
  weights[4]. Все skin-программы (`skin_world`, `skin_shadow`,
  `skin_reflect`) делят общий [_skin_common.glsl](../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/_skin_common.glsl)
  (формат skin-палитры + layout).
- ✅ Soft-веса людей (5-pair bidirectional blend, HEAD parent) применяются
  ко всем трём подсистемам автоматически — общий VBO + общая skin-палитра
  per-character per-frame.
- ✅ Не-люди (Балрог / летучие мыши / Bane / Гаргулья) идут через тот же
  runtime path с trivial весами `(w0=255, остальное=0)`. Авто-риг к ним
  не применяется (5-pair таблица — про human anatomy).
- ✅ OLD-инфраструктура полностью отсутствует: ни одного упоминания
  `MMBodyParts_*` / `MM_pMatrix` / `MM_pNormal` / `ge_draw_multi_matrix` /
  `ge_skin_mesh_draw` / `ge_draw_skinned` / `skin_vert.glsl` /
  `shadow_sil_vert.glsl` / `FIGURE_draw_prim_tween` /
  `FIGURE_draw_prim_tween_reflection` / `FIGURE_rpoint` /
  `g_skin_world_path_enabled` / `g_skin_soft_rig_enabled` /
  `g_tpose_override_enabled` в живом коде (только stale comments-ссылки
  как историческая справка в figure_globals.cpp / pose_composer.cpp /
  render_interp.cpp — не блокер).

**Invariant §2.1 формально выполнен.** Фаза 2 закрыта по invariant'у.
Осталось только **P2-B** (texture array / atlas per (chunk, MeshID))
как перф-апгрейд — это не invariant, отдельная оптимизация (один draw
call на персонажа вместо ~3-5 на материал).

**Файлы коммита:**
- [new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/common_frag.glsl](../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/common_frag.glsl)
  — снят uniform и early-exit.
- [new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp](../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
  — снято 3 декларации `s_*_u_diagnostic_color` + 3 getUniformLocation +
  3 setUniform.

---

### 2026-05-21 — Post-P2-I follow-up: wibble world-XZ phase fix ✓ подписано

После P2-I (consolidated reflection draw) долгоживущий баг ряби в лужах
(«дёрганое поведение wibble, скачет при вращении камеры»; запись на стр. 67
старой `known_issues_and_bugs.md`) стал next-to-fix. Реализована
**опция (1)** из перечня направлений фикса — привязка фазы к world-XZ
поверхности воды вместо screen-space row. Пользователь подтвердил
визуально 2026-05-21. Запись перенесена в `known_issues_and_bugs_resolved.md`
секция «Визуальные проблемы».

**Что сделано:**

- **`WIBBLE_simple` ([wibble.cpp](../new_game/src/engine/graphics/postprocess/wibble.cpp)):**
  инверсия POLY-камеры (double precision `mat3_invert_operator` —
  `POLY_cam_matrix` плохо обусловлена, det ~1e-12 после `MATRIX_skew`);
  прокидка в шейдер `inv_poly_cam`, `cam_pos`, `screen_mid/mul × fbo_scale`,
  `ZCLIP`, viewport.
- **`wibble_frag.glsl`** переписан: per-pixel ray-cast через инверсию
  POLY-перспективы на плоскость `Y=0` → world XZ → `sin(world_xz × wn + phase)`.
  Per-pixel AA по `fwidth(water_xz)` гасит амплитуду на грейзинг-лучах.
  Тюн-параметры: `BASE_WAVELENGTH = 30`, `S_AMP_FLOOR = 25`, `EDGE_FADE_PX = 6`
  (все подобраны глазами, помечены в комментариях).
- **Фиксированный anchor `Y=0` (не per-puddle water_height):** соседние
  puddle-bbox'ы overlap'аются на экране; per-puddle anchor давал бы
  discrete pattern jumps при вращении камеры (зависит от того какая
  лужа «выиграла» overlap). Общий anchor → одинаковый world_xz во всех
  overlap'нутых вызовах → плавно. Для плоских уличных уровней UC
  визуально неотличимо от честной привязки к поверхности лужи.
- **Compound-wibble fix в [aeng.cpp:3168-3429](../new_game/src/engine/graphics/pipeline/aeng.cpp#L3168):**
  оригинал вызывал `WIBBLE_simple` per (puddle × reflection bbox intersection);
  каждый вызов копирует FB→scratch и пишет обратно — overlap'нутые вызовы
  давали 2×/3× warp в overlap-зоне. Заменено на union интерсекций per
  reflection bbox + ОДИН `WIBBLE_simple` на reflection. Каждый пиксель
  warp'ится максимум один раз.
- **Fallback при отсутствии overlap'a с лужей:** если в кадре ни одна
  лужа не пересекла данный reflection bbox (камера повернулась —
  лужа ушла), wibble всё равно рисуется поверх всего reflection bbox'а
  с подобранным глазами пресетом параметров. Сознательное отклонение
  от оригинала ради непрерывности эффекта; на сухом асфальте ряби
  не видно.

**Файлы коммита:**
- [new_game/src/engine/graphics/postprocess/wibble.cpp](../new_game/src/engine/graphics/postprocess/wibble.cpp)
  — `mat3_invert_operator` (double precision), POLY camera state forwarding.
- [new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/wibble_frag.glsl](../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/wibble_frag.glsl)
  — переписан целиком на world-XZ phase.
- [new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.cpp](../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.cpp)
  — 6 новых uniform locations.
- [new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h](../new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h)
  — поля `GEWibbleParams` под POLY camera state.
- [new_game/src/engine/graphics/pipeline/aeng.cpp](../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — `WibbleAccum` union per reflection bbox; fallback-wibble; один
  `WIBBLE_simple` на reflection.
- [known_issues_and_bugs/known_issues_and_bugs.md](../known_issues_and_bugs/known_issues_and_bugs.md)
  — удалена запись «Wibble-отражения в лужах: поведение дёрганое...».
- [known_issues_and_bugs/known_issues_and_bugs_resolved.md](../known_issues_and_bugs/known_issues_and_bugs_resolved.md)
  — добавлена запись в секцию «Визуальные проблемы».

**Что НЕ исправлено этой работой:**
- Активная запись «Wibble amplitude не скейлится с разрешением» — отдельный
  баг. Амплитуда по-прежнему в пикселях через `AMP_SCALE = 1/8` в шейдере;
  world-XZ привязка изменила **где** считается фаза, а не **насколько**
  смещается пиксель. Баг остаётся актуальным.

---

### 2026-05-21 — P2-I: отражения в лужах через единый soft skin path ✓ подписано

Reflection теперь рисуется через тот же bind-space VBO + skin-палитру что
и тело / тень. Soft-веса P2-E применяются к отражению автоматически.
Один draw call на материал. Подтверждено пользователем 2026-05-21: рябь
работает, модель видна правильной стороной, освещение реагирует на сцену.
Ждёт коммита.

**Что сделано:**

- **`_skin_common.glsl`** — новый shared include для всех skin шейдеров.
  Содержит `MAX_BONES`, layout (`a_position`/`a_bones`/`a_weights`),
  uniform `u_skin`. CMake embed-скрипт делает text substitution на
  `#include "_skin_common.glsl"` при сборке. Применён к `skin_world_vert`
  и `skin_shadow_vert` — рабочий код не изменился, только организация.
- **`skin_reflect_vert.glsl`** (новый, ~115 строк) — helper-стиль skin
  blend (как у shadow), но добавляет:
  - `world.y = 2.0 * u_reflect_height - world.y` (mirror Y).
  - Distance-from-water term в `v_specular.a`:
    `fmul_total = clamp(fmul_camera + (dy * 255/MAX_DY), 0, 255)`.
  - Per-character flat colour из `u_reflect_color` (`.zyxw` swap, BGRA→RGBA).
  - Screen-xform математика — побайтовая копия `skin_world_vert.glsl`.
- **`ge_skin_reflect_draw_range`** (core.cpp) — engine API параллельно
  с `ge_skin_world_draw_range` / `ge_shadow_silhouette_draw`. Внутри
  делает временный `glFrontFace(GL_CCW)` (mirror Y флипает winding;
  body использует CW-front по D3D-конвенции — для reflection нужен
  обратный) и восстанавливает после draw. Backface cull остаётся
  включённым — outer surface visible, inner side culled.
- **`FIGURE_draw_reflection`** в figure.cpp переписан с нуля
  (~150 строк, было ~120 + ~370 в `_prim_tween_reflection`):
  - Получает consolidated mesh + bone AABB через
    `FIGURE_get_skin_mesh_for_thing` (расширен out_prim_obj для materials/ranges).
  - Строит skin-палитру теми же хелперами что body
    (`figure_build_skin_world_palette`, `figure_build_screen_xform_bake`).
  - NIGHT lighting per-character: `NIGHT_get_light_at(pelvis)` →
    `NIGHT_get_colour` → halve + force alpha 0xff (legacy formula).
  - **Screen-space bbox для wibble** через `POLY_transform` per bone-
    AABB corner (8 углов × 15 костей = 120 точек, ~копейки CPU). Это
    даёт coords в `POLY_screen_width` display scale — согласовано с
    puddle pp[i].X. Использование `screen_xform_bake` тут давало bbox
    в FBO scale (g_viewData.dwWidth, ~1.33× display), что ломало
    wibble intersection.
  - Per-material draw через `ge_skin_reflect_draw_range`.
- **Bonus fix:** legacy баг «отражения в лужах не реагируют на
  освещение» исчез как побочный эффект — flat NIGHT-tinted colour
  per-character теперь честно доходит до пикселя через `color × texture`
  в шейдере, реагирует на NIGHT field. Запись перенесена из активного
  `known_issues_and_bugs.md` в resolved-секцию «Визуальные проблемы».

**Что удалено физически:**

- `FIGURE_draw_prim_tween_reflection` (figure.cpp/h) — ~370 строк
  CPU-софт-rasteriser'а.
- `FIGURE_rpoint[]` / `FIGURE_rpoint_upto` / `FIGURE_Rpoint` struct /
  `FIGURE_MAX_RPOINTS` define (figure_globals.cpp/h).
- `FIGURE_reflect_height` global (figure_globals.cpp/h) — стал мёртвым
  после удаления `_prim_tween_reflection` (только writer, нет
  readers). Только `FIGURE_reflect_x1/y1/x2/y2` живы (читает aeng для
  wibble bbox).

**Pitfalls пройденные:**

1. **Bbox в неправильной scale** — первый расчёт через
   `screen_xform_bake` давал координаты в FBO viewport scale
   (`g_viewData.dwWidth ≈ 1192` при display 897), а puddle bbox от
   `POLY_transform` — в display scale. Wibble intersection всегда был
   пустой. Переключение на `POLY_transform` per corner решило.
2. **Frontface toggle** — первая попытка `glFrontFace(GL_CW)` была
   no-op (body уже использует CW-front через `ge_set_cull_mode(CCW)`,
   которая мapит в `glFrontFace(GL_CW)` — D3D-конвенция, меши UC
   авторены CW-out). Правильный toggle для reflection —
   `glFrontFace(GL_CCW)` (после mirror outer faces в screen становятся
   CCW). Альтернатива `glDisable(GL_CULL_FACE)` тоже работает но рисует
   обе стороны (лишняя фрагментная работа).
3. **Empty per-bone AABB** — кости без вершин остаются в state
   `min=+1e30 / max=-1e30`. Без skip'а corners уходят в бесконечность
   через skin → screen overflow → bbox разрушен. Skip same way как
   `SMAP_person_gpu`.
4. **`FIGURE_get_skin_mesh_for_thing` не возвращал TomsPrimObject*** —
   нужен был для materials/ranges. Добавил опциональный
   `out_prim_obj` параметр (default nullptr) — backward-compatible.

**Файлы коммита (P2-I):**
- `new_game/CMakeLists.txt` — embed `_skin_common.glsl` + `skin_reflect_vert.glsl` + text-substitution `#include`.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/_skin_common.glsl` — новый.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_reflect_vert.glsl` — новый.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_world_vert.glsl` — refactor под `#include`.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_shadow_vert.glsl` — refactor под `#include`.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp` — `s_program_skin_reflect` + uniforms + `ge_skin_reflect_draw_range`.
- `new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h` — декларация `ge_skin_reflect_draw_range`.
- `new_game/src/engine/graphics/geometry/figure.{cpp,h}` — переписана
  `FIGURE_draw_reflection`, удалена `FIGURE_draw_prim_tween_reflection`,
  расширен `FIGURE_get_skin_mesh_for_thing` (out_prim_obj).
- `new_game/src/engine/graphics/geometry/figure_globals.{cpp,h}` —
  удалены `FIGURE_rpoint[]`, `FIGURE_rpoint_upto`, `FIGURE_Rpoint`,
  `FIGURE_MAX_RPOINTS`, `FIGURE_reflect_height`.
- `known_issues_and_bugs/known_issues_and_bugs.md` — удалена запись про
  отражения и освещение.
- `known_issues_and_bugs/known_issues_and_bugs_resolved.md` — добавлена
  запись в секцию «Визуальные проблемы».

**Что осталось от Фазы 2:**
- **P2-B** — texture array per (chunk, MeshID) (perf upgrade).
- **P2-J** — финальная зачистка любых оставшихся debug-тумблеров /
  старого кода (после P2-I структурно почти нечего — было раньше
  снесено в P2-G.7 / P2-H).

После P2-B + P2-J — Фаза 2 закрыта. Invariant §2.1 «один soft skin
path на ВСЁ» **выполнен**: тело + тени + отражения людей идут через
один и тот же bind-space VBO + skin-палитру + три родственных шейдера
(`skin_world`, `skin_shadow`, `skin_reflect`) которые делят общую
часть через `_skin_common.glsl`. Не-люди тоже через тот же path с
trivial весами (P2-G).

---

### 2026-05-21 — Handoff: P2-H закоммичен, следующий — P2-I (отражения в лужах)

**Состояние репо:** ветка `optimize-fps`. P2-G + P2-H закоммичены. Билд
чистый (302/302). Тело + тени персонажей и не-людей идут через ОДИН
soft skin path (`skin_world_vert.glsl` / `skin_shadow_vert.glsl` —
делят формат skin-палитры). Soft-веса людей применяются и к телу, и к
тени автоматически.

**Что осталось из Фазы 2 (по §6, в порядке):**

1. **P2-I** — отражения в лужах через тот же skin path.
2. **P2-B** — texture array (perf).
3. После P2-I: invariant §2.1 фактически выполнен (один skin path на
   тело + тени + отражения). P2-B — perf-апгрейд, не invariant.

---

#### Следующая сессия — план P2-I

**Чего достичь:** отражения персонажей/животных в лужах должны
рисоваться через ТОТ ЖЕ `skin_world_vert.glsl` + skin-палитру что и
тело. Отличие — финальная проекционная матрица: отражение зеркалится
относительно плоскости лужи + та же camera·projection·viewport. Один
draw call на персонажа.

**Где сейчас живёт reflection code:**

- **`figure.cpp:FIGURE_draw_prim_tween_reflection`** (~370 строк) —
  CPU-софтверный rasteriser. Per-body-part, читает pose snapshot, для
  каждой вершины: bone transform → mirror-flip Y относительно
  `FIGURE_reflect_height` → проекция в экран → запись в
  `FIGURE_rpoint[FIGURE_MAX_RPOINTS]` буфер с fade-by-distance.
- **`figure.cpp:FIGURE_draw_reflection`** — top-level вызов, проходит
  все body parts через `_reflection`.
- **`figure_globals.cpp:FIGURE_rpoint[]`** + `FIGURE_rpoint_upto` —
  CPU rasteriser-style buffer screen-space points.
- **`figure_globals.cpp:FIGURE_reflect_x1/y1/x2/y2`** — bounding box.
- **`figure_globals.cpp:FIGURE_reflect_height`** — Y-координата плоскости.
- Где это потребляется (читатели `FIGURE_rpoint[]`): найти grep'ом
  `FIGURE_rpoint` — там должен быть POLY/AENG код который рисует
  собранные точки в фреймбуфер с wibble-эффектом (см. `wibble.cpp` /
  related, и/или баг "Wibble-отражения" в known_issues — там есть
  разбор подсистемы).

**Что использовать (уже готовый API):**

- **`FIGURE_get_skin_mesh_for_thing(p_thing, &mesh, &bone_aabb,
  &bone_count, &chunk, &bind_inv)`** в `figure.h` — возвращает
  consolidated bind-space VBO + per-bone bind-space AABB + chunk +
  inverse-bind палитру для ЛЮБОГО скиннингового Thing. Lazy-builds
  TPO + bind-space mesh. Это тот же хелпер что использует SMAP_person_gpu.
- **Skin-палитра build:** см. inline в `SMAP_person_gpu` (smap.cpp) или
  static `figure_build_skin_world_palette` в figure.cpp — обе пишут
  3 vec4/bone в `out[bone_count * 12]` формате M*v.
- **Mirror-projection матрица:** новый shader uniform — обычная mat4
  где Y компонента отражается относительно плоскости воды плюс умножение
  на `camera × projection × viewport`. Можно собрать на CPU перед
  draw'ом (по аналогии с `figure_build_screen_xform_bake` для body).

**Что нужно сделать:**

1. **Новый шейдер `skin_reflect_vert.glsl`** (или расширить
   `skin_world_vert.glsl` uniform-флагом). Структура близка к
   `skin_shadow_vert.glsl`: bind-space verts + skin-палитра + одна
   финальная проекционная матрица. Отличие от shadow: вместо orthographic
   shadow-projection — mirror + camera-projection (даёт нормальную
   текстурированную геометрию, не silhouette). Если идём через единый
   `skin_world_vert.glsl` с переключателем — добавить `uniform int
   u_reflection_mode` и в нём mirror-trasform + screen_xform.
   Вопрос дизайна: чистый отдельный shader vs flag в общем — решить
   по факту, оба варианта рабочие.
2. **Engine API `ge_skin_reflect_draw_range`** (или extend
   `ge_skin_world_draw_range` под reflection-режим) — принимает skin
   палитру + reflection projection матрицу + fade params (для затухания
   по дистанции от плоскости лужи).
3. **Переписать `FIGURE_draw_reflection`** — заменить loop через
   `_reflection`/`FIGURE_rpoint[]` на:
   - Получить consolidated mesh через `FIGURE_get_skin_mesh_for_thing`.
   - Построить skin-палитру (как в body / shadow).
   - Построить reflection projection матрицу (mirror Y по `FIGURE_reflect_height` +
     camera/projection/viewport).
   - Per-material draw через новый skin-reflect API.
4. **Гейт визуально** — пользователь сверяет отражения людей и
   животных в лужах с прежним поведением. Особенно сравнить с D3D или
   `last_working_d3d_build` если визуальные различия неясны.
5. **После гейта** — удалить весь старый CPU reflection raster:
   - `FIGURE_draw_prim_tween_reflection` (~370 строк) — функция.
   - `FIGURE_rpoint[]`, `FIGURE_rpoint_upto`, `FIGURE_reflect_x1/y1/x2/y2`,
     `FIGURE_reflect_height` — глобалы и их storage.
   - Consumer-side в POLY/AENG код который читал `FIGURE_rpoint[]`.
   - **Wibble**: проверить интегрируется ли он с новым GPU-путём (см.
     known_issues "Wibble-отражения" — фундаментально wibble завязан
     на screen-space row index reflection-bbox'а; если мы убираем CPU
     bbox и рисуем геометрию напрямую в финальный FB, wibble может
     потеряться. Это отдельная подзадача — либо порт wibble под новый
     путь, либо принять что wibble visual deserves redesign).

**Pitfalls (учиться на P2-G / P2-H):**

- **Stale cache на mission unload** — уже починено в P2-H.7
  follow-up (см. `FIGURE_clean_all_LRU_slots` теперь чистит
  `D3DAnimObjKeys[]` + `bind_palette_invalidate_all()`). Если для
  reflection потребуется ещё какой-то per-chunk кэш — добавить чистку
  туда же.
- **LRU eviction** — `figure_anim_obj_get_consolidated` детектит
  evicted-but-keyed слот и перестраивает (см. fix в P2-H.7). Если
  reflection путь использует `FIGURE_get_skin_mesh_for_thing` —
  получает корректное поведение даром.
- **Fog anchor** — body draw делает
  `POLY_set_local_rotation(pose[0], identity)` перед
  `figure_build_screen_xform_bake` чтобы `g_matWorld._43` был
  валидным per-character view-Z для fog. Если reflection использует
  fog/distance-fade — нужно поставить аналогичный anchor.
- **Order matters**: reflection обычно рисуется ПОСЛЕ body и тени в
  кадре (или в отдельный pass с зеркальным FBO). Проверить когда
  именно — это диктует когда у нас уже есть consolidated mesh (после
  тени и тела точно есть).

**Что почитать новому агенту перед стартом:**

1. `CLAUDE.md` (rooot).
2. `skeletal_skinning_phase2_plan.md` целиком (этот файл) — особенно
   §2.1 invariant, §4 алгоритм soft, §6 этапность, §7 риски, и эту
   handoff-запись + предыдущие P2-G/P2-H записи в §11.
3. `skeletal_skinning_plan.md` (Фаза 1) — для контекста скиннинговой
   архитектуры. Особенно «Стадия 1E — тени» и пункт про CPU reflection
   как отдельный путь в §6 рисков.
4. `known_issues_and_bugs.md`:
   - "Wibble-отражения в лужах: поведение дёрганое..." — фундаментальное
     ограничение алгоритма, может стать проще после P2-I (или останется,
     wibble — отдельный сабсистем).
   - "Отражение упавшего рядом с лужей NPC «сочится» наружу" —
     возможно лечится правильным mirror-clip'ом в новом пути.
5. `new_game/src/engine/graphics/geometry/figure.cpp` — особенно
   `FIGURE_get_skin_mesh_for_thing`, `figure_build_consolidated_skin_world`,
   `FIGURE_draw_hierarchical_prim_recurse` (для понимания body draw
   pattern), `ANIM_obj_draw` (для не-людей).
6. `new_game/src/engine/graphics/lighting/smap.cpp:SMAP_person_gpu` —
   образец вызова `FIGURE_get_skin_mesh_for_thing` + построения skin
   палитры + специальной проекционной матрицы + draw через единый
   shader. P2-I — буквально та же структура с другой матрицей.

---

### 2026-05-20 — P2-H: тени персонажей через единый soft skin path ✓ подписано

Тени теперь рисуются через **тот же** `skin_world_vert.glsl`-эквивалент
шейдер (`skin_shadow_vert.glsl`) что и тело — единый soft-skin path для
обоих. Soft-веса (HEAD parent у людей) применяются к тени автоматически
потому что shadow и body разделяют один VBO и одну skin-палитру на
кадр. Билд чистый (302/302). Пользователь подтвердил визуально 1:1.

**Что снесено (~880 строк mostly delete):**

- **`shadow_sil_vert.glsl`** (файл) + `SHADER_SHADOW_SIL_VERT` (CMake).
  Старый шейдер читал MODEL-space verts + single-bone WORLD affine
  палитру. Заменён на `skin_shadow_vert.glsl` — BIND-space verts +
  multi-bone skin палитра (тот же формат что у `skin_world_vert.glsl`).
- **`s_prim_shadow_mesh[MAX_PRIM_OBJECTS]`** — per-prim shadow mesh
  кэш в smap.cpp. Был статическим model-space мешем для каждого
  body-part прима. Заменён на общий consolidated bind-space VBO с тела.
- **`s_prim_shadow_min[]` / `s_prim_shadow_max[]`** — per-prim model-
  space AABB. Заменён на `TomsPrimObject::skin_consolidated_bone_aabb`
  (bind-space, per-bone, сохраняется при build консолидированного меша).
- **`smap_get_prim_shadow_mesh`** — lazy builder per-prim shadow meshей.
- **`smap_bone_world`** — per-bone world transform из pose snapshot.
  Тень теперь читает ту же per-character skin-палитру что и тело.
- **`SMAP_setup_box`** — shadow box из per-prim AABB. Заменён инлайном
  в `SMAP_person_gpu` через per-bone bind-space AABB.
- **`SMAP_shadow_prim_cache_reset`** + вызов в `clear_prims`
  (per-prim cache больше нет — нечего сбрасывать).

**Что добавлено (~400 строк):**

- **`skin_shadow_vert.glsl`** — новый shadow-vertex shader. Читает
  `a_position` (bind-space), `a_bones`/`a_weights` (multi-bone palette
  indices/weights), применяет `u_skin[bone*3..bone*3+2]` (та же
  layout что у `skin_world_vert.glsl`) + `u_shadow_proj` (orthographic
  world→shadow-clip). Прозрачно поддерживает multi-bone soft-skinning
  (`world = Σ w_i × (skin[bones_i] × bind_pos)`).
- **`s_program_shadow_sil`** теперь линкуется к `SHADER_SKIN_SHADOW_VERT`
  + старый `SHADER_SHADOW_SIL_FRAG` (frag без изменений). Uniform
  `s_ss_u_xform` → `s_ss_u_skin` (та же сигнатура `vec4 u_skin[N*3]`
  что у body шейдера).
- **`ge_shadow_silhouette_draw(mesh, skin_palette, bone_count, shadow_proj16)`**
  — сигнатура переименована (palette → skin_palette,
  palette_n → bone_count). Принимает full skin-палитру вместо
  single-bone world affine. Один draw call на персонажа.
- **`FIGURE_get_skin_mesh_for_thing`** (public helper в figure.h/.cpp)
  — возвращает consolidated mesh + per-bone bind-space AABB + chunk +
  bind_inv для любого скиннингового Thing (15-bone person или
  flat-skeleton creature). Lazy-builds TPO + bind-space VBO если
  ещё не построены. Используется `SMAP_person_gpu` (если тень бежит
  раньше body draw в кадре — triggers builds сам).
- **`figure_tpo_build_person`** (static helper) — factored-out
  TPO walk из `FIGURE_draw_hierarchical_prim_recurse`. Используется
  обоими путями (body + shadow).
- **`figure_person_iIndex`** (static helper) — factored-out iIndex
  расчёт для D3DPeopleObj[] (PersonType + PersonID → cache slot).
- **Per-bone bind-space AABB** на `TomsPrimObject::skin_consolidated_bone_aabb`
  (`bone_count × 6 floats`). Считается раз при `figure_build_consolidated_skin_world`
  группировкой вершин по `bones[0]`. Освобождается в `FIGURE_clean_LRU_slot`.

**Регрессионный bugfix в P2-G.7 follow-up (попутно поймали при P2-H гейте):**

- **Crash on mission reload**: `figure_anim_obj_get_consolidated` возвращал
  evicted-but-keyed cache слот без rebuild → `FIGURE_touch_LRU_of_object`
  ассертил по `bLRUQueueNumber == 0xff`. Поправлено: после поиска по
  ключу проверяется `wNumMaterials == 0` и слот перестраивается.
- **Stale cache на mission unload**: `FIGURE_clean_all_LRU_slots` теперь
  дополнительно обнуляет `D3DAnimObjKeys[]` и вызывает
  `bind_palette_invalidate_all()` — указатели на chunk пересекаются
  между миссиями (heap reused), а контент chunk'а свежий.

**Файлы коммита (P2-H):**
- `new_game/CMakeLists.txt` — embed skin_shadow_vert.glsl, drop shadow_sil_vert.glsl.
- `new_game/src/buildings/prim.cpp` — drop SMAP_shadow_prim_cache_reset() call.
- `new_game/src/buildings/prim_types.h` — `skin_consolidated_bone_aabb` поле + count.
- `new_game/src/engine/graphics/geometry/bind_palette.cpp` — комментарий освежён.
- `new_game/src/engine/graphics/geometry/figure.{cpp,h}` — TPO factoring,
  `FIGURE_get_skin_mesh_for_thing` хелпер, per-bone AABB build, регрессионный
  fix `figure_anim_obj_get_consolidated`, mission-end cleanup.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp` —
  shadow program → skin_shadow_vert, ge_shadow_silhouette_draw API
  обновлён под skin палитру.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/shadow_sil_vert.glsl`
  — **удалён**.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_world_vert.glsl` —
  обновлены комментарии (a_bone теперь legacy unused).
- `new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h` —
  ge_shadow_silhouette_draw сигнатура + GESkinVertex.bone комментарий.
- `new_game/src/engine/graphics/lighting/smap.cpp` — массивный rewrite
  `SMAP_person_gpu` (один draw per character через консолидированный
  путь), удалены `s_prim_shadow_mesh[]`/`s_prim_shadow_min/max[]`/
  `smap_get_prim_shadow_mesh`/`smap_bone_world`/`SMAP_setup_box`/
  `SMAP_shadow_prim_cache_reset`.
- `new_game/src/engine/graphics/lighting/smap.h` — drop
  `SMAP_shadow_prim_cache_reset` declaration.

**Что осталось от Фазы 2:**
- **P2-I** — отражения в лужах через тот же skin path (сейчас CPU-soft).
- **P2-B** — texture array (perf upgrade).

После P2-I + P2-B — Фаза 2 закрыта.

---

### 2026-05-20 — P2-G.7 follow-up: muzzle flash flat-color path + fog anchor

В первом проходе P2-G.7 поймал два бага на гейте у пользователя:

1. **Муззл-флэш потускнел / пропал у пистолета и AK** (у дробовика
   остался виден). Регрессия: `figure_draw_held_item` рисовал прим
   261/262/263 через `MESH_draw_poly_at_matrix` (= `MESH_draw_guts`)
   с per-vertex NIGHT-освещением. В оригинале муззл-флэш-прим рисуется
   через **другой** software-путь — POLY_buffer accumulation с
   хардкоженым `pp->colour = 0xff808080` (плоский серый) + аддитивная
   текстура. На ночных сценах NIGHT-освещение даёт тёмный модуляющий
   цвет → пламя гасится в чёрный. Дробовик визуально пережил потому
   что его текстура муззл-флэша на порядок ярче, и модуляция всё ещё
   читается.

   Починено: добавлен `figure_draw_muzzle_flash(prim)` —
   точная копия legacy WITHIN(prim,261,263) ветки из
   `FIGURE_draw_prim_tween_person_only`. `figure_draw_held_item`
   диспатчит: 256/258/260 → MESH_draw_poly_at_matrix (lit гун),
   261/262/263 → `figure_draw_muzzle_flash` (unlit плоский серый).
   Гун-муззл pos write для FX остаётся как было.

2. **Цивилианы/бандиты иногда чернеют от тумана** (на Insane Assault
   например). Регрессия P2-G.7: после удаления `_just_set_matrix`-цикла
   в `FIGURE_draw_hierarchical_prim_recurse` больше не устанавливался
   `g_matWorld` для каждого перса. Per-material loop читает
   `g_mm_fog_view_z = g_matWorld._43` — для безоружных персов читалось
   stale-значение от предыдущего draw (другого перса, или leftover из
   меню), которое могло быть далеко за far-fog → персонаж в чёрном
   тумане. У армированных персов g_matWorld успевал перезаписаться
   через `MESH_draw_poly_at_matrix` (= POLY_set_local_rotation на руку),
   но это hand-bone позиция, а не root — слегка неточно.

   Починено: перед `figure_build_screen_xform_bake` явно якорим
   `g_matWorld` через `POLY_set_local_rotation(current[0].pos_x,
   current[0].pos_y, current[0].pos_z, identity_matrix)` —
   то же что в `ANIM_obj_draw`. Save/restore в screen_xform_bake
   сохраняет этот anchor для последующего fog-чтения.

Оба бага пользователь подтвердил починенными. Билд чистый (302/302).
Готов коммит «P2-G.7: delete OLD shader infrastructure + held-item
fixes + fog anchor».

**Оставшиеся pre-existing баги (не моя зона, чинить отдельно после
коммита):**
- Огонь из ствола (муззл-флэш) рисуется на 1 кадр и в высоких FPS
  визуально слабый. Регрессия из коммита fps-unlock. Лочка на 25 FPS
  делает вспышку снова яркой.
- Искры в точке попадания пуль (и в стволе дробовика) очень мелкие.
  Регрессия из коммита 6129d4554673 (downsized grenade particles).
  Уменьшение применилось не только для гранаты, но и для остальных
  spark-эффектов.

---

### 2026-05-20 — P2-G.7: удалён весь OLD shader infrastructure ✓ invariant §2.1 выполнен

**Состояние репо:** ветка `optimize-fps`, билд чистый (302/302). После
визуального гейта P2-G тела (Балрог/люди/не-люди визуально 1:1) — снесён
весь оставшийся легаси код. Теперь в коде ровно **один** GPU skin shader
(`skin_world_vert.glsl`) на ВСЁ что движется костями (люди, не-люди, в
будущем — тени и отражения). Cleanup-коммит готов.

**Что снесено физически (net −1900 строк, удалены файлы/функции/глобалы):**

- **Шейдер**: `skin_vert.glsl` (файл) + `SHADER_SKIN_VERT` (CMake embed).
- **GL state**: `s_program_skin` + все 26 `s_sk_u_*` uniforms +
  `s_vao_skin` (core.cpp).
- **GL API**: `ge_draw_skinned`, `ge_skin_mesh_draw`,
  `skin_bind_and_set_uniforms` (core.cpp).
- **Pipeline API**: `ge_draw_multi_matrix`, `GEMultiMatrix` struct,
  `GEMMVertexType` enum, `s_skin_palette`/`_n`/`_lightdirs`/`_fadetable`/
  `_unlit`/`_cache_slot` accumulators + `mm_flush_skin` (polypage.cpp).
- **Figure functions**: `FIGURE_draw_prim_tween`,
  `FIGURE_draw_prim_tween_person_only`,
  `FIGURE_draw_prim_tween_person_only_just_set_matrix`,
  `FIGURE_draw_hierarchical_prim_recurse_individual_cull`,
  `figure_skin_cache_slot` (figure.cpp).
- **Figure globals**: `MM_pMatrix`, `MM_pNormal`, `MMBodyParts_pMatrix`,
  `MMBodyParts_pNormal` + их align-storage + MMLightingTableInit
  (figure_globals.cpp/h).
- **TomsPrimObject**: поле `skin_gpu` + связанная free-logic в
  `FIGURE_clean_LRU_slot` (prim_types.h, figure.cpp).
- **FIGURE_draw else-fallback** для non-15-bone person'ов
  (заменено ASSERT — все люди 15-bone).

**Что добавлено для замены OLD path:**

- **`MESH_draw_poly_at_matrix`** (mesh.cpp/h) — variant of `MESH_draw_poly`
  принимающий готовую матрицу вместо yaw/pitch/roll. Используется для
  оружия/муззл-флэша в руке: ригидный prim рисуется по кости hand из
  pose snapshot. Гранатат уже использовал `MESH_draw_poly` (через
  identity rotation), но gun/muzzle нужна полная ротация кости —
  поэтому отдельный API.
- **`figure_draw_held_item`** (figure.cpp, static) — хелпер для
  оружия/муззл-флэша. Читает `pose[hand_part]` из render_interp,
  конструирует rotation matrix, рисует через MESH_draw_poly_at_matrix.
  Для gun-prim'ов 256/258/260 ещё считает muzzle-точку для gunfire FX
  (хардкоженый индекс вершины muzzle — preserved от легаси per-bone
  пути).
- **`FIGURE_draw_hierarchical_prim_recurse` целиком переписан** — было
  ~390 строк с двумя обходами скелета (TPO build + per-bone _just_set_matrix
  цикл) + ge_draw_multi_matrix fallback. Стало ~180 строк: один обход
  для TPO build (тот же что был) + figure_draw_held_item для оружия
  + world-skin draw loop (был и раньше, теперь без OLD-fallback else
  ветки).

**Invariant §2.1 — выполнен:**
- ✅ ОДИН skin shader на GPU — `skin_world_vert.glsl`.
- ✅ Тело людей + тело не-людей идут через тот же `ge_skin_world_draw_range`.
- ✅ Авто-риг (5-pair bidirectional) применяется только к 15-bone людям
  (`bone_count == POSE_PERSON_BONE_COUNT` гейт в
  `figure_build_consolidated_skin_world`), не-люди получают тривиальные
  веса (1 кость на вершину) автоматически.
- ✅ Оружие/муззл-флэш/граната в руке — это **жёсткие** прибитые-к-кости
  меши, не скининг → они идут через `MESH_draw_poly_at_matrix`
  (rigid-body path, тот же что у статичной геометрии). Это **не**
  нарушение invariant'а — invariant про skinning path, а это
  rigid-body. Один skinning path и один rigid path = две явных
  ортогональных категории, чисто.

**Что осталось от Фазы 2 (по плану §6, в порядке приоритета):**
- **P2-H** — тени через ТОТ ЖЕ soft skin path (сейчас тени per-prim через
  `s_prim_shadow_mesh`+shadow_sil_vert.glsl — отдельный shader).
- **P2-I** — отражения в лужах через ТОТ ЖЕ skin path (сейчас CPU-soft).
- **P2-B** — texture array (perf-апгрейд, не блокирует).

**Файлы коммита:**
- `new_game/src/engine/graphics/geometry/figure.{cpp,h}` — основная работа.
- `new_game/src/engine/graphics/geometry/figure_globals.{cpp,h}` — снос MM globals.
- `new_game/src/engine/graphics/geometry/mesh.{cpp,h}` — MESH_draw_poly_at_matrix.
- `new_game/src/buildings/prim_types.h` — снос skin_gpu.
- `new_game/src/engine/graphics/pipeline/polypage.{cpp,h}` — снос ge_draw_multi_matrix + GEMultiMatrix.
- `new_game/src/engine/graphics/pipeline/aeng_globals.h` — снос forward-декларации GEMultiMatrix.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp` — снос s_program_skin + uniforms + ge_draw_skinned + ge_skin_mesh_draw + skin_bind_and_set_uniforms.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_vert.glsl` — **удалён**.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_world_vert.glsl` — комменты освежены, убраны ссылки на удалённый skin_vert.
- `new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h` — снос ge_draw_skinned + ge_skin_mesh_draw деклараций.
- `new_game/CMakeLists.txt` — снос embed skin_vert.glsl.

---

### 2026-05-20 — P2-G тело: не-люди (Bane / Balrog / летучие мыши / Гаргулья) на новый soft skin path ✓ подписано

**Состояние репо:** ветка `optimize-fps`, билд чистый (302/302). Пользователь
подтвердил визуальный гейт: Балрог отрисовывается корректно, люди тоже —
регрессий нет. Готов коммит «P2-G: non-people through unified world-skin path».

**Что сделано:**

1. **`bind_palette` обобщён на произвольное число костей.**
   API сменился: `bind_palette_get(chunk_ptr, ..., out_bone_count)` —
   принимает указатель на `GameKeyFrameChunk` вместо `anim_type` индекса.
   Кэш `s_cache[MAX_PALETTE_CACHE=16]` ключуется указателем на chunk
   (линейный поиск; для ~5 chunks игры с большим запасом). Build
   функция ветвится по `chunk->ElementCount`:
   - **15 костей** (DARCI/ROPER/ROPER2/CIV) → существующая логика
     A-позы + FK (`build_palette_person`).
   - **Любое другое N** ≤ `POSE_MAX_BONES=20` → плоский билд
     (`build_palette_flat`): identity rotation, translation =
     `float(ae0[i].OffsetX/Y/Z)` (raw keyframe-0 offsets без FK
     chain, без scale — масштаб живёт только в per-frame `current`).

2. **`figure_build_consolidated_skin_world` стал универсальным.**
   Принимает указатель на chunk. Для не-людей `bMatIndex` ASSERT
   проверяется по `bone_count` рига. Авто-риг (5-pair bidirectional
   blend) гейтится `if (bone_count == POSE_PERSON_BONE_COUNT)` — на
   плоских скелетах он no-op (HEAD=11 / HAND=7,10 / FOOT=3,14 индексы
   к ним не относятся, плюс защита от ложных совпадений).

3. **Кэш консолидированных не-people TomsPrimObject'ов.**
   Новый `D3DAnimObj[MAX_NUMBER_D3D_ANIMALS=32]` + параллельный
   `D3DAnimObjKeys[]` (key = chunk_ptr + start_object). Линейный
   поиск, lazy-build через ту же TPO machinery что у людей
   (`FIGURE_TPO_init_3d_object` / `add_prim_to_current_object` с
   `ubSubObjectNumber=i` / `finish_3d_object`).

4. **`TPO_MAX_NUMBER_PRIMS` поднят 16 → 20** (= `POSE_MAX_BONES`,
   `MAX_NUM_BODY_PARTS_AT_ONCE`). Балрог имеет >16 частей —
   старый предел вызывал ASSERT в `FIGURE_TPO_add_prim_to_current_object`
   при первом появлении Балрога в кадре.

5. **`ANIM_obj_draw` полностью переписан.**
   Раньше: цикл по 0..ele_count-1 с `FIGURE_draw_prim_tween` на каждую
   часть → старый shader через `ge_draw_multi_matrix`.
   Теперь: один консолидированный VBO на (chunk, start_object), один
   `ge_skin_world_draw_range` на материал — **тот же path что у
   15-bone people**.
   - Веса тривиальные (`w0=255`, остальное 0) — каждая вершина к своей
     одной кости. Авто-риг к не-людям не применяется.
   - Per-frame скин-палитра: `current[i] × inv_bind[i]` для всех
     `bone_count` костей (FK не нужен — `compose_flat_skeletal_pose` уже
     даёт world transforms с body angles + scale).
   - Перед `figure_build_screen_xform_bake` ставим `g_matWorld` на
     `current[0]` (~root bone в world space) → корректный per-character
     `g_mm_fog_view_z` (после restore в bake-функции).
   - Lighting: тот же `BuildMMLightingTable(NULL, 0xffff00ff)` +
     `MM_pcFadeTable` что был в OLD пути, lightdir scaled by 251
     (как у людей).

6. **`figure_build_skin_world_palette` обобщён.**
   Сигнатура теперь `(current, inv_bind, bone_count, out_palette*)` —
   принимает указатель + длину вместо фиксированного `[POSE_PERSON_BONE_COUNT * 12]`.
   Skeleton debug-overlay гейтится `if (... && bone_count == POSE_PERSON_BONE_COUNT)`
   — для плоских скелетов отключен (там нет `body_part_parent[]`).

**Что осталось на P2-G.7 (cleanup, отдельный коммит):**

После этого коммита OLD shader infrastructure (`skin_vert.glsl`,
`s_program_skin`, `ge_skin_mesh_draw`, `ge_draw_skinned`,
`ge_draw_multi_matrix`, `MM_pMatrix`, `MM_pNormal`,
`MMBodyParts_pMatrix`) **физически ещё в коде**. Её всё ещё используют:

- **`FIGURE_draw_prim_tween_person_only`** — оружие в руке, муззл-флэш,
  граната. Рисуется через старый `ge_draw_multi_matrix`. Это жёсткие
  меши на одной кости (= кость руки). План: завернуть в тот же
  `skin_world_vert.glsl` с палитрой из 1 кости (= кость руки) или
  через generic rigid-body API.
- **`FIGURE_draw` else-fallback** (line ~3262) для non-15-bone
  person'ов — мёртвая ветка (все люди в игре 15-bone), но остаётся
  до явной чистки.
- **`FIGURE_draw_prim_tween`** — без живых callers после P2-G тела
  (ANIM_obj_draw больше не вызывает), но функция ещё в коде (через
  `FIGURE_draw` else).

P2-G.7 — следующий коммит: завернуть held items + удалить весь
OLD path. После этого invariant §2.1 = «один soft путь на ВСЁ» выполнен.

**Файлы коммита:**
- `new_game/src/engine/graphics/geometry/bind_palette.{h,cpp}` —
  API сменился на chunk-ptr + bone_count, добавлен flat branch.
- `new_game/src/engine/graphics/geometry/figure.cpp` —
  generalized `figure_build_skin_world_palette` и
  `figure_build_consolidated_skin_world`, переписан `ANIM_obj_draw`,
  новый хелпер `figure_anim_obj_get_consolidated`.
- `new_game/src/engine/graphics/geometry/figure_globals.{h,cpp}` —
  `D3DAnimObj[]` + `D3DAnimObjKeys[]` cache, `TPO_MAX_NUMBER_PRIMS` 16→20.
- `known_issues_and_bugs/known_issues_and_bugs.md` — 2 пользовательских
  баг-репорта добавлены (тень полицейской машины с квадратом на
  Headline Hostage, MIB death-light поверх столбов).

---

### 2026-05-20 — Handoff: P2-J частично выполнен (people-only), P2-G следующий

**Состояние репо:** ветка `optimize-fps`. People на NEW soft пути работают;
HEAD parent blend применяется с дефолтами `band=20, w_max=1.0`. Билд чистый.
Готовится коммит «P2-J cleanup (partial) + handoff to P2-G».

**Что снесено в этой сессии (P2-J partial cleanup):**
- Все 3 дебаг-кнопки скиннинга: K (T-pose), F (hard/soft), R (OLD/NEW path).
- Все live-тюнер кнопки (Shift+стрелки band/w_max ±, Backspace parent/child).
- Persistent skin tuner overlay (`skin_tune_overlay_render`).
- 3 глобала: `g_skin_world_path_enabled`, `g_skin_soft_rig_enabled`, `g_tpose_override_enabled`.
- T-pose A-pose FK override блок (~52 строки) в `render_interp.cpp`.
- OLD consolidated person builder `figure_build_consolidated_skin` (~140 строк).
- `FIGURE_invalidate_all_skin_consolidated_world` — больше нечем инвалидировать.
- `ge_skin_mesh_draw_range` (OLD consolidated draw — без callers).
- Поле `skin_consolidated` (bone-local) из `TomsPrimObject`.
- Все диагностические `diag_log` toggles в figure.cpp.
- Итого: ~943 удалённых строк / 283 добавленных в 11 файлах.

**Что осталось от OLD path (НЕ удалено) — гарантирует работу не-людей:**
- `s_program_skin`, `skin_vert.glsl`, `s_sk_u_*` uniforms.
- `skin_bind_and_set_uniforms`, `ge_skin_mesh_draw`, `ge_draw_skinned`.
- `ge_draw_multi_matrix` → polypage.cpp → старый шейдер.

Это используется как **fallback streaming-путь** для **не-15-bone ригов**:
Балрог, летучие мыши, Bane, Гаргулья. Они не имеют bind_palette (она
хардкоднута на 15-bone person rig) и не могут идти через `ge_skin_world_draw_range`.

**Багфикс в этой сессии (важно — не повторить):**
`figure_build_consolidated_skin_world` (NEW builder) **полагался на OLD builder
для заполнения `skin_consolidated_ranges`** — массива индексных слайсов по
материалам. Когда я снёс OLD builder, NEW остался без ranges → драв-путь
`if (use_world_path && skinRanges)` падал в else → Дарси шла в fallback
(OLD streaming) даже будучи 15-bone person. Зафиксировано: NEW builder теперь
сам аллоцирует и заполняет `ranges_storage` в конце.

**Дефолты HEAD parent зашиты в код, тюнер удалён:**
В `bind_palette.cpp:138-142`:
```cpp
SkinTuneGroup g_skin_tune_groups[SKIN_TUNE_GROUP_COUNT] = {
    /* HEAD  */ { 20.0f, 1.0f, 0.0f, 0.0f },  // band=20, w_max=1.0
    /* HANDS */ { 0.0f, 0.0f, 0.0f, 0.0f },
    /* FEET  */ { 0.0f, 0.0f, 0.0f, 0.0f },
};
```

Тюнер удалён — параметры теперь только через правку этих констант + ребилд.
Алгоритм 5-pair bidirectional blend остаётся, читает значения из массива.
Известная проблема (швы на запястьях/щиколотках не лечатся универсально)
задокументирована в `known_issues_and_bugs.md` как Post-1.0 — обусловлена
отсутствием intermediate upper-body кости в скелете.

**Следующая сессия — план P2-G (обязательный, §6 строки 467-484):**

Цель: распространить NEW soft skin path на не-людей (Балрог / летучие мыши
/ Bane / Гаргулья). После P2-G — удалить остаток OLD shader infrastructure.

Что делать:
1. **Расширить `bind_palette` на произвольные риги** — сейчас хардкод
   `POSE_PERSON_BONE_COUNT = 15`. Нужно поддержать N костей по факту
   AnimType: `MaxElements` чанка. Bind-палитра для не-15-bone:
   - Bind rotation = identity (flat скелет, нет FK иерархии).
   - Bind position = bone rest offset из keyframe 0.
   - `inv_bind = inverse(bind)`.
2. **Распространить `figure_build_consolidated_skin_world` на не-людей.**
   Для них:
   - Веса вершин — **trivial**: `(w0=1.0, w1..w3=0)`, `bones[0] = bMatIndex`.
   - **Авто-риг к ним НЕ применяется** (он про человеческую анатомию,
     5-pair таблица в figure.cpp бьёт по индексам 15-bone person rig).
3. **Per-frame skin palette** для них — `current[i] = world_pos[i]`
   из render_interp без FK chain (flat скелет — каждая кость свой
   world transform независимо).
4. **Расширить шейдер `skin_world_vert.glsl`** на бóльший
   `GE_SKIN_MAX_BONES` если 15 не хватит. У Балрога / летучих мышей
   bone count неизвестен — проверить.
5. **Гейт визуально 1:1** со старым flat-rigid рендером — пользователь
   должен подтвердить что Балрог / летучие мыши / Bane / Гаргулья
   выглядят как раньше.
6. После гейта: **удалить весь OLD shader infrastructure**:
   - `s_program_skin`, все `s_sk_u_*` uniforms.
   - `skin_vert.glsl` файл.
   - `skin_bind_and_set_uniforms`, `ge_skin_mesh_draw`, `ge_draw_skinned`.
   - Соответствующие пути в `polypage.cpp` / `ge_draw_multi_matrix`.

После P2-G останется **один soft путь на ВСЁ** = invariant §2.1 выполнен.

**Файлы текущей итерации (что меняется в коммите):**
- `new_game/src/buildings/prim_types.h` — удалено поле `skin_consolidated`.
- `new_game/src/engine/graphics/geometry/bind_palette.h/.cpp` — удалены два глобал-тоггла, упрощены комменты.
- `new_game/src/engine/graphics/geometry/figure.cpp` — удалён OLD consolidated builder, упрощён draw-путь, исправлен ranges bug, удалён `FIGURE_invalidate_all`.
- `new_game/src/engine/graphics/geometry/figure.h` — удалена `FIGURE_invalidate_all_skin_consolidated_world`.
- `new_game/src/engine/graphics/render_interp.h/.cpp` — удалён `g_tpose_override_enabled` + весь A-pose FK блок.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp` — удалена `ge_skin_mesh_draw_range`.
- `new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h` — удалена декларация `ge_skin_mesh_draw_range`.
- `new_game/src/game/game_tick.cpp` — удалены 3 кнопки + весь F-тюнер + manipulator + overlay-вызов.
- `known_issues_and_bugs/known_issues_and_bugs.md` — записаны 2 post-1.0 issue (skinning seams + N cycler missing female civ).

---

### 2026-05-20 — Handoff: P2-D подписан, P2-E работает но шов шеи требует тюнинга

**Состояние репо:** ветка `optimize-fps`. P2-A / P2-C / P2-D / P2-E
закоммичены. Билд чистый.

**Где мы:** P2-E (auto-rig + soft weights) работает end-to-end — pipeline
доходит до GPU, видна разница между NEW hard и NEW soft. НО: визуальное
улучшение слабее ожидаемого. Шов на шее (HEAD↔TORSO) еле сглаживается
даже с bidirectional блендингом (§4.7). Нужно либо крутить параметры,
либо менять алгоритм. **Это первое что делать в следующей сессии.**

**Что подписано пользователем:**
- P2-D: «Все можно проверил, разницы не вижу между 2мя версиями» —
  гейт прошёл, multi-bone vertex format готов под soft (trivial веса
  дают 1:1 с baked).
- P2-E pipeline работает: 3-state тумблер F (OLD baked → NEW hard →
  NEW soft) различим визуально на конечностях.

**Что НЕ подписано (требует доделки в следующей сессии):**
- Шея у Дарси НЕ сглаживается заметно даже с bidirectional блендингом.
  «Разница в шее есть но очень небольшая» — пользователь.
- Параметры `SOFT_BAND_FRACTION=0.4`, `SOFT_W_MAX=0.5` — стартовые из
  плана §4.5. Возможно надо `W_MAX=0.7..0.8`, `BAND_FRACTION=0.6..0.8`
  чтобы шов реально размылся. Алгоритм может также требовать апгрейда —
  возможно нужно блендить более 2 костей у сложных стыков (шея =
  HEAD+TORSO+(возможно PELVIS через цепь)).

**Текущий 3-state тумблер F** (живёт до P2-J):
- 0: OLD skinning - hard rig (`g_skin_world_path_enabled=false`)
- 1: NEW skinning - hard rig (`world_enabled=true, soft_enabled=false`)
- 2: NEW skinning - soft rig (оба true)
- При переходе soft↔ не-soft инвалидируется кэш `skin_consolidated_world`
  через `FIGURE_invalidate_all_skin_consolidated_world()`.
- Клавиша **K** (A-поза override) и **N** (model cycler) тоже живы —
  полезны для статической сверки.

**Реализованные нюансы (важно для будущих изменений):**

1. **`AnimKeyFrames[0].FirstElement == NULL` — sentinel pattern.**
   В bind_palette.cpp `build_palette` сканируем `chunk.MaxKeyFrames`
   до первого keyframe с непустым FirstElement. Без этого
   `bind_palette_get` возвращал false для ВСЕХ anim_type, и весь
   world path молча падал на baked fallback. Урок: не предполагать
   что слот 0 живой в чанковых структурах.

2. **MM_vLightDir подаётся `× 251` на CPU.** В шейдере остаётся
   `dot(N, L) / 251`, как и в legacy `skin_vert.glsl`. Без этого
   множителя `cos_nl` уменьшался в 251 раз и весь модель уплывал в
   серединку фейд-таблицы. Сделано в figure.cpp при заполнении
   `lightdir_world[]` (константа `MM_LIGHT_SCALE = 251.0f`).

3. **`normalize(world_normal)` в шейдере.** Произведение нескольких
   fixed-point матриц может дать чуть больше единичной длины (drift),
   плюс соft-блендинг невесомых нормалей даёт sub-unit длину. Без
   нормализации `wrap` уплывал за [0,1] и при определённых ракурсах
   клампился в idx=0 → весь персонаж чернел. Сейчас норм-стандарт.

4. **`g_matWorld` save/restore в `figure_build_screen_xform_bake`.**
   Функция внутри делает `POLY_set_local_rotation_none()` чтобы
   получить camera-only вариант g_matWorld для бейка проекции. Если
   не восстановить — последующий per-material loop читает
   `g_mm_fog_view_z = g_matWorld._43` и получает camera-offset вместо
   character-view-Z. При повороте камеры фог считал расстояние от
   камеры → персонаж смыкался к чёрному. Save/restore + повторный
   `ge_set_transform(World, ...)` фиксит.

5. **Конвенция матриц.** GEMatrix в bind_palette используется в
   «M·v» форме: R в верхнем 3×3, t в **столбце 4** (`_14, _24, _34`).
   ОТЛИЧАЕТСЯ от GEMatrix в `g_matWorld` / `g_matProjection` где R^T
   в верхнем 3×3, t в **строке 4** (`_41, _42, _43`). Это контейнерный
   контейнер — данные внутри GEMatrix интерпретируются по-разному в
   разных контекстах. Не путать.

**Текущий алгоритм soft rig** (`figure_build_consolidated_skin_world`,
3 pass'а под `if (g_skin_soft_rig_enabled)`):
- Pass A: для каждой кости `bone_length[i]` = max расстояния вершин
  до её origin в bind-space.
- Pass B (§4.5): child→parent. Для каждой не-pelvis вершины:
  `t = dist_to_own_joint / band[own]`, `w_parent = (1-t)*W_MAX`,
  `w_own = 1 - w_parent`. PELVIS остаётся w0=255.
- Pass C (§4.7): parent→child. Для каждой вершины ищем ближайший
  child joint в пределах его band'а; если найден — забираем кусок из
  weights[0] в weights[2] и в bones[2] кладём этого ребёнка.

Параметры: `SOFT_BAND_FRACTION = 0.4f`, `SOFT_W_MAX = 0.5f`,
`MIN_BAND = 0.001f` (защита от div-by-zero).

**Diagnostic infrastructure (живёт до P2-J):**
- `u_diagnostic_color` uniform в `common_frag.glsl` + early-exit при
  alpha > 0.5. По умолчанию (0,0,0,0) — pipeline не активен. Можно
  пере-армировать в core.cpp если нужна цветовая диагностика
  (синий/красный/зелёный = baked/hard/soft).
- `diag_log` лямбда в `FIGURE_draw_hierarchical_prim_recurse` — пишет
  в stderr.log причину skip world path для каждого (TPO, причина)
  pair'а один раз. Не спамит.
- `bp_diag` лямбда в `bind_palette.cpp build_palette` — пишет
  причину сбоя bind palette построения.

**Следующая сессия — план:**

1. Прочитать этот файл целиком (§2.1 invariant, §4 алгоритм soft,
   §6 этапность, §7 риски, эту запись).
2. Запустить игру, переключить F в state 2 (NEW soft), посмотреть на
   Дарси с пистолетом (как на скриншотах в чате). Нагрузить параметры
   `SOFT_W_MAX` (0.5 → 0.7 → 0.9), `SOFT_BAND_FRACTION` (0.4 → 0.6 →
   0.8). Файл [figure.cpp](../new_game/src/engine/graphics/geometry/figure.cpp),
   функция `figure_build_consolidated_skin_world`, поиск
   `SOFT_BAND_FRACTION` / `SOFT_W_MAX`. После каждого изменения нужна
   ПЕРЕСБОРКА VBO — при включённом soft нажать F несколько раз чтобы
   попасть в hard и обратно в soft, тем самым инвалидируя кэш (или
   быстрее: добавить горячую клавишу для invalidate без toggle).
3. Если тюнингом не закрывается — апгрейд алгоритма. Идеи:
   (a) Расширить blend на третью кость (PELVIS→TORSO→HEAD цепочка
       для шейных вершин);
   (b) Использовать `smoothstep` вместо линейного `(1-t)`;
   (c) Перейти от distance-к-joint на distance-к-bone-segment-axis
       (более точно для длинных костей).
4. Когда пользователь подпишет soft rig визуально — закрыть P2-E
   гейт. Двигаться в **P2-G** (не-люди на новый путь, trivial веса).

**Файлы итерации P2-E:**
- [figure.cpp](../new_game/src/engine/graphics/geometry/figure.cpp) — 3
  pass'а soft, `FIGURE_invalidate_all_skin_consolidated_world`,
  `figure_build_screen_xform_bake` save/restore, MM_LIGHT_SCALE.
- [figure.h](../new_game/src/engine/graphics/geometry/figure.h) —
  объявление `FIGURE_invalidate_all_skin_consolidated_world`.
- [bind_palette.{h,cpp}](../new_game/src/engine/graphics/geometry/bind_palette.cpp) —
  `g_skin_soft_rig_enabled` toggle, sentinel keyframe scan, диагностика.
- [skin_world_vert.glsl](../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_world_vert.glsl) —
  normalize(world_normal).
- [common_frag.glsl](../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/common_frag.glsl) —
  u_diagnostic_color early-exit.
- [core.cpp](../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp) —
  u_diagnostic_color locations × 3 программы.
- [game_tick.cpp](../new_game/src/game/game_tick.cpp) — 3-state F toggle.

### 2026-05-20 — Architectural invariant зафиксирован + P2-C подписан

**Invariant §2.1 добавлен:** в итоге Фазы 2 ОДИН skin path на всё (тело,
тени, отражения, люди, не-люди). Никаких rigid-фолбэков. Не-люди идут
через тот же runtime path с trivial весами (single-bone эффективно),
авто-риг к ним не применяется (только для людей). Раньше план в §9
ошибочно оставлял старый rigid путь для не-15-bone скелетов — исправлено.

**Этапность §6 переработана:**
- **P2-B** — НЕ отложен. Просто сдвинут по приоритету в конец (перед
  P2-J): сначала переносим всё на soft skin путь, потом перф-апгрейд
  texture array.
- **P2-F** — удалён как отдельный шаг. Debug-тумблеры (K, F, hard/soft)
  живут до самого конца (полезны для A/B-сверки на P2-G/H/I/B) и
  удаляются вместе со всем старым кодом на P2-J.
- **P2-G** (обязательный) — не-люди (Балрог, бат, Bane, Гаргулья)
  через ТОТ ЖЕ soft skin path с trivial весами (`w0=1`). Авто-риг к
  ним не применяется, но runtime — один на всё.
- **P2-H** (обязательный) — тени через ТОТ ЖЕ skin path с альтернативной
  финальной проекцией (shadow-clip). Soft-веса деформируют тень
  автоматически.
- **P2-I** (обязательный) — отражения в лужах через ТОТ ЖЕ skin path
  с зеркальной проекцией. Soft-веса автоматически.
- **P2-B (выполнение)** (обязательный, после P2-I) — texture array /
  atlas, перф-апгрейд.
- **P2-J** (обязательный) — удаление **ВСЕГО** старого кода. После
  этого этапа в коде остаётся только новый soft skinning путь на GPU.
  Включает: старый рендер тела, старые тени, старые отражения, все
  debug-тумблеры (K/F/hard-soft). Без этого этапа Фаза 2 НЕ
  завершена.

**P2-C подписан** пользователем («Все что можно проверил - разницы не
вижу между 2мя версиями»). Что вошло:
- `bind_palette.{h,cpp}` — статическая bind-палитра A-позы (15 костей,
  ±30° Z на плечах), кэш per anim_type. Конвенция "M·v" (R в верхнем
  3×3, t в столбце 4) — отличается от GEMatrix engine row-vector, но
  локально консистентна.
- `skin_world_vert.glsl` — новый шейдер: bind-space verts + per-bone
  world skin (`current × inv_bind`) + per-character screen bake
  (camera·projection·viewport). Perspective-divide/NDC математика
  дословно скопирована из `skin_vert.glsl` для 1:1.
- `figure_build_consolidated_skin_world(pPrimObj, anim_type)` —
  параллельный bind-space меш. Поле `skin_consolidated_world` на
  TomsPrimObject.
- `s_program_skin_world` + `ge_skin_world_draw_range` — GL API. Шейдер
  embed'нут в CMake.
- Тумблер `g_skin_world_path_enabled` на клавише **F** — A/B между
  baked (P2-A) и world (P2-C). По умолчанию off. ВРЕМЕННЫЙ —
  удаляется на P2-J вместе со всем старым путём.

**Гейт P2-C закрыт.** Идём в P2-D (расширение vertex format под 4 веса,
эффективно ещё single-bone).

### 2026-05-20 — Handoff на P2-C (для следующей сессии)

**Старт следующей сессии:** прочитать этот документ целиком (особенно §3
наработки, §4 алгоритм, §5 архитектура, §6 этапность, §7 риски).
Состояние репо: P2-A закоммичен, ветка `optimize-fps`. Идём в **P2-C**.

**Что разведано в этой сессии при подходе к P2-C (важные находки):**

1. **`MMBodyParts_pMatrix` — это BAKED матрица «World × Projection ×
   Viewport»**, не чистый world. Это видно по существующему
   `skin_vert.glsl` (`new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/skin_vert.glsl`)
   который делает странный `zbuf = 1 - ZCLIP/sw` потому что палитра
   уже спроецировала вершину. **Для multi-bone это не годится** —
   блендить вершины в screen-space нельзя (проекция нелинейна).
   Нужны чистые world-матрицы + проекция в конце.

2. **Источник чистых world-матриц — `render_interp_get_cached_pose(Thing*)`**
   (render_interp.cpp:1127). Возвращает `BoneInterpTransform[15]`
   (pos + Matrix33 rot, всё в world space, БЕЗ проекции). Уже
   используется тенями (smap_bone_world). Идеальный источник для
   `current_palette` в новом пути.

3. **Глобалы для view × projection доступны:** `g_matWorld` (= view
   matrix в обычной терминологии, см. poly.cpp:596+), `g_matProjection`
   (см. poly.cpp:362-377). Это то что нужно в новый шейдер uniform'ом
   вместо baked palette.

4. **Bind-палитра считается per-chunk один раз.** Источник: per-bone
   offsets из любого keyframe чанка (`chunk->AnimKeyFrames[0].FirstElement[i].OffsetX/Y/Z`),
   локальные повороты = identity для всех костей кроме плеч (5, 8) где
   ±30° по Z (см. §3.2). FK от PELVIS в model-space (origin = 0).

5. **Существующий код T-позы override** в render_interp.cpp
   (ветка `if (g_tpose_override_enabled)` в `render_interp_compute_pose`)
   делает РОВНО ту FK что нужна для bind-палитры — A-поза с identity +
   ±30° на плечах. **Можно почти дословно скопировать математику**
   при написании `compute_bind_palette()`.

**Конкретный план первого шага P2-C:**

P2-C.1 (изолированный, тестируемый):
- Новая функция `compute_bind_palette(int anim_type, GEMatrix out_world[15], GEMatrix out_inv_bind[15])`.
- Источник offsets: `game_chunk[anim_type].AnimKeyFrames[0].FirstElement[i].Offset*`.
- A-поза: identity local rot для всех + ±30° Z для 5 и 8.
- FK от пелвиса в (0,0,0), identity world rot.
- inv_bind[i] = инверсия (для rigid body: inv_R = R^T, inv_T = -R^T × T).
- Кэш per (anim_type), 3 слота (DARCI/ROPER/CIV).
- Применять только когда `chunk->ElementCount == 15` (отбраковать
  балрога и пр. — у них своя структура).

P2-C.2 (новый шейдер, существенный):
- Новый файл `skin_world_vert.glsl` — принимает `u_skin_matrix[15]`
  (3 vec4 каждая = mat3 rot + vec3 pos, 60 floats), `u_view` (mat4),
  `u_projection` (mat4). Логика: `world_pos = skin_matrix[bone] × bind_pos`,
  `clip_pos = projection × view × world_pos`. Лайтинг/туман — копия
  существующего `skin_vert.glsl` с поправкой что direction теперь в
  world space.
- Нормаль: тоже трансформ скин-матрицы (rot часть).
- Подключить в core.cpp как `s_program_skin_world`.

P2-C.3 (vert format + build):
- Расширить `GESkinVertex`: вершины теперь в **bind-space**, не
  bone-local. Удобно сохранить старое поле `bone` (= bMatIndex) для
  выбора матрицы в шейдере.
- В `figure_build_consolidated_skin`: после копирования vert'a из
  pPrimObj, домножить позицию на `bind_palette[bMatIndex]` → теперь
  vert в общем bind-space. Нормаль тоже трансформ.
- НЕ менять размер struct если можно (44 байт остаются). Просто
  семантика поля `position` меняется с bone-local на bind-space.

P2-C.4 (draw path):
- Per-frame compute `skin_matrix[15] = current[i] × inv_bind[i]` на
  CPU. Источник current — `render_interp_get_cached_pose(p_person)`.
- Передать в шейдер uniform'ом.
- Передать `g_matWorld` (view) и `g_matProjection`.
- Заменить вызов `ge_skin_mesh_draw_range` на новый `ge_skin_world_draw_range`
  (новый API, использует новый шейдер).

P2-C.5 (A/B тумблер):
- Debug-флаг `g_skin_world_path_enabled` (или клавиша) — переключает
  между старым baked-путём (P2-A) и новым world-путём (P2-C).
- Гейт: визуально 1:1. Особенно проверить: перекрытия со стенами/
  машинами вблизи (z-precision), персонаж в катсцене, ночь/туман.

**Минимальный test sequence для подписи 1:1:**
- Дарси стоит — должна выглядеть идентично.
- Дарси бежит — должна выглядеть идентично.
- В катсцене (RTA intro) — идентично.
- Ночью в тумане — идентично.
- Близкий план с перекрытиями стен — без z-fight'а.

**Риски (что наиболее вероятно сломается):**
- Порядок умножения матриц (column vs row major у `Matrix33`).
- Z-precision при новой проекции — может не совпасть с inverse-Z из
  старого шейдера. Возможно понадобится sync'нуть z-write конвенцию.
- Нормали: если в bind-палитре есть scale (не должно быть, но
  проверить) — нужен inverse-transpose. Иначе rot достаточно.

**Что НЕ менять в P2-C:**
- Геометрию vert format размер: 44 байт остаются (P2-D расширит).
- Веса/риг: single-bone, w0=1 эффективно. Это P2-E.
- Тени/отражения: не трогать (они идут через `SMAP_person_gpu` и
  отдельный CPU-путь, не зависят от skin-палитры).

### 2026-05-20 — P2-A сделан и подписан + P2-B отложен

**P2-A сделан и подписан** пользователем («Вроде все ок»). Что вошло:
- Новое API `ge_skin_mesh_draw_range` (core.cpp + game_graphics_engine.h).
- Поля `skin_consolidated` + `skin_consolidated_ranges` на `TomsPrimObject`
  ([prim_types.h](../new_game/src/buildings/prim_types.h)); init в
  `FIGURE_TPO_init_3d_object`, free в `FIGURE_clean_LRU_slot`.
- Функция `figure_build_consolidated_skin` (figure.cpp) — собирает все
  материалы pPrimObj в один VBO/EBO; декодирует strip-индексы в triangle
  list, оффсетит вершины по материалам, записывает per-material
  `(start, count)` диапазоны.
- Draw-loop в `FIGURE_draw_hierarchical_prim_recurse` — пробует
  консолидированный путь, fallback на старый `ge_draw_multi_matrix` если
  build провалился. Каждый материал: `pa->RS.SetChanged()` биндит свою
  текстуру, потом `ge_skin_mesh_draw_range` со своим диапазоном из
  общего меша. Эффект: draws на персонажа упали с ~15-30 до ~3-5.
- `individual_cull` путь, тени, отражения — НЕ тронуты (остались на
  старой схеме).

**P2-B отложен (см. этап выше).** Идём в P2-C (bind-pose & skin
matrices) — это первая ступень к soft rig'у, не требует texture array.

### 2026-05-20 — План создан

Зафиксирована вся информация полученная в обсуждении: угол A-позы (30° Z,
±знаки на плечах), факт single-bone в iter-1 (не 4-вес как в дизайне),
артефакты MIB-плаща и висящих кистей, выбор алгоритма (топология +
falloff), архитектура (vertex format, bind-pose, texture array, skin
matrices), этапность P2-A..F. Ждём отмашки пользователя на старт P2-A.
