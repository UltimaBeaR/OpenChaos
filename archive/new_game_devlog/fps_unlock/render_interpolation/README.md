# Render Interpolation — рабочая папка

Подзадача в рамках общей задачи [FPS unlock](../README.md). Цель — плавная отрисовка игрового мира на render-rate выше physics-rate за счёт интерполяции между двумя последними physics-снапшотами.

> **Базовые принципы и контекст всей задачи** → [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md) (читать первым).
> **Технический документ родительской задачи** → [`../fps_unlock.md`](../fps_unlock.md).

## Статус

**Готово к 1.0.** Полный комплект интерполяции:
- Тело персонажа (Дарси, NPC, bat'ы / Bane / Balrog / Gargoyle) — через **per-bone world-space pose snapshot** (поза captured per-tick → render лерпит world transform каждой кости). Архитектурно решает cancel-out cases (лестница, прыжки) и anim transitions автоматически. См. [`world_pose_snapshot_plan.md`](world_pose_snapshot_plan.md) — выполненный план миграции.
- Позиция Things (`Thing.WorldPos`) + углы машин (`Genus.Vehicle->Angle/Tilt/Roll`) + камеры (FC + EWAY катсценная) + DIRT pool (листья / гильзы / etc).

Остаточные баги: мельтешащие педестрианы (#1) — spawn-stale, не связан с pose path; #2a комбо-input регресс — не render-interp баг (gameplay/state-handler); #3 партиклы (низкий приоритет); #4 short-path lerp при быстром вращении (теоретический). См. [`known_issues.md`](known_issues.md).

Runtime toggle клавишей **3** (`g_render_interp_enabled`). Состояние видно в overlay как `IP: on/off`. Под основной строкой timing'а — диагностический вывод анимации Дарси (`darci anim=... frame=N/M tw=...`). Полный набор debug-клавиш → [`../debug_physics_render_rate.md`](../debug_physics_render_rate.md).

## Документы

| Файл | Содержание |
|---|---|
| [`architecture.md`](architecture.md) | **Главный документ.** Как устроена система: capture + frame-scope + alpha + pose snapshot. API, файлы, типы, формулы lerp, debug logging |
| [`coverage.md`](coverage.md) | Что интерполируется и что нет, и почему. Какие классы Things, какие DrawType, разделение «physics-rate vs wall-clock» |
| [`known_issues.md`](known_issues.md) | Активные баги + known limitations |
| [`known_issues_resolved.md`](known_issues_resolved.md) | Закрытые баги (исторический архив) |
| [`plans.md`](plans.md) | Что делать дальше. Расширение покрытия, отладочные средства, открытые вопросы |
| [`world_pose_snapshot_plan.md`](world_pose_snapshot_plan.md) | **Исторический документ.** Архитектурный план миграции с substitution на pose snapshot — все фазы выполнены 2026-05-03 |

## С чего начать агенту

1. [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md) — фундаментальный принцип (физика на тиках, визуал на wall-clock, рендер ничему не служит) — обязательно
2. Этот README — статус
3. [`architecture.md`](architecture.md) — как сейчас сделано
4. [`known_issues.md`](known_issues.md) — что не работает и какие гипотезы
5. [`plans.md`](plans.md) — что планируем

## Правило обновления

**Документы в этой папке актуализируются на каждое изменение системы.** Это «живой снимок» текущего состояния, не история. Если меняется код интерполяции — синхронно обновлять `architecture.md`, `coverage.md`, и `known_issues.md`.

История изменений ведётся в git (commit log). Здесь — только то-как-сейчас.
