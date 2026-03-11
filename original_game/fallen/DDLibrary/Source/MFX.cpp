#define	ASYNC_FILE_IO	0
#define TALK_3D			0

#include "MFX.h"

#include "..\headers\fc.h"
#include "..\headers\env.h"
#include "..\headers\demo.h"
#include "resource.h"
#ifndef TARGET_DC
#include <cmath>
#endif

#include "drive.h"

#if ASYNC_FILE_IO
#include "asyncfile2.h"
#endif

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <SDL2/SDL_audio.h>

#define		COORDINATE_UNITS	float(1.0 / 256.0)	// 1 tile ~= 1 meter

enum SampleType
{
	SMP_Ambient = 0,
	SMP_Music,
	SMP_Effect
};

struct MFX_Sample
{
	MFX_Sample*	prev_lru;		// prev LRU entry (i.e. sample used before this one)
	MFX_Sample*	next_lru;		// next LRU entry (i.e. sample used after this one)
	char*		fname;			// sample filename
#if ASYNC_FILE_IO
	unsigned char* loadBuffer;
#endif
	unsigned int handle;
	bool		is3D;
	int			size;
	int			usecount;
	int			type;			// SampleType of sample - used to get volume
	float		linscale;		// linear scaling for sample volume - used to set 3D distances
	bool		loading;
};

#define MAX_SAMPLE		552
#define MAX_SAMPLE_MEM	64*1024*1024

// MFX_QWave
//
// control block for a queued wave

struct MFX_QWave
{
	MFX_QWave*	next;		// next voice to be queued, or NULL
	ULONG		wave;		// sound sample to be played
	ULONG		flags;
	bool		is3D;
	SLONG		x, y, z;	// coordinates of the voice
	float		gain;
};

#define MAX_QWAVE		32	// number of queued wave slots
#define MAX_QVOICE		5	// maximum waves queued per voice

struct MFX_Voice
{
	UWORD		id;			// channel_id for this voice
	unsigned int handle;
	ULONG		wave;		// sound sample playing on this voice
	ULONG		flags;
	bool		is3D;
	SLONG		x, y, z;		// coordinates of this voice
	Thing*		thing;		// thing this voice belongs to
	MFX_QWave*	queue;		// queue of samples to play
	SLONG		queuesz;	// number of queued samples
	MFX_Sample*	smp;		// sample being played
	bool		playing;
	float		ratemult;
	float		gain;
};

#define MAX_VOICE	64
#define	VOICE_MSK	63		// mask for voice indices

ALCdevice* alDevice;
ALCcontext* alContext;

static MFX_Voice	Voices[MAX_VOICE];
static MFX_QWave	QWaves[MAX_QWAVE];
static MFX_QWave*	QFree;		// first free queue elt. (NEVER NULL - we waste one element)
static MFX_QWave*	QFreeLast;	// last free queue elt.

static MFX_Sample	Samples[MAX_SAMPLE];
static MFX_Sample	TalkSample;
static int			NumSamples;
static MFX_Sample	LRU;			// sentinel for LRU dllist

static const float	MinDist = 512;			// min distance for 3D provider
static const float	MaxDist = (64 << 8);	// max distance for 3D provider
static float		Gain2D = 1.0;			// gain for 2D voices
static float		LX,LY,LZ;				// listener coords

static float		Volumes[3];				// volumes for each SampleType

static int			AllocatedRAM = 0;

static char*		GetFullName(char *fname);
void				SetLinearScale(SLONG wave, float linscale);	// set volume scaling for sample (1.0 = normal)
void				SetPower(SLONG wave, float dB);				// set power for sample in dB (0 = normal)
static void			InitVoices();
static void			LoadWaveFile(MFX_Sample* sptr);
static void			LoadTalkFile(char* filename);
#if ASYNC_FILE_IO
static void			LoadWaveFileAsync(MFX_Sample* sptr);
#endif
static void			UnloadWaveFile(MFX_Sample* sptr);
static void			UnloadTalkFile();
static void			FinishLoading(MFX_Voice* vptr);

static void			PlayVoice(MFX_Voice* vptr);
static void			MoveVoice(MFX_Voice* vptr);
static void			SetVoiceRate(MFX_Voice* vptr, float mult);
static void			SetVoiceGain(MFX_Voice* vptr, float gain);

extern CBYTE*		sound_list[];

