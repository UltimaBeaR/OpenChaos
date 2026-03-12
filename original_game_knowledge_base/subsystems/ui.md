# UI и фронтенд — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/Panel.cpp`, `fallen/Headers/Panel.h` — HUD, прицел, полосы здоровья
- `fallen/Source/Frontend.cpp`, `fallen/Headers/Frontend.h` — главное меню, экраны
- `fallen/Source/GameMenu.cpp`, `fallen/Headers/GameMenu.h` — меню паузы, победа/поражение
- `fallen/Source/MenuFont.cpp`, `fallen/Headers/MenuFont.h` — шрифт меню
- `fallen/Source/Font2D.cpp`, `fallen/Headers/Font2D.h` — игровой шрифт (HUD)

---

## 1. HUD (Heads-Up Display)

### Health Bar
- **Позиция PC:** (10, 10), размер 100×10 пикселей
- **Отрисовка:** красная полоса на чёрном фоне, padding 2px
- **Вычисление:** `health >> 1` (здоровье 0–200 → ширина 0–100)

### Stamina Bar
- **Позиция:** (10, 30)
- **Вычисление:** `(stamina * 100) >> 8`

### Прицел (Gun Sight)
```c
void PANEL_draw_gun_sight(x, y, z, radius, scale);
```
Масштаб зависит от типа цели:
- Person: scale = 256
- Special / Barrel: scale = 128–200
- Vehicle: scale = 450
- Boss (Balrog): scale = 450

Позиция привязана к голове персонажа (sub-object #11), Z смещается на 30–80 px.

### Полосы здоровья врагов (Enemy Health Bars)
- Отображаются над врагом во время боя
- Для MIB: `health * 100 / 700`
- Радиус отрисовки: 60–300 px в зависимости от дистанции

### Отслеживание врагов (Tracking System)
```c
struct TrackEnemy {
    Thing *PThing;
    SWORD State;    // 0 = UNUSED, 1 = TRACKING
    SWORD Timer;
    SWORD Face;
};
#define MAX_TRACK 4   // максимум 4 врага одновременно
```
Функции: `track_enemy()`, `track_gun_sight()`

### Портреты и здоровье врагов на HUD
```c
// PC: до 4 врагов, координаты: x = i*150+5, y = 436
PANEL_draw_face(c0*150+5, 450-14, face, 32);
PANEL_draw_health_bar(40+c0*150, 450, health);
```

### Другие элементы HUD
```c
void PANEL_draw_number(float x, float y, UBYTE digit); // 7-сегментный дисплей (время)
void PANEL_draw_completion_bar(SLONG completion);       // прогресс-бар (0-256)
void PANEL_draw_search(SLONG timer);                    // экран обыска
void PANEL_inventory(Thing *darci, Thing *player);      // инвентарь
void PANEL_flash_sign(SLONG which, SLONG flip);         // дорожные знаки в транспорте
```

**Знаки для транспорта:**
```c
#define PANEL_SIGN_WHICH_UTURN                0
#define PANEL_SIGN_WHICH_TURN_RIGHT           1
#define PANEL_SIGN_WHICH_DONT_TAKE_RIGHT_TURN 2
#define PANEL_SIGN_WHICH_STOP                 3
```

---

## 2. Система меню (Frontend)

### Режимы Frontend
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
```

### Структуры меню
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

### Типы элементов меню
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

### Фоны меню (TGA текстуры, 4 сезонных варианта)
```
"title leaves1.tga", "title rain1.tga", "title snow1.tga", "title blood1.tga"
"map leaves darci.tga", "map rain darci.tga", ...
"briefing leaves darci.tga", "briefing rain darci.tga", ...
"config leaves.tga", "config rain.tga", ...
```

### DirectDraw Surfaces для фонов
```c
LPDIRECTDRAWSURFACE4 screenfull_back  = NULL;  // главное меню
LPDIRECTDRAWSURFACE4 screenfull_map   = NULL;  // выбор районов
LPDIRECTDRAWSURFACE4 screenfull_config= NULL;  // конфиг
LPDIRECTDRAWSURFACE4 screenfull_brief = NULL;  // брифинг
```

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

---

## 7. Что переносить в новую версию

| Компонент | Подход |
|-----------|--------|
| HUD (health, stamina, прицел) | Перенести 1:1 по позициям, заменить DirectX на ImGui/собственный рендер |
| Tracking system (4 врага) | Перенести 1:1 |
| Frontend (меню) | Переработать визуально, логику меню перенести 1:1 |
| GameMenu (пауза/победа/поражение) | Перенести 1:1 |
| MenuFont эффекты | Перенести основные (futzing, centred, shake) |
| Font2D | Заменить на TrueType шрифт (FreeType/stb_truetype) |
| Фоны меню (сезонные TGA) | Перенести (те же TGA ресурсы) |
| Диалоги NPC | Перенести 1:1 |
| Help messages | Перенести 1:1 |
| DirectDraw Surfaces | Заменить на текстуры (OpenGL/SDL) |
| Портреты врагов (FACE1-6) | Перенести (те же текстуры) |
| PSX/DC специфика | Удалить |
