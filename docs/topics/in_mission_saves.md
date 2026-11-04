# In-mission saves (quick save / quick load)

An optional feature: save the running mission and load it again later, also after quitting the game. The original
game saves only campaign progress between missions; its in-mission save code never worked.

## Enabling it

Behind the feature flag `ext_in_mission_saves`, **off by default**; with the flag off the feature is inert. One
related change is not behind the flag: a mission restart now stops sound and music before the reload, because a
playing voice holds a raw `Thing*` that the reload invalidates. To enable it, set `.ext_in_mission_saves = true` in `new_game/src/feature_flags.cpp` and rebuild.

## Using it

- **Save:** pause → **Save game**. One file per level, `saves/quicksave_<level>.sav` in the user folder next to the
  campaign slots, about 3.5 MB. No key or button is bound; messages are always English.
- **Load:** pause → **Load game**, shown when this level has a save. The mission-failed screen offers it too when
  the save matches your current campaign progress.
- **Campaign progress** travels with the save. If it differs from your current progress, loading from the pause
  menu asks first. A load restores it in memory only; slot files change when you next save the campaign.

## How it works

All logic is in `new_game/src/missions/quicksave*`; the file format is documented in `quicksave_format.h`. A save is
a pure read of the frozen game state while the pause menu is up, with every pointer written as an index or offset.
A load validates the whole file first, so a bad file leaves the running mission alone. It then restarts the level
through the normal `GS_REPLAY` path, and `QUICKSAVE_apply_pending()` overlays the saved state before the first tick.

## What is saved and what is not

**Saved:** 46 memory pools from `save_table[]` (the reload rebuilds the other entries), the campaign data, game state
kept in plain globals (counters, EWAY script clock, mission timer, statistics), AI gang tables, camera and sniper
scope, grenades, ribbons and the grappling-hook rope. All gameplay state comes back.

**Deliberately not restored:** sound and music (they restart), purely visual effects, the random-number stream,
`GAME_TURN` (it restarts at 0), and short-lived statics in other files: melee and arrest cooldowns, combo and block
timers, steering accumulators, camera smoothing. The reload resets them, or they are idle under the limits below.

## Why saving is limited to on foot, out of combat, outside cutscenes

The player must be alive and on foot, not fighting, grappling, carrying, being hit or arrested; no NPC may be
killing, hunting, arresting or sniping the player within its sight range of 8 map squares (these AI states never time
out, so one that lost the player far away is ignored); and no cutscene, tutorial message or modal dialog may be up. A
refused save says why in the menu. This is a trade-off, not a technical limit:

1. **Minimal invasion, to keep the feature easy to maintain.** Under these limits the player's transient combat,
   input and vehicle state is idle, so it can be reset on load. Saving it would need accessors in many game files.
2. **Too many edge cases to test.** Vehicles (entering, driving, leaving, passenger), mid-punch, mid-grapple,
   blocking and a sniper aiming each bring a matrix of states we cannot test properly.

## Path to an unrestricted save

1. Bump `QS_FORMAT_VERSION` and add the new data as extra records.
2. Save and restore `GAME_TURN`, then audit every pool field that stores a future `GAME_TURN` value. `Person` and
   `Vehicle` were never audited for this; it is the main correctness risk.
3. Add accessors for the statics in `combat/combat_cooldown.cpp`, `combat/combo_gate.cpp`, `things/characters/person.cpp`,
   `game/input_actions.cpp` and `engine/effects/psystem.cpp`; some must first move to file scope.
4. Add a focus resync in `camera/fc.cpp` for camera smoothing.
5. Drop the vehicle and combat clauses of `qs_allowed_now()`, then test the edge cases listed above.

Saving inside a cutscene is not planned: it has no value for the player.

## Adding a field to the format

Add it to a disk struct in `quicksave_format.h` with a fixed-width type, a pointer as an index or offset, a wall-clock
stamp as an age. Bump `QS_FORMAT_VERSION`: the reader refuses every other version. Update that struct's `static_assert`s,
write the field in the encoder and restore it in `qs_read_apply()`, after the owning system's init and any pool it uses.

## Debug verifier

After a load, Debug builds re-encode the live state and compare it with the file, logging each mismatch to `stderr`.
Expect `QUICKSAVE verify: OK`; the fields a load recomputes on purpose are listed in `QS_VERIFY_SKIPS`.
