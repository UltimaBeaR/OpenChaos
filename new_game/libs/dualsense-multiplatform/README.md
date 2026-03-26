# Dualsense-Multiplatform (vendored)

Cross-platform library for full DualSense (PS5) controller support: input, adaptive triggers,
haptic feedback, LED control, touchpad, gyroscope, and audio-to-haptic conversion.

Used by OpenChaos for native DualSense support (Stage 5.1, Iteration B).

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
