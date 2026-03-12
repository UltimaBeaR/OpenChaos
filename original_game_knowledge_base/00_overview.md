# Обзор архитектуры оригинальной игры

## Общая информация

- **Игра:** Urban Chaos (рабочее название: "Fallen")
- **Студия:** MuckyFoot Productions
- **Год:** 1999
- **Платформы:** PC (DirectX), PSX, Dreamcast
- **Язык:** C (несмотря на расширение .cpp — C-стиль везде)
- **Компилятор оригинала:** Visual Studio 6, DirectX 6/7
- **Зависимости:** DirectX 6 SDK, Bink Video (binkw32.lib), Miles Sound System (mss32.lib), DirectShow (quartz.lib)

---

## Структура исходников

```
original_game/
├── fallen/              — основная игра (Urban Chaos)
│   ├── Source/          — ~150 .cpp файлов, игровая логика
│   ├── Headers/         — заголовки игры
│   ├── DDEngine/        — движок рендеринга (~60 .cpp)
│   ├── DDLibrary/       — низкоуровневая работа с DirectX (~22 .cpp)
│   ├── Editor/          — редактор карт/анимаций (~60 .cpp) [НЕ НУЖЕН]
│   ├── GEdit/           — геометрический редактор [НЕ НУЖЕН]
│   ├── Ledit/           — редактор уровней [НЕ НУЖЕН]
│   ├── SeEdit/          — редактор звука [НЕ НУЖЕН]
│   ├── PSXENG/          — PSX-специфичный движок [НЕ НУЖЕН]
│   ├── psxlib/          — PSX библиотека [НЕ НУЖЕН]
│   ├── Glide Engine/    — 3dfx Glide рендерер (мёртвый код) [НЕ НУЖЕН]
│   ├── Glide Library/   — HAL для Glide [НЕ НУЖЕН]
│   ├── AutoRun/         — лаунчер
│   ├── Loader/          — загрузчик игры
│   └── Debug/           — МИКС: артефакты сборки + ресурсы игры в подпапках
│       ├── Meshes/      — 3D меши
│       ├── Textures/    — текстуры
│       ├── clumps/      — .txc архивы (clump-файлы)
│       ├── data/        — конфиги, изображения, звуковые данные
│       ├── levels/      — файлы уровней
│       ├── text/        — текстовые строки UI
│       ├── bink/        — Bink-видео катсцены (.bik)
│       ├── stars/       — текстуры звёздного неба
│       ├── server/      — (уточнить)
│       └── config.ini   — конфиг игры (из Steam-версии)
├── MFLib1/              — кросс-платформенная графическая библиотека студии (~40 .cpp)
│   ├── Source/C/Common/ — платформонезависимые системы (рисование, математика, тригонометрия, текст)
│   └── Source/C/Windows/ — Windows-специфичные реализации (D3D, Display, File I/O, Keyboard, Mouse, Memory)
├── MFStdLib/            — стандартные утилиты (файлы, память, математика)
├── MuckyBasic/          — собственный скриптовый язык (VM + lexer + parser + compiler)
└── thrust/              — побочный проект, не относится к Urban Chaos (игнорировать)
```

---

## Масштаб кодовой базы

- ~500K+ строк кода
- ~350+ .cpp файлов, ~150+ .h заголовков
- Крупнейшие файлы:
  - `DDLibrary/DIManager.cpp` — ~62K строк (DirectInput)
  - `DDLibrary/GDisplay.cpp` — ~51K строк (управление дисплеем)
  - `DDLibrary/DDManager.cpp` — ~49K строк (Direct3D)
  - `fallen/Source/Game.cpp` — ~53K строк (игровая логика)
  - `fallen/Source/Mission.cpp` — ~43K строк (миссии)
  - `fallen/Source/Special.cpp` — ~41K строк (оружие/предметы)

---

## Entry Point и главный игровой цикл

**Entry point:** `fallen/Source/Main.cpp` → `WinMain()` (PC) / `main()` (PSX)

Последовательность запуска:
1. `WinMain()` создаёт окно, инициализирует хост
2. `SetupHost(H_CREATE_LOG)` — инициализация движка
3. Передаёт управление в `game()` из `Game.cpp`

**Главный игровой цикл** (функция `game()` в `Game.cpp`):

