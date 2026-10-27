# 64 — non-Windows + Xbox over USB: controller not detected

- Status: ❄️ deferred
- Area: input
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Repro (2026-04-17): Xbox controller connected via USB — the game doesn't see the controller at all. **Reproduces on macOS and Steam Deck (Linux).** The controller simply doesn't activate — none of the "bad behavior" symptoms like the DualSense, it's just not in the enumeration at all. **Online testers (e.g. gamepad-tester.com) also don't see the Xbox controller on the same OS+USB** → the problem is below us, at the system level. **Nuance:** both tests on Mac and Steam Deck were via a docking station — possibly the dock doesn't pass through the Xbox USB properly. The Xbox controller was capricious even on Windows — it didn't connect the first time without disconnecting other USB devices. This greatly increases the likelihood that on Mac/Linux via a dock the root cause is the same — power / USB enumeration. **Plan:** a known limitation for the release, recommend BT or a direct connection without a dock; if it turns out the bug is ours — raise it back to active.

Old-list status: *Known limitation (probably not ours)*.
