# Обзор архитектуры — Urban Chaos

- **Игра:** Urban Chaos (рабочее название: "Fallen"), MuckyFoot Productions, 1999
- **Платформы:** PC (DirectX), PSX, Dreamcast
- **Язык:** C (несмотря на .cpp — C-стиль везде, без классов и наследования)
- **Компилятор оригинала:** Visual Studio 6, DirectX 6/7
- **Зависимости:** DirectX 6 SDK, Bink Video (binkw32.lib), Miles Sound System (mss32.lib)
- **Кодовая база:** ~500K+ строк, ~350+ .cpp, ~150+ .h

⚠️ Кодовая база — **пре-релизный snapshot**, не финал. Подробнее: [cut_features.md](cut_features.md)

---

## Структура исходников

```
original_game/
├── fallen/              — основная игра
│   ├── Source/          — ~150 .cpp, игровая логика
│   ├── Headers/         — заголовки
│   ├── DDEngine/        — движок рендеринга (~60 .cpp)
│   ├── DDLibrary/       — низкоуровневый DirectX (~22 .cpp)
│   ├── Editor/          — редактор карт [НЕ НУЖЕН]
│   ├── GEdit/           — геометрический редактор [НЕ НУЖЕН]
│   ├── Ledit/           — редактор уровней [НЕ НУЖЕН]
│   ├── SeEdit/          — редактор звука [НЕ НУЖЕН]
│   ├── PSXENG/ psxlib/  — PSX [НЕ НУЖЕН]
│   ├── Glide Engine/    — 3dfx Glide (мёртвый код) [НЕ НУЖЕН]
│   └── Debug/           — ресурсы игры (Steam) + артефакты сборки
├── MFLib1/              — кросс-платформенная графическая библиотека студии
├── MFStdLib/            — утилиты (файлы, память, математика)
├── MuckyBasic/          — скриптовый язык (VM + компилятор)
└── thrust/              — посторонний проект (игнорировать)
```

Крупнейшие файлы: `DIManager.cpp` 62K, `GDisplay.cpp` 51K, `Game.cpp` 53K, `Mission.cpp` 43K, `Special.cpp` 41K.

---

## Entry Point и главный цикл

`fallen/Source/Main.cpp` → `WinMain()` → `SetupHost()` → `game()` (Game.cpp)

```c
game() {
    game_startup()
    while (SHELL_ACTIVE && GAME_STATE) {
        if (GS_ATTRACT_MODE) game_attract_mode()   // меню
        if (GS_PLAY_GAME)    game_loop()            // симуляция + рендеринг
        if (GS_EDITOR)       editor_loop()          // НЕ НУЖЕН
        if (GS_CONFIGURE_NET) CNET_configure()      // НЕ НУЖЕН
    }
    game_shutdown()
}
```

**Тайминг:** 30 FPS рендеринг, 15 FPS логика, динамический TICK_RATIO.
Подробнее → [subsystems/physics.md](subsystems/physics.md)

---

## Ключевые архитектурные паттерны

### Thing — базовая сущность

Все объекты (игрок, NPC, машины, снаряды, здания) — это `Thing`.
Полиморфизм через `union` + function pointers (не C++ наследование).

```c
struct Thing {
    UBYTE Class, State, SubState;
    void (*StateFn)(Thing*);          // обработчик текущего состояния
    GameCoord WorldPos;               // 32.8 fixed-point координаты
    union { DrawTween*; DrawMesh*; } Draw;
    union { Vehicle*; Person*; Player*; Projectile*; ... } Struct;
};
```

MAX_THINGS = 700 (400 основных + 300 вторичных).
15 классов: CLASS_PLAYER, CLASS_PERSON, CLASS_VEHICLE, CLASS_PROJECTILE, CLASS_ANIMAL, CLASS_BUILDING, CLASS_SPECIAL, CLASS_SWITCH, CLASS_TRACK, CLASS_PLAT, CLASS_CHOPPER, CLASS_PYRO, CLASS_BARREL, CLASS_BAT, CLASS_CAMERA.

### MapWho — пространственный хэш

128×128 массив ячеек. К каждой ячейке — linked list объектов в этой области.
Паттерн от Bullfrog. `cnet.cpp` — реализация.

