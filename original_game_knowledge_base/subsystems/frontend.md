# Frontend — Система меню Urban Chaos

**Ключевые файлы:**
- `fallen/Source/frontend.cpp`, `fallen/Headers/frontend.h` — главное меню, все режимы
- `fallen/Source/Attract.cpp`, `fallen/Headers/Attract.h` — главный цикл frontend
- `fallen/Source/startscr.cpp`, `fallen/Headers/startscr.h` — мост frontend→game (STARTSCR_mission)

---

## 1. Frontend → Game Pipeline (полный флоу запуска миссии)

```
game_attract_mode() [Attract.cpp]
  └─► FRONTEND_loop() каждый кадр [frontend.cpp]
        └─► FRONTEND_input() при нажатии ввода
              - FE_MAPSCREEN: ENTER → mode = 100 + mission_selected
              - mode ≥ 100:   ENTER → return FE_START (из FRONTEND_input)
        └─► if (res == FE_START):
              1. STARTSCR_mission = "levels\\" + FRONTEND_MissionFilename(urban.sty, mode-100)
              2. Lookup в whattoload[]: dontload / has_balrog / bane
                 (DONT_load = 0 → всё грузится всегда, невзирая на маску!)
              3. VIOLENCE = TRUE (FALSE для туториалов: FTutor1, Album1, Gangorder1)
              4. FRONTEND_diddle_music() → MUSIC_bodge_code 1-4
              5. return STARTS_START
  └─► case STARTS_START:
        GAME_STATE &= ~GS_ATTRACT_MODE
        GAME_STATE |=  GS_PLAY_GAME
        go_into_game = TRUE
        ATTRACT_loadscreen_init() → показать "e3load.tga"
        stop_all_fx_and_music()

elev.cpp::ELEV_load() [вызывается из game_loop при GS_PLAY_GAME]:
  └─► extern CBYTE STARTSCR_mission[]
  └─► if (*STARTSCR_mission):
        ELEV_fname_level = STARTSCR_mission (копия для Restart)
        ELEV_load_name(STARTSCR_mission)   → загрузка .iam + .ucm + всего
        *STARTSCR_mission = 0              → сброс после загрузки
```

**FRONTEND_MissionFilename(script, i):**
- Читает urban.sty построчно (как ParseMissionData)
- Фильтрует по `district_selected` и `mission_hierarchy & 4` (только available миссии)
- Возвращает i-ю доступную миссию в выбранном районе (поле `fn` из .sty)
- Побочный эффект: устанавливает `mission_launch = mdata->ObjID`

**Переменные метаданных (глобальные, устанавливаются перед запуском):**
```c
CBYTE STARTSCR_mission[_MAX_PATH] // "levels\\police1.ucm" — имя файла
UBYTE this_level_has_the_balrog   // 1 = Baalrog3.ucm (спец-поведение)
UBYTE this_level_has_bane         // 1 = Finale1.ucm только (index 27 в whattoload[])
UBYTE is_semtex                   // 1 = index 20 = skymiss2.ucm (странное имя)
SLONG DONT_load                   // bitmask; жёстко = 0 → грузим всё
BOOL  VIOLENCE                    // вкл/выкл насилие (туториалы = FALSE)
SLONG MUSIC_bodge_code            // 0-4, overrides музыкальную тему уровня
```

**MUSIC_bodge_code значения:**
- 0 = обычная тема уровня
- 1 = levels\\fight* или levels\\FTutor
- 2 = levels\\Assault
- 3 = levels\\testdrive
- 4 = levels\\Finale1

**VIOLENCE = FALSE для индексов в whattoload[]:**
- index 1 = FTutor1.ucm (combat tutorial)
- index 31 = Gangorder1.ucm
- index 32 = Album1.ucm

---

## 2. Attract.cpp — Главный цикл

`game_attract_mode()` = главный frontend loop (название "attract mode" устарело).
Вызывается из game_loop() при `GS_ATTRACT_MODE`.

**Структура файла:**
- `game_attract_mode()` — основная функция-цикл
- `ATTRACT_loadscreen_init()` — показывает e3load.tga loading screen
- `ATTRACT_loadscreen_draw(completion)` — прогресс-бар загрузки (DC: цветные блоки, PC: completion bar)
- `ScoresDraw()` — статистика + mucky/speedrun time таблица (вызывается после уровня)
- `any_button_pressed()` — проверка любой кнопки джойпада

