# Plans — что делать дальше

> Архитектура → [`architecture.md`](architecture.md). Текущие баги → [`known_issues.md`](known_issues.md).

## Ближайшие задачи

### Фикс мельтешащих педестрианов

См. [`known_issues.md`](known_issues.md) баг #1. Стратегия — детерминированный ловящий механизм без угадывания порогов:

**Кандидат A: hook на установку Class из CLASS_NONE в alloc_thing.**
- В [`alloc_thing` thing.cpp:602](../../../new_game/src/things/core/thing.cpp#L602) после `t_thing->Class = classification;` вызывать `render_interp_invalidate(t_thing)`.
- Семантика: «новый жилец появился в slot'е, snapshot должен быть свеже-инициализирован».
- Плюсы: явный момент, не зависит от угадывания.
- Минусы: уже есть hook на `free_thing`, который по идее покрывает то же. Если оба hook'а одновременно — дублирование, но безопасное (просто лишний reset).

**Кандидат B: hook на `add_thing_to_map`.**
- Эта функция вызывается caller'ом **после** заполнения `WorldPos` — на момент вызова thing уже готов к симуляции.
- Если invalidate здесь — первый capture гарантированно увидит правильную WorldPos.
- Минусы: не все Things проходят через `add_thing_to_map` (нужно проверить). Может быть хрупкий контракт.

**Кандидат C: lazy detection в самом capture.**
- Хранить в snapshot `last_class` и `last_drawtype`. При capture сравнивать с актуальными — если отличаются (CLASS_NONE → активный класс или класс сменился), reset valid.
- Плюсы: автоматический, не требует hook'ов.
- Минусы: может пропустить случай где slot реcycled на тот же Class (хотя prev/curr разные, проявление будет как промелёк).

**Рекомендация:** начать с А (наименее инвазивно), проверить по логу что промельки исчезли. Если останутся — добавить C как safety net.

### Расширение углов на DrawMesh

**Vehicle уже сделан** — закрыт отдельной веткой `Genus.Vehicle->Angle/Tilt/Roll` с direction-aware lerp по `Vehicle->VelR`. См. [`known_issues.md`](known_issues.md) → «Закрытые баги».

Что ещё не покрыто:
- **DT_MESH** — статичные mesh-объекты. Углы в `Draw.Mesh->Angle/Tilt/Roll` (UWORD). Большинство таких объектов не вращаются вообще, но некоторые могут (вращающиеся pickup'ы / эффекты).
- **DT_BIKE** — велосипеды. Также через DrawMesh.
- **DT_CHOPPER** — вертолёты. `alloc_chopper` ставит `Draw.Mesh = dm`, формат тот же DrawMesh. Раньше ошибочно числились в Tween-семействе (см. `known_issues.md` → «Закрытые баги», cutscene crash). Если потребуется плавность поворотов вертолёта — расширить DrawMesh-ветку.
- **DT_PYRO** — pyro-эффекты. `alloc_pyro` не ставит `Draw.X` вообще, state в `Genus.Pyro`. Если будет видимое дёрганье — нужен отдельный путь через Genus-структуру (аналогично Vehicle).

Если потребуется — единая инфраструктура: в `ThingSnap` добавить `mesh_angle/tilt/roll_prev/curr` + флаг `has_mesh_angles`, ветку в capture для `DrawType == DT_MESH/DT_BIKE/DT_CHOPPER`, ветку в ctor/dtor.

**Связанная задача:** короткий путь lerp ломается при быстром вращении (>180°/тик). См. [`known_issues.md`](known_issues.md) баг #4 — для Vehicle закрыто через VelR; для других классов остаётся открытым (актуально для CLASS_PROJECTILE/CLASS_PYRO/CLASS_BARREL если они начнут крутиться слишком быстро).

**Приоритет:** низкий. DT_MESH объекты в большинстве статичны, видимых дёрганий пока не наблюдается.

### Партиклы (если потребуется)

`PARTICLE_*` pool интерполируется аналогично — но там **другой контейнер** (не Thing-массив, отдельный pool). Нужна параллельная мини-система:
- `g_particle_snaps[MAX_PARTICLES]`
- Capture в конце physics-tick (после `PARTICLE_Run`)
- Применение в frame-scope перед `PARTICLE_Draw`

**Приоритет:** низкий. Делать только если визуально будет дёрганье партиклов на 60 Hz рендере.

## Открытые вопросы / неясные места

### Mark teleport — где должен срабатывать

Большинство известных кейсов закрыто:
- EWAY scene transitions (`EWAY_DO_CREATE_ENEMY`, `EWAY_DO_MOVE_THING`) → `render_interp_mark_teleport` вызывается в [`eway.cpp`](../../../new_game/src/missions/eway.cpp).
- PCOM wander recycle (machines/pedestrians teleporting across map) → `render_interp_mark_teleport` в [`pcom.cpp`](../../../new_game/src/ai/pcom.cpp) (5 callsites).
- Camera warehouse boundary → `render_interp_mark_camera_teleport` в [`fc.cpp`](../../../new_game/src/camera/fc.cpp).
- DIRT pool recycle → `render_interp_mark_dirt_teleport` в [`dirt.cpp`](../../../new_game/src/world_objects/dirt.cpp) (2 callsites).

Возможные оставшиеся кандидаты для hook'ов (если всплывут визуальные баги):
- **Cutscene cut'ы**: `AENG_set_camera` в [`playcuts.cpp`](../../../new_game/src/missions/playcuts.cpp) — между разными кадрами катсцены камера резко скачет.
- **Смена focus камеры**: в `FC_*` модуле, когда `fc->focus` переключается на другой Thing.
- **Загрузка чекпойнта** / переключение миссии.
- **Респаун Дарси после смерти**.
- **Выход из машины** — Дарси резко позиционируется рядом с машиной.

Стратегия: добавлять `mark_teleport` по факту обнаружения визуальных багов в этих сценариях.

### Что делать с `Genus.*->Draw.Angle` для не-vehicle

Vehicle хранит угол в `Genus.Vehicle->Draw.Angle`. Возможно у других классов тоже есть угол в `Genus`-структуре, отдельной от `Thing.Draw.Tweened/Mesh`. Нужна ревизия:
- `Genus.Chopper`?
- `Genus.Pyro`?
- `Genus.Vehicle` — точно
- Прочие — проверить

Если найдутся — расширить систему ещё на эти случаи.

### Баги формул реалтайм-эффектов под ошибочный physics rate

В `CORE_PRINCIPLE.md` упомянуто: оригинал работал на ~30 Hz (PS1) / ~22 Hz (PC retail), мы сейчас на 20 Hz (дизайновая частота, `THING_TICK_BASE_MS = 1000 / UC_PHYSICS_DESIGN_HZ = 50ms`). При портировании могли быть формулы wall-clock эффектов **подстроенные под 30 Hz** — теперь они бегут чуть быстрее или медленнее реального.

Это **отдельный класс багов**, не специфичный для интерполяции, но может проявиться при ревизии всей задачи fps_unlock. Кандидаты для проверки:
- Rain density / drip-spawn частоты
- Ribbon (электрические провода) — параметры волн
- Sparks (электрические заборы) — частота ис кров
- Fire / immolation animation
- Любые wall-clock анимации с константами кадров вместо мс

Эту ревизию стоит делать **после** того как интерполяция стабилизирована — иначе слишком много мест меняется одновременно.

## Что НЕ планируется

- **Forward extrapolation** (предсказание t+1 по prev/curr). Слишком ненадёжно — при промахе предсказания получается «дёрганье назад». Стандарт игр — backward interpolation, render lag by 1 tick.
- **Поднятие physics Hz с timer scaling**. Это альтернативный путь к плавности (см. [`../fps_unlock.md`](../fps_unlock.md)), но требует огромной правки таймеров и тестирования. Render-интерполяция дешевле и не трогает gameplay.
- **Уменьшение render lag**. 50мс рендер-лага неизбежны при backward interpolation на 20 Hz physics. Если станет важна input latency — это будет другая задача (hz scaling), не доработка интерполяции.
