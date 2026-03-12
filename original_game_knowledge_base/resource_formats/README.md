# Форматы ресурсов — Urban Chaos

Ресурсы находятся в `original_game/fallen/Debug/` (Steam-версия, рабочая).
Загрузчик ресурсов переносится 1:1 в новую версию.

**Ключевые файлы загрузки:** `fallen/Source/io.cpp`, `fallen/Source/Level.cpp`, `fallen/Headers/io.h`

---

## Структура папки ресурсов

```
Debug/
├── levels/           — скрипты уровней (.ucm — скомпилированный MuckyBasic)
├── Meshes/           — 3D модели (.SEX исходники, .IMP скомпилированные)
├── Textures/         — текстуры (.TGA, 32-bit RGBA)
├── clumps/           — архивы текстур уровней (.TXC)
├── data/
│   ├── sfx/1622/     — звуки (WAV, PCM 16-bit)
│   │   ├── Ambience/ — фоновые звуки
│   │   ├── music01-08/ — музыка по вариантам мира
│   │   └── ...
│   ├── Lighting/     — файлы освещения уровней (.lgt)
│   └── font3d/       — 3D шрифты
├── talk2/            — диалоги (WAV, имена кодированы: Level.ucm{index}.wav)
├── text/             — локализованные строки (TXT)
├── bink/             — Bink-видео катсцены (.bik)
├── stars/            — текстуры звёздного неба
├── server/           — (назначение уточнить)
└── config.ini        — конфигурационный файл игры
```

---

## Форматы по типам

### 1. Карты уровней (`.pam`, `.pi`, `.pp`)

**Функция загрузки:** `io.cpp::load_game_map()`

**Бинарный формат:**
```
[4 байта]   SLONG save_type  — версия файла
[если save_type > 23]: [4 байта] размер OB-данных

[PAP_Hi данные] — высокое разрешение (128×128)
  struct PAP_Hi {                    // 8 байт каждый
      UWORD Texture;    // индекс текстуры (3 старших бита свободны)
      UWORD Flags;      // PAP_FLAG_SHADOW_1/2/3, PAP_FLAG_REFLECTIVE, PAP_FLAG_HIDDEN и т.д.
      SBYTE Alt;        // высота (умножить на 8 для реальной)
      SBYTE Height;     // не используется
  };
  Итого: 128 × 128 × 8 = 131 072 байт

[PAP_Lo данные] — низкое разрешение (32×32)
  struct PAP_Lo {                    // 8 байт каждый
      UWORD MapWho;       // индекс объекта/маршрута
      SWORD Walkable;     // >0 = prim_face4, <0 = крыша
      UWORD ColVectHead;  // голова linked list коллизионных векторов
      SBYTE water;        // высота воды (-127 = нет воды)
      UBYTE Flag;         // флаги warehouse
  };

[объекты уровня] — позиции и параметры всех объектов
  struct LoadGameThing {
      UWORD Type;         // класс объекта (CLASS_*)
      UWORD SubStype;     // подтип
      SLONG X, Y, Z;      // позиция в мировых координатах
      ULONG Flags;        // флаги объекта
      UWORD IndexOther;   // индекс связанного объекта
      UWORD AngleX/Y/Z;   // углы поворота
      ULONG Dummy[4];     // резерв
  };

[если save_type >= 20]: [4 байта] SLONG texture_set — набор текстур (0-21)

[SuperMap данные] — load_super_map(handle, save_type)
[текстуры крыши] — если save_type >= 26: 128×128 × 2 байта
```

**Наборы текстур (texture_set 0–21):** определяют визуальный стиль уровня (город, снег, лес и т.д.)

---

### 2. 3D Модели

#### .SEX файлы — исходный текстовый формат
ASCII текст, похожий на 3DS MAX экспорт:
```
# комментарий
Version: 3.14

Triangle mesh: {MeshName}
    Pivot: (x, y, z)
    Matrix: (m00, m01, m02, m10, m11, m12, m20, m21, m22)  -- матрица 3×3
    Material: DiffuseRGB (r,g,b), shininess 0.25, ...
    Vertex: (x, y, z)
    Texture Vertex: (u, v)
    Face: v1, v2, v3 [, v4]  -- треугольник или квадрат
```

#### .IMP файлы — скомпилированный бинарный формат
```
[4 байта]  SLONG версия = 1
[4 байта]  SLONG количество сеток
Для каждой сетки:
  [C-строка] имя сетки (нулл-терминированная)
  Для каждого материала:
    [2 байта]  UWORD количество вершин
    [2 байта]  UWORD количество граней
    [6 байт]   UWORD начало/конец индексов (3 × UWORD)
    [float]    границы 3D box (координаты)
    [3 × float] Diffuse RGB (значения 0..0.5)
    [float]    shininess
    [float]    shinstr (strength)
    [1 байт]   UBYTE flags (single-sided, filtered alpha и т.д.)
    [64 байта] строка текстуры diffuse
    [64 байта] строка bump map
  Вершины + UV + грани (индексированные)
```

