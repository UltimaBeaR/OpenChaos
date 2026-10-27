# 1 — Plan: getting back to the project after 1.0

- Area: project
- Tags: plan
- Kind: targeted
- Called: "plan for getting back to the project"
- Created: 2026-10-27
- Original: [original-prompts/1-plan-getting-back-to-the-project-after.md](../original-prompts/1-plan-getting-back-to-the-project-after.md)

## Goal
The project is maintainable again after a long break: the work is organized in the new task system, the old docs are archived, the docs are in English, and the open GitHub issues are triaged. After that, bugs are fixed from the backlog. Done when all steps are ✅.

## Steps
1. ✅ Set up the new task system (skill `work-tracking`, `tracker/`)
2. ✅ Move the old system (known issues, planning, devlogs, stage-4 skills/tools) to `archive/`
3. ✅ Delete the original-game knowledge base; keep cut features + resource formats in `docs/topics/`
4. ✅ Migrate the open items of the old known-issues list and the stage-13 roadmap into tasks and plans
5. ✅ Update CLAUDE.md and README for the new structure
6. ✅ Triage the GitHub issues → tasks [#99](99-crash-in-the-fallen-police-dialogue.md) (issue #10),
   [#100](100-keyboard-key-rebinding.md) + #101 (issue #9), #102 + a note in #90 (issue #8, already closed)
   > access: read-only fine-grained token in `GH_TOKEN` (user-level Claude Code settings)
7. ⬜ Reply on GitHub issues #8, #9, #10 (the user)
8. ⬜ [#2](2-translate-the-living-docs-to-english.md) — translate the living docs to English

## Notes
- Bug-fixing approach agreed with the user: reproduce ourselves first; ask the issue author for help when unsure; prefer cheap, locally verifiable fixes; for anything needing in-game testing prepare a short exact check scenario.
- Rewriting the project on Rust + Vulkan was discussed and rejected for now (resets all testing to zero, breaks contributor momentum).
