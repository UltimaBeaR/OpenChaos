# Видеоролики (Bink Video Playback)

**Задача:** восстановить воспроизведение .bik видеороликов, вырезанное при модернизации.
**Статус:** ✅ ГОТОВО
**Трекер:** `known_issues_and_bugs.md` → "Видеоролики не воспроизводятся" (исправлено)

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

### 2026-04-03: Реализация завершена

**Что сделано:**
- FFmpeg добавлен как зависимость через vcpkg (avcodec, avformat, swscale, swresample)
- Новый модуль: `engine/video/video_player.cpp` + `.h`
- Видео: FFmpeg decode → sws_scale (RGB24) → GL текстура → fullscreen quad (собственный шейдер)
- Аудио: FFmpeg decode → swr_convert (S16) → OpenAL streaming (4 буферных слота, аккумулятор без потерь)
- A/V sync: wall clock master, видео ждёт по pts
- Letterboxing: автоматический aspect ratio с чёрными полосами
- Скип: клавиатура (Esc/Enter/Space), мышь, SDL gamepad, DualSense (edge-triggered)
- DualSense: snapshot состояния при старте ролика, скип только на новое нажатие (не hold)
- Интеграция: game.cpp/elev.cpp вызывают `video_play_cutscene()` напрямую, `video_play_intro()` при старте
- Рендер через ge_video_* API (backend-agnostic): создание текстуры, upload, fullscreen quad + swap
- Интро вызывается после `GetInputDevice()` чтобы DualSense был инициализирован

**Проблемы решённые в процессе:**
- Рассинхрон A/V: начальная реализация использовала audio master clock через AL_SAMPLE_OFFSET — 
  ненадёжно (сбрасывается при underrun). Переключено на wall clock.
- Аудио артефакты (щелчки): streaming дропал данные при полных буферах. 
  Решено через accumulation buffer — данные никогда не теряются.
- DualSense не скипал: ds_init() вызывался после video_play_intro(). Перенесён вызов.
- DualSense скипал все 3 ролика: кнопка оставалась нажатой. Добавлен edge detection.
- Зависание (deadlock): sync loop ждал audio clock, который не двигался после underrun.
  Устранено переходом на wall clock.

**Файлы:**
- `new_game/src/engine/video/video_player.h` — API (3 функции)
- `new_game/src/engine/video/video_player.cpp` — реализация (~550 строк)
- `new_game/CMakeLists.txt` — FFmpeg find_package, линковка, DLL копирование
- `new_game/vcpkg.json` — зависимость ffmpeg
- `new_game/src/engine/graphics/.../backend_opengl/game/core.cpp` — ge_run_cutscene stub → video_play_cutscene
- `new_game/src/game/game.cpp` — video_play_intro() при старте
