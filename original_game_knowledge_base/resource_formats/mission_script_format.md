# Формат Mission Script (.sty) — Urban Chaos

**Файл:** `data/urban.sty` (английская версия)
Для других языков: `data/<Language>.sty` (French.sty, Deutsch.sty, Italian.sty, spanish.sty)
Максимальный размер: 20KB (лимит буфера `loaded_in_script[SCRIPT_MEMORY]`)

---

## Общая структура

```
// Comment lines (ignored)
// Version N
//
// Object ID : Group ID : ... (column header comment)
//
<mission entry>
<mission entry>
...
//
[districts]
<district entry>
<district entry>
...
```

Файл читается текстово (`"rt"`), построчно через `LoadStringScript()`.
Строки разделены `\n` (10); CRLF — оба символа присутствуют, trailing CR=13 в briefing обрезается.

---

## Секция миссий (Version 4 — текущая)

Каждая строка — одна миссия. Разделитель: ` : ` (пробел-двоеточие-пробел).

```
ObjID : GroupID : ParentID : ParentIsGroup : Type : Flags : District : Filename : Title : Briefing
```

### Поля

| Поле | Тип | Описание |
|------|-----|----------|
| `ObjID` | int | Уникальный ID миссии; индекс в `mission_hierarchy[]` (0-based) |
| `GroupID` | int | Группа для совместных условий разблокировки (сейчас не используется?) |
| `ParentID` | int | ObjID родительской миссии — должна быть завершена для разблокировки |
| `ParentIsGroup` | int | 0=Parent это ObjID; 1=Parent это GroupID |
| `Type` | int | Тип миссии (1=обычная; другие значения не исследованы) |
| `Flags` | int | Флаги (в текущих данных всегда 0) |
| `District` | int | Номер района (0-based); ObjID 0→25 для 27 районов |
| `Filename` | str | Имя .ucm файла (без пути, с расширением) |
| `Title` | str | Название миссии (до `:`) |
| `Briefing` | str | Текст задания (до конца строки); `@` → `\r` (перенос строки) |

**Пример строки (Version 4):**
```
6 : 0 : 3 : 0 : 1 : 0 : 4 : police1.ucm : RTA : Okay Stern, I guess you're ready...
```

---

## Исторические версии формата

Парсер (`FRONTEND_ParseMissionData`) поддерживает несколько версий:

| Версия | Формат | Отличие от v4 |
|--------|--------|---------------|
| `default` | `ObjID:GroupID:ParentID:ParentIsGroup:Type:fn:title:brief` | нет Flags, нет District |
| `v2` | `ObjID:GroupID:ParentID:ParentIsGroup:Type:fn:*n:*n:title:brief` | нет Flags/District; два пропущенных числа (`*n:*n`) |
| `v3` | `ObjID:GroupID:ParentID:ParentIsGroup:Type:District:fn:title:brief` | нет Flags |
| `v4` | полный формат с Flags и District | текущий |

Версия читается из строки `// Version N` в заголовке файла.
Строки комментариев (начинающиеся с `//`) при парсинге проверяются на `strstr(text,"Version")`.

---

## Секция [districts]

После строки `[districts]` следует список районов. Индекс = номер в списке (0-based).

```
[districts]
=0,0                       // район 0 — нет имени, нет позиции на карте
=0,0                       // район 1 — аналогично
Baseball Ground=420,163    // район 2 — название и координаты (X,Y) на карте
```

**Формат:** `Name=X,Y` или `=0,0` (безымянный).
Координаты — пиксельные координаты точки на карте выбора районов (640×480).

### Полный список районов из urban.sty

```
 0: =0,0
 1: =0,0
 2: Baseball Ground      (420,163)
 3: Botanical Gardens    (267,181)
 4: West District        (195,160)
 5: Southside Beach      (333,376)
 6: Rio canal            (327,293)
 7: =0,0
 8: =0,0
 9: Central Park         (208,203)
10: =0,0
11: =0,0
12: Snow Plains          (225,368)
13: Gangland             (472,385)
14: Skyline              (276,275)
15: Academy              (523,337)
16: Banes Estate         (244,97)
17: =0,0
18: Medical Centre       (562,160)
19: El Cossa Island      (603,260)
20: Test track           (381,137)
21: Television Centre    (564,299)
22: Assault Course       (378,103)
23: Clandon Heights      (347,201)
24: Combat Centre        (465,134)
25: Oval Circuit         (601,207)
26: Advanced Circuit     (380,132)
27: West Block           (203,264)
```

---

## suggest_order[] — рекомендуемый порядок прохождения

**Захардкожен** в `frontend.cpp` (строки 224-267), не читается из файла.
Массив указателей на строки (имена .ucm), заканчивается sentinel-строкой `"!"`.
Используется в `FRONTEND_MissionHierarchy()` для выбора текущей рекомендованной миссии.

```c
CBYTE *suggest_order[] = {
    "testdrive1a.ucm",  // 0 — первая рекомендованная
    "FTutor1.ucm",
    "Assault1.ucm",
    "police1.ucm",
    "police2.ucm",
    "Bankbomb1.ucm",
    "police3.ucm",
    "Gangorder2.ucm",
    "police4.ucm",
    "bball2.ucm",
    "wstores1.ucm",
    "e3.ucm",
    "westcrime1.ucm",
    "carbomb1.ucm",
    "botanicc.ucm",
    "Semtex.ucm",
    "AWOL1.ucm",
    "mission2.ucm",
    "park2.ucm",
    "MIB.ucm",
    "skymiss2.ucm",
    "factory1.ucm",
    "estate2.ucm",
    "Stealtst1.ucm",
    "snow2.ucm",
    "Gordout1.ucm",
    "Baalrog3.ucm",
    "Finale1.ucm",       // основной сюжет до сюда

    "Gangorder1.ucm",    // бонусные/альтернативные
    "FreeCD1.ucm",
    "jung3.ucm",

    "fight1.ucm",        // арены боя
    "fight2.ucm",
    "testdrive2.ucm",
    "testdrive3.ucm",

    "Album1.ucm",        // скрытая

    "!"                  // sentinel
};
```

**Алгоритм выбора:** для каждой доступной (flag&4) незавершённой (!(flag&2)) миссии находится минимальный индекс в `suggest_order[]`. Эта миссия становится `district_flash`/`selected`.
⚠️ Миссии не в списке (`suggest_order[order][0]=='!'` до нахождения) пропускаются при автовыборе.

---

## Загрузка и использование

```c
// Формирование имени файла (frontend.cpp ~4180)
strcpy(MISSION_SCRIPT, "data\\");
strcat(MISSION_SCRIPT, lang);   // "urban" для английского, иначе язык из XLAT
strcat(MISSION_SCRIPT, ".sty");

// Кэширование в память (один раз при запуске)
CacheScriptInMemory(MISSION_SCRIPT);  // читает всё в loaded_in_script[20KB]

// Повторные чтения — из памяти (LoadStringScript)
FileOpenScript();               // loaded_in_script_read_upto = loaded_in_script
LoadStringScript(buf);          // читает до \n, NULL-terminates
FileCloseScript();              // loaded_in_script_read_upto = NULL
```

**Демо-версия:** `demo1.sty` — отдельный файл с одной/несколькими миссиями.
**Закомментированная строка:** был вариант `data\urban.sty` захардкоженным — заменён на динамический.
