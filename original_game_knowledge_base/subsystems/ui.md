# UI и фронтенд — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/overlay.cpp` — OVERLAY_handle(): главный per-frame HUD dispatcher
- `fallen/DDEngine/Source/panel.cpp` — PANEL_last(): отрисовка всех элементов HUD
- `fallen/Source/Frontend.cpp`, `fallen/Headers/Frontend.h` — главное меню, экраны
- `fallen/Source/GameMenu.cpp`, `fallen/Headers/GameMenu.h` — меню паузы, победа/поражение
- `fallen/Source/MenuFont.cpp`, `fallen/Headers/MenuFont.h` — шрифт меню
- `fallen/Source/Font2D.cpp`, `fallen/Headers/Font2D.h` — игровой шрифт (HUD)

---

## 1. HUD (Heads-Up Display)

### Per-frame pipeline (overlay.cpp → panel.cpp)

`OVERLAY_handle()` вызывается каждый кадр из game loop (после draw_screen, до screen_flip):
1. Сброс D3D viewport на полный экран (для letterbox mode)
2. `PANEL_start()` — начало HUD-кадра
3. `PANEL_draw_buffered()` — таймеры обратного отсчёта (MM:SS, до 8 одновременно)
4. `OVERLAY_draw_gun_sights()` — прицелы на целях
5. `OVERLAY_draw_enemy_health()` — HP полоска над целью
6. `PANEL_last()` — **ОСНОВНОЙ HUD** (см. ниже)
7. `PANEL_inventory()` — список оружия при переключении
8. Debug info (cheat==2): FPS + мировые координаты
9. `PANEL_finish()` — конец HUD-кадра

**Условия:** если `EWAY_stop_player_moving()` — прицелы и HP не рисуются; если `GS_LEVEL_WON` — `PANEL_last()` не вызывается.

### PANEL_last() — основной HUD (panel.cpp)

Базовая позиция: `m_iPanelXPos`=0 (PC) / 32 (DC), `m_iPanelYPos`=480 (PC) / 448 (DC).

| Элемент | Позиция (относительно base) | Детали |
|---------|---------------------------|--------|
| **Здоровье** | center (+74, -90), R=66px | Круговой индикатор, 8 сегментов, arc -43°→-227°. Darci: hp*(1/200), Roper: hp*(1/400), Vehicle: hp*(1/300) |
| **Стамина** | (+107, -36) | 5 цветных квадратов 10×10, spacing +3x/-3y. Stamina/25 → red→orange→yellow→green→bright green |
| **Оружие** | (+170, -63) | Спрайт текущего оружия |
| **Патроны** | (+215, -50) | Число справа от оружия |
| **Радар/маяки** | center (+74, -90), R=50px | Стрелки/точки миссионных маяков + красные/серые точки врагов |
| **Crime rate** | (+170, -116) | Пульсирующий красный % |
| **Таймер** | (+171, -118) | MM:SS, мигает красным при <30s |
| **Граната** | (+160, -73) | Обратный отсчёт при выдернутом кольце |
| **Текст/диалоги** | зависит от позиции панели | Речь NPC с портретами |

### Gun Sights (overlay.cpp → panel.cpp)

`track_gun_sight()` регистрирует до MAX_TRACK=4 целей ЗА КАДР (сбрасывается в конце).
`OVERLAY_draw_gun_sights()` рисует прицелы через `PANEL_draw_gun_sight()`:

| Класс цели | Позиция | Scale |
|------------|---------|-------|
| CLASS_PERSON | Голова (bone 11 через calc_sub_objects_position) | 256 |
| CLASS_SPECIAL | WorldPos + 30Y | 128 |
| CLASS_BAT (Balrog) | WorldPos + 30Y | 450 |
| CLASS_BAT (другие) | WorldPos + 30Y | 128 |
| CLASS_BARREL | WorldPos + 80Y | 200 |
| CLASS_VEHICLE | WorldPos + 80Y | 450 |

Прицел: 4 линии внешних (R=164) + 4 внутренних (R=20+accuracy/4). Цвет: зелёный (accuracy=255) → красный (accuracy=0).
При гранате в руках: `show_grenade_path()` (если не в warehouse).

### Enemy Health (overlay.cpp → panel.cpp)

