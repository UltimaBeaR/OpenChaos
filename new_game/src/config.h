#pragma once

// Compile-time constants not exposed to the user config.
// Runtime settings (resolution, vsync, detail levels, etc.) live in OpenChaos.config.json.

// FOV pre-scale applied to the camera lens before auto-zoom.
// 1.0 = original game FOV. Adjust only with care — affects sky, poly correction, etc.
static constexpr float OC_FOV_MULTIPLIER = 1.0f;

// Language file path relative to the game directory.
// Full localization support is not yet implemented (see known_issues_and_bugs.md).
static constexpr const char* OC_LANGUAGE_FILE = "text/lang_english.txt";

// Max characters that get a full (detailed silhouette) shadow at once;
// everyone else gets the simple blob. Packed into the shared shadow page
// (TEXTURE_SHADOW_SIZE) as AENG_AA_BUF_SIZE tiles in a square grid, so:
//   OC_MAX_DETAILED_SHADOWS <= (TEXTURE_SHADOW_SIZE / AENG_AA_BUF_SIZE)^2
// Cost: each one is a separate GPU silhouette pass per frame — raising
// this scales per-frame GPU work where that many people are on screen.
// May become config/hardware-driven later.
static constexpr int OC_MAX_DETAILED_SHADOWS = 6;
