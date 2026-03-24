#ifndef ENGINE_LIGHTING_CACHE_H
#define ENGINE_LIGHTING_CACHE_H

#include "engine/core/types.h"

// Small fixed-size cache for per-object lighting data.
// Each entry stores a user-supplied blob (e.g. precomputed light colours)
// keyed by an integer. Used by DrawMesh objects to avoid recomputing lighting
// every frame.

// uc_orig: CACHE_Index (fallen/Headers/cache.h)
typedef UBYTE CACHE_Index;

// uc_orig: CACHE_Info (fallen/Headers/cache.h)
// Returned by CACHE_get_info; describes a cache entry.
typedef struct
{
    SLONG key;
    void* data;
    SLONG num_bytes;

} CACHE_Info;

// uc_orig: CACHE_init (fallen/Headers/cache.h)
void CACHE_init(void);

// uc_orig: CACHE_create (fallen/Headers/cache.h)
// Creates a cache entry for the given key and data blob.
// Returns the cache index, or CACHE_Index(-1) on failure.
CACHE_Index CACHE_create(
    SLONG key,
    void* data,
    UWORD num_bytes);

// uc_orig: CACHE_invalidate (fallen/Headers/cache.h)
void CACHE_invalidate(CACHE_Index c_index);
// uc_orig: CACHE_invalidate_all (fallen/Headers/cache.h)
void CACHE_invalidate_all(void);

// Flag-based selective invalidation.
// All entries start with their flag set; use CACHE_flag_clear_all + CACHE_flag_set
// on live entries, then call CACHE_invalidate_unflagged to drop stale entries.
// uc_orig: CACHE_flag_clear_all (fallen/Headers/cache.h)
void CACHE_flag_clear_all(void);
// uc_orig: CACHE_flag_set (fallen/Headers/cache.h)
void CACHE_flag_set(CACHE_Index c_index);
// uc_orig: CACHE_flag_clear (fallen/Headers/cache.h)
void CACHE_flag_clear(CACHE_Index c_index);
// uc_orig: CACHE_invalidate_unflagged (fallen/Headers/cache.h)
void CACHE_invalidate_unflagged(void);

#endif // ENGINE_LIGHTING_CACHE_H
