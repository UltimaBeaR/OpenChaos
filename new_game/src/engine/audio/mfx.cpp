// uc_orig: TALK_3D (fallen/DDLibrary/Source/MFX.cpp)
#define TALK_3D 0

#include "engine/audio/mfx_globals.h"
#include "engine/core/math.h"
#include <algorithm>
#include <cmath>
#include <AL/al.h>
#include <AL/alext.h>
#include "engine/platform/sdl3_bridge.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "engine/io/env.h"
#include "engine/io/file.h"
#include "assets/sound_id.h"

// uc_orig: GetFullName (fallen/DDLibrary/Source/MFX.cpp)
static char* GetFullName(char* fname);
// uc_orig: SetLinearScale (fallen/DDLibrary/Source/MFX.cpp)
static void SetLinearScale(SLONG wave, float linscale);
// uc_orig: SetPower (fallen/DDLibrary/Source/MFX.cpp)
static void SetPower(SLONG wave, float dB);
// uc_orig: InitVoices (fallen/DDLibrary/Source/MFX.cpp)
static void InitVoices();
// uc_orig: LoadWaveFile (fallen/DDLibrary/Source/MFX.cpp)
static void LoadWaveFile(MFX_Sample* sptr);
// uc_orig: LoadTalkFile (fallen/DDLibrary/Source/MFX.cpp)
static void LoadTalkFile(char* filename);
// uc_orig: UnloadWaveFile (fallen/DDLibrary/Source/MFX.cpp)
static void UnloadWaveFile(MFX_Sample* sptr);
// uc_orig: UnloadTalkFile (fallen/DDLibrary/Source/MFX.cpp)
static void UnloadTalkFile();
// uc_orig: FinishLoading (fallen/DDLibrary/Source/MFX.cpp)
static void FinishLoading(MFX_Voice* vptr);
// uc_orig: PlayVoice (fallen/DDLibrary/Source/MFX.cpp)
static void PlayVoice(MFX_Voice* vptr);
// uc_orig: MoveVoice (fallen/DDLibrary/Source/MFX.cpp)
static void MoveVoice(MFX_Voice* vptr);
// uc_orig: SetVoiceRate (fallen/DDLibrary/Source/MFX.cpp)
static void SetVoiceRate(MFX_Voice* vptr, float mult);
// uc_orig: SetVoiceGain (fallen/DDLibrary/Source/MFX.cpp)
static void SetVoiceGain(MFX_Voice* vptr, float gain);

extern CBYTE* sound_list[];

// uc_orig: MFX_init (fallen/DDLibrary/Source/MFX.cpp)
void MFX_init()
{
    alDevice = alcOpenDevice(nullptr);
    alContext = alcCreateContext(alDevice, nullptr);
    alcMakeContextCurrent(alContext);

    InitVoices();

    LRU.prev_lru = LRU.next_lru = &LRU;
    AllocatedRAM = 0;

    NumSamples = 0;
    CBYTE** names = sound_list;
    MFX_Sample* sptr = Samples;

    while (strcmp(names[0], "!")) {
        sptr->prev_lru = NULL;
        sptr->next_lru = NULL;
        sptr->fname = NULL;
        sptr->handle = 0;
        sptr->is3D = true;
        sptr->size = 0;
        sptr->type = SMP_Effect;
        sptr->linscale = 1.0;
        sptr->loading = false;

        if (oc_stricmp("null.wav", names[0])) {
            FILE* fd = MF_Fopen(GetFullName(names[0]), "rb");
            if (fd) {
                sptr->fname = names[0];
                if (((!oc_strnicmp(names[0], "music", 5)) || (!oc_strnicmp(names[0], "generalmusic", 12))) && (!strstr(names[0], "Club1")) && (!strstr(names[0], "Acid"))) {
                    sptr->is3D = false;
                    sptr->type = SMP_Music;
                }

                fseek(fd, 0, SEEK_END);
                sptr->size = ftell(fd);
                MF_Fclose(fd);
            } else {
            }
        }

        NumSamples++;

        if (sptr->fname) {
            int gain = INI_get_int("data/sfx/powerlvl.ini", "PowerLevels", sptr->fname, 0);
            if (gain) {
                gain *= 4;
                SetPower(sptr - Samples, float(gain));
            }
        }

        sptr++;
        names++;
    }

    sptr = &TalkSample;

    sptr->prev_lru = NULL;
    sptr->next_lru = NULL;
    sptr->fname = NULL;
    sptr->handle = 0;
    sptr->is3D = TALK_3D ? true : false;
    sptr->size = 0;
    sptr->type = SMP_Effect;
    sptr->linscale = 1.0;
    sptr->loading = false;

    Volumes[SMP_Ambient] = float(ENV_get_value_number("ambient_volume", 127, "Audio")) / 127.0f;
    Volumes[SMP_Music] = float(ENV_get_value_number("music_volume", 127, "Audio")) / 127.0f;
    Volumes[SMP_Effect] = float(ENV_get_value_number("fx_volume", 127, "Audio")) / 127.0f;
}

