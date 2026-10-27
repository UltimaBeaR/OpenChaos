# 101 — Environmental reverb (EAX-like)

- Status: ❄️ deferred
- Area: audio
- Tags: analysis, feature, gh-issue
- Created: 2026-10-27
- Related: GitHub issue [UltimaBeaR/OpenChaos#9](https://github.com/UltimaBeaR/OpenChaos/issues/9) (question 3: "Does
  EAX reverberation work?")

## Description
Environmental reverb = the acoustics of the place the player is in (dry outdoors, echoing tunnel, dense small room,
long tail in a big hall). Back in 1999 it was done by Creative's EAX on Sound Blaster cards. Our build has no reverb:
audio goes through OpenAL Soft, which can do the same in software (EFX reverb), no special hardware needed.

Possible feature: add reverb as a **new optional effect, off by default** (it is not a parity fix — see notes).

## Important notes
- **The original PC version did not use reverb** (checked 2026-10-28). The original PC sound backend was Miles
  (`MFX_Miles.cpp`); the community OpenAL port removed it from `original_game/`, but it is still in the PieroZ repo
  history, parent of commit `ed87b53` = `f2270b3`
  ([MFX_Miles.cpp](https://github.com/PieroZ/MuckyFoot-UrbanChaos/blob/f2270b3f5b46cbcb12a08d7274a83bad94b0ef1b/fallen/DDLibrary/Source/MFX_Miles.cpp)):
  - at startup it prefers the "Microsoft DirectSound3D with Creative Labs EAX(TM)" 3D provider (L296) — that is why
    the retail data ships `MSSEAX.M3D` — but uses it only for 3D positioning;
  - with EAX selected it sets "EAX effect volume" = 0 (L576-580) and "EAX sample reverb mix" = 0 for every sound
    (L939-943), i.e. reverb is explicitly off;
  - `MFX_set_environment` exists only in other backends (QMDX, the older DirectSound path, PS1), is commented out in
    the PC header and has no callers;
  - the mission zone flag `ZF_REVERB` (editor string "Reverb") is never read.
  So "no reverb" matches the original PC behaviour (pre-release sources; no sign the final release differs).
- **Effort estimate:**
  - simple version ≈ 1 day of work + a 1–2 minute listening check by the user: one "room" reverb preset via OpenAL Soft
    EFX on game sounds (not music/UI) while the player is in a reverb zone, smooth transition at the zone border;
  - first step before anything: check whether `ZF_REVERB` is actually set in the level data — if not, "indoors" has to
    be detected another way (e.g. the inside-building state): +1–2 days and more risk;
  - no reference to compare with (EAX doesn't work on modern Windows without wrappers) — the sound is tuned by ear;
  - different presets per place (tunnel / hall / room) — a separate refinement, +a couple of days.
- **Cost:** some CPU for the reverb DSP — probably fine on the Steam Deck, but measure (release build).
