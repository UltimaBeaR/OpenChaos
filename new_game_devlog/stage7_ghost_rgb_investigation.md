# Ghost RGB Investigation — шрифты меню и HUD иконки

Дата: 2026-03-29
Обновление: 2026-04-09 — **ПОФИКШЕНО**

## Итог: корневая причина и фикс

**Баг:** `gl_bleed_edges()` в `backend_opengl/game/core.cpp` читала и писала **один массив** пикселей. Edge bleeding копирует RGB из соседних пикселей в прозрачные (alpha=0, RGB=0). При in-place модификации каждый заполненный пиксель становился "соседом" для следующего — цепная реакция заполняла весь чёрный фон текстуры паразитным RGB.

**Почему в D3D6 работало:** D3D6 бэкенд (`d3d_texture.cpp` Reload_TGA) читает из `tga[]` (source), пишет в D3D surface (destination) — два раздельных буфера. Цепной реакции нет.

**Доказательство:** BMP дамп PCdisplay01 на D3D6 ветке: 30872 пикселя с RGB=0 (чёрный фон за иконками). На OpenGL: 0 чёрных пикселей — все заполнены edge bleeding.

**Фикс:** `gl_bleed_edges()` теперь создаёт read-only копию массива (`memcpy`), ищет соседей в копии, пишет результат в оригинал. Цепная реакция устранена.

---

## Общая проблема

Текстуры из clump (A4R4G4B4, 4 бита на канал) содержат **ненулевой RGB в пикселях с alpha=0** ("ghost RGB"). При additive blending (One/One) alpha не участвует в blend equation — ghost RGB добавляется к фону и виден на экране.

**В D3D6:** ghost фон **не виден вообще**. Видна только тонкая окантовка (~1px) вокруг непрозрачных частей (букв/иконки) — примерно в половину яркости от основной картинки или меньше. Фон за буквами/иконками полностью чистый.

**В OpenGL RGBA8:** виден ВЕСЬ ghost фон — полосы, градиенты, артефакты artwork'а на полную яркость.

