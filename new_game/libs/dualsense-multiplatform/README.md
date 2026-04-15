# Dualsense-Multiplatform (vendored)

Cross-platform library for full DualSense (PS5) controller support: input, adaptive triggers,
haptic feedback, LED control, touchpad, gyroscope, and audio-to-haptic conversion.

Used by OpenChaos for native DualSense support (Stage 5.1, Iteration B).

## ⚠️ Local patches

This vendored copy is **NOT pristine** — it carries a local patch that is
not yet upstreamed. To find every patch site in the source, grep for the
marker `OPENCHAOS-PATCH`:

```
git grep "OPENCHAOS-PATCH" Dualsense-Multiplatform/Source
```

Each patch is wrapped in a `=== OPENCHAOS-PATCH BEGIN ... END ===` comment
block so the boundaries are unambiguous.

**Active patches (2026-04-15):**

- **Trigger feedback status reading** — adds 4 fields to `FInputContext`
  (`LeftTriggerFeedbackState`, `RightTriggerFeedbackState`,
  `bLeftTriggerEffectActive`, `bRightTriggerEffectActive`) and reads
  bytes 41/42 from the input report inside `DualSenseRaw`. Lets us
  detect physical adaptive-trigger click events directly from hardware
  signal instead of guessing by analog trigger position.

  Full rationale, reverse-engineering sources, empirical measurements,
  PR preparation checklist, and cross-platform validation TODOs are in
  [`new_game_devlog/dualsense_lib_pr_notes.md`](../../../new_game_devlog/dualsense_lib_pr_notes.md).

  Patch sites:
  - `Source/Public/GCore/Types/Structs/Context/InputContext.h` — new struct fields
  - `Source/Public/GImplementations/Utils/GamepadInput.h` — read code in `DualSenseRaw`

When updating the vendored copy to a newer upstream commit, **either**
re-apply this patch on top, **or** check whether it has been merged
upstream and remove the marker blocks accordingly.

## Structure

```
.gitignore                 -- ignores everything in the clone except what we need
VENDORED.md                -- commit hash, date
README.md                  -- this file
Dualsense-Multiplatform/   -- full clone (minus .git/)
  Source/Public/            -- public headers          (tracked)
  Source/Private/           -- implementation           (tracked)
  Libs/miniaudio/           -- audio lib (audio-to-haptic)  (tracked)
  CMakeLists.txt            -- library build            (tracked)
  LICENSE                   -- MIT                      (tracked)
  Tests/, .github/, ...     -- everything else          (ignored by .gitignore)
```

## Updating

1. Delete `Dualsense-Multiplatform/`
2. From this directory (`new_game/libs/dualsense-multiplatform/`):
   `git clone https://github.com/rafaelvaloto/Dualsense-Multiplatform.git Dualsense-Multiplatform`
3. Remove only `Dualsense-Multiplatform/.git/`
4. Pull the miniaudio submodule:
   `cd Dualsense-Multiplatform && git clone https://github.com/rafaelvaloto/Gamepad-Core-Audio.git Libs/miniaudio && rm -rf Libs/miniaudio/.git`
5. Update `VENDORED.md` with new commit hash and date
