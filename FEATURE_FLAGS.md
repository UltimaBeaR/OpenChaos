# Feature flags — cut and experimental features

Runtime flags for functionality that differs from the original release.

**All flags are OFF by default** = behavior identical to the original release (vanilla version).

## Flag categories

- **`cut_*`** — cut content: the code exists in the pre-release sources but is disabled (commented out, under `#ifdef UNUSED`, or otherwise deactivated). Absent from the final PC release.
- **`ext_*`** — extended content: added by OpenChaos, did not exist in the original. New features on top of the original game.

## How to enable

Change the value in `new_game/src/feature_flags.cpp` and rebuild.
In the future (stage 13) — a settings UI for enabling experimental features without recompiling.

## Files

- `new_game/src/feature_flags.h` — flag declarations (struct FeatureFlags)
- `new_game/src/feature_flags.cpp` — default values (all OFF)

---

## Cut flags (cut content)

### `cut_bats` — Bats

Bats are cut enemies. Their spawn code was disabled by MuckyFoot in the pre-release sources (`#ifdef UNUSED`). Absent from the final PC release.

**Where they appear:** the Stern Revenge mission (playing as Roper with a shotgun, the graveyard) — 23 bats, 5 flocks across the whole map, including the graveyard right at the start. They may also appear in other missions.

**Behavior when enabled:** bats fly around the level in flocks, bite (without a bite animation), and attack the player. They can be shot and killed, and the death animations work.

**Problems:** the textures are placeholders. The model is passable, but at the very least custom textures are needed. The model may also need work.

**Where in the code:** `new_game/src/missions/eway.cpp` — `EWAY_create_animal`, case `EWAY_SUBTYPE_ANIMAL_BAT`. AI/rendering logic is in `bat.cpp`.

**What's needed for a full enable:**
- Custom textures for the bat model (the current ones are placeholders)
- Testing on all missions where bats are placed in the data
- Balancing (if needed)

**Discovered:** 2026-04-11, during an ASan run on Stern Revenge. ASSERT(0) was crashing on mission load.

---

## Ext flags (extended content)

### `ext_in_mission_saves` — In-mission quick save / quick load

The original game saves only campaign progress between missions; its in-mission save code never worked. This flag adds a quick save inside a mission.

**Behavior when enabled:** the pause menu gets **Save game** and, when this level has a quick save, **Load game**; the level-failed screen offers **Load game** when the quick save matches your current campaign progress. One quick save per level, written to the user folder next to the campaign slots (`saves/quicksave_<level>.sav`). Saving is allowed on foot, outside a fight, when no enemy is engaged with the player and outside cutscenes; otherwise the pause menu shows one line saying why. Loading reloads the level and puts every gameplay system back where it was. The menu items use the game's translated strings; the messages are always English. No key or button is bound, and the Help screens are unchanged.

**What a load does not restore** (by design): sound and music (they restart), purely visual effects (particles, sparks, debris, decals, screen fades, on-screen text), the random-number stream, and the short melee/arrest cooldowns.

**With the flag off** (the default) the feature is inert: no menu item, no file, no key. One related change is not behind the flag: a mission restart now stops sound and music before the reload, because a playing voice holds a raw `Thing*` that the reload invalidates.

**How to enable:** set `.ext_in_mission_saves = true` in `new_game/src/feature_flags.cpp` and rebuild. There is no config key.

**Where in the code:** `new_game/src/missions/quicksave.h`, `quicksave.cpp`, `quicksave_format.h`. Design and limits: [docs/topics/in_mission_saves.md](docs/topics/in_mission_saves.md).