### Fixed-Point Math

Координаты: **32.8 fixed-point** (позиция × 256).
Вся физика и координаты используют 32.8 fixed-point (integer-only).

### State Machine

Каждый Thing: `State` + `StateFn` (функция текущего состояния).
При смене состояния — назначается новая функция.

### Процедурная генерация геометрии зданий

Фасады зданий, заборы, пожарные лестницы — генерируются из wall-данных при загрузке (`building.cpp`). Переносится 1:1.

---

## Системы и их файлы

| Система | Файлы | Документация |
|---------|-------|-------------|
| Игровые объекты (Thing) | Thing.h, Thing.cpp | [game_objects.md](subsystems/game_objects.md) |
| Персонажи / AI | Person.cpp, ai-*.cpp | [ai.md](subsystems/ai.md), [characters.md](subsystems/characters.md) |
| Управление игроком | Player.cpp, interfac.cpp | [controls.md](subsystems/controls.md) |
| Транспорт | Vehicle.cpp | [vehicles.md](subsystems/vehicles.md) |
| Физика / коллизии | collide.cpp, maths.cpp | [physics.md](subsystems/physics.md) |
| Карта мира | Map.cpp, pap.cpp | [world_map.md](subsystems/world_map.md) |
| Рендеринг | DDEngine/, DDLibrary/ | [rendering.md](subsystems/rendering.md) |
| Анимации | Anim.cpp, morph.cpp | [characters.md](subsystems/characters.md) |
| Звук / Музыка | Sound.cpp, music.cpp | [audio.md](subsystems/audio.md) |
| Скриптинг | MuckyBasic/ | Вне скоупа, см. [analysis_scope.md](analysis_scope.md) |
| Система миссий | Mission.cpp | [missions.md](subsystems/missions.md) |
| Оружие / Предметы | Special.cpp | [weapons_items.md](subsystems/weapons_items.md) |
| UI / HUD / Frontend | interfac.cpp, frontend.cpp | [ui.md](subsystems/ui.md) |
| Визуальные эффекты | Effect.cpp, Fire.cpp, cloth.cpp | [effects.md](subsystems/effects.md) |
| Камера | cam.cpp, fc.cpp | [camera.md](subsystems/camera.md) |
| Прогресс / Сохранения | frontend.cpp, Game.cpp | [player_progress.md](subsystems/player_progress.md) |
| Математика / Утилиты | maths.cpp, Matrix.cpp | [math_utils.md](subsystems/math_utils.md) |
| Форматы ресурсов | io.cpp, Level.cpp | [resource_formats/](resource_formats/README.md) |

---

## Зависимости модулей

```
Main.cpp → game() [Game.cpp]
  ├─→ Thing.cpp → Person.cpp, Player.cpp, Vehicle.cpp, Projectile.cpp
  ├─→ Map.cpp   → collide.cpp, Nav.cpp/mav.cpp, pap.cpp
  ├─→ aeng.cpp  → poly.cpp, mesh.cpp, texture.cpp, light.cpp
  ├─→ Sound.cpp → music.cpp
  └─→ interfac.cpp (UI + ввод)
```

---

## Главные файлы (быстрый доступ)

| Файл | Назначение |
|------|-----------|
| `fallen/Source/Main.cpp` | Entry point |
| `fallen/Source/Game.cpp` | Главный цикл (~53K строк) |
| `fallen/Source/Mission.cpp` | Миссии (~43K строк) |
| `fallen/Source/Person.cpp` | AI персонажей |
| `fallen/Source/Special.cpp` | Оружие/предметы (~41K строк) |
| `fallen/Source/collide.cpp` | Физика |
| `fallen/DDEngine/aeng.cpp` | Рендеринг (~16K строк) |
| `fallen/DDLibrary/DDManager.cpp` | Direct3D (~49K строк) |
| `fallen/DDLibrary/DIManager.cpp` | DirectInput (~62K строк) |
| `fallen/Headers/Thing.h` | Базовый объект |
| `fallen/Headers/pap.h` | Формат карты |
| `MuckyBasic/` | Скриптовый движок |
