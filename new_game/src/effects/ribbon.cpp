#include "game.h"
#include "effects/ribbon.h"
#include "effects/ribbon_globals.h"
#include "engine/graphics/pipeline/poly.h"

// Helper: constructs a GameCoord inline.
// uc_orig: Coord (fallen/Source/ribbon.cpp)
static inline GameCoord Coord(SLONG x, SLONG y, SLONG z)
{
    GameCoord bob = { x, y, z };
    return bob;
}

// uc_orig: RIBBON_draw_ribbon (fallen/DDEngine/Source/drawxtra.cpp)
// Renders a single ribbon as a triangle strip using POLY_add_triangle.
// Iterates the circular point buffer from Tail to Head, building overlapping triangles
// with UV coordinates derived from point index and fade based on age (ctr vs FadePoint).
void RIBBON_draw_ribbon(Ribbon* ribbon)
{
    POLY_Point pp[3];
    POLY_Point* tri[3] = { &pp[0], &pp[1], &pp[2] };
    UBYTE i, p, ctr;
    SLONG id, spread;
    float vo, vs;

    i = ribbon->Tail;
    p = ctr = 0;
    vo = ribbon->Scroll;
    vo *= 0.015625f;
    vs = ribbon->TextureV;
    vs = 1.0f / vs;
    do {
        POLY_transform(ribbon->Points[i].X, ribbon->Points[i].Y, ribbon->Points[i].Z, &pp[p]);
        if (ribbon->Flags & RIBBON_FLAG_FADE) {
            if (ribbon->Flags & RIBBON_FLAG_IALPHA)
                id = ((256 * ctr) / ribbon->FadePoint);
            else
                id = 255 - ((256 * ctr) / ribbon->FadePoint);
            if (id < 0)
                id = 0;
            if (id > 255)
                id = 255;
            pp[p].colour = (id << 24) | ribbon->RGB;
        } else {
            pp[p].colour = (((SLONG)ribbon->FadePoint) << 24) | ribbon->RGB;
        }
        pp[p].specular = 0xFF000000;
        if (i & 1) {
            pp[p].u = 0;
        } else {
            pp[p].u = ribbon->TextureU;
        }
        pp[p].v = i;
        pp[p].v *= vs;
        pp[p].v += vo;
        p++;
        i++;
        ctr++;
        if (i == ribbon->Size)
            i = 0;
        if (p == 3)
            p = 0;
        if (ctr > 2) {
            if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                POLY_add_triangle(tri, ribbon->Page, UC_FALSE);
            }
        }
        ASSERT(ctr < MAX_RIBBON_SIZE);
    } while (i != ribbon->Head);
}

// uc_orig: RIBBON_draw (fallen/Source/ribbon.cpp)
void RIBBON_draw()
{
    SLONG i;
    for (i = 0; i < MAX_RIBBONS; i++)
        if ((ribbon_Ribbons[i].Flags & RIBBON_FLAG_USED) && (RIBBON_length(i + 1) > 2))
            RIBBON_draw_ribbon(&ribbon_Ribbons[i]);
}

// uc_orig: RIBBON_process (fallen/Source/ribbon.cpp)
void RIBBON_process()
{
    SLONG i, j;

    for (i = 0; i < MAX_RIBBONS; i++)
        if (ribbon_Ribbons[i].Flags & RIBBON_FLAG_USED) {
            ribbon_Ribbons[i].Scroll += ribbon_Ribbons[i].SlideSpeed;
            if (ribbon_Ribbons[i].Flags & RIBBON_FLAG_CONVECT)
                for (j = 0; j < ribbon_Ribbons[i].Size; j++)
                    ribbon_Ribbons[i].Points[j].Y += 22;
            if (ribbon_Ribbons[i].Life > 0)
                ribbon_Ribbons[i].Life--;
            if (!ribbon_Ribbons[i].Life)
                RIBBON_free(i);
        }
}

// uc_orig: RIBBON_init (fallen/Source/ribbon.cpp)
void RIBBON_init()
{
    memset((UBYTE*)ribbon_Ribbons, 0, sizeof(ribbon_Ribbons));
}

