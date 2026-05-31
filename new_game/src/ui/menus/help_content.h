#ifndef UI_MENUS_HELP_CONTENT_H
#define UI_MENUS_HELP_CONTENT_H

// In-game "Controls / how-to-play" help content.
//
// Each topic has a short title (shown in the help list and as the detail-screen
// heading) and a body string drawn through the rich-text glyph drawer
// (input_glyph_text_draw) on the detail screen. Bodies may contain inline glyph
// tokens like {jump} / {use} which resolve to device-specific button glyphs.
//
// THIS INCREMENT: bodies are SHORT placeholders that fit one screen — no paging.
// Real content / paging slot in later without changing this interface.

struct HelpTopic {
    const char* title;
    const char* body;
};

// Topic table and its element count. Add a topic by appending an entry to
// HELP_TOPICS in help_content.cpp — HELP_TOPIC_COUNT is derived automatically.
extern const HelpTopic HELP_TOPICS[];
extern const int HELP_TOPIC_COUNT;

#endif // UI_MENUS_HELP_CONTENT_H