Код отрисовки **идентичен** между D3D6 и OpenGL (проверено diff'ом с stage_5_1_done). Разница — в GPU/runtime поведении, не в игровом коде.

**Важно про dgVoodoo:** dgVoodoo (D3D6→D3D11 wrapper) тут ни при чём. Ресурсы (текстуры в clump) взяты из оригинального релиза игры. В оригинале на реальном D3D6 железе этот баг отсутствует — шрифты и иконки рисуются чисто. Значит D3D6 hardware generic'ом обрабатывает эту ситуацию правильно. Это не "dgVoodoo как-то скрывает баг", а "оригинальная игра работала без бага, и мы должны воспроизвести это поведение".

---

## Баг 1: Шрифты меню (olyfont2.tga, POLY_PAGE_NEWFONT_INVERSE)

### Описание
Яркие прямоугольники за буквами в меню паузы и других местах где используется MENUFONT.

### Текстура
- `olyfont2.tga` (TEXTURE_page_lcdfont, page 1458)
- Загружается с `ge_texture_font2_on()` → GL_TEX_FLAG_FONT2
- Из clump: alpha=YES, 51279 пикселей alpha=0, из них 11665 с non-zero RGB (maxRGB=255/238/238)
- Буквы имеют **alpha > 0**
- Весь фон имеет **alpha = 0** (с ghost RGB)

### Рендеринг
- Poly page: POLY_PAGE_NEWFONT_INVERSE
- Blend: Modulate + One/One (additive)
- Vertex color: (fade, fade, fade, fade), обычно fade=255
- Текстура используется ТОЛЬКО этим poly page (и POLY_PAGE_NEWFONT для darkening pass)

### Что починило (костыль, но работает)
**Обнуление RGB для ВСЕХ пикселей с alpha=0 в Font2 текстурах**, после edge bleeding.

```cpp
// В gl_load_tga(), ПОСЛЕ gl_bleed_edges():
if (tex.flags & GL_TEX_FLAG_FONT2) {
    for (int32_t i = 0; i < count; i++) {
        if (pixels[i].alpha == 0) {
            pixels[i].red = 0;
            pixels[i].green = 0;
            pixels[i].blue = 0;
        }
    }
}
```

### Почему работает
У olyfont2.tga **все буквы имеют alpha > 0**, весь фон alpha = 0. Поэтому можно безопасно обнулить RGB по условию alpha=0 — это убивает ghost и не трогает контент.

### Побочные эффекты
Также обнуляет RGB от edge bleeding (который копирует соседний RGB в прозрачные пиксели для билинейной фильтрации). Это убирает 1px edge bleeding halo, но для additive blend это правильно — edge bleeding нужен для SrcAlpha/InvSrcAlpha, не для One/One.

### Почему костыль
Это не то что делает D3D6. D3D6 как-то generic'ом подавляет ghost при том же самом коде и данных. Фикс обнулением RGB — это очистка данных текстуры, а не воспроизведение поведения D3D6 GPU.

---

## Баг 2: HUD иконка кулака (PCdisplay01.tga, POLY_PAGE_LASTPANEL2_ADD)

### Описание
Видимый фон (горизонтальные полосы, градиент) вокруг и за иконкой кулака в HUD.

### Текстура
- `PCdisplay01.tga` (TEXTURE_page_lastpanel2, page 1477)
- Из clump: alpha=YES, 61404 пикселя alpha=0 (из них 30532 с non-zero RGB, maxRGB=238/255/221), 3114 alpha=255, 1018 промежуточных
- Текстура — **атлас** с двумя разными областями:
  - **Верхняя половина** (y<120): HUD дуги HP — alpha>0 для формы дуг, alpha=0 для фона (с ghost — радужный градиент). Рендерится через POLY_PAGE_LASTPANEL2_ALPHA (ModulateAlpha + SrcAlpha/InvSrcAlpha)
  - **Нижняя половина** (y>=120): иконки оружия (бита, взрывчатка, кулак) — **ВСЁ alpha=0**, иконки хранятся **только в RGB**. Рендерится через POLY_PAGE_LASTPANEL2_ADD (Modulate + One/One)
- Область кулака: UV (126,124)-(162,168). В регионе (110,110)-(180,180): alpha>0 всего 111 пикселей, alpha=0 с RGB=0 — НОЛЬ, alpha=0 dim(<32) — 1929, alpha=0 bright(>=32) — 2860

### Рендеринг
- BodgePageIntoAdd(POLY_PAGE_LASTPANEL2_ALPHA) → POLY_PAGE_LASTPANEL2_ADD
- Blend: Modulate + One/One (additive)
- Vertex color: 0xFFffffff

### Почему НЕЛЬЗЯ починить как шрифт
Иконка кулака сама имеет **alpha=0** (изображение ТОЛЬКО в RGB). Обнуление RGB по условию alpha=0 убивает и ghost, и иконку — нельзя отличить по alpha.

### Что починило (ТРИ фикса вместе, но ломает другой UI)

**FIX 1 — alpha=255 для non-alpha текстур:**
```cpp
if (!load_alpha) {
    for (int32_t i = 0; i < count; i++) {
        pixels[i].alpha = 255;
    }
}
```
Зачем: D3D6 возвращает alpha=1.0 для текстур без alpha-канала. Без этого non-alpha текстуры имеют alpha=0 из clump (5:6:5), что ломает Modulate ALPHAOP=SELECTARG1.

**FIX 2 — реконструкция alpha из max(RGB) для alpha текстур:**
```cpp
if (load_alpha && !(tex.flags & GL_TEX_FLAG_FONT2)) {
    for (int32_t i = 0; i < count; i++) {
        if (pixels[i].alpha == 0 && (pixels[i].red | pixels[i].green | pixels[i].blue)) {
            uint8_t mx = pixels[i].red;
            if (pixels[i].green > mx) mx = pixels[i].green;
            if (pixels[i].blue > mx) mx = pixels[i].blue;
            pixels[i].alpha = mx;
        }
    }
}
```
Зачем: даёт пикселям с alpha=0 реконструированный alpha на основе яркости RGB. Яркие иконки → high alpha, тусклый ghost → low alpha.

**FIX 3 — SrcAlpha/One для LASTPANEL_ADD и LASTPANEL2_ADD:**
```cpp
// В poly_render.cpp, case POLY_PAGE_LASTPANEL2_ADD:
pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);  // было One
pa->RS.SetDstBlend(GEBlendFactor::One);        // было One
```
Зачем: additive с alpha-аттенуацией. Ghost с low reconstructed alpha → малый вклад. Иконка с high reconstructed alpha → яркий вклад.

### Результат трёх фиксов
- Кулак: виден ✓
- Шрифты: OK ✓
- Другой UI: **СЛОМАН** ✗

### Почему ломает другой UI
FIX 2 (reconstruction) применяется ко ВСЕМ alpha текстурам. Текстура PCdisplay01.tga — атлас:
- **Верхняя часть** (HUD дуги) рендерится через POLY_PAGE_LASTPANEL2_ALPHA (SrcAlpha/InvSrcAlpha). Ghost в верхней части (радужный градиент) получает reconstruction alpha → видна радуга через alpha blend.
- **multifontPC.tga** (шрифт FONT2D на карте миссий) тоже получает reconstruction alpha → шрифт глючит.

### Возможное направление для правильного фикса
Reconstruction нужна ТОЛЬКО для нижней половины PCdisplay текстур (иконки, y>=120), но НЕ для верхней (HUD дуги) и НЕ для других текстур. Но это текстуро-специфичный хак.

Правильное решение — понять что делает D3D6 GPU generic'ом. Код игры идентичен. Данные текстур идентичны (из того же clump). Документация D3D6 по D3DTBLEND_MODULATE описывает только texture stage states (COLOROP=MODULATE, ALPHAOP=SELECTARG1/SELECTARG2) — это уже реализовано и не влияет на One/One blend.

---

## Документация D3D6: маппинг D3DTBLEND → texture stage states

Из DirectX 6 SDK (дословно):

```
D3DTBLEND_MODULATE:
  SetTextureStageState(0, COLOROP,   D3DTOP_MODULATE);
  SetTextureStageState(0, COLORARG1, D3DTA_TEXTURE);
  SetTextureStageState(0, COLORARG2, D3DTA_DIFFUSE);

  if (the_texture_has_an_alpha_channel) {
      SetTextureStageState(0, ALPHAOP,   D3DTOP_SELECTARG1);
      SetTextureStageState(0, ALPHAARG1, D3DTA_TEXTURE);
  } else {
      SetTextureStageState(0, ALPHAOP,   D3DTOP_SELECTARG2);
      SetTextureStageState(0, ALPHAARG2, D3DTA_DIFFUSE);
  }

D3DTBLEND_MODULATEALPHA:
  SetTextureStageState(0, COLOROP,   D3DTOP_MODULATE);
  SetTextureStageState(0, COLORARG1, D3DTA_TEXTURE);
  SetTextureStageState(0, COLORARG2, D3DTA_DIFFUSE);

  SetTextureStageState(0, ALPHAOP,   D3DTOP_MODULATE);
  SetTextureStageState(0, ALPHAARG1, D3DTA_TEXTURE);
  SetTextureStageState(0, ALPHAARG2, D3DTA_DIFFUSE);
```

Значение operations:
- `D3DTOP_MODULATE`: result = arg1 × arg2
- `D3DTOP_SELECTARG1`: result = arg1
- `D3DTOP_SELECTARG2`: result = arg2
- `D3DTA_TEXTURE`: текстурный сэмпл
- `D3DTA_DIFFUSE`: интерполированный вершинный цвет

Подтверждено Wine реализацией: [wine/dlls/ddraw/device.c](https://github.com/wine-mirror/wine/blob/master/dlls/ddraw/device.c) — функция `fixup_texture_alpha_op()` переключает ALPHAOP при смене текстуры.

### Наша OpenGL реализация (в common_frag.glsl)

```glsl
// D3D6 hardware returns alpha=1.0 for textures without alpha channel.
// Clump 5:6:5 textures have alpha=0 in data — эмулируем в шейдере при сэмплировании:
float tex_alpha = (u_tex_has_alpha != 0) ? tex.a : 1.0;

// Modulate:
color.rgb = color.rgb * tex.rgb;                              // COLOROP=MODULATE ✓
color.a = (u_tex_has_alpha != 0) ? tex_alpha : color.a;      // ALPHAOP: SELECTARG1 или SELECTARG2 ✓

// ModulateAlpha:
color.rgb = color.rgb * tex.rgb;                              // COLOROP=MODULATE ✓
color.a = color.a * tex_alpha;                                // ALPHAOP=MODULATE ✓
```

Uniform `u_tex_has_alpha` передаётся из бэкенда — определяется при `ge_bind_texture()` через поиск `contains_alpha` по gl_id текстуры. Функция `ge_bound_texture_contains_alpha()` добавлена в контракт (все 3 бэкенда).

**Вопрос:** реализация texture stage states 1:1 по документации. Но ghost всё равно виден. Texture stage output идёт в framebuffer blend. При One/One additive blend alpha от texture stage **не влияет** на blend factors. Значит если D3D6 подавляет ghost — это происходит НЕ через texture stage states и НЕ через SRCBLEND/DSTBLEND. Где-то есть ещё механизм.

---

## Полезные данные для дальнейшего исследования

### Текстуры с ghost RGB

Получить дамп: скилл `.claude/skills/dump-texture/SKILL.md` (debug stats или полный PNG дамп).

Ключевые текстуры для исследования:

| Текстура | Как найти page ID | Описание |
|----------|-------------------|----------|
| olyfont2.tga | `TEXTURE_page_lcdfont` в texture.cpp (= TEXTURE_NUM_STANDARD + 50) | Font2 шрифт меню, POLY_PAGE_NEWFONT_INVERSE |
| multifontPC.tga | `TEXTURE_page_font2d` в texture.cpp (= TEXTURE_NUM_STANDARD + 17) | FONT2D шрифт, POLY_PAGE_FONT2D |
| PCdisplay.tga | `TEXTURE_page_lastpanel` в texture.cpp (= TEXTURE_NUM_STANDARD + 65) | HUD панель фон |
| PCdisplay01.tga | `TEXTURE_page_lastpanel2` в texture.cpp (= TEXTURE_NUM_STANDARD + 69) | HUD атлас: дуги HP (верх) + иконки оружия (низ) |

TEXTURE_NUM_STANDARD = 22 × 64 = 1408 (из `texture.cpp`, `TEXTURE_NUM_STANDARD` = количество стандартных мировых текстур).

### Структура атласа PCdisplay01.tga (256×256)
- **Верхняя половина** (y < ~120): HUD дуги HP/армора. Alpha > 0 для формы дуг, alpha = 0 для фона (ghost = яркий радиальный градиент). Рендерится через POLY_PAGE_LASTPANEL2_ALPHA (ModulateAlpha + SrcAlpha/InvSrcAlpha).
- **Нижняя половина** (y >= ~120): иконки оружия (бита, взрывчатка, кулак). **ВСЁ alpha = 0**, иконки хранятся **только в RGB**. Рендерится через POLY_PAGE_LASTPANEL2_ADD (Modulate + One/One additive, через BodgePageIntoAdd).

### Wine реализация D3DTBLEND_MODULATE
Подтверждает документацию: COLOROP=MODULATE, ALPHAOP=SELECTARG1(TEX) или SELECTARG2(DIFFUSE). Функция `fixup_texture_alpha_op()` меняет ALPHAOP при смене текстуры. [wine/dlls/ddraw/device.c](https://github.com/wine-mirror/wine/blob/master/dlls/ddraw/device.c)

### PSX реализация
Полностью другая: CLUT-палитры, blend mode 0 (50%+50%), POLY_FT4 примитивы. Нет ghost в принципе — прозрачность через palette index. Файл: `original_game/fallen/PSXENG/SOURCE/panel.cpp`.
