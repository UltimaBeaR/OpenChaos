#include "missions/playcuts.h"
#include "missions/playcuts_globals.h"
#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/characters/person.h"
#include "engine/effects/psystem.h"
#include "effects/environment/ribbon.h"
#include "effects/environment/dirt.h"
#include "things/items/balloon.h"
#include "effects/combat/pow.h"
#include "world/environment/puddle.h"
#include "effects/weather/drip.h"
#include "engine/audio/mfx.h"
#include "engine/audio/sound.h"
#include "things/items/grenade.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/aeng.h"
#include "world/navigation/wmove.h"

#include "ui/camera/fc.h"
#include "ui/camera/fc_globals.h"
// DRAW2D_Box migrated to draw2d.h (iteration 136).
#include "engine/graphics/pipeline/draw2d.h"

// Lerp rate for slow-motion cutscenes (frames per cutscene tick in slomo mode).
// uc_orig: SLOMO_RATE (fallen/Source/playcuts.cpp)
#define SLOMO_RATE (10)
// Fractional shift used for position/rotation interpolation weights.
// uc_orig: LERP_SHIFT (fallen/Source/playcuts.cpp)
#define LERP_SHIFT (8)

// Channel type constants.
// uc_orig: CT_CHAR (fallen/Source/playcuts.cpp)
#define CT_CHAR (1)
// uc_orig: CT_CAM (fallen/Source/playcuts.cpp)
#define CT_CAM (2)
// uc_orig: CT_WAVE (fallen/Source/playcuts.cpp)
#define CT_WAVE (3)
// uc_orig: CT_TEXT (fallen/Source/playcuts.cpp)
#define CT_TEXT (5)

// Packet type constants.
// uc_orig: PT_ANIM (fallen/Source/playcuts.cpp)
#define PT_ANIM (1)
// uc_orig: PT_WAVE (fallen/Source/playcuts.cpp)
#define PT_WAVE (3)
// uc_orig: PT_CAM (fallen/Source/playcuts.cpp)
#define PT_CAM (4)
// uc_orig: PT_TEXT (fallen/Source/playcuts.cpp)
#define PT_TEXT (5)

// Packet flag bits.
// uc_orig: PF_BACKWARDS (fallen/Source/playcuts.cpp)
#define PF_BACKWARDS (1)
// uc_orig: PF_SECURICAM (fallen/Source/playcuts.cpp)
#define PF_SECURICAM (1)
// uc_orig: PF_INTERPOLATE_MOVE (fallen/Source/playcuts.cpp)
#define PF_INTERPOLATE_MOVE (2)
// uc_orig: PF_SMOOTH_MOVE_IN (fallen/Source/playcuts.cpp)
#define PF_SMOOTH_MOVE_IN (4)
// uc_orig: PF_SMOOTH_MOVE_OUT (fallen/Source/playcuts.cpp)
#define PF_SMOOTH_MOVE_OUT (8)
// uc_orig: PF_SMOOTH_MOVE_BOTH (fallen/Source/playcuts.cpp)
#define PF_SMOOTH_MOVE_BOTH (PF_SMOOTH_MOVE_IN | PF_SMOOTH_MOVE_OUT)
// uc_orig: PF_INTERPOLATE_ROT (fallen/Source/playcuts.cpp)
#define PF_INTERPOLATE_ROT (16)
// uc_orig: PF_SMOOTH_ROT_IN (fallen/Source/playcuts.cpp)
#define PF_SMOOTH_ROT_IN (32)
// uc_orig: PF_SMOOTH_ROT_OUT (fallen/Source/playcuts.cpp)
#define PF_SMOOTH_ROT_OUT (64)
// uc_orig: PF_SMOOTH_ROT_BOTH (fallen/Source/playcuts.cpp)
#define PF_SMOOTH_ROT_BOTH (PF_SMOOTH_ROT_IN | PF_SMOOTH_ROT_OUT)
// uc_orig: PF_SLOMO (fallen/Source/playcuts.cpp)
#define PF_SLOMO (128)

