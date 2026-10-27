# Формат анимаций — Urban Chaos

**Отдельных файлов анимаций нет.** Анимации встроены в `.IMP`/`.MOJ` файлы моделей.
**Ключевые файлы:** `fallen/Editor/Headers/Anim.h`, `fallen/Source/Anim.cpp`, `fallen/Source/animtmap.cpp`, `fallen/Source/morph.cpp`

---

## Система анимации: Vertex Morphing

Оригинал использует **vertex morphing** (не скелетную анимацию):
- Нет костей и скинов
- Персонаж = набор отдельных мешей-сегментов тела
- Анимация = интерполяция позиций вершин между keyframe'ами
- `DrawTween` — промежуточное состояние для рендеринга

---

## Структуры данных

### GameKeyFrameElement — элемент кости в кадре

```c
// Editor/Headers/Anim.h
struct GameKeyFrameElement {
    struct CMatrix33 CMatrix;         // сжатая матрица ротации 3×3
    SWORD OffsetX, OffsetY, OffsetZ;  // смещение от родительского элемента
    UWORD Next;                        // следующий элемент в цепочке
    UWORD Parent;                      // родительский элемент (иерархия)
};
```

### GameKeyFrame — ключевой кадр

```c
struct GameKeyFrame {
    UBYTE XYZIndex;                    // индекс для быстрого поиска
    UBYTE TweenStep;                   // скорость tween к следующему кадру
    SWORD Flags;                       // флаги кадра
    GameKeyFrameElement *FirstElement; // цепочка элементов (костей)
    GameKeyFrame *PrevFrame;           // предыдущий кадр в анимации
    GameKeyFrame *NextFrame;           // следующий кадр в анимации
    GameFightCol *Fight;               // хитбоксы боя (только у кадров удара)
};
```

### DrawTween — полная структура (drawtype.h)

```c
typedef struct {
    UBYTE TweakSpeed;        // скорость воспроизведения (множитель)
    SBYTE Locked;            // -1=разблокировано, 0+=заблокировано на sub-object
    UBYTE FrameIndex;        // текущий кадр в последовательности
    UBYTE QueuedFrameIndex;  // индекс кадра в очереди

    SWORD Angle, AngleTo;    // текущий и целевой угол поворота
    SWORD Roll, DRoll;       // крен и дельта-крен
    SWORD Tilt, TiltTo;      // наклон и целевой наклон

    SLONG CurrentAnim;       // ID текущей анимации (ANIM_*)
    SLONG AnimTween;         // прогресс интерполяции: 0→255 между кадрами
    SLONG TweenStage;        // legacy-поле (не используется в runtime)

    GameKeyFrame *CurrentFrame;   // текущая поза
    GameKeyFrame *NextFrame;      // следующий кадр (к нему интерполируем)
    GameKeyFrame *InterruptFrame; // не используется в runtime
    GameKeyFrame *QueuedFrame;    // вторичная очередь для цепочки анимаций

    GameKeyFrameChunk *TheChunk;  // банк анимаций персонажа

    UBYTE Flags;    // DT_FLAG_* (используется редко)
    UBYTE Drawn;    // GAME_TURN последнего рендера
    UBYTE MeshID;   // вариант меша
    UBYTE PersonID; // вариант текстуры/лица (0-10)
} DrawTween;
```

**TweenStage** — legacy, устарело; инициализируется в 0 и не обновляется.
**AnimTween** — рабочее поле: 0→255+ за время одного перехода между кадрами.

---

## Runtime: управление анимациями

### tween_to_anim() vs set_anim() — ключевое различие