// uc_orig: MFX_term (fallen/DDLibrary/Source/MFX.cpp)
void MFX_term()
{
    MFX_free_wave_list();

    for (int ii = 0; ii < NumSamples; ii++) {
        UnloadWaveFile(&Samples[ii]);
    }
    NumSamples = 0;

    UnloadTalkFile();

    alcMakeContextCurrent(NULL);
    alcDestroyContext(alContext);
    alcCloseDevice(alDevice);
}

// uc_orig: GetFullName (fallen/DDLibrary/Source/MFX.cpp)
static char* GetFullName(char* fname)
{
    CBYTE buf[MAX_PATH];
    static CBYTE pathname[MAX_PATH];

    if (strchr(fname, '-')) {
        CHAR* ptr = strrchr(fname, '\\');
        if (!ptr) ptr = strrchr(fname, '/');
        ptr = ptr + 1;
        sprintf(buf, "talk2/misc/%s", ptr);
        strcpy(pathname, "./");
        strcat(pathname, buf);
        return pathname;
    } else {
        sprintf(buf, "data/sfx/1622/%s", fname);
        if (!oc_strnicmp(fname, "music", 5)) {
            if (!MUSIC_WORLD) {
                MUSIC_WORLD = 1;
            }
            buf[19] = '0' + (MUSIC_WORLD / 10);
            buf[20] = '0' + (MUSIC_WORLD % 10);
        }
    }

    strcpy(pathname, "./");
    strcat(pathname, buf);

    return pathname;
}

// uc_orig: SetLinearScale (fallen/DDLibrary/Source/MFX.cpp)
static void SetLinearScale(SLONG wave, float linscale)
{
    if ((wave >= 0) && (wave < NumSamples)) {
        Samples[wave].linscale = linscale;
    }
}

// uc_orig: SetPower (fallen/DDLibrary/Source/MFX.cpp)
static void SetPower(SLONG wave, float dB)
{
    SetLinearScale(wave, float(exp(log(10) * dB / 20)));
}

// uc_orig: InitVoices (fallen/DDLibrary/Source/MFX.cpp)
static void InitVoices()
{
    int ii;

    for (ii = 0; ii < MAX_VOICE; ii++) {
        Voices[ii].id = 0;
        Voices[ii].wave = 0;
        Voices[ii].flags = 0;
        Voices[ii].x = 0;
        Voices[ii].y = 0;
        Voices[ii].z = 0;
        Voices[ii].thing = NULL;
        Voices[ii].queue = NULL;
        Voices[ii].queuesz = 0;
        Voices[ii].smp = NULL;
        Voices[ii].handle = NULL;
        Voices[ii].is3D = false;
    }

    for (ii = 0; ii < MAX_QWAVE; ii++) {
        QWaves[ii].next = &QWaves[ii + 1];
    }
    QWaves[ii - 1].next = NULL;
    QFree = &QWaves[0];
    QFreeLast = &QWaves[ii - 1];

    LX = LY = LZ = 0;
}

// uc_orig: Hash (fallen/DDLibrary/Source/MFX.cpp)
static int Hash(UWORD channel_id)
{
    return (channel_id * 37) & VOICE_MSK;
}

// uc_orig: FindVoice (fallen/DDLibrary/Source/MFX.cpp)
static MFX_Voice* FindVoice(UWORD channel_id, ULONG wave)
{
    int offset = Hash(channel_id);

    for (int ii = 0; ii < MAX_VOICE; ii++) {
        int vn = (ii + offset) & VOICE_MSK;
        if ((Voices[vn].id == channel_id) && (Voices[vn].wave == wave)) {
            return &Voices[vn];
        }
    }

    return NULL;
}

// uc_orig: FindFirst (fallen/DDLibrary/Source/MFX.cpp)
static MFX_Voice* FindFirst(UWORD channel_id)
{
    int offset = Hash(channel_id);

    for (int ii = 0; ii < MAX_VOICE; ii++) {
        int vn = (ii + offset) & VOICE_MSK;
        if (Voices[vn].id == channel_id) {
            return &Voices[vn];
        }
    }

    return NULL;
}

