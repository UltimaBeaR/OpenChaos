#include "ui/menus/help_content.h"

// Placeholder help topics. {jump} / {use} tokens are expanded to the active
// device's button glyph by the rich-text drawer. Bodies can be any length —
// the detail screen wraps them and lets the player scroll (whole lines only),
// so they no longer have to fit a single screen.
//
// To add a topic: append a { title, body } entry below. HELP_TOPIC_COUNT updates
// automatically from the array size.
const HelpTopic HELP_TOPICS[] = {
    { "MOVEMENT",
      "Move with the left stick. The harder you push, the faster Darci runs; "
      "ease off and she walks quietly, drawing far less attention from the "
      "people and patrols around her.\n"
      "\n"
      "Press {jump} to jump. Tap it while running to clear gaps, hop low "
      "railings, or spring upward to catch a ledge above you. Timing matters: "
      "jump a fraction too late and you will come up short.\n"
      "\n"
      "Hold the action button {use} to interact with whatever Darci is facing: "
      "open doors, throw switches, pick objects up, and pull yourself onto "
      "crates, walls, and ledges. The prompt only works when something usable "
      "is directly in front of you.\n"
      "\n"
      "Walk into a wall and Darci presses flat against it. From cover you can "
      "edge along narrow ledges, shuffle past long drops, and lean out to look "
      "around a corner before stepping into the open where you might be seen.\n"
      "\n"
      "Sprinting burns stamina, so you cannot run flat out forever — let it "
      "recover between bursts. Watch your footing near rooftops and water: a "
      "long fall will hurt or kill, and deep water will sweep you away if you "
      "drop into it carelessly.\n"
      "\n"
      "Put the moves together to cross the city fast and unseen: run, {jump} "
      "across a gap, grab the far ledge, haul yourself up with {use}, and keep "
      "moving before anyone realises you were ever there.\n"
      "\n"
      "Speed is not always your friend. A sprinting figure is heard from much "
      "further away than someone picking their way carefully through the dark, "
      "and a guard who hears running footsteps will turn to look long before "
      "they would have noticed a quiet walk. Slow down as you near anyone you "
      "would rather not meet.\n"
      "\n"
      "Crouching lowers Darci's profile and muffles her steps even more. Move "
      "from shadow to shadow, pause behind cover, and time your dashes for the "
      "moments when a patrol is looking the other way. Patience usually beats "
      "raw speed when the alternative is a fight you cannot win.\n"
      "\n"
      "Height is an advantage. Rooftops, gantries, and walkways let you cross "
      "ground that is crawling with trouble below, and they open up approaches "
      "that a street-level route never would. Look up as often as you look "
      "ahead — the way forward is not always on the ground.\n"
      "\n"
      "To climb, line Darci up with the edge and press {jump} as you reach it. "
      "She will catch the lip and hang; push the stick up to haul herself over, "
      "or let go to drop back down. Hanging costs strength, so do not dangle "
      "longer than you have to.\n"
      "\n"
      "Coming down is its own skill. Lower yourself over an edge with {use} to "
      "shorten the drop instead of leaping blindly, and aim for soft landings "
      "where you can. Two short descents are almost always safer than one long "
      "plunge that leaves you limping.\n"
      "\n"
      "Water can be a shortcut or a trap. A calm canal will hide you and carry "
      "you quietly past trouble, but a strong current drags you off course and "
      "you cannot swim forever. Know where you mean to climb out before you "
      "ever get in.\n"
      "\n"
      "Above all, keep moving with a plan. Read the ground ahead, pick the line "
      "that keeps you out of sight, and string your jumps, climbs, and dashes "
      "together so you are never caught standing still in the open with nowhere "
      "left to go." },
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