```c
// tween_to_anim() — плавный переход (не прерывает текущую)
void tween_to_anim(Thing *p_person, SLONG anim) {
    // Устанавливает NextFrame → плавный переход ПОСЛЕ текущего кадра
    // AnimTween НЕ сбрасывается → продолжает с текущего места
    p_person->Draw.Tweened->NextFrame = global_anim_array[AnimType][anim];
    p_person->Draw.Tweened->QueuedFrame = 0;
    p_person->Draw.Tweened->CurrentAnim = anim;
}

// set_anim() — немедленное переключение (прерывает текущую)
void set_anim(Thing *p_person, SLONG anim) {
    p_person->Draw.Tweened->AnimTween = 0;          // СБРОС прогресса
    p_person->Draw.Tweened->CurrentFrame = global_anim_array[AnimType][anim];
    p_person->Draw.Tweened->NextFrame = ...->NextFrame;
    p_person->Draw.Tweened->Locked = 0;             // СНЯТЬ блокировку
    p_person->Draw.Tweened->CurrentAnim = anim;
}
```

| Аспект | tween_to_anim() | set_anim() |
|--------|-----------------|------------|
| Переключение | Плавное (ставит в очередь) | **Мгновенное** |
| AnimTween | Не сбрасывается | **Сброс в 0** |
| Locked | Уважает | **Снимает** |
| Использование | Ходьба→бег, стойка→удар | Попадание, смерть, KO |

### Неприрываемые анимации (FLAG_PERSON_LOCK_ANIM_CHANGE)

Когда флаг установлен, `tween_to_anim()` откладывает смену через `locked_anim_change()`.
Анимации, которые блокируют смену: лазание по лестнице, граплинг, некоторые боевые.
`set_anim()` всегда снимает блокировку (`Locked = 0`).

### Цикл обновления анимации (каждый кадр)

```
tween_step = (CurrentFrame->TweenStep << 1) * TICK_RATIO / (1 << TICK_SHIFT)
AnimTween += tween_step

while AnimTween >= 256:
    AnimTween -= 256
    // Пересчёт для следующего кадра:
    AnimTween = AnimTween * NextFrame->TweenStep / CurrentFrame->TweenStep
    CurrentFrame = NextFrame
    if QueuedFrame:               // Цепочка анимаций
        NextFrame = QueuedFrame
        QueuedFrame = 0
        return "anim_chained"
    elif NextFrame->NextFrame:
        NextFrame = NextFrame->NextFrame
    else:
        return "anim_ended"       // Анимация завершена, удерживать последний кадр
```

### TweenStep — скорость интерполяции

- `TweenStep = 0`: моментальный переход (нет tween)
- `TweenStep = 1-10`: быстрый (боевые удары)
- `TweenStep = 20-50`: нормальный (ходьба, бег)
- Чем выше — тем медленнее переход

### Цепочка через QueuedFrame

`QueuedFrame` позволяет связать анимации: когда текущая заканчивается → `QueuedFrame` становится `NextFrame`. Используется для punch combo chains.

---

## .ALL файл — бинарный формат

Полная документация из `io.cpp::load_anim_system()` + `load_insert_game_chunk()`:

```
[4 байта] SLONG save_type              — версия файла (2, 3, 4 или 5)

── Mesh секция (.moj блоки) ──
[если save_type > 2]:
  [4 байта] SLONG count                — количество .moj блоков (вариантов скина)
  [count × embedded .moj блок]
[если save_type == 2]:
  [1 × embedded .moj блок]

── Keyframe chunk секция ──
[4 байта] SLONG chunk_save_type        — версия chunk (должна быть > 1)
[4 байта] SLONG ElementCount          — число body parts (15 для людей)
[2 байта] SWORD MaxPeopleTypes         — число вариантов скина
[2 байта] SWORD MaxAnimFrames          — всего именованных слотов анимаций
[4 байта] SLONG MaxElements            — всего GameKeyFrameElement записей
[2 байта] SWORD MaxKeyFrames           — всего GameKeyFrame записей
[2 байта] SWORD MaxFightCols           — число хитбоксов боя
[MaxPeopleTypes * sizeof(BodyDef)] PeopleTypes[] — skin→body mapping
[2 байта] UWORD check                  — must == 666

[4 байта] ULONG addr1                  — runtime адрес AnimKeyFrames (для pointer fixup)
[MaxKeyFrames * sizeof(GameKeyFrame)] AnimKeyFrames[]
[2 байта] UWORD check                  — must == 666

[4 байта] ULONG addr2                  — runtime адрес Elements
[MaxElements * sizeof(GameKeyFrameElement)] TheElements[]
  — каждый: CMatrix33 (сжатая матрица 3×3) + SWORD OffsetX/Y/Z (6 байт)
[2 байта] UWORD check                  — must == 666

[MaxAnimFrames * sizeof(GameKeyFrame*)] AnimList[]
  — указатели (ULONG в файле), при загрузке пересчитываются через a_off
[2 байта] UWORD check                  — must == 666

[4 байта] ULONG addr3                  — runtime адрес FightCols
[MaxFightCols * sizeof(GameFightCol)] FightCols[]
[2 байта] UWORD check                  — must == 666
```