// uc_orig: FindNext (fallen/DDLibrary/Source/MFX.cpp)
static MFX_Voice* FindNext(MFX_Voice* vptr)
{
    int offset = Hash(vptr->id);

    int ii = ((vptr - Voices) - offset) & VOICE_MSK;

    for (ii++; ii < MAX_VOICE; ii++) {
        int vn = (ii + offset) & VOICE_MSK;
        if (Voices[vn].id == vptr->id) {
            return &Voices[vn];
        }
    }

    return NULL;
}

// uc_orig: FindFree (fallen/DDLibrary/Source/MFX.cpp)
static MFX_Voice* FindFree(UWORD channel_id)
{
    int offset = Hash(channel_id);

    for (int ii = 0; ii < MAX_VOICE; ii++) {
        int vn = (ii + offset) & VOICE_MSK;
        if (!Voices[vn].smp) {
            return &Voices[vn];
        }
    }

    return NULL;
}

// uc_orig: FreeVoiceSource (fallen/DDLibrary/Source/MFX.cpp)
static void FreeVoiceSource(MFX_Voice* vptr)
{
    if (vptr->handle) {
        vptr->smp->usecount--;
        alSourceStop(vptr->handle);
        alDeleteSources(1, &vptr->handle);
        vptr->handle = 0;
    }
}

// uc_orig: FreeVoice (fallen/DDLibrary/Source/MFX.cpp)
static void FreeVoice(MFX_Voice* vptr)
{
    if (!vptr)
        return;

    QFreeLast->next = vptr->queue;
    while (QFreeLast->next) {
        QFreeLast = QFreeLast->next;
    }
    vptr->queue = NULL;
    vptr->queuesz = 0;

    FreeVoiceSource(vptr);

    if (vptr->thing) {
        vptr->thing->Flags &= ~FLAGS_HAS_ATTACHED_SOUND;
    }
    vptr->thing = NULL;
    vptr->flags = 0;
    vptr->id = 0;
    vptr->wave = 0;
    vptr->smp = NULL;
}

// uc_orig: GetVoiceForWave (fallen/DDLibrary/Source/MFX.cpp)
static MFX_Voice* GetVoiceForWave(UWORD channel_id, ULONG wave, ULONG flags)
{
    if (flags & MFX_OVERLAP) {
        return FindFree(channel_id);
    }

    MFX_Voice* vptr;

    if (flags & (MFX_QUEUED | MFX_NEVER_OVERLAP)) {
        vptr = FindFirst(channel_id);
    } else {
        vptr = FindVoice(channel_id, wave);
    }

    if (!vptr) {
        vptr = FindFree(channel_id);
    } else {
        if ((flags & (MFX_NEVER_OVERLAP | MFX_QUEUED)) == MFX_NEVER_OVERLAP) {
            return NULL;
        }
    }

    return vptr;
}

// uc_orig: SetupVoiceTalk (fallen/DDLibrary/Source/MFX.cpp)
static SLONG SetupVoiceTalk(MFX_Voice* vptr, char* filename)
{
    vptr->id = 0;
    vptr->wave = NumSamples;
    vptr->flags = 0;
    vptr->thing = NULL;
    vptr->queue = NULL;
    vptr->queuesz = 0;
    vptr->smp = NULL;
    vptr->queuesz = 0;
    vptr->smp = NULL;
    vptr->playing = false;
    vptr->ratemult = 1.0;
    vptr->gain = 1.0;

    if (!Volumes[SMP_Effect]) {
        return UC_FALSE;
    }

    LoadTalkFile(filename);
    if (!TalkSample.handle) {
        return UC_FALSE;
    }

    vptr->smp = &TalkSample;
    FinishLoading(vptr);

    return UC_TRUE;
}