// uc_orig: RIBBON_alloc (fallen/Source/ribbon.cpp)
SLONG RIBBON_alloc(SLONG flags, UBYTE max_segments, SLONG page, SLONG life, UBYTE fade, UBYTE scroll, UBYTE u, UBYTE v, SLONG rgb)
{
    static SLONG NextRibbon = 0;
    SLONG LastRibbon;

    LastRibbon = NextRibbon;
    while (ribbon_Ribbons[NextRibbon++].Flags) {
        if (NextRibbon == MAX_RIBBONS)
            NextRibbon = 0;
        if (NextRibbon == LastRibbon)
            return 0;
    }

    if (max_segments > MAX_RIBBON_SIZE)
        max_segments = MAX_RIBBON_SIZE;
    ribbon_Ribbons[NextRibbon - 1].Size = max_segments;
    ribbon_Ribbons[NextRibbon - 1].Page = page;
    ribbon_Ribbons[NextRibbon - 1].RGB = rgb;
    ribbon_Ribbons[NextRibbon - 1].Life = life;
    ribbon_Ribbons[NextRibbon - 1].Flags = flags | RIBBON_FLAG_USED;
    ribbon_Ribbons[NextRibbon - 1].Scroll = 0;
    ribbon_Ribbons[NextRibbon - 1].FadePoint = fade;
    ribbon_Ribbons[NextRibbon - 1].SlideSpeed = scroll;
    if (!v)
        v = max_segments;
    ribbon_Ribbons[NextRibbon - 1].TextureU = u;
    ribbon_Ribbons[NextRibbon - 1].TextureV = v;

    return NextRibbon;
}

// uc_orig: RIBBON_free (fallen/Source/ribbon.cpp)
void RIBBON_free(SLONG ribbon)
{
    ribbon_Ribbons[ribbon].Flags = 0;
    ribbon_Ribbons[ribbon].Head = ribbon_Ribbons[ribbon].Tail = 0;
}

// uc_orig: RIBBON_extend (fallen/Source/ribbon.cpp)
void RIBBON_extend(SLONG ribbon, SLONG x, SLONG y, SLONG z)
{
    ribbon--; // 1 based so that "if (!RIBBON_alloc())" is okay
    if (!(ribbon_Ribbons[ribbon].Flags & RIBBON_FLAG_USED))
        return;

    // Don't extend ribbons if we are paused.
    extern SLONG GAMEMENU_menu_type;
    if (GAMEMENU_menu_type != 0 /*GAMEMENU_MENU_TYPE_NONE*/) {
        return;
    }

    ribbon_Ribbons[ribbon].Points[ribbon_Ribbons[ribbon].Head] = Coord(x, y, z);
    ribbon_Ribbons[ribbon].Head++;
    if (ribbon_Ribbons[ribbon].Head == ribbon_Ribbons[ribbon].Size)
        ribbon_Ribbons[ribbon].Head = 0;
    if (ribbon_Ribbons[ribbon].Head == ribbon_Ribbons[ribbon].Tail) {
        ribbon_Ribbons[ribbon].Tail++;
        if (ribbon_Ribbons[ribbon].Tail == ribbon_Ribbons[ribbon].Size)
            ribbon_Ribbons[ribbon].Tail = 0;
    }
}

// uc_orig: RIBBON_length (fallen/Source/ribbon.cpp)
SLONG RIBBON_length(SLONG ribbon)
{
    SLONG len;
    ribbon--; // 1 based so that "if (!RIBBON_alloc())" is okay
    if (ribbon_Ribbons[ribbon].Flags & RIBBON_FLAG_USED) {
        len = ribbon_Ribbons[ribbon].Head - ribbon_Ribbons[ribbon].Tail;
        if (len < 0)
            len += ribbon_Ribbons[ribbon].Size;
        return len + 1;
    }
    return 0;
}

// uc_orig: RIBBON_life (fallen/Source/ribbon.cpp)
void RIBBON_life(SLONG ribbon, SLONG life)
{
    ribbon--; // 1 based so that "if (!RIBBON_alloc())" is okay
    if (ribbon_Ribbons[ribbon].Flags & RIBBON_FLAG_USED) {
        ribbon_Ribbons[ribbon].Life = life;
    }
}