### Pointer fixup механизм

GameKeyFrame::NextFrame/PrevFrame/FirstElement/Fight/AnimList в файле = оригинальные runtime адреса
на момент сохранения. При загрузке вычисляются offsets и все указатели перебазируются:
```
a_off  = new_AnimKeyFrames_ptr - addr1
ae_off = new_TheElements_ptr   - addr2
af_off = new_FightCols_ptr     - addr3
```
`save_type > 4`: `convert_keyframe_to_pointer()` делает это чище.

### Встроенный .moj блок в .all файле

Идентичен standalone .moj, но читается из уже открытого handle:
```
[4 байта] SLONG save_type
[2 байта] UWORD so  (start prim_object index)
[2 байта] UWORD eo  (end prim_object index = so + count)
[(eo-so) × read_a_prim()] — каждый prim = PrimObject header + PrimPoints + PrimFaces
```

### Использование параметра type

- `type=0` → загрузить все скин-варианты (создания, мировые объекты)
- `type=1` → person.all: Darci всегда, остальные только если не в `DONT_load` bitmask
- `type=2` → thug.all: выборочно по классу персонажа (cop, rasta, MIB, mechanic, tramp)

---

## Загрузка анимаций

```c
// Anim.cpp
load_anim_system(GameKeyFrameChunk *chunk, CBYTE *name)
append_anim_system(chunk, name, start_index, flags)  // добавить дополнительные анимации
```

**global_anim_array[4][450]** — главный индекс:
- `[AnimType]` = 0 (DARCI), 1 (ROPER/hero), 2 (ROPER2), 3 (CIV)
- `[AnimID]` = индекс ANIM_* константы

Файлы-источники:
- `data\darci1.all` → ANIM_TYPE_DARCI
- `data\hero.all` → ANIM_TYPE_ROPER
- `data\bossprtg.all` → ANIM_TYPE_CIV
- Дополнения: `police1.all` (+ индекс 200 для ROPER), `newciv.all` (CIV_M_START для CIV)

---

## Константы анимаций (animate.h)

Именованные константы для каждой анимации:

```c
// animate.h, строки 688–740+
#define ANIM_IDLE               0
#define ANIM_WALK               1
#define ANIM_RUN                2
// ... (52+ ACTION_* + 250+ ANIM_* констант)

// Специфичные для zipwire:
#define ANIM_WIRE_SLIDE_PULLUP  246
#define ANIM_WIRE_SLIDE_HANG    247
#define ANIM_HAND_OVER_HAND     37
#define ANIM_DEATH_SLIDE        68

// Grappling hook (never shipped):
#define ANIM_GRAPPLING_HOOK_PICKUP  140
#define ANIM_GRAPPLING_HOOK_WINDUP  137
#define ANIM_GRAPPLING_HOOK_RELEASE 138
```

---

## Хранение в файлах

Анимации физически хранятся внутри структур PrimObject в `.IMP`/`.MOJ` файлах. При загрузке модели анимации извлекаются и сохраняются в системе AnimBank для конкретного персонажа.

