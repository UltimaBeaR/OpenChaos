# 129 — Sprint does not consume stamina (the user's original requests)

Task: [../tasks/129-sprint-does-not-consume-stamina.md](../tasks/129-sprint-does-not-consume-stamina.md)

## Initial request (2026-11-04)
> Я не помню чтобы это было сознательным решением - убирать потребление стамины. Можешь проанализировать код и правки - почему этот баг появился?

## Refinement (2026-11-04, after the analysis of the reporter's proposed fix)
> Давай фиксить и давай сделаем что если перс на месте стоит - то восстанавливается. это же по сути баг оригинала был?

## Refinement (2026-11-04, after noticing no recovery while running with the button held; replaces the standing-still exception)
> Давай полностью оригинальное поведение возвращать
