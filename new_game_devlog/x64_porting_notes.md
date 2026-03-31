# x64 Porting — типичные баги и паттерны фиксов

Накопленные знания по портированию с 32-бит на 64-бит. Каждый паттерн встречался в реальном коде.

## Pointer truncation (обрезка адреса до 32 бит)

### Pointer alignment
```cpp
// БАГ: (DWORD) обрезает верхние 32 бита адреса
ptr = (type*)(((DWORD)buffer + 31) & ~31);
// ФИКС:
ptr = (type*)(((uintptr_t)buffer + 31) & ~(uintptr_t)31);
```
**Где встречалось:** farfacet.cpp, fastprim.cpp, superfacet.cpp, aeng.cpp, figure_globals.cpp (7 мест), figure.cpp.
**Варианты синтаксиса:** `(DWORD)var`, `(SLONG)var`, `DWORD(var)`, `SLONG(var)` — искать все.

### Pointer в 32-бит поле
```cpp
// БАГ: указатель не помещается в SLONG
packet->pos.X = (SLONG)text_ptr;
// ФИКС: хранить offset, не указатель
packet->pos.X = (SLONG)(text_ptr - base_buffer) + 1;  // +1 если 0 = sentinel
```
**Где встречалось:** playcuts.cpp (текст катсцен в GameCoord.X).

### Callback с pointer-параметрами
```cpp
// БАГ: SLONG параметр обрезает указатель
typedef SLONG (*Callback)(Widget*, SLONG code, SLONG data1, SLONG data2);
widget->data[0] = (SLONG)MemAlloc(sz);
// ФИКС:
typedef intptr_t (*Callback)(Widget*, SLONG code, intptr_t data1, intptr_t data2);
widget->data[0] = (intptr_t)MemAlloc(sz);
```
**Где встречалось:** widget.h/cpp (WIDGET_Data callback, INPUT/LISTS/TEXTS_Data).

### PRNG seed из pointer
```cpp
// БАГ: UB, хотя не крашит
dr = ((SLONG)pyro) + b;
// ФИКС: double cast
dr = ((SLONG)(uintptr_t)pyro) + b;
```
**Где встречалось:** pyro.cpp (2 места), vehicle.cpp (siren animation).

## Файловый I/O — sizeof mismatch

### fread структур с указателями
```cpp
// БАГ: sizeof(GameKeyFrame) = 20 на x86, 36 на x64
FileRead(handle, data, sizeof(GameKeyFrame) * count);
// ФИКС: on-disk структура с uint32_t вместо указателей
FileRead(handle, disk_data, sizeof(GameKeyFrame_Disk) * count);
// + поэлементное копирование с (uintptr_t) кастом
```
**Где встречалось:** anim_loader.cpp (load_game_chunk, load_append_game_chunk), playcuts.cpp (CPChannel).

### Pointer relocation при изменённом sizeof
```cpp
// БАГ: byte-offset relocation не работает если sizeof элемента изменился
new_ptr = old_ptr + (new_base - old_base);  // неверно если sizeof(Element) разный
// ФИКС: index-based relocation
idx = (old_ptr - old_base) / sizeof(Element_Disk);  // old sizeof
new_ptr = &new_array[idx];
```
**Где встречалось:** anim_loader.cpp (old-format relocation для GameKeyFrame, GameFightCol).

## Sentinel -1 в pointer-полях

```cpp
// БАГ: -1 (0xFFFFFFFF) загружается через uint32_t → uintptr_t
// zero-extends до 0x00000000FFFFFFFF → (intptr_t) видит как ПОЛОЖИТЕЛЬНОЕ
if (((intptr_t)p[c0].Fight) < 0)  // НЕ СРАБАТЫВАЕТ на x64!
// ФИКС: sign-extend через int32_t
int32_t idx = (int32_t)(uintptr_t)p[c0].Fight;
if (idx < 0) p[c0].Fight = NULL;
```
**Где встречалось:** memory.cpp (convert_keyframe_to_pointer, convert_animlist_to_pointer, convert_fightcol_to_pointer). Самый коварный баг — крашит не при загрузке, а при первом обращении к fight collision.

## Поиск оставшихся багов

Паттерны для grep по всему `new_game/src/` (исключая `backend_directx6/`):
```
(SLONG)ptr, (ULONG)ptr, SLONG(ptr), ULONG(ptr)
(DWORD)ptr, (int)ptr, (unsigned)ptr
(uint32_t)ptr, (int32_t)ptr
```
Каждое найденное место — проверить: это каст числа или указателя?