// Forward declarations for functions still in old/ code.
// uc_orig: hardware_input_continue (fallen/Source/Game.cpp)
extern SLONG hardware_input_continue(void);
// uc_orig: lock_frame_rate (fallen/Source/Game.cpp)
extern void lock_frame_rate(SLONG fps);
// uc_orig: person_normal_animate (fallen/Source/playcuts.cpp)
extern SLONG person_normal_animate(Thing* p_person);

// Inline screen flip: always flush the frame (no PSX-specific guard needed).
// uc_orig: screen_flip (fallen/Source/playcuts.cpp)
inline void screen_flip(void)
{
    AENG_flip();
}

// Linear interpolation between two values.
// uc_orig: LERPValue (fallen/Source/playcuts.cpp)
inline SLONG LERPValue(SLONG a, SLONG b, SLONG m)
{
    return a + (((b - a) * m) >> LERP_SHIFT);
}

// Angle-aware LERP: wraps to take the shorter arc.
// uc_orig: LERPAngle (fallen/Source/playcuts.cpp)
inline int LERPAngle(SLONG a, SLONG b, SLONG m)
{
    if ((a - b) > 1024)
        b += 2048;
    if ((b - a) > 1024)
        a += 2048;
    return LERPValue(a, b, m);
}

// uc_orig: PLAYCUTS_Read_Packet (fallen/Source/playcuts.cpp)
static void PLAYCUTS_Read_Packet(MFFileHandle handle, CPPacket* packet)
{
    FileRead(handle, packet, sizeof(CPPacket));

    switch (packet->type) {
    case PT_TEXT:
        if (packet->pos.X) {
            SLONG l;
            FileRead(handle, &l, sizeof(l));
            FileRead(handle, PLAYCUTS_text_ptr, l);
            packet->pos.X = (SLONG)PLAYCUTS_text_ptr;
            PLAYCUTS_text_ptr += l;
            *PLAYCUTS_text_ptr = 0;
            PLAYCUTS_text_ptr++;
            PLAYCUTS_text_ctr += (l + 1);
        }
        break;
    }
}

// uc_orig: PLAYCUTS_Read_Channel (fallen/Source/playcuts.cpp)
static void PLAYCUTS_Read_Channel(MFFileHandle handle, CPChannel* channel)
{
    SLONG packnum;

    FileRead(handle, channel, sizeof(CPChannel));
    channel->packets = PLAYCUTS_packets + PLAYCUTS_packet_ctr;
    PLAYCUTS_packet_ctr += channel->packetcount;

    for (packnum = 0; packnum < channel->packetcount; packnum++)
        PLAYCUTS_Read_Packet(handle, channel->packets + packnum);
}

// uc_orig: PLAYCUTS_Read (fallen/Source/playcuts.cpp)
CPData* PLAYCUTS_Read(MFFileHandle handle)
{
    UBYTE channum;
    CPData* cutscene;

    cutscene = PLAYCUTS_cutscenes + PLAYCUTS_cutscene_ctr;
    PLAYCUTS_cutscene_ctr++;
    cutscene->channels = 0;
    FileRead(handle, &cutscene->version, 1);
    FileRead(handle, &cutscene->channelcount, 1);
    cutscene->channels = PLAYCUTS_tracks + PLAYCUTS_track_ctr;
    PLAYCUTS_track_ctr += cutscene->channelcount;
    for (channum = 0; channum < cutscene->channelcount; channum++)
        PLAYCUTS_Read_Channel(handle, cutscene->channels + channum);
    return cutscene;
}


// uc_orig: PLAYCUTS_Free (fallen/Source/playcuts.cpp)
void PLAYCUTS_Free(CPData* cutscene)
{
    // Stub — pool-allocated, use PLAYCUTS_Reset() to reclaim.
}

// uc_orig: PLAYCUTS_Reset (fallen/Source/playcuts.cpp)
void PLAYCUTS_Reset()
{
    PLAYCUTS_cutscene_ctr = 0;
    PLAYCUTS_track_ctr = 0;
    PLAYCUTS_packet_ctr = 0;
}

