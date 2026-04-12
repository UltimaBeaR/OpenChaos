# Этап 12 — Расследование: плоское освещение персонажей

**Статус:** расследование завершено, гипотеза подтверждена структурно. Фикс не сделан, ждём имплементации в отдельной итерации.

**Дата:** 2026-04-12

**Связанные баги в [known_issues_and_bugs.md](known_issues_and_bugs.md):**
- "Освещение персонажей отличается от финального релиза" (более плоское освещение, помечено "Нужно в 1.0")
- "Также 'слишком ярко ночью' на Auto Destruct" — упомянуто в том же баге, скорее всего тот же корневой баг (отсутствие directional lighting → персонаж принимает общий ambient одинаково со всех сторон, что ночью при низком ambient + сильных point lights даёт неверный результат)

После имплементации фикса проверить **обе** проблемы — они должны исчезнуть вместе.

---

## 1. Симптом

Освещение персонажей в нашей сборке плоское — нет светлой/тёмной стороны как в релизной PC-версии. Все стороны модели выглядят одинаково. У PieroZ (D3D6 build) визуально освещение работает (есть скриншот-доказательство — на персонаже видны тени на лице, теневая сторона тела).

## 2. Архитектура освещения персонажей в оригинале — "Tom's MultiMatrix"

Систему писал **Tom Forsyth** (графический инженер MuckyFoot). Он использовал нестандартное расширение Direct3D 5/6 — флаг `D3DDP_MULTIMATRIX` в `DrawIndexedPrimitive`. Это extension не входил в стандартный D3D, реализован был только частью драйверов.

### Структура `D3DMULTIMATRIX`

```c
struct D3DMULTIMATRIX {
    D3DMATRIX*  lpd3dMatrices;   // массив матриц костей (по матрице на body part)
    void*       lpvVertices;     // массив вершин (D3DVERTEX, с нормалью)
    D3DCOLOR*   lpLightTable;    // 128-entry lookup table: индекс normal → цвет
    float*      lpvLightDirs;    // направления света на каждую матрицу (в локальном пространстве кости)
};
```

### Хитрость с упаковкой вершины (важно!)

Tom Forsyth перепаковал данные вершины так, чтобы в одну `D3DVERTEX` поместить и нормаль, и индекс матрицы. Layout `D3DVERTEX` (= наш `GEVertex`):

```
offset 0..11  : x, y, z (3 × float)
offset 12..15 : nx (float)
offset 16..19 : ny (float)
offset 20..23 : nz (float)
offset 24..31 : tu, tv (2 × float)
```

Макрос `SET_MM_INDEX(vertex, idx)` **перезаписывает первый байт `nx`** значением matrix index:

```c
#define SET_MM_INDEX(v, idx) (((unsigned char*)&(v))[12] = (idx))
```

После этого:
- Byte 12 = matrix index (0..N для bone matrices)
- Bytes 13..15 = старшие три байта мантиссы float `nx` (нормаль слегка зашумлена в 1 байте, остаётся достаточно точной)
- Bytes 16..23 = `ny`, `nz` без изменений

Драйвер D3D6 при `D3DDP_MULTIMATRIX`:
1. Читает byte 12 как matrix index → берёт `lpd3dMatrices[idx]`
2. Трансформирует position через эту матрицу
3. Читает (`nx`, `ny`, `nz`) как нормаль (с микро-погрешностью в `nx`)
4. Делает per-vertex освещение через `lpLightTable` + `lpvLightDirs[matIndex]`
5. Растеризует

### `BuildMMLightingTable`

Заполняет таблицу `MM_pcFadeTable[128]` каждый кадр для каждого персонажа:
- Index 0..63: линейный ramp от ambient до full lit (для светлой стороны, dot > 0)
- Index 64..127: flat ambient colour (для теневой стороны, dot ≤ 0)

Алгоритм:
1. Найти доминантное направление света: `vTotal = sum(brightness × light_direction)` для NIGHT_amb + всех NIGHT_found point lights
2. Нормализовать `vTotal`
3. Посчитать ambient и fully-lit colours по dot products
4. Применить fudge-факторы:
   - Brightness формула: `0.35×R + 0.45×G + 0.2×B`
   - `AMBIENT_FACTOR = 0.5 × 256`
   - x2 если ambient < 100 (night fudge)
   - x2 для PC (общий boost)
5. Заполнить таблицу 128 D3DCOLOR значений

