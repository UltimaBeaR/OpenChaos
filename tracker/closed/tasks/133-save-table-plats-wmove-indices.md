# 133 — SAVE_TABLE_PLATS / SAVE_TABLE_WMOVE point at the wrong entries

- Status: closed 2026-11-04
- Area: architecture
- Tags: bug, gh-issue
- Created: 2026-11-04
- Related: GitHub issue [UltimaBeaR/OpenChaos#12](https://github.com/UltimaBeaR/OpenChaos/issues/12) (Allexin)

## Description
The two indices were off by one, so the platform limit read the `Maximum` of `wmove` (192 instead of 32) and the wmove
limit read that of `mav_opt` (1024 instead of 192). `PLAT_init` and `WMOVE_init` also cleared too much memory, into the
next pools. Now `SAVE_TABLE_PLATS` = 44, `SAVE_TABLE_WMOVE` = 45.

## Important notes
- From the original: its table has a `#ifdef BIKE` entry that is compiled out, and the indices counted it.
- Checked all 96 `.ucm` files for an upper bound (2026-11-04): at most 15 platforms and at most 155 wmove faces (counting
  every vehicle as 5 faces), so nothing reaches the real limits (31 and 191). Among the 33 campaign missions of
  `urban.sty`: at most 4 platforms and 145 faces.
- The over-clearing was harmless at level load by call order: those pools are filled later.
- Still open: a custom map above 191 faces overruns the pool, since `WMOVE_create` only has an ASSERT (not fatal in
  release). Not changed here.

## How to verify
Start **Estate of Emergency** (most vehicles) and **Day Of Reckoning** (most platforms), drive/walk on them a bit, restart
the mission and also switch to another level: everything works as before, no new asserts or crashes.
