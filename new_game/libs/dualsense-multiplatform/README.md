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

**Active patches (2026-04-15..16):**

1. **Trigger feedback status reading** — adds 4 fields to `FInputContext`
   (`LeftTriggerFeedbackState`, `RightTriggerFeedbackState`,
   `bLeftTriggerEffectActive`, `bRightTriggerEffectActive`) and reads
   bytes 41/42 from the input report inside `DualSenseRaw`. Lets us
   detect physical adaptive-trigger click events directly from hardware
   signal instead of guessing by analog trigger position.

   Patch sites:
   - `Source/Public/GCore/Types/Structs/Context/InputContext.h` — new struct fields
   - `Source/Public/GImplementations/Utils/GamepadInput.h` — read code in `DualSenseRaw`

2. **Weapon25 packing rewrite** — the original `Weapon25()` function in
   `GamepadTrigger.h` packed parameters into HID bytes using a scheme
   that does not match the reverse-engineered Sony Weapon (0x25) layout.
   Effect: `StartZone` / `Amplitude` / `Behavior` parameters had no
   meaningful effect on trigger behavior — hardware received garbage
   and produced a near-default click regardless of input. Patch
   rewrites the packing to match the Nielk1 gist spec. Verified by A/B
   comparison (same input values produce dramatically different feel
   with vs. without patch).

   Patch site:
   - `Source/Public/GImplementations/Utils/GamepadTrigger.h` — `Weapon25()` function body

**Other known lib issues (NOT patched yet):**

Most other trigger effect functions in `GamepadTrigger.h` are also
broken in a similar way (Bow22, MachineGun26, Machine27 use incorrect
packing; Galloping23 has the bitmap right but broken feet encoding).
See the full audit in
[`new_game_devlog/dualsense_lib_pr_notes.md`](../../../new_game_devlog/dualsense_lib_pr_notes.md)
section 6.6. OpenChaos does not currently use those effects in
production so they are left untouched.

Full rationale, reverse-engineering sources, empirical measurements,
PR preparation checklists, and cross-platform validation status are
all in [`dualsense_lib_pr_notes.md`](../../../new_game_devlog/dualsense_lib_pr_notes.md).

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