`OVERLAY_draw_enemy_health()` — показывает HP полоску в 3D мире НАД целью:
- Условие: FIGHT mode ИЛИ SUB_STATE_AIM_GUN, и есть Target
- MIB: `hp*100/700`, BAT_BALROG: `(100*hp)>>8` (radius 300), Person: `hp*100/health[PersonType]` (radius 60)
- Рендеринг: `PANEL_draw_local_health()` — PC: 54×4px бар, DC: 54×8px

### Инвентарь (panel.cpp)

`PANEL_inventory()` — вертикальный список оружия, появляется при переключении (PopupFade > 0):
- Позиция X: base+170, Y: ±35px между элементами (вверх если панель внизу, вниз если вверху)
- Текущее оружие подсвечено; остальные приглушены
- Не рисуется при вождении / катсцене

### Другие элементы HUD
```c
void PANEL_draw_number(float x, float y, UBYTE digit); // 7-сегментный дисплей (время)
void PANEL_draw_completion_bar(SLONG completion);       // прогресс-бар (0-256)
void PANEL_draw_search(SLONG timer);                    // экран обыска (PSX; PC закомментирован)
void PANEL_flash_sign(SLONG which, SLONG flip);         // дорожные знаки в транспорте
```

### МЁРТВЫЙ КОД overlay.cpp
- `track_enemy()` — `#ifdef OLD_POO`: старые портреты врагов (заменено PANEL_draw_local_health)
- `help_system()` — `#ifdef UNUSED`: proximity help для машин/предметов
- `show_help_text()` — тело закомментировано
- DAMAGE_TEXT система — `#ifdef DAMAGE_TEXT` (НЕ определён): плавающие числа урона
- Beacons (minimap) — закомментированы
- ScoresDraw в overlay — перенесён в GAMEMENU
- CRIME_RATE display — закомментирован
- init_punch_kick() — PSX NTSC only

---

## 2. Система меню (Frontend)

→ Детальная документация: [frontend.md](frontend.md)

Краткое: `game_attract_mode()` [Attract.cpp] → `FRONTEND_loop()` каждый кадр → выбор миссии → `STARTSCR_mission` set → `STARTS_START` → `GS_PLAY_GAME` → `elev.cpp` loads level.

---

## 3. Меню паузы (GameMenu)

### Типы меню в игре
```c
#define GAMEMENU_MENU_TYPE_NONE  0  // нет меню
#define GAMEMENU_MENU_TYPE_PAUSE 1  // пауза
#define GAMEMENU_MENU_TYPE_WON   2  // победа
#define GAMEMENU_MENU_TYPE_LOST  3  // поражение
#define GAMEMENU_MENU_TYPE_SURE  4  // подтверждение
```

### Содержимое меню
```c
// Пауза:       "Game Paused" → Resume / Restart / Abandon
// Победа:      "Level Complete"
// Поражение:   "Level Lost" → Restart / Abandon
// Подтверждение: "Are you sure?" → Okay / Cancel
```

### Анимация появления меню
```c
GAMEMENU_background   // затемнение фона (0–640 ms)
GAMEMENU_fadein_x     // позиция фейда (8-bit fixed point)
GAMEMENU_slowdown     // замедление времени (8.8 fixed point, 0 → 0x10000)
```

### Управление меню
- **PC:** ESC — открыть/закрыть, Up/Down — навигация, Enter/Space — выбор
- **DC:** Start — открыть/закрыть, X+Y одновременно — скрыть

---

## 4. Шрифты

### MenuFont (пропорциональный, для меню)
```c
struct CharData {
    float x, y;         // координаты символа в текстуре [0-1]
    float ox, oy;       // конец символа
    UBYTE width, height;
    UBYTE xofs, yofs;   // смещения
};

void MENUFONT_Load(CBYTE *fn, SLONG page, CBYTE *fontlist);
void MENUFONT_Draw(SWORD x, SWORD y, UWORD scale, CBYTE *msg, SLONG rgb, UWORD flags, SWORD max=-1);
void MENUFONT_Dimensions(CBYTE *fn, SLONG &x, SLONG &y, SWORD max=-1, SWORD scale=256);
```

