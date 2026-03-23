#ifndef ENGINE_EFFECTS_FLAMENGINE_H
#define ENGINE_EFFECTS_FLAMENGINE_H

#include "core/types.h"
#include "engine/io/file.h"

// 2D fire/flame effect engine. Simulates a 256x256 cellular automaton (fire field)
// and uploads the result each frame to a Direct3D texture page (POLY_PAGE_MENUFLAME).
// Used for the main menu background flame effect.

// uc_orig: FlameXY2 (fallen/DDEngine/Headers/flamengine.h)
// Byte-packed 2D position within the 256x256 fire field.
struct FlameXY2 {
    UBYTE x, y;
};

// uc_orig: FlameXY (fallen/DDEngine/Headers/flamengine.h)
// Alias for FlameXY2, also accessible as a flat 16-bit offset into the field.
union FlameXY {
    FlameXY2 loc;
    UWORD ofs;
};

// uc_orig: FlameParticle (fallen/DDEngine/Headers/flamengine.h)
// One "emitter" in the flame field — sets a pixel value and optionally moves.
struct FlameParticle {
    FlameXY pos;
    UBYTE jx, jy;    // jitter range (random position offset each frame)
    UBYTE ex, ey;    // emitter origin (used to reset wandering particles)
    UBYTE life;      // on/off flag (nonzero = active)
    SWORD pulse;     // current brightness value (0..255 range)
    UBYTE prate;     // pulse speed
    UBYTE pmode;     // pulse mode: 0=ramp up, 1=ramp down, 2/3=cycle
    UBYTE wmode;     // walk mode: 0=static, 1=fountain, 2=cascade, etc.
    ULONG pstart;    // pulse range start
    ULONG pend;      // pulse range end
};

// uc_orig: FlameParams (fallen/DDEngine/Headers/flamengine.h)
// Configuration blob loaded from a .pal fire preset file.
struct FlameParams {
    UBYTE blur, dark, convec;    // blur enable, darkening enable, convection mode
    UBYTE palette[768];          // 256-entry RGB palette for fire colourisation
    UWORD free, posn;            // particle pool state
    UBYTE randomize;             // extra randomness flag (selects ConvectionBlur2)
    FlameParticle particles[2000];
};

// uc_orig: Flamengine (fallen/DDEngine/Headers/flamengine.h)
// Fire simulation object. One instance per active fire effect.
class Flamengine {
private:
    UBYTE data[256 * 256 * 2];   // fire field (double-buffered)
    UBYTE work[256 * 256 * 2];   // working buffer for blur pass

    // uc_orig: Flamengine::AddParticles (fallen/DDEngine/Headers/flamengine.h)
    void AddParticles();
    // uc_orig: Flamengine::AddParticles2 (fallen/DDEngine/Headers/flamengine.h)
    void AddParticles2();
    // uc_orig: Flamengine::Feedback (fallen/DDEngine/Headers/flamengine.h)
    void Feedback();
    // uc_orig: Flamengine::ConvectionBlur (fallen/DDEngine/Headers/flamengine.h)
    void ConvectionBlur();
    // uc_orig: Flamengine::ConvectionBlur2 (fallen/DDEngine/Headers/flamengine.h)
    void ConvectionBlur2();
    // uc_orig: Flamengine::Darkening (fallen/DDEngine/Headers/flamengine.h)
    void Darkening();
    // uc_orig: Flamengine::UpdateTexture (fallen/DDEngine/Headers/flamengine.h)
    void UpdateTexture();
    // uc_orig: Flamengine::ReadHeader (fallen/DDEngine/Headers/flamengine.h)
    void ReadHeader(MFFileHandle handle);
    // uc_orig: Flamengine::ReadParts (fallen/DDEngine/Headers/flamengine.h)
    void ReadParts(MFFileHandle handle);
    // uc_orig: Flamengine::BlitOffset (fallen/DDEngine/Headers/flamengine.h)
    void BlitOffset();

public:
    // uc_orig: Flamengine::params (fallen/DDEngine/Headers/flamengine.h)
    FlameParams params;

    // uc_orig: Flamengine::Flamengine (fallen/DDEngine/Headers/flamengine.h)
    Flamengine(char* name);
    // uc_orig: Flamengine::~Flamengine (fallen/DDEngine/Headers/flamengine.h)
    ~Flamengine();

    // uc_orig: Flamengine::Run (fallen/DDEngine/Headers/flamengine.h)
    // Advances the simulation one frame (add particles, blur/darken, upload to texture).
    void Run();

    // uc_orig: Flamengine::Blit (fallen/DDEngine/Headers/flamengine.h)
    // Draws the flame texture as a full-screen quad.
    void Blit();

    // uc_orig: Flamengine::BlitHalf (fallen/DDEngine/Headers/flamengine.h)
    // Draws the flame texture as a half-screen quad (left or right side).
    void BlitHalf(CBYTE side);
};

#endif // ENGINE_EFFECTS_FLAMENGINE_H
