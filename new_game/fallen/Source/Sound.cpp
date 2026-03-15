//	Sound.cpp
//	Guy Simmons, 8th May 1998.

// claude-ai: SOUND SYSTEM OVERVIEW
// claude-ai: All audio goes through the MFX layer (mfx.h / mfx.cpp), which wraps Miles Sound System (MSS32).
// claude-ai: MSS32 = Miles Sound System by RAD Game Tools — proprietary, Windows-only, NOT portable.
// claude-ai: Every MFX_* call (MFX_play_xyz, MFX_play_ambient, MFX_play_thing, MFX_set_gain, MFX_stop, etc.)
// claude-ai: is an MSS32 API call — replace entirely with miniaudio or SDL_mixer in the new version.
// claude-ai:
// claude-ai: KEY AUDIO CHANNELS (channel IDs defined in Sound.h / mfx.h):
// claude-ai:   AMBIENT_EFFECT_REF  — looping ambient background sounds (rain, tropical, etc.)
// claude-ai:   SIREN_REF           — infrequent distant events (sirens, aircraft, wolves)
// claude-ai:   THUNDER_REF         — thunder strikes during rain
// claude-ai:   WEATHER_REF         — continuous weather layer (rain, wind, indoor ambience)
// claude-ai:   WIND_REF            — wind sounds (city, snow biomes)
// claude-ai:   music_id            — dynamic music channel (AMBIENT_EFFECT_REF+2)
// claude-ai:
// claude-ai: MUSIC STATE MACHINE (music_id / play_music / next_music):
// claude-ai:   Music modes: MUSIC_MODE_NONE, MUSIC_MODE_NORMAL, MUSIC_MODE_TENSION,
// claude-ai:                MUSIC_MODE_PURSUIT, MUSIC_MODE_VICTORY, MUSIC_MODE_DEFEAT
// claude-ai:   Transitions driven by game state — see process_weather() and play_music().
// claude-ai:   Tracks use MFX_PAIRED_TRK1/TRK2 flags for seamless crossfading between two stereo tracks.
// claude-ai:
// claude-ai: 3D POSITIONAL AUDIO:
// claude-ai:   MFX_play_xyz(channel, wave_id, flags, x, y, z) — positions sound in world space.
// claude-ai:   MFX_play_thing(channel, wave_id, flags, p_thing) — attaches sound to a game Thing.
// claude-ai:   Distances in the same fixed-point units as world coordinates (>>8 = world units).
// claude-ai:   Volume falloff and Doppler handled by MSS32 internally — must reimplement in new version.
// claude-ai:
// claude-ai: VOICE/DIALOGUE: Talk2/ directory, language-controlled by IsEnglish flag.
// claude-ai:
// claude-ai: PORTING STRATEGY:
// claude-ai:   - KEEP: trigger logic, 3D position calculations, music state machine, ambient scheduling
// claude-ai:   - REPLACE: all MFX_* calls → miniaudio or SDL_mixer equivalents
// claude-ai:   - DISCARD: PSX / TARGET_DC conditional blocks entirely

#include "Game.h"

#include "cnet.h"
#include "Sound.h"
#include "MFStdLib.h"
#include "pap.h"
#include "ns.h"
#include "fc.h"
#include "mfx.h"
#include "statedef.h"
#include "ware.h"
#include "frontend.h"
#include "eway.h"
#include "DCLowLevel.h"

// types of world

// claude-ai: WorldType maps TEXTURE_SET numbers to biome categories, controlling ambient sound selection.
// claude-ai: TEXTURE_SET values: 1=Jungle, 5=Snow, 10=Estate, 16=QuietCity, default=BusyCity.
enum WorldType {
    Jungle = 0,
    Snow,
    Estate,
    BusyCity,
    QuietCity
};

WorldType wtype;

enum HeightType {
    PlayerHeight = 0,
    OnGround,
    InAir
};

//---------------------------------------------------------------
// Maps textures to soundfx
#ifndef PSX
UBYTE* SOUND_FXMapping; //[1024];
// UWORD *SOUND_FXGroups;//[128][2]; // "128 groups should be enough for anybody"(TM)
SOUNDFXG* SOUND_FXGroups; //[128][2]; // because its a 2d array we need to fudge the system into thingink its 2d still using typedef
#else
/*
UBYTE SOUND_FXMapping[512];
UWORD SOUND_FXGroups[8][2];	// "8 should be more than enough for the PSX"
*/
#endif

//---------------------------------------------------------------
/*
extern SLONG	CAM_cur_x,
                                CAM_cur_y,
                                CAM_cur_z,
                                CAM_cur_yaw;
*/
//---------------------------------------------------------------

// claude-ai: Countdown timers (in game ticks) controlling how often random ambient events fire.
// claude-ai: siren_time starts at 300 ticks (~5 seconds at 60fps), resets to 1500-5000 after each event.
// claude-ai: thunder_time resets to 300+ ticks between strikes (only when GF_RAINING flag is set).
SLONG creature_time = 400,
      siren_time = 300,
      in_sewer_time = 0,
      thunder_time = 0;

SLONG music_id = 0;
SWORD world_type = WORLD_TYPE_CITY_POP;