**Мёртвый код:**
- `level_won()` / `level_lost()` — `#if 0` (dead code); заменены GAMEMENU overlay
- Старый attract demo (playback .pkt файлов) — закомментирован целиком
- `BRIEFING_select()` — `#ifdef OBEY_SCRIPT` → OBEY_SCRIPT не определён → не компилируется
- `STARTSCR_notify_gameover()` в Game.cpp — закомментирован → `auto_advance` никогда не ставится
- `TIMEOUT_DEMO` / demo_timeout — E3 demo таймаут, `=0` → dead code

**startscr.cpp (game build):**
- В non-EDITOR сборке = только `CBYTE STARTSCR_mission[_MAX_PATH] = {0};` (одна строка!)
- Весь остальной код — `#ifdef EDITOR` (старый pre-release mission selector, декабрь 1998)
- Комментарий: "most of this file is obsolete // just a few torn scraps remain..."

---

## 3. Режимы Frontend

```c
FE_MAINMENU        = 1
FE_MAPSCREEN       = 2   // выбор района
FE_MISSIONSELECT   = 3   // выбор миссии
FE_MISSIONBRIEFING = 4   // брифинг
FE_LOADSCREEN      = 5
FE_SAVESCREEN      = 6
FE_CONFIG          = 7
FE_CONFIG_VIDEO    = 8
FE_CONFIG_AUDIO    = 9
FE_CONFIG_INPUT_KB = 10  // клавиатура
FE_CONFIG_INPUT_JP = 11  // джойпад
FE_QUIT            = 12
FE_CONFIG_OPTIONS  = 13
FE_SAVE_CONFIRM    = 14
FE_CREDITS         = -5
FE_START           = -3  // внутренний: запуск миссии (не режим, а результат)
FE_BACK            = -4  // возврат в предыдущий стек
```

**Специальный паттерн:** mode ≥ 100 = режим брифинга (`100 + mission_index`).
Не является FE_* константой, просто числовой диапазон.

---

## 4. Структуры меню

```c
struct MenuData {
    UBYTE Type;        // OT_BUTTON, OT_SLIDER, OT_MULTI, OT_LABEL, OT_KEYPRESS, OT_PADPRESS
    UBYTE LabelID;
    CBYTE *Label;
    CBYTE *Choices;    // Для OT_MULTI — строка вариантов
    SLONG Data;
    UWORD X, Y;
};

struct MenuState {
    UBYTE items;
    UBYTE selected;    // текущий пункт
    SWORD scroll;
    MenuStack stack[10];  // стек вложенных меню
    UBYTE stackpos;
    SBYTE mode;
};
```

**Типы элементов меню:**
| Тип | Описание |
|-----|---------|
| `OT_BUTTON` | обычная кнопка |
| `OT_BUTTON_L` | большая кнопка |
| `OT_SLIDER` | ползунок (0–255) |
| `OT_MULTI` | выбор из нескольких опций |
| `OT_KEYPRESS` | переназначение клавиши |
| `OT_PADPRESS` | переназначение кнопки джойпада |
| `OT_LABEL` | заголовок (не интерактивный) |
| `OT_RESET` | сброс настроек |

---

## 5. FRONTEND_input() — обработка ввода в меню

Отдельная функция (frontend.cpp:3573), вызывается из `FRONTEND_display()`.
Возвращает `FE_START` (начало миссии), `FE_EDITOR`, `-1` (выход), или `0` (продолжение).

**Навигация по пунктам:**
- `KB_UP` / `INPUT_MASK_FORWARDS` → `selected--` (с wrap); OT_LABEL и greyed OT_BUTTON пропускаются
- `KB_DOWN` / `INPUT_MASK_BACKWARDS` → `selected++` (с wrap)
- `KB_HOME` → прыжок на первый активный пункт
- `KB_END` → прыжок на последний активный пункт
- В режиме `FE_MAPSCREEN`: UP/DOWN также меняют `mission_selected`; LEFT/RIGHT — `district_selected`

