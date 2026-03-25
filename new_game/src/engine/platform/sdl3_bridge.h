#pragma once

// SDL3 bridge — the ONLY file in the project that includes SDL3 headers.
//
// The project compiles with /Zp1 (1-byte struct packing) for binary resource
// format compatibility.  SDL3 headers contain static_asserts that fail under
// /Zp1, so sdl3_bridge.cpp is compiled with /Zp8.  All other code accesses
// SDL3 functionality exclusively through functions declared here.

#include <cstdint>

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

struct SDL3_WavData {
    uint8_t* buffer;
    uint32_t size;
    int channels;
    int freq;
};

bool sdl3_load_wav(const char* path, SDL3_WavData* out);
void sdl3_free_wav(uint8_t* buffer);