Эта функция у нас портирована **1:1** в [new_game/src/engine/graphics/geometry/figure.cpp:59-244](new_game/src/engine/graphics/geometry/figure.cpp#L59-L244). Корректна.

### `MM_pNormal` (= `lpvLightDirs`)

Per-bone light direction. На каждый body part трансформируется `MM_vLightDir` в локальные координаты кости и записывается в `MM_pNormal`. Magic множитель `fNormScale = 251.0f` — для интерпретации драйвером.

В нашем коде заполняется в [figure.cpp:2057-2066](new_game/src/engine/graphics/geometry/figure.cpp#L2057-L2066) (и аналогично в hierarchical path):
```cpp
GEVector vTemp;
vTemp.x = MM_vLightDir.x * fmatrix[0] + MM_vLightDir.y * fmatrix[3] + MM_vLightDir.z * fmatrix[6];
vTemp.y = MM_vLightDir.x * fmatrix[1] + MM_vLightDir.y * fmatrix[4] + MM_vLightDir.z * fmatrix[7];
vTemp.z = MM_vLightDir.x * fmatrix[2] + MM_vLightDir.y * fmatrix[5] + MM_vLightDir.z * fmatrix[8];

MM_pNormal[0] = 0.0f;          // padding
MM_pNormal[1] = vTemp.x * fNormScale;  // 251.0f
MM_pNormal[2] = vTemp.y * fNormScale;
MM_pNormal[3] = vTemp.z * fNormScale;
```

## 3. Software fallback `DrawIndPrimMM` — заглушка

Tom Forsyth написал software эмуляцию extension'а в [original_game/fallen/DDEngine/Source/polypage.cpp:1013](original_game/fallen/DDEngine/Source/polypage.cpp#L1013):

```c
HRESULT DrawIndPrimMM(...) {
    ...
    for (int i = 0; i < 3; i++) {
        ...
        BYTE bMatIndex = ((unsigned char*)(pLVertCur))[12];
        D3DMATRIX *pmCur = &(d3dmm->lpd3dMatrices[bMatIndex]);
        // pre-transform position через bone matrix
        pTLVert[i].dvSX = ...;
        pTLVert[i].dvSY = ...;
        pTLVert[i].dvSZ = ...;
        ...
        if (dwFVFType == D3DFVF_VERTEX) {
            // I don't actually do lighting yet - just make sure it's visible.
            pTLVert[i].dcColor    = 0xffffffff;
            pTLVert[i].dcSpecular = 0xffffffff;
        } else {
            pTLVert[i].dcColor    = pLVert[wIndex[i]].dcColor;
            pTLVert[i].dcSpecular = pLVert[wIndex[i]].dcSpecular;
        }
    }
    DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, pTLVert, ...);
}
```

**Ключевая цитата:** `// I don't actually do lighting yet - just make sure it's visible.`

Это **заглушка**. Tom написал её "чтобы было что-то видно", но реальное освещение через `lpLightTable` не реализовал — это должен был делать драйвер через hardware path.

## 4. Хронология что происходило

1. **Tom Forsyth написал hardware path**: прямой вызов
   ```c
   lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, &d3dmm,
                                  pMat->wNumVertices, pwStripIndices,
                                  pMat->wNumStripIndices, D3DDP_MULTIMATRIX);
   ```
   Драйвер D3D6 разбирал `&d3dmm`, делал освещение через `lpLightTable` + `lpvLightDirs`. На реальном PC того времени это работало.

2. **Tom написал software fallback `DrawIndPrimMM`** — заглушка с `dcColor = 0xffffffff`, без реального освещения.

3. **В пре-релизных исходниках MuckyFoot** (то что у нас в `original_game/`) hardware path **закомменчен `#if 0`**, активен software fallback. См. [original_game/fallen/DDEngine/Source/figure.cpp:4069-4128](original_game/fallen/DDEngine/Source/figure.cpp#L4069-L4128):
   ```c
   #if 0
       // hardware path with D3DDP_MULTIMATRIX — закомменчен
       hres = lp_D3D_Device->DrawIndexedPrimitive(..., D3DDP_MULTIMATRIX);
   #else
       // active software fallback path
       hres = DrawIndPrimMM(...);
   #endif
   ```
   Причина переключения неизвестна — возможно для Dreamcast, для портативности, или из-за нестабильности extension'а на части драйверов. **С момента этого переключения освещение персонажей в пре-релизе сломано** — software fallback не реализует lighting.

4. **В финальном PC-релизе** MuckyFoot эту дыру закрыли. Точно как — неизвестно (кода финала нет). Варианты:
   - Вернули hardware path
   - Дописали реальное освещение в `DrawIndPrimMM` software
   - Сделали pre-lit вершины при кэшировании mesh
   
   Любой из этих вариантов даёт визуальное освещение которое мы видим в релизной игре.

5. **Stage 7 у нас** (переход с D3D6 на OpenGL): портировали `DrawIndPrimMM` 1:1 как `ge_draw_multi_matrix` в [new_game/src/engine/graphics/pipeline/polypage.cpp:571](new_game/src/engine/graphics/pipeline/polypage.cpp#L571), заменив D3D-вызовы на `ge_*`. Заглушка переехала вместе с кодом:
   ```cpp
   if (unlit) {
       pTLVert[i].color = 0xffffffff;
       pTLVert[i].specular = fog_specular;
   }
   ```
   **Мы ничего не убирали** — освещения не было ещё в пре-релизных исходниках MuckyFoot.

6. **PieroZ v0.2.0** (тег `v0.2.0`, commit `7e9a74e`): остался на D3D6 + Windows.
   - Diff `figure.cpp` PieroZ vs `original_game/figure.cpp` показал что код **практически идентичен** — отличия только в наших `claude-ai:` комментариях и нескольких мелких compile-фиксах PieroZ (`int i` объявления, отключённые ассерты).
   - PieroZ не менял ни `DrawIndPrimMM`, ни активную ветку MM-пути.
   - `USE_TOMS_ENGINE_PLEASE_BOB = 1` в `aeng.h` — то же что в пре-релизе.
   - **Тем не менее на скриншоте у него есть объёмное освещение.**
   - Точная причина не подтверждена. Гипотеза: D3D6 driver на современной Windows как-то поддерживает Tom's MultiMatrix extension (через DirectX wrapper или legacy emulation), и hardware-логика в драйвере применяет освещение даже если код game идёт через software `DrawIndPrimMM`. Либо PieroZ всё-таки активирует какой-то другой путь который я не разглядел в `#if`-блоках. Не критично для нашего фикса — у нас OpenGL, нам в любом случае реализовывать самостоятельно.

## 5. Где именно проблема в нашем коде

**НЕ в OpenGL backend.** Это общий pipeline-слой.

Цепочка вызовов для рисования персонажа:
```
FIGURE_draw(thing)
  └─ if (ele_count == 15)  // обычные люди
       FIGURE_draw_hierarchical_prim_recurse(thing)
         └─ FIGURE_draw_prim_tween_person_only_just_set_matrix(...)
              └─ ge_draw_multi_matrix(GEMMVertexType::Unlit, &mm, ...)  ← ВОТ ЗДЕСЬ
     else
       FIGURE_draw_prim_tween(...)
         └─ ge_draw_multi_matrix(GEMMVertexType::Unlit, &mm, ...)  ← И ЗДЕСЬ
```

Оба пути приводят в одну функцию `ge_draw_multi_matrix` в [new_game/src/engine/graphics/pipeline/polypage.cpp:571](new_game/src/engine/graphics/pipeline/polypage.cpp#L571). На CPU:
1. Проходит по индексам
2. Для каждой вершины берёт bone matrix (по `bMatIndex` из byte 12)
3. Трансформирует position в screen space
4. **Заполняет color = 0xffffffff** ← вот заглушка
5. Передаёт готовые `GEVertexTL` в `ge_draw_indexed_primitive` (бэкенду)

Backend (OpenGL) получает уже готовые TL-вершины с белым цветом и честно растеризует — поэтому персонаж получается = текстура × белый = текстура без освещения.

## 6. Критический баг: неправильный каст типа вершины

При расследовании выяснилось что в `ge_draw_multi_matrix` есть **фундаментальный layout-mismatch**.

### Структуры наших вершин

[new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h:107-128](new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h#L107-L128):

```cpp
// Lit vertex (= D3DLVERTEX) — pre-lit, без нормали
struct GEVertexLit {
    float x;            // offset 0
    float y;            // offset 4
    float z;            // offset 8
    uint32_t _reserved; // offset 12  (padding to match D3DLVERTEX layout)
    uint32_t color;     // offset 16
    uint32_t specular;  // offset 20
    union { float u;  float tu; };  // offset 24
    union { float v;  float tv; };  // offset 28
};

// Unlit vertex (= D3DVERTEX) — с нормалью, без цвета
struct GEVertex {
    float x;   // offset 0
    float y;   // offset 4
    float z;   // offset 8
    float nx;  // offset 12
    float ny;  // offset 16
    float nz;  // offset 20
    union { float u;  float tu; };  // offset 24
    union { float v;  float tv; };  // offset 28
};
```

Обе структуры по 32 байта, layout по позиции совпадает в первых 12 байтах и в последних 8 (UV). Различие — middle 12 байт (offsets 12..23):
- В `GEVertexLit`: `_reserved (4 byte) + color (4 byte) + specular (4 byte)`
- В `GEVertex`: `nx (float) + ny (float) + nz (float)`

### Что хранит кэш персонажей

`FIGURE_TPO_finish_3d_object` в [new_game/src/engine/graphics/geometry/figure.cpp:1144](new_game/src/engine/graphics/geometry/figure.cpp#L1144) аллоцирует вершины как `GEVertex`:

```cpp
TPO_pVert = (GEVertex*)MemAlloc(MAX_VERTS * sizeof(GEVertex));
```

И заполняет/сравнивает поля `nx, ny, nz`:
```cpp
ASSERT(pFirstVertex[iVertIndex].nx == vert.nx);
ASSERT(pFirstVertex[iVertIndex].ny == vert.ny);
ASSERT(pFirstVertex[iVertIndex].nz == vert.nz);
```

То есть **в `pPrimObj->pD3DVertices` лежат `GEVertex` с настоящими нормалями** (не pre-lit colors).

### Что делает `ge_draw_multi_matrix`

[new_game/src/engine/graphics/pipeline/polypage.cpp:585](new_game/src/engine/graphics/pipeline/polypage.cpp#L585):

```cpp
GEVertexLit* pLVert = (GEVertexLit*)mm->lpvVertices;  // ← НЕПРАВИЛЬНЫЙ КАСТ
```

Каст `GEVertex*` к `GEVertexLit*`. Поля интерпретируются неверно:
- `pLVert->_reserved` (offset 12) на самом деле = `nx` (float)
- `pLVert->color` (offset 16) на самом деле = `ny` (float)
- `pLVert->specular` (offset 20) на самом деле = `nz` (float)

Затем:
```cpp
BYTE bMatIndex = ((unsigned char*)(pLVertCur))[12];
```

Читает первый байт offset 12 — это первый байт `nx`, **который перезаписан макросом `SET_MM_INDEX` в TPO** при компиляции mesh:
```cpp
// В FIGURE_TPO_finish_3d_object или вокруг него:
SET_MM_INDEX(vert, body_part_index);  // pишет в byte 12
```

То есть `bMatIndex` читается **корректно случайно** — из-за того что `SET_MM_INDEX` затирает первый байт `nx` matrix index'ом. Но остальные 3 байта `nx` остаются как float (с микро-погрешностью).

Bone matrix берётся правильно (`mm->matrices[bMatIndex]`) — поэтому позиции вершин трансформируются корректно, и геометрия персонажа отображается правильно. **Но освещение** — мы пытаемся читать `pLVertCur->color`, который на самом деле `float ny`. Если бы мы передавали `Lit` вместо `Unlit` — то в TL вершину писалось бы бинарное представление float `ny` интерпретированное как ULONG color. Получился бы шум, не освещение.

### Что должно быть

Нужно кастовать `mm->lpvVertices` к **`GEVertex*`** (с нормалью), читать `nx, ny, nz` (помня что первый байт `nx` перезаписан matIndex — либо использовать только `ny, nz` для dot product, либо восстановить `nx` через игнорирование старшего байта мантиссы), считать dot product с `lpvLightDirs[matIndex]`, конвертировать в индекс 0..127 и брать цвет из `lpLightTable`.

## 7. Подытоживание корневой причины

**Освещение персонажей у нас плоское по двум сложившимся причинам:**

1. **Исторически:** в пре-релизных исходниках MuckyFoot software fallback `DrawIndPrimMM` это **заглушка без освещения**. Так было задумано Tom Forsyth'ом — реальная работа лежала на hardware D3D extension. Мы унаследовали эту заглушку 1:1 при портировании в Stage 7.

2. **Технически:** наш `ge_draw_multi_matrix` делает **неправильный type cast** `GEVertex*` → `GEVertexLit*`. Поля интерпретируются как color/specular, а на самом деле это `ny, nz` нормали. matIndex читается случайно правильно из-за того что `SET_MM_INDEX` затирает первый байт `nx`.

Чтобы починить — нужно реализовать освещение которое в оригинале делал D3D6 driver: per-vertex dot product нормали с `lpvLightDirs` и lookup в `lpLightTable`. Все нужные данные у нас уже есть, надо только правильно их интерпретировать и связать.

## 8. План фикса

### Вариант A — фикс в `ge_draw_multi_matrix` (рекомендуется для 1.0)

Локальный фикс в одном месте. OpenGL backend не трогаем, формат данных не меняем. Объём — десятки строк кода.

Шаги:
1. **Изменить каст** `mm->lpvVertices` с `GEVertexLit*` на `GEVertex*`.
2. **Достать нормаль** из `(nx, ny, nz)` с учётом того что byte 12 (первый байт `nx`) затёрт matIndex'ом. Возможные подходы:
   - Реконструировать `nx` обратно через `nx = sqrt(1 - ny² - nz²)` со знаком из старого `nx` (но знак потерян, можно хранить отдельно или принять что байт точности не критичен)
   - Обнулять старший байт мантиссы перед использованием (потеря 1 байта точности — допустимо)
   - Альтернативно: переделать упаковку, хранить matIndex в другом месте — но это меняет TPO, бэкенд и многое ещё, не подходит для локального фикса
3. **Трансформировать нормаль** через bone matrix `mm->matrices[bMatIndex]` (rotation part) — нормаль из object space переводится в world space.
4. **Взять light direction** из `mm->lpvLightDirs` — это уже трансформированный в локальное пространство тела вектор, scaled на `fNormScale = 251.0f`. Нормализовать обратно (поделить на 251) если нужно.
5. **Посчитать dot product** между нормалью вершины и light direction (с правильным знаком — учесть что в `BuildMMLightingTable` направления "negative" для NIGHT lights).
6. **Конвертировать dot в индекс 0..127**:
   - Если dot > 0: `index = (int)(dot × 63)` — берётся ramp 0..63
   - Если dot ≤ 0: `index = 64 + (int)(...)` — flat ambient 64..127
   - Точную формулу определить экспериментально, проверяя визуально что соответствует release-версии
7. **Брать цвет** из `mm->lpLightTable[index]` вместо `0xffffffff`.

Это путь который Tom Forsyth должен был дописать в `DrawIndPrimMM` но не дописал.

### Вариант B — GPU skinning (после 1.0)

Перенести скиннинг персонажей на GPU полностью:
- VBO с position+normal+matIndex для каждой вершины
- Bone matrices в UBO
- Все вычисления (transform + lighting) в вершинном шейдере
- Освещение можно сделать настоящим per-pixel вместо table lookup, плюс легче добавить нормал-маппинг и т.п.

Правильнее архитектурно, быстрее (CPU цикл по вершинам уходит), даёт возможность улучшать графику в будущем. Но требует:
- Нового пути в OpenGL бэкенде
- Переписывания `ge_draw_multi_matrix` или замены на новый API
- Возможно — изменение формата TPO кеша

Слишком много для 1.0. После релиза — да, желательно перенести.

## 9. Пункты которые нужно проверить при имплементации

1. **Точный формат `MM_pNormal` / `lpvLightDirs`** — это `float[4]` (padding + xyz) per body part, или массив на все body parts? В `FIGURE_draw_prim_tween` заполняется только индексы 0..3 (4 float) — это для одной кости. В hierarchical path найти где заполняется для каждой кости — там должен быть массив `[N × 4]`.

2. **Что значит `fNormScale = 251.0f`** — magic number для драйвера? Проверить есть ли соответствие в `BuildMMLightingTable` (там скейлы `fColourScale = (1/(256×128)) × 255 × NIGHT_LIGHT_MULTIPLIER` и `/64.0f` для делителя). 251 близко к 255 — возможно это нужно для правильного маппинга dot product → диапазон [0..63].

3. **Знак dot product** — комментарии в оригинале говорят: `// REMEMBER THAT ALL THE LIGHT DIRECTIONS ARE NEGATIVE, EXCEPT FOR THE AMBIENT ONE, WHICH IS CORRECT.` Нужно учесть это при вычислении.

4. **`SET_MM_INDEX` определение** — найти где определён, в каком файле, что именно перезаписывает. В оригинале вокруг строк 4165 PieroZ. У нас должно быть в figure_globals или похожем.

5. **Pre-lit path** — в `ge_draw_multi_matrix` есть `else` ветка для `GEMMVertexType::Lit`, которая использует `pLVert->color` как уже посчитанный цвет. Нужно проверить вызывается ли она вообще для персонажей и кто её вызывает (не персонажи, а что-то ещё — например объекты с уже посчитанным per-vertex освещением).

6. **Тестовые сцены для проверки** — после фикса проверить:
   - Дневная миссия снаружи (например начало Target UC) — должно появиться объёмное освещение с тенями со стороны от солнца
   - Ночная миссия (Auto Destruct) — проверить что не "слишком ярко" больше
   - Интерьер с point lights — проверить что направленный свет от ламп ложится правильно
   - Сравнить визуально со скриншотами/видео релизной PC-версии

7. **Регрессия для не-персонажей** — `ge_draw_multi_matrix` используется ещё в [aeng.cpp:3086](new_game/src/engine/graphics/pipeline/aeng.cpp#L3086) с `GEMMVertexType::Lit` для отрисовки floor (с уже посчитанным освещением). Этот путь трогать не должно, но проверить что не задели.

## 10. Связанные файлы для изменения

Основной файл фикса:
- [new_game/src/engine/graphics/pipeline/polypage.cpp:571](new_game/src/engine/graphics/pipeline/polypage.cpp#L571) — функция `ge_draw_multi_matrix`

Возможно потребуется глянуть/изменить:
- [new_game/src/engine/graphics/geometry/figure.cpp:59-244](new_game/src/engine/graphics/geometry/figure.cpp#L59-L244) — `BuildMMLightingTable` (проверить scale/диапазон)
- [new_game/src/engine/graphics/geometry/figure.cpp:1144](new_game/src/engine/graphics/geometry/figure.cpp#L1144) — `FIGURE_TPO_finish_3d_object` (проверить как вершины упакованы)
- [new_game/src/engine/graphics/geometry/figure.cpp:2057-2066](new_game/src/engine/graphics/geometry/figure.cpp#L2057-L2066) — заполнение `MM_pNormal` (light direction в локальном space кости)
- [new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h:107-128](new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h#L107-L128) — структуры `GEVertex`/`GEVertexLit`

Не трогать:
- OpenGL backend
- Формат данных в `pPrimObj->pD3DVertices`
- `BuildMMLightingTable` (она корректна)

## 11. Что НЕ удалось установить (открытые вопросы)

1. **Точно как PieroZ получает освещение.** Diff показал что код практически идентичен пре-релизу MuckyFoot. Гипотеза: D3D6 driver на современной Windows эмулирует Tom's MultiMatrix extension. Не критично — мы реализуем сами для OpenGL.

2. **Какой именно путь использовали MuckyFoot в финальном PC-релизе** — hardware вернули, software дописали, или pre-lit вершины. Кода финала нет.

3. **Где определён `SET_MM_INDEX` у нас** (если вообще портирован) — нужно найти при имплементации.

4. **Точная формула dot → index** для эмуляции hardware lookup — придётся подбирать визуально.

## 11a. Ответы на вопросы из секций 9 и 11 (расследование 2026-04-12, продолжение)

После грепа по коду все технические вопросы из секции 9 закрыты. Из секции 11 остались только два вопроса про чужой код (PieroZ, финальный релиз MuckyFoot) — они не блокируют имплементацию.

### Формат `lpvLightDirs` (вопрос 9.1)

`float[N × 4]` со страйдом 4. Layout одной записи: `[pad, x, y, z]`. Индексация: `lpvLightDirs[matIdx*4 + 1..3]` = xyz light direction в **bone-local** пространстве.

Два варианта аллокации в коде:
- **`MM_pNormal`** ([figure_globals.cpp:213](../new_game/src/engine/graphics/geometry/figure_globals.cpp#L213)): одна запись (4 float). Используется в `FIGURE_draw_prim_tween` и других не-hierarchical путях, где все вершины ссылаются на одну bone matrix (matIdx = 0).
- **`MMBodyParts_pNormal`** ([figure_globals.cpp:215](../new_game/src/engine/graphics/geometry/figure_globals.cpp#L215)): `MAX_NUM_BODY_PARTS_AT_ONCE × 4` floats. Используется в hierarchical path для людей. Заполняется в [figure.cpp:3919](../new_game/src/engine/graphics/geometry/figure.cpp#L3919) как `pnorm = &MMBodyParts_pNormal[matIdx << 2]`.

В обоих случаях формула трансформации одна и та же ([figure.cpp:2057-2066](../new_game/src/engine/graphics/geometry/figure.cpp#L2057-L2066)): `M^T × MM_vLightDir` (транспонирование upper-3x3 bone matrix), что для ортонормальной матрицы эквивалентно `M^(-1) × MM_vLightDir` — то есть направление света переводится из world space в bone-local space. Поскольку нормаль вершины тоже хранится в bone-local space, dot product корректен без дополнительных трансформаций.

### Магия `fNormScale = 251.0f` (вопрос 9.2)

Объяснилась через парную magic-константу при кэшировании вершин:
- Вершинные нормали в TPO-кэше хранятся scaled на **`1.0f / 256.0f`** ([figure.cpp:1330](../new_game/src/engine/graphics/geometry/figure.cpp#L1330)).
- Per-bone light direction scaled на **`251.0f`**.
- Raw dot product вершинной нормали с per-bone light dir ≈ `(n · l) × 251/256 ≈ 0.98 × (n · l)` — почти настоящий косинус.
- Истинный косинус восстанавливается умножением: `cos = dot_raw × 256/251`.

Магия 251 была выбрана так чтобы byte-quantized dot product в hardware D3D-драйвере укладывался в индекс 0..63 таблицы. У нас float — можем считать честно через `× 256/251` и не зависеть от квантования.

### Знак dot product (вопрос 9.3)

Дополнительно негировать **не нужно**. Все знаки уже учтены внутри `BuildMMLightingTable` при формировании дoминантного направления `vTotal`:
- Для NIGHT_amb (солнце/небо) знак положительный: `vTotal += fBright × NIGHT_amb_norm`
- Для NIGHT_found (point lights) знак отрицательный: `vTotal -= fBright2 × nf->d`

К моменту сохранения в `MM_vLightDir = vTotal` ([figure.cpp:129](../new_game/src/engine/graphics/geometry/figure.cpp#L129)) направление уже корректно и указывает "куда свет идёт". Per-vertex код просто делает `dot(normal_local, lightdir_local)` — никаких инверсий.

### `SET_MM_INDEX` у нас (вопрос 9.4 / 11.3)

Определён в [polypage.h:188](../new_game/src/engine/graphics/pipeline/polypage.h#L188):
```cpp
#define SET_MM_INDEX(v, i) (((unsigned char*)&v)[12] = (unsigned char)i)
```
Идентичен оригиналу. Применяется в [figure.cpp:1366](../new_game/src/engine/graphics/geometry/figure.cpp#L1366) при компиляции TPO mesh (записывает индекс body part) и 8 раз в [aeng.cpp](../new_game/src/engine/graphics/pipeline/aeng.cpp) для не-hierarchical случаев (всегда `0`).

**Реконструкция `nx`:** byte 12 — это младший байт мантиссы float `nx`. С учётом scale = 1/256, `nx ≈ 0.001..0.004`, и затирание младшего байта мантиссы даёт погрешность порядка 10⁻¹⁰. Это ниже шума любого квантования. **Можно читать `nx` как float как есть**, без реконструкции.

### Pre-lit `else` ветка (вопрос 9.5)

`GEMMVertexType::Lit` ветка в `ge_draw_multi_matrix` используется только для floor draw в [aeng.cpp:3225](../new_game/src/engine/graphics/pipeline/aeng.cpp#L3225) с `lpvLightDirs = NULL`. Это путь с предрассчитанными per-vertex цветами — фикс её не касается.

### Структура `MM_pcFadeTable[128]` и формула dot → index (вопрос 9.6 / 11.4)

Из чтения [figure.cpp:149-220](../new_game/src/engine/graphics/geometry/figure.cpp#L149-L220):
- **idx 0** = pure ambient (`cvAmb × fColourScale`)
- **idx 0..63** = линейный ramp от ambient до fully lit, шаг = `cvLight × fColourScale / 64`
- **idx 63** = fully lit (`(cvAmb + cvLight) × fColourScale`)
- **idx 64..127** = flat ambient (заполнено одним значением, по сути дублирует idx 0)

Старт-формула индексации (точное соответствие "честному" Lambert lookup):
```cpp
float cos = dot_raw * (256.0f / 251.0f);
int idx;
if (cos >= 0.0f) {
    idx = (int)(cos * 64.0f);
    if (idx > 63) idx = 63;
} else {
    idx = 64;  // shadow side: flat ambient
}
DWORD color = ((DWORD*)mm->lpLightTable)[idx];
```

Это эквивалент того что hardware D3D-драйвер делал внутри. Если визуально получится не как в релизной PC — будем подбирать множитель `64.0f` (например `* 80` для более контрастного освещения).

### Тестовые сцены (вопрос 9.6)

Без изменений из секции 9 — после фикса проверить:
- Дневная миссия снаружи (Target UC) — должны появиться объёмные тени со стороны от солнца
- Ночная миссия (Auto Destruct) — проверить что не "слишком ярко" больше
- Интерьер с point lights — направленный свет от ламп должен ложиться правильно
- Сравнить со скриншотами/видео релизной PC-версии

### Регрессия для не-персонажей (вопрос 9.7)

Подтверждено: `ge_draw_multi_matrix` для floor draw идёт через `Lit` ветку, которую фикс не трогает. Для всех остальных случаев `Unlit` (`SET_MM_INDEX` со значением 0 в `aeng.cpp`) фикс будет работать корректно — там lpvLightDirs указывает на `MM_pNormal` (одна запись), и matIdx=0 даст ту же одну light direction для всех вершин.

### Открытые вопросы из секции 11 — статус

1. **Как PieroZ получает освещение** — остаётся гипотезой (D3D6 driver на современной Windows эмулирует Tom's MultiMatrix). Не блокирует, мы реализуем сами для OpenGL.
2. **Что MuckyFoot сделали в финальном PC-релизе** — не узнать, кода нет. Не блокирует.
3. **`SET_MM_INDEX` у нас** — ✅ закрыт (см. выше).
4. **Точная формула dot → index** — частично закрыт: есть стартовая формула эквивалентная честному Lambert; финальный множитель подбирается визуально после имплементации.

---

## 11b. Принятые решения (2026-04-12)

### Выбор варианта: A

Делаем **Вариант A** — локальный software-фикс в `ge_draw_multi_matrix`. Десятки строк, OpenGL backend и формат данных не трогаем. Это закрытие дыры которую Tom Forsyth должен был дописать в `DrawIndPrimMM`, но не дописал.

Вариант B (GPU skinning) откладываем на после 1.0 — архитектурно правильнее, но слишком много изменений для релиза.

### План имплементации (финальный)

В [polypage.cpp:571](../new_game/src/engine/graphics/pipeline/polypage.cpp#L571) `ge_draw_multi_matrix`, ветка `unlit`:

1. Параллельно с существующим `GEVertexLit* pLVert` (нужен для path с pre-lit вершинами floor) добавить `GEVertex* pVert = (GEVertex*)mm->lpvVertices;` — для unlit path. Поля `x/y/z/u/v` имеют одинаковые offset'ы в обеих структурах, так что текущие чтения position/UV не ломаются.
2. В unlit-ветке заменить `pTLVert[i].color = 0xffffffff;` на:
   ```cpp
   GEVertex* pVertCur = pVert + wVertIndex;
   float nx = pVertCur->nx;  // младший байт мантиссы затёрт matIdx — погрешность ~1e-10, игнорируем
   float ny = pVertCur->ny;
   float nz = pVertCur->nz;

   float* pLightDirs = (float*)mm->lpvLightDirs;
   float lx = pLightDirs[bMatIndex * 4 + 1];
   float ly = pLightDirs[bMatIndex * 4 + 2];
   float lz = pLightDirs[bMatIndex * 4 + 3];

   float dot_raw = nx * lx + ny * ly + nz * lz;
   float cos = dot_raw * (256.0f / 251.0f);  // undo fNormScale × (1/256) packing

   int idx;
   if (cos >= 0.0f) {
       idx = (int)(cos * 64.0f);
       if (idx > 63) idx = 63;
   } else {
       idx = 64;  // shadow side: flat ambient
   }
   pTLVert[i].color = ((DWORD*)mm->lpLightTable)[idx];
   pTLVert[i].specular = fog_specular;
   ```
3. Lit-ветку (`else`) не трогаем — она используется для floor draw в [aeng.cpp:3225](../new_game/src/engine/graphics/pipeline/aeng.cpp#L3225) с pre-lit per-vertex цветами.
4. Если визуально получится недостаточно контрастно — играть с множителем `64.0f` (например `80.0f` или `96.0f`) или с `256.0f / 251.0f`.

### Безопасность доступа к `lpvLightDirs`

`MM_pNormal` — одна запись (4 float), `MMBodyParts_pNormal` — массив `[N×4]`. Функция не знает что именно ей передали. Это безопасно потому что:
- Non-hierarchical путь (`MM_pNormal`) всегда имеет `bMatIndex = 0` для всех вершин (см. `SET_MM_INDEX(*pv, 0)` в [aeng.cpp](../new_game/src/engine/graphics/pipeline/aeng.cpp)).
- Hierarchical путь (`MMBodyParts_pNormal`) имеет настоящие per-bone matIdx, и массив достаточно большой.

### План тестирования

Тестировать на трёх сценах последовательно (запускать только когда предыдущая работает корректно):

1. **Первая миссия Target UC** (дневная улица, начало кампании). Главная проверка: на Дарси должно появиться объёмное освещение с явной светлой/тёмной стороной. Солнце сильное и стабильное, нет point lights, есть много референсного видео из релизной PC-версии для сравнения. Если базовое Lambert работает — здесь это будет видно сразу.
2. **Auto Destruct (ночь)**. Проверка второго симптома: "слишком ярко ночью". По гипотезе он должен исчезнуть автоматически вместе с основным фиксом (потому что плоское освещение усредняло по всем направлениям и выдавало завышенный результат при сильных point lights в тёмной сцене).
3. **Любой интерьер с лампами**. Проверка что направленный свет от point lights ложится на персонажа правильно (светлая сторона смотрит на лампу).

Пользователь умеет загрузить любую сцену и в нашей сборке, и в релизной PC. Скриншоты для попиксельного сравнения буду просить по факту, после первого билда.

---

## 12. После имплементации

Когда фикс будет сделан:
1. Обновить запись в [known_issues_and_bugs.md](known_issues_and_bugs.md) — переместить "Освещение персонажей отличается от финального релиза" в [known_issues_and_bugs_resolved.md](known_issues_and_bugs_resolved.md), указать ссылку на этот файл и коммит.
2. Перепроверить второй симптом — "слишком ярко ночью на Auto Destruct". Он скорее всего исчезнет вместе с основным фиксом, но если нет — отдельно расследовать.
3. Записать решение коротко в `new_game_devlog/` для истории.
4. Этот файл расследования можно либо оставить как есть (исторический документ), либо переместить в devlog.