void SewerSoundProcess();

static SLONG wind_id = 0, tick_tock = 0;

// claude-ai: Volume levels for ambient layers: 0=silent, 255=full. Crossfaded each tick in process_ambient_effects().
// claude-ai: indoors_vol/outdoors_vol crossfade based on GF_INDOORS flag. weather_vol fades to 128 indoors.
static SLONG indoors_id = 0, outdoors_id = 0, rain_id = 0, rain_id2 = 0, thunder_id = 0, indoors_vol = 0, outdoors_vol = 255, weather_vol = 255, music_vol = 255, next_music = 0;

#ifndef PSX
void init_ambient(void)
{
    indoors_id = 0, outdoors_id = 0, rain_id = 0, rain_id2 = 0, thunder_id = 0, indoors_vol = 0, outdoors_vol = 255, weather_vol = 255, music_vol = 255, next_music = 0;
    wind_id = 0, tick_tock = 0;

    creature_time = 400,
    siren_time = 300,
    in_sewer_time = 0,
    thunder_time = 0;

    music_id = 0;
}

// PlayAmbient3D
//
// play an ambient sound at a random 3D position

// claude-ai: Places a sound at a random angle around the player, at a fixed distance.
// claude-ai: angle is in PSX angle units (0-2047 = full circle). COS/SIN return 16-bit fixed-point values.
// claude-ai: COS(angle) << 4 shifts the value 4 bits left — effectively scales the offset radius.
// claude-ai: HeightType: PlayerHeight=same Y as player, OnGround=Y=0, InAir=player Y + 512-1535 units upward.
// claude-ai: MFX_play_xyz — MSS32 API — replace entirely with miniaudio or SDL_mixer
void PlayAmbient3D(SLONG channel, SLONG wave_id, SLONG flags, HeightType height = PlayerHeight)
{
    Thing* p_player = NET_PERSON(PLAYER_ID);

    SLONG angle = Random() & 2047;

    SLONG x = p_player->WorldPos.X + (COS(angle) << 4);
    SLONG y = p_player->WorldPos.Y;
    SLONG z = p_player->WorldPos.Z + (SIN(angle) << 4);

    if (height == OnGround)
        y = 0;
    else if (height == InAir)
        y += (512 + (Random() & 1023)) << 8;

    MFX_play_xyz(channel, wave_id, 0, x, y, z);

    // For DC sound system...

    // MFX_play_ambient(0,wave_id,0);
}

// SND_BeginAmbient
//
// begin ambient sounds

// claude-ai: Called once when a level starts. Sets wtype based on TEXTURE_SET and starts the base ambient loop.
// claude-ai: Only Jungle (TEXTURE_SET=1) starts a looping ambient here — other biomes rely on process_weather().
// claude-ai: MFX_play_ambient — MSS32 API — replace entirely with miniaudio or SDL_mixer
void SND_BeginAmbient()
{
    // map across from the texture set to the world type
    switch (TEXTURE_SET) {
    case 1:
        wtype = Jungle;
        MFX_play_ambient(AMBIENT_EFFECT_REF, S_TROPICAL, MFX_LOOPED);
        break;

    case 5:
        wtype = Snow;
        break;

    case 10:
        wtype = Estate;
        break;

    case 16:
        wtype = QuietCity;
        break;

    default:
        wtype = BusyCity;
        break;
    }

#ifdef _DEBUG
    static char* desc[] = { "Jungle", "Snow", "Estate", "Busy City", "Quiet City" };
    TRACE("WORLD TYPE IS \"%s\"\n", desc[wtype]);
#endif
}