// uc_orig: SetupVoice (fallen/DDLibrary/Source/MFX.cpp)
static void SetupVoice(MFX_Voice* vptr, UWORD channel_id, ULONG wave, ULONG flags, bool is3D)
{
    vptr->id = channel_id;
    vptr->wave = wave;
    vptr->flags = flags;
    vptr->is3D = is3D;
    vptr->thing = NULL;
    vptr->queue = NULL;
    vptr->queuesz = 0;
    vptr->smp = NULL;
    vptr->playing = false;
    vptr->ratemult = 1.0;
    vptr->gain = 1.0;

    if (wave >= NumSamples) {
        return;
    }

    MFX_Sample* sptr = &Samples[wave];

    if ((sptr->type != SMP_Music) && (GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON))) {
        return;
    }

    float level = Volumes[sptr->type];

    if (!level) {
        return;
    }

    if (!sptr->handle) {
        LoadWaveFile(sptr);
        if (!sptr->handle) {
            return;
        }
    }

    if (sptr->prev_lru) {
        sptr->prev_lru->next_lru = sptr->next_lru;
        sptr->next_lru->prev_lru = sptr->prev_lru;
    }

    if (AllocatedRAM > MAX_SAMPLE_MEM) {
        MFX_Sample* sptr = LRU.next_lru;
        while (sptr != &LRU) {
            MFX_Sample* next = sptr->next_lru;
            if (!sptr->usecount) {
                UnloadWaveFile(sptr);
                if (AllocatedRAM <= MAX_SAMPLE_MEM) {
                    break;
                }
            }
            sptr = next;
        }
    }

    sptr->next_lru = &LRU;
    sptr->prev_lru = LRU.prev_lru;
    sptr->next_lru->prev_lru = sptr;
    sptr->prev_lru->next_lru = sptr;

    vptr->smp = sptr;

    if (!sptr->loading) {
        FinishLoading(vptr);
    }
}

// uc_orig: FinishLoading (fallen/DDLibrary/Source/MFX.cpp)
static void FinishLoading(MFX_Voice* vptr)
{
    MFX_Sample* sptr = vptr->smp;

    alGenSources(1, &vptr->handle);

    if (vptr->handle) {
        vptr->is3D &= sptr->is3D;
        sptr->usecount++;
        alSourcei(vptr->handle, AL_BUFFER, sptr->handle);
        alSourcef(vptr->handle, AL_GAIN, Volumes[sptr->type]);

        if (vptr->is3D) {
            alSourcei(vptr->handle, AL_DISTANCE_MODEL, AL_LINEAR_DISTANCE_CLAMPED);
            alSourcef(vptr->handle, AL_REFERENCE_DISTANCE, MinDist * COORDINATE_UNITS * sptr->linscale);
            alSourcef(vptr->handle, AL_MAX_DISTANCE, MaxDist * COORDINATE_UNITS * sptr->linscale);
        } else {
            alSourcei(vptr->handle, AL_SOURCE_SPATIALIZE_SOFT, AL_FALSE);
            alSourcei(vptr->handle, AL_DIRECT_CHANNELS_SOFT, AL_TRUE);
        }
    }

    MoveVoice(vptr);
    if (vptr->ratemult != 1.0) {
        SetVoiceRate(vptr, vptr->ratemult);
    }
    if (vptr->gain != 1.0) {
        SetVoiceGain(vptr, vptr->gain);
    }
    if (vptr->playing) {
        PlayVoice(vptr);
    }
}

// uc_orig: PlayVoice (fallen/DDLibrary/Source/MFX.cpp)
static void PlayVoice(MFX_Voice* vptr)
{
    if (vptr->handle) {
        if (vptr->flags & MFX_LOOPED) {
            alSourcei(vptr->handle, AL_LOOPING, AL_TRUE);
        }
        alSourcePlay(vptr->handle);
    }
    vptr->playing = true;
}

// uc_orig: MoveVoice (fallen/DDLibrary/Source/MFX.cpp)
static void MoveVoice(MFX_Voice* vptr)
{
    if (vptr->is3D) {
        float x = vptr->x * COORDINATE_UNITS;
        float y = vptr->y * COORDINATE_UNITS;
        float z = vptr->z * COORDINATE_UNITS;

        ALfloat position[3];
        if ((fabs(x - LX) < 0.5) && (fabs(y - LY) < 0.5) && (fabs(z - LZ) < 0.5)) {
            position[0] = LX;
            position[1] = LY;
            position[2] = LZ;
        } else {
            position[0] = x;
            position[1] = y;
            position[2] = z;
        }
        alSourcefv(vptr->handle, AL_POSITION, position);
    }
}

// uc_orig: SetVoiceRate (fallen/DDLibrary/Source/MFX.cpp)
static void SetVoiceRate(MFX_Voice* vptr, float mult)
{
    if (vptr->handle) {
        alSourcef(vptr->handle, AL_PITCH, mult);
    }
    vptr->ratemult = mult;
}

// uc_orig: SetVoiceGain (fallen/DDLibrary/Source/MFX.cpp)
static void SetVoiceGain(MFX_Voice* vptr, float gain)
{
    if (vptr->smp == NULL) {
        return;
    }

    gain *= Volumes[vptr->smp->type];

    if (vptr->handle) {
        if (vptr->is3D) {
            alSourcef(vptr->handle, AL_GAIN, gain);
        } else {
            alSourcef(vptr->handle, AL_GAIN, gain * Gain2D);
        }
    }
    if (vptr->queue) {
        vptr->queue->gain = gain;
    }
    vptr->gain = gain;
}

