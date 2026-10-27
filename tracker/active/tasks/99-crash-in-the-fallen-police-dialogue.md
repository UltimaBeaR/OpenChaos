# 99 — Crash in The Fallen during the police dialogue (EWAY_cam_cant_interrupt ASSERT)

- Status: 🔵 open
- Area: crash
- Tags: bug, gh-issue
- Created: 2026-10-27
- Related: GitHub issue [UltimaBeaR/OpenChaos#10](https://github.com/UltimaBeaR/OpenChaos/issues/10) (Sargiltheold)

## Description
Reported for v1.0.2: The Fallen crashes during the police officer dialogue, right after "You're the first one on
the scene", at the same point every time (also when the dialogue isn't interrupted). Message:
`Assertion failed: EWAY_cam_cant_interrupt == 0, eway.cpp line 2543`.

## Important notes
- This exact assert was already removed in commit `e6d5d8dc` ("Assert causing crash in the fallen mission is
  removed…"), which IS part of the v1.0.2 tag. In current code `eway.cpp:2542-2545` is only a comment about the
  removed assert. The same commit made release-build ASSERTs non-fatal.
- So the reporter most likely runs a binary without that commit (an older build, or a 1.0.2 package for their
  platform that was built from an older commit). Also the text format "Assertion failed: …" differs from our own
  ASSERT output — worth asking for the exact text/screenshot.
- First step: check the published v1.0.2 packages (per platform) really contain the fix; then get from the reporter
  the OS, the package they downloaded and the exact message / `OpenChaos.asserts.log` (that log exists only in builds
  with commit `e6d5d8dc`, so its absence points to an older binary).
- No code change is expected unless the reporter runs a build with the fix and still crashes.
- 2026-10-28: the user played the scenario on a **debug build of current main** — past "You're the first one on the
  scene", no assert, no hang (debug ASSERTs are fatal, so the assert is really gone from the code). The same day the
  user checked the **published v1.0.2 release package** — also fine.
- 2026-10-28: the report matches the old assert exactly: right before `e6d5d8dc`, `eway.cpp` line **2543** was
  `ASSERT(EWAY_cam_cant_interrupt == 0);` (same line number as in the report), and the crash log of that time wrote
  "Assertion failed: <expr>" + file + line. Same mission and moment as the original report that led to `e6d5d8dc`.
  Conclusion: the reporter runs a build older than v1.0.2 (1.0.1 or earlier).


## How to verify
Start mission **The Fallen**, walk to the police officer, let the dialogue play past "You're the first one on the
scene" (once without pressing anything, once skipping lines). Expected: the dialogue and the following cutscene play
to the end, no crash, no "ASSERTION FAILURE" message.
