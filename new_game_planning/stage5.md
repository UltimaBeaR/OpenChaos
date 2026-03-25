# Этап 5 — Фиксы пре-релизных багов

Пре-релизная кодовая база содержит баги которых нет в финальной игре.
Список багов → [`prerelease_fixes.md`](prerelease_fixes.md)

**Критерий:** поведение игры совпадает с финальной Steam PC версией.

## Лог

### Z-buffer mismatch — персонажи поверх стен (OC-ZBUF-MISMATCH) ✅

`DrawIndPrimMM` использовал линейное z вместо обратного → персонажи/полы через hardware path
всегда «выигрывали» z-тест у геометрии через software POLY path. Фикс — одна строка в
`polypage.cpp:615`. Детали → [`stage5_bug_zbuffer_mismatch.md`](../new_game_devlog/stage5_bug_zbuffer_mismatch.md)

### Envmap на машинах в figure.cpp — мёртвый код, не баг

При расследовании z-buffer бага обнаружен envmap блок для `CLASS_VEHICLE` в оригинале
(`figure.cpp:5130-5260`), отсутствующий в пре-релизе. Оказался мёртвым кодом:
CLASS_VEHICLE рисуется через `MESH_draw_poly()` (не figure.cpp), envmap в mesh.cpp
уже работает (стёкла, хромированные бамперы). CLASS_BIKE — байков нет ни на одном уровне.