// uc_orig: IsVoiceDone (fallen/DDLibrary/Source/MFX.cpp)
static bool IsVoiceDone(MFX_Voice* vptr)
{
    if (vptr->flags & MFX_LOOPED) {
        return false;
    }

    ALint state;
    ALint posn;

    if (vptr->handle) {
        alGetSourcei(vptr->handle, AL_SOURCE_STATE, &state);
        alGetSourcei(vptr->handle, AL_SAMPLE_OFFSET, &posn);
    } else if (vptr->smp && vptr->smp->loading) {
        return false;
    } else {
        return true;
    }

    if (vptr->flags & MFX_EARLY_OUT) {
        return (vptr->smp->size - posn < 440 * 2);
    }

    return (state != AL_PLAYING);
}

// uc_orig: QueueWave (fallen/DDLibrary/Source/MFX.cpp)
static void QueueWave(MFX_Voice* vptr, UWORD wave, ULONG flags, bool is3D, SLONG x, SLONG y, SLONG z)
{
    if ((flags & MFX_SHORT_QUEUE) && vptr->queue) {
        vptr->queue->flags = flags;
        vptr->queue->wave = wave;
        vptr->queue->x = x;
        vptr->queue->y = y;
        vptr->queue->z = z;
        return;
    }

    if (vptr->queuesz > MAX_QVOICE) {
        return;
    }
    if (QFree == QFreeLast) {
        return;
    }

    MFX_QWave* qptr = vptr->queue;

    if (qptr) {
        while (qptr->next)
            qptr = qptr->next;
        qptr->next = QFree;
        QFree = QFree->next;
        qptr = qptr->next;
    } else {
        vptr->queue = QFree;
        QFree = QFree->next;
        qptr = vptr->queue;
    }

    qptr->next = NULL;
    qptr->flags = flags;
    qptr->wave = wave;
    qptr->is3D = is3D;
    qptr->x = x;
    qptr->y = y;
    qptr->z = z;
    qptr->gain = 1.0;
}

// uc_orig: TriggerPairedVoice (fallen/DDLibrary/Source/MFX.cpp)
static void TriggerPairedVoice(UWORD channel_id)
{
    MFX_Voice* vptr = FindFirst(channel_id);

    if (!vptr || !vptr->smp) {
        return;
    }

    vptr->flags &= ~MFX_PAIRED_TRK2;
    PlayVoice(vptr);
}

// uc_orig: PlayWave (fallen/DDLibrary/Source/MFX.cpp)
static UBYTE PlayWave(UWORD channel_id, ULONG wave, ULONG flags, bool is3D, SLONG x, SLONG y, SLONG z, Thing* thing)
{
    MFX_Voice* vptr = GetVoiceForWave(channel_id, wave, flags);

    if (!vptr) {
        return 0;
    }

    if (thing) {
        vptr->x = (thing->WorldPos.X >> 8);
        vptr->y = (thing->WorldPos.Y >> 8);
        vptr->z = (thing->WorldPos.Z >> 8);
    } else {
        vptr->x = x;
        vptr->y = y;
        vptr->z = z;
    }

    if (vptr->smp) {
        if ((vptr->smp->type != SMP_Music) && (GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON))) {
            return 0;
        }
        if (flags & MFX_QUEUED) {
            QueueWave(vptr, wave, flags, is3D, vptr->x, vptr->y, vptr->z);
            return 2;
        }
        if ((vptr->wave == wave) && !(flags & MFX_REPLACE)) {
            MoveVoice(vptr);
            return 0;
        }
        FreeVoice(vptr);
    }

    SetupVoice(vptr, channel_id, wave, flags, is3D);
    MoveVoice(vptr);
    if (!(flags & MFX_PAIRED_TRK2)) {
        PlayVoice(vptr);
    }
    if (thing) {
        vptr->thing = thing;
        thing->Flags |= FLAGS_HAS_ATTACHED_SOUND;
    }
    if (flags & MFX_PAIRED_TRK1) {
        TriggerPairedVoice(channel_id + 1);
    }

    return 1;
}