**Флаги эффектов:**
```c
#define MENUFONT_FUTZING    (1)    // пульсирование
#define MENUFONT_HALOED     (2)    // ореол
#define MENUFONT_GLIMMER    (4)    // мерцание
#define MENUFONT_CENTRED    (8)    // центрирование
#define MENUFONT_FLANGED   (16)    // фланговый эффект
#define MENUFONT_SINED     (32)    // синусоидальная волна
#define MENUFONT_SHAKE   (1024)    // тряска
#define MENUFONT_RIGHTALIGN (128)  // выравнивание вправо
```

**Fade-in эффект:**
```c
MENUFONT_fadein_init();    // инициализация
MENUFONT_fadein_line(x);   // установить линию фейда
MENUFONT_fadein_draw();    // отрисовка с фейдом
```

### Font2D (моноширинный, для HUD)
```c
#define FONT2D_LETTER_HEIGHT 16

void FONT2D_DrawLetter(CBYTE chr, SLONG x, SLONG y, ULONG rgb=0xffffff, SLONG scale=256, ...);
void FONT2D_DrawString(CBYTE *chr, SLONG x, SLONG y, ...);
void FONT2D_DrawStringWrap(CBYTE *chr, SLONG x, SLONG y, ...);
void FONT2D_DrawStringCentred(CBYTE *chr, SLONG x, SLONG y, ...);
```

**Поддерживаемые символы:** латиница, цифры, пунктуация, немецкие/французские/испанские/итальянские символы.

---

## 5. Диалоги и уведомления

### Текст над персонажами
```c
void PANEL_new_text(Thing *who, SLONG delay, CBYTE *fmt, ...);      // речь персонажа
void PANEL_new_help_message(CBYTE *fmt, ...);                       // подсказка игроку
void PANEL_new_info_message(CBYTE *fmt, ...);                       // информационное сообщение
```

### Система разговоров NPC (из pcom.h)
```c
void PCOM_make_people_talk_to_eachother(
    Thing *p_person_a, Thing *p_person_b,
    UBYTE is_a_asking_a_question,
    UBYTE stay_looking_at_eachother,
    UBYTE make_the_person_talked_at_listen = TRUE);

void PCOM_stop_people_talking_to_eachother(Thing *p_person_a, Thing *p_person_b);
```

**Состояния диалога:**
```c
PCOM_AI_SUBSTATE_TALK_ASK    = 21  // задаёт вопрос
PCOM_AI_SUBSTATE_TALK_TELL   = 22  // рассказывает
PCOM_AI_SUBSTATE_TALK_LISTEN = 23  // слушает
```

### Аудиоречь
- Файлы в `talk2/` с поддержкой нескольких языков
- Локализация через `xlat_str()` функции

### Damage Text (если DAMAGE_TEXT define)
```c
struct DamageValue {
    UBYTE Type;    // INFO_NUMBER (1) или INFO_TEXT (2)
    CBYTE *text_ptr;
    SWORD Age;
    SWORD Value;   // урон
    SWORD X, Y, Z; // 3D позиция
};
#define MAX_DAMAGE_VALUES 16
```

### Help System
```c
#define HELP_GRAB_CABLE  0  // "Jump up to grab cables"
#define HELP_PICKUP_ITEM 1  // "Press action to pickup items"
#define HELP_USE_CAR     2  // "Press action to use vehicles"
#define HELP_USE_BIKE    3  // "Press action to use bikes"

SLONG should_i_add_message(SLONG type);  // не показывалось ли недавно?
void arrow_object(Thing *p_special, SLONG dir, SLONG type);  // стрелка над объектом
void arrow_pos(SLONG x, SLONG y, SLONG z, SLONG dir, SLONG type);
```

---

## 6. Рендеринг UI

### Poly Pages для UI
```c
POLY_PAGE_FONT2D         // шрифты
POLY_PAGE_COLOUR         // монохромные фигуры
POLY_PAGE_ALPHA          // полупрозрачные
POLY_PAGE_ALPHA_OVERLAY  // оверлей с альфой
POLY_PAGE_FACE1...FACE6  // портреты персонажей
```

### Базовая функция отрисовки квада
```c
void PANEL_draw_quad(float left, float top, float right, float bottom, SLONG page,
                     ULONG colour=0xffffffff, float u1=0, float v1=0, float u2=1, float v2=1);
```

### Глубина на Dreamcast
```c
#ifdef TARGET_DC
float PANEL_GetNextDepthBodge();  // RHW для сортировки (небольшое смещение каждый раз)
void PANEL_ResetDepthBodge();     // сброс в начале кадра
#endif
```

