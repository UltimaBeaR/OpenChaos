#include "engine/audio/music.h"
#include "engine/audio/music_globals.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"

#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "buildings/ware.h"
#include "buildings/ware_globals.h"
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

    // Modes 1-14 are normal music modes with tracks in lookup_table below.
    // Mode > 14 means warehouse ambience override (set by MUSIC_mode_process as
    // 14 + amb). This was likely an unfinished attempt to play indoor music here —
    // MuckyFoot never extended lookup_table beyond 14 entries, and instead implemented
    // indoor sound through sound.cpp (MFX_play_ambient with S_TUNE_CLUB_START etc.).
    // The mode > 14 value is still useful: it makes music_current_mode != music_request_mode
    // which triggers fade-out of outdoor music, preventing it from clashing with the
    // indoor sound. Without this, both outdoor and indoor music would play simultaneously.
    // Original code had no bounds check here — on x86 it read past the table into
    // adjacent memory (harmless by luck). ASan catches this as global-buffer-overflow.
    if (mode > 14)
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

    // Override music mode when player is inside a warehouse with ambience.
    // Sets mode > 14 which suppresses normal music (MUSIC_play_the_mode returns
    // early for mode > 14). The actual indoor sound (club music etc.) is played
    // separately by sound.cpp via MFX_play_ambient.
    // This was likely an unfinished attempt to play indoor music through the music
    // system (lookup_table was never extended beyond 14 entries). MuckyFoot later
    // implemented indoor sound through sound.cpp but left this code in place.
    // It still serves a purpose: mode > 14 forces fade-out of outdoor music.
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