// Find the packet that exactly matches frame 'cell', or NULL if none.
// uc_orig: PLAYCUTS_Get_Packet (fallen/Source/playcuts.cpp)
static CPPacket* PLAYCUTS_Get_Packet(CPChannel* chan, SLONG cell)
{
    CPPacket* pkt = chan->packets;
    SLONG ctr = 1;
    if ((cell < 0) || (cell > 2000))
        return 0;
    while (pkt && (pkt->start != cell)) {
        pkt++;
        ctr++;
        if (ctr > chan->packetcount)
            pkt = 0;
    }
    return pkt;
}

// Find the nearest packets bracketing frame 'cell' from left and right.
// Returns TRUE if both sides have valid packets.
// uc_orig: PLAYCUTS_find_surrounding_packets (fallen/Source/playcuts.cpp)
static BOOL PLAYCUTS_find_surrounding_packets(CPChannel* chan, SLONG cell, SLONG* left, SLONG* right)
{
    SLONG leftmax = -1, rightmin = 2001, packctr = 0;
    CPPacket* pack;
    *left = -1;
    *right = 2001;
    pack = chan->packets;
    for (packctr = 0; packctr < chan->packetcount; packctr++, pack++) {
        if ((pack->start <= cell) && (pack->start > leftmax)) {
            leftmax = pack->start;
            *left = packctr;
        }
        if ((pack->start >= cell) && (pack->start < rightmin)) {
            rightmin = pack->start;
            *right = packctr;
        }
    }
    return ((leftmax > -1) && (rightmin < 2000));
}

// Interpolate and apply a character animation channel for the given frame.
// uc_orig: LERPAnim (fallen/Source/playcuts.cpp)
static void LERPAnim(CPChannel* chan, Thing* person, SLONG cell, SLONG sub_ctr)
{
    SLONG left, right, pos, dist, a, mult, smoothmult, smoothin, smoothout;
    int x, y, z;
    CPPacket *pktA, *pktB;
    GameCoord posn;

    PLAYCUTS_find_surrounding_packets(chan, cell, &left, &right);
    if (right == 2001)
        no_more_packets++;
    if (left < 0)
        return;
    pktA = chan->packets + left;
    pos = cell - pktA->start;

    if ((right < 2001) && (right != left)) {
        pktB = chan->packets + right;
        dist = pktB->start - pktA->start;
        mult = (pos << LERP_SHIFT);
        if (PLAYCUTS_slomo) {
            mult += (PLAYCUTS_slomo_ctr << LERP_SHIFT) / SLOMO_RATE;
        }
        mult /= dist;

        if (pktA->flags & (PF_SMOOTH_MOVE_BOTH | PF_SMOOTH_ROT_BOTH)) {
            smoothmult = (mult - 128) * 4;
            smoothmult = (SIN(smoothmult & 2047) >> 8) + 256;
            smoothmult >>= 1;
            smoothin = (mult - 256) * 2;
            smoothin = 256 + (SIN(smoothin & 2047) >> 8);
            smoothout = SIN(mult * 2) >> 8;
        }

        if (pktA->flags & PF_INTERPOLATE_MOVE) {
            if ((pktA->flags & PF_SMOOTH_MOVE_BOTH) == PF_SMOOTH_MOVE_BOTH) {
                x = LERPValue(pktA->pos.X, pktB->pos.X, smoothmult);
                y = LERPValue(pktA->pos.Y, pktB->pos.Y, smoothmult);
                z = LERPValue(pktA->pos.Z, pktB->pos.Z, smoothmult);
            } else if (pktA->flags & PF_SMOOTH_MOVE_IN) {
                x = LERPValue(pktA->pos.X, pktB->pos.X, smoothin);
                y = LERPValue(pktA->pos.Y, pktB->pos.Y, smoothin);
                z = LERPValue(pktA->pos.Z, pktB->pos.Z, smoothin);
            } else if (pktA->flags & PF_SMOOTH_MOVE_OUT) {
                x = LERPValue(pktA->pos.X, pktB->pos.X, smoothout);
                y = LERPValue(pktA->pos.Y, pktB->pos.Y, smoothout);
                z = LERPValue(pktA->pos.Z, pktB->pos.Z, smoothout);
            } else {
                x = LERPValue(pktA->pos.X, pktB->pos.X, mult);
                y = LERPValue(pktA->pos.Y, pktB->pos.Y, mult);
                z = LERPValue(pktA->pos.Z, pktB->pos.Z, mult);
            }
        } else {
            x = pktA->pos.X;
            y = pktA->pos.Y;
            z = pktA->pos.Z;
        }

        if (pktA->flags & PF_INTERPOLATE_ROT) {
            if ((pktA->flags & PF_SMOOTH_ROT_BOTH) == PF_SMOOTH_ROT_BOTH) {
                mult = smoothmult;
            } else if (pktA->flags & PF_SMOOTH_ROT_IN) {
                mult = smoothin;
            } else if (pktA->flags & PF_SMOOTH_ROT_OUT) {
                mult = smoothout;
            }
            a = LERPAngle(pktA->angle, pktB->angle, mult);
        } else {
            a = pktA->angle;
        }
        person->Draw.Tweened->Angle = a;
        posn.X = x;
        posn.Y = y;
        posn.Z = z;
        move_thing_on_map(person, &posn);
    }

    set_anim(person, pktA->index);
    if (pktA->flags & 1)
        pos = (pktA->length - pos) - 1;
    if (pos <= 0)
        return;
    TICK_RATIO = pos << TICK_SHIFT;
    if (PLAYCUTS_slomo) {
        TICK_RATIO += (PLAYCUTS_slomo_ctr << TICK_SHIFT) / SLOMO_RATE;
    }
    person_normal_animate(person);
}

