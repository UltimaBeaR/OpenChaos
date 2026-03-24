#include "game/game_types.h"

#include "engine/audio/sound.h"
#include "engine/platform/uc_common.h"
#include "world/map/pap.h"
#include "world/environment/ns.h"
#include "ui/camera/fc.h"
#include "ui/camera/fc_globals.h"         // FC_cam
#include "engine/audio/mfx.h"
#include "things/core/statedef.h"
#include "world/environment/ware.h"
#include "world/environment/ware_globals.h" // WARE_ware
#include "ui/frontend.h"
#include "ui/frontend_globals.h"          // IsEnglish
#include "missions/eway.h"

// Internal height category used by PlayAmbient3D.
// uc_orig: HeightType (fallen/Source/Sound.cpp)
enum HeightType {
    PlayerHeight = 0,
    OnGround,
    InAir
};

// uc_orig: init_ambient (fallen/Source/Sound.cpp)
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

// Places a sound at a random angle around the player at a fixed distance.
// HeightType controls the Y position: PlayerHeight=player Y, OnGround=Y=0, InAir=above player.
// uc_orig: PlayAmbient3D (fallen/Source/Sound.cpp)
static void PlayAmbient3D(SLONG channel, SLONG wave_id, SLONG flags, HeightType height = PlayerHeight)
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
}

// uc_orig: SND_BeginAmbient (fallen/Source/Sound.cpp)
void SND_BeginAmbient()
{
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
}

// Fires infrequent random ambient events (sirens, thunder, birds) each game tick when outdoors.
// Countdown timers control event frequency: siren_time, thunder_time.
// uc_orig: new_outdoors_effects (fallen/Source/Sound.cpp)
static void new_outdoors_effects()
{
    SLONG dx, wave_id;

    siren_time--;
    if (siren_time < 0) {
        wave_id = -1;
        siren_time = 5000;

        switch (wtype) {
        case Jungle:
            break;

        case Snow:
            wave_id = S_AMB_WOLF1;
            siren_time = 1500 + ((Random() & 0xFFFF) >> 5);
            break;

        case Estate:
            wave_id = S_AMB_AIRCRAFT1;
            siren_time = 500 + ((Random() & 0xFFFF) >> 5);
            break;

        case BusyCity:
            wave_id = SOUND_Range(S_SIREN_START, S_SIREN_END);
            siren_time = 300 + ((Random() & 0xFFFF) >> 5);
            break;

        case QuietCity:
            break;
        }

        if (wave_id != -1)
            PlayAmbient3D(SIREN_REF, wave_id, 0, InAir);
    }

    // Thunder strikes during rain.
    if (GAME_FLAGS & GF_RAINING) {
        thunder_time--;
        if (thunder_time < 0) {
            dx = Random() & 2047;
            dx = SIN(dx) >> 8;

            thunder_time = 300 + ((Random() & 0xFFFF) >> 5);
            wave_id = S_THUNDER_START + (Random() % 4);

            PlayAmbient3D(THUNDER_REF, wave_id, MFX_REPLACE, InAir);
        }
    }

    static SLONG jungle_sounds[] = { S_BIRDCALL, S_COCKATOO_START, S_COCKATOO_END, S_CRICKET, S_BIRD_END };
    static SLONG snow_sounds[] = { S_CROW, S_AMB_WOLF1, S_CROW, S_AMB_WOLF2 };

    // Birdsong and jungle/snow creature sounds fire randomly when outside.
    if ((wtype == Estate) || (wtype == Jungle) || (wtype == Snow)) {
        if ((Random() & 0x1F00) == 0x1F00) {
            if (wtype == Estate)
                wave_id = S_BIRD_START + (Random() % 6);
            else if (wtype == Jungle)
                wave_id = jungle_sounds[Random() % 5];
            else if (wtype == Snow)
                wave_id = snow_sounds[Random() & 3];

            PlayAmbient3D(AMBIENT_EFFECT_REF, wave_id, MFX_QUEUED, InAir);
        }
    }

    // City ambient: dogs, cats, breaking glass.
    static SLONG city_animals[] = { S_DOG_START, S_DOG_START + 1, S_DOG_END,
        S_CAT_START, S_CAT_START + 1, S_CAT_START + 2, S_CAT_END };
    if (wtype == BusyCity) {
        if ((Random() & 0x1FC0) == 0x1FC0) {
            if (Random() & 48)
                wave_id = city_animals[Random() % 7];
            else
                wave_id = SOUND_Range(S_CITYMISC_START, S_CITYMISC_END);

            PlayAmbient3D(AMBIENT_EFFECT_REF, wave_id, 0, OnGround);
        }
    }

    // Foghorn for the jungle map (player on the dock side).
    if ((wtype == Jungle) && (NET_PERSON(PLAYER_ID)->WorldPos.X > 0x400000)) {
        if ((Random() & 0x1FC0) == 0x1FC0) {
            SLONG volume = (NET_PERSON(PLAYER_ID)->WorldPos.X - 0x400000) >> 14;
            MFX_play_ambient(AMBIENT_EFFECT_REF, S_FOGHORN, 0);
            MFX_set_gain(AMBIENT_EFFECT_REF, S_FOGHORN, volume);
        }
    }
}

