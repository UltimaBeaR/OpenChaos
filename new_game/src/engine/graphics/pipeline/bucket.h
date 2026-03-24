#ifndef ENGINE_GRAPHICS_PIPELINE_BUCKET_H
#define ENGINE_GRAPHICS_PIPELINE_BUCKET_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "engine/core/types.h"

// Bucket sort rendering system.
// Polygons are sorted by Z into one of MAX_LISTS render lists,
// each with up to MAX_BUCKETS depth slots.
// All bucket memory comes from a single static pool to avoid per-frame allocation.

// uc_orig: MAX_LISTS (fallen/DDEngine/Headers/Bucket.h)
#define MAX_LISTS 24
// uc_orig: MAX_BUCKETS (fallen/DDEngine/Headers/Bucket.h)
#define MAX_BUCKETS 1024
// uc_orig: BUCKET_POOL_SIZE (fallen/DDEngine/Headers/Bucket.h)
#define BUCKET_POOL_SIZE (512 * 1024)

// Render list indices — textures 0-5, then special lists at the end.
// uc_orig: TEXTURE_LIST_1 (fallen/DDEngine/Headers/Bucket.h)
#define TEXTURE_LIST_1 0
// uc_orig: TEXTURE_LIST_2 (fallen/DDEngine/Headers/Bucket.h)
#define TEXTURE_LIST_2 1
// uc_orig: TEXTURE_LIST_3 (fallen/DDEngine/Headers/Bucket.h)
#define TEXTURE_LIST_3 2
// uc_orig: TEXTURE_LIST_4 (fallen/DDEngine/Headers/Bucket.h)
#define TEXTURE_LIST_4 3
// uc_orig: TEXTURE_LIST_5 (fallen/DDEngine/Headers/Bucket.h)
#define TEXTURE_LIST_5 4
// uc_orig: TEXTURE_LIST_6 (fallen/DDEngine/Headers/Bucket.h)
#define TEXTURE_LIST_6 5

// uc_orig: LAST_NORMAL_PAGE (fallen/DDEngine/Headers/Bucket.h)
#define LAST_NORMAL_PAGE (MAX_LISTS - 8)

// uc_orig: SKY_LIST_2 (fallen/DDEngine/Headers/Bucket.h)
#define SKY_LIST_2 (MAX_LISTS - 7)
// uc_orig: SKY_LIST_1 (fallen/DDEngine/Headers/Bucket.h)
#define SKY_LIST_1 (MAX_LISTS - 6)
// uc_orig: MASK_LIST_1 (fallen/DDEngine/Headers/Bucket.h)
#define MASK_LIST_1 (MAX_LISTS - 5)
// uc_orig: COLOUR_LIST_1 (fallen/DDEngine/Headers/Bucket.h)
#define COLOUR_LIST_1 (MAX_LISTS - 4)
// uc_orig: WATER_LIST_1 (fallen/DDEngine/Headers/Bucket.h)
#define WATER_LIST_1 (MAX_LISTS - 3)
// uc_orig: ALPHA_LIST_1 (fallen/DDEngine/Headers/Bucket.h)
#define ALPHA_LIST_1 (MAX_LISTS - 2)
// uc_orig: FOG_LIST_1 (fallen/DDEngine/Headers/Bucket.h)
#define FOG_LIST_1 (MAX_LISTS - 1)

// Bucket type identifiers.
// uc_orig: BUCKET_NONE (fallen/DDEngine/Headers/Bucket.h)
#define BUCKET_NONE  0
// uc_orig: BUCKET_QUAD (fallen/DDEngine/Headers/Bucket.h)
#define BUCKET_QUAD  1
// uc_orig: BUCKET_TRI (fallen/DDEngine/Headers/Bucket.h)
#define BUCKET_TRI   2
// uc_orig: BUCKET_LINE (fallen/DDEngine/Headers/Bucket.h)
#define BUCKET_LINE  3
// uc_orig: BUCKET_POINT (fallen/DDEngine/Headers/Bucket.h)
#define BUCKET_POINT 4

// Allocate a bucket entry of type s from the pool and advance the pointer.
// uc_orig: GET_BUCKET (fallen/DDEngine/Headers/Bucket.h)
#define GET_BUCKET(s) \
    (s*)e_buckets;    \
    e_buckets += sizeof(s)

// True when the bucket pool is exhausted.
// uc_orig: BUCKETS_FULL (fallen/DDEngine/Headers/Bucket.h)
#define BUCKETS_FULL (e_buckets >= e_end_buckets)

// uc_orig: BucketHead (fallen/DDEngine/Headers/Bucket.h)
struct BucketHead {
    BucketHead* NextBucket;
    ULONG BucketType;
};

// uc_orig: BucketQuad (fallen/DDEngine/Headers/Bucket.h)
struct BucketQuad {
    BucketHead BucketHeader;
    UWORD P[4];
};

// uc_orig: BucketTri (fallen/DDEngine/Headers/Bucket.h)
struct BucketTri {
    BucketHead BucketHeader;
    UWORD P[4];
};

// uc_orig: BucketLine (fallen/DDEngine/Headers/Bucket.h)
struct BucketLine {
    BucketHead BucketHeader;
    UWORD P[2];
};

// uc_orig: BucketPoint (fallen/DDEngine/Headers/Bucket.h)
struct BucketPoint {
    BucketHead BucketHeader;
    UWORD P[2];
};

// uc_orig: e_bucket_pool (fallen/DDEngine/Source/Bucket.cpp)
extern UBYTE e_bucket_pool[BUCKET_POOL_SIZE];
// uc_orig: e_buckets (fallen/DDEngine/Source/Bucket.cpp)
extern UBYTE* e_buckets;
// uc_orig: e_end_buckets (fallen/DDEngine/Source/Bucket.cpp)
extern UBYTE* e_end_buckets;
// uc_orig: bucket_lists (fallen/DDEngine/Source/Bucket.cpp)
extern BucketHead* bucket_lists[MAX_LISTS][MAX_BUCKETS + 1];

// Reset pool pointer to beginning (call each frame).
// uc_orig: reset_buckets (fallen/DDEngine/Headers/Bucket.h)
inline void reset_buckets(void)
{
    e_buckets = e_bucket_pool;
    e_end_buckets = &e_bucket_pool[BUCKET_POOL_SIZE];
}

// Returns 1 if the D3D vertex has out-of-range sz or rhw values.
// uc_orig: check_vertex (fallen/DDEngine/Headers/Bucket.h)
inline SLONG check_vertex(D3DTLVERTEX* v)
{
    if (v->sz < 0.0 || v->sz > 1.0 || v->rhw < 0.0 || v->rhw > 1.0)
        return (1);
    else
        return (0);
}

// uc_orig: vertex_pool (fallen/DDEngine/Source/aeng.cpp)
extern D3DTLVERTEX vertex_pool[];

// Insert a bucket header into the appropriate sort list at depth z.
// uc_orig: add_bucket (fallen/DDEngine/Headers/Bucket.h)
inline void add_bucket(BucketHead* header, UBYTE bucket_type, SLONG list_index, SLONG z)
{
    header->BucketType = bucket_type;
    header->NextBucket = bucket_lists[list_index][z];
    bucket_lists[list_index][z] = header;
}

// Clear bucket_lists and reset pool pointer.
// uc_orig: init_buckets (fallen/DDEngine/Headers/Bucket.h)
void init_buckets(void);

#endif // ENGINE_GRAPHICS_PIPELINE_BUCKET_H