// Interpolate and apply a camera channel for the given frame.
// uc_orig: LERPCamera (fallen/Source/playcuts.cpp)
static void LERPCamera(CPChannel* chan, SLONG cell, SLONG sub_ctr)
{
    SLONG x, y, z, p, a, left, right, dist, pos, mult, smoothin, smoothout, smoothmult, lens;
    CPPacket *pktA, *pktB;

    if (!PLAYCUTS_find_surrounding_packets(chan, cell, &left, &right)) {
        if (right == 2001)
            no_more_packets++;
        return;
    }

    pktA = chan->packets + left;
    pktB = chan->packets + right;
    dist = pktB->start - pktA->start;
    pos = cell - pktA->start;

    mult = (pos * 256);
    if (PLAYCUTS_slomo) {
        mult += (PLAYCUTS_slomo_ctr << LERP_SHIFT) / SLOMO_RATE;
    }
    mult /= dist;

    if (pktA->flags & (PF_SMOOTH_MOVE_BOTH | PF_SMOOTH_ROT_BOTH)) {
        smoothmult = (mult - 128) * 4;
        smoothmult = (SIN(smoothmult & 2047) >> 8) + 256;
        smoothmult >>= 1;
        smoothin = (mult - 256) * 2;
        smoothin = 256 + (SIN(smoothin & 2047) >> 8);
        smoothout = SIN(mult * 2) >> 8;
    }

    if (pktA->flags & PF_INTERPOLATE_MOVE) {
        if ((pktA->flags & PF_SMOOTH_MOVE_BOTH) == PF_SMOOTH_MOVE_BOTH) {
            x = LERPValue(pktA->pos.X, pktB->pos.X, smoothmult);
            y = LERPValue(pktA->pos.Y, pktB->pos.Y, smoothmult);
            z = LERPValue(pktA->pos.Z, pktB->pos.Z, smoothmult);
            lens = LERPValue(pktA->length & 0xff, pktB->length & 0xff, smoothmult);
        } else if (pktA->flags & PF_SMOOTH_MOVE_IN) {
            x = LERPValue(pktA->pos.X, pktB->pos.X, smoothin);
            y = LERPValue(pktA->pos.Y, pktB->pos.Y, smoothin);
            z = LERPValue(pktA->pos.Z, pktB->pos.Z, smoothin);
            lens = LERPValue(pktA->length & 0xff, pktB->length & 0xff, smoothin);
        } else if (pktA->flags & PF_SMOOTH_MOVE_OUT) {
            x = LERPValue(pktA->pos.X, pktB->pos.X, smoothout);
            y = LERPValue(pktA->pos.Y, pktB->pos.Y, smoothout);
            z = LERPValue(pktA->pos.Z, pktB->pos.Z, smoothout);
            lens = LERPValue(pktA->length & 0xff, pktB->length & 0xff, smoothout);
        } else {
            x = LERPValue(pktA->pos.X, pktB->pos.X, mult);
            y = LERPValue(pktA->pos.Y, pktB->pos.Y, mult);
            z = LERPValue(pktA->pos.Z, pktB->pos.Z, mult);
            lens = LERPValue(pktA->length & 0xff, pktB->length & 0xff, mult);
        }
    } else {
        x = pktA->pos.X;
        y = pktA->pos.Y;
        z = pktA->pos.Z;
        lens = pktA->length & 0xff;
    }
    lens = ((lens - 0x7f) * 1.5f) + 0xff;
    FC_cam[0].lens = (lens * 0x24000) >> 8;

    if (pktA->flags & PF_INTERPOLATE_ROT) {
        if ((pktA->flags & PF_SMOOTH_ROT_BOTH) == PF_SMOOTH_ROT_BOTH) {
            mult = smoothmult;
        } else if (pktA->flags & PF_SMOOTH_ROT_IN) {
            mult = smoothin;
        } else if (pktA->flags & PF_SMOOTH_ROT_OUT) {
            mult = smoothout;
        }
        p = LERPAngle(pktA->pitch, pktB->pitch, mult);
        a = LERPAngle(pktA->angle, pktB->angle, mult);
    } else {
        p = pktA->pitch;
        a = pktA->angle;
    }

    AENG_set_camera(x, y, z, (a) & 2047, (p) & 2047, 0);
    PLAYCUTS_fade_level = LERPValue(pktA->length >> 8, pktB->length >> 8, mult);
}

