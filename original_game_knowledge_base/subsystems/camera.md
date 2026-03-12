# Камера — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/fc.cpp` — "The final camera?", основная логика камеры
- `fallen/DDEngine/cam.cpp` — низкоуровневая камера рендерера

---

## 1. Структура FC_Cam

```c
// fc.cpp
struct FC_Cam {
    // позиция, высота, дистанция, yaw, pitch
};

#define FC_MAX_CAMS  [несколько камер]
FC_Cam FC_cam[FC_MAX_CAMS];
```

---

## 2. Режимы камеры

### Стандартный (от третьего лица)
- Следует за игроком с обходом коллизий
- Raycasting от игрока к позиции камеры — при пересечении стены придвигается

### Режим с оружием (gun-out)
```c
// fc.cpp — #ifndef MARKS_PRIVATE_VERSION
// При GUN_OUT состоянии персонажа:
*dheight = 0;     // камера не поднимается над обычной высотой
*ddist   = 200;   // дополнительная дистанция отдаления
```
Управляется флагом компиляции `MARKS_PRIVATE_VERSION`:
- Если определён — режим gun-out камеры отключён
- В финальной версии — **активен** (MARKS_PRIVATE_VERSION не определён)

### В автомобиле
Дистанция и высота камеры зависят от типа транспорта (разные значения для Van, Car, Police и т.д.).

---

## 3. Параметры рендерера камеры

```c
// cam.cpp / rendering.md
POLY_camera_set(x, y, z, yaw, pitch, roll, view_dist, lens):
// lens ~ 1.5 (zoom фактор, чем выше — уже FOV)
// view_dist ~ 6000 units (из config.ini: draw_distance=22)
```

**Splitscreen:**
- `POLY_SPLITSCREEN_NONE`, `TOP`, `BOTTOM`
- Поддержка разделённого экрана (для кооп режима, но кооп не реализован)

---

## 4. Что переносить

- Стандартную камеру от третьего лица с raycasting collision avoidance — 1:1
- Режим gun-out (смещение дистанции +200) — 1:1
- Per-vehicle-type параметры камеры — 1:1
- Splitscreen — не нужен (кооп не реализован)