void MFX_init()
{
	alDevice = alcOpenDevice(nullptr);
	alContext = alcCreateContext(alDevice, nullptr);
	alcMakeContextCurrent(alContext);

#if ASYNC_FILE_IO
	InitAsyncFile();
#endif

	InitVoices();

	// initialize Samples[]
	LRU.prev_lru = LRU.next_lru = &LRU;
	AllocatedRAM = 0;

	NumSamples = 0;
	CBYTE		buf[_MAX_PATH];
	CBYTE**		names = sound_list;
	MFX_Sample*	sptr = Samples;

	while (strcmp(names[0], "!"))
	{
		sptr->prev_lru = NULL;
		sptr->next_lru = NULL;
		sptr->fname = NULL;
		sptr->handle = 0;
		sptr->is3D = true;
		sptr->size = 0;
		sptr->type = SMP_Effect;
		sptr->linscale = 1.0;
		sptr->loading = false;

		if (stricmp("null.wav", names[0]))
		{
			FILE*	fd = MF_Fopen(GetFullName(names[0]), "rb");
			if (fd)
			{
				sptr->fname = names[0];
				if (((!strnicmp(names[0], "music",5))||(!strnicmp(names[0], "generalmusic",12)))&&(!strstr(names[0],"Club1"))&&(!strstr(names[0],"Acid")))
				{
					sptr->is3D = false;
					sptr->type = SMP_Music;
				}

				fseek(fd, 0, SEEK_END);
				sptr->size = ftell(fd);
				MF_Fclose(fd);
			}
			else
			{
				TRACE("NOT FOUND: %s (%s)\n", names[0], GetFullName(names[0]));
			}
		}

		NumSamples++;

		if (sptr->fname)
		{
			int gain = GetPrivateProfileInt("PowerLevels", sptr->fname, 0, "data\\sfx\\powerlvl.ini");
			if (gain)
			{
				gain *= 4;
				SetPower(sptr-Samples, float(gain));
				TRACE("Setting %s gain to %d dB\n",sptr->fname,gain);
			}
		}

		sptr++;
		names++;
	}

	ASSERT(NumSamples <= MAX_SAMPLE);

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

void MFX_term()
{
	MFX_free_wave_list();

	// free waves
	for (int ii = 0; ii < NumSamples; ii++)
	{
		UnloadWaveFile(&Samples[ii]);
	}
	NumSamples = 0;

	UnloadTalkFile();

	alcMakeContextCurrent(NULL);
	alcDestroyContext(alContext);
	alcCloseDevice(alDevice);

#if ASYNC_FILE_IO
	TermAsyncFile();
#endif
}

static char* GetFullName(char* fname)
{
	CBYTE	buf[MAX_PATH];
	static CBYTE	pathname[MAX_PATH];

	if (strchr(fname,'-'))  // usefully, all the taunts etc have a - in them, and none of the other sounds do... bonus!
	{
		CHAR *ptr = strrchr(fname,'\\')+1;
		sprintf(buf,"talk2\\misc\\%s",ptr);
		strcpy(pathname, GetSFXPath());
		strcat(pathname, buf);
		return pathname;
	}
	else
	{
		sprintf(buf, "data\\sfx\\1622\\%s", fname);
		if (!strnicmp(fname, "music", 5))
		{
			if (!MUSIC_WORLD)
			{
				MUSIC_WORLD = 1;
			}
			buf[19] = '0' + (MUSIC_WORLD / 10);
			buf[20] = '0' + (MUSIC_WORLD % 10);
		}
	}

	strcpy(pathname, GetSFXPath());
	strcat(pathname, buf);

	return pathname;
}

void SetLinearScale(SLONG wave, float linscale)
{
	if ((wave >= 0) && (wave < NumSamples))
	{
		Samples[wave].linscale = linscale;
	}
}

void SetPower(SLONG wave, float dB)
{
	SetLinearScale(wave, float(exp(log(10) * dB / 20)));
}

static void InitVoices()
{
	int	ii;

	for (ii = 0; ii < MAX_VOICE; ii++)
	{
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

	for (ii = 0; ii < MAX_QWAVE; ii++)
	{
		QWaves[ii].next = &QWaves[ii+1];
	}
	QWaves[ii-1].next = NULL;
	QFree = &QWaves[0];
	QFreeLast = &QWaves[ii-1];

	LX = LY = LZ = 0;
}

// hash a channel ID to a voice ID
static inline int Hash(UWORD channel_id)
{
	return (channel_id * 37) & VOICE_MSK;
}

static MFX_Voice* FindVoice(UWORD channel_id, ULONG wave)
{
	int	offset = Hash(channel_id);

	for (int ii = 0; ii < MAX_VOICE; ii++)
	{
		int	vn = (ii + offset) & VOICE_MSK;
		if ((Voices[vn].id == channel_id) && (Voices[vn].wave == wave))
		{
			return &Voices[vn];
		}
	}

	return NULL;
}

static MFX_Voice* FindFirst(UWORD channel_id)
{
	int	offset = Hash(channel_id);

	for (int ii = 0; ii < MAX_VOICE; ii++)
	{
		int	vn = (ii + offset) & VOICE_MSK;
		if (Voices[vn].id == channel_id)
		{
			return &Voices[vn];
		}
	}

	return NULL;
}

// find the next active voice with the same channel ID
static MFX_Voice* FindNext(MFX_Voice* vptr)
{
	int	offset = Hash(vptr->id);

	int	ii = ((vptr - Voices) - offset) & VOICE_MSK;

	for (ii++; ii < MAX_VOICE; ii++)
	{
		int	vn = (ii + offset) & VOICE_MSK;
		if (Voices[vn].id == vptr->id)
		{
			return &Voices[vn];
		}
	}

	return NULL;
}

static MFX_Voice* FindFree(UWORD channel_id)
{
	int	offset = Hash(channel_id);

	for (int ii = 0; ii < MAX_VOICE; ii++)
	{
		int	vn = (ii + offset) & VOICE_MSK;
		if (!Voices[vn].smp)
		{
			return &Voices[vn];
		}
	}

	return NULL;
}

static void FreeVoiceSource(MFX_Voice* vptr)
{
	if (vptr->handle)
	{
		vptr->smp->usecount--;
		alSourceStop(vptr->handle);
		alDeleteSources(1, &vptr->handle);
		vptr->handle = 0;
	}
}

static void FreeVoice(MFX_Voice* vptr)
{
	if (!vptr)	return;

	// remove queue
	QFreeLast->next = vptr->queue;
	while (QFreeLast->next)
	{
		QFreeLast = QFreeLast->next;
	}
	vptr->queue = NULL;
	vptr->queuesz = 0;

	// reset data
	FreeVoiceSource(vptr);

	if (vptr->thing)
	{
		vptr->thing->Flags &= ~FLAGS_HAS_ATTACHED_SOUND;
	}
	vptr->thing = NULL;
	vptr->flags = 0;
	vptr->id = 0;
	vptr->wave = 0;
	vptr->smp = NULL;
}

static MFX_Voice* GetVoiceForWave(UWORD channel_id, ULONG wave, ULONG flags)
{
	// just return a new voice if overlapped
	if (flags & MFX_OVERLAP)
	{
		return FindFree(channel_id);
	}

	MFX_Voice*	vptr;

	if (flags & (MFX_QUEUED | MFX_NEVER_OVERLAP))
	{
		// find first voice on this channel, if any
		vptr = FindFirst(channel_id);
	}
	else
	{
		// find voice playing this sample, if any
		vptr = FindVoice(channel_id, wave);
	}

	if (!vptr)
	{
		vptr = FindFree(channel_id);
	}
	else
	{
		// found a voice - return NULL if not queued but never overlapped (else queue)
		if ((flags & (MFX_NEVER_OVERLAP | MFX_QUEUED)) == MFX_NEVER_OVERLAP)
		{
			return NULL;
		}
	}

	return vptr;
}

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

	if (!Volumes[SMP_Effect])
	{
		return FALSE;
	}

	LoadTalkFile(filename);
	if (!TalkSample.handle)
	{
		return FALSE;
	}

	vptr->smp = &TalkSample;
	FinishLoading(vptr);

	return TRUE;
}

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

	if (wave >= NumSamples)
	{
		return;
	}

	MFX_Sample*	sptr = &Samples[wave];

	// once the level's won or lost, no more sounds
	if ((sptr->type!=SMP_Music)&&(GAME_STATE & (GS_LEVEL_LOST|GS_LEVEL_WON))) 
	{
		return;
	}

	float level = Volumes[sptr->type];

	if (!level)
	{
		return;
	}

	// load the sample
	if (!sptr->handle)
	{
#if ASYNC_FILE_IO
		LoadWaveFileAsync(sptr);
#else
		LoadWaveFile(sptr);
#endif
		if (!sptr->handle)
		{
			return;
		}
	}

	// unlink from LRU queue
	if (sptr->prev_lru)
	{
		sptr->prev_lru->next_lru = sptr->next_lru;
		sptr->next_lru->prev_lru = sptr->prev_lru;
	}

	// free some stuff if we've got too many loaded
	if (AllocatedRAM > MAX_SAMPLE_MEM)
	{
		MFX_Sample*	sptr = LRU.next_lru;
		while (sptr != &LRU)
		{
			MFX_Sample*	next = sptr->next_lru;
			if (!sptr->usecount)
			{
				UnloadWaveFile(sptr);
				if (AllocatedRAM <= MAX_SAMPLE_MEM)
				{
					break;
				}
			}
			sptr = next;
		}
	}

	// link in at front
	sptr->next_lru = &LRU;
	sptr->prev_lru = LRU.prev_lru;
	sptr->next_lru->prev_lru = sptr;
	sptr->prev_lru->next_lru = sptr;

	vptr->smp = sptr;

	if (!sptr->loading)
	{
		FinishLoading(vptr);
	}
}
//TODO: Review memory leak due to OpenAL Usage
// set up voice after sample has loaded
static void FinishLoading(MFX_Voice* vptr)
{
	MFX_Sample*	sptr = vptr->smp;

	alGenSources(1, &vptr->handle);

	if (vptr->handle)
	{
		vptr->is3D &= sptr->is3D;
		sptr->usecount++;
		alSourcei(vptr->handle, AL_BUFFER, sptr->handle);
		alSourcef(vptr->handle, AL_GAIN, Volumes[sptr->type]);
			
		if (vptr->is3D)
		{
			alSourcei(vptr->handle, AL_DISTANCE_MODEL, AL_LINEAR_DISTANCE_CLAMPED);
			alSourcef(vptr->handle, AL_REFERENCE_DISTANCE, MinDist * COORDINATE_UNITS * sptr->linscale);
			alSourcef(vptr->handle, AL_MAX_DISTANCE, MaxDist * COORDINATE_UNITS * sptr->linscale);
		}
		else
		{
			alSourcei(vptr->handle, AL_SOURCE_SPATIALIZE_SOFT, AL_FALSE);
			alSourcei(vptr->handle, AL_DIRECT_CHANNELS_SOFT, AL_TRUE);
		}
	}

	MoveVoice(vptr);
	if (vptr->ratemult != 1.0)
	{
		SetVoiceRate(vptr, vptr->ratemult);
	}
	if (vptr->gain != 1.0)
	{
		SetVoiceGain(vptr, vptr->gain);
	}
	if (vptr->playing)
	{
		PlayVoice(vptr);
	}
}

