# Звуковая система — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/Sound.cpp` — основная звуковая логика
- `fallen/Headers/Sound.h` — константы и структуры
- `fallen/Source/Music.cpp` — музыкальная система
- `fallen/Headers/Music.h` — режимы музыки
- `fallen/DDLibrary/MFX.cpp` — обёртка над Miles Sound System
- `fallen/Headers/MFX.h` — MFX API

---

## 1. Движок: Miles Sound System (MSS32)

**Библиотека:** `mss32.lib` / `mss32.dll` — проприетарная. **Заменить при портировании.**

MFX — тонкая обёртка над MSS32:
```c
MFX_play(channel, wave_id, flags)           // 2D звук
MFX_play_xyz(channel, wave_id, flags,
             x>>8, y>>8, z>>8)              // 3D позиционный звук
MFX_stop(channel)                           // Остановить канал
MFX_set_volume(channel, volume)             // Громкость 0..127
MFX_update_xyz(channel, x>>8, y>>8, z>>8)  // Обновить 3D позицию
```

Координаты для 3D аудио передаются со сдвигом `>>8` (из world units → mapsquares).

---

## 2. Каналы (Channel IDs)

Каналы разделены на группы:

```c
// Каналы привязанные к объектам:
// Каждый Thing может иметь свой звуковой канал
// FLAGS_HAS_ATTACHED_SOUND — флаг наличия привязанного звука

// Специальные каналы (вне диапазона Things):
WIND_REF    = MAX_THINGS + 100  // Ветер (фоновый)
WEATHER_REF = MAX_THINGS + 101  // Погода (дождь)
MUSIC_REF   = MAX_THINGS + 102  // Музыкальный канал
// + ещё несколько каналов для ambient (до +105)
```

---

## 3. 3D Позиционный звук

```c
// При создании звука у объекта:
if (thing->Flags & FLAGS_HAS_ATTACHED_SOUND) {
    MFX_update_xyz(channel,
        thing->WorldPos.X >> 8,
        thing->WorldPos.Y >> 8,
        thing->WorldPos.Z >> 8);
}

// Звук двигателя машины — обновляется каждый кадр
// Голоса, шаги — однократные события
```

---

## 4. Музыкальные режимы (MUSIC_MODE_*)

14 режимов динамической музыки с плавными переходами:

| Режим | Описание |
|-------|----------|
| `MUSIC_MODE_NONE` | Нет музыки |
| `MUSIC_MODE_CRAWL` | Осторожное движение / скрытность |
| `MUSIC_MODE_DRIVING1` | Езда на машине (вариант 1) |
| `MUSIC_MODE_DRIVING2` | Езда на машине (вариант 2) |
| `MUSIC_MODE_DRIVING_START` | Начало езды |
| `MUSIC_MODE_FIGHT1` | Бой (вариант 1) |
| `MUSIC_MODE_FIGHT2` | Бой (вариант 2) |
| `MUSIC_MODE_SPRINT1` | Бег (вариант 1) |
| `MUSIC_MODE_SPRINT2` | Бег (вариант 2) |
| `MUSIC_MODE_AMBIENT` | Фоновая атмосфера |
| `MUSIC_MODE_CINEMATIC` | Катсцена |
| `MUSIC_MODE_VICTORY` | Победа в миссии |
| `MUSIC_MODE_FAIL` | Провал миссии |
| `MUSIC_MODE_MENU` | Главное меню |

**Файлы музыки** (`data/sfx/1622/musik01-musik08/`):
- 8 вариантов с подпапками по действиям: `Crawl`, `Driving1`, `Driving2`, `DrivingStart`, `Fight1`, `Fight2`, `Sprint1`, `Sprint2`
- Смена происходит плавно через fade-out / fade-in

---

## 5. Логика смены музыки

```c
// Приоритет режимов (от высшего к низшему):
// 1. CINEMATIC (катсцены)
// 2. FIGHT1/FIGHT2 (обнаружен враг)
// 3. DRIVING1/DRIVING2 (в машине)
// 4. SPRINT1/SPRINT2 (бег)
// 5. CRAWL (скрытное движение)
// 6. AMBIENT (стоит на месте)

// Переход:
//   1. Fade out текущего трека (несколько секунд)
//   2. Fade in нового трека
//   3. Выбор конкретного файла из musik01-musik08 — случайный или по типу мира
```

---

## 6. Ambient система (Sound.cpp)

### 3D позиционирование звуков

```c
void PlayAmbient3D(channel, wave_id, flags, height) {
    // Случайная позиция вокруг игрока (радиус ~16 ед.)
    // Height: PlayerHeight (Y игрока), OnGround (Y=0), InAir (Y+512-1535)
    // Использует COS/SIN из lookup tables
    MFX_play_xyz(channel, wave_id, 0, x, y, z);
}
```