// claude-ai: new_outdoors_effects — called every game tick when player is outdoors.
// claude-ai: Manages three categories of infrequent random ambient events:
// claude-ai:   1. SIREN_REF: biome-specific distant events (sirens/aircraft/wolves) via siren_time countdown
// claude-ai:   2. THUNDER_REF: thunder during rain (GF_RAINING) via thunder_time countdown
// claude-ai:   3. AMBIENT_EFFECT_REF: frequent bird/animal/city sounds triggered by random bitmask checks
// claude-ai: All three use PlayAmbient3D to position sounds in 3D space around the player.
void new_outdoors_effects()
{
#ifndef PSX
    SLONG c0, dx, dy, dz, wave_id;

    // make "siren" effects
    // these are infrequent effects e.g. airplanes, sirens etc.
    siren_time--;
    if (siren_time < 0) {
        wave_id = -1;
        siren_time = 5000;

        switch (wtype) {
        case Jungle:
            break;

        case Snow:
            // this one goes on forever, so it's made extra rare
            wave_id = S_AMB_WOLF1;
            siren_time = 1500 + ((Random() & 0xFFFF) >> 5);
            break;

        case Estate:
            wave_id = S_AMB_AIRCRAFT1;
            siren_time = 500 + ((Random() & 0xFFFF) >> 5);
            break;

        case BusyCity:
            //			wave_id = S_SIREN_START + (Random() % 4);
            wave_id = SOUND_Range(S_SIREN_START, S_SIREN_END);
            siren_time = 300 + ((Random() & 0xFFFF) >> 5);
            break;

        case QuietCity:
            break;
        }

        if (wave_id != -1)
            PlayAmbient3D(SIREN_REF, wave_id, 0, InAir);
    }

    // make thunder
    if (GAME_FLAGS & GF_RAINING) {
        thunder_time--;
        if (thunder_time < 0) {
            dx = Random() & 2047;
            dz = COS(dx) >> 8;
            dx = SIN(dx) >> 8;

            thunder_time = 300 + ((Random() & 0xFFFF) >> 5);
            wave_id = S_THUNDER_START + (Random() % 4);

            PlayAmbient3D(THUNDER_REF, wave_id, MFX_REPLACE, InAir);
        }
    }

    static SLONG jungle_sounds[] = { S_BIRDCALL, S_COCKATOO_START, S_COCKATOO_END, S_CRICKET, S_BIRD_END };
    static SLONG snow_sounds[] = { S_CROW, S_AMB_WOLF1, S_CROW, S_AMB_WOLF2 };

    // play birdsong/junglesong randomly and frequently
    if ((wtype == Estate) || (wtype == Jungle) || (wtype == Snow)) {
        if ((Random() & 0x1F00) == 0x1F00) {
            if (wtype == Estate)
                wave_id = S_BIRD_START + (Random() % 6);
            else if (wtype == Jungle)
                wave_id = jungle_sounds[Random() % 5];
            else if (wtype == Snow)
                wave_id = snow_sounds[Random() & 3];

#ifndef TARGET_DC
            TRACE("playing amb sound: %d\n", wave_id);
#endif

            //			PlayAmbient3D(AMBIENT_EFFECT_REF, wave_id, (wtype == Snow) ? MFX_QUEUED : 0, InAir);
            PlayAmbient3D(AMBIENT_EFFECT_REF, wave_id, MFX_QUEUED, InAir);
        }
    }

    //	Chuck in some cats, dogs, breaking glass
    static SLONG city_animals[] = { S_DOG_START, S_DOG_START + 1, S_DOG_END,
        S_CAT_START, S_CAT_START + 1, S_CAT_START + 2, S_CAT_END };
    if (wtype == BusyCity) {
        if ((Random() & 0x1FC0) == 0x1FC0) {
            if (Random() & 48)
                wave_id = city_animals[Random() % 7]; // 3 in 4
            else
                wave_id = SOUND_Range(S_CITYMISC_START, S_CITYMISC_END);

            PlayAmbient3D(AMBIENT_EFFECT_REF, wave_id, 0, OnGround);
        }
    }

    // foghorn for the jungle map
    if ((wtype == Jungle) && (NET_PERSON(PLAYER_ID)->WorldPos.X > 0x400000)) {
        if ((Random() & 0x1FC0) == 0x1FC0) {
            SLONG volume = (NET_PERSON(PLAYER_ID)->WorldPos.X - 0x400000) >> 14;
            MFX_play_ambient(AMBIENT_EFFECT_REF, S_FOGHORN, 0);
            MFX_set_gain(AMBIENT_EFFECT_REF, S_FOGHORN, volume);
        }
    }
#endif
}

// claude-ai: process_ambient_effects — called every game tick. Crossfades indoor/outdoor volume layers.
// claude-ai: Indoors: ramps indoors_vol toward 255, outdoors_vol toward 0, weather_vol toward 128 (half).
// claude-ai: Outdoors: reverses the crossfade, then fires new_outdoors_effects() for random ambient events.
// claude-ai: Volume ramps are 1 unit/tick = slow ~4-second crossfade at 60fps (255 ticks).
// claude-ai: NOTE: the A3DVolume calls below are commented out — MSS32 3D volume was never wired up here.
void process_ambient_effects(void)
{

    if (GAME_FLAGS & GF_INDOORS) {
        if (indoors_vol < 255)
            indoors_vol++;
        if (outdoors_vol > 0)
            outdoors_vol--;
        if (weather_vol > 128)
            weather_vol--;
    } else {
        if (indoors_vol > 0)
            indoors_vol--;
        if (outdoors_vol < 255)
            outdoors_vol++;
        if (weather_vol < 255)
            weather_vol++;
        new_outdoors_effects();
    }
    /*	A3DVolume(indoors_id,indoors_vol);
            A3DVolume(outdoors_id,outdoors_vol);
            A3DVolume(rain_id,weather_vol);
            A3DVolume(thunder_id,weather_vol);*/
}

