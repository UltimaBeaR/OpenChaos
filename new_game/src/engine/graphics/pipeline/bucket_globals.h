#ifndef ENGINE_GRAPHICS_PIPELINE_BUCKET_GLOBALS_H
#define ENGINE_GRAPHICS_PIPELINE_BUCKET_GLOBALS_H

#include "engine/graphics/pipeline/bucket.h"

// uc_orig: e_bucket_pool (fallen/DDEngine/Source/Bucket.cpp)
extern UBYTE e_bucket_pool[BUCKET_POOL_SIZE];
// uc_orig: e_buckets (fallen/DDEngine/Source/Bucket.cpp)
extern UBYTE* e_buckets;
// uc_orig: e_end_buckets (fallen/DDEngine/Source/Bucket.cpp)
extern UBYTE* e_end_buckets;
// uc_orig: bucket_lists (fallen/DDEngine/Source/Bucket.cpp)
extern BucketHead* bucket_lists[MAX_LISTS][MAX_BUCKETS + 1];

#endif // ENGINE_GRAPHICS_PIPELINE_BUCKET_GLOBALS_H