// Crossfades indoor/outdoor volume layers each game tick.
// Indoors: ramps indoors_vol toward 255, outdoors_vol toward 0, weather_vol toward 128.
// Outdoors: reverses the crossfade and fires random ambient events.
// uc_orig: process_ambient_effects (fallen/Source/Sound.cpp)
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
}

// Handles rain/wind volume based on height, indoor building ambience, and player death sound.
// Called every game tick. was_in_or_out tracks indoor/outdoor transition to restart looping sounds.
// uc_orig: process_weather (fallen/Source/Sound.cpp)
void process_weather(void)
{
    SLONG x = NET_PERSON(PLAYER_ID)->WorldPos.X,
          y = NET_PERSON(PLAYER_ID)->WorldPos.Y,
          z = NET_PERSON(PLAYER_ID)->WorldPos.Z;

    // Maps WARE ambience field (1-based) to indoor looping sound IDs.
    // uc_orig: indoors_waves (fallen/Source/Sound.cpp)
    static SLONG indoors_waves[] = { S_AMB_POLICE1, S_AMB_POSHEETA, S_AMB_OFFICE1, S_TUNE_CLUB_START };

    // Play death sound once when player enters STATE_DEAD.
    static bool only_once = false;
    if (NET_PERSON(0)->State == STATE_DEAD) {
        if (!only_once)
            MFX_play_thing(THING_INDEX(NET_PERSON(0)), S_HEART_FAIL, 0, NET_PERSON(0));
        only_once = true;
    } else {
        only_once = false;
    }

    Thing* p_player = NET_PERSON(PLAYER_ID);
    static UBYTE was_in_or_out = 0; // 0=unknown, 1=indoors, 2=outdoors

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

        // Rain gain attenuates with height above ground (~16 units of height per gain step).
        SLONG rain_gain = 255 - (above_ground >> 4);
        if (rain_gain < 0)
            rain_gain = 0;
        else if (rain_gain > 255)
            rain_gain = 255;

        // Wind gain increases linearly with absolute height.
        SLONG wind_gain = abs_height >> 4;
        if (wind_gain < 0)
            wind_gain = 0;
        else if (wind_gain > 255)
            wind_gain = 255;

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
                wind_gain = 127;
            }

            if (wind_wave != -1) {
                MFX_play_ambient(WIND_REF, wind_wave, 0);
                MFX_set_gain(WIND_REF, wind_wave, wind_gain);
            }
        }
    } else {
        SLONG amb;

        if (was_in_or_out != 1) {
            MFX_stop(WEATHER_REF, MFX_WAVE_ALL);
            MFX_stop(WIND_REF, MFX_WAVE_ALL);
            was_in_or_out = 1;

            amb = WARE_ware[p_player->Genus.Person->Ware].ambience;
            if (amb == 4) {
                MFX_play_ambient(WEATHER_REF, S_TUNE_CLUB_START, MFX_LOOPED);
            } else if (amb)
                MFX_play_ambient(WEATHER_REF, indoors_waves[amb - 1], MFX_LOOPED);
        }
    }
}