// claude-ai: process_weather — called every game tick. Handles:
// claude-ai:   - Player death sound trigger (one-shot via only_once flag)
// claude-ai:   - Outdoor rain/wind: volume based on height above ground (rain louder near ground, wind louder up high)
// claude-ai:   - Indoor ambience: plays building-specific looping sounds from WARE ambience table
// claude-ai:   - was_in_or_out tracks indoor/outdoor state to stop/start loops on transition
// claude-ai: Rain gain formula: 255 - (above_ground >> 4) — attenuates ~16 units of height per gain step.
// claude-ai: Wind gain formula: abs_height >> 4 — increases linearly with absolute height.
// claude-ai: All MFX_* calls here — MSS32 API — replace entirely with miniaudio or SDL_mixer
void process_weather(void)
{
#ifdef PSX
    return;
#else

    SLONG x = NET_PERSON(PLAYER_ID)->WorldPos.X,
          y = NET_PERSON(PLAYER_ID)->WorldPos.Y,
          z = NET_PERSON(PLAYER_ID)->WorldPos.Z;

    // these map GEDIT settings to sound headers
    // claude-ai: indoors_waves: index 0=police station, 1=upscale shop, 2=office, 3=nightclub (index from WARE ambience field, 1-based)
    static SLONG indoors_waves[] = { S_AMB_POLICE1, S_AMB_POSHEETA, S_AMB_OFFICE1, S_TUNE_CLUB_START };

    // handle death sounds - in process_weather(), yeah right!
    static bool only_once = false;
    if (NET_PERSON(0)->State == STATE_DEAD) {
        if (!only_once)
            MFX_play_thing(THING_INDEX(NET_PERSON(0)), S_HEART_FAIL, 0, NET_PERSON(0));
        only_once = true;
    } else {
        only_once = false;
    }

    // do rain and wind
    Thing* p_player = NET_PERSON(PLAYER_ID);
    static UBYTE was_in_or_out = 0; // 0 is unknown, 1 is in, 2 is out

    if (!p_player->Genus.Person->Ware) {
        SLONG ground = PAP_calc_map_height_at(x >> 8, z >> 8);

        if (ground == -32767)
            ground = p_player->WorldPos.Y;
        else
            ground <<= 8;

        SLONG above_ground = (y - ground) >> 8;
        SLONG abs_height = y >> 8;

        if (was_in_or_out != 2) {
            MFX_stop(WEATHER_REF, MFX_WAVE_ALL);
            was_in_or_out = 2;
            tick_tock = 0;
        }

        //		TRACE("ABOVE %X ABS %X\n", above_ground, abs_height);

        SLONG rain_gain = 255 - (above_ground >> 4);
        if (rain_gain < 0)
            rain_gain = 0;
        else if (rain_gain > 255)
            rain_gain = 255;

        SLONG wind_gain = abs_height >> 4;
        if (wind_gain < 0)
            wind_gain = 0;
        else if (wind_gain > 255)
            wind_gain = 255;

        //		TRACE("Rain %d Wind %d\n", rain_gain, wind_gain);

        if (GAME_FLAGS & GF_RAINING) {
            if ((GAME_TURN & 0x3f) == 0)
                MFX_play_ambient(WEATHER_REF, S_RAIN_START, MFX_LOOPED);
            MFX_set_gain(WEATHER_REF, S_RAIN_START, rain_gain);
        } else if ((wtype == BusyCity) || (wtype == QuietCity)) {
            if ((GAME_TURN & 0x3f) == 0)
                MFX_play_ambient(WEATHER_REF, S_AMBIENCE_END, MFX_LOOPED);
            MFX_set_gain(WEATHER_REF, S_AMBIENCE_END, rain_gain);
        }

        SLONG wind_wave = -1;

        if (GAME_TURN - tick_tock >= 25) {

            if ((wtype == BusyCity) || (wtype == QuietCity)) {
                tick_tock = GAME_TURN + 30 + (Random() & 63);
                wind_wave = S_WIND_START + (Random() % 7);
            } else if (wtype == Snow) {
                tick_tock = GAME_TURN + (Random() & 15);
                wind_wave = S_START_SNOW_WIND + (Random() % 5);
                //				wind_gain = 255;
                wind_gain = 127;
            }

            if (wind_wave != -1) {
                MFX_play_ambient(WIND_REF, wind_wave, 0);
                MFX_set_gain(WIND_REF, wind_wave, wind_gain);
                //				MFX_play_ambient(WIND_REF,wind_wave,MFX_QUEUED);
                //				MFX_set_queue_gain(WIND_REF,wind_wave,wind_gain);
            }
        }
    } else {
        SLONG amb;

        if (was_in_or_out != 1) {
            MFX_stop(WEATHER_REF, MFX_WAVE_ALL);
            MFX_stop(WIND_REF, MFX_WAVE_ALL);
            was_in_or_out = 1;

            //
            // This must be done by the music system for the DC!
            //

#ifndef TARGET_DC

            amb = WARE_ware[p_player->Genus.Person->Ware].ambience;
            if (amb == 4) {
                //				if (Random()&1)
                MFX_play_ambient(WEATHER_REF, S_TUNE_CLUB_START, MFX_LOOPED);
                // no Binary Finary *sniff*
                /*				else
                                                        MFX_play_ambient(WEATHER_REF,S_TUNE_CLUB_END,MFX_LOOPED);*/
            } else if (amb)
                MFX_play_ambient(WEATHER_REF, indoors_waves[amb - 1], MFX_LOOPED);

#endif
        }
    }
#endif // PSX
}

