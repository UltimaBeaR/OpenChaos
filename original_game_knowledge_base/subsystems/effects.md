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
- `fallen/Source/ribbon.cpp`, `fallen/Headers/ribbon.h` — ленточные следы (огонь/дым)
- `fallen/Source/bang.cpp` — визуальные взрывы (каскадные сферы)
- `fallen/Source/glitter.cpp` — искры/блеск (linked list частиц с гравитацией)

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
#define FIRE_MAX_FLAMES  256   // пул языков пламени
#define FIRE_MAX_FIRE    8     // максимум очагов одновременно

struct FIRE_Flame {
    SBYTE dx, dz;     // направление движения
    UBYTE die;        // время жизни (32-63 кадра)
    UBYTE counter;    // текущий возраст
    UBYTE height;     // высота пламени (0-255)
    UBYTE next;       // следующий в linked list
    UBYTE points;     // количество точек отображения (2-4)
    UBYTE shit;       // выравнивание
    UBYTE angle[4];   // углы анимации
    UBYTE offset[4];  // вертикальные смещения
};

struct FIRE_Fire {
    UBYTE num;    // активных языков (0=не используется)
    UBYTE next;   // первый язык в списке
    UBYTE size;   // размер очага
    UBYTE shrink; // скорость убывания (за 4 кадра)
    UWORD x, z;   // позиция
    SWORD y;
};
```

**Алгоритм генерации пламени:**
- Смещение: dx/dz в диапазоне -31..+224 (random & 0xff - 0x1f)
- Высота: 255 - манхэттенское расстояние(dx, dz)
- Время жизни: 32 + (random & 0x1f) кадров
- Точки = (height >> 6) + 1, ограничено 2-4

**Обновление (каждый кадр):**
- Нечётные точки: angle += 31, offset -= 17
- Чётные точки: angle -= 33, offset += 21
- Каждые 4 кадра: size -= shrink; добавлять языки пока count < size >> 2

Создание: `FIRE_create(x, y, z, size, life)`.

---

## 3. Искры / электричество (spark.cpp)

```c
#define SPARK_MAX_SPARKS = 32   // пул искр

struct SPARK_Point {
    UBYTE type;   // LIMB(0), CIRCULAR(1), GROUND(2), POINT(3)
    UBYTE flag;   // FAST, SLOW, CLAMP_X/Y/Z, DART_ABOUT, STILL
    UBYTE dist;   // макс. радиус от центра
    SBYTE dx,dy,dz;  // скорость
    union { struct{UWORD x,y,z;} Pos; struct{UWORD person;UWORD limb;} Peep; };
};

struct SPARK_Spark {
    UBYTE used, die;         // активность, оставшиеся кадры
    UBYTE num_points;        // всегда 4
    UBYTE next;              // mapwho linked list
    UBYTE map_z, map_x;
    UBYTE glitter;           // ID эффекта блёсток
    SPARK_Point point[4];    // контрольные точки
};
```

**Типы поведения точек:**
- LIMB: прикреплена к кости персонажа, скорость = 0
- CIRCULAR: поддерживает фиксированное расстояние от point[0]
- GROUND: Y = высота PAP в данной точке
- POINT: фиксированная позиция

**Флаги:**
- FAST: скорость <<= 2; SLOW: скорость >>= 2
- CLAMP_X/Y/Z: обнуляет соответствующую ось скорости
- DART_ABOUT: периодически меняет случайную скорость

Звук: `S_ELEC_START + (spark_id % 5)` — 5 вариантов электрического звука.
Создание из сферы: `SPARK_in_sphere(x,y,z, radius, max_life, max_create)` — до 16 вариантов.

---

## 4. Симуляция ткани (cloth.cpp) — PC only, **ОТКЛЮЧЕНА**

```c
#define CLOTH_MAX_CLOTH  16     // пул объектов
#define CLOTH_MAX_LINKS  256    // пружинных связей