#### .TXC файлы — архивы текстур уровней
Бинарный архив. Содержит текстуры для конкретного уровня (TGA → внутренний формат).
Структура заголовка: начинается с 4-байтного значения (вероятно размер/count).

---

### 3. Текстуры

**Формат:** TGA (Targa Image Format)
- 32-bit RGBA или 24-bit RGB
- Размеры: 512×512, 256×256, 128×128, 64×64, 32×32 пикселей
- Мип-маппинг: рассчитывается при загрузке, не хранится в файле

**Функция:** `TEXTURE_choose_set(texture_set)` выбирает набор текстур по типу мира.

---

### 4. Анимации

**Где хранятся:** встроены в файлы примов (внутри структур PrimObject), отдельных файлов нет.

**Структура KeyFrame:**
```c
struct GameKeyFrame {
    SBYTE dx, dy, dz;    // смещение относительно базовой позиции
    SBYTE rotation[3];   // углы Эйлера
    // дополнительные поля анимации
};
```

**Загрузка:** `load_anim_system(GameKeyFrameChunk *chunk, CBYTE *name)`

---

### 5. Звук

**Основные звуки** (`data/sfx/1622/`): WAV, PCM 16-bit, моно или стерео.
Организованы по категориям: Ambience, Footsteps, General, музыка.

**Диалоги** (`talk2/`): WAV файлы.
Именование: `{LevelName}.ucm{EventIndex}.wav` или `{CharacterName}{Index}.wav`.
Пример: `AWOL1.ucm12.wav`, `POLICEM10.wav`.

**Музыка:** 8 вариантов (musik01–musik08) с подпапками по действиям:
- `Crawl`, `Driving1`, `Driving2`, `DrivingStart`, `Fight1`, `Fight2`, `Sprint1`, `Sprint2`
- Динамическая смена в зависимости от действий игрока.

**Движок:** Miles Sound System (mss32.lib) — необходимо заменить на кросс-платформенный (miniaudio или SDL_mixer).

---

### 6. Скрипты (.ucm)

**Формат:** Бинарный bytecode MuckyBasic (LINK_Header + инструкции + data table).

**Структура начала файла (hex):**
```
0a 00 00 00   -- первые 4 байта (значение ~10)
01 00 00 00   -- вторые 4 байта (версия?)
00 00 00 00   -- дополнительные поля
[нули]        -- область инициализации
[offset 0x100+] -- строка пути файла освещения, напр. Data\Lighting\gpost3.lgt
```

Подробнее — см. [subsystems/muckybasic.md](../subsystems/muckybasic.md).

---

### 7. Конфигурация (`config.ini`)

INI-формат (Windows-стиль). Ключевые параметры:

```ini
[Game]
language=text\lang_english.txt  # путь к файлу строк
scanner_follows=1

[Render]
draw_distance=22                 # дальность видимости
video_res=4                      # разрешение экрана
max_frame_rate=30                # ограничение FPS
detail_stars=1                   # звёзды
detail_shadows=1                 # тени
detail_puddles=1                 # лужи
detail_rain=1                    # дождь

[Audio]
ambient_volume=127
music_volume=127
fx_volume=127
3D_sound_driver=Microsoft DirectSound3D  # заменить!

[Joypad]
joypad_kick=4                    # DirectInput кнопки
joypad_punch=3
joypad_jump=0
joypad_action=1
joypad_move=7
joypad_camera=6

[Keyboard]
keyboard_left=203                # DirectInput коды клавиш
keyboard_right=205
```

---

### 8. Текстовые строки (`text/`)

**Формат:** Простой текстовый файл.
- Первая строка: название языка (`English`, `French`, `German`...)
- Каждая следующая строка: одна UI-строка
- Управляющие коды: `|16`, `|4`, `|1`, `|7` — специальные символы/кнопки

**Локализации:**
- `lang_english.txt`, `lang_french.txt`, `lang_german.txt`, `lang_italian.txt`, `lang_spanish.txt`
- Для уровней: `citsez.txt`, `citsez_french.txt` и т.д.

**Пример:**
```
English
Press |16 to continue.
- G A M E   O V E R -
The car is locked.
Level Complete
```

---

## Приоритет для загрузчиков в новой версии

| Приоритет | Формат | Сложность | Критичность |
|-----------|--------|-----------|-------------|
| 1 | Текстуры (.TGA) | Низкая — стандартный формат | Нужна сразу |
| 2 | Текстовые строки | Низкая | Нужна сразу |
| 3 | Конфиг (config.ini) | Низкая | Нужна сразу |
| 4 | Карты уровней (.pam/.pi/.pp) | Высокая | Критична |
| 5 | 3D модели (.IMP) | Средняя | Критична |
| 6 | Звук (.WAV) | Низкая | Важна |
| 7 | Скрипты (.ucm) | Очень высокая | Критична для миссий |
| 8 | Анимации | Средняя | Критична |