// claude-ai: SOUND_reset — stops all playing audio and resets all ambient state. Called on level load/unload.
// claude-ai: MFX_CHANNEL_ALL + MFX_WAVE_ALL = stop everything on every channel.
// claude-ai: MFX_stop — MSS32 API — replace entirely with miniaudio or SDL_mixer
void SOUND_reset(void)
{

    // free up any playing sources
    // A3DStopAll();
    MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);

    indoors_id = 0;
    outdoors_id = 0;
    rain_id = 0;
    rain_id2 = 0;
    thunder_id = 0;
    indoors_vol = 0;
    outdoors_vol = 255, weather_vol = 255;
    music_id = 0;
    next_music = 0;
    music_vol = 255;
}
#endif

//---------------------------------------------------------------
//						Version independent
//---------------------------------------------------------------
/*
inline SLONG SOUND_Range(SLONG start, SLONG end) {
        SLONG diff=(end-start)+1;

        return start+(rand()%diff);
}
*/

// claude-ai: SOUND_Gender — returns gender code for voice line selection: 1=male, 2=female, 0=unknown/NPC.
// claude-ai: Used to pick gender-appropriate voice samples from Talk2/ directory.
UBYTE SOUND_Gender(Thing* p_thing)
{
    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_DARCI:
    case PERSON_SLAG_TART:
    case PERSON_SLAG_FATUGLY:
    case PERSON_HOSTAGE:
        return 2;
    case PERSON_ROPER:
    case PERSON_COP:
    case PERSON_THUG_RASTA:
    case PERSON_THUG_GREY:
    case PERSON_THUG_RED:
    case PERSON_MECHANIC:
    case PERSON_TRAMP:
    case PERSON_MIB1:
    case PERSON_MIB2:
    case PERSON_MIB3:
        return 1;
    case PERSON_CIV:
        if (p_thing->Draw.Tweened->MeshID == 9)
            return 2;
        return 1;
    default:
        return 0;
    }
}

// claude-ai: SOUND_Curious — plays an "uncertain/alert" voice line for the given NPC type.
// claude-ai: Only fires for English language builds (IsEnglish flag). Localised builds have no NPC curiosity VO.
// claude-ai: MFX_play_thing — MSS32 API — replace entirely with miniaudio or SDL_mixer
void SOUND_Curious(Thing* p_thing)
{
    SWORD snd_a, snd_b;

    if (!IsEnglish)
        return;

    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_THUG_RASTA:
        snd_a = S_BTHUG_UNCERTAIN_START;
        snd_b = S_BTHUG_UNCERTAIN_END;
        break;
    case PERSON_THUG_GREY:
        snd_a = S_WTHUG1_UNCERTAIN_START;
        snd_b = S_WTHUG1_UNCERTAIN_END;
        break;
    case PERSON_THUG_RED:
        snd_a = S_WTHUG2_UNCERTAIN_START;
        snd_b = S_WTHUG2_UNCERTAIN_END;
        break;
    default:
        return;
    }
    snd_a = SOUND_Range(snd_a, snd_b);
    MFX_play_thing(THING_NUMBER(p_thing), snd_a, 0, p_thing);
}

// claude-ai: DieSound / PainSound / EffortSound / MinorEffortSound / ScreamFallSound / StopScreamFallSound:
// claude-ai: Character-specific voice reactions triggered by game events. Each picks a random sample in a range.
// claude-ai: MFX_QUEUED|MFX_SHORT_QUEUE: queues the sound but discards queue if it gets too long (short queue).
// claude-ai: MFX_play_thing — MSS32 API — replace entirely with miniaudio or SDL_mixer
void DieSound(Thing* p_thing)
{
    SWORD hit_a, hit_b;
    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_DARCI:
        hit_a = S_DARCI_HIT_START;
        hit_b = S_DARCI_HIT_END;
        break;
    case PERSON_ROPER:
        hit_a = S_ROPER_HIT_START;
        hit_b = S_ROPER_HIT_END;
        break;
    case PERSON_COP:
        hit_a = S_COP_DIE_START;
        hit_b = S_COP_DIE_END;
        break;
    default:
        return;
    }
    hit_a = SOUND_Range(hit_a, hit_b);
    MFX_play_thing(THING_NUMBER(p_thing), hit_a, MFX_QUEUED | MFX_SHORT_QUEUE, p_thing);
}

void PainSound(Thing* p_thing)
{
    SWORD hit_a, hit_b;

    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_DARCI:
        hit_a = S_DARCI_HIT_START;
        hit_b = S_DARCI_HIT_END;
        break;
    case PERSON_ROPER:
        hit_a = S_ROPER_HIT_START;
        hit_b = S_ROPER_HIT_END;
        break;
    case PERSON_COP:
        hit_a = S_COP_PAIN_START;
        if (Random() & 3)
            hit_b = hit_a + 1;
        else
            hit_b = S_COP_PAIN_END;
        break;
    case PERSON_THUG_RASTA:
        hit_a = S_RASTA_PAIN_START;
        if (Random() & 3)
            hit_b = hit_a + 1;
        else
            hit_b = S_RASTA_PAIN_END;
        break;
    case PERSON_THUG_GREY:
        hit_a = S_GGREY_PAIN_START;
        if (Random() & 3)
            hit_b = hit_a + 1;
        else
            hit_b = S_GGREY_PAIN_END;
        break;
    case PERSON_THUG_RED:
        hit_a = S_GRED_PAIN_START;
        hit_b = S_GRED_PAIN_END;
        break;
    case PERSON_HOSTAGE:
        hit_a = S_HOSTAGE_PAIN_START;
        hit_b = S_HOSTAGE_PAIN_END;
        break;
    case PERSON_MECHANIC:
        hit_a = S_WORKMAN_PAIN_START;
        hit_b = S_WORKMAN_PAIN_END;
        break;
    case PERSON_TRAMP:
        hit_a = S_TRAMP_PAIN_START;
        if (Random() & 3)
            hit_b = hit_a + 1;
        else
            hit_b = S_TRAMP_PAIN_END;
        break;
    default:
        return;
    }
    hit_a = SOUND_Range(hit_a, hit_b);
    MFX_play_thing(THING_NUMBER(p_thing), hit_a, MFX_QUEUED | MFX_SHORT_QUEUE, p_thing);
}

