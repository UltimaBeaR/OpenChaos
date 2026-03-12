# MuckyBasic — Скриптовый язык Urban Chaos

## ⚠️ ВАЖНО: MuckyBasic НЕ используется в игре

**`.ucm` файлы в `fallen/Debug/levels/` — это EWAY Mission Data, НЕ MuckyBasic скрипты.**

Реальная система событий и скриптинга в игре — **EWAY (Event Waypoint System)** (`fallen/Source/eway.cpp`, 8229 строк). EWAY работает через непрерывный опрос условий каждый кадр и выполнение действий (до 512 вэйпойнтов на уровень, 41 тип условий, 57 типов действий). Подробности — в [missions.md](missions.md).

MuckyBasic — **автономный инструмент** (отдельная папка `MuckyBasic/`): компилятор BASIC-подобного языка с собственной VM. Нет ни одного механизма интеграции с игровым движком. Судя по всему, это экспериментальный проект, который так и не был встроен в игру.

**Практический вывод:** Для воспроизведения логики миссий нужно реализовывать EWAY, а не MuckyBasic.

---

**Ключевые файлы:**
- `MuckyBasic/` — весь движок скриптов
  - `lex.*` — лексер (токенизация)
  - `parse.*` — парсер (AST)
  - `cg.*` — кодогенератор (bytecode)
  - `link.*` — линкер объектных файлов
  - `vm.*` — Virtual Machine (исполнение bytecode)
  - `sysvar.h` — системные поля структур

**Скомпилированные скрипты миссий:** `fallen/Debug/levels/*.ucm` (~20+ файлов)
**Примеры исходников:** `MuckyBasic/test.mbs`, `MuckyBasic/test2.mbs` — тестовые .mbs файлы

---

## 1. Общая архитектура

**Тип:** Компилятор в bytecode + интерпретирующая VM (стек-машина)

**Конвейер:**
```
исходник.mbs  →  [Лексер]  →  Токены
              →  [Парсер]  →  AST (Abstract Syntax Tree)
              →  [CodeGen] →  исходник.mbo (объектный файл)
              →  [Линкер]  →  исполняемый.mbe
              →  [VM]      →  Исполнение
```

**Файловые расширения:**
- `.mbs` — исходный текст скрипта (в ресурсах НЕТ, только скомпилированные)
- `.mbo` — объектный файл (LINK_Header + инструкции + data table)
- `.mbe` — исполняемый файл после линкования
- `.ucm` — скомпилированные игровые скрипты (в `levels/`)

---

## 2. Типы данных

**17 типов, каждое значение = ML_Data (8 байт):**

```c
#define ML_TYPE_UNDEFINED    0   // Не инициализировано
#define ML_TYPE_SLUMBER      1   // 32-bit signed int (SLONG)
#define ML_TYPE_FLUMBER      2   // 32-bit float
#define ML_TYPE_STRCONST     3   // String константа (индекс в data table)
#define ML_TYPE_STRVAR       4   // String переменная (malloc'd)
#define ML_TYPE_BOOLEAN      5   // True/False
#define ML_TYPE_POINTER      6   // Указатель на ML_Data
#define ML_TYPE_STRUCTURE    7   // Структура (на куче)
#define ML_TYPE_ARRAY        8   // Массив (на куче, многомерный)
#define ML_TYPE_CODE_POINTER 9   // Адрес инструкции (для GOSUB/RETURN)
#define ML_TYPE_STACK_BASE  10   // Сохранённый frame pointer
#define ML_TYPE_TEXTURE     11   // LL_Texture* (графическая текстура)
#define ML_TYPE_BUFFER      12   // LL_Buffer* (вершинный буфер)
#define ML_TYPE_NUM_ARGS    13   // Число аргументов функции
#define ML_TYPE_MATRIX      14   // Матрица 3×3 (3 вектора)
#define ML_TYPE_VECTOR    15  // Вектор XYZ (3 float)
```

