# DualSense improvements (part of Stage 13)

Everything concerning DualSense-specific features. Basic support was done in Stage 5.1 (LED, adaptive triggers for the pistol, vibration). Here — what's left.

Technical documentation for the library API → [stage5_1_dualsense.md](stage5_1_dualsense.md)
Generic gamepad improvements (generic + shared) → [stage13_gamepad.md](stage13_gamepad.md)

---

## ~~Sync the adaptive trigger with the shot~~ ✅ Done
Implemented via act-bit detection. See [`known_issues_and_bugs_resolved.md`](../known_issues_and_bugs/known_issues_and_bugs_resolved.md) — "DualSense adaptive trigger: click doesn't coincide with the shot".

## ~~Weapon-specific feedback (shotgun, machine gun)~~ ✅ Done (2026-04-19)

Implemented for all three main weapons. The actual configuration — in [`weapon_feel.cpp`](../new_game/src/engine/input/weapon_feel.cpp):

- **Pistol:** Weapon25 click zones 4-6, strength 5. Envelope on the DualSense slow motor.
- **Machine gun (AK47):** Machine (0x27) two-amplitude pulse (start=4, end=9, ampA=7, ampB=3, freq=8, period=80) — a continuous rattle across the whole range. Envelope on the DualSense slow motor + **on Xbox per-motor routing 75/100** (both motors, low slightly weaker for a more "layered" feel).
- **Shotgun:** Weapon25 click (zones 3-6, strength 7 — heavier than the pistol) as a baseline + **post-shot Vibration burst** — after a shot, `ds_trigger_vibration` (position=0, freq=60Hz) is engaged on the trigger with a linearly decaying amplitude 8→0 over 0.25s. Emulates recoil in the trigger. Infrastructure: `TriggerEffectType::Vibration` + `post_shot_vibration_*` fields in the profile + `weapon_feel_tick_trigger_vibration()` API + override in `gamepad_triggers_update`.

Implementation details — in the resolved-bugs archive.

**Reference:** modern PS5 shooters (Returnal, Ratchet & Clank, COD MW2) — each weapon has its own trigger profile and haptics.

## Touchpad (B5)
- API: `EnableTouch()`, read `TouchPosition`/`TouchRelative`/`TouchFingerCount`
- Options: camera, map, quick inventory access
- Requires a design decision

## Gyroscope (B6)
- API: `EnableMotionSensor()`, read `Gyroscope`/`Accelerometer`
- Use: precise aiming in first-person mode
- Requires calibration and UI

## Audio-to-haptic (B7)

Gameplay idea: converting in-game sounds into tactile feedback
on the controller.

- Priority effects: gunfire (recoil), explosions, hits, the car
  engine (the feel of the revs), footsteps on different surfaces.
- For 1.0 we take a **simplified path** — the envelope of the real WAV
  modulates the regular rumble motors (low-frequency + high-frequency).
  Doesn't require a new path in libDualsense.
- **Full audio-haptic** (PCM via HID 0x32 straight into the controller's
  DSP — the way Astro's Playroom does it) — a separate task,
  blocked on the not-yet-implemented transport in libDualsense. When it
  appears, we develop the game side (which sounds to play, how to
  normalize them) here. See the library plan:
  [`own_dualsense_lib_plan.md §13.2`](../new_game_devlog/dualsense_libs_reference/own_dualsense_lib_plan.md).

## Vibration in video cutscenes
- `MDEC_vibra[]` — frame-synchronized vibration for intro/endgame
- Requires integration with the video player

## Vibration in menus
- Test vibration when toggling the option in settings
- The option is not in the UI yet

## libDualsense follow-up (post-1.0 library improvements)

The whole list of deferred tasks for libDualsense itself is collected in the
library plan:
[`new_game_devlog/dualsense_libs_reference/own_dualsense_lib_plan.md`](../new_game_devlog/dualsense_libs_reference/own_dualsense_lib_plan.md).

In short, what's there:
- **§12 Convenience layer** — introduce a clear split into a raw API
  (internal, bytes/HID units — the current state) and a convenience API
  (public, percentages / normalized float / physical units / enums).
  Right now the library is a **mixed bag**: some fields are already convenience
  (sticks/triggers/battery in `DS_InputState`, enums for
  MuteLed/AudioRoute), some are still raw (rumble 0..255, LED brightness
  0..2 with an inverted scale, IMU int16 without applied calibration,
  audio volumes 0..255). The inconsistency table — in §12.1 of the plan.
  Along the way, reworking the in-game tester into friendly formats (it currently
  shows raw bytes for diagnosing wire bugs — this was justified during
  the development stage, but gets in the way of normal use).
  See §12.5 of the plan.
- **§13.1 USB Audio Class PCM output** (regular sound to the controller's
  speaker/jack) — **not doing it, documentation only**. Works over USB
  "by itself" like a regular OS sound card, but we don't need it.
- **§13.2 Audio haptics via HID 0x32** — PCM vibration (like Astro's
  Playroom: the sound of footsteps in snow is "felt" in the palms). The only
  way to deliver custom audio content to a **BT controller**
  (USB Audio Class over BT is unavailable). Requires reverse-engineering
  Sony's undocumented wire format. This is the same thing as "Audio-to-haptic
  (B7)" above in this same document — there it's a high-level description of the feature
  from the game's side, and in §13.2 of the library plan — exactly how to implement
  the API inside the library. They're related.