void EffortSound(Thing* p_thing)
{
    SWORD snd_a, snd_b;
    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_DARCI:
        snd_a = S_DARCI_EFFORT_START;
        snd_b = S_DARCI_EFFORT_END;
        break;
    case PERSON_ROPER:
        snd_a = S_ROPER_EFFORT_START;
        snd_b = S_ROPER_EFFORT_END;
        break;
    default:
        return;
    }
    snd_a = SOUND_Range(snd_a, snd_b);
    MFX_play_thing(THING_NUMBER(p_thing), snd_a, 0, p_thing);
}

void MinorEffortSound(Thing* p_thing)
{
    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_DARCI:
        MFX_play_thing(THING_NUMBER(p_thing), S_DARCI_SHORT_EFFORT, 0, p_thing);
        break;
    case PERSON_ROPER:
        MFX_play_thing(THING_NUMBER(p_thing), S_ROPER_SHORT_EFFORT, 0, p_thing);
        break;
    }
}

void ScreamFallSound(Thing* p_thing)
{
    SWORD snd_a, snd_b;
    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_DARCI:
        snd_a = S_DARCI_SCREAM_FALL_START;
        snd_b = S_DARCI_SCREAM_FALL_END;
        break;
    case PERSON_ROPER:
    case PERSON_CIV:
        snd_a = S_ROPER_SCREAM_FALL_START;
        snd_b = S_ROPER_SCREAM_FALL_END;
        break;
    default:
        return;
    }
    snd_a = SOUND_Range(snd_a, snd_b);
    MFX_play_thing(THING_NUMBER(p_thing), snd_a, 0, p_thing);
}

void StopScreamFallSound(Thing* p_thing)
{
    SWORD snd_a, snd_b, snd;
    switch (p_thing->Genus.Person->PersonType) {
    case PERSON_DARCI:
        snd_a = S_DARCI_SCREAM_FALL_START;
        snd_b = S_DARCI_SCREAM_FALL_END;
        break;
    case PERSON_ROPER:
    case PERSON_CIV:
        snd_a = S_ROPER_SCREAM_FALL_START;
        snd_b = S_ROPER_SCREAM_FALL_END;
        break;
    default:
        return;
    }
    for (snd = snd_a; snd <= snd_b; snd++) {
        MFX_stop(THING_NUMBER(p_thing), snd);
    }
}

// claude-ai: SOUND_InitFXGroups — parses an INI file [Groups] section to populate SOUND_FXGroups table.
// claude-ai: Each group defines a [start, end] range of sound IDs for random selection via SOUND_Range().
// claude-ai: Uses Windows GetPrivateProfileSection (winapi) — PC/non-PSX only. Not needed on PSX (static 8 groups).
// claude-ai: KEEP this logic in new version (sound group ranges needed). REPLACE GetPrivateProfileSection with custom INI parser.
void SOUND_InitFXGroups(CBYTE* fn)
{
#ifndef PSX
#ifndef TARGET_DC
    CBYTE* buff = new char[32768];
    CBYTE *pt, *split;
    CBYTE name[128], value[128];
    CBYTE index = 0;
    GetPrivateProfileSection("Groups", buff, 32767, fn);
    pt = buff;
    while (*pt) {
        // for some reason sscanf snarled up :(
        split = strrchr(pt, '=');
        if (split) {
            *split = 0;
            strcpy(name, pt); // ok, so we throw it away...
            pt = split + 1;
            strcpy(value, pt);
            //	    TRACE("ini file %s : %s",name,value);
            split = strchr(value, ',') + 1; // extra skips...
            split = strchr(split, ',') + 1;
            split = strchr(split, ',') + 1; // extra skips for the -1's...
            split = strchr(split, ',') + 1;
            strcpy(name, split);
            split = strchr(name, ',');
            *split = 0;
            split++;
            // ok, so we now have the low # in name and hi # in split
            SOUND_FXGroups[index][0] = atoi(name);
            SOUND_FXGroups[index][1] = atoi(split);
            TRACE("Group %d goes from %d to %d\n", index, SOUND_FXGroups[index][0], SOUND_FXGroups[index][1]);
            index++;
        }
        pt += strlen(pt) + 1; // +1 to skip the 0 terminator
    }

    delete[] buff;
#endif
#endif
}

