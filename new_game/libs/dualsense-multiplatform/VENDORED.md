# Vendored Library Info

- **Source:** https://github.com/rafaelvaloto/Dualsense-Multiplatform
- **Commit:** b8a76fff41cbd5909f7f71b57b6f3cea88b52bfa
- **Date:** 2026-02-16
- **Vendored on:** 2026-03-26
- **Removed from clone:** `.git/` only
- **Unneeded files** (Tests/, .github/, etc.) are ignored via `.gitignore`

## Submodule: Libs/miniaudio

- **Source:** https://github.com/rafaelvaloto/Gamepad-Core-Audio
- **Files:** `miniaudio.h` (public domain, mackron/miniaudio) + `miniaudio_impl.cpp` (MIT, Rafael Valoto)
- **Manually downloaded** (submodule was not initialized during vendoring)

## Notes

- The library's top-level `CMakeLists.txt` has a bug: uses `CMAKE_SOURCE_DIR` for miniaudio
  path, which breaks under `add_subdirectory`. We work around this by including `Source/`
  directly and adding miniaudio manually in our CMakeLists.txt. No vendored code is modified
  for this workaround.

## ⚠️ Local patches (NOT pristine)

As of 2026-04-15 this vendored copy carries a local patch on top of the
upstream commit listed above. Patch sites are wrapped in
`=== OPENCHAOS-PATCH BEGIN ... END ===` comment blocks — grep for
`OPENCHAOS-PATCH` to find every modification.

See [`README.md`](README.md) and the project devlog
[`dualsense_libs_reference/dualsense_lib_pr_notes.md`](../../../dualsense_libs_reference/dualsense_lib_pr_notes.md)
for the full list of active patches, rationale, and upstream PR plan.

When pulling a newer upstream version, either re-apply the patch on top
or verify it has been merged upstream and remove the marker blocks.
