//
// music controller thingy
// matthew rosenfeld
// 14 june 99
//

#include "music.h"
#include "sound.h"
#include "mfx.h"
#include "ware.h"

UWORD music_current_wave = 0;
UWORD music_max_gain = 128 << 8;
/*
UBYTE music_current_gain=0;
UBYTE music_fademode=0;
UBYTE music_queue_flags;
UWORD music_queue_wave;

#define MUSIC_IDLE			0
#define MUSIC_FADING_IN		1
#define MUSIC_FADING_OUT	2
#define	MUSIC_FADING_ACROSS	3

#define LOOSE_TRANSITION (MUSIC_FLAG_QUEUED|MUSIC_FLAG_FADE_OUT)

// play or queue a piece according to flags
// return value is a MUSIC_WAS flag
UBYTE MUSIC_play(UWORD wave, UBYTE flags) {
        if ((wave!=music_current_wave)&&(flags&MUSIC_FLAG_OVERRIDE)) MUSIC_stop(0);
        //TRACE("%d MUSIC_play w%d f%d\n",GAME_TURN,wave,flags);
        if ((flags&LOOSE_TRANSITION)==LOOSE_TRANSITION) {
                music_queue_wave=wave;
                music_queue_flags=flags^MUSIC_FLAG_QUEUED;
                music_fademode=MUSIC_FADING_ACROSS;
                //TRACE("%d FADED\n",GAME_TURN);
                return MUSIC_WAS_FADED;
        } else {
                SLONG mfxflags;
                UBYTE res;
                mfxflags=MFX_SHORT_QUEUE|MFX_NEVER_OVERLAP;
                mfxflags|=(flags&MUSIC_FLAG_LOOPED)?MFX_LOOPED:MFX_EARLY_OUT;
                mfxflags|=(flags&MUSIC_FLAG_QUEUED)?MFX_QUEUED:0;
                res=MFX_play_stereo(MUSIC_REF,wave,mfxflags);
                //TRACE("result: %d\n",res);
                if (flags&MUSIC_FLAG_FADE_IN) {
                        music_fademode=MUSIC_FADING_IN;
                        music_current_gain=0;
                } else {
                        music_fademode=MUSIC_IDLE;
                        music_current_gain=music_max_gain;
                }
                if (!(mfxflags&MFX_QUEUED)) {
                        MFX_set_gain(MUSIC_REF,wave,music_current_gain);
                        //TRACE("%d INSTA_GAIN %d\n",GAME_TURN,music_current_gain);
                }
                else {
                        //TRACE("%d QUEUE GAIN %d\n",GAME_TURN,music_max_gain);
                        MFX_set_queue_gain(MUSIC_REF,wave,music_max_gain);
                }
                music_current_wave=wave;
                return res; // matches
        }
        //TRACE("%d FUCKED\n",GAME_TURN);
        return MUSIC_WAS_FUCKED;
}

// stop current piece playing, optionally fading out
void  MUSIC_stop(BOOL fade) {
        if (fade) {
                music_fademode=MUSIC_FADING_OUT;
        } else {
                MFX_stop(MUSIC_REF,MFX_WAVE_ALL);
                music_current_gain=0;
                music_fademode=0;
                music_current_wave=0;
        }
}


// find out the currently playing wave
UWORD MUSIC_wave(void) {
        return MFX_get_wave(MUSIC_REF);
}

// call this each game loop to keep things fading in and out nicely and stuff
void  MUSIC_process() {
        switch(music_fademode) {
        case MUSIC_FADING_IN:
                if (music_current_gain<music_max_gain) music_current_gain++;
                break;
        case MUSIC_FADING_OUT:
        case MUSIC_FADING_ACROSS:
                if (music_current_gain>0)
                        music_current_gain--;
                else {
                        MFX_stop(MUSIC_REF,music_current_wave);
                        if (music_fademode==MUSIC_FADING_ACROSS)
                                MUSIC_play(music_queue_wave,music_queue_flags);
                }
                break;
        case MUSIC_IDLE:
                return;
        }
        //TRACE("%d process-loop set-gain %d\n",GAME_TURN, music_current_gain);
        MFX_set_gain(MUSIC_REF,music_current_wave,music_current_gain);
//	ASSERT(music_current_gain<=40);
//	ASSERT(music_max_gain<=40);
}
*/

/**********************************************************************************
 *
 *      Experimental new music system that hopefully doesn't suck
 */

UBYTE music_current_mode = 0;
UBYTE music_request_mode = 0;
SLONG music_current_level = 0;
UBYTE music_mode_override = 0;

UBYTE MUSIC_bodge_code = 0;