```c
game() {
    game_startup()           // Инициализация дисплея, загрузка UI ресурсов

    while (SHELL_ACTIVE && GAME_STATE) {
        if (GAME_STATE & GS_ATTRACT_MODE)
            game_attract_mode()  // Заставки, меню

        if (GAME_STATE & GS_EDITOR)
            editor_loop()        // Внутриигровой редактор карт

        if (GAME_STATE & GS_PLAY_GAME)
            game_loop()          // Основная симуляция + рендеринг

        if (GAME_STATE & GS_CONFIGURE_NET)
            CNET_configure()     // Настройки сети (только PC)
    }

    game_shutdown()          // Очистка, сохранение состояния
}
```

**Game State флаги:**
```c
#define GS_ATTRACT_MODE    (1<<0)  // Frontend/меню
#define GS_PLAY_GAME       (1<<1)  // Активный геймплей
#define GS_EDITOR          (1<<2)  // Редактор карт
#define GS_CONFIGURE_NET   (1<<3)  // Настройки сети
#define GS_REPLAY          (1<<4)  // Воспроизведение демо
```

---

## Тайминг игрового цикла

### Рендеринг (cap FPS)
```c
// Game.cpp — lock_frame_rate():
// Busy-loop spin-wait: while(GetTickCount() - tick1 < 1000/fps) {}
env_frame_rate = ENV_get_value_number("max_frame_rate", 30, "Render");  // из config.ini
```
- Default: **30 FPS рендеринг** (настраивается через config.ini)
- Spinloop через `GetTickCount()` — нет Sleep, постоянная проверка

### Базовая скорость логики
```c
// Game.h
#define NORMAL_TICK_TOCK  (1000/15)   // 66.67 мс — базовый тик (15 FPS логики)
#define TICK_SHIFT        8
```
- Базовая скорость игровой логики — **15 FPS** (1 тик = 66.67 мс)
- Рендеринг (30 FPS) и логика (15 FPS) **разделены**

### TICK_RATIO — адаптивное масштабирование времени
```c
// Thing.cpp — process_things_tick():
TICK_TOCK  = GetTickCount() - prev_tick;   // фактическое время кадра (мс)
TICK_RATIO = (TICK_TOCK << TICK_SHIFT) / NORMAL_TICK_TOCK;
//         = (реальное_мс << 8) / 66.67
// При норме (66.67мс кадр): TICK_RATIO = 256 (= 1.0)
// При медленном кадре (133мс): TICK_RATIO = 512 (= 2.0)
// При быстром кадре (33мс): TICK_RATIO = 128 (= 0.5)

TICK_RATIO = SmoothTicks(TICK_RATIO);   // 4-кадровая скользящая средняя (антиджиттер)
SATURATE(TICK_RATIO, MIN_TICK_RATIO, MAX_TICK_RATIO);  // зажим 0.5x..2.0x
```

**Особые режимы:**
```c
// Без frame-rate independence (non-FRI режим):
tick_diff = 1000/25;     // принудительно 40мс (25 FPS логика)

// Сетевая игра:
tick_diff = 1000/20;     // принудительно 50мс (20 FPS для синхронизации)

// Slow-motion:
TICK_RATIO = 32;         // ~12.5% скорости (32/256)
```

### Константы физики транспорта
```c
// Vehicle.cpp
#define TICK_LOOP        4                   // под-итераций подвески за кадр
#define TICKS_PER_SECOND (20*TICK_LOOP)      // = 80 — только для калибровки формул!
// НЕ частота кадров. Константы физики откалиброваны под это значение:
#define GRAVITY (-(128*10*256) / (80*80))    // = -5120 юнит/тик²
// Подвеска: count=TICK_LOOP; while(--count) → 3 итерации за кадр
```

### Итоговая таблица тайминга

| Аспект | Значение |
|--------|---------|
| Рендеринг (default) | 30 FPS, spin-loop via GetTickCount() |
| Базовый тик логики | 15 FPS (NORMAL_TICK_TOCK = 66.67ms) |
| TICK_RATIO при норме | 256 (= 1.0), динамически масштабируется |
| Сглаживание | 4-кадровая скользящая средняя (SmoothTicks) |
| Slow-motion | TICK_RATIO = 32 (~12.5% скорости) |
| Сетевой режим | принудительно 20 FPS для синхронизации |
| GAME_TURN | счётчик кадров уровня, +1 каждый кадр |

---

## Ключевые архитектурные паттерны

### 1. Thing — базовая сущность всех объектов

Все игровые объекты (игрок, NPC, машины, снаряды, здания и т.д.) — это `Thing`.
Полиморфизм реализован через union + function pointers, без C++ наследования.