/*
SLONG	play_quick_wave_old(WaveParams *wave,SLONG sample,SLONG id,SLONG mode)
{
        return play_quick_wave_xyz(wave->Mode.Cartesian.X,wave->Mode.Cartesian.Y,wave->Mode.Cartesian.Z,sample,id,mode);
}
*/
#ifndef PSX
// claude-ai: play_ambient_wave — plays a sound offset from the camera position.
// claude-ai: flags&1: randomise angle ±128 units around 896 (roughly "ahead and to the side")
// claude-ai: flags==0: angle=1024 (exactly behind camera — 1024/2048 = 180° in PSX angle units)
// claude-ai: ang+=FC_cam[0].yaw applies camera rotation so "behind" is relative to camera direction.
// claude-ai: dx/dz: direction vector computed from angle using SIN/COS lookup tables (PSX integer math).
// claude-ai: range!=256: scale dx/dz by range/256 — range=256 means ~1 world unit, larger=further away.
// claude-ai: MFX_play_xyz — MSS32 API — replace entirely with miniaudio or SDL_mixer
SLONG play_ambient_wave(SLONG sample, SLONG id, SLONG mode, SLONG range, UBYTE flags)
{
    SLONG x, y, z, dx, dy, dz, ang;

    if (flags & 1)
        ang = 896 + (Random() & 0xff);
    else
        ang = 1024;
    //	ang+=CAM_cur_yaw;
    ang += FC_cam[0].yaw;
    ang &= 2047;

    dx = (SIN(ang) / 256);
    dz = (COS(ang) / 256);
    if (range != 256) {
        dx *= range;
        dz *= range;
        dx /= 256;
        dz /= 256;
    }

    //	x=CAM_cur_x; y=CAM_cur_y; z=CAM_cur_z;
    x = FC_cam[0].x;
    y = FC_cam[0].y;
    z = FC_cam[0].z;

    MFX_play_xyz(id, sample, mode, x, y, z);
    return id;
    //	return play_quick_wave_xyz(x+dx,y,z+dz,sample,id,mode);
}
#endif

// claude-ai: play_glue_wave — called from script/level data to trigger positioned sounds.
// claude-ai: type=0: overlapping one-shot on channel 0 (multiple instances allowed via MFX_OVERLAP)
// claude-ai: type=1: replaces current music_id channel sound at the given XYZ position (MFX_REPLACE)
// claude-ai: Called "glue" because it bridges the scripting/level system to the audio system.
// claude-ai: MFX_play_xyz — MSS32 API — replace entirely with miniaudio or SDL_mixer
void play_glue_wave(UWORD type, UWORD id, SLONG x, SLONG y, SLONG z)
{
    switch (type) {
    case 0:
        //		play_quick_wave_xyz(x,y,z,id,0,0);
        MFX_play_xyz(0, id, MFX_OVERLAP, x, y, z);
        break;
    case 1:
        //		music_id=play_quick_wave_xyz(FC_cam[0].x,FC_cam[0].y,FC_cam[0].z,id,music_id,0);
        MFX_play_xyz(music_id, id, MFX_REPLACE, x, y, z);
        break;
    }
}

// claude-ai: play_music — MUSIC STATE MACHINE entry point. Plays a music track on the paired stereo system.
// claude-ai: music_id is fixed at AMBIENT_EFFECT_REF+2. Two channels: (music_id) and (music_id-1).
// claude-ai: track=1: plays on MFX_PAIRED_TRK1 (channel music_id-1); track=0: plays on MFX_PAIRED_TRK2 (channel music_id).
// claude-ai: MFX_PAIRED_TRK1/TRK2: MSS32 paired-track system — two channels that crossfade between each other.
// claude-ai: MFX_EARLY_OUT: if another sound is already queued, skip this one rather than waiting.
// claude-ai: This function is called from the game state machine when MUSIC_MODE_* changes.
// claude-ai: KEEP: the two-channel crossfade concept. REPLACE: MFX_play_stereo with miniaudio or SDL_mixer.
// claude-ai: MFX_play_stereo — MSS32 API — replace entirely with miniaudio or SDL_mixer
void play_music(UWORD id, UBYTE track)
{
    SLONG flags;
    music_id = AMBIENT_EFFECT_REF + 2;
    //	flags=(looped) ? MFX_LOOPED : 0;
    flags = MFX_SHORT_QUEUE | MFX_QUEUED | MFX_EARLY_OUT;
    flags |= (track) ? MFX_PAIRED_TRK1 : MFX_PAIRED_TRK2;
    MFX_play_stereo(music_id - track, id, flags);
    //	MFX_set_gain(music_id-track,id,128);
}

