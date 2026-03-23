#include "engine/graphics/geometry/fastprim_globals.h"

// uc_orig: FASTPRIM_lvert_buffer (fallen/DDEngine/Source/fastprim.cpp)
D3DLVERTEX* FASTPRIM_lvert_buffer;
// uc_orig: FASTPRIM_lvert (fallen/DDEngine/Source/fastprim.cpp)
D3DLVERTEX* FASTPRIM_lvert;
// uc_orig: FASTPRIM_lvert_max (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_lvert_max;
// uc_orig: FASTPRIM_lvert_upto (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_lvert_upto;
// uc_orig: FASTPRIM_lvert_free_end (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_lvert_free_end;
// uc_orig: FASTPRIM_lvert_free_unused (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_lvert_free_unused;

// uc_orig: FASTPRIM_index (fallen/DDEngine/Source/fastprim.cpp)
UWORD* FASTPRIM_index;
// uc_orig: FASTPRIM_index_max (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_index_max;
// uc_orig: FASTPRIM_index_upto (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_index_upto;
// uc_orig: FASTPRIM_index_free_end (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_index_free_end;
// uc_orig: FASTPRIM_index_free_unused (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_index_free_unused;

// uc_orig: FASTPRIM_matrix_buffer (fallen/DDEngine/Source/fastprim.cpp)
UBYTE FASTPRIM_matrix_buffer[sizeof(D3DMATRIX) + 32];
// uc_orig: FASTPRIM_matrix (fallen/DDEngine/Source/fastprim.cpp)
D3DMATRIX* FASTPRIM_matrix;

// uc_orig: FASTPRIM_call (fallen/DDEngine/Source/fastprim.cpp)
FASTPRIM_Call FASTPRIM_call[FASTPRIM_MAX_CALLS];
// uc_orig: FASTPRIM_call_upto (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_call_upto;

// uc_orig: FASTPRIM_prim (fallen/DDEngine/Source/fastprim.cpp)
FASTPRIM_Prim FASTPRIM_prim[FASTPRIM_MAX_PRIMS];

// uc_orig: FASTPRIM_queue (fallen/DDEngine/Source/fastprim.cpp)
UBYTE FASTPRIM_queue[FASTPRIM_MAX_QUEUE];
// uc_orig: FASTPRIM_queue_start (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_queue_start;
// uc_orig: FASTPRIM_queue_end (fallen/DDEngine/Source/fastprim.cpp)
SLONG FASTPRIM_queue_end;