static void PlayVoice(MFX_Voice* vptr)
{
	if (vptr->handle)
	{
		if (vptr->flags & MFX_LOOPED)
		{
			alSourcei(vptr->handle, AL_LOOPING, AL_TRUE);
		}
		alSourcePlay(vptr->handle);
	}
	vptr->playing = true;
}

static void MoveVoice(MFX_Voice* vptr)
{
	if (vptr->is3D)
	{
		float	x = vptr->x * COORDINATE_UNITS;
		float	y = vptr->y * COORDINATE_UNITS;
		float	z = vptr->z * COORDINATE_UNITS;

		ALfloat position[3];
		if ((fabs(x - LX) < 0.5) && (fabs(y - LY) < 0.5) && (fabs(z - LZ) < 0.5))
		{
			// set exactly at the listener if within epsilon
			position[0] = LX;
			position[1] = LY;
			position[2] = LZ;
		}
		else
		{
			position[0] = x;
			position[1] = y;
			position[2] = z;
		}
		alSourcefv(vptr->handle, AL_POSITION, position);
	}
}

static void SetVoiceRate(MFX_Voice* vptr, float mult)
{
	if (vptr->handle)
	{
		alSourcef(vptr->handle, AL_PITCH, mult);
	}
	vptr->ratemult = mult;
}