// uc_orig: PlayTalk (fallen/DDLibrary/Source/MFX.cpp)
static UBYTE PlayTalk(char* filename, SLONG x, SLONG y, SLONG z)
{
    MFX_Voice* vptr = GetVoiceForWave(0, NumSamples, 0);
    if (!vptr) {
        return 0;
    }

    if (vptr->smp) {
        FreeVoice(vptr);
    }

    if (x | y | z) {
        TalkSample.is3D = TALK_3D ? true : false;
    } else {
        TalkSample.is3D = false;
    }

    vptr->x = x;
    vptr->y = y;
    vptr->z = z;

    if (!SetupVoiceTalk(vptr, filename)) {
        return UC_FALSE;
    }

    MoveVoice(vptr);
    PlayVoice(vptr);

    return 1;
}

// uc_orig: MFX_get_volumes (fallen/DDLibrary/Source/MFX.cpp)
void MFX_get_volumes(SLONG* fx, SLONG* amb, SLONG* mus)
{
    *fx = SLONG(127 * Volumes[SMP_Effect]);
    *amb = SLONG(127 * Volumes[SMP_Ambient]);
    *mus = SLONG(127 * Volumes[SMP_Music]);
}

// uc_orig: MFX_set_volumes (fallen/DDLibrary/Source/MFX.cpp)
void MFX_set_volumes(SLONG fx, SLONG amb, SLONG mus)
{
    fx = std::min(std::max(fx, (SLONG)0), (SLONG)127);
    amb = std::min(std::max(amb, (SLONG)0), (SLONG)127);
    mus = std::min(std::max(mus, (SLONG)0), (SLONG)127);

    Volumes[SMP_Effect] = float(fx) / 127.0f;
    Volumes[SMP_Ambient] = float(amb) / 127.0f;
    Volumes[SMP_Music] = float(mus) / 127.0f;

    ENV_set_value_number("ambient_volume", amb, "Audio");
    ENV_set_value_number("music_volume", mus, "Audio");
    ENV_set_value_number("fx_volume", fx, "Audio");
}

// uc_orig: MFX_play_xyz (fallen/DDLibrary/Source/MFX.cpp)
void MFX_play_xyz(UWORD channel_id, ULONG wave, ULONG flags, SLONG x, SLONG y, SLONG z)
{
    PlayWave(channel_id, wave, flags, true, x >> 8, y >> 8, z >> 8, NULL);
}

// uc_orig: MFX_play_thing (fallen/DDLibrary/Source/MFX.cpp)
void MFX_play_thing(UWORD channel_id, ULONG wave, ULONG flags, Thing* p)
{
    PlayWave(channel_id, wave, flags, true, 0, 0, 0, p);
}

// uc_orig: MFX_play_ambient (fallen/DDLibrary/Source/MFX.cpp)
void MFX_play_ambient(UWORD channel_id, ULONG wave, ULONG flags)
{
    if (wave < NumSamples) {
        Samples[wave].is3D = false;
        if (Samples[wave].type == SMP_Effect) {
            Samples[wave].type = SMP_Ambient;
        }
    }
    PlayWave(channel_id, wave, flags, true, FC_cam[0].x, FC_cam[0].y, FC_cam[0].z, NULL);
}

// uc_orig: MFX_play_stereo (fallen/DDLibrary/Source/MFX.cpp)
UBYTE MFX_play_stereo(UWORD channel_id, ULONG wave, ULONG flags)
{
    return PlayWave(channel_id, wave, flags, false, 0, 0, 0, NULL);
}

// uc_orig: MFX_stop (fallen/DDLibrary/Source/MFX.cpp)
void MFX_stop(SLONG channel_id, ULONG wave)
{
    if (channel_id == MFX_CHANNEL_ALL) {
        for (int ii = 0; ii < MAX_VOICE; ii++) {
            FreeVoice(&Voices[ii]);
        }
    } else {
        if (wave == MFX_WAVE_ALL) {
            MFX_Voice* vptr = FindFirst(channel_id);
            while (vptr) {
                FreeVoice(vptr);
                vptr = FindNext(vptr);
            }
        } else {
            FreeVoice(FindVoice(channel_id, wave));
        }
    }
}

// uc_orig: MFX_stop_attached (fallen/DDLibrary/Source/MFX.cpp)
void MFX_stop_attached(Thing* p)
{
    for (int ii = 0; ii < MAX_VOICE; ii++) {
        if (Voices[ii].thing == p)
            FreeVoice(&Voices[ii]);
    }
}

// uc_orig: MFX_set_pitch (fallen/DDLibrary/Source/MFX.cpp)
void MFX_set_pitch(UWORD channel_id, ULONG wave, SLONG pitchbend)
{
    MFX_Voice* vptr = FindVoice(channel_id, wave);
    if (!vptr || !vptr->smp) {
        return;
    }

    float pitch = float(pitchbend + 256) / 256.0f;
    SetVoiceRate(vptr, pitch);
}