```c
struct Thing {
    UBYTE Class, State, OldState, SubState;
    ULONG Flags;
    THING_INDEX Child, Parent, LinkChild, LinkParent;  // иерархия через linked lists
    GameCoord WorldPos;                                // позиция (32.8 fixed-point!)
    void (*StateFn)(Thing*);                           // функция-обработчик текущего состояния

    union {
        DrawTween *Tweened;    // анимированный персонаж
        DrawMesh  *Mesh;       // статичный меш
    } Draw;

    union {                    // данные специфичные для типа
        VehiclePtr  Vehicle;
        PersonPtr   Person;
        ProjectilePtr Projectile;
        PlayerPtr   Player;
        // ... etc
    } Struct;
};
```

**Лимиты:** MAX_THINGS = 400 основных + 300 вторичных = 700 активных сущностей

**Классы (15 типов):**
- `CLASS_PLAYER` — главный герой (Darci)
- `CLASS_PERSON` — NPC (мирные, враги, союзники)
- `CLASS_VEHICLE` — машины, мотоциклы
- `CLASS_PROJECTILE` — пули, гранаты
- `CLASS_ANIMAL` — собаки, голуби
- `CLASS_BUILDING` — статичные строения
- `CLASS_SPECIAL` — скриптованные объекты
- `CLASS_SWITCH` — двери, рычаги
- `CLASS_TRACK` — waypoints для AI
- `CLASS_PLAT` — движущиеся платформы
- `CLASS_CHOPPER` — вертолёты
- `CLASS_PYRO` — огонь (эффекты)
- `CLASS_BARREL`, `CLASS_BAT`, `CLASS_CAMERA`

### 2. MapWho — пространственный хэш (паттерн от Bullfrog)

2D массив ячеек карты (128×128). К каждой ячейке прикреплён linked list объектов в этой области.
Используется для быстрого spatial lookup: "какие объекты рядом с этой точкой".
`cnet.cpp` — оптимизированная реализация этого.

### 3. Фиксированная точка (Fixed-Point Math)

Координаты мира хранятся в формате **32.8 fixed-point** (позиция сдвинута на 8 бит влево).
- Причина (1999 год): FPU тогда работало медленно; fixed-point детерминирован на всех платформах
- **Важно для физики:** при переписывании решить — оставить fixed-point или float с SSE2

### 4. State Machine (машина состояний)

Каждый Thing имеет:
- `State` — текущий ID состояния
- `StateFn` — указатель на функцию обработки этого состояния
- При смене состояния — назначается новая функция

Пример: Person может быть IDLE, WALKING, FIGHTING, DEAD — каждый вызывает свою функцию.

### 5. Процедурная генерация геометрии

Здания, заборы, пожарные лестницы — не хранятся как готовые меши,
а генерируются процедурно из wall-данных во время загрузки. (`building.cpp`)

### 6. Платформенная компиляция через препроцессор

```
#ifdef PSX        → PlayStation 1
#ifdef TARGET_DC  → Dreamcast
#ifdef VERSION_D3D → PC DirectX (нужен)
```
Для новой версии нужен только `VERSION_D3D`-путь. PSX/DC код выпиливается.

---

## Основные игровые системы

### Person System (Персонажи / NPC)
**Файлы:** `Person.cpp`, `Person.h`

Типы персонажей:
- `PERSON_DARCI` — протагонист
- `PERSON_ROPER` — союзник
- `PERSON_COP`, `PERSON_THUG`, `PERSON_CIV` — NPC
- 15 типов персонажей всего

Behaviour flags (40+):
- Движение: `DRIVING`, `BIKING`, `SLIDING`
- Бой: `GUN_OUT`, `PUNCHING`, `BLOCKING`, `GRAPPLING`
- Состояние: `BURNING`, `HELPLESS`, `KO`
- Физика: `PEEING` (да, физика жидкости есть!)

### Vehicle System (Транспорт)
**Файлы:** `Vehicle.cpp`, `Vehicle.h`

- Автомобили — можно угонять и управлять (игрок и NPC)
- 9 типов: Van, Car, Taxi, Police, Ambulance, Jeep, MeatWagon, Sedan, WildcatVan
- Модель повреждений с визуальной деформацией (6 зон)
- Физика на основе spring-damper подвески (4 пружины)
- `bike.cpp` (`BIKE` флаг) — незавершённая фича мотоцикла, в финал не вошла, **не переносить**

### Player System (Управление игроком)
**Файлы:** `Player.cpp`, `Player.h`

Protagonist Darci:
- Боевые приёмы: удар кулаком, kick, блок, захват (grapple)
- Оружие: пистолеты, гранаты, melee
- Движение: ходьба, бег, прыжок, подъём, слайд
- Взаимодействие с окружением