static void SetVoiceGain(MFX_Voice* vptr, float gain)
{
	if (vptr->smp == NULL)
	{
		return;
	}

	gain *= Volumes[vptr->smp->type];

	if (vptr->handle)
	{
		if (vptr->is3D)
		{
			alSourcef(vptr->handle, AL_GAIN, gain);
		}
		else
		{
			alSourcef(vptr->handle, AL_GAIN, gain * Gain2D);
		}		
	}
	if (vptr->queue)
	{
		vptr->queue->gain = gain;
	}
	vptr->gain = gain;
}

static bool IsVoiceDone(MFX_Voice* vptr)
{
	if (vptr->flags & MFX_LOOPED)
	{
		return false;
	}

	ALint state;
	ALint posn;

	if (vptr->handle)
	{
		alGetSourcei(vptr->handle, AL_SOURCE_STATE, &state);
		alGetSourcei(vptr->handle, AL_SAMPLE_OFFSET, &posn);
	}
	else if (vptr->smp && vptr->smp->loading)
	{
		return false;
	}
	else
	{
		return true;
	}

	if (vptr->flags & MFX_EARLY_OUT)
	{
		return (vptr->smp->size - posn < 440*2);
	}

	return (state != AL_PLAYING);
}

static void QueueWave(MFX_Voice* vptr, UWORD wave, ULONG flags, bool is3D, SLONG x, SLONG y, SLONG z)
{
	if ((flags & MFX_SHORT_QUEUE) && vptr->queue)
	{
		// short queue - just blat it over the queued one
		vptr->queue->flags = flags;
		vptr->queue->wave = wave;
		vptr->queue->x = x;
		vptr->queue->y = y;
		vptr->queue->z = z;
		return;
	}

	if (vptr->queuesz > MAX_QVOICE)
	{
		return;
	}
	// no free slots
	if (QFree == QFreeLast)
	{
		return;
	}

	// allocate a queue element
	MFX_QWave*	qptr = vptr->queue;

	if (qptr)
	{
		while (qptr->next)	qptr = qptr->next;
		qptr->next = QFree;
		QFree = QFree->next;
		qptr = qptr->next;
	}
	else
	{
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

static void TriggerPairedVoice(UWORD channel_id)
{
	MFX_Voice*	vptr = FindFirst(channel_id);

	if (!vptr || !vptr->smp)
	{
		return;
	}

	vptr->flags &= ~MFX_PAIRED_TRK2;
	PlayVoice(vptr);
}
//TODO: Review memory leak due to OpenAL Usage
static UBYTE PlayWave(UWORD channel_id, ULONG wave, ULONG flags, bool is3D, SLONG x, SLONG y, SLONG z, Thing* thing)
{
	MFX_Voice*	vptr = GetVoiceForWave(channel_id, wave, flags);

	if (!vptr)
	{
		return 0;
	}

	if (thing)
	{
		vptr->x = (thing->WorldPos.X >> 8);
		vptr->y = (thing->WorldPos.Y >> 8);
		vptr->z = (thing->WorldPos.Z >> 8);
	}
	else
	{
		vptr->x = x;
		vptr->y = y;
		vptr->z = z;
	}

	if (vptr->smp)
	{
		// once the level's won or lost, no more sounds
		if ((vptr->smp->type != SMP_Music) && (GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON)))
		{
			return 0;
		}
		if (flags & MFX_QUEUED)
		{
			QueueWave(vptr, wave, flags, is3D, vptr->x, vptr->y, vptr->z);
			return 2;
		}
		if ((vptr->wave == wave) && !(flags & MFX_REPLACE))
		{
			MoveVoice(vptr);
			return 0;
		}
		FreeVoice(vptr);
	}

	SetupVoice(vptr, channel_id, wave, flags, is3D);
	MoveVoice(vptr);
	if (!(flags & MFX_PAIRED_TRK2))
	{
		PlayVoice(vptr);
	}
	if (thing)
	{
		vptr->thing = thing;
		thing->Flags |= FLAGS_HAS_ATTACHED_SOUND;
	}
	if (flags & MFX_PAIRED_TRK1)
	{
		TriggerPairedVoice(channel_id + 1);
	}

	return 1;
}

static UBYTE PlayTalk(char* filename, SLONG x, SLONG y, SLONG z)
{
	MFX_Voice*	vptr = GetVoiceForWave(0, NumSamples, 0);
	if (!vptr)
	{
		return 0;
	}

	if (vptr->smp)
	{
		FreeVoice(vptr);
	}

	if (x | y | z)
	{
		TalkSample.is3D = TALK_3D ? true : false;
	}
	else
	{
		TalkSample.is3D = false;
	}

	vptr->x = x;
	vptr->y = y;
	vptr->z = z;

	if (!SetupVoiceTalk(vptr, filename))
	{
		return FALSE;
	}

	MoveVoice(vptr);
	PlayVoice(vptr);

	return 1;
}

void MFX_get_volumes(SLONG* fx, SLONG* amb, SLONG* mus)
{
	*fx = SLONG(127 * Volumes[SMP_Effect]);
	*amb = SLONG(127 * Volumes[SMP_Ambient]);
	*mus = SLONG(127 * Volumes[SMP_Music]);
}

void MFX_set_volumes(SLONG fx, SLONG amb, SLONG mus)
{
	fx = min(max(fx, 0), 127);
	amb = min(max(amb, 0), 127);
	mus = min(max(mus, 0), 127);

	Volumes[SMP_Effect] = float(fx) / 127.0f;
	Volumes[SMP_Ambient] = float(amb) / 127.0f;
	Volumes[SMP_Music] = float(mus) / 127.0f;

	ENV_set_value_number("ambient_volume", amb, "Audio");
	ENV_set_value_number("music_volume", mus, "Audio");
	ENV_set_value_number("fx_volume", fx, "Audio");
}

void MFX_play_xyz(UWORD channel_id, ULONG wave, ULONG flags, SLONG x, SLONG y, SLONG z)
{
	PlayWave(channel_id, wave, flags, true, x >> 8, y >> 8, z >> 8, NULL);
}

void MFX_play_thing(UWORD channel_id, ULONG wave, ULONG flags, Thing* p)
{
	PlayWave(channel_id, wave, flags, true, 0,0,0, p);
}

void MFX_play_ambient(UWORD channel_id, ULONG wave, ULONG flags)
{
	if (wave < NumSamples)
	{
		// save 3D channels for non-ambient sounds
		Samples[wave].is3D = false;
		if (Samples[wave].type == SMP_Effect)
		{
			// use this volume setting
			Samples[wave].type = SMP_Ambient;
		}
	}
	PlayWave(channel_id, wave, flags, true, FC_cam[0].x, FC_cam[0].y, FC_cam[0].z, NULL);
}

UBYTE MFX_play_stereo(UWORD channel_id, ULONG wave, ULONG flags)
{
	return PlayWave(channel_id, wave, flags, false, 0, 0, 0, NULL);
}

void MFX_stop(SLONG channel_id, ULONG wave)
{
	if (channel_id == MFX_CHANNEL_ALL)
	{
		for (int ii = 0; ii < MAX_VOICE; ii++)
		{
			FreeVoice(&Voices[ii]);
		}
	}
	else
	{
		if (wave == MFX_WAVE_ALL)
		{
			MFX_Voice*	vptr = FindFirst(channel_id);
			while (vptr)
			{
				FreeVoice(vptr);
				vptr = FindNext(vptr);
			}
		}
		else
		{
			FreeVoice(FindVoice(channel_id, wave));
		}
	}
}

// stop all sounds attached to a thing
void MFX_stop_attached(Thing *p)
{
	for (int ii = 0; ii < MAX_VOICE; ii++)
	{
		if (Voices[ii].thing == p)	FreeVoice(&Voices[ii]);
	}
}

void MFX_set_pitch(UWORD channel_id, ULONG wave, SLONG pitchbend)
{
	MFX_Voice* vptr = FindVoice(channel_id, wave);
	if (!vptr || !vptr->smp)
	{
		return;
	}

	float pitch = float(pitchbend + 256) / 256.0f;
	SetVoiceRate(vptr, pitch);
}

void MFX_set_gain(UWORD channel_id, ULONG wave, UBYTE gain)
{
	MFX_Voice* vptr = FindVoice(channel_id, wave);
	if (!vptr || !vptr->smp)
	{
		return;
	}

	float fgain = float(gain) / 255.0f;
	SetVoiceGain(vptr, fgain);
}

void MFX_set_queue_gain(UWORD channel_id, ULONG wave, UBYTE gain)
{
	float fgain = float(gain) / 256.0f;

	MFX_Voice* vptr = FindFirst(channel_id);
	while (vptr)
	{
		if (!vptr->queue && (vptr->wave == wave))
		{
			SetVoiceGain(vptr, fgain);
		}

		for (MFX_QWave* qptr = vptr->queue; qptr; qptr = qptr->next)
		{
			if (qptr->wave == wave)
			{
				qptr->gain = fgain;
			}
		}

		vptr = FindNext(vptr);
	}
}

static void LoadWaveFile(MFX_Sample* sptr)
{
	if (!sptr->fname || sptr->handle)
	{
		return;
	}

	SDL_AudioSpec spec;
	Uint32 bufferSize;
	Uint8* dataBuffer;
	if (!SDL_LoadWAV(GetFullName(sptr->fname), &spec, &dataBuffer, &bufferSize))
	{
		return;
	}
	ASSERT(SDL_AUDIO_BITSIZE(spec.format) == 16);

	sptr->size = bufferSize;
	AllocatedRAM += bufferSize;

	sptr->loading = false;
	sptr->is3D = sptr->is3D && spec.channels == 1;
	alGenBuffers(1, &sptr->handle);
	alBufferData(sptr->handle, spec.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
		dataBuffer, bufferSize, spec.freq);
	SDL_FreeWAV(dataBuffer);
}

static void LoadTalkFile(char* filename)
{
	if (TalkSample.handle)
	{
		alDeleteBuffers(1, &TalkSample.handle);
		AllocatedRAM -= TalkSample.size;
		TalkSample.handle = 0;
	}

	SDL_AudioSpec spec;
	Uint32 bufferSize;
	Uint8* dataBuffer;
	if (!SDL_LoadWAV(filename, &spec, &dataBuffer, &bufferSize))
	{
		return;
	}
	ASSERT(SDL_AUDIO_BITSIZE(spec.format) == 16);

	TalkSample.size = bufferSize;
	AllocatedRAM += TalkSample.size;

	TalkSample.loading = false;
	TalkSample.is3D = TalkSample.is3D && spec.channels == 1;
	alGenBuffers(1, &TalkSample.handle);
	alBufferData(TalkSample.handle, spec.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
		dataBuffer, bufferSize, spec.freq);
	SDL_FreeWAV(dataBuffer);
}

#if ASYNC_FILE_IO

static void LoadWaveFileAsync(MFX_Sample* sptr)
{
	if (!sptr->fname || sptr->handle)
	{
		return;
	}

	FILE*	fd = MF_Fopen(GetFullName(sptr->fname), "rb");
	if (!fd)
	{
		return;
	}

	fseek(fd, 0, SEEK_END);
	sptr->size = ftell(fd);
	MF_Fclose(fd);

	sptr->loadBuffer = new unsigned char[sptr->size];

	AllocatedRAM += sptr->size;

	if (!LoadAsyncFile(GetFullName(sptr->fname), sptr->loadBuffer, sptr->size, sptr))
	{
		delete[] sptr->loadBuffer;
		AllocatedRAM -= sptr->size;

		// fall back to synchronous loading
		LoadWaveFile(sptr);
		return;
	}

	sptr->loading = true;
}

#endif

static void UnloadWaveFile(MFX_Sample* sptr)
{
	if (!sptr->handle || sptr->usecount > 0)
	{
		return;
	}

	// unlink
	if (sptr->prev_lru)
	{
		sptr->prev_lru->next_lru = sptr->next_lru;
		sptr->next_lru->prev_lru = sptr->prev_lru;
		sptr->next_lru = sptr->prev_lru = NULL;
	}

	// cancel pending IO
#if ASYNC_FILE_IO
	if (sptr->loading)
	{
		CancelAsyncFile(sptr);
		sptr->loading = false;
	}
#endif

	// free
	alDeleteBuffers(1, &sptr->handle);
	sptr->handle = 0;

	AllocatedRAM -= sptr->size;
}

static void UnloadTalkFile()
{
	if (!TalkSample.handle)
	{
		return;
	}
	alDeleteBuffers(1, &TalkSample.handle);
	TalkSample.handle = 0;

	AllocatedRAM -= TalkSample.size;
}

void MFX_load_wave_list()
{
	MFX_free_wave_list();

	// free waves
	for (int ii = 0; ii < NumSamples; ii++)
	{
		if (Samples[ii].type == SMP_Music)
		{
			UnloadWaveFile(&Samples[ii]);
		}
	}
	UnloadWaveFile(&TalkSample);
}

void MFX_free_wave_list()
{
	// reset the music system
extern void MUSIC_reset();
	MUSIC_reset();

	MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);

	InitVoices();
}

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

	float	xorient = float(COS(heading)) / 65536;
	float	zorient = float(SIN(heading)) / 65536;

	ALfloat position[] = {
		LX,
		LY,
		LZ
	};
	ALfloat orientation[6] = { -xorient, 0.0f, -zorient, //front
		0.0f, 1.0f, 0.0f }; //up
	alListenerfv(AL_POSITION, position);
	alListenerfv(AL_ORIENTATION, orientation);

	// move voices so the epsilon checks
	// get made
	for (int ii = 0; ii < MAX_VOICE; ii++)
	{
		if (Voices[ii].is3D)
		{
			MoveVoice(&Voices[ii]);
		}
	}
}

