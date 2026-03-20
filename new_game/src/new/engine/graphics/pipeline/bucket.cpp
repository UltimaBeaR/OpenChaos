#include "engine/graphics/pipeline/bucket.h"
#include "engine/graphics/pipeline/bucket_globals.h"
#include <string.h>

// uc_orig: init_buckets (fallen/DDEngine/Source/Bucket.cpp)
void init_buckets(void)
{
    memset(bucket_lists, 0, sizeof(bucket_lists));
    reset_buckets();
}