// uc_orig: MFX_set_gain (fallen/DDLibrary/Source/MFX.cpp)
void MFX_set_gain(UWORD channel_id, ULONG wave, UBYTE gain)
{
    MFX_Voice* vptr = FindVoice(channel_id, wave);
    if (!vptr || !vptr->smp) {
        return;
    }

    float fgain = float(gain) / 255.0f;
    SetVoiceGain(vptr, fgain);
}

// uc_orig: MFX_set_queue_gain (fallen/DDLibrary/Source/MFX.cpp)
void MFX_set_queue_gain(UWORD channel_id, ULONG wave, UBYTE gain)
{
    float fgain = float(gain) / 256.0f;

    MFX_Voice* vptr = FindFirst(channel_id);
    while (vptr) {
        if (!vptr->queue && (vptr->wave == wave)) {
            SetVoiceGain(vptr, fgain);
        }

        for (MFX_QWave* qptr = vptr->queue; qptr; qptr = qptr->next) {
            if (qptr->wave == wave) {
                qptr->gain = fgain;
            }
        }

        vptr = FindNext(vptr);
    }
}

// uc_orig: LoadWaveFile (fallen/DDLibrary/Source/MFX.cpp)
static void LoadWaveFile(MFX_Sample* sptr)
{
    if (!sptr->fname || sptr->handle) {
        return;
    }

    SDL3_WavData wav;
    if (!sdl3_load_wav(GetFullName(sptr->fname), &wav)) {
        return;
    }

    sptr->size = wav.size;
    AllocatedRAM += wav.size;

    sptr->loading = false;
    sptr->is3D = sptr->is3D && wav.channels == 1;
    alGenBuffers(1, &sptr->handle);
    alBufferData(sptr->handle, wav.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
        wav.buffer, wav.size, wav.freq);
    sdl3_free_wav(wav.buffer);
}

// uc_orig: LoadTalkFile (fallen/DDLibrary/Source/MFX.cpp)
static void LoadTalkFile(char* filename)
{
    if (TalkSample.handle) {
        alDeleteBuffers(1, &TalkSample.handle);
        AllocatedRAM -= TalkSample.size;
        TalkSample.handle = 0;
    }

    SDL3_WavData wav;
    if (!sdl3_load_wav(filename, &wav)) {
        return;
    }

    TalkSample.size = wav.size;
    AllocatedRAM += TalkSample.size;

    TalkSample.loading = false;
    TalkSample.is3D = TalkSample.is3D && wav.channels == 1;
    alGenBuffers(1, &TalkSample.handle);
    alBufferData(TalkSample.handle, wav.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
        wav.buffer, wav.size, wav.freq);
    sdl3_free_wav(wav.buffer);
}

// uc_orig: UnloadWaveFile (fallen/DDLibrary/Source/MFX.cpp)
static void UnloadWaveFile(MFX_Sample* sptr)
{
    if (!sptr->handle || sptr->usecount > 0) {
        return;
    }

    if (sptr->prev_lru) {
        sptr->prev_lru->next_lru = sptr->next_lru;
        sptr->next_lru->prev_lru = sptr->prev_lru;
        sptr->next_lru = sptr->prev_lru = NULL;
    }

    alDeleteBuffers(1, &sptr->handle);
    sptr->handle = 0;

    AllocatedRAM -= sptr->size;
}

// uc_orig: UnloadTalkFile (fallen/DDLibrary/Source/MFX.cpp)
static void UnloadTalkFile()
{
    if (!TalkSample.handle) {
        return;
    }
    alDeleteBuffers(1, &TalkSample.handle);
    TalkSample.handle = 0;

    AllocatedRAM -= TalkSample.size;
}

// uc_orig: MFX_load_wave_list (fallen/DDLibrary/Source/MFX.cpp)
void MFX_load_wave_list()
{
    MFX_free_wave_list();

    for (int ii = 0; ii < NumSamples; ii++) {
        if (Samples[ii].type == SMP_Music) {
            UnloadWaveFile(&Samples[ii]);
        }
    }
    UnloadWaveFile(&TalkSample);
}

// uc_orig: MFX_free_wave_list (fallen/DDLibrary/Source/MFX.cpp)
void MFX_free_wave_list()
{
    extern void MUSIC_reset();
    MUSIC_reset();

    MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);

    InitVoices();
}