**Структура значения ML_Data:**
```c
typedef struct {
    SLONG type;     // Тип (ML_TYPE_*)
    union {         // Значение (8 байт)
        SLONG slumber;
        float flumber;
        CBYTE *strvar;
        ML_Data *data;           // Для указателей
        ML_Structure *structure; // Для структур
        ML_Array *array;         // Для массивов
        LL_Texture *lt;          // Для текстур
        LL_Buffer *lb;           // Для вершинных буферов
        // ... остальные типы
    };
} ML_Data;
```

**Таблица символов (Symbol Table) — 4 уровня:**
1. `ST_TABLE_SYSTEM` — встроенные поля (`.x`, `.y`, `.z`, `.rhw`, `.colour`, `.specular`, `.u`, `.v`)
2. `ST_TABLE_LIBRARY` — функции из библиотек
3. `ST_TABLE_GLOBAL` — глобальные переменные и функции
4. `ST_TABLE_LOCAL` — локальные переменные функции

---

## 3. Синтаксис языка

### Базовые конструкции

```basic
// Комментарии (как в C++)
// Это комментарий

// Присваивание
x = 42
y = 3.14
name = "hello"

// Объявление локальных переменных
LOCAL var1, var2
LOCAL my_array[10]
LOCAL matrix_2d[5][3]
```

### Управляющие конструкции

```basic
// Однострочный IF
IF (условие) THEN statement ELSE statement

// Многострочный IF
MIF (условие) THEN
    statements...
MELSE
    statements...
MENDIF

// Цикл FOR
FOR i = 1 TO 10 STEP 1
    PRINT i
NEXT i

// Цикл WHILE
WHILE (counter < 5)
    counter = counter + 1
LOOP

// Переходы
GOTO label
label: statement
GOSUB function_label
RETURN

// Выход
EXIT
```

### Функции

```basic
FUNCTION add(a, b)
    LOCAL result
    result = a + b
    RETURN result
ENDFUNC

// Вызов
sum = CALL add(10, 20)
```

### Операторы

- Арифметика: `+`, `-`, `*`, `/`, `%`
- Сравнение: `=`, `<>`, `<`, `>`, `<=`, `>=`
- Логика: `AND`, `OR`, `NOT`, `XOR`
- Структуры: `.` (поле), `[i]` (индекс массива)

### Встроенные функции

```basic
// Математика
x = SQRT(16)
x = ABS(-5)
x = SIN(0.5), COS(0.5), TAN(0.5)
x = ASIN(0.5), ACOS(0.5), ATAN(0.5), ATAN2(y, x)
x = RANDOM(max)    // случайное от 0 до max-1
ms = TIMER()       // миллисекунды

// Строки
s = LEFT(str, n)
s = MID(str, start, len)
s = RIGHT(str, n)
n = LEN(str)

// Векторы и матрицы
v = VECTOR(x, y, z)
m = MATRIX(v1, v2, v3)
d = v1.dprod(v2)       // Скалярное произведение
c = v1.cprod(v2)       // Векторное произведение
n = NORMALISE(v)       // Нормализация
t = TRANSPOSE(m)       // Транспонирование

// Константы направлений
UP, DOWN, LEFT, RIGHT, FORWARDS, BACKWARDS  // Единичные векторы

// Вывод
PRINT value1, value2   // С новой строкой
PRINT "text",          // Без новой строки

// Ввод
s = INPUT()

// Графика (MuckyBasic работает и как инструмент для демо/тестов)
tex = TEXTURE("path/to/file.bmp", UNDEFINED)
buf = BUFFER(vertex_array, n_verts, index_array, n_indices)
DRAW buf, tex, renderstate_flags
CLS color, depth
FLIP

// Клавиатура
pressed = KEY[keycode]
char = INKEY()
```

---

## 4. Virtual Machine (VM)

**Архитектура — стек-машина:**
```c
SLONG    *VM_code;          // Инструкции (массив SLONGs)
ML_Data  *VM_stack;         // Стек значений (8 байт каждое)
ML_Data  *VM_global;        // Глобальные переменные
UBYTE    *VM_data;          // Статическая data table (строки, константы)
SLONG    *VM_code_pointer;  // Указатель на текущую инструкцию
ML_Data  *VM_stack_top;     // Вершина стека
ML_Data  *VM_stack_base;    // База текущего stack frame
```