### Collision & Physics (Физика и коллизии)
**Файлы:** `collide.cpp`, `collide.h`, `maths.cpp`

- **CollisionVect:** до 10 000 динамических барьеров в мире
- **WalkLink:** до 30 000 связей полигонов для определения walkable-поверхностей
- **move_thing()**, **move_thing_quick()** — основные функции движения с тест коллизий
- **Height Maps:** terrain elevation хранится как массив байт на ячейку
- **Stride Walking:** поддержка ступенчатого рельефа (бордюры, платформы)

Ключевые структуры:
```c
struct CollisionVect {
    SLONG X[2], Z[2];   // Начало/конец барьера
    SWORD Y[2];          // Высота
    UBYTE PrimType;      // Уровень этажа
    SWORD Face;          // Связанный полигон
};

struct WalkLink {
    UWORD Next;
    SWORD Face;          // ID walkable-поверхности
};
```

### Map System (Карта мира)
**Файлы:** `Map.cpp`, `Map.h`, `pap.cpp`, `pap.h`

- **Размер сетки:** 128×128 ячеек карты
- **Разрешение:** 256 единиц на ячейку (`ELE_SIZE=256`)
- **Height Field:** terrain elevation на ячейку
- **MapWho:** spatial hash (linked lists) для быстрого поиска сущностей
- **MapElement** (14 байт):
  ```c
  struct MapElement {
      LIGHT_Colour Colour;
      SBYTE        AltNotUsedAnyMore;   // устарело, не используется
      SWORD        Walkable;
      THING_INDEX  MapWho;              // голова linked list объектов в ячейке
  };
  ```

### Rendering System (Рендеринг)
**Файлы:** `DDEngine/`, `DDLibrary/`

Архитектура рендерера:
- `GDisplay.cpp` — DisplayManager, совместимость DirectX 9+
- `poly.cpp` — рендеринг и батчинг треугольников
- `mesh.cpp` — загрузка и рендеринг 3D моделей
- `texture.cpp`, `D3DTexture.cpp` — управление текстурами
- `cam.cpp` — камера от третьего лица с обходом коллизий
- `sky.cpp` — рендеринг фона/неба
- `light.cpp`, `Gamut.cpp` — динамические источники света

**Важная находка:** Динамических теней НЕТ — только статически запечённое освещение.
Тени под персонажами — плоские спрайты (`shadow.cpp`).

Подсистемы рендерера:
- `aa.cpp` — AABBng engine (аппроксимированное освещение)
- `Quaternion.cpp` — кватернионы для поворотов
- `FMatrix.cpp`, `Matrix.cpp` — 4×4 матрицы трансформаций
- `vertexbuffer.cpp` — управление vertex buffers
- `renderstate.cpp` — depth test, alpha blend и т.д.

### Sound System (Звук)
**Файлы:** `Sound.cpp`, `Sound.h`, `music.cpp`

- Использует **Miles Sound System (MSS32)** — Redbook audio driver
- 3D аудио с позиционированием
- Музыка с переходами по состоянию игры
- SFX привязан к Things через `FLAGS_HAS_ATTACHED_SOUND`

### Animation System (Анимация)
**Файлы:** `Anim.cpp`, `animtmap.cpp`, `morph.cpp`

- **Animation Tween:** smooth interpolation между keyframes
- **DrawTween:** хранит интерполированную позу для рендеринга
- Нет скелетной анимации! Вместо костей — swap мешей + tweening
- Несколько animation banks на персонажа
- Smooth blending между состояниями анимации

### Scripting System (Скриптинг)
**Файлы:** `../MuckyBasic/`

Собственный язык **MuckyBasic**:
- Логика миссий (цели, условия)
- Event triggers
- Поведение NPC
- Скомпилированный bytecode исполняется на VM

### UI System
**Файлы:** `interfac.cpp`, `gamemenu.cpp`, `frontend.cpp`, `panel.cpp`

- In-game HUD (здоровье, патроны, цели)
- Frontend меню (старт, save/load, настройки)
- Диалоги NPC
- Экраны брифинга миссий

### Special Effects
- `Effect.cpp` — particle system
- `psystem.cpp` — продвинутые particle effects
- `Fire.cpp` — физика огня (распространяется по материалам геометрии!)
- `cloth.cpp` — симуляция ткани (одежда персонажей)
- `shadow.cpp` — рендеринг теней (спрайт под персонажем)
- `spark.cpp`, `glitter.cpp` — визуальные эффекты
- `pee.h`, `PeeSetup.cpp` — физика жидкости (да, это реально есть)