**Активация (ENTER / SPACE / KB_PENTER / any_button):**
- `OT_BUTTON` → `menu_mode_queued = item->Data`; `fade_mode=2` (transition)
- `OT_MULTI` → цикл вариантов (`Data&0xff` increment mod `Data>>8`)
- `OT_SLIDER` → только LEFT/RIGHT (ENTER ничего не делает)
- `OT_KEYPRESS` → `grabbing_key=1`; следующая нажатая клавиша (кроме ESC) записывается в `item->Data`
- `OT_PADPRESS` → `grabbing_pad=1`; следующая кнопка джойпада записывается
- `FE_MAPSCREEN` → `menu_mode_queued = 100 + mission_selected` (переход к брифингу)
- `mode≥100` (брифинг) → возвращает `FE_START` (запуск миссии)

**LEFT/RIGHT:**
- `OT_SLIDER`: `Data ± 2` (двойной шаг, кроме крайних значений)
- `OT_MULTI`: `Data ± 1` (в пределах max)
- `FE_MAPSCREEN`: листание районов влево/вправо

**ESC / INPUT_MASK_CANCEL:**
- Если `fade_mode==2` → отмена transition (откат `fade_mode=1`)
- Если `stackpos>0` → `menu_mode_queued=FE_BACK`; сохраняет настройки через `FRONTEND_storedata()`
- Если `stackpos==0` → `menu_mode_queued=FE_MAINMENU` (или FE_QUIT если WANT_AN_EXIT_MENU_ITEM)
- `fade_mode=6` (fade-out)

**Джойпад (INPUT_TYPE_JOY):**
- Deadzone: `NOISE_TOLERANCE=4096` (ось ±4096 от 32768)
- Диагонали запрещены: lY > MAX → только BACKWARDS (очищает LEFT/RIGHT)
- Edge-triggered: `input==last_input → input=0` (нет автоповтора держания)
- `first_pad`: первое движение (без кнопок) при старте игнорируется (баг PC джойстиков)
- `last_button`: кнопка засчитывается только при первом нажатии (edge, не hold)

---

## 6. Визуальные компоненты

### Фоны меню (TGA текстуры, 4 сезонных варианта)
```
"title leaves1.tga", "title rain1.tga", "title snow1.tga", "title blood1.tga"
"map leaves darci.tga", "map rain darci.tga", ...
"briefing leaves darci.tga", "briefing rain darci.tga", ...
"config leaves.tga", "config rain.tga", ...
```

Тема (leaves/rain/snow/blood) = `menu_theme` = 0-3 по `complete_point`:
- `<8` → 0 (leaves), `<16` → 1 (rain), `<24` → 2 (snow), `≥24` → 3 (blood)

### DirectDraw Surfaces для фонов
```c
LPDIRECTDRAWSURFACE4 screenfull_back  = NULL;  // главное меню
LPDIRECTDRAWSURFACE4 screenfull_map   = NULL;  // выбор районов
LPDIRECTDRAWSURFACE4 screenfull_config= NULL;  // конфиг
LPDIRECTDRAWSURFACE4 screenfull_brief = NULL;  // брифинг
```

### Kibble (частицы/анимация в меню)
- `FRONTEND_kibble_init()` — инициализация sparkle-частиц на экране
- `FRONTEND_kibble_process()` — обновление частиц каждый кадр
- Перезапускается при смене темы menus

### FRONTEND_display() (главный render функция)
Порядок: background → xition → kibble → menu items → title wibble → districts map

---

## 7. Что переносить в новую версию

| Компонент | Подход |
|-----------|--------|
| STARTSCR_mission механизм | Перенести: простая глобальная строка → можно std::string |
| FE_* режимы | Перенести логику, переработать визуально |
| Стек меню (stackpos/stack[10]) | Перенести 1:1 |
| whattoload[] / DONT_load | Упростить: просто грузить всё (DONT_load=0 уже так) |
| VIOLENCE флаг | Перенести (для туториалов) |
| MUSIC_bodge_code | Перенести (задаёт музыкальную тему уровня) |
| DirectDraw Surfaces фонов | Заменить на OpenGL текстуры |
| Kibble частицы | Опционально перенести |
| Seasonal themes (4 варианта) | Перенести (те же TGA ресурсы) |
| Мотоцикл/гарпун в whattoload | Не переносить (эти миссии не портируются) |
