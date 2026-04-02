# Видеоролики (Bink Video Playback)

**Задача:** восстановить воспроизведение .bik видеороликов, вырезанное при модернизации.
**Трекер:** `known_issues_and_bugs.md` → "Видеоролики не воспроизводятся"

---

## Исходное состояние

**Видеофайлы** (6 штук, формат Bink Video, в `original_game_resources/bink/`):
- `Eidos.bik` — логотип издателя
- `logo24.bik` — логотип Mucky Foot
- `PCINTRO_withsound.bik` — основное интро
- `New_PCcutscene1_300.bik` — катсцена 1 (после park2, MIB intro)
- `New_PCcutscene2_300.bik` — катсцена 2 (перед финальным уровнем)
- `New_PCcutscene3_300.bik` — катсцена 3 (финальные титры)

**Оригинальная реализация:** Bink SDK (RAD Game Tools) — проприетарный, вырезан при форке.

**Текущие точки интеграции (все — заглушки):**

| Функция | Где | Что делает |
|---------|-----|------------|
| `ge_run_cutscene(id)` | `game_graphics_engine.h:314` | Общий API, вызывается из игрового кода |
| → OpenGL backend | `backend_opengl/game/core.cpp:1373` | `{}` — пустая заглушка |
| → DX6 backend | `backend_directx6/game/core.cpp:551` | Делегирует в `the_display.RunCutscene(id)` |
| `PlayQuickMovie(type, lang, skip)` | `backend_directx6/common/display.cpp:273` | Заглушка с комментарием |
| `Display::RunFMV()` | `display.cpp:552` | Вызывает `PlayQuickMovie(0,0,true)` при старте (если `play_movie=1` в ENV) |
| `Display::RunCutscene(which, lang, skip)` | `display.cpp:565` | Вызывает `PlayQuickMovie(which, ...)` |

**Вызовы из игрового кода:**
- `game.cpp:995` — `ge_run_cutscene(1)` — MIB intro после park2
- `game.cpp:999` — `ge_run_cutscene(3)` — финальные титры
- `elev.cpp:2077` — `ge_run_cutscene(2)` — перед финальным уровнем
- `Display::Init()` → `RunFMV()` — интро при запуске (3 ролика: Eidos, logo, intro)

**Важно:** `ge_run_cutscene(id)` — это Bink ролики. Не путать с `PLAYCUTS` системой (in-game скриптовые катсцены с анимациями персонажей) — она работает.

---

## План реализации

### 1. FFmpeg как зависимость

- vcpkg: `ffmpeg[bink]` (нужен feature flag `bink` для декодера Bink Video/Audio)
- Проверить что vcpkg-шный FFmpeg включает `bink` декодер (может быть выключен по умолчанию)
- CMakeLists.txt: `find_package(FFmpeg)` или ручной `find_library`
- Нужные компоненты: `libavformat` (контейнер), `libavcodec` (декодирование), `libavutil` (утилиты), `libswscale` (конвертация пикселей), `libswresample` (конвертация аудио)

### 2. Видеоплеер (новый модуль)

**Файл:** `new_game/src/engine/video/bink_player.cpp` + `.h`

**API (минимальный):**
```c
// Воспроизвести .bik файл полноэкранно. Блокирующий вызов.
// Возвращает когда ролик закончился или пользователь нажал skip.
void video_play(const char* filename, bool allow_skip);
```

**Внутренняя логика:**
1. `avformat_open_input()` → открыть .bik файл
2. `avformat_find_stream_info()` → найти видео и аудио стримы
3. Создать `avcodec_open2()` для каждого стрима
4. Цикл воспроизведения:
   - `av_read_frame()` → читать пакеты
   - Видео пакет → `avcodec_send_packet/receive_frame` → `sws_scale` (→ RGB) → GL текстура → fullscreen quad
   - Аудио пакет → `avcodec_send_packet/receive_frame` → `swr_convert` (→ PCM S16) → OpenAL буфер
   - A/V sync: ждать по video pts (audio идёт потоком через OpenAL queue)
   - Input poll: проверять SDL events → skip при нажатии
5. Cleanup: закрыть кодеки, контексты, освободить текстуру

### 3. Рендеринг видеокадра

- Одна GL текстура (GL_TEXTURE_2D, GL_RGB, размер = размер видео)
- `glTexSubImage2D` каждый кадр (обновление без пересоздания)
- Fullscreen quad — можно использовать существующий TL quad рендеринг или простой отдельный шейдер
- Letterboxing: если аспект видео ≠ аспект окна → чёрные полосы

### 4. Аудио (OpenAL streaming)

- Очередь из 3-4 буферов (OpenAL buffer queue на одном source)
- Заполнять буферы по мере декодирования
- `alSourceQueueBuffers` / `alSourceUnqueueBuffers`
- PCM format: S16, моно или стерео (зависит от .bik)

### 5. A/V синхронизация

- Аудио — master clock (OpenAL играет непрерывно)
- Видео — подстраивается: если video pts < audio time → пропустить кадр; если video pts > audio time → подождать
- Для простых коротких роликов (30 сек — 2 мин) достаточно базовой синхронизации

### 6. Интеграция

- `ge_run_cutscene(id)` в OpenGL бэкенде → `video_play("bink/New_PCcutsceneN_300.bik", true)`
- Маппинг id → файл (как в оригинале: id=1,2,3 → cutscene1,2,3)
- Интро: вызов при старте (аналог `RunFMV`) → 3 ролика последовательно
- Управление: skip по Enter/Escape/Space/геймпад A/B

### 7. Кросс-платформенность

- FFmpeg и OpenAL — кросс-платформенные
- GL текстура + fullscreen quad — уже кросс-платформенное
- SDL events — уже кросс-платформенные
- Никаких platform-specific вещей не нужно

---

## Риски и вопросы

- **FFmpeg Bink decoder:** может быть выключен в vcpkg по умолчанию → нужно проверить feature flags
- **Лицензия FFmpeg:** LGPL-2.1 (как OpenAL Soft) → динамическая линковка обязательна
- **Размер зависимости:** FFmpeg большой, но можно собрать minimal (только нужные декодеры)
- **Альтернатива FFmpeg:** libavcodec Bink decoder был написан для FFmpeg в рамках реверс-инжиниринга, других open-source реализаций нет

---

## Лог работы

(записи добавляются по мере выполнения)
