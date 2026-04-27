#pragma once

// Compile-time constants not exposed to the user config.
// Runtime settings (resolution, vsync, detail levels, etc.) live in openchaos/config.json.

// FOV pre-scale applied to the camera lens before auto-zoom.
// 1.0 = original game FOV. Adjust only with care — affects sky, poly correction, etc.
static constexpr float OC_FOV_MULTIPLIER = 1.0f;

// Language file path relative to the game directory.
// Full localization support is not yet implemented (see known_issues_and_bugs.md).
static constexpr const char* OC_LANGUAGE_FILE = "text/lang_english.txt";