UBYTE just_asked_for_mode_now;
UBYTE just_asked_for_mode_number;

SLONG last_MFX_QUICK_play_id;
SLONG last_MFX_QUICK_mode;

// ---- assorted 'system level' utilities ----
float music_volume;

void MUSIC_play_the_mode(UBYTE mode)
{
    just_asked_for_mode_now = TRUE;
    just_asked_for_mode_number = mode;

    if (!mode)
        return;

    // for the PC, do an MFX_play with a random constant within the appropriate range to mode
    // for the PSX, queue up the appropriate track if it's not playing already

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

    // the timer tune is designed 'standalone' not looping, so no fade in and no repeats.
    if (mode == MUSIC_MODE_TIMER) {
        if (music_current_wave == S_TUNE_TIMER1)
            return;
        music_current_level = music_max_gain;
    }

    music_current_wave = 0;

    if (!music_current_level) // this is the first play of the mode
        music_current_wave = lookup_table[mode - 1][0];

    if (!music_current_wave) // there wasn't a special one-off and/or this isn't the first play
        music_current_wave = SOUND_Range(lookup_table[mode - 1][1], lookup_table[mode - 1][2]);

    if (((mode == MUSIC_MODE_GAMELOST) || (mode == MUSIC_MODE_GAMEWON) || (mode == MUSIC_MODE_CHAOS)) && !music_current_level)
        MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);

    MFX_play_stereo(MUSIC_REF, music_current_wave, MFX_SHORT_QUEUE | MFX_QUEUED | MFX_EARLY_OUT | MFX_NEVER_OVERLAP);
    MFX_set_gain(MUSIC_REF, music_current_wave, music_current_level >> 8);
}

void MUSIC_set_the_volume(SLONG vol)
{
    if (vol > music_max_gain)
        vol = music_max_gain;
    else if (vol < 0)
        vol = 0;

    music_current_level = vol;

    // for the PC, use MFX_set_gain
    // for the PSX, set the CDaudio playback volume

    MFX_set_gain(MUSIC_REF, music_current_wave, vol >> 8);
}

void MUSIC_stop_the_mode()
{
    // for the PC, use MFX_stop to halt wave playback
    MFX_stop(MUSIC_REF, MFX_WAVE_ALL);
}

// ---- called by the outside world to clear everything between missions ----

void MUSIC_reset()
{
    MUSIC_stop_the_mode();
    music_mode_override = music_request_mode = music_current_mode = music_current_level = 0;
}

// ---- called by the outside world to sync up music ----

void MUSIC_mode(UBYTE mode)
{
    if (music_mode_override > mode)
        mode = music_mode_override;
    else
        music_mode_override = 0; // hah!
    if ((mode > music_current_mode) || !mode)
        music_request_mode = mode & 127;
}

// ---- called by the outside world to keep it all running ----

extern SLONG REAL_TICK_RATIO;

void MUSIC_mode_process()
{

    // we can be in silence, and want to stay that way.

    // we can be playing, and want to stay that way.

    // we can be stopping or changing, either way we want to fade out current piece while still looping it.

    // we can be starting or changing (pt2), either way we want to fade in new piece while still looping it.
    if (music_current_mode != music_request_mode) { // we need to do something about it.
        if (!music_current_mode) { // we're currently idle
            music_current_mode = music_request_mode;
            MUSIC_set_the_volume(0); // fade-in handler takes it from here
        } else {
            if (music_current_level) // we're playing audible stuff
                MUSIC_set_the_volume(music_current_level - (2 * REAL_TICK_RATIO)); // so fade it out
            else { // we're silent but think we're playing stuff
                MUSIC_stop_the_mode();
                music_current_mode = 0; // the handler above takes it from here
            }
        }
    }

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

    if (music_current_mode == music_request_mode) // we are happy
    {
        // if the current piece hasn't faded in yet...
        if (music_current_level < music_max_gain)
            MUSIC_set_the_volume(music_current_level + (3 * REAL_TICK_RATIO));
    }
}

// this is the 'max' gain, fade in/out will go from/to 0 from/to this value
void MUSIC_gain(UBYTE gain)
{
    music_max_gain = gain;
    music_max_gain <<= 8;
    if (music_max_gain < music_current_level)
        MUSIC_set_the_volume(music_max_gain);
}

SLONG MUSIC_is_playing(void)
{
    SLONG MFX_QUICK_play_id = last_MFX_QUICK_play_id;

    if (MFX_QUICK_play_id == last_MFX_QUICK_play_id && MFX_QUICK_still_playing()) {
        return TRUE;
    } else {
        return FALSE;
    }
}