void MFX_update()
{
#if ASYNC_FILE_IO
	// check for async completions
	MFX_Sample*	sptr;

	while (sptr = (MFX_Sample*)GetNextCompletedAsyncFile())
	{
		SDL_AudioSpec spec;
		Uint32 bufferSize;
		Uint8* dataBuffer;

		SDL_RWops* rwops = SDL_RWFromMem(sptr->loadBuffer, sptr->size);
		if (!SDL_LoadWAV_RW(rwops, SDL_TRUE, &spec, &dataBuffer, &bufferSize))
		{
			delete[] sptr->loadBuffer;
			AllocatedRAM -= sptr->size;

			continue;
		}
		ASSERT(SDL_AUDIO_BITSIZE(spec.format) == 16);

		delete[] sptr->loadBuffer;
		AllocatedRAM -= sptr->size;

		sptr->size = bufferSize;
		AllocatedRAM += sptr->size;

		sptr->loading = false;
		sptr->is3D = sptr->is3D && spec.channels == 1;
		alGenBuffers(1, &sptr->handle);
		alBufferData(sptr->handle, spec.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
			dataBuffer, bufferSize, spec.freq);
		SDL_FreeWAV(dataBuffer);

		// and trigger any voices that were waiting for it
		for (int ii = 0; ii < MAX_VOICE; ii++)
		{
			if (Voices[ii].smp == sptr)
			{
				FinishLoading(&Voices[ii]);				
			}
		}
	}
#endif

	for (int ii = 0; ii < MAX_VOICE; ii++)
	{
		MFX_Voice*	vptr = &Voices[ii];

		if (vptr->flags & MFX_PAIRED_TRK2)
		{
			continue;
		}
		if (!vptr->smp)
		{
			continue;
		}

		if (IsVoiceDone(vptr))
		{
			if (!vptr->queue)
			{
				FreeVoice(vptr);				
			}
			else
			{
				// get next wave from queue
				MFX_QWave*	qptr = vptr->queue;
				vptr->queue = qptr->next;

				if (qptr->flags & MFX_PAIRED_TRK1)
				{
					TriggerPairedVoice(Voices[ii].id + 1);
				}

				// free the old sample and set up the new one
				Thing*	thing = vptr->thing;
				FreeVoiceSource(vptr);
				SetupVoice(vptr, vptr->id, qptr->wave, qptr->flags & ~MFX_PAIRED_TRK2, qptr->is3D);
				vptr->thing = thing;

				// set the position
				if ((vptr->flags & MFX_MOVING) && vptr->thing)
				{
					vptr->x = vptr->thing->WorldPos.X >> 8;
					vptr->y = vptr->thing->WorldPos.Y >> 8;
					vptr->z = vptr->thing->WorldPos.Z >> 8;
				}
				else
				{
					vptr->x = qptr->x;
					vptr->y = qptr->y;
					vptr->z = qptr->z;
				}

				// relocate and play
				MoveVoice(vptr);
				PlayVoice(vptr);
				SetVoiceGain(vptr, qptr->gain);

				// release queue element
				qptr->next = QFree;
				QFree = qptr;
			}
		}
		else
		{
			if (vptr->flags & MFX_CAMERA)
			{
				vptr->x = FC_cam[0].x >> 8;
				vptr->z = FC_cam[0].z >> 8;
				if (!(vptr->flags & MFX_LOCKY))
				{
					vptr->y = FC_cam[0].y >> 8;
				}
				MoveVoice(vptr);
			}
			if ((vptr->flags & MFX_MOVING) && vptr->thing)
			{
				vptr->x = vptr->thing->WorldPos.X >> 8;
				vptr->y = vptr->thing->WorldPos.Y >> 8;
				vptr->z = vptr->thing->WorldPos.Z >> 8;
				MoveVoice(vptr);
			}
		}
	}
}

UWORD MFX_get_wave(UWORD channel_id, UBYTE index)
{
	MFX_Voice*	vptr = FindFirst(channel_id);

	while (index--)
	{
		vptr = FindNext(vptr);
	}

	return vptr ? vptr->wave : 0;
}

SLONG MFX_QUICK_play(CBYTE* str, SLONG x, SLONG y, SLONG z)
{
	return PlayTalk(str, x,y,z);
}

SLONG MFX_QUICK_still_playing()
{
	MFX_Voice*	vptr = GetVoiceForWave(0, NumSamples, 0);
	if (!vptr)
	{
		return 0;
	}

	return IsVoiceDone(vptr) ? 0 : 1;
}

void MFX_QUICK_stop()
{
	MFX_stop(0, NumSamples);
}

void MFX_QUICK_wait()
{
	while (MFX_QUICK_still_playing());
	MFX_QUICK_stop();
}

