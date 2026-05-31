#include "ui/menus/help_content.h"
#include "ui/menus/help_content_bodies.h"

// Topic table: title + the three per-device bodies (defined in
// help_content_{kbm,xbox,ps}.cpp). The detail screen picks the body for the
// active device. To add a topic: declare its bodies in help_content_bodies.h,
// define them in all three device files, and add a row here.
const HelpTopic HELP_TOPICS[] = {
    { "MOVEMENT", HELP_MOVEMENT_KBM, HELP_MOVEMENT_XBOX, HELP_MOVEMENT_PS },
    { "COMBAT",   HELP_COMBAT_KBM,   HELP_COMBAT_XBOX,   HELP_COMBAT_PS },
    { "CLIMBING", HELP_CLIMBING_KBM, HELP_CLIMBING_XBOX, HELP_CLIMBING_PS },
    { "WEAPONS",  HELP_WEAPONS_KBM,  HELP_WEAPONS_XBOX,  HELP_WEAPONS_PS },

    // Dev tool — input-prompt catalog (device-aware list of every mapped glyph,
    // see input_prompt_map.cpp). Hidden from the list unless
    // OC_DEBUG_INPUT_PROMPT_CATALOG is on. Bodies ignored (input_test = true).
    // Must stay LAST (see help_content.h / GAMEMENU_help_list_count).
    { "INPUT TEST", nullptr, nullptr, nullptr, true },
};

const int HELP_TOPIC_COUNT = (int)(sizeof(HELP_TOPICS) / sizeof(HELP_TOPICS[0]));
