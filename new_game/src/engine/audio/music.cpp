#include "engine/audio/music.h"
#include "engine/audio/music_globals.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"

#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "world/environment/ware.h"
#include "world/environment/ware_globals.h"
#include "engine/audio/sound.h"

// REAL_TICK_RATIO is defined in Thing.cpp; it mirrors TICK_RATIO but is frozen when game is paused.
extern SLONG REAL_TICK_RATIO;

// --- internal helpers ---

// uc_orig: MUSIC_play_the_mode (fallen/Source/music.cpp)
static void MUSIC_play_the_mode(UBYTE mode)
{
    just_asked_for_mode_now = UC_TRUE;
    just_asked_for_mode_number = mode;

    if (!mode)
        return;

    // Lookup table: [mode-1][0] = one-shot intro track, [1] = track range start, [2] = track range end.
    static SLONG lookup_table[14][3] = {
        { S_TUNE_DRIVING_START, S_TUNE_DRIVING, S_TUNE_DRIVING2 },
        { 0, S_TUNE_SPRINT, S_TUNE_SPRINT2 },
        { 0, S_TUNE_CRAWL, S_TUNE_CRAWL },
        { 0, S_TUNE_FIGHT, S_TUNE_FIGHT2 },
        { 0, S_TUNE_COMBAT_TRAINING, S_TUNE_COMBAT_TRAINING },
        { 0, S_TUNE_DRIVING_TRAINING, S_TUNE_DRIVING_TRAINING },
        { 0, S_TUNE_ASSAULT_TRAINING, S_TUNE_ASSAULT_TRAINING },
        { 0, S_TUNE_ASIAN_DUB, S_TUNE_ASIAN_DUB },
        { 0, S_TUNE_TIMER1, S_TUNE_TIMER1 },
        { 0, S_DEAD, S_DEAD },
        { 0, S_LEVEL_COMPLETE, S_LEVEL_COMPLETE },
        { 0, S_BRIEFING, S_BRIEFING },
        { 0, S_TUNE_FRONTEND, S_TUNE_FRONTEND },
        { 0, S_TUNE_CHAOS, S_TUNE_CHAOS },
    };

    // The timer tune plays standalone (no loop), so skip fade-in and repeats.
    if (mode == MUSIC_MODE_TIMER) {
        if (music_current_wave == S_TUNE_TIMER1)
            return;
        music_current_level = music_max_gain;
    }

    music_current_wave = 0;

    if (!music_current_level) // first play of this mode: use intro track if available
        music_current_wave = lookup_table[mode - 1][0];

    if (!music_current_wave) // no intro or not first play: pick random from range
        music_current_wave = SOUND_Range(lookup_table[mode - 1][1], lookup_table[mode - 1][2]);

    if (((mode == MUSIC_MODE_GAMELOST) || (mode == MUSIC_MODE_GAMEWON) || (mode == MUSIC_MODE_CHAOS)) && !music_current_level)
        MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);

    MFX_play_stereo(MUSIC_REF, music_current_wave, MFX_SHORT_QUEUE | MFX_QUEUED | MFX_EARLY_OUT | MFX_NEVER_OVERLAP);
    MFX_set_gain(MUSIC_REF, music_current_wave, music_current_level >> 8);
}

// uc_orig: MUSIC_set_the_volume (fallen/Source/music.cpp)
static void MUSIC_set_the_volume(SLONG vol)
{
    if (vol > music_max_gain)
        vol = music_max_gain;
    else if (vol < 0)
        vol = 0;

    music_current_level = vol;
    MFX_set_gain(MUSIC_REF, music_current_wave, vol >> 8);
}

// uc_orig: MUSIC_stop_the_mode (fallen/Source/music.cpp)
static void MUSIC_stop_the_mode()
{
    MFX_stop(MUSIC_REF, MFX_WAVE_ALL);
}

// --- public API ---

// uc_orig: MUSIC_reset (fallen/Source/music.cpp)
void MUSIC_reset()
{
    MUSIC_stop_the_mode();
    music_mode_override = music_request_mode = music_current_mode = music_current_level = 0;
}

// uc_orig: MUSIC_mode (fallen/Source/music.cpp)
void MUSIC_mode(UBYTE mode)
{
    if (music_mode_override > mode)
        mode = music_mode_override;
    else
        music_mode_override = 0;
    if ((mode > music_current_mode) || !mode)
        music_request_mode = mode & 127;
}

// uc_orig: MUSIC_mode_process (fallen/Source/music.cpp)
void MUSIC_mode_process()
{
    if (music_current_mode != music_request_mode) {
        if (!music_current_mode) {
            music_current_mode = music_request_mode;
            MUSIC_set_the_volume(0);
        } else {
            if (music_current_level)
                MUSIC_set_the_volume(music_current_level - (2 * REAL_TICK_RATIO));
            else {
                MUSIC_stop_the_mode();
                music_current_mode = 0;
            }
        }
    }

    // If the player is in a warehouse with custom ambience, override current mode.
    if (NETPERSON != NULL) {
        if (NET_PERSON(0) != NULL) {
            if (NET_PERSON(0)->Genus.Person->Ware) {
                SLONG amb = WARE_ware[NET_PERSON(0)->Genus.Person->Ware].ambience;
                if (amb) {
                    music_current_mode = 14 + amb;
                }
            }
        }
    }

    MUSIC_play_the_mode(music_current_mode);

    if (music_current_mode == music_request_mode) {
        if (music_current_level < music_max_gain)
            MUSIC_set_the_volume(music_current_level + (3 * REAL_TICK_RATIO));
    }
}

// uc_orig: MUSIC_gain (fallen/Source/music.cpp)
void MUSIC_gain(UBYTE gain)
{
    music_max_gain = gain;
    music_max_gain <<= 8;
    if (music_max_gain < music_current_level)
        MUSIC_set_the_volume(music_max_gain);
}

// uc_orig: MUSIC_is_playing (fallen/Source/music.cpp)
SLONG MUSIC_is_playing(void)
{
    SLONG MFX_QUICK_play_id = last_MFX_QUICK_play_id;

    if (MFX_QUICK_play_id == last_MFX_QUICK_play_id && MFX_QUICK_still_playing()) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}