---

## Зависимости между модулями

```
Main.cpp
  ↓
Game.cpp (главный цикл)
  ├─→ Thing.cpp (управление сущностями)
  │     ├─→ Person.cpp (AI NPC)
  │     ├─→ Player.cpp (протагонист)
  │     ├─→ Vehicle.cpp (машины)
  │     └─→ Pjectile.cpp (снаряды)
  │
  ├─→ Map.cpp (мир)
  │     ├─→ collide.cpp (физика)
  │     ├─→ Nav.cpp / mav.cpp (pathfinding)
  │     └─→ pap.cpp (платформы/пол)
  │
  ├─→ DDEngine/aeng.cpp (рендеринг)
  │     ├─→ poly.cpp (треугольники)
  │     ├─→ mesh.cpp (3D модели)
  │     ├─→ texture.cpp (материалы)
  │     └─→ light.cpp (освещение)
  │
  ├─→ Sound.cpp (аудио)
  │     └─→ music.cpp (музыка)
  │
  ├─→ State.cpp (система поведения)
  ├─→ Command.cpp (ввод/AI команды)
  └─→ interfac.cpp (UI)
```

---

## Неоконченные фичи (найдены в коде, не в финальной игре)

- **Мотоцикл** (`bike.cpp`, флаг `BIKE`) — код есть, в финале **не вошёл**, не переносить
- **Grappling hook** с физикой верёвки — частично реализован
- **Канализация** (sewer system) — отключена
- **Интерьеры зданий** — процедурная генерация есть, но stripped
- **Grappling/захваты** — частичная реализация

---

## Что НЕ нужно в новой версии

- Поддержка PSX (`#ifdef PSX` блоки)
- Поддержка Dreamcast (`#ifdef TARGET_DC` блоки)
- Редакторы: `Editor/`, `GEdit/`, `Ledit/`, `SeEdit/`
- Glide renderer (`Glide Engine/`, `Glide Library/`) — мёртвый код
- Software renderer (`sw.cpp`) — fallback для машин без 3D ускорителя
- Miles Sound System → заменить на кросс-платформенный аналог
- DirectX 6 → заменить на OpenGL/Vulkan
- Экспериментальные недоделанные фичи (см. выше)
- MuckyBasic → заменить на JavaScript/QuickJS или другой современный скриптовый движок

---

## Ресурсы игры

Находятся в подпапках `original_game/fallen/Debug/` (Steam-версия, рабочие).
Артефакты сборки в корне Debug/ (.obj, .exe, .dll, .pdb, .log) — не нужны.

**Подпапки с ресурсами:**
- `Meshes/` — 3D меши
- `Textures/` — текстуры
- `clumps/` — .txc архивы
- `data/` — игровые данные
- `levels/` — файлы уровней
- `talk2/` — диалоги
- `text/` — текстовые строки
- `bink/` — Bink-видео катсцены (.bik)
- `stars/` — текстуры звёздного неба
- `server/` — (уточнить назначение)
- `config.ini` — конфигурационный файл

---

## Главные файлы (быстрый доступ)

| Файл | Назначение |
|------|-----------|
| `fallen/Source/Main.cpp` | Entry point (WinMain) |
| `fallen/Source/Game.cpp` | Главный игровой цикл (~53K строк) |
| `fallen/Source/Mission.cpp` | Система миссий (~43K строк) |
| `fallen/Source/Person.cpp` | AI персонажей |
| `fallen/Source/Player.cpp` | Управление игроком |
| `fallen/Source/Vehicle.cpp` | Транспорт |
| `fallen/Source/collide.cpp` | Физика/коллизии |
| `fallen/Source/Special.cpp` | Оружие/предметы (~41K строк) |
| `fallen/Source/Map.cpp` | Карта мира |
| `fallen/Source/building.cpp` | Процедурная генерация зданий |
| `fallen/DDEngine/aeng.cpp` | Главный рендеринг (~16K строк) |
| `fallen/DDLibrary/DDManager.cpp` | Direct3D (~49K строк) |
| `fallen/DDLibrary/GDisplay.cpp` | Управление дисплеем (~51K строк) |
| `fallen/DDLibrary/DIManager.cpp` | DirectInput (~62K строк) |
| `fallen/Headers/Thing.h` | Структура базового объекта |
| `fallen/Headers/pap.h` | Формат карты (heightmap) |
| `MuckyBasic/` | Скриптовый движок (VM + компилятор) |
