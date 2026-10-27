# DualSense — known hardware / firmware quirks

Controller behaviors that look like bugs but **reproduce in daidr/dualsense-tester** (the Web HID reference tester)
on the same hardware (checked 2026-04-19). So they are properties of the DS5 firmware / audio ASIC, not of our
library — don't spend time "fixing" them from the host side.

Origin: section 14 of the (archived) plan of our own lib,
`archive/new_game_devlog/dualsense_libs_reference/own_dualsense_lib_plan.md`.

## Audio cross-leak between outputs

**Symptom:** a `WAVEOUT_CTRL` test tone routed to the headphones (`audioControl = 0x00`, prepare payload
`[4]=4, [6]=6`) is heard **both in the headphones and in the built-in speaker**. Routed to the speaker (`0x30`), it
partly leaks into the jack when headphones are connected. Very clear over BT, quiet but present over USB.

**Likely cause:** the DS5 internal audio chain doesn't fully separate the outputs when the test tone is generated —
the generator feeds both amps, and the `audioControl` nibble controls only relative gain, not a physical switch.

**Workaround (ours and daidr's):** when the tone is on, the UI forces the volume of the inactive output to 0.
`audio_apply_state()` in `input_debug_dualsense.cpp` does it (single-slider model: the active output gets the
volume, the inactive one 0).

**Why not fix:** impossible from the host — `audioControl` is the only routing control and it's already correct.

## Speaker gets louder when headphones are plugged in (speaker mode)

**Symptom:** with `audio_route = Speaker` (`audioControl = 0x30`) and the test tone on, plugging headphones in makes
the **speaker noticeably louder**, although the host sends identical output reports.

**Likely cause:** with speaker output explicitly requested the firmware doesn't mute the speaker on plug-detect and
seems to switch to some gain-compensation / power mode.

**Workaround:** none — no HID path tells the controller to keep the speaker gain independent of the jack.

## Test tone keeps playing after the game exits

**Symptom:** the `WAVEOUT_CTRL` tone started from the debug panel keeps sounding after the game is closed normally
(LEDs, lightbar, rumble and triggers are reset correctly). Only reconnecting the controller stops it.

**Tried, didn't help:** `test_command(WAVEOUT_CTRL, {0,1,0})` with 0x81 polling (0x80 goes out, reply times out);
two disables 80 ms apart; quiet output before/after the disable, with/without `audio_volumes_enabled`; a 200+ ms delay
before `device_close`.

**Likely cause:** the DS5 audio generator is a firmware subsystem separate from the HID output path; once the HID
handle is closed nothing processes the disable (possibly tied to the 0x81 polling cadence, or a race with process
teardown).

**Workaround:** none reliable. `ds_shutdown` makes a best-effort attempt (double disable with a pause, quiet output,
50 ms pre-close delay) — sometimes works, usually not.

## About the `WAVEOUT_CTRL` test tone in general

`WAVEOUT_CTRL` + `BUILTIN_MIC_CALIB_DATA_VERIFY` is **Sony's factory diagnostic mechanism**, not a production feature.
It's flaky in several places: a command is processed only with 0x81 polling (fire-and-forget is ignored by the
firmware), the cross-leak, the gain boost on jack plug, and it doesn't stop on shutdown. Use it only to check that
the audio hardware works at all.

## General rule

If a behavior **reproduces in daidr** on the same hardware, it's a hardware/firmware limit, not our bug: record it
here and move on.
