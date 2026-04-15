#pragma once

// Weapon feel — HARDWARE MOTOR DELAY TEST (temporary, 2026-04-15)
//
// Measures whether the DualSense Weapon25 effect has a physical engagement
// delay between HID mode=AIM_GUN and the moment the motor is actually ready
// to deliver a click on an R2 press. If such a delay exists, it would
// explain I1 (rapid-tap missing clicks) in the weapon_haptic_and_adaptive_trigger
// devlog.
//
// Protocol — started by pressing `\` (KB_BACKSLASH) while holding a gun
// on foot:
//
//   1. For each delay N in a sweep series {0, 25, 50, 100, 200, 400, 800}ms:
//      a. Wait until the player releases R2 below reset_threshold.
//      b. Force trigger mode=NONE on HID.
//      c. Wait N milliseconds (wall clock).
//      d. Force trigger mode=AIM_GUN on HID.
//      e. Open a 2s window during which the player is expected to press R2
//         and mentally note whether a physical click was felt.
//      f. Wait for Y (click) / N (no click) key answer.
//   2. After the last delay, print a summary line to CONSOLE_status and the
//      weapon_feel_debug.log.
//
// While the test is active:
//   - weapon_feel_evaluate_fire returns no-shoot so the character does not
//     actually fire during the test.
//   - gamepad_triggers_update is bypassed so it does not fight our forced
//     mode transitions.
//
// All screen output goes through CONSOLE_status (game-tick-safe, persistent
// green text in the top-left). Results also go to weapon_feel_debug.log.

void weapon_feel_test_tick();

// True while a test run is in progress. Used by evaluate_fire and
// gamepad_triggers_update to get out of the way.
bool weapon_feel_test_is_active();
