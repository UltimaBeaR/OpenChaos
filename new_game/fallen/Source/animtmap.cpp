#include "game.h"
#include "animtmap.h"

struct AnimTmap anim_tmaps[MAX_ANIM_TMAPS]; // 2656 bytes


void sync_animtmaps(void)
{
    SLONG c0;
    struct AnimTmap* p_anim;
    p_anim = &anim_tmaps[1];
    for (c0 = 1; c0 < MAX_ANIM_TMAPS; c0++) {
        if (p_anim->Flags) {
            p_anim->Current = 0;
            p_anim->Timer = 0;
        }
        p_anim++;
    }
}

void animate_texture_maps(void)
{

    SLONG c0;
    struct AnimTmap* p_anim;
    p_anim = &anim_tmaps[1];
    for (c0 = 1; c0 < MAX_ANIM_TMAPS; c0++) {
        if (p_anim->Flags) {
            if (p_anim->Timer++ > p_anim->Delay[p_anim->Current]) {
                p_anim->Timer = 0;
                //				LogText(" anim %d does a next frame\n ",c0);
                if (p_anim->Current == 15 || !(p_anim->Flags & (1 << p_anim->Current + 1))) {
                    p_anim->Current = 0;
                } else
                    p_anim->Current++;
            }
        }
        p_anim++;
    }
}
void load_animtmaps(void)
{
    MFFileHandle handle;
    SLONG how_many;
    SLONG save_type;

    handle = FileOpen("data/tmap.ani");
    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, 4);
        FileRead(handle, (UBYTE*)&how_many, 4);
        if (how_many >= MAX_ANIM_TMAPS)
            how_many = MAX_ANIM_TMAPS - 1;
        FileRead(handle, (UBYTE*)&anim_tmaps[0], sizeof(struct AnimTmap) * how_many);
        FileClose(handle);
    }
    sync_animtmaps();
}

void save_animtmaps(void)
{
    MFFileHandle handle;
    SLONG how_many = MAX_ANIM_TMAPS;
    SLONG save_type = 1;

    handle = FileCreate("data/tmap.ani", 1);
    if (handle != FILE_OPEN_ERROR) {

        FileWrite(handle, (UBYTE*)&save_type, 4);
        FileWrite(handle, (UBYTE*)&how_many, 4);
        FileWrite(handle, (UBYTE*)&anim_tmaps[0], sizeof(struct AnimTmap) * how_many);
        FileClose(handle);
    }
}
