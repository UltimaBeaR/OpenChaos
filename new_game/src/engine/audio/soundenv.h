#ifndef ENGINE_AUDIO_SOUNDENV_H
#define ENGINE_AUDIO_SOUNDENV_H

// Calculates ground quads for 3D positional audio environment geometry.
// uc_orig: SOUNDENV_precalc (fallen/Headers/soundenv.h)
void SOUNDENV_precalc(void);

// Uploads computed audio environment data (currently a stub).
// uc_orig: SOUNDENV_upload (fallen/Headers/soundenv.h)
void SOUNDENV_upload(void);

#endif // ENGINE_AUDIO_SOUNDENV_H