### Биомы ambient (по типу мира)

| Биом | Определение | Звуки |
|------|------------|-------|
| Jungle | texture_set = 1 | Тропический шум, туманный горн (если X > 0x400000), какаду/сверчки/птицы |
| Snow | texture_set = 5 | Вой волков каждые 1500+ кадров |
| Estate | texture_set = 10 | Пролёты самолётов каждые 500+ кадров |
| BusyCity | (default) | Собаки, кошки, бьющееся стекло, полицейское радио |
| QuietCity | texture_set = 16 | Минимальный ambient |

**Тип биома** определяется при загрузке через `TEXTURE_SET` константы.

### Система приоритетов каналов

```c
MFX_REPLACE   // заменить текущий звук на канале
MFX_QUEUED    // добавить в очередь канала
MFX_OVERLAP   // играть параллельно с текущим
MFX_LOOPED    // зациклить
MFX_SHORT_QUEUE  // высокоприоритетная очередь
MFX_WAVE_ALL  // остановить все звуки на канале
```

### Indoor/Outdoor переход

`process_ambient_effects()` каждый кадр:
- Флаг `GF_INDOORS` → fade in `indoors_id`, fade out `outdoors_id`
- На улице → наоборот
- Скорость fade определяется расстоянием до здания

Типы indoor ambient (по `WARE_ware[].ambience`): office, police, posheeta, club.

### Погода

`process_weather()` каждый кадр:
- `above_ground = (y - ground_height) >> 8`
- Дождь: `gain = 255 - (above_ground >> 4)` (громче у земли)
- Ветер: `gain = abs_height >> 4` (громче на высоте)
- Ночью всегда дождь (`GF_RAINING` флаг)

### Голосовые функции персонажей

```c
SOUND_Gender(thing)  // 0=неизвестно, 1=мужской, 2=женский голос
DieSound(thing)      // звук смерти по типу персонажа
PainSound(thing)     // звук попадания
EffortSound(thing)   // звук усилия
ScreamFallSound(thing) // крик при падении насмерть
```

Таймер ambient-звуков: `creature_time = 400`, `siren_time = 300`.

---

## 7. Звуки шагов

```c
// Флаг на Thing:
FLAGS_PLAYED_FOOTSTEP  // bit 12 — звук шага воспроизведён в этом кадре

// Логика:
// - Шаг воспроизводится при касании земли (анимационный keyframe)
// - Тип звука: асфальт, металл, трава — по типу поверхности (PAP_Hi.Texture)
// - Флаг сбрасывается каждый кадр перед обработкой
```

---

## 8. Диалоги

**Расположение:** `talk2/`
**Формат:** WAV, именование: `{LevelName}.ucm{EventIndex}.wav` или `{CharacterName}{Index}.wav`

```c
// Воспроизведение диалога (из WPT_PLAY_DIALOG):
MFX_play(DIALOG_CHANNEL, wave_id, MFX_FLAG_DIALOG);
// Субтитры: соответствующая строка из text/ файлов
```

---

## 9. Параметры громкости (из config.ini)

```ini
[Audio]
ambient_volume=127    // 0..127
music_volume=127      // 0..127
fx_volume=127         // 0..127
3D_sound_driver=Microsoft DirectSound3D  // ЗАМЕНИТЬ при портировании
```

---

## 10. Портирование

**MSS32 → miniaudio:**

| MSS32 / MFX | miniaudio |
|-------------|-----------|
| `MFX_play_xyz()` | `ma_sound_set_position()` + `ma_sound_start()` |
| `MFX_update_xyz()` | `ma_sound_set_position()` |
| `MFX_set_volume()` | `ma_sound_set_volume()` |
| `MFX_stop()` | `ma_sound_stop()` |
| 3D positional | `ma_engine` с `MA_SOUND_FLAG_SPATIALIZATION` |
| Fade transitions | `ma_sound_set_volume()` с timer |

**Альтернативы:** SDL_mixer (проще, менее гибкий 3D), OpenAL (мощнее, тяжелее).

**Рекомендация:** miniaudio — single-header, кросс-платформенный, поддерживает 3D spatial audio, легко интегрируется.

---

## 11. Что переносить в новую версию

**Переносить 1:1:**
- 14 MUSIC_MODE_* с той же логикой переключения и приоритетами
- 3D позиционный звук (обновление позиции каждый кадр для движущихся объектов)
- FLAGS_HAS_ATTACHED_SOUND механизм
- Ambient систему (5 типов по texture_set)
- Систему диалогов (WAV + субтитры из text/)
- Параметры громкости из config.ini

**Заменить:**
- MSS32 → miniaudio (или SDL_mixer как fallback)
- DirectSound3D driver → кросс-платформенная 3D аудио-система
