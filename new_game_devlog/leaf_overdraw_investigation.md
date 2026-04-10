# Investigation: листья/debris рисуются поверх additive эффектов

**Дата:** 2026-04-10

## Проблема

Несколько РАЗНЫХ типов геометрии рисуются **поверх** additive эффектов (bloom фонарей, огня, дыма, взрывов). В оригинальном D3D6 рендерере (релиз PC + PieroZ) additive эффекты корректно рисуются поверх.

**Затронутые типы (это разные системы):**
1. **Листва деревьев** (canopy) — рисуется через `ge_draw_indexed_primitive_lit` в `AENG_draw_dirt`, POLY_PAGE_LEAF. Треугольники, world-space, CPU transform → immediate draw.
2. **Листья на земле** (ground debris) — рисуется тоже через `AENG_draw_dirt`, тот же immediate draw path (POLY_PAGE_LEAF). Отдельные объекты от листвы деревьев — это DIRT_TYPE_LEAF из DIRT системы.
3. **Трупы** — рисуются через отдельный путь (FIGURE_draw), но тоже поверх additive эффектов.
4. **Некоторые ограждения (fence)** — тоже рисуются поверх дыма. Расположены на Urban Shakedown у полицейского участка полукругом. Вероятно используют alpha test (для прозрачных прутьев). Большинство объектов НЕ имеют этой проблемы — затронуты только те, что используют alpha test или особый render path.

**Общий паттерн:** проблема у объектов с alpha test + DepthWrite=false (или особым render path). Обычная opaque геометрия (стены, земля, машины) рисуется нормально.

Листва деревьев и листья на земле — разные системы (DIRT vs canopy), но обе проходят через `AENG_draw_dirt` → immediate draw.

## Архитектура рендеринга

### Два пути рисования

1. **Deferred (PolyPage pipeline)** — bloom, огонь, взрывы, дым, и вся остальная геометрия. Полигоны добавляются через `POLY_add_quad/triangle` → `PolyPage` буфер → рисуются в `POLY_frame_draw()`. Alpha sort: bucket sort по Z на 2048 уровней, back-to-front (bucket 0 = далёкие, Z маленький; bucket 2047 = ближние, Z большой).

2. **Immediate (ge_draw_indexed_primitive_lit)** — листья деревьев, ground debris, снежинки. Рисуются напрямую через `AENG_draw_dirt()` → `POLY_Page[POLY_PAGE_LEAF].RS.SetChanged()` → `ge_draw_indexed_primitive_lit()`. Не попадают в PolyPage буфер, не участвуют в alpha sort.

### Порядок вызовов (одинаков в D3D6 и GL)

```
1. Prim objects (включая BLOOM_draw для фонарей) → добавляют bloom в PolyPage буфер
2. AENG_draw_bangs/fire/sparks → добавляют огонь/взрывы в PolyPage буфер
3. POLY_frame_draw(TRUE, TRUE) → flush всех PolyPage: opaque pass + alpha sort
4. AENG_draw_dirt() → immediate draw листьев через ge_draw_indexed_primitive_lit
5. POLY_frame_draw(TRUE, TRUE) → flush deferred mesh polys (cans, brass) от draw_dirt
```

Листья на шаге 4 рисуются **после** bloom/огня (шаг 3). С alpha test + no blend, opaque пиксели листа полностью перезаписывают framebuffer, включая bloom.

**В оригинале D3D6 — тот же порядок вызовов**, тот же immediate draw для листьев, тот же deferred для bloom. Но визуально bloom корректно поверх листьев.

### Z формулы — совместимы

Проверено: обе пути дают одинаковый depth value.

- PolyPage (bloom): `pv.z = 1.0 - pts->Z`, где `Z = POLY_ZCLIP_PLANE / view_z` → `pv.z = 1 - near/view_z`
- ge_draw_indexed_primitive_lit (листья): `tl.z = cz * rhw`, где cz из projection matrix → `tl.z = 1 - near/view_z`

Проекционная матрица настроена так что `cz/cw = 1 - near/z` — совпадает с кастомной POLY_perspective формулой.

## Что проверялось и не помогло

### 1. Перемещение AENG_draw_dirt до pyrotechnics
Идея: нарисовать листья до огня/bloom. Не помогло — bloom добавляется в deferred буфер ещё раньше (при рисовании prim objects), а фактически рисуется при `POLY_frame_draw`.

### 2. Перемещение AENG_draw_dirt до основного POLY_frame_draw
Идея: листья до flush. Не помогло — bloom всё ещё в deferred буфере, рисуется при flush после листьев... но тогда bloom должен быть поверх. Проблема: bloom и другие deferred polys добавляются в буфер в разных местах кода, часть до dirt, часть после.

### 3. Реверс alpha sort порядка (i=2047→0 вместо 0→2047)
Не помогло — листья вне alpha sort, реверс на них не влияет.

