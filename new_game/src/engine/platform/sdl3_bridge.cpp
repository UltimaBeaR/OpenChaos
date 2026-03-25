// SDL3 bridge — the ONLY translation unit that includes SDL3 headers.
// Compiled with /Zp8 (see CMakeLists.txt set_source_files_properties).
// All SDL3 interactions in the project go through functions declared in sdl3_bridge.h.

#include "engine/platform/sdl3_bridge.h"
#include <SDL3/SDL_audio.h>

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

bool sdl3_load_wav(const char* path, SDL3_WavData* out)
{
    SDL_AudioSpec spec;
    Uint8* buf;
    Uint32 len;
    if (!SDL_LoadWAV(path, &spec, &buf, &len)) {
        return false;
    }
    out->buffer = buf;
    out->size = len;
    out->channels = spec.channels;
    out->freq = spec.freq;
    return true;
}

void sdl3_free_wav(uint8_t* buffer)
{
    SDL_free(buffer);
}