float CLOTH_elasticity = 0.0003f;  // жёсткость пружины
float CLOTH_damping    = 0.95f;    // затухание скорости
float CLOTH_gravity    = -0.15f;   // ускорение вниз
```

Физика (`CLOTH_process()` в `#if 0` — не выполняется): Verlet-интеграция, 3 итерации/кадр, сила ветра (-0.2, 0.0, 0.25) с синус-модуляцией, пружинные связи (смежные + диагональные), блокированные точки как якоря.
Структуры float-based: `CLOTH_Point {float x,y,z,dx,dy,dz}`, grid WxH точек. Рендеринг через нормали. **Не переносить**.

---

## 5. Эффект расширяющегося кольца (Effect.cpp)

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

## 11. Ленточные следы (ribbon.cpp)

**Файлы:** `fallen/Source/ribbon.cpp`, `fallen/Headers/ribbon.h`, рендерер в `fallen/DDEngine/Source/drawxtra.cpp`

**Что это:** Circular-buffer trail renderer для огня, дыма, колёсных следов. Лента = цепочка 3D точек, рендерится как triangle strip с текстурой и альфа-фейдом.

**Структура Ribbon (40+ байт):**
```
Flags(4) | Page(4) | Life(4) | RGB(4) | Size(1) | Head(1) | Tail(1) | Scroll(1) |
FadePoint(1) | SlideSpeed(1) | TextureU(1) | TextureV(1) | Points[32] (GameCoord×32)
```

**Лимиты:** MAX_RIBBONS=64, MAX_RIBBON_SIZE=32 точек на ленту

**Флаги:**
- `RIBBON_FLAG_USED` (1) — слот занят
- `RIBBON_FLAG_FADE` (2) — alpha fade от головы к хвосту (или наоборот с IALPHA)
- `RIBBON_FLAG_SLIDE` (4) — текстурная прокрутка (V += SlideSpeed каждый кадр)
- `RIBBON_FLAG_DOUBLED` (8) — не используется в коде
- `RIBBON_FLAG_CONVECT` (16) — конвекция: все точки дрейфуют вверх +22/кадр (для огня)
- `RIBBON_FLAG_IALPHA` (32) — инвертированный alpha (для дыма: прозрачный→непрозрачный)

**API:**
- `RIBBON_alloc(flags, max_segments, page, life=-1, fade=0, scroll=0, u=1, v=0, rgb=0xFFFFFF)` → ribbon ID (1-based, 0=ошибка); circular scan для свободного слота
- `RIBBON_extend(ribbon, x, y, z)` — добавить точку в голову; circular buffer (Head++, Tail++ при переполнении); пауза (GAMEMENU) = skip
- `RIBBON_free(ribbon)` — освободить; `RIBBON_life(ribbon, life)` — задать оставшееся время жизни
- `RIBBON_length(ribbon)` — кол-во точек (Head-Tail с wraparound)
- `RIBBON_process()` — per-frame: Scroll+=SlideSpeed; CONVECT: Y+=22 для всех точек; Life-- → free при Life==0
- `RIBBON_draw()` → вызывает `RIBBON_draw_ribbon()` в drawxtra.cpp

**Рендеринг (drawxtra.cpp:2536):** triangle strip Tail→Head; U чередуется 0/TextureU; V = index × (1/TextureV) + Scroll×0.015625; alpha fade через FadePoint; POLY_add_triangle

**Кто использует:**
| Вызывающий | Текстура | Флаги | Для чего |
|-----------|---------|-------|----------|
| pyro.cpp (IMMOLATE) | FLAMES3 | CONVECT+FADE+SLIDE | Огонь на горящем персонаже (2+3 ленты) |
| pyro.cpp (FLICKER) | FLAMES3 | CONVECT+FADE+SLIDE | Мерцающее пламя |
| drawxtra.cpp (FLAMER) | FLAMES3 | — | Огонь на бочке |
| drawxtra.cpp (CHOPPER/FIREWORK) | — | — | Взрывы вертолёта/фейерверк |
| bike.cpp (BIKE) | SMOKER | FADE+SLIDE+IALPHA | Дымовой след колёс (НЕ переносить — bike мёртв) |

