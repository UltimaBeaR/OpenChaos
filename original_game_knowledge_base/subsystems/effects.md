# Визуальные эффекты — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/psystem.cpp`, `fallen/Headers/PSystem.h` — система частиц
- `fallen/Source/fire.cpp`, `fallen/Headers/fire.h` — огонь
- `fallen/Source/cloth.cpp`, `fallen/Headers/cloth.h` — симуляция ткани
- `fallen/Source/Effect.cpp` — расширяющееся кольцо эффект
- `fallen/Source/drip.cpp`, `fallen/Headers/drip.h` — капли дождя
- `fallen/Source/mist.cpp`, `fallen/Headers/mist.h` — туман/дымка
- `fallen/Source/fog.cpp`, `fallen/Headers/fog.h` — объёмный туман
- `fallen/Source/water.cpp`, `fallen/Headers/water.h` — динамическая вода
- `fallen/Source/puddle.cpp`, `fallen/Headers/puddle.h` — лужи (PC-only)
- `fallen/Source/shadow.cpp` — запечённые тени (загрузка уровня)

---

## 1. Система частиц (psystem.cpp)

```c
// PSystem.h
#ifdef PSX
#define PSYSTEM_MAX_PARTICLES 64
#else
#define PSYSTEM_MAX_PARTICLES 2048   // PC
#endif
```

**API:**
```c
void PARTICLE_Add(
    SLONG x, SLONG y, SLONG z,      // позиция (в блоках <<8)
    SLONG dx, SLONG dy, SLONG dz,   // скорость
    SLONG page,                      // текстурная страница (POLY_PAGE_*)
    SLONG size,
    SLONG colour,                    // ARGB
    SLONG flags,                     // PFLAG_SPRITEANI и др.
    SLONG anim_start, SLONG anim_end,
    SLONG anim_speed,
    SLONG gravity, SLONG drag
);
```

**Timing:** использует `GetTickCount()` для delta-time (независимо от frame rate).
Первый кадр обрабатывается за `NORMAL_TICK_TOCK`.

**Флаги частиц:**
- `PFLAG_SPRITEANI` — анимированный спрайт (цикл по страницам anim_start..anim_end)

---

## 2. Огонь (fire.cpp)

```c
#define FIRE_MAX_FLAMES  256   // максимум отдельных языков пламени
#define FIRE_MAX_FIRE    8     // максимум очагов огня одновременно
```

**Особенности:**
- Огонь распространяется по материалам геометрии (`building.cpp`)
- Каждый очаг — набор спрайтов с аддитивной альфой (`POLY_PAGE_FLAMES`, `POLY_PAGE_FLAG_ADD_ALPHA`)
- Дым: полупрозрачные спрайты (`POLY_PAGE_SMOKE`)
- `PYRO_FIREBOMB` — взрыв с огнём (для транспорта при уничтожении)

---

## 3. Симуляция ткани (cloth.cpp) — PC only

```c
#define CLOTH_MAX_CLOTH  16     // максимум объектов ткани
#define CLOTH_MAX_LINKS  256    // ограничений (связей между точками)
```

**Метод:** constraint-based simulation (не PSX — там нет).
- Использует `float` (не fixed-point) — только на PC
- Точки ткани соединены жёсткими ограничениями длины
- Применяется для флагов, баннеров, одежды

---

## 4. Эффект расширяющегося кольца (Effect.cpp)

Простой эффект — тип рисования `DT_EFFECT`:
```c
// Effect.cpp
SPEED  = OnFace   // стартует 128
RADIUS = Index    // = -(SPEED*50)/64 изначально

// Каждый кадр:
SPEED  = (SPEED * 50) / 64   // затухание ~78% от предыдущего
RADIUS += SPEED               // кольцо расширяется

// Когда SPEED == 0 → объект удаляется
```

---

## 5. Капли дождя (drip.cpp)

```c
#define DRIP_MAX_DRIPS   1024

struct DRIP_Drip {
    UWORD x; SWORD y; UWORD z;
    UBYTE size;
    UBYTE fade;    // 0 = нет капли; DRIP_SFADE = 255 при создании
    UBYTE flags;   // DRIP_FLAG_PUDDLES_ONLY
};

#define DRIP_SFADE  255    // начальная прозрачность
#define DRIP_SSIZE  (rand() & 0x7)   // начальный размер
#define DRIP_DFADE  16     // уменьшение fade за кадр
#define DRIP_DSIZE  4      // увеличение размера за кадр
```

**`DRIP_create_if_in_puddle()`** (PC only, `#ifndef TARGET_DC`):
- Проверяет `PAP_FLAG_REFLECTIVE` и `MAV_SPARE_FLAG_WATER`
- Если в луже — создаёт каплю + вызывает `PARTICLE_Add()` для анимации всплеска (`POLY_PAGE_SPLASH`)