// Process one channel for the current frame, dispatching to anim/camera/wave handlers.
// uc_orig: PLAYCUTS_Update (fallen/Source/playcuts.cpp)
static void PLAYCUTS_Update(CPChannel* chan, Thing* thing, SLONG read_head, SLONG sub_ctr)
{
    CPPacket* pkt;
    int index, lens;
    SLONG l, r;

    pkt = PLAYCUTS_Get_Packet(chan, read_head);
    if (!pkt) {
        switch (chan->type) {
        case CT_CHAR:
            LERPAnim(chan, thing, read_head, sub_ctr);
            break;
        case CT_CAM:
            LERPCamera(chan, read_head, sub_ctr);
            break;
        default:
            PLAYCUTS_find_surrounding_packets(chan, read_head, &l, &r);
            if (r == 2001)
                no_more_packets++;
            break;
        }
        return;
    }
    switch (pkt->type) {
    case PT_ANIM:
        index = pkt->index;
        while (index > 1023) {
            index -= 1024;
        }
        if (thing) {
            move_thing_on_map(thing, &pkt->pos);
            thing->Draw.Tweened->Angle = pkt->angle;
            set_anim(thing, index);
            if (pkt->flags & 1)
                LERPAnim(chan, thing, read_head, sub_ctr);
        }
        break;

    case PT_CAM:
        AENG_set_camera(pkt->pos.X, pkt->pos.Y, pkt->pos.Z, pkt->angle & 2047, pkt->pitch & 2047, 0);
        lens = ((pkt->length & 0xff) - 0x7f);
        lens = (lens * 1.5f) + 0xff;
        FC_cam[0].lens = (lens * 0x24000) >> 8;
        PolyPage::SetGreenScreen(pkt->flags & PF_SECURICAM);
        PLAYCUTS_slomo = pkt->flags & PF_SLOMO;
        PLAYCUTS_fade_level = pkt->length >> 8;
        break;

    case PT_WAVE:
        MFX_play_xyz(MUSIC_REF + 10, pkt->index - 2048, 0, pkt->pos.X << 8, pkt->pos.Y << 8, pkt->pos.Z << 8);
        break;

    case PT_TEXT:
        if (pkt->pos.X)
            text_disp = (CBYTE*)pkt->pos.X;
        else
            text_disp = 0;
        break;
    }
}

