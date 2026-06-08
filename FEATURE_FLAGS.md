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

(none yet)
