# Character lighting fix (Stage 12, 2026-04-12)

Краткая запись по фиксу плоского освещения персонажей. Полное расследование — [`stage12_character_lighting_investigation.md`](../new_game_planning/stage12_character_lighting_investigation.md).

## Проблема

Персонажи в нашей сборке отображались плоско освещёнными — без светлой/тёмной стороны. В релизной PC и в PieroZ-сборке освещение работает.

## Корневая причина (двойная)

1. **Историческая:** в пре-релизных исходниках MuckyFoot software fallback `DrawIndPrimMM` был заглушкой Tom Forsyth'а (`// I don't actually do lighting yet — just make sure it's visible`). Реальное освещение делал hardware D3D6 driver через расширение `D3DDP_MULTIMATRIX`, software путь писал `color = 0xffffffff`. При портировании в Stage 7 мы унаследовали эту заглушку 1:1 как `ge_draw_multi_matrix`.

2. **Технический баг:** каст `mm->lpvVertices` к `GEVertexLit*` был неправильным — данные на самом деле содержат `GEVertex` (с нормалями). Offsets `x/y/z/u/v` случайно совпадают в обеих структурах, поэтому геометрия рисовалась корректно, но поля `color`/`specular` интерпретировали байты как будто это были цвета, а на самом деле это были float `ny`/`nz`.

## Фикс

Реализовано per-vertex software освещение в `ge_draw_multi_matrix` ([polypage.cpp:646](../new_game/src/engine/graphics/pipeline/polypage.cpp#L646)):

1. Добавлен параллельный указатель `GEVertex* pVert` для чтения нормалей
2. Для каждой unlit-вершины: `dot(normal_bone_local, lightdir_bone_local)` → индекс в 128-entry fade table → цвет
3. Применён **half-Lambert** wrap (Valve/HL2 техника): `wrap = cos × 0.5 + 0.5`, индекс в лит-рампе 0..63 по всей модели

## Грабли

**Критическая ошибка по пути:** неправильный масштаб нормалей. В [building.cpp:6485](../new_game/src/buildings/building.cpp#L6485) комментарий: "prim_normal stored at length 256". Значит умножение на `1/256` в figure.cpp даёт **unit vector**, а не tiny 1/256-vector. Моя первая формула `cos = dot × 256/251` была в 256 раз больше нужного — `cos_nl` получался ~251 для любой лит-вершины, что клиппилось в `idx = 63` для лит-стороны или `idx = 64` для теневой. Получалось **бинарное освещение** вместо плавного градиента, и поэтому множитель (64/80/192) не влиял визуально.

Как нашли: хардкодили `idx = 0` и `idx = 63` чтобы проверить есть ли range в таблице. Range был, но вся модель уже саму была в idx=63 или idx=64 из-за overflow в формуле.

Правильная формула: `cos_nl = dot_raw / 251.0f` — получаем чистый косинус в [-1, 1].

## Почему half-Lambert, а не классический Lambert

После фикса масштаба чистый Lambert дал корректное освещение, но персонажи визуально читались как "слишком тёмные" относительно окружения. Причины:

- Теневая сторона → idx 64 (flat ambient, cutoff)
- Вертексы под скользящим углом к свету (cos ≈ 0.1..0.3) → idx 6..19, почти ambient → большая часть модели тёмная
- Окружение (здания, машины) использует другой lighting path и остаётся ярким → персонаж "проваливается" в сцену

Half-Lambert решает это напрямую: `wrap = cos × 0.5 + 0.5` маппит весь диапазон косинуса на 0..1, все вертексы получают индекс в лит-рампе, нет резкого cutoff'а. Передняя сторона всё так же fully lit (idx 63), терминатор на idx 32 (половинная яркость), задняя сторона на idx 0 (чистый ambient). Результат — мягкий градиент без "пропадающей" теневой стороны, персонаж визуально интегрируется в сцену.

Это стандартная техника для character shading с HL2 (2004). Документация: https://developer.valvesoftware.com/wiki/Half_Lambert

## Почему не как в релизе

Релизная PC и PieroZ (D3D6 emulation) воспроизводят hardware byte-quantized освещение 1999 года — резкие переходы, пересвеченная лит-сторона ("пластиковые" персонажи). Пользователь визуально сравнил обе версии и предпочёл нашу: в релизе персонажи выглядят чрезмерно переосвещёнными и оторванными от окружения (environment при этом освещён нормально), у нас — более естественно и гармонично.

Мы уже отходили от канона релиза ради улучшения визуала (AA, mipmap'ы, texture filtering) — это в одном ряду. Float-точность современного железа позволяет сделать лучше чем byte-math 1999.

## Перенос на GPU (post-1.0)

Весь блок тривиально переезжает в vertex shader одной строкой:
```glsl
float cos_nl = dot(normal_bone_local, lightDir_bone_local);
float wrap = cos_nl * 0.5 + 0.5;
vec3 color = texture(fadeTable1D, wrap).rgb;  // или mix(ambient, fullyLit, wrap)
```

При переходе на GPU skinning (post-1.0) fade table можно либо загрузить как 1D текстуру (буквальный перенос), либо вообще выкинуть и считать ambient/diffuse напрямую. Текущий CPU-код — это просто software-имитация того что в шейдере делается в одну строку.

## Тюнинг контраста

Множитель `64.0f` в формуле — параметр "растянутости" лит-рампы:
- **64** (текущее) — чистый half-Lambert, натуральный вид
- 80..128 — чуть контрастнее, ближе к byte-quantized look
- 192..256 — почти бинарное on/off, резкий терминатор (близко к релизу)

Выбрано 64 как наиболее гармоничное с окружением.

## Связанные файлы

- [new_game/src/engine/graphics/pipeline/polypage.cpp](../new_game/src/engine/graphics/pipeline/polypage.cpp) — `ge_draw_multi_matrix`, сам фикс
- [new_game/src/engine/graphics/geometry/figure.cpp](../new_game/src/engine/graphics/geometry/figure.cpp) — `BuildMMLightingTable` (не тронута, работает корректно), `MM_pNormal`/`MMBodyParts_pNormal` заполнение per-bone light dirs
- [new_game_planning/stage12_character_lighting_investigation.md](../new_game_planning/stage12_character_lighting_investigation.md) — полное расследование с подробностями всех масштабов и структур
