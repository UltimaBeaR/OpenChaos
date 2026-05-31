#include "ui/menus/help_content.h"

// Placeholder help topics. Bodies are intentionally short so they fit a single
// detail screen (no paging this increment). {jump} / {use} tokens are expanded
// to the active device's button glyph by input_glyph_text_draw.
//
// To add a topic: append a { title, body } entry below. HELP_TOPIC_COUNT updates
// automatically from the array size.
const HelpTopic HELP_TOPICS[] = {
    { "MOVEMENT",
      "Move with the left stick. Press {jump} to jump.\n"
      "Hold the action button {use} to interact with the world." },
    { "COMBAT",
      "Face an enemy and attack to fight.\n"
      "Use {use} for special moves. Keep moving to avoid hits." },
    { "CLIMBING",
      "Approach a ledge and press {jump} to pull yourself up.\n"
      "Drop down by walking off an edge." },
    { "WEAPONS",
      "Pick up weapons found in the level.\n"
      "Press {use} to fire or use the held item." },
};

const int HELP_TOPIC_COUNT = (int)(sizeof(HELP_TOPICS) / sizeof(HELP_TOPICS[0]));