// uc_orig: MFX_set_listener (fallen/DDLibrary/Source/MFX.cpp)
void MFX_set_listener(SLONG x, SLONG y, SLONG z, SLONG heading, SLONG roll, SLONG pitch)
{
    x >>= 8;
    y >>= 8;
    z >>= 8;

    LX = x * COORDINATE_UNITS;
    LY = y * COORDINATE_UNITS;
    LZ = z * COORDINATE_UNITS;

    heading += 0x200;
    heading &= 0x7FF;

    float xorient = float(COS(heading)) / 65536;
    float zorient = float(SIN(heading)) / 65536;

    ALfloat position[] = {
        LX,
        LY,
        LZ
    };
    ALfloat orientation[6] = { -xorient, 0.0f, -zorient,
        0.0f, 1.0f, 0.0f };
    alListenerfv(AL_POSITION, position);
    alListenerfv(AL_ORIENTATION, orientation);

    for (int ii = 0; ii < MAX_VOICE; ii++) {
        if (Voices[ii].is3D) {
            MoveVoice(&Voices[ii]);
        }
    }
}

// uc_orig: MFX_update (fallen/DDLibrary/Source/MFX.cpp)
void MFX_update()
{

    for (int ii = 0; ii < MAX_VOICE; ii++) {
        MFX_Voice* vptr = &Voices[ii];

        if (vptr->flags & MFX_PAIRED_TRK2) {
            continue;
        }
        if (!vptr->smp) {
            continue;
        }

        if (IsVoiceDone(vptr)) {
            if (!vptr->queue) {
                FreeVoice(vptr);
            } else {
                MFX_QWave* qptr = vptr->queue;
                vptr->queue = qptr->next;

                if (qptr->flags & MFX_PAIRED_TRK1) {
                    TriggerPairedVoice(Voices[ii].id + 1);
                }

                Thing* thing = vptr->thing;
                FreeVoiceSource(vptr);
                SetupVoice(vptr, vptr->id, qptr->wave, qptr->flags & ~MFX_PAIRED_TRK2, qptr->is3D);
                vptr->thing = thing;

                if ((vptr->flags & MFX_MOVING) && vptr->thing) {
                    vptr->x = vptr->thing->WorldPos.X >> 8;
                    vptr->y = vptr->thing->WorldPos.Y >> 8;
                    vptr->z = vptr->thing->WorldPos.Z >> 8;
                } else {
                    vptr->x = qptr->x;
                    vptr->y = qptr->y;
                    vptr->z = qptr->z;
                }

                MoveVoice(vptr);
                PlayVoice(vptr);
                SetVoiceGain(vptr, qptr->gain);

                qptr->next = QFree;
                QFree = qptr;
            }
        } else {
            if (vptr->flags & MFX_CAMERA) {
                vptr->x = FC_cam[0].x >> 8;
                vptr->z = FC_cam[0].z >> 8;
                if (!(vptr->flags & MFX_LOCKY)) {
                    vptr->y = FC_cam[0].y >> 8;
                }
                MoveVoice(vptr);
            }
            if ((vptr->flags & MFX_MOVING) && vptr->thing) {
                vptr->x = vptr->thing->WorldPos.X >> 8;
                vptr->y = vptr->thing->WorldPos.Y >> 8;
                vptr->z = vptr->thing->WorldPos.Z >> 8;
                MoveVoice(vptr);
            }
        }
    }
}

// uc_orig: MFX_get_wave (fallen/DDLibrary/Source/MFX.cpp)
UWORD MFX_get_wave(UWORD channel_id, UBYTE index)
{
    MFX_Voice* vptr = FindFirst(channel_id);

    while (index--) {
        vptr = FindNext(vptr);
    }

    return vptr ? vptr->wave : 0;
}

// uc_orig: MFX_QUICK_play (fallen/DDLibrary/Source/MFX.cpp)
SLONG MFX_QUICK_play(CBYTE* str, SLONG x, SLONG y, SLONG z)
{
    return PlayTalk(str, x, y, z);
}

// uc_orig: MFX_QUICK_still_playing (fallen/DDLibrary/Source/MFX.cpp)
SLONG MFX_QUICK_still_playing()
{
    MFX_Voice* vptr = GetVoiceForWave(0, NumSamples, 0);
    if (!vptr) {
        return 0;
    }

    return IsVoiceDone(vptr) ? 0 : 1;
}

// uc_orig: MFX_QUICK_stop (fallen/DDLibrary/Source/MFX.cpp)
void MFX_QUICK_stop()
{
    MFX_stop(0, NumSamples);
}

// uc_orig: MFX_QUICK_wait (fallen/DDLibrary/Source/MFX.cpp)
void MFX_QUICK_wait()
{
    while (MFX_QUICK_still_playing())
        ;
    MFX_QUICK_stop();
}
