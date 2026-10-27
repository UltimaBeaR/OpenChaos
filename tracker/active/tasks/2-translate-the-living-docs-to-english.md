# 2 — Translate the living docs to English

- Status: 🔵 open
- Area: project
- Tags: feature
- Created: 2026-10-27
- Related: step of plan [#1](1-plan-getting-back-to-the-project-after.md)

## Description
Everything written into the repo is in English (the repo is public; the assistant still talks to the user in
Russian). `archive/` is NOT translated — it stays as is.

Scope as of 2026-10-27 (lines containing Cyrillic):
- `docs/topics/dualsense/` — ~2600 lines: `daidr_dualsense_tester_review.md` (990), `own_dualsense_lib_plan.md`
  (658), `dualsense_lib_pr_notes.md` (370), `dualsense_protocol_reference.md` (184),
  `dualsense_adaptive_trigger_facts.md` (171), `libs_contents.md` (163), `README.md`, `lib_scope.md`.
- `docs/topics/resource_formats/` — ~640 lines (7 files + README).
- `docs/topics/debug_keys.md` (115), `docs/topics/original_cut_features.md` (49), `docs/topics/x64_porting_notes.md` (46).
- Skills — only a few Russian trigger words in descriptions (keep them: they help triggering on Russian requests).
- Code comments: ~35 Cyrillic lines in ~18 source files (e.g. `frontend_globals.cpp`, `weapon_feel.cpp`, `aeng.cpp`,
  `person.cpp`, `act_foot.h`, `common_frag.glsl`) — the 1.0 goal was "no Cyrillic anywhere" (archive `stage12.md`).
- `tools/coan/usage.md` (39 lines).

## Important notes
- `CLAUDE.md`, the `work-tracking` skill and the tracker were written in English from the start (2026-10-27).
- 2026-10-27: the DualSense history docs (own-lib plan, daidr review, PR notes, libs download list) were moved back
  to `archive/new_game_devlog/dualsense_libs_reference/`; the DualSense part left to translate is
  `dualsense_protocol_reference.md`, `dualsense_adaptive_trigger_facts.md`, `lib_scope.md` (README and
  `dualsense_hardware_quirks.md` are already English).
