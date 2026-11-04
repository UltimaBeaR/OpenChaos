# 132 — save_table pools: 8-byte alignment on x64

- Status: closed 2026-11-04
- Area: architecture
- Tags: bug, gh-issue
- Created: 2026-11-04
- Related: GitHub issue [UltimaBeaR/OpenChaos#13](https://github.com/UltimaBeaR/OpenChaos/issues/13) (Allexin)

## Description
`init_memory()` carved the `save_table[]` pools out of one block, rounding each to 4 bytes. On x64 structs with pointers
need 8, so pools could start misaligned (`EWAY_counter` is 10 bytes, rounded to 12, so `vehicles` right after it is off).
x86-64 tolerates it, but it's undefined behaviour and can fault on a strict-alignment target such as a future ARM port.
Now each pool is rounded to 8 (`MEM_POOL_ALIGN`), including in the size calculation.

## Important notes
- 8 rather than `alignof(std::max_align_t)` (the other option in the issue): 8 is what the pool structs need. The block
  size now counts the padding, so it doesn't rely on the spare 1024 bytes.
- No gameplay change: only the pool addresses inside the block move. Only `memory.cpp` uses the block itself.
- The debug-only `load_whole_game` path still uses its own 4-byte padding; it reads a format that nothing writes
  (`save_whole_game` is an empty stub).

## How to verify
Start any mission and play for a couple of minutes: everything behaves as before, no new crashes or asserts.
