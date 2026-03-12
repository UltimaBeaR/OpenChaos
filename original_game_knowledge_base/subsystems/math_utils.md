# Математика и утилиты — Urban Chaos

**Ключевые файлы:**
- `fallen/Headers/FMatrix.h` — матрицы 3x3 для PSX (целочисленные углы 0-2047)
- `fallen/Source/FMatrix.cpp` — реализация PSX матриц
- `fallen/DDEngine/Headers/Matrix.h` — матрицы 3x3 для PC (float, радианы)
- `fallen/DDEngine/Source/Matrix.cpp` — реализация PC матриц
- `fallen/DDEngine/Headers/Quaternion.h` — кватернионы (float)
- `fallen/DDEngine/Source/Quaternion.cpp` — реализация кватернионов
- `MFStdLib/Source/StdLib/StdMaths.cpp` — SinTable, CosTable, AtanTable, Root()

---

## ВАЖНО: Два независимых математических стека

| Система | Файлы | Углы | Арифметика | Платформа |
|---------|-------|------|------------|-----------|
| PSX | FMatrix.h + StdMaths | **0–2047** за полный оборот | Целочисленная, lookup tables | PSX |
| PC/DDEngine | Matrix.h + libm | **Радианы** (float) | IEEE 754 float | PC, Dreamcast |

---

## 1. PSX система (FMatrix.h / StdMaths)

### Углы: 0–2047 за полный оборот

```c
// FMatrix.h строка 9: "0 to 2047 for a full rotation"
// Пример использования (FMatrix.cpp строки 13-21):
cy = COS(yaw   & 2047);
cp = COS(pitch & 2047);
cr = COS(roll  & 2047);
sy = SIN(yaw   & 2047);
```

### Таблицы sin/cos (StdMaths.cpp)

```c
// StdMaths.h строки 9-10:
#define SIN(a)  SinTable[a]
#define COS(a)  CosTable[a]

// StdMaths.cpp:
SLONG SinTable[];             // 2560 entries (НЕ 2048!) — индексы 0-2559
SLONG *CosTable = &SinTable[512];  // COS = SIN со сдвигом 512 (π/2)
// Почему 2560: CosTable нужны индексы 0..2047 → SinTable[512..2559] → 512+2048=2560
```

Значения масштабированы: `65536 = 1.0` (16.16 fixed-point, используется `>>16`).
Также есть `SinTableF[512]` (float, только 0°–90°, т.е. 0..511) и `CosTableF` (указатель на SinTableF — **осторожно: обращение к индексу 0 корректно, но CosTableF как смещение не определён формально**).
В maths.cpp: `sinx = SIN(xangle & (2048-1)) >> 1` → точность 15 бит (×32768).

### FMATRIX_calc (FMatrix.cpp)

```c
// FMatrix.h строка 17:
void FMATRIX_calc(SLONG matrix[9], SLONG yaw, SLONG pitch, SLONG roll);
// matrix[9] — row-major 3x3, значения в fixed-point ×65536
```

### FMATRIX_find_angles (FMatrix.cpp строки 309-395)

```c
void FMATRIX_find_angles(SLONG *matrix, SLONG *yaw, SLONG *pitch, SLONG *roll);
// Возвращает углы в диапазоне 0-2047
// *yaw   = 1024 - Arctan(x, z);
// *pitch = 1024 - Arctan(y, xz);
// *roll  = 1024 - Arctan(sin_roll, cos_roll);
```

### Arctan (StdMaths.cpp строки 15-88)

```c
SLONG Arctan(SLONG X, SLONG Y);
// Возвращает значение 0-2047 (не радианы!)
// Использует AtanTable[256] — таблица arctangent 0-90° в масштабе 0-2047
// Определяет квадрант вручную, добавляет 512/1024/1536 смещение
```

**AtanTable[]** (StdMaths.cpp строки 99-150):
```c
SWORD AtanTable[] = {
    2048L*0/131072L,      // 0° = 0
    2048L*81/131072L,     // ~0.06°
    // ... 256 entries, покрывает 0-90° в масштабе 0-512
};
```

---

## 2. PC/DDEngine система (Matrix.h / Matrix.cpp)

### Углы: радианы (float)

```c
// Matrix.cpp строки 35-41:
sy = sin(yaw);    // libm
sp = sin(pitch);
sr = sin(roll);
cy = cos(yaw);
cp = cos(pitch);
cr = cos(roll);
```

### Хранение матрицы: 3×3, row-major, 9 float