/*
void	NewFreeWaveList() {
#ifdef USE_A3D
        A3DFreeWaveList();
#else
        FreeWaveList();
#endif
}

void	NewLoadWaveFile(CBYTE *name) {
#ifdef USE_A3D
        A3DLoadWaveFile(name);
#else
extern	void	LoadWave(CBYTE *wave_name);

        LoadWave(name);
#endif
}

void	NewLoadWaveList(CBYTE *names[]) {
  SLONG i;
  CBYTE buff[_MAX_PATH];
#ifndef PSX
  if (names==0) names=sound_list;

  NewFreeWaveList();
  i=0;
  while (strcmp(names[i],"!")) {

//	  if (stricmp("C:\\fallen\\Data\\sfx\\1622\\NULL.wav",names[i]))
          if (stricmp("NULL.wav",names[i])) {
                strcpy(buff,"Data\\sfx\\1622\\");
                strcat(buff,names[i]);
            NewLoadWaveFile(buff);
          }
          i++;
  }
#endif
}
*/

#if !defined(PSX) && !defined(TARGET_DC)

// THE SEWERS ARE DEAD, LONG LIVE THE SEWERS

// inline SLONG SewerHeight(NS_Hi *nh) { return (nh->bot << 5) + (-32 * 0x100); }

// #define SEWER_SOUND_MAX	20

// GameCoord SewerSounds[SEWER_SOUND_MAX];

void SOUND_SewerPrecalc()
{
    /*	NS_Hi *ns;
            SBYTE temp_map[PAP_SIZE_HI][PAP_SIZE_HI];
            SLONG x,y,sx,sy;
            SLONG h,item,ctr,debugctr,debug2;

            for (item=0;item<10;item++)
                    SewerSounds[item].X=SewerSounds[item].Y=SewerSounds[item].Z=0;

            memset(temp_map,0,PAP_SIZE_HI*PAP_SIZE_HI);

            // mark waterfalls
            debugctr=0;
            for (y=1;y<PAP_SIZE_HI-1;y++)
              for (x=1;x<PAP_SIZE_HI-1;x++)
              {
                    ns=&NS_hi[x][y];
                    h=SewerHeight(ns);
                    temp_map[x][y]=-((ns->water>0)&&
                                         (!(ns->packed&NS_HI_TYPE_ROCK))&&
                                         (!(ns->packed&NS_HI_TYPE_CURVE))&&
                                                     ( (h>SewerHeight(&NS_hi[x+1][y]))||
                                                     (h>SewerHeight(&NS_hi[x-1][y]))||
                                                     (h>SewerHeight(&NS_hi[x][y+1]))||
                                                     (h>SewerHeight(&NS_hi[x][y-1]))
                                                    ));
                    if (temp_map[x][y]) debugctr++;

              }

            TRACE("waterfall ctr: %d\n",debugctr);

            // create source entries
            ctr=0; debugctr=debug2=0;
            for (y=1;y<PAP_SIZE_HI-1;y++)
                    for (x=1;x<PAP_SIZE_HI-1;x++)
                            if (temp_map[x][y]) {
                                    debug2++;
                                    item=0;
                                    for (sx=-1;sx<2;sx++) {
                                            for (sy=-1;sy<2;sy++) {
                                                    if (sx||sy) {
                                                            if (temp_map[x+sx][y+sy]>0) item=temp_map[x+sx][y+sy];
                                                    }
                                            }
                                    }
                                    if (item) {
                                            temp_map[x][y]=item;
                                            SewerSounds[item-1].X+=x << (8+PAP_SHIFT_HI);
                                            SewerSounds[item-1].Z+=y << (8+PAP_SHIFT_HI);
                                            SewerSounds[item-1].X>>=1;
                                            SewerSounds[item-1].Z>>=1;
                                            debugctr++;
                                    } else {
                                            if (ctr<=SEWER_SOUND_MAX) {
                                                    SewerSounds[ctr].X = x << (8+PAP_SHIFT_HI);
                                                    SewerSounds[ctr].Z = y << (8+PAP_SHIFT_HI);
                                                    ctr++;
                                                    temp_map[x][y]=ctr;
                                            } else TRACE("ran out of waterfall sound sources\n");
                                    }
                      }
            TRACE("waterfall checked: %d  reused: %d\n",debug2,debugctr);
            for (item=0;item<ctr;item++) {
    //		SewerSounds[item].Y = NS_calc_splash_height_at(SewerSounds[item].X>>8,SewerSounds[item].Z>>8);
                    SewerSounds[item].Y = -1280 << 8;
            }
            */
}

void SewerSoundProcess()
{
    /*	static SLONG id = 0;
            SLONG i,d,w,d2,dx,dz;

            w=-1; d=-1;
            for (i=0;i<10;i++)
                    if (SewerSounds[i].X || SewerSounds[i].Y || SewerSounds[i].Z) {
                            dx=(SewerSounds[i].X-NET_PERSON(PLAYER_ID)->WorldPos.X)>>8;
                            dz=(SewerSounds[i].Z-NET_PERSON(PLAYER_ID)->WorldPos.Z)>>8;
                            d2=SDIST2(dx,dz);
                            if ((d2<d)||(d==-1)) { d=d2; w=i; }
                    }
            if (w>=0) {
                    id=play_quick_wave_xyz(SewerSounds[w].X,SewerSounds[w].Y,SewerSounds[w].Z,S_WATERFALL,id,WAVE_LOOP|WAVE_PLAY_NO_INTERRUPT);

            } else {

            }*/
}

#endif