// uc_orig: PLAYCUTS_Play (fallen/Source/playcuts.cpp)
void PLAYCUTS_Play(CPData* cutscene)
{
    typedef Thing* ThingPtr;

    UBYTE env_frame_rate = 20;
    SLONG read_head = 0, sub_ctr = 0;
    UBYTE channum;
    ThingPtr* cs_things;
    Thing* darci = NET_PERSON(0);
    CPChannel* chan;

    text_disp = 0;
    PLAYCUTS_fade_level = 255;
    PLAYCUTS_slomo = 0;
    PLAYCUTS_slomo_ctr = 0;
    PLAYCUTS_playing = 1;

    cs_things = new ThingPtr[cutscene->channelcount];
    for (channum = 0, chan = cutscene->channels; channum < cutscene->channelcount; channum++, chan++) {
        if (chan->type == CT_CHAR) {
            cs_things[channum] = TO_THING(
                create_person(
                    chan->index - 1,
                    0,
                    0,
                    0,
                    0));
        } else {
            cs_things[channum] = 0;
        }
    }

    Keys[KB_SPACE] = 0;
    LastKey = 0;
    no_more_packets = 0;

    remove_thing_from_map(darci);

    while (SHELL_ACTIVE && (!Keys[KB_SPACE]) && (!no_more_packets) && !hardware_input_continue()) {
        PARTICLE_Run();
        RIBBON_process();
        DIRT_process();
        ProcessGrenades();
        WMOVE_draw();
        BALLOON_process();
        POW_process();
        PUDDLE_process();
        DRIP_process();

        for (channum = 0; channum < cutscene->channelcount; channum++) {
            chan = cutscene->channels + channum;
            PLAYCUTS_Update(chan, cs_things[channum], read_head, sub_ctr);
        }
        if (no_more_packets < channum)
            no_more_packets = 0;

        MFX_set_listener(FC_cam[0].x, FC_cam[0].y, FC_cam[0].z, -(FC_cam[0].yaw >> 8), -(1024), -(FC_cam[0].pitch >> 8));

        AENG_draw(UC_FALSE);

        if (text_disp) {
            POLY_frame_init(UC_FALSE, UC_FALSE);
            FONT2D_DrawStringCentred(text_disp, 320, 400, 0x7fffffff, 256, POLY_PAGE_FONT2D);
            POLY_frame_draw(UC_FALSE, UC_FALSE);
        }

        if (PLAYCUTS_fade_level < 255) {
            POLY_frame_init(UC_FALSE, UC_FALSE);
            DRAW2D_Box(0, 0, 640, 480, (255 - PLAYCUTS_fade_level) << 24, 1, 255);
            POLY_frame_draw(UC_FALSE, UC_FALSE);
        }

        MFX_update();

        screen_flip();

        lock_frame_rate(env_frame_rate);

        GAME_TURN++;

        if (PLAYCUTS_slomo) {
            PLAYCUTS_slomo_ctr++;
            if (PLAYCUTS_slomo_ctr == SLOMO_RATE) {
                read_head++;
                PLAYCUTS_slomo_ctr = 0;
            }
        } else {
            read_head++;
        }
    }

    add_thing_to_map(darci);

    Keys[KB_SPACE] = 0;

    for (channum = 0, chan = cutscene->channels; channum < cutscene->channelcount; channum++, chan++) {
        if (cs_things[channum])
            THING_kill(cs_things[channum]);
    }
    delete[] cs_things;
    PLAYCUTS_playing = 0;
}
