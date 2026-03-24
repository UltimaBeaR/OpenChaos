#ifndef ENGINE_AUDIO_MUSIC_H
#define ENGINE_AUDIO_MUSIC_H

#include "engine/core/types.h"

// uc_orig: MUSIC_MODE_SILENT (fallen/Headers/music.h)
#define MUSIC_MODE_SILENT (0)
// uc_orig: MUSIC_MODE_DRIVING (fallen/Headers/music.h)
#define MUSIC_MODE_DRIVING (1)
// uc_orig: MUSIC_MODE_SPRINTING (fallen/Headers/music.h)
#define MUSIC_MODE_SPRINTING (2)
// uc_orig: MUSIC_MODE_CRAWLING (fallen/Headers/music.h)
#define MUSIC_MODE_CRAWLING (3)
// uc_orig: MUSIC_MODE_FIGHTING (fallen/Headers/music.h)
#define MUSIC_MODE_FIGHTING (4)
// uc_orig: MUSIC_MODE_TRAIN_COMBAT (fallen/Headers/music.h)
#define MUSIC_MODE_TRAIN_COMBAT (5)
// uc_orig: MUSIC_MODE_TRAIN_DRIVING (fallen/Headers/music.h)
#define MUSIC_MODE_TRAIN_DRIVING (6)
// uc_orig: MUSIC_MODE_TRAIN_ASSAULT (fallen/Headers/music.h)
#define MUSIC_MODE_TRAIN_ASSAULT (7)
// uc_orig: MUSIC_MODE_FINAL_RECKONING (fallen/Headers/music.h)
#define MUSIC_MODE_FINAL_RECKONING (8)
// uc_orig: MUSIC_MODE_TIMER (fallen/Headers/music.h)
#define MUSIC_MODE_TIMER (9)
// uc_orig: MUSIC_MODE_GAMELOST (fallen/Headers/music.h)
#define MUSIC_MODE_GAMELOST (10)
// uc_orig: MUSIC_MODE_GAMEWON (fallen/Headers/music.h)
#define MUSIC_MODE_GAMEWON (11)
// uc_orig: MUSIC_MODE_BRIEFING (fallen/Headers/music.h)
#define MUSIC_MODE_BRIEFING (12)
// uc_orig: MUSIC_MODE_FRONTEND (fallen/Headers/music.h)
#define MUSIC_MODE_FRONTEND (13)
// uc_orig: MUSIC_MODE_CHAOS (fallen/Headers/music.h)
#define MUSIC_MODE_CHAOS (14)
// uc_orig: MUSIC_MODE_FORCE (fallen/Headers/music.h)
// Set this bit in the mode value to force an immediate switch without fade.
#define MUSIC_MODE_FORCE (128)

// Set the maximum gain (volume cap). gain is 0..255 mapped to 0..max.
// uc_orig: MUSIC_gain (fallen/Headers/music.h)
void MUSIC_gain(UBYTE gain);

// Request a music mode change. Higher-priority modes override lower ones.
// uc_orig: MUSIC_mode (fallen/Headers/music.h)
void MUSIC_mode(UBYTE mode);

// Per-frame update: handles fade-in/out transitions and actually plays the track.
// uc_orig: MUSIC_mode_process (fallen/Headers/music.h)
void MUSIC_mode_process();

// Stop playback and reset all music state between missions.
// uc_orig: MUSIC_reset (fallen/Headers/music.h)
void MUSIC_reset();

// Returns UC_TRUE if the most recently started quick-play track is still playing.
// uc_orig: MUSIC_is_playing (fallen/Headers/music.h)
extern SLONG MUSIC_is_playing(void);

// uc_orig: music_mode_override (fallen/Headers/music.h)
extern UBYTE music_mode_override;
// uc_orig: MUSIC_bodge_code (fallen/Headers/music.h)
extern UBYTE MUSIC_bodge_code;

#endif // ENGINE_AUDIO_MUSIC_H
