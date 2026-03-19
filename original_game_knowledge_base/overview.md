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

Указаны только значимые папки. Папки помеченные `— IGNORED` полностью пропускаются при анализе.

```
original_game/
├── fallen/              — основная игра
│   ├── Source/          — ~150 .cpp, игровая логика ← основной анализ здесь
│   ├── Headers/         — заголовки
│   ├── DDEngine/        — движок рендеринга (~60 .cpp)
│   ├── DDLibrary/       — низкоуровневый DirectX (~22 .cpp)
│   ├── Editor/          — редактор уровней (в основном IGNORED). ⚠️ Содержит рантайм-код: Anim.h (анимационные структуры, включается через Structs.h), Prim.h (прим-объекты), prim_draw.h (rotate_obj для interact.cpp), map.h (edit_map для building/collide), outline.cpp (build_roof_grid — рантайм), ed.h (ED_Light для загрузки .lgt), Thing.h (MapThingPSX для io.cpp save_type==18)
│   ├── GEdit/           — IGNORED: инструмент редактора уровней (GEdit.h включается 43+ файлами без guard, но все вызовы — в #ifdef EDITOR блоках)
│   ├── Ledit/           — IGNORED: инструмент редактора уровней
│   ├── SeEdit/          — IGNORED: инструмент редактора звука
│   ├── PSXENG/ psxlib/  — IGNORED: PSX-специфичный рендеринг (psxlib/GHost.cpp — исключение: PSX controls задокументированы)
│   ├── Glide Engine/    — IGNORED: 3dfx Glide рендерер, VERSION_GLIDE не определён, мёртвый код
│   └── Debug/           — ресурсы игры (Steam) + артефакты сборки
├── MFLib1/              — IGNORED: внутренняя графическая библиотека студии, не игровой код
├── MFStdLib/            — базовые утилиты: типы (SLONG, UBYTE, UWORD — MFTypes.h), память (StdMem.cpp), математика (inline.h: MUL64/DIV64), Always.h. Активно используется всей кодовой базой.
├── MuckyBasic/          — IGNORED: скриптовый VM + компилятор, не интегрирован с игрой (нет механизма вызова из игрового кода)
└── thrust/              — IGNORED: посторонний проект, не относится к игре
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
| Камера | fc.cpp (активная); cam.cpp — МЁРТВЫЙ КОД (`#ifdef DOG_POO`) | [camera.md](subsystems/camera.md) |
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

---

## Карта связей (что читать вместе)

```
collide.cpp      → physics.md + navigation.md + characters.md
Person.cpp       → ai.md + ai_structures.md + ai_behaviors.md + combat.md + controls.md + player_states.md
pcom.cpp         → ai.md + ai_behaviors.md
eway.cpp         → missions.md + game_objects.md
Mission.cpp      → missions.md + weapons_items.md
interfac.cpp     → controls.md + player_states.md + camera.md
Vehicle.cpp      → vehicles.md + physics.md
Special.cpp      → weapons_items.md + combat.md
Building.cpp     → buildings_interiors.md + world_map.md + navigation.md
interact.cpp     → interaction_system.md + physics.md + controls.md + characters.md
plat.cpp         → game_objects.md + missions.md (waypoints)
chopper.cpp      → game_objects.md + ai.md
```

---

## Аннотированные исходники (// claude-ai: комментарии)

hm.cpp, Furn.cpp, cutscene.cpp, ob.cpp, elev.cpp, Anim.cpp, mesh.cpp, id.cpp, facet.cpp, supermap.cpp, io.cpp, Prim.cpp, Game.cpp, figure.cpp, walkable.cpp, stair.cpp, gamemenu.cpp, wmove.cpp, Map.cpp, night.cpp, overlay.cpp, guns.cpp, grenade.cpp, Nav.cpp, Wallhug.cpp, barrel.cpp, pow.cpp, pyro.cpp, dirt.cpp, ribbon.cpp, bang.cpp, interact.cpp, plat.cpp, chopper.cpp, Pjectile.cpp, startscr.cpp

pcom.cpp (~178), Special.cpp (~477), bat.cpp (~128, Bane AI), canid.cpp (~101, собаки инертны), Controls.cpp (~215), collide.cpp (~77), Person.cpp (~8 блоков), eway.cpp (~60+ блоков), elev.cpp (~30+), interfac.cpp (~50+ блоков), ware.cpp, inside2.cpp, Attract.cpp, frontend.cpp, fire.cpp (~99), psystem.cpp (~158), Vehicle.cpp (~309), Building.cpp (~141), mav.cpp (~152), sound.cpp (~103), door.cpp (~33), psxlib/GHost.cpp