**Для новой игры:** Перенести 1:1 (circular buffer + triangle strip). Рендеринг заменить на modern API, но логика буфера и конвекции — сохранить.

---

## 11a. Визуальные взрывы (bang.cpp)

**Файл:** `fallen/Source/bang.cpp`

**Что это:** Иерархическая система визуальных взрывов — расширяющиеся сферические частицы с каскадным спавном дочерних. **Чисто визуальная** — урон/shockwave в `create_shockwave()` (combat.md).

**Лимиты:** BANG_Bang[64], BANG_Phwoar[4096] (частицы в linked list)

**BANG_Type — тип взрыва (4 типа в каскаде):**
- Type 0 (BIG): невидимый корень (radius=16), спавнит 6 MIDDLE дочерних по расписанию
- Type 1 (MIDDLE): radius=0→растёт (grow=120), белый→чёрный fade, спавнит NEARLY
- Type 2 (NEARLY): аналогично, меньше
- Type 3 (END): финальные частицы (radius=40), без дочерних

**Цвет:** начальный RGB + per-frame delta (dr,dg,db в 4-bit fixed point) → colour fade

**Физика частиц:**
- Направление: нормализованный вектор длиной 64, размещение на поверхности сферы
- Рост: `radius += grow>>2 + 1`; grow затухает
- Дрейф вверх: `y += counter << 1`
- Каскад: child[6] — массив {counter, type, where} → спавн дочерних при age==counter

**Пространственная оптимизация:** MapWho linked list для culling при рендеринге

**Для новой игры:** Перенести каскадную логику и тайминги; рендеринг заменить на GPU particles.

---

## 12. Взрывы (POW), Пиротехника (PYRO), Мусор (DIRT)

**Вынесено в отдельный файл:** → [effects_pyro_pow_dirt.md](effects_pyro_pow_dirt.md)

Краткая сводка:
- **POW** (pow.cpp): sprite-based визуальные взрывы, 8 типов (sphere/semisphere + cascade spawning), POW_Sprite[256], POW_Pow[32]
- **PYRO** (pyro.cpp): 18 типов пиротехники (Thing+state machine), IMMOLATE=-10HP/frame, GAMEOVER=8 dlights, MAX_PYROS=64
- **DIRT** (dirt.cpp): 16 типов ambient debris (листья/банки/головы/кровь/снег), DIRT_MAX_DIRT=1024, pigeon ОТКЛЮЧЁН

---

## 12. Что переносить в новую версию (обновлено)

| Система | Подход |
|---------|--------|
| Частицы (PSYSTEM 2048) | Переработать на GPU particles, сохранить API |
| Огонь (FIRE 8×256) | Перенести лимиты, переработать рендеринг |
| Ткань (CLOTH 16) | **НЕ переносить** — отключена в оригинале (`#if 0`) |
| DT_EFFECT кольцо | Перенести 1:1 |
| Капли дождя (1024) | Перенести 1:1 |
| Туман/mist (4096 точек, 8 зон) | Перенести UV-анимацию, переработать рендеринг |
| Лужи | Сохранить precalculate, переработать reflective rendering |
| Статические тени | Перенести алгоритм 1:1 |
| **POW взрывы (32×256)** | Перенести логику спавна/каскадов, заменить спрайты на GPU particles |
| **PYRO пиротехника (64)** | Перенести все 18 типов (state machine); рендеринг через новый particle system |
| **DIRT мусор (1024)** | Перенести: листья, банки, гильзы, кровь, головы, снег. Голубей **НЕ** переносить (отключены). Физику bounce/waft перенести 1:1 |
| Искры/электричество (SPARK 32) | Перенести 1:1, адаптировать рендеринг |
| **RIBBON ленты (64×32)** | Перенести circular buffer + конвекцию 1:1, рендеринг на modern API |
| **BANG взрывы (64×4096)** | Перенести каскадную логику, рендеринг на GPU particles |
