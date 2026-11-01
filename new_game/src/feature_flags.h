#ifndef FEATURE_FLAGS_H
#define FEATURE_FLAGS_H

// Runtime feature flags for cut/experimental features.
// All flags default to OFF (original game behavior).
// See FEATURE_FLAGS.md for descriptions.
//
// Naming convention:
//   cut_*  — features cut from the final release but present in pre-release code
//   ext_*  — new features added by OpenChaos (not in original game)

struct FeatureFlags {
    // --- Cut features (existed in pre-release, absent in final release) ---
    bool cut_bats; // Flying bats on missions (placeholder models)

    // --- Extended features (added by OpenChaos) ---
    bool ext_in_mission_saves; // In-mission quick save/load (see docs/topics/in_mission_saves.md)
};

extern FeatureFlags g_feature_flags;

#endif