### 4. DepthWrite=true для POLY_PAGE_LEAF
Не помогло — листья всё равно рисуются после flush, depth write не меняет порядок отрисовки.

### 5. DepthEnabled=false для POLY_PAGE_LEAF
Не тестировалось до конца — сломает рисование листьев за стенами.

### 6. Alpha blend вместо alpha test для POLY_PAGE_LEAF
Изменён POLY_PAGE_LEAF render state на ModulateAlpha + SrcAlpha/InvSrcAlpha. Не помогло.

### 7. Defer bloom pages (частичный эффект)
Пропуск POLY_PAGE_BLOOM1/BLOOM2/FINALGLOW/LENSFLARE в alpha sort первого `POLY_frame_draw`, чтобы они остались в буфере и нарисовались во втором `POLY_frame_draw` (после dirt). **Частично помогло** — bloom от фонарей стал рисоваться поверх **листвы деревьев** (canopy). Но НЕ фиксит:
- Листья на земле (ground debris) поверх огня
- Листва деревьев поверх дыма/огня (только bloom пофикшен, остальные additive — нет)
- Трупы поверх additive эффектов
- Любые комбинации с additive эффектами которые НЕ в списке deferred pages

Это **не фикс** — это workaround для конкретных bloom pages. Проблема системная.

**Как воспроизвести defer bloom**: в `poly.cpp` добавить static bool `s_defer_bloom_pages` и skip BLOOM1/BLOOM2/FINALGLOW/LENSFLARE в alpha sort цикле когда флаг true. В `aeng.cpp` включить флаг перед первым `POLY_frame_draw`, выключить перед вторым (после dirt).

## Render state сравнение (D3D6 vs GL)

POLY_PAGE_LEAF:
- D3D6: `MODULATE`, `ZWRITEENABLE=FALSE`, `ALPHATESTENABLE=TRUE`, `ZENABLE` наследуется (TRUE)
- GL: `Modulate`, `DepthWrite=false`, `AlphaTestEnabled=true`, `DepthEnabled=true`
- **Совпадают.**

POLY_PAGE_BLOOM1:
- D3D6: `ZWRITEENABLE=FALSE`, `ALPHABLENDENABLE=TRUE`, `SRCBLEND=SRCALPHA`, `DESTBLEND=ONE`
- GL: `DepthWrite=false`, `AlphaBlendEnabled=true`, `SrcAlpha+One`
- **Совпадают.**

Alpha ref: 0x07 в обоих. Alpha func: Greater в обоих.

## Текстура листьев

`leaf.tga`: 64×64, **имеет alpha канал**. 3675 пикселей alpha=0 (прозрачные), 421 alpha=255 (opaque). Бинарный cutout — без промежуточных значений.

## Ключевое наблюдение

~~В D3D6 тот же код (тот же порядок вызовов, те же render states) даёт правильный результат. Значит разница — в реализации OpenGL рендерера.~~ **ОПРОВЕРГНУТО** — см. ниже.

## Проверка на D3D6 ветке (2026-04-10)

**Результат: проблема ПРОЯВЛЯЕТСЯ и на D3D6 бэкенде в нашем коде.**

Проверено на коммите `c1804954` (stage 7 final) с `GRAPHICS_BACKEND=d3d6`. Листья/debris рисуются поверх additive эффектов — тот же баг что и в OpenGL.

**Вывод: проблема НЕ в реализации OpenGL бэкенда.** Она в общем коде.

## Проверка на самом раннем билде (2026-04-10)

**Результат: проблема ПРОЯВЛЯЕТСЯ даже на самом начальном билде** — до любых изменений кода, до реструктуризации (Stage 4), до выделения абстракции render engine (Stage 7). Коммит сразу после перехода на clang.

## Итоговый вывод

**Это изначальный баг пре-релизных исходников MuckyFoot.** Не внесён при рефакторинге.

- В **релизной PC-версии** (Steam) проблемы нет → MuckyFoot пофиксили между пре-релизом и финальной версией
- В **PieroZ D3D6 сборке** — дым и взрывы корректно поверх листьев/крон/fences → PieroZ пофиксил. Примечание: билборды света (bloom) у PieroZ не отображаются вообще, но дым/взрывы работают правильно
- В **нашем коде** (любой коммит, любой бэкенд) проблема есть → баг в пре-релизных исходниках, не внесён нами

## Дальнейшие действия

1. **Посмотреть коммиты PieroZ** (`github.com/PieroZ/MuckyFoot-UrbanChaos`) — найти фикс этой проблемы. PieroZ работал с теми же исходниками и пофиксил.
2. Портировать фикс PieroZ в наш код.
3. Если фикс PieroZ не найдётся — сравнить рендеринг пре-релиза с финальной PC-версией (лонгплеи) для понимания что изменилось.
