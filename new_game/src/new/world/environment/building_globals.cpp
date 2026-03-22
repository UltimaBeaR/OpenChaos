#include "world/environment/building_globals.h"

// uc_orig: edit_map (fallen/Source/Building.cpp)
// Editor-only tile map (BUILD_MODE_EDITOR). In BUILD_MODE_DX the PAP supermap is used instead.
struct DepthStrip edit_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH]; // 2MB

// uc_orig: tex_map (fallen/Source/Building.cpp)
// Editor-only texture override map. If non-zero, overrides edit_map texture for that tile.
UWORD tex_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

// uc_orig: edit_map_roof_height (fallen/Source/Building.cpp)
SBYTE edit_map_roof_height[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

// uc_orig: start_point (fallen/Source/Building.cpp)
SLONG start_point[200];

// uc_orig: next_roof_bound (fallen/Source/Building.cpp)
// Already declared extern in fallen/Headers/building.h; defined here.
UWORD next_roof_bound = 1;

// uc_orig: background_prim (fallen/Source/Building.cpp)
UWORD background_prim = 0;

// uc_orig: end_prim_point (fallen/Source/Building.cpp)
// Initialized at build time to MAX_PRIM_POINTS - 2 after save_table is set up.
UWORD end_prim_point = 0;

// uc_orig: end_prim_face4 (fallen/Source/Building.cpp)
UWORD end_prim_face4 = 0;

// uc_orig: end_prim_face3 (fallen/Source/Building.cpp)
UWORD end_prim_face3 = MAX_PRIM_FACES3 - 2;

// uc_orig: end_prim_object (fallen/Source/Building.cpp)
UWORD end_prim_object = MAX_PRIM_OBJECTS - 2;

// uc_orig: end_prim_multi_object (fallen/Source/Building.cpp)
UWORD end_prim_multi_object = MAX_PRIM_MOBJECTS - 2;

// uc_orig: next_building_object (fallen/Source/Building.cpp)
UWORD next_building_object = 1;

// uc_orig: end_building_object (fallen/Source/Building.cpp)
UWORD end_building_object = MAX_BUILDING_OBJECTS - 2;

// uc_orig: next_building_facet (fallen/Source/Building.cpp)
UWORD next_building_facet = 1;

// uc_orig: end_building_facet (fallen/Source/Building.cpp)
UWORD end_building_facet = MAX_BUILDING_FACETS - 2;

// uc_orig: diff_page_count1 (fallen/Source/Building.cpp)
UWORD diff_page_count1 = 0;

// uc_orig: diff_page_count2 (fallen/Source/Building.cpp)
UWORD diff_page_count2 = 0;

// uc_orig: page_count (fallen/Source/Building.cpp)
UWORD page_count[64 * 8];

// uc_orig: window_list (fallen/Source/Building.cpp)
struct FWindow window_list[MAX_WINDOWS];

// uc_orig: wall_list (fallen/Source/Building.cpp)
struct FWall wall_list[MAX_WALLS];

// uc_orig: storey_list (fallen/Source/Building.cpp)
struct FStorey storey_list[MAX_STOREYS];

// uc_orig: building_list (fallen/Source/Building.cpp)
struct FBuilding building_list[MAX_BUILDINGS];

// uc_orig: building_facets (fallen/Source/Building.cpp)
struct BuildingFacet building_facets[MAX_BUILDING_FACETS];

// uc_orig: building_objects (fallen/Source/Building.cpp)
struct BuildingObject building_objects[MAX_BUILDING_OBJECTS];

// uc_orig: build_mode (fallen/Source/Building.cpp)
// Controls which data source to use: BUILD_MODE_EDITOR or BUILD_MODE_DX.
SLONG build_mode = 1;

// uc_orig: room_ids (fallen/Source/Building.cpp)
struct RoomID room_ids[MAX_INSIDE_STOREYS];

// uc_orig: next_inside (fallen/Source/Building.cpp)
// Next free slot in room_ids[]. Starts at 1 (0 = invalid/null).
SLONG next_inside = 1;

// uc_orig: floor_texture_sizes (fallen/Source/Building.cpp)
UWORD floor_texture_sizes[] = {
    16, 32, 64, 128
};

// uc_orig: textures_xy (fallen/Source/Building.cpp)
struct TXTY textures_xy[200][5];

// uc_orig: dx_textures_xy (fallen/Source/Building.cpp)
struct DXTXTY dx_textures_xy[200][5];

// uc_orig: textures_flags (fallen/Source/Building.cpp)
UBYTE textures_flags[200][5];

// uc_orig: WindowCount (fallen/Source/Building.cpp)
SLONG WindowCount;

// uc_orig: block_min_x (fallen/Source/Building.cpp)
SLONG block_min_x;

// uc_orig: block_max_x (fallen/Source/Building.cpp)
SLONG block_max_x;

// uc_orig: edge_pool_ptr (fallen/Source/Building.cpp)
struct Edge* edge_pool_ptr = 0;

// uc_orig: build_x (fallen/Source/Building.cpp)
SLONG build_x;

// uc_orig: build_y (fallen/Source/Building.cpp)
SLONG build_y;

// uc_orig: build_z (fallen/Source/Building.cpp)
SLONG build_z;

// uc_orig: build_min_y (fallen/Source/Building.cpp)
SLONG build_min_y;

// uc_orig: build_max_y (fallen/Source/Building.cpp)
SLONG build_max_y;

// Internal edge-list scanner state — originally file-scope statics in Building.cpp.
// Exposed here (non-static) so that all chunks (building.cpp + old/Building.cpp) can share them.

// uc_orig: edge_heads_ptr (fallen/Source/Building.cpp)
UWORD* edge_heads_ptr = 0;

// uc_orig: next_edge (fallen/Source/Building.cpp)
ULONG next_edge;

// uc_orig: edge_min_z (fallen/Source/Building.cpp)
ULONG edge_min_z;

// uc_orig: flag_blocks (fallen/Source/Building.cpp)
SLONG* flag_blocks = 0;

// uc_orig: flag_blocks2 (fallen/Source/Building.cpp)
SLONG* flag_blocks2 = 0;

// uc_orig: cut_blocks (fallen/Source/Building.cpp)
UWORD* cut_blocks = 0;

// uc_orig: global_y (fallen/Source/Building.cpp)
SLONG global_y;

// uc_orig: build_seed (fallen/Source/Building.cpp)
// LCG seed for the building geometry deterministic PRNG. Initially 0x12345678.
SLONG build_seed = 0x12345678;

// uc_orig: current_building (fallen/Source/Building.cpp)
SLONG current_building = 0;

// uc_orig: lookup_roof (fallen/Source/Building.cpp)
// Roof auto-tiling table: 256 entries indexed by 8-neighbour occupancy bitmask.
// Each entry: low 8 bits = texture atlas index, high 2 bits = rotation (ROT1/2/3).
UWORD lookup_roof[256] = {
    // Index 0: no neighbours — isolated tile
    62,
    // Index 1
    62,
    // Index 2
    58,
    // Index 3
    58,
    // Index 4
    62,
    // Index 5
    62,
    // Index 6
    58,
    // Index 7
    58,
    // Index 8
    58 + ROT3,
    // Index 9
    58 + ROT3,
    // Index 10
    63,
    // Index 11
    55,
    // Index 12
    58 + ROT3,
    // Index 13
    58 + ROT3,
    // Index 14
    63,
    // Index 15
    55,
    // Index 16
    58 + ROT1,
    // Index 17
    58 + ROT1,
    // Index 18
    63 + ROT1,
    // Index 19
    63 + ROT1,
    // Index 20
    58 + ROT1,
    // Index 21
    58 + ROT1,
    // Index 22
    55 + ROT1,
    // Index 23
    55 + ROT1,
    // Index 24
    57,
    // Index 25
    57,
    // Index 26
    65,
    // Index 27
    61,
    // Index 28
    57,
    // Index 29
    57,
    // Index 30
    60,
    // Index 31
    54,
    // Index 32
    62,
    // Index 33
    62,
    // Index 34
    58,
    // Index 35
    58,
    // Index 36
    62,
    // Index 37
    62,
    // Index 38
    58,
    // Index 39
    58,
    // Index 40
    58 + ROT3,
    // Index 41
    58 + ROT3,
    // Index 42
    63,
    // Index 43
    55,
    // Index 44
    58 + ROT3,
    // Index 45
    58 + ROT3,
    // Index 46
    63,
    // Index 47
    55,
    // Index 48
    58 + ROT1,
    // Index 49
    58 + ROT1,
    // Index 50
    63 + ROT1,
    // Index 51
    63 + ROT1,
    // Index 52
    58 + ROT1,
    // Index 53
    58 + ROT1,
    // Index 54
    55 + ROT1,
    // Index 55
    55 + ROT1,
    // Index 56
    57,
    // Index 57
    57,
    // Index 58
    60,
    // Index 59
    61,
    // Index 60
    57,
    // Index 61
    57,
    // Index 62
    60,
    // Index 63
    54,
    // Index 64
    58 + ROT2,
    // Index 65
    58 + ROT2,
    // Index 66
    57 + ROT1,
    // Index 67
    57 + ROT1,
    // Index 68
    58 + ROT2,
    // Index 69
    58 + ROT2,
    // Index 70
    57 + ROT1,
    // Index 71
    57 + ROT1,
    // Index 72
    63 + ROT3,
    // Index 73
    63 + ROT3,
    // Index 74
    0,
    // Index 75
    60 + ROT3,
    // Index 76
    63 + ROT3,
    // Index 77
    63 + ROT3,
    // Index 78
    0,
    // Index 79
    60 + ROT3,
    // Index 80
    63 + ROT2,
    // Index 81
    63 + ROT2,
    // Index 82
    0,
    // Index 83
    0,
    // Index 84
    63 + ROT2,
    // Index 85
    63 + ROT2,
    // Index 86
    61 + ROT1,
    // Index 87
    61 + ROT1,
    // Index 88
    65 + ROT2,
    // Index 89
    65 + ROT2,
    // Index 90
    0,
    // Index 91
    0,
    // Index 92
    65 + ROT2,
    // Index 93
    65 + ROT2,
    // Index 94
    0,
    // Index 95
    59,
    // Index 96
    58 + ROT2,
    // Index 97
    58 + ROT2,
    // Index 98
    57 + ROT1,
    // Index 99
    57 + ROT1,
    // Index 100
    58 + ROT2,
    // Index 101
    58 + ROT2,
    // Index 102
    57 + ROT1,
    // Index 103
    57 + ROT1,
    // Index 104
    55 + ROT3,
    // Index 105
    55 + ROT3,
    // Index 106
    61 + ROT3,
    // Index 107
    54 + ROT3,
    // Index 108
    55 + ROT3,
    // Index 109
    55 + ROT3,
    // Index 110
    61 + ROT3,
    // Index 111
    54 + ROT3,
    // Index 112
    63 + ROT2,
    // Index 113
    63 + ROT2,
    // Index 114
    65 + ROT1,
    // Index 115
    65 + ROT1,
    // Index 116
    63 + ROT2,
    // Index 117
    63 + ROT2,
    // Index 118
    61 + ROT1,
    // Index 119
    61 + ROT1,
    // Index 120
    60 + ROT2,
    // Index 121
    60 + ROT2,
    // Index 122
    0,
    // Index 123
    59 + ROT3,
    // Index 124
    60 + ROT2,
    // Index 125
    60 + ROT2,
    // Index 126
    64,
    // Index 127
    56,
    // Index 128
    62,
    // Index 129
    62,
    // Index 130
    58,
    // Index 131
    58,
    // Index 132
    62,
    // Index 133
    62,
    // Index 134
    58,
    // Index 135
    58,
    // Index 136
    58 + ROT3,
    // Index 137
    58 + ROT2,
    // Index 138
    63,
    // Index 139
    55,
    // Index 140
    58 + ROT3,
    // Index 141
    58,
    // Index 142
    63,
    // Index 143
    55,
    // Index 144
    58 + ROT1,
    // Index 145
    58 + ROT1,
    // Index 146
    63 + ROT1,
    // Index 147
    63 + ROT1,
    // Index 148
    58 + ROT1,
    // Index 149
    58 + ROT1,
    // Index 150
    55 + ROT1,
    // Index 151
    55 + ROT1,
    // Index 152
    57,
    // Index 153
    57,
    // Index 154
    65,
    // Index 155
    61,
    // Index 156
    57,
    // Index 157
    57,
    // Index 158
    60,
    // Index 159
    54,
    // Index 160
    62,
    // Index 161
    62,
    // Index 162
    58,
    // Index 163
    58,
    // Index 164
    62,
    // Index 165
    62,
    // Index 166
    58,
    // Index 167
    58,
    // Index 168
    58 + ROT3,
    // Index 169
    58 + ROT3,
    // Index 170
    63,
    // Index 171
    55,
    // Index 172
    58 + ROT3,
    // Index 173
    58 + ROT3,
    // Index 174
    63,
    // Index 175
    55,
    // Index 176
    58 + ROT1,
    // Index 177
    58 + ROT1,
    // Index 178
    63 + ROT1,
    // Index 179
    63 + ROT1,
    // Index 180
    58 + ROT1,
    // Index 181
    58 + ROT1,
    // Index 182
    55 + ROT1,
    // Index 183
    55 + ROT1,
    // Index 184
    57,
    // Index 185
    57,
    // Index 186
    65,
    // Index 187
    61,
    // Index 188
    57,
    // Index 189
    57,
    // Index 190
    60,
    // Index 191
    54,
    // Index 192
    58 + ROT2,
    // Index 193
    58 + ROT2,
    // Index 194
    57 + ROT1,
    // Index 195
    57 + ROT1,
    // Index 196
    58 + ROT2,
    // Index 197
    58 + ROT2,
    // Index 198
    57 + ROT1,
    // Index 199
    57 + ROT1,
    // Index 200
    63 + ROT3,
    // Index 201
    63 + ROT3,
    // Index 202
    61 + ROT3,
    // Index 203
    60 + ROT3,
    // Index 204
    63 + ROT3,
    // Index 205
    63 + ROT3,
    // Index 206
    65 + ROT3,
    // Index 207
    60 + ROT3,
    // Index 208
    55 + ROT2,
    // Index 209
    55 + ROT2,
    // Index 210
    60 + ROT1,
    // Index 211
    60 + ROT1,
    // Index 212
    55 + ROT2,
    // Index 213
    55 + ROT2,
    // Index 214
    54 + ROT1,
    // Index 215
    54 + ROT1,
    // Index 216
    61 + ROT2,
    // Index 217
    61 + ROT2,
    // Index 218
    0,
    // Index 219
    64 + ROT1,
    // Index 220
    61 + ROT2,
    // Index 221
    61 + ROT2,
    // Index 222
    58 + ROT1,
    // Index 223
    56 + ROT1,
    // Index 224
    58 + ROT2,
    // Index 225
    58 + ROT2,
    // Index 226
    57 + ROT1,
    // Index 227
    57 + ROT1,
    // Index 228
    58 + ROT2,
    // Index 229
    58 + ROT2,
    // Index 230
    57 + ROT1,
    // Index 231
    57 + ROT1,
    // Index 232
    55 + ROT3,
    // Index 233
    55 + ROT3,
    // Index 234
    61 + ROT3,
    // Index 235
    54 + ROT3,
    // Index 236
    55 + ROT3,
    // Index 237
    55 + ROT3,
    // Index 238
    61 + ROT3,
    // Index 239
    54 + ROT3,
    // Index 240
    55 + ROT2,
    // Index 241
    55 + ROT2,
    // Index 242
    60 + ROT1,
    // Index 243
    60 + ROT1,
    // Index 244
    55 + ROT2,
    // Index 245
    55 + ROT2,
    // Index 246
    54 + ROT1,
    // Index 247
    54 + ROT1,
    // Index 248
    54 + ROT2,
    // Index 249
    54 + ROT2,
    // Index 250
    59 + ROT2,
    // Index 251
    56 + ROT3,
    // Index 252
    54 + ROT2,
    // Index 253
    54 + ROT2,
    // Index 254
    56 + ROT2,
    // Index 255: all neighbours occupied — interior tile
    37,
};
