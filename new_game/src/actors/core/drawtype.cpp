#include "actors/core/drawtype.h"
#include "engine/platform/platform.h"
#include "missions/game_types.h"
#include "world/level_pools.h"

// Sentinel value stored in DrawMesh.Angle to mark the slot as free.
// uc_orig: DRAW_MESH_NULL_ANGLE (fallen/Source/drawtype.cpp)
#define DRAW_MESH_NULL_ANGLE (0xfafa)

// uc_orig: init_draw_tweens (fallen/Source/drawtype.cpp)
// Zero-initialize the DrawTween pool and mark every slot as unused.
void init_draw_tweens(void)
{
    SLONG c0;
    memset((UBYTE*)DRAW_TWEENS, 0, sizeof(DrawTween) * MAX_DRAW_TWEENS);
    DRAW_TWEEN_COUNT = 0;
    for (c0 = 0; c0 < MAX_DRAW_TWEENS; c0++) {
        DRAW_TWEENS[c0].Flags |= DT_FLAG_UNUSED;
    }
}

// uc_orig: alloc_draw_tween (fallen/Source/drawtype.cpp)
// Find an unused DrawTween slot, mark it as active, and return it.
// type parameter is the draw type constant (e.g. DT_TWEEN, DT_ROT_MULTI) — unused here,
// the caller sets DrawType on the Thing after receiving the slot.
DrawTween* alloc_draw_tween(SLONG type)
{
    SLONG c0;
    DrawTween* new_draw = 0;

    for (c0 = 0; c0 < MAX_DRAW_TWEENS; c0++) {
        if (DRAW_TWEENS[c0].Flags & DT_FLAG_UNUSED) {
            new_draw = TO_DRAW_TWEEN(c0);
            new_draw->Flags &= ~DT_FLAG_UNUSED;
            return (new_draw);
        }
    }
    ASSERT(0);
    return (0);
}

// uc_orig: free_draw_tween (fallen/Source/drawtype.cpp)
// Release a DrawTween slot back to the pool by zeroing it and setting the UNUSED flag.
void free_draw_tween(DrawTween* draw_tween)
{
    memset((UBYTE*)draw_tween, 0, sizeof(DrawTween));
    draw_tween->Flags |= DT_FLAG_UNUSED;
}

// uc_orig: init_draw_meshes (fallen/Source/drawtype.cpp)
// Initialize the DrawMesh pool by setting every slot's Angle to the NULL sentinel.
void init_draw_meshes(void)
{
    SLONG i;

    for (i = 0; i < MAX_DRAW_MESHES; i++) {
        DRAW_MESHES[i].Angle = DRAW_MESH_NULL_ANGLE;
    }

    DRAW_MESH_COUNT = 0;
}

// uc_orig: alloc_draw_mesh (fallen/Source/drawtype.cpp)
// Find a free DrawMesh slot (Angle == DRAW_MESH_NULL_ANGLE), mark it active, and return it.
// Returns NULL if the pool is exhausted.
DrawMesh* alloc_draw_mesh(void)
{
    SLONG i;

    DrawMesh* ans;

    ASSERT(DRAW_MESH_COUNT < MAX_DRAW_MESHES);

    for (i = 0; i < MAX_DRAW_MESHES; i++) {
        if (DRAW_MESHES[i].Angle == DRAW_MESH_NULL_ANGLE) {
            ans = &DRAW_MESHES[i];

            ans->Angle = 0;
            ans->Cache = NULL;
            ans->Hm = 255; // 255 means no Hm.

            return ans;
        }
    }

    return NULL;
}

// uc_orig: free_draw_mesh (fallen/Source/drawtype.cpp)
// Release a DrawMesh slot by setting its Angle back to the NULL sentinel.
void free_draw_mesh(DrawMesh* drawmesh)
{
    ASSERT(WITHIN(drawmesh, &DRAW_MESHES[0], &DRAW_MESHES[MAX_DRAW_MESHES - 1]));

    drawmesh->Angle = DRAW_MESH_NULL_ANGLE;
}
