# Input System — переработка системы управления

Все работы по новой/унифицированной системе управления собраны здесь. Папка
переехала из `fps_unlock/` потому что задача шире чем FPS unlock — затрагивает
меню, debug-клавиши, vehicle controls и общую структуру обработки ввода.

## Что здесь

| Файл | Что внутри |
|------|-----------|
| [current_plan.md](current_plan.md) | **Текущая работа.** План модуля `input_frame` — edge-detect, sticky press, stick-as-direction, universal auto-repeat. Урезанная версия большого плана. |
| [changelog.md](changelog.md) | Журнал изменений по задаче (хронологически). При возврате к задаче через время — читать в первую очередь чтобы понять где остановились. |
| [full_plan_deferred.md](full_plan_deferred.md) | **Отложенный** полный план с actions/ремапом/контекстами/reverse-lookup. Сохранён для истории. К нему можно вернуться позже как к расширению поверх `input_frame`. |

## С чего начать новому агенту

1. Прочитать [current_plan.md](current_plan.md) — что делаем сейчас и почему.
2. Прочитать [changelog.md](changelog.md) — на каком этапе остановились.
3. Если нужен контекст FPS unlock (откуда задача растёт) — [`../fps_unlock/README.md`](../fps_unlock/README.md), [`../fps_unlock/CORE_PRINCIPLE.md`](../fps_unlock/CORE_PRINCIPLE.md).
4. Не читать `full_plan_deferred.md` пока не появится явная задача расширения с actions — он сейчас не блокирует и не описывает текущую работу.

## Связь с FPS unlock issues

Open issue'и #5/#15/#20 в [`../fps_unlock/fps_unlock_issues.md`](../fps_unlock/fps_unlock_issues.md) частично закрываются миграцией соответствующих потребителей на новый модуль (Phase 4 в [current_plan.md](current_plan.md)). Они остаются в active списке fps_unlock пока миграция не закончена и не подтверждена.
