# `libs/` contents — что положить и откуда взять

Папка `dualsense_libs_reference/libs/` под **gitignore** — локальная,
не коммитится в репо. Чтобы воссоздать её с нуля, скачай все репо из
списка ниже и положи каждое в указанную подпапку.

**Формат каждого пункта**:
- **Репо**: ссылка на GitHub
- **Куда положить**: путь относительно `dualsense_libs_reference/libs/`
- **Проверка**: файл который должен существовать после распаковки —
  если есть, значит всё правильно распаковалось
- **Доп. действия**: если нужно что-то ещё сделать после распаковки

---

## Обязательные (5 штук)

### 1. duaLib — ⭐ главный источник истины для trigger effects

- **Репо**: https://github.com/Nielk1/duaLib
- **Куда положить**: `libs/duaLib/`
- **Проверка**: `libs/duaLib/src/source/triggerFactory.cpp`
- **Доп. действия**: нет
- **Зачем**: файл `triggerFactory.cpp` — авторитетная C++ реализация
  всех trigger effect packing'ов от автора спеки (Nielk1, MIT).
  Будем частично копировать в нашу либу.

### 2. DualSense-Windows (DS5W) — ⭐ input parsing reference

- **Репо**: https://github.com/Ohjurot/DualSense-Windows
- **Куда положить**: `libs/DualSense-Windows/`
- **Проверка**: `libs/DualSense-Windows/VS19_Solution/DualSenseWindows/src/DualSenseWindows/DS5_Input.cpp`
- **Доп. действия**: нет
- **Зачем**: независимый рабочий C++ код с корректным input parser'ом.
  Подтверждает наши feedback offsets `0x29`/`0x2A`. Плюс CRC32 для
  BT output reports.

### 3. nondebug/dualsense — HID offsets documentation

- **Репо**: https://github.com/nondebug/dualsense
- **Куда положить**: `libs/nondebug-dualsense/`
- **Проверка**: `libs/nondebug-dualsense/dualsense-explorer.html`
- **Доп. действия**: нет
- **Зачем**: HTML страница содержит JavaScript с хардкоженными
  offsets всех полей input report'а. Reference для layout байтов.

### 4. rafaelvaloto pristine — чистая копия текущей вендоренной либы

- **Репо**: https://github.com/rafaelvaloto/Dualsense-Multiplatform
- **Куда положить**: `libs/rafaelvaloto-pristine/`
- **Проверка**: `libs/rafaelvaloto-pristine/Source/Public/GImplementations/Utils/GamepadTrigger.h`
- **Доп. действия**: нет. Важно: файл **НЕ** должен содержать
  маркера `OPENCHAOS-PATCH` — это именно **чистая** копия для
  baseline diff'ов. Если маркер есть — значит скачал не из апстрима,
  а из нашего форка по ошибке.
- **Зачем**: baseline для diff'ов против нашей патченой версии, для
  подготовки upstream PR.

### 5. rafaelvaloto-patched — зеркало нашего патченого форка

- **Репо**: **не с GitHub**. Это копия из папки `new_game/libs/dualsense-multiplatform/`
  этого же репозитория.
- **Куда положить**: `libs/rafaelvaloto-patched/`
- **Проверка**: `libs/rafaelvaloto-patched/Dualsense-Multiplatform/Source/Public/GImplementations/Utils/GamepadTrigger.h`
  должен содержать строку `=== OPENCHAOS-PATCH BEGIN: Weapon25 packing rewrite ===`
- **Доп. действия**: скопируй папку целиком:
  - **Windows**: через Explorer — выдели `new_game/libs/dualsense-multiplatform/`,
    скопируй (Ctrl+C), вставь в `dualsense_libs_reference/libs/`, переименуй
    вставленную папку в `rafaelvaloto-patched`.
  - **Или из командной строки (любая OS)**:
    ```
    cp -r new_game/libs/dualsense-multiplatform dualsense_libs_reference/libs/rafaelvaloto-patched
    ```
- **Зачем**: удобно делать `diff -ru libs/rafaelvaloto-pristine libs/rafaelvaloto-patched/Dualsense-Multiplatform`
  чтобы видеть все наши локальные патчи одним выводом, без прыжков
  по дереву репо.
- **Обновление**: когда в `new_game/libs/dualsense-multiplatform/`
  появляются новые патчи — удали `libs/rafaelvaloto-patched/` и
  скопируй заново.

---

## Опциональные

### 6. DualSenseX (Paliverse)

