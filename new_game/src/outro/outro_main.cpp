#include "outro/outro_main.h"
#include "outro/outro_back.h"
#include "outro/core/outro_cam.h"
#include "outro/core/outro_cam_globals.h"
#include "outro/outro_credits.h"
#include "outro/core/outro_font.h"
#include "outro/outro_wire.h"
#include "outro/core/outro_key.h"
#include "outro/core/outro_os.h"

// uc_orig: MAIN_main (fallen/outro/outroMain.cpp)
void MAIN_main()
{
    // Preserve original extern declaration: the_end lives in Game.cpp (missions layer).
    extern UBYTE the_end;

    SLONG time1 = OS_ticks();

    // Show a loading or "THE END" screen while the rest of the outro initialises.
    FONT_init();
    OS_clear_screen();
    OS_scene_begin();

    {
        CBYTE* str;

        if (the_end) {
            str = "THE END";
        } else {
            str = "Loading...";
        }

        FONT_draw(FONT_FLAG_JUSTIFY_CENTRE, 0.5F, 0.4F, 0xffffff, 1.5F, -1, 0.0F, str);
    }

    OS_scene_end();
    OS_show();

    CAM_init();
    BACK_init();
    CREDITS_init();
    WIRE_init();

    // Keep "THE END" visible for at least 4 seconds before starting the sequence.
    if (the_end) {
        while (OS_ticks() < time1 + 4000)
            ;
    }

    OS_ticks_reset();
    OS_sound_loop_start();

    while (1) {
        OS_sound_loop_process();

        if (OS_process_messages() == OS_QUIT_GAME) {
            return;
        }

        if (KEY_on[KEY_ESCAPE]) {
            return;
        }

        CAM_process();

        OS_camera_set(
            CAM_x,
            CAM_y,
            CAM_z,
            256.0F,
            CAM_yaw,
            CAM_pitch,
            0.0F,
            CAM_lens,
            0.3F, 0.3F,
            1.0F, 1.0F);

        OS_scene_begin();

        BACK_draw();

        WIRE_draw();
        CREDITS_draw();

        if (the_end) {
            float shimmer = 0.0F;

            if (OS_ticks() > 4096) {
                shimmer = (OS_ticks() - 4096) * (1.0F / 4096.0F);
            }

            if (WITHIN(shimmer, 0.0F, 1.0F)) {
                FONT_draw(FONT_FLAG_JUSTIFY_CENTRE, 0.503F, 0.403F, 0x000000, 1.5F, -1, shimmer, "THE END");
                FONT_draw(FONT_FLAG_JUSTIFY_CENTRE, 0.500F, 0.400F, 0xffffff, 1.5F, -1, shimmer, "THE END");
            }
        }

        OS_scene_end();
        OS_show();
    }
}