---

## 6. Дымка/туман (mist.cpp)

```c
#define MIST_MAX_POINTS  4096   // точки UV-сетки тумана (PC и PSX одинаково)
#define MIST_MAX_MIST    8      // зон тумана

struct MIST_Mist {
    UBYTE type;      // 0-3, определяет тайл текстуры (2×2 атлас)
    UBYTE detail;    // количество квадратов сетки (3-255)
    UWORD p_index;   // индекс в MIST_point[]
    SLONG height;    // высота над землёй
    SLONG x1, z1, x2, z2;   // границы зоны
};
```

**Анимация:**
- `MIST_gust(x1,z1, x2,z2)` — порыв ветра перемещает UV точки сетки
- `MIST_process()` — каждый кадр: `du/dv` интегрируются в `u/v`, затухание 25% + "spring" возврат 0.5%
- Внутренние точки сетки "вибрируют" — `MIST_off_u/v_odd/even` (синус/косинус от `MIST_get_turn`)
- Крайние точки сетки не вибрируют (граница статична)

**PC-only код** под `#ifndef PSX`:
- `MIST_get_detail()`, `MIST_get_point()`, `MIST_get_texture()` — доступ к сетке для рендеринга

---

## 7. Объёмный туман (fog.cpp)

```c
// fog.h
FOG_TYPE_TRANS1..4   // прозрачность: 1=максимально прозрачный, 4=самый густой
```

**API:**
```c
void FOG_gust(SLONG x1, SLONG z1, SLONG x2, SLONG z2);  // реакция на ветер
```

Туман реагирует на движение персонажей/машин через `FOG_gust()` — аналогично `MIST_gust()`.

---

## 8. Динамическая вода (water.cpp)

**Хранение:** linked list `water faces` на lo-res mapsquare.

**API:**
```c
void WATER_gush(SLONG x1, SLONG z1, SLONG x2, SLONG z2);  // возмущение поверхности
```

- Машины и персонажи создают возмущения через `WATER_gush()`
- Анимация поверхности — волновая деформация UV

---

## 9. Лужи (puddle.cpp) — PC only (`#ifndef TARGET_DC`)

**Принцип:** предвычисляются при загрузке уровня.

```c
void PUDDLE_precalculate();   // анализ геометрии → нахождение ям/бордюров
BOOL PUDDLE_in(SLONG x, SLONG z);  // попадает ли точка в лужу
void PUDDLE_splash(SLONG x, SLONG y, SLONG z);  // интерактивный всплеск
```

Лужи формируются вдоль:
- Нижних граней бордюров (curbs)
- Краёв зданий, где накапливается вода
- Отражение: если `PAP_FLAG_REFLECTIVE` — квадрат отражает (зеркальный эффект)

---

## 10. Статические тени (shadow.cpp)

Тени **запекаются при загрузке уровня**, не в рантайме (см. также rendering.md раздел 6).

```c
// shadow.cpp
#define SHADOW_DIR_X  (+147)   // направление солнца
#define SHADOW_DIR_Y  (-148)
#define SHADOW_DIR_Z  (-147)

SLONG shadow_dist = 32;  // дальность проверки луча
```

**Алгоритм `SHADOW_do()`:**
1. Для каждого hi-res квадрата — проверить 3 соседних квадрата (mx-1, mz+0), (mx, mz+1), (mx-1, mz+1)
2. `SHADOW_in(x,y,z)` — кастует луч в направлении солнца, проверяет `there_is_a_los()`
3. Если луч блокирован → квадрат в тени
4. Тип тени кодируется в 3 бита (`shadow[8]` lookup) → записывается в `PAP_FLAG_SHADOW_1/2/3`
5. Обрабатываются: пол (floor), крыши (roof tiles), roof faces, walkable faces

---

## 11. Что переносить в новую версию

| Система | Подход |
|---------|--------|
| Частицы (PSYSTEM_MAX_PARTICLES=2048) | Переработать на GPU particles, сохранить API |
| Огонь (8 очагов, 256 языков) | Перенести лимиты, переработать рендеринг |
| Ткань (CLOTH_MAX_CLOTH=16) | Перенести constraint solver, адаптировать float-физику |
| DT_EFFECT кольцо | Перенести 1:1 |
| Капли дождя (1024) | Перенести 1:1 |
| Туман/mist (4096 точек, 8 зон) | Перенести UV-анимацию, переработать рендеринг |
| Лужи | Сохранить precalculate подход, переработать reflective rendering |
| Статические тени | Перенести алгоритм 1:1, результат хранить в том же формате |
| Динамические тени персонажей | Sprite-тени как в оригинале — или улучшить до shadow maps |
