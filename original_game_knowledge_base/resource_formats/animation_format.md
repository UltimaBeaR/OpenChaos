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

### DrawTween — интерполированное состояние

```c
// Хранит текущую интерполированную позу для рендеринга
struct DrawTween {
    // интерполированные позиции сегментов меша
    // угол поворота персонажа
    // текущий keyframe + следующий + прогресс interpolation
};
```

---

## Загрузка анимаций

```c
// Anim.cpp
load_anim_system(GameKeyFrameChunk *chunk, CBYTE *name)
```

- Анимации организованы в банки (`AnimBank`) — набор keyframe'ов для одного персонажа/объекта
- Каждый персонаж может иметь несколько animation banks (разные действия)
- При смене action — выбирается нужный банк, запускается interpolation

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

---

## Что переносить

| Компонент | Перенос | Примечание |
|-----------|---------|------------|
| GameKeyFrame / GameKeyFrameElement | 1:1 | Данные уже в файлах, только парсер |
| DrawTween система | 1:1 | Критично для рендеринга |
| AnimBank / load_anim_system | 1:1 | Критично |
| Vertex morphing (не скелет) | 1:1 | Это принципиальное архитектурное решение |
| TweenStep interpolation | 1:1 | Влияет на feel анимаций |
