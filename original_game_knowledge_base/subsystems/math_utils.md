# Математика и утилиты — Urban Chaos

**Ключевые файлы:**
- `fallen/DDEngine/Headers/Matrix.h` — матрицы 3x3, макросы
- `fallen/DDEngine/Headers/Quaternion.h` — кватернионы
- `fallen/DDEngine/Source/Matrix.cpp` — реализация матриц
- `fallen/DDEngine/Source/Quaternion.cpp` — реализация кватернионов
- `fallen/DDEngine/Source/Maths.cpp` — lookup tables (arctan, sqrt)
- `MuckyBasic/ml.h` — Vector3 и Matrix типы для VM

---

## 1. Матрицы (Matrix.h / Matrix.cpp)

**Хранение: 3×3, row-major, 9 float:**
```
matrix[0] matrix[1] matrix[2]
matrix[3] matrix[4] matrix[5]
matrix[6] matrix[7] matrix[8]
```

**Макросы:**
```c
MATRIX_MUL(m, x, y, z)               // умножение вектора (x,y,z) на матрицу m
MATRIX_MUL_BY_TRANSPOSE(m, x, y, z)  // умножение на транспонированную
MATRIX_TRANSPOSE(m)                   // транспонирование in-place (swap элементов)
```

**Функции:**
```c
MATRIX_calc(matrix, yaw, pitch, roll)       // Euler → Matrix3x3
MATRIX_vector(vector, yaw, pitch)           // Euler → направляющий вектор
MATRIX_skew(matrix, skew, zoom, scale)      // применить aspect ratio и zoom
MATRIX_3x3mul(a, m, n)                      // a = m * n
MATRIX_rotate_about_its_*(matrix, angle)    // ротация вокруг собственной оси
MATRIX_find_angles(matrix) → Direction      // Matrix → (yaw, pitch, roll)
```

**Замечания:**
- Все углы в радианах
- На PC: `sin()` / `cos()` из `<math.h>`
- На Dreamcast: быстрые intrinsics `_SinCosA()` (точность до 2e-21)
- `MATRIX_find_angles()` обрабатывает gimbal lock (pitch > π/4)

---

## 2. Кватернионы (Quaternion.h / Quaternion.cpp)

**Структура:**
```c
struct CQuaternion {
    float w, x, y, z;
};
```

**Функции:**
```c
QuatSlerp(q1, q2, t)     // SLERP интерполяция
QuatMul(q1, q2)           // умножение кватернионов
EulerToQuat(yaw, pitch, roll) → CQuaternion
QuatToMatrix(q) → FloatMatrix   // Quaternion → Matrix3x3
MatrixToQuat(m) → CQuaternion   // Matrix3x3 → Quaternion
```

**SLERP реализация:**
```c
// Если кватернионы близки (dot > 0.95) — линейная интерполяция
// Иначе стандартный SLERP:
scale0 = sin((1-t) * omega) / sin(omega)
scale1 = sin(t * omega) / sin(omega)
```

**Проверки:**
```c
is_unit(matrix)          // каждая строка должна быть unit-вектором
check_isonormal(matrix)  // проверка ортогональности и handedness
// Допуск: 0.03
```

**Integer версия (для PSX):**
- `MatrixToQuatInteger()` / `QuatToMatrixInteger()` — fixed-point 15.15
- `QuatSlerpInteger()` — SLERP с таблицей `acos_table[1025]`
- `cmat_to_mat()` — распаковка сжатой матрицы CMatrix33

**Сжатая матрица (CMatrix33 для PSX):**
- 3 SLONG с битовыми полями: `CMAT0_MASK`, `CMAT1_MASK`, `CMAT2_MASK`
- Распаковка: `((value & mask) >> shift) / 512.f`
- Упаковка: `float * 32768.f` → SLONG (fixed-point 16.15)

---

## 3. Lookup таблицы (Maths.cpp)

**Arctan:**
```c
AtanTable[]            // таблица для быстрого arctangent
Arctan(X, Y)           // ≈ atan2(Y, X), через таблицу + bit-shift + XOR-swap
```

**Fast integer sqrt:**
```c
Root(square)           // быстрый целочисленный sqrt
// Алгоритм: Newton-Raphson + инициализация из таблицы ini_table[32]
// Используется в физике и quaternion операциях
```

**Таблиц sin/cos нет** — на PC используется стандартная `libm`.

---

## 4. Fixed-point математика

| Формат | Масштаб | Где используется |
|--------|---------|-----------------|
| 15.15 | `1 << 15` = 32768 | Матрицы ротации (PSX) |
| 16.16 | `1 << 16` = 65536 | Координаты (SLONG GameCoord) |
| Сдвиги | `>>15` или `>>16` | Делення в fixed-point арифметике |

---

## 5. Vector3 (из MuckyBasic ml.h)

```c
struct ml_vector {
    float x, y, z;
};
```

Используется и в VM, и в игровом коде. Операции (через опкоды VM):
- `DOT` — скалярное произведение
- `CROSS` — векторное произведение
- `NORMALISE` — нормализация
- `TRANSPOSE` — транспонирование матрицы

---

## 6. Что переносить в новую версию

| Компонент | Подход |
|-----------|--------|
| Matrix3x3 | Использовать **glm::mat3** (GLM библиотека) |
| Quaternion | Использовать **glm::quat** |
| Vector3 | Использовать **glm::vec3** |
| SLERP | Встроен в glm (`glm::slerp`) |
| Arctan/sqrt lookup | Не нужны — современный CPU достаточно быстр |
| Fixed-point 16.16 | Сохранить для координат физики (важно для 1:1 воспроизведения) |
| CMatrix33 (PSX сжатая) | Не переносить — только для PSX |
| Функции Euler↔Matrix↔Quat | Через GLM |