**Полный набор опкодов (81 инструкция):**

```c
// Загрузка/хранение
ML_DO_PUSH_CONSTANT       (0)   // Загрузить константу на стек
ML_DO_PUSH_GLOBAL_VALUE   (6)   // Загрузить значение глобальной переменной
ML_DO_PUSH_GLOBAL_ADDRESS (23)  // Загрузить адрес глобальной переменной
ML_DO_PUSH_LOCAL_VALUE    (43)  // Загрузить значение локальной переменной
ML_DO_PUSH_LOCAL_ADDRESS  (44)  // Загрузить адрес локальной переменной
ML_DO_PUSH_FIELD_VALUE    (26)  // Поле структуры
ML_DO_PUSH_FIELD_ADDRESS  (25)  // Адрес поля структуры
ML_DO_PUSH_ARRAY_VALUE    (28)  // Элемент массива
ML_DO_PUSH_ARRAY_ADDRESS  (27)  // Адрес элемента массива
ML_DO_ASSIGN              (24)  // *stack[-1] = stack[-2]
ML_DO_POP                 (42)  // Удалить значение

// Арифметика
ML_DO_ADD    (1),  ML_DO_MINUS (7),  ML_DO_TIMES (8)
ML_DO_DIVIDE (9),  ML_DO_MOD   (10), ML_DO_UMINUS (5)

// Сравнение
ML_DO_EQUALS  (14), ML_DO_NOTEQUAL (36)
ML_DO_GT (15), ML_DO_LT (16), ML_DO_GTEQ (17), ML_DO_LTEQ (18)

// Логика
ML_DO_AND (12), ML_DO_OR (13), ML_DO_NOT (19), ML_DO_XOR (32)

// Управление потоком
ML_DO_GOTO         (4)   // Безусловный переход
ML_DO_IF_FALSE_GOTO (11)  // Переход если FALSE
ML_DO_IF_TRUE_GOTO  (37)  // Переход если TRUE
ML_DO_GOSUB        (30)   // Вызов подпрограммы
ML_DO_RETURN       (31)   // Возврат
ML_DO_ENTERFUNC    (40)   // Вход в функцию (создать stack frame)
ML_DO_ENDFUNC      (41)   // Выход из функции
ML_DO_EXIT          (3)   // Выход из программы
ML_DO_SWAP         (39)   // Поменять местами два указателя на стеке
ML_DO_NOP          (64)   // Нет операции

// Математические функции
ML_DO_SQRT (20), ML_DO_ABS (22)
ML_DO_SIN (57), ML_DO_COS (58), ML_DO_TAN (59)
ML_DO_ASIN (60), ML_DO_ACOS (61), ML_DO_ATAN (62), ML_DO_ATAN2 (63)

// Строки
ML_DO_LEFT (65), ML_DO_MID (66), ML_DO_RIGHT (67), ML_DO_LEN (68)

// Векторы и матрицы
ML_DO_VECTOR (72), ML_DO_MATRIX (71)
ML_DO_PUSH_IDENTITY_MATRIX (69), ML_DO_PUSH_ZERO_VECTOR (70)
ML_DO_PUSH_UP (73), ML_DO_PUSH_DOWN (74), ML_DO_PUSH_LEFT (75)
ML_DO_PUSH_RIGHT (76), ML_DO_PUSH_FORWARDS (77), ML_DO_PUSH_BACKWARDS (78)
ML_DO_DOT (79), ML_DO_CROSS (80)

// I/O и графика
ML_DO_PRINT (2), ML_DO_NEWLINE (21), ML_DO_PUSH_INPUT (29)
ML_DO_TEXTURE (47), ML_DO_BUFFER (48), ML_DO_DRAW (49)
ML_DO_CLS (50), ML_DO_FLIP (51)
ML_DO_KEY_VALUE (52), ML_DO_KEY_ASSIGN (53)
ML_DO_INKEY_VALUE (54), ML_DO_INKEY_ASSIGN (55)

// Прочее
ML_DO_TIMER (56), ML_DO_PUSH_RANDOM_SLUMBER (38)
```