```
matrix[0] matrix[1] matrix[2]
matrix[3] matrix[4] matrix[5]
matrix[6] matrix[7] matrix[8]
```

### Макросы (Matrix.h)

```c
MATRIX_MUL(m, x, y, z)               // умножение вектора (x,y,z) на матрицу m
MATRIX_MUL_BY_TRANSPOSE(m, x, y, z)  // умножение на транспонированную
MATRIX_TRANSPOSE(m)                   // транспонирование in-place (SWAP_FL)
```

### Функции (Matrix.h / Matrix.cpp)

```c
void MATRIX_calc(float matrix[9], float yaw, float pitch, float roll);
void MATRIX_vector(float vector[3], float yaw, float pitch);
void MATRIX_skew(float matrix[9], float skew, float zoom, float scale);
void MATRIX_3x3mul(float a[9], float m[9], float n[9]);   // a = m * n
void MATRIX_rotate_about_its_x(float matrix[9], float angle);
void MATRIX_rotate_about_its_y(float matrix[9], float angle);
void MATRIX_rotate_about_its_z(float matrix[9], float angle);
Direction MATRIX_find_angles(float matrix[9]);             // Matrix → (yaw, pitch, roll)
```

### Direction struct (Matrix.h строки 127-133)

```c
typedef struct {
    float yaw;    // в радианах
    float pitch;  // в радианах
    float roll;   // в радианах
} Direction;
```

`MATRIX_find_angles()` реализация (Matrix.cpp строки 367-422):
```c
ans.pitch = (float)asin(matrix[7]);
ans.yaw   = (float)atan2(matrix[6], matrix[8]);
ans.roll  = (float)atan2(sin_roll, cos_roll);
```
Обрабатывает gimbal lock (pitch > π/4).

---

## 3. Кватернионы (Quaternion.h / Quaternion.cpp) — только PC

```c
struct CQuaternion {
    float w, x, y, z;
};
```

### Функции

```c
QuatSlerp(q1, q2, t)            // SLERP интерполяция
QuatMul(q1, q2)                  // умножение кватернионов
EulerToQuat(yaw, pitch, roll) → CQuaternion
QuatToMatrix(q) → float[9]       // Quaternion → Matrix3x3
MatrixToQuat(m) → CQuaternion    // Matrix3x3 → Quaternion
```

### Integer версия (для PSX)

```c
MatrixToQuatInteger()    // fixed-point 15.15
QuatToMatrixInteger()
QuatSlerpInteger()       // SLERP с таблицей acos_table[1025]
```

**SLERP оптимизация:**
```c
// Если кватернионы близки (dot > 0.95) — линейная интерполяция
// Иначе стандартный SLERP:
scale0 = sin((1-t) * omega) / sin(omega)
scale1 = sin(t * omega) / sin(omega)
```

### Сжатая матрица CMatrix33 (для PSX)

- 3 SLONG с битовыми полями: `CMAT0_MASK`, `CMAT1_MASK`, `CMAT2_MASK`
- Распаковка: `((value & mask) >> shift) / 512.f`
- Упаковка: `float * 32768.f` → SLONG (fixed-point 16.15)
- Распаковывает `cmat_to_mat()`

---

## 4. Root() — целочисленный sqrt

```c
// StdMaths.cpp строки 92-95:
SLONG Root(SLONG square) {
    return (int)sqrt(square);  // обёртка над стандартным sqrt()
}
```

Используется в обеих системах для нормализации матриц (FMatrix.cpp строки 213, 233).

---

## 5. Fixed-point форматы

| Формат | Масштаб | Где используется |
|--------|---------|-----------------|
| 15.15 | `1 << 15` = 32768 | Кватернион integer версия |
| 16.16 | `1 << 16` = 65536 | Координаты (SLONG GameCoord) |
| PSX sin/cos | `>>1` от таблицы | FMatrix multiply |

---

## 6. Что переносить в новую версию

| Компонент | Подход |
|-----------|--------|
| Matrix3x3 float | **glm::mat3** |
| Quaternion float | **glm::quat** |
| Vector3 float | **glm::vec3** |
| SLERP | Встроен в glm (`glm::slerp`) |
| PSX FMatrix / integer quaternion | **Не переносить** (только PSX) |
| CMatrix33 (PSX сжатая) | **Не переносить** |
| AtanTable / SinTable lookup | **Не нужны** — libm достаточно быстр |
| Fixed-point 16.16 (GameCoord) | Сохранить для физики (1:1 воспроизведение) |
| Root() wrapper | Не нужен — использовать `std::sqrt()` |
