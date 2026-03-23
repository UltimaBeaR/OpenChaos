#include "engine/graphics/pipeline/bucket_globals.h"

// uc_orig: e_bucket_pool (fallen/DDEngine/Source/Bucket.cpp)
UBYTE e_bucket_pool[BUCKET_POOL_SIZE];
// uc_orig: e_buckets (fallen/DDEngine/Source/Bucket.cpp)
UBYTE* e_buckets;
// uc_orig: e_end_buckets (fallen/DDEngine/Source/Bucket.cpp)
UBYTE* e_end_buckets;
// uc_orig: bucket_lists (fallen/DDEngine/Source/Bucket.cpp)
BucketHead* bucket_lists[MAX_LISTS][MAX_BUCKETS + 1];