---

## 5. Системные поля для вершин (sysvar.h)

MuckyBasic поддерживает прямую работу с вершинами через системные поля:
```c
.x, .y, .z    // Координаты вершины (float)
.rhw          // Reciprocal Homogeneous W
.colour       // Цвет RGBA (DWORD)
.specular     // Спекулярное отражение
.u, .v        // Текстурные координаты (float)
```

Это подтверждает что MuckyBasic был инструментом для графических демо/тестов (работает с `MFLib1` типами `LL_Texture*`, `LL_Buffer*`), а не игровым скриптингом.

---

## 6. Важные выводы

**Сложность языка:** СРЕДНЯЯ
- Похож на классический BASIC с улучшениями
- Поддерживает структуры, массивы, функции, матрицы, векторы
- Динамическая типизация через ML_Data

**Для новой игры:** MuckyBasic не нужно реализовывать и не нужно переносить. Логика миссий — EWAY, см. [missions.md](missions.md).

---

## 7. Бинарные форматы файлов (справочно)

### Объектный файл .mbo

```
LINK_Header {
    SLONG version;                   // ML_VERSION_NUMBER = 1
    SLONG num_instructions;
    SLONG data_table_length_in_bytes;
    SLONG num_globals;
    SLONG num_functions;
    SLONG num_lines;
    SLONG num_jumps;
    SLONG num_fields;
    SLONG num_global_refs;
    SLONG num_undef_refs;            // ссылки на внешние функции
    SLONG num_field_refs;
    SLONG num_data_table_refs;
}
SLONG   Instructions[num_instructions]  + MAGIC(12345678)
CBYTE   DataTable[data_table_length]    + MAGIC
LINK_Global   Globals[num_globals]      + MAGIC
LINK_Function Functions[num_functions]  + MAGIC
LINK_Line     Lines[num_lines]          + MAGIC
LINK_Jump     Jumps[num_jumps]          + MAGIC
LINK_Field    Fields[num_fields]        + MAGIC
LINK_Globalref   GlobalRefs[...]        + MAGIC
LINK_Undefref    UndefRefs[...]         + MAGIC
LINK_Fieldref    FieldRefs[...]         + MAGIC
LINK_Datatableref DataTableRefs[...]    + MAGIC
SLONG   debug_data_length
CBYTE   debug_data[debug_data_length]   // содержит имена переменных/функций!
```

### Ключевые структуры:
```c
struct LINK_Global {
    UWORD index;    // номер в таблице глобальных переменных
    UBYTE export;   // TRUE = экспортируется
    UBYTE local;    // TRUE = локально в файле
    SLONG name;     // индекс в debug_data (имя переменной!)
};

struct LINK_Function {
    SLONG name;         // индекс в debug_data (имя функции!)
    SLONG export;       // TRUE = экспортируется
    SLONG line_start;
    SLONG line_end;
    SLONG num_args;
};

struct LINK_Undefref {
    SLONG name;         // имя внешней функции в debug_data
    SLONG instruction;  // инструкция GOSUB, ссылающаяся на неё
};
```

### Исполняемый файл .mbe

```
ML_Header {
    SLONG version;
    SLONG instructions_memory_in_bytes;
    SLONG data_table_length_in_bytes;
    SLONG num_globals;
}
SLONG instructions[]   // весь bytecode
CBYTE data_table[]     // строковые константы, имена
```

**Примечание:** `.ucm` файлы в `levels/` — это НЕ `.mbe`. Они загружаются EWAY-системой как EventPoint массивы, не MuckyBasic VM.

### Стек-кадр функции (VM):
```
При GOSUB → push адрес возврата (CODE_POINTER)
При ENTERFUNC(n):
    → push старый VM_stack_base (STACK_BASE)
    → VM_stack_base = текущая вершина стека
    → локальные переменные: VM_stack_base[0], [1], ...
При ENDFUNC:
    → восстановить VM_stack_base
    → pop до адреса возврата
    → jump к адресу возврата
```