// Stops all audio and resets ambient state. Called on level load/unload.
// uc_orig: SOUND_reset (fallen/Source/Sound.cpp)
void SOUND_reset(void)
{
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

// Returns gender code for voice line selection: 1=male, 2=female, 0=unknown.
// uc_orig: SOUND_Gender (fallen/Source/Sound.cpp)
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

// Plays an alert/uncertainty voice line for the NPC (English builds only).
// uc_orig: SOUND_Curious (fallen/Source/Sound.cpp)
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

// uc_orig: DieSound (fallen/Source/Sound.cpp)
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

// uc_orig: PainSound (fallen/Source/Sound.cpp)
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

// uc_orig: EffortSound (fallen/Source/Sound.cpp)
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

// uc_orig: MinorEffortSound (fallen/Source/Sound.cpp)
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

// uc_orig: ScreamFallSound (fallen/Source/Sound.cpp)
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

// uc_orig: StopScreamFallSound (fallen/Source/Sound.cpp)
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

// Parses an INI [Groups] section to build the SOUND_FXGroups table of [start, end] sample ranges.
// Uses Windows GetPrivateProfileSection to read the INI file.
// uc_orig: SOUND_InitFXGroups (fallen/Source/Sound.cpp)
void SOUND_InitFXGroups(CBYTE* fn)
{
    CBYTE* buff = new char[32768];
    CBYTE *pt, *split;
    CBYTE name[128], value[128];
    CBYTE index = 0;
    GetPrivateProfileSection("Groups", buff, 32767, fn);
    pt = buff;
    while (*pt) {
        split = strrchr(pt, '=');
        if (split) {
            *split = 0;
            strcpy(name, pt);
            pt = split + 1;
            strcpy(value, pt);
            // Skip 4 commas to reach the [start, end] values.
            split = strchr(value, ',') + 1;
            split = strchr(split, ',') + 1;
            split = strchr(split, ',') + 1;
            split = strchr(split, ',') + 1;
            strcpy(name, split);
            split = strchr(name, ',');
            *split = 0;
            split++;
            SOUND_FXGroups[index][0] = atoi(name);
            SOUND_FXGroups[index][1] = atoi(split);
            index++;
        }
        pt += strlen(pt) + 1;
    }

    delete[] buff;
}

// Plays a positioned sound relative to the camera. Used for ambient sound effects triggered from script.
// type=0: overlapping one-shot at XYZ. type=1: replaces current music channel at XYZ.
// uc_orig: play_ambient_wave (fallen/Source/Sound.cpp)
SLONG play_ambient_wave(SLONG sample, SLONG id, SLONG mode, SLONG range, UBYTE flags)
{
    SLONG x, y, z;

    x = FC_cam[0].x;
    y = FC_cam[0].y;
    z = FC_cam[0].z;

    MFX_play_xyz(id, sample, mode, x, y, z);
    return id;
}

// Triggers a sound from level script data. type=0: overlapping one-shot. type=1: replaces music channel.
// uc_orig: play_glue_wave (fallen/Source/Sound.cpp)
void play_glue_wave(UWORD type, UWORD id, SLONG x, SLONG y, SLONG z)
{
    switch (type) {
    case 0:
        MFX_play_xyz(0, id, MFX_OVERLAP, x, y, z);
        break;
    case 1:
        MFX_play_xyz(music_id, id, MFX_REPLACE, x, y, z);
        break;
    }
}

// Plays a music track using MSS32 paired-track crossfade system.
// track=1 plays on TRK1 (music_id-1), track=0 plays on TRK2 (music_id).
// uc_orig: play_music (fallen/Source/Sound.cpp)
void play_music(UWORD id, UBYTE track)
{
    SLONG flags;
    music_id = AMBIENT_EFFECT_REF + 2;
    flags = MFX_SHORT_QUEUE | MFX_QUEUED | MFX_EARLY_OUT;
    flags |= (track) ? MFX_PAIRED_TRK1 : MFX_PAIRED_TRK2;
    MFX_play_stereo(music_id - track, id, flags);
}

// Sewer waterfall sound precalculation — body commented out in original (sewer sounds were cut).
// uc_orig: SOUND_SewerPrecalc (fallen/Source/Sound.cpp)
void SOUND_SewerPrecalc()
{
    // Stub — sewer sound system was cut.
}

// uc_orig: SewerSoundProcess (fallen/Source/Sound.cpp)
void SewerSoundProcess()
{
    // Stub — sewer sound system was cut.
}
