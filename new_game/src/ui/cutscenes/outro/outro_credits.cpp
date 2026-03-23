#include "ui/cutscenes/outro/outro_credits.h"
#include "ui/cutscenes/outro/outro_credits_globals.h"
#include "ui/cutscenes/outro/outro_key.h"
#include "ui/cutscenes/outro/outro_os.h"
#include "ui/cutscenes/outro/outro_font.h"

// uc_orig: CREDITS_init (fallen/outro/credits.cpp)
void CREDITS_init()
{
    CREDITS_current_section = 0;
    CREDITS_current_y = 1.0F;
    CREDITS_now = 0;
    CREDITS_last = 0;
}

// uc_orig: CREDITS_draw (fallen/outro/credits.cpp)
void CREDITS_draw()
{
    SLONG i;
    SLONG dont_draw;
    SLONG flag;
    float x;
    float y;
    float shimmer;
    float scale;

    CREDITS_Section* cs;

    CREDITS_now = OS_ticks();

    // Never be more than 1/2 a second behind.
    if (CREDITS_last < CREDITS_now - 500) {
        CREDITS_last = CREDITS_now - 500;
    }

    // Speed key: S advances the scroll faster.
    if (KEY_on[KEY_S]) {
        CREDITS_last -= 200;
    }

    // Scroll upwards at a fixed rate.
    CREDITS_current_y -= (CREDITS_now - CREDITS_last) * 0.00005F;

    ASSERT(WITHIN(CREDITS_current_section, 0, CREDITS_NUM_SECTIONS - 1));

    cs = &CREDITS_section[CREDITS_current_section];

    // Compute shimmer: fade in at start and fade out at end.
    if (CREDITS_current_y > 0.6F) {
        shimmer = 1.0F - (1.0F - CREDITS_current_y) * (1.0F / 0.4F);
    } else if (CREDITS_current_end_y < 0.50F) {
        shimmer = 1.0F - (CREDITS_current_end_y - 0.10F) * (1.0F / 0.4F);
    }

    SATURATE(shimmer, 0.0F, 1.0F);

    // Draw the section title in the top-left corner.
    FONT_draw(
        FONT_FLAG_JUSTIFY_LEFT,
        0.03F, 0.05F,
        0xffffff,
        1.5F,
        -1,
        shimmer,
        cs->title);

    y = CREDITS_current_y;

    for (i = 0; cs->line[i] == NULL || cs->line[i][0] != '!'; i++) {
        flag = FONT_FLAG_JUSTIFY_LEFT;
        scale = 0.6F;
        dont_draw = UC_FALSE;

        if (cs->line[i] == NULL) {
            // Blank line — skip, still advance y.
            dont_draw = UC_TRUE;
        } else if (y < 0.10F) {
            // Below visible area — don't draw.
            dont_draw = UC_TRUE;
        } else if (y < 0.30F) {
            shimmer = 1.0F - (y - 0.10F) * (1.0F / 0.2F);
        } else if (y < 0.75F) {
            shimmer = 0.0F;
        } else if (y < 0.95F) {
            shimmer = (y - 0.75F) * (1.0F / 0.2F);
        } else {
            // Above visible area — don't draw.
            dont_draw = UC_TRUE;
        }

        if (cs->line[i]) {
            CBYTE* text = cs->line[i];

            // Check for style modifiers: ~B (bold/large) or ~I (italic).
            if (text[0] == '~') {
                switch (text[1]) {
                case 'B':
                    scale = 1.0F;
                    break;

                case 'I':
                    flag |= FONT_FLAG_ITALIC;
                    break;

                default:
                    ASSERT(0);
                    break;
                }

                text += 2;
            }

            // Handle tab indentation.
            x = 0.06F;

            while (*text == '\t') {
                text += 1;
                x += 0.03F;
            }

            if (!dont_draw) {
                FONT_draw(
                    flag,
                    x, y,
                    0xffffff,
                    scale,
                    -1,
                    shimmer,
                    text);
            }
        }

        y += 0.05F * scale;
    }

    CREDITS_current_end_y = y;

    // Advance to the next section when the current one has scrolled off screen.
    if (y < 0.10F) {
        CREDITS_current_section += 1;
        CREDITS_current_y = 1.0F;

        if (CREDITS_current_section >= CREDITS_NUM_SECTIONS) {
            CREDITS_current_section = 0;
        }
    }

    CREDITS_last = CREDITS_now;
}
