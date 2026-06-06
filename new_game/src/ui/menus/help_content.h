#ifndef UI_MENUS_HELP_CONTENT_H
#define UI_MENUS_HELP_CONTENT_H

// In-game "Controls / how-to-play" help content.
//
// Each topic has a title plus a SEPARATE body per input device (keyboard&mouse /
// Xbox / PlayStation) — there is no unified text, because usage and wording can
// differ per device. The detail screen shows the body for the active device and
// swaps it when the device changes. Bodies are drawn through the rich-text glyph
// drawer (input_glyph_text_draw*) and may contain inline glyph tokens `{id}` from
// input_prompt_map (e.g. {kb_w}, {xb_a}, {ps_cross}).
//
// The body text lives in per-device files (help_content_kbm/xbox/ps.cpp), with
// reusable snippets in help_content_common.h; this table just wires titles to the
// three bodies.

struct HelpTopic {
    const char* title;
    const char* body_kbm; // keyboard & mouse body
    const char* body_xbox; // Xbox body
    const char* body_ps; // PlayStation body
    // When true this is the input-prompt CATALOG (dev tool, gated by
    // OC_DEBUG_INPUT_PROMPT_CATALOG), not text — the bodies are ignored. Omitted
    // in normal entries (aggregate-init leaves it false). Must stay LAST in
    // HELP_TOPICS (see GAMEMENU_help_list_count).
    bool input_test;
};

// Topic table and its element count. Add a topic by appending an entry to
// HELP_TOPICS in help_content.cpp — HELP_TOPIC_COUNT is derived automatically.
extern const HelpTopic HELP_TOPICS[];
extern const int HELP_TOPIC_COUNT;

#endif // UI_MENUS_HELP_CONTENT_H
