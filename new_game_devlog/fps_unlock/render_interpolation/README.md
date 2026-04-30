# Render Interpolation — рабочая папка

Подзадача в рамках общей задачи [FPS unlock](../README.md). Цель — плавная отрисовка игрового мира на render-rate выше physics-rate за счёт интерполяции между двумя последними physics-снапшотами.

> **Базовые принципы и контекст всей задачи** → [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md) (читать первым).
> **Технический документ родительской задачи** → [`../fps_unlock.md`](../fps_unlock.md).

## Статус

**В работе.** Базовая система работает — позиции и углы Tween (Дарси, NPC, animals, chopper, pyro), позиция машин, камера интерполируются на render-rate. **Vertex-morph анимация** персонажей теперь тоже плавная (с lerp'ом через границу keyframe и через loop wrap, см. [`architecture.md`](architecture.md)).

Остаточные баги: мельтешащие педестрианы (#1), мигание на смене анимации idle→sprint (#2), поворот машин рывками (#3) — см. [`known_issues.md`](known_issues.md).

Runtime toggle клавишей **3** (`g_render_interp_enabled`). Состояние видно в overlay как `IP: on/off`. Под основной строкой timing'а — диагностический вывод анимации Дарси (`darci anim=... frame=N/M tw=...`). Полный набор debug-клавиш → [`../debug_physics_render_rate.md`](../debug_physics_render_rate.md).

## Документы

| Файл | Содержание |
|---|---|
| [`architecture.md`](architecture.md) | **Главный документ.** Как устроена система: capture + frame-scope + alpha. API, файлы, типы, формулы lerp, debug logging |
| [`coverage.md`](coverage.md) | Что интерполируется и что нет, и почему. Какие классы Things, какие DrawType, разделение «physics-rate vs wall-clock» |
| [`known_issues.md`](known_issues.md) | Текущие баги и что уже пробовали. Что не сработало и почему |
| [`plans.md`](plans.md) | Что делать дальше. Расширение покрытия, отладочные средства, открытые вопросы |

## С чего начать агенту

1. [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md) — фундаментальный принцип (физика на тиках, визуал на wall-clock, рендер ничему не служит) — обязательно
2. Этот README — статус
3. [`architecture.md`](architecture.md) — как сейчас сделано
4. [`known_issues.md`](known_issues.md) — что не работает и какие гипотезы
5. [`plans.md`](plans.md) — что планируем

## Правило обновления

**Документы в этой папке актуализируются на каждое изменение системы.** Это «живой снимок» текущего состояния, не история. Если меняется код интерполяции — синхронно обновлять `architecture.md`, `coverage.md`, и `known_issues.md`.

История изменений ведётся в git (commit log). Здесь — только то-как-сейчас.