- **Репо**: https://github.com/Paliverse/DualSenseX
- **Куда положить**: `libs/DualSenseX-Paliverse/`
- **Проверка**: `libs/DualSenseX-Paliverse/UDP Example (C#) for v2.0/DSX_UDP_Example.zip`
- **Доп. действия**: распакуй внутренний архив:
  - Найди файл `UDP Example (C#) for v2.0/DSX_UDP_Example.zip`
  - Распакуй его содержимое в соседнюю папку `UDP Example (C#) for v2.0/extracted/`
  - Проверка после распаковки: `libs/DualSenseX-Paliverse/UDP Example (C#) for v2.0/extracted/DSX_UDP_Example/DSX_UDP_Example/Resources.cs`
- **Зачем**: проект закрытый (exe не в репо), но внутренний UDP
  example содержит C# файлы `Resources.cs` и `Program.cs` с enum'ами
  и комментариями про parameter ranges каждого trigger эффекта. Это
  наш "API contract" от работающего коммерческого продукта — полезен
  как sanity check для наших констант. Основной код DualSenseX
  закрытый, его там нет.

### 7. Nielk1 trigger gist — backup raw text

- **URL**: https://gist.githubusercontent.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db/raw
- **Куда положить**: `libs/nielk1_trigger_gist.txt` (отдельный файл,
  не папка)
- **Проверка**: файл существует и содержит слова `Weapon` и `startAndStopZones`
- **Доп. действия**: просто сохрани содержимое URL как plain text
  (правый клик → "Save As" в браузере, или `curl -L ... > ...`)
- **Зачем**: резервная копия на случай если GitHub rate-limit'ит
  или WebFetch агента возвращает 403. Содержит исходные formulas
  для packing'ов (уже продублированы в `dualsense_lib_pr_notes.md`
  но оригинал может содержать context что мы не скопировали).

### 8. dogtopus HID descriptor gist — backup

- **URL**: https://gist.githubusercontent.com/dogtopus/894da226d73afb3bdd195df41b3a26aa/raw
- **Куда положить**: `libs/dogtopus_hid_descriptor.txt`
- **Проверка**: файл существует
- **Доп. действия**: нет, просто сохрани содержимое URL как plain text
- **Зачем**: низкоуровневый HID descriptor DualSense как справочник.
  Не критично, но может пригодиться при отладке новой либы.

---

## Итоговая структура после установки всех обязательных

```
dualsense_libs_reference/libs/
├── duaLib/                              <- пункт 1
├── DualSense-Windows/                   <- пункт 2
├── nondebug-dualsense/                  <- пункт 3
├── rafaelvaloto-pristine/               <- пункт 4
└── rafaelvaloto-patched/                <- пункт 5 (копия из new_game/libs/)
```

Опциональные (если качал):
```
libs/
├── DualSenseX-Paliverse/                <- пункт 6 (+ распаковать внутренний zip)
├── nielk1_trigger_gist.txt              <- пункт 7 (файл)
└── dogtopus_hid_descriptor.txt          <- пункт 8 (файл)
```

---

## Обновление копий (когда исходники меняются)

- **Обязательные 1-4**: просто удали старую папку и качни новую
  версию с GitHub.
- **Пункт 5 (rafaelvaloto-patched)**: удали `libs/rafaelvaloto-patched/`
  и скопируй заново из `new_game/libs/dualsense-multiplatform/`. Это
  нужно каждый раз когда в игровом патче появляются правки под
  маркерами `OPENCHAOS-PATCH`.
- **Опциональные**: нужно редко, обновляй по желанию.

**Важно про пункт 4 (pristine)**: если в апстриме
`rafaelvaloto/Dualsense-Multiplatform` появятся новые коммиты — надо
не только обновить pristine копию, но и проверить что наш патч в
`new_game/libs/dualsense-multiplatform/` всё ещё применяется поверх
новой базы. Это может потребовать merge/rebase work.

---

## Зачем это всё

Главная задача — **переписать DualSense-зависимость OpenChaos с
вендоренной broken либы на свою минимальную**. План — [`own_dualsense_lib_plan.md`](own_dualsense_lib_plan.md).
Референсные либы нужны чтобы:

1. **Заимствовать `triggerFactory.cpp`** из duaLib (MIT) — готовая
   авторитетная реализация всех trigger effect packing'ов.
2. **Сверять input parser** с DS5W — независимая работающая
   реализация, подтверждает наши feedback offsets.
3. **Сверять HID offsets** с nondebug — исходная документация всех
   полей input report'а.
4. **Получать diff'ы** между pristine и patched rafaelvaloto — для
   подготовки upstream PR или для понимания каких фич мы оставляем
   позади при миграции на свою либу.

Без локальных копий каждый шаг работы над своей либой требовал бы
web fetches и лотереи с rate limits. С ними вся работа **offline,
быстрая, воспроизводимая**.
