# 131 — PR #16: in-mission quick saves

- Status: 🔵 open
- Area: gameplay
- Tags: feature, gh-issue
- Created: 2026-11-04
- Original: [original-prompts/131-pr-16-in-mission-quick-saves.md](../original-prompts/131-pr-16-in-mission-quick-saves.md)
- Related: PR [UltimaBeaR/OpenChaos#16](https://github.com/UltimaBeaR/OpenChaos/pull/16) (Allexin), issues
  [#14](https://github.com/UltimaBeaR/OpenChaos/issues/14), [#17](https://github.com/UltimaBeaR/OpenChaos/issues/17);
  task "Quick saves (checkpoints) within a mission" (#20)

## Description
Decide what to do with PR #16: a quick save inside a mission (pause menu, on foot, out of combat, outside cutscenes),
behind the compile-time flag `ext_in_mission_saves`, off by default. About 4200 lines, almost all in a new module
`missions/quicksave*`.

This is a new mechanic that departs from the original, so it needs close review and a decision before merging.

## Important notes
- Reviewed so far (2026-11-04): only the changes outside the new module, and that the flag gates it. With the flag off the
  feature is inert, except that a mission restart now stops sound and music. The module itself is **not reviewed**.
- **Issue #14 depends on this PR.** Four `save_table[]` entries (`cutscene_*`) point at static arrays, so `init_memory()`
  writes an arena address into the first bytes of each array and about 0.9 MB of the arena stays unused. It looks harmless
  today and comes from the original (the entries are PC-only; the arrays are pointers on PS1). The natural fix is to
  remove these four entries, but the PR works with this table and addresses its pools by literal indices, so the fix can
  conflict with it. Decide on the PR first, then fix #14 accordingly.
