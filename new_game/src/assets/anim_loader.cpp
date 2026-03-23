#include "fallen/Headers/Game.h"   // Temporary: Game struct, ENGINE_palette macro, MFFileHandle, etc.
#include "engine/animation/anim_types.h"  // KeyFrameChunk, GameKeyFrameChunk, KeyFrameElement, etc.
#include "world/environment/prim_types.h"  // PrimObject, PrimFace3/4, PrimPoint, PRIM_OBJ_*
#include "world/environment/prim_globals.h" // prim_names[]
#include "missions/memory_globals.h" // Temporary: prim_points, prim_faces3/4, prim_objects, prim_multi_objects

#include "assets/anim_loader.h"
#include "assets/anim_loader_globals.h"
#include "assets/anim_globals.h"           // the_elements, game_chunk, anim_chunk
#include "assets/anim.h"                   // free_game_chunk
#include "assets/level_loader.h"           // change_extension, DATA_DIR, read_object_name
#include "assets/level_loader_globals.h"
#include "world/map/supermap_globals.h"    // Temporary: DONT_load (load_anim_system uses it to skip unused skins)

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// These functions are defined in memory.cpp (not yet migrated).
extern void convert_keyframe_to_pointer(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count);
extern void convert_animlist_to_pointer(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count);
extern void convert_fightcol_to_pointer(GameFightCol* p, GameFightCol* p_fight, SLONG count);
extern CBYTE* body_part_names[];
extern void write_a_prim(SLONG prim, MFFileHandle handle);
extern void add_point(SLONG x, SLONG y, SLONG z);
extern struct PrimFace4* create_a_quad(UWORD p1, UWORD p0, UWORD p3, UWORD p2, SWORD texture_style, SWORD texture_piece);
extern SLONG build_prim_object(SLONG sp, SLONG sf3, SLONG sf4);
extern void save_prim_asc(UWORD prim, UWORD version);
extern void free_game_chunk(GameKeyFrameChunk* the_chunk);

// uc_orig: load_frame_numbers (fallen/Source/io.cpp)
void load_frame_numbers(CBYTE* vue_name, UWORD* frames, SLONG max_frames)
{
    CBYTE name[200];
    SLONG len;
    FILE* f_handle;
    SLONG result = 0;
    SLONG val, val2, index = 0;

    memset((UBYTE*)frames, 0, sizeof(UWORD) * max_frames);

    strcpy(name, vue_name);
    len = strlen(name);
    name[len - 3] = 'T';
    name[len - 2] = 'X';
    name[len - 1] = 'T';

    f_handle = MF_Fopen(name, "r");
    if (f_handle) {
        CBYTE string[100];
        do {
            result = fscanf(f_handle, "%s", &string[0]);
            if (result >= 0) {
                if (string[0] == '*') {
                    // Fill all remaining slots with sequential indices.
                    for (; val < max_frames; val++) {
                        if (frames[val] == 0) {
                            frames[val] = index + 1;
                            index++;
                        }
                    }
                } else if (string[0] == '-') {
                    // Load a range: "-N M" means frames[N..M] = next sequential indices.
                    result = sscanf(&string[1], "%d", &val);
                    result = fscanf(f_handle, "%d", &val2);

                    if ((result >= 0) && (val2 < max_frames) && val < val2) {
                        for (; val <= val2; val++) {
                            frames[val] = index + 1;
                            index++;
                        }
                    }
                } else {
                    result = sscanf(&string[0], "%d", &val);

                    if ((result >= 0) && (val < max_frames)) {
                        // Record the playback position for frame 'val'.
                        frames[val] = index + 1;
                        index++;
                    }
                }
            }
        } while (result >= 0);
        MF_Fclose(f_handle);
    } else {
        // No .TXT file — load all frames in natural order.
        for (index = 0; index < max_frames; index++) {
            frames[index] = index + 1;
        }
    }
}

// uc_orig: invert_mult (fallen/Source/io.cpp)
void invert_mult(struct Matrix33* mat, struct PrimPoint* pp)
{
    Matrix33 temp_mat;
    SLONG i, j;
    SLONG x, y, z;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            temp_mat.M[i][j] = mat->M[j][i];
        }

    x = (pp->X * temp_mat.M[0][0]) + (pp->Y * temp_mat.M[0][1]) + (pp->Z * temp_mat.M[0][2]) >> 15;
    y = (pp->X * temp_mat.M[1][0]) + (pp->Y * temp_mat.M[1][1]) + (pp->Z * temp_mat.M[1][2]) >> 15;
    z = (pp->X * temp_mat.M[2][0]) + (pp->Y * temp_mat.M[2][1]) + (pp->Z * temp_mat.M[2][2]) >> 15;

    pp->X = x;
    pp->Y = y;
    pp->Z = z;
}

// uc_orig: sort_multi_object (fallen/Source/io.cpp)
void sort_multi_object(struct KeyFrameChunk* the_chunk)
{
    SLONG c0, c1, c2,
        so, eo,
        sp, ep;
    struct KeyFrameElement* the_element;
    struct PrimObject* p_obj;
    struct KeyFrame* the_keyframe;
    SLONG multi;

    // Body is fully commented out in the original — no-op.
}

// uc_orig: set_default_people_types (fallen/Source/io.cpp)
void set_default_people_types(struct KeyFrameChunk* the_chunk)
{
    SLONG c0, c1;

    for (c0 = 0; c0 < 20; c0++) {
        strcpy(the_chunk->PeopleNames[c0], "Undefined");
        for (c1 = 0; c1 < MAX_BODY_BITS; c1++) {
            the_chunk->PeopleTypes[c0].BodyPart[c1] = c1;
        }
    }
}

// uc_orig: make_compress_matrix (fallen/Source/io.cpp)
void make_compress_matrix(struct KeyFrameElement* the_element, struct Matrix33* matrix)
{
    ULONG encode;
    SLONG u, v, w;

    the_element->CMatrix.M[0] = ((((matrix->M[0][0] >> 6)) << 20) & CMAT0_MASK) + ((((matrix->M[0][1] >> 6)) << 10) & CMAT1_MASK) + ((((matrix->M[0][2] >> 6)) << 0) & CMAT2_MASK);
    the_element->CMatrix.M[1] = ((((matrix->M[1][0] >> 6)) << 20) & CMAT0_MASK) + ((((matrix->M[1][1] >> 6)) << 10) & CMAT1_MASK) + ((((matrix->M[1][2] >> 6)) << 0) & CMAT2_MASK);
    the_element->CMatrix.M[2] = ((((matrix->M[2][0] >> 6)) << 20) & CMAT0_MASK) + ((((matrix->M[2][1] >> 6)) << 10) & CMAT1_MASK) + ((((matrix->M[2][2] >> 6)) << 0) & CMAT2_MASK);
}

// uc_orig: normalise_max_matrix (fallen/Source/io.cpp)
void normalise_max_matrix(float fe_matrix[3][3], float* x, float* y, float* z)
{
    float len;
    SLONG h, w;

    len = fe_matrix[0][0] * fe_matrix[0][0];
    len += fe_matrix[0][1] * fe_matrix[0][1];
    len += fe_matrix[0][2] * fe_matrix[0][2];

    len = sqrt(len);

    for (h = 0; h < 3; h++) {
        fe_matrix[h][0] = fe_matrix[h][0] / len;
        fe_matrix[h][1] = fe_matrix[h][1] / len;
        fe_matrix[h][2] = fe_matrix[h][2] / len;
    }
}

// uc_orig: load_multi_vue (fallen/Source/io.cpp)
void load_multi_vue(struct KeyFrameChunk* the_chunk, float shrink_me)
{
    CBYTE temp_string[512],
        transform_name[32];
    SLONG c0, c1,
        last_frame = 0,
        frame = -1,
        result;
    float fe_matrix[3][3],
        fe_offset_x,
        fe_offset_y,
        fe_offset_z;
    FILE* f_handle;
    struct Matrix33 temp_matrix;
    struct KeyFrame* the_key_frame;
    struct KeyFrameElement* the_element;
    SLONG pivot;
    UWORD frame_id[4501];
    SLONG funny_fanny = 0;
    SLONG c2;

    if (the_chunk->ElementCount != 15)
        funny_fanny = 1;

    set_default_people_types(the_chunk);

    f_handle = MF_Fopen(the_chunk->VUEName, "r");
    if (f_handle) {

        the_chunk->FirstElement = &the_elements[current_element];
        the_chunk->KeyFrameCount = 0;
        load_frame_numbers(the_chunk->VUEName, frame_id, 4500);
        do {
            result = fscanf(f_handle, "%s", temp_string);
            if (!strcmp(temp_string, "frame")) {
                SLONG read_frame;
                fscanf(f_handle, "%d", &read_frame);
                if (frame_id[read_frame] == 0) {
                    // Skip this frame — not in the include list.
                } else {
                    frame = frame_id[read_frame] - 1;

                    if (frame > last_frame)
                        last_frame = frame;
                    the_key_frame = &the_chunk->KeyFrames[frame];
                    the_key_frame->ChunkID = 0;
                    the_key_frame->FrameID = frame;
                    the_key_frame->TweenStep = 4;
                    the_key_frame->ElementCount = the_chunk->ElementCount;
                    the_key_frame->FirstElement = &the_elements[current_element];
                    for (c0 = 0; c0 < the_key_frame->ElementCount;) {
                        fscanf(f_handle, "%s", temp_string);
                        if (!strcmp(temp_string, "transform")) {
                            read_object_name(f_handle, transform_name);
                            for (c2 = 0; c2 < strlen(transform_name); c2++) {
                                transform_name[c2] = tolower(transform_name[c2]);
                            }

                            if ((!strcmp(transform_name, "lfoot")) || (!strcmp(transform_name, "pivot")))
                                pivot = 1;
                            else
                                pivot = 0;

                            fscanf(f_handle, "%f%f%f", &fe_matrix[0][0], &fe_matrix[0][1], &fe_matrix[0][2]);
                            fscanf(f_handle, "%f%f%f", &fe_matrix[1][0], &fe_matrix[1][1], &fe_matrix[1][2]);
                            fscanf(f_handle, "%f%f%f", &fe_matrix[2][0], &fe_matrix[2][1], &fe_matrix[2][2]);
                            fscanf(f_handle, "%f%f%f", &fe_offset_x, &fe_offset_y, &fe_offset_z);

                            if (!strcmp(transform_name, "lfoot") || !strcmp(transform_name, "pivot")) {
                                the_key_frame->Dx = (SWORD)(fe_offset_x * shrink_me);
                                the_key_frame->Dy = (SWORD)(fe_offset_y * shrink_me);
                                the_key_frame->Dz = (SWORD)(fe_offset_z * shrink_me);
                            } else {
                                // Find which body part matches this transform name.
                                for (c1 = 0; c1 < the_chunk->ElementCount; c1++) {
                                    if (!memcmp(transform_name, prim_names[prim_multi_objects[the_chunk->MultiObject].StartObject + c1], strlen(transform_name) - 0))
                                        break;
                                }

                                if (c1 < the_chunk->ElementCount) {
                                    the_element = &the_elements[(current_element + c1)];

                                    temp_matrix.M[0][0] = (SLONG)(fe_matrix[0][0] * (1 << 15));
                                    temp_matrix.M[0][1] = (SLONG)(fe_matrix[0][1] * (1 << 15));
                                    temp_matrix.M[0][2] = (SLONG)(fe_matrix[0][2] * (1 << 15));
                                    temp_matrix.M[1][0] = (SLONG)(fe_matrix[1][0] * (1 << 15));
                                    temp_matrix.M[1][1] = (SLONG)(fe_matrix[1][1] * (1 << 15));
                                    temp_matrix.M[1][2] = (SLONG)(fe_matrix[1][2] * (1 << 15));
                                    temp_matrix.M[2][0] = (SLONG)(fe_matrix[2][0] * (1 << 15));
                                    temp_matrix.M[2][1] = (SLONG)(fe_matrix[2][1] * (1 << 15));
                                    temp_matrix.M[2][2] = (SLONG)(fe_matrix[2][2] * (1 << 15));

                                    make_compress_matrix(the_element, &temp_matrix);

                                    the_element->OffsetX = (SWORD)(fe_offset_x * shrink_me);
                                    the_element->OffsetY = (SWORD)(fe_offset_y * shrink_me);
                                    the_element->OffsetZ = (SWORD)(fe_offset_z * shrink_me);

                                    c0++;
                                }
                            }
                        }
                    }
                    current_element += the_key_frame->ElementCount;
                }
            }
        } while (result >= 0);
        MF_Fclose(f_handle);
        the_chunk->LastElement = &the_elements[current_element];
    }
    the_chunk->KeyFrameCount = last_frame;
    sort_multi_object(the_chunk);
}

// uc_orig: load_anim_mesh (fallen/Source/io.cpp)
SLONG load_anim_mesh(CBYTE* fname, float scale)
{
    SLONG ele_count;
    ele_count = load_a_multi_prim(fname);

    return (ele_count);
}

// uc_orig: load_key_frame_chunks (fallen/Source/io.cpp)
void load_key_frame_chunks(KeyFrameChunk* the_chunk, CBYTE* vue_name, float scale)
{
    SLONG c0;
    SLONG ele_count = 0;

    x_centre = 0;
    y_centre = 0;
    z_centre = 0;

    the_chunk->KeyFrameCount = 0;
    strcpy(the_chunk->VUEName, "Data\\");
    strcat(the_chunk->VUEName, vue_name);
    strcpy(the_chunk->ASCName, the_chunk->VUEName);
    strcpy(the_chunk->ANMName, the_chunk->VUEName);
    c0 = 0;
    while (the_chunk->ASCName[c0] != '.' && the_chunk->ASCName[c0] != 0)
        c0++;
    if (the_chunk->ASCName[c0] == '.') {
        the_chunk->ASCName[c0 + 1] = 'A';
        the_chunk->ASCName[c0 + 2] = 'S';
        the_chunk->ASCName[c0 + 3] = 'C';
        the_chunk->ASCName[c0 + 4] = 0;

        the_chunk->ANMName[c0 + 1] = 'A';
        the_chunk->ANMName[c0 + 2] = 'N';
        the_chunk->ANMName[c0 + 3] = 'M';
        the_chunk->ANMName[c0 + 4] = 0;
    }
    the_chunk->MultiObject = next_prim_multi_object;
    ele_count = load_anim_mesh(the_chunk->ASCName, scale);
    if (ele_count) {
        SLONG ret = 1, count = 0;

        while (ret) {
            SLONG in;

            in = '1' + count;
            the_chunk->ASCName[5] = in;
            ret = load_anim_mesh(the_chunk->ASCName, scale);
            count++;
        }
    }
    the_chunk->MultiObjectEnd = next_prim_multi_object - 1;
    the_chunk->MultiObjectStart = the_chunk->MultiObject;
    if (ele_count) {
        the_chunk->ElementCount = ele_count;

        // Centre all loaded mesh points around the mesh pivot.
        {
            SLONG multi;
            SLONG c1,
                sp, ep;
            struct PrimObject* p_obj;

            for (multi = the_chunk->MultiObjectStart; multi <= the_chunk->MultiObjectEnd; multi++)
                for (c0 = prim_multi_objects[multi].StartObject; c0 < prim_multi_objects[multi].EndObject; c0++) {
                    p_obj = &prim_objects[c0];
                    sp = p_obj->StartPoint;
                    ep = p_obj->EndPoint;

                    for (c1 = sp; c1 < ep; c1++) {
                        prim_points[c1].X -= x_centre;
                        prim_points[c1].Y -= y_centre;
                        prim_points[c1].Z -= z_centre;
                    }
                }
        }

        load_multi_vue(the_chunk, scale);
    }
}

// uc_orig: read_a_prim (fallen/Source/io.cpp)
void read_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type)
{
    SLONG c0;
    SLONG sf3, ef3, sf4, ef4, sp, ep;
    SLONG dp;

    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&prim_names[next_prim_object], 32);

        FileRead(handle, (UBYTE*)&sp, sizeof(sp));
        FileRead(handle, (UBYTE*)&ep, sizeof(ep));

        ASSERT(ep - sp >= 0 && ep - sp < 10000);

        dp = next_prim_point - sp;

        // Read vertex data: old format uses 32-bit SLONG coords; newer uses 16-bit SWORD.
        if (save_type <= 3) {
            struct OldPrimPoint {
                SLONG X, Y, Z;
            };
            for (c0 = sp; c0 < ep; c0++) {
                OldPrimPoint op;
                FileRead(handle, (UBYTE*)&op, sizeof(OldPrimPoint));
                prim_points[next_prim_point].X = (SWORD)op.X;
                prim_points[next_prim_point].Y = (SWORD)op.Y;
                prim_points[next_prim_point].Z = (SWORD)op.Z;
                next_prim_point++;
            }
        } else {
            for (c0 = sp; c0 < ep; c0++) {
                FileRead(handle, (UBYTE*)&prim_points[next_prim_point], sizeof(struct PrimPoint));
                next_prim_point++;
            }
        }

        FileRead(handle, (UBYTE*)&sf3, sizeof(sf3));
        FileRead(handle, (UBYTE*)&ef3, sizeof(ef3));
        ASSERT(ef3 - sf3 >= 0 && ef3 - sf3 < 10000);
        for (c0 = sf3; c0 < ef3; c0++) {
            SLONG c1;
            FileRead(handle, (UBYTE*)&prim_faces3[next_prim_face3], sizeof(struct PrimFace3));
            for (c1 = 0; c1 < 3; c1++)
                prim_faces3[next_prim_face3].Points[c1] += dp;
            next_prim_face3++;
        }

        FileRead(handle, (UBYTE*)&sf4, sizeof(sf4));
        FileRead(handle, (UBYTE*)&ef4, sizeof(ef4));
        ASSERT(ef4 - sf4 >= 0 && ef4 - sf4 < 10000);
        for (c0 = sf4; c0 < ef4; c0++) {
            SLONG c1;
            FileRead(handle, (UBYTE*)&prim_faces4[next_prim_face4], sizeof(struct PrimFace4));
            for (c1 = 0; c1 < 4; c1++)
                prim_faces4[next_prim_face4].Points[c1] += dp;
            // Animated mesh sub-objects are never walkable.
            prim_faces4[next_prim_face4].FaceFlags &= ~FACE_FLAG_WALKABLE;
            next_prim_face4++;
        }

        prim_objects[next_prim_object].StartPoint = next_prim_point - (ep - sp);
        prim_objects[next_prim_object].EndPoint = next_prim_point;
        prim_objects[next_prim_object].StartFace3 = next_prim_face3 - (ef3 - sf3);
        prim_objects[next_prim_object].EndFace3 = next_prim_face3;
        prim_objects[next_prim_object].StartFace4 = next_prim_face4 - (ef4 - sf4);
        prim_objects[next_prim_object].EndFace4 = next_prim_face4;
        next_prim_object++;
    }
}

// uc_orig: load_a_multi_prim (fallen/Source/io.cpp)
SLONG load_a_multi_prim(CBYTE* name)
{
    SLONG c0;
    MFFileHandle handle;
    SLONG save_type = 0;
    SLONG so, eo;
    CBYTE ext_name[80];

    change_extension(name, "moj", ext_name);
    handle = FileOpen(ext_name);
    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

        FileRead(handle, (UBYTE*)&so, sizeof(so));
        FileRead(handle, (UBYTE*)&eo, sizeof(eo));

        prim_multi_objects[next_prim_multi_object].StartObject = next_prim_object;
        prim_multi_objects[next_prim_multi_object].EndObject = next_prim_object + (eo - so);
        for (c0 = so; c0 < eo; c0++)
            read_a_prim(c0, handle, save_type);

        FileClose(handle);
        next_prim_multi_object++;
        return (next_prim_multi_object - 1);
    } else
        return (0);
}

// uc_orig: find_matching_face (fallen/Source/io.cpp)
SLONG find_matching_face(struct PrimPoint* p1, struct PrimPoint* p2, struct PrimPoint* p3, UWORD prim)
{
    SLONG c0, sf, ef, point;
    sf = prim_objects[prim].StartFace4;
    ef = prim_objects[prim].EndFace4;

    for (c0 = sf; c0 <= ef; c0++) {
        if (prim_points[prim_faces4[c0].Points[0]].X == p1->X && prim_points[prim_faces4[c0].Points[0]].Y == p1->Y && prim_points[prim_faces4[c0].Points[0]].Z == p1->Z && prim_points[prim_faces4[c0].Points[1]].X == p2->X && prim_points[prim_faces4[c0].Points[1]].Y == p2->Y && prim_points[prim_faces4[c0].Points[1]].Z == p2->Z && prim_points[prim_faces4[c0].Points[2]].X == p3->X && prim_points[prim_faces4[c0].Points[2]].Y == p3->Y && prim_points[prim_faces4[c0].Points[2]].Z == p3->Z) {
            return (c0);
        }
    }
    return (-1);
}

// uc_orig: create_kline_bottle (fallen/Source/io.cpp)
void create_kline_bottle(void)
{
    float x, y, z, u, v;
    float sqrt_2, a = 1.0;
    struct PrimFace4* p_f4;

    float step = PI / 10.0;

    // Disabled in original — mesh generation code is fully commented out.
    return;
}

// uc_orig: load_palette (fallen/Source/io.cpp)
void load_palette(CBYTE* palette)
{
    FILE* handle;

    handle = MF_Fopen(palette, "rb");

    if (handle == NULL) {
        goto file_error;
    }

    if (fread(ENGINE_palette, 1, sizeof(ENGINE_palette), handle) != sizeof(ENGINE_palette)) {
        MF_Fclose(handle);
        goto file_error;
    }

    MF_Fclose(handle);

    return;

file_error:;

    SLONG i;

    for (i = 0; i < 256; i++) {
        ENGINE_palette[i].red = rand();
        ENGINE_palette[i].green = rand();
        ENGINE_palette[i].blue = rand();
    }
}

// uc_orig: save_insert_a_multi_prim (fallen/Source/io.cpp)
SLONG save_insert_a_multi_prim(MFFileHandle handle, SLONG multi)
{
    // Not implemented in this build — save path is disabled.
    return (0);
}

// uc_orig: save_insert_game_chunk (fallen/Source/io.cpp)
SLONG save_insert_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk)
{
    // Not implemented in this build.
    return (1);
}

// uc_orig: save_anim_system (fallen/Source/io.cpp)
SLONG save_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name)
{
    // Not implemented in this build.
    return (0);
}

// uc_orig: load_insert_game_chunk (fallen/Source/io.cpp)
SLONG load_insert_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk)
{
    SLONG save_type = 0, c0;
    SLONG temp;
    ULONG addr1, addr2, a_off, ae_off;
    ULONG af_off, addr3;
    UWORD check;

    struct GameKeyFrameElementBig* temp_mem;

    FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

    ASSERT(save_type > 1);

    FileRead(handle, (UBYTE*)&p_chunk->ElementCount, sizeof(p_chunk->ElementCount));

    FileRead(handle, (UBYTE*)&p_chunk->MaxPeopleTypes, sizeof(p_chunk->MaxPeopleTypes));
    FileRead(handle, (UBYTE*)&p_chunk->MaxAnimFrames, sizeof(p_chunk->MaxAnimFrames));
    FileRead(handle, (UBYTE*)&p_chunk->MaxElements, sizeof(p_chunk->MaxElements));
    FileRead(handle, (UBYTE*)&p_chunk->MaxKeyFrames, sizeof(p_chunk->MaxKeyFrames));
    FileRead(handle, (UBYTE*)&p_chunk->MaxFightCols, sizeof(p_chunk->MaxFightCols));

    if (p_chunk->MaxPeopleTypes)
        p_chunk->PeopleTypes = (struct BodyDef*)MemAlloc(MAX_PEOPLE_TYPES * sizeof(struct BodyDef));

    if (p_chunk->MaxKeyFrames)
        p_chunk->AnimKeyFrames = (struct GameKeyFrame*)MemAlloc((p_chunk->MaxKeyFrames + 500) * sizeof(struct GameKeyFrame));

    if (p_chunk->MaxAnimFrames)
        p_chunk->AnimList = (struct GameKeyFrame**)MemAlloc((p_chunk->MaxAnimFrames + 200) * sizeof(struct GameKeyFrame*));

    if (p_chunk->MaxElements)
        p_chunk->TheElements = (struct GameKeyFrameElement*)MemAlloc((p_chunk->MaxElements + 500 * 15) * sizeof(struct GameKeyFrameElement));

    temp_mem = (struct GameKeyFrameElementBig*)MemAlloc(p_chunk->MaxElements * sizeof(struct GameKeyFrameElementBig));

    if (p_chunk->MaxFightCols)
        p_chunk->FightCols = (struct GameFightCol*)MemAlloc((p_chunk->MaxFightCols + 50) * sizeof(struct GameFightCol));

    // Load the people types
    FileRead(handle, (UBYTE*)&p_chunk->PeopleTypes[0], sizeof(struct BodyDef) * p_chunk->MaxPeopleTypes);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Load the keyframe linked lists
    FileRead(handle, (UBYTE*)&addr1, sizeof(addr1));
    FileRead(handle, (UBYTE*)&p_chunk->AnimKeyFrames[0], sizeof(struct GameKeyFrame) * p_chunk->MaxKeyFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Load the anim elements
    FileRead(handle, (UBYTE*)&addr2, sizeof(addr2));
    FileRead(handle, (UBYTE*)&p_chunk->TheElements[0], sizeof(struct GameKeyFrameElement) * p_chunk->MaxElements);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Load the anim list
    FileRead(handle, (UBYTE*)&p_chunk->AnimList[0], sizeof(struct GameKeyFrame*) * p_chunk->MaxAnimFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    FileRead(handle, (UBYTE*)&addr3, sizeof(addr3));

    FileRead(handle, (UBYTE*)&p_chunk->FightCols[0], sizeof(struct GameFightCol) * p_chunk->MaxFightCols);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    if (save_type < 3) {
        // Convert old tween step counts to step sizes.
        for (c0 = 0; c0 < p_chunk->MaxKeyFrames; c0++) {
            p_chunk->AnimKeyFrames[c0].TweenStep = (256 / (p_chunk->AnimKeyFrames[c0].TweenStep + 1)) >> 1;
        }
    }

    if (save_type > 4) {
        // Newer format: pointers stored as index offsets, use helpers to fix them up.
        convert_keyframe_to_pointer(p_chunk->AnimKeyFrames, p_chunk->TheElements, p_chunk->FightCols, p_chunk->MaxKeyFrames);
        convert_animlist_to_pointer(p_chunk->AnimList, p_chunk->AnimKeyFrames, p_chunk->MaxAnimFrames);
        convert_fightcol_to_pointer(p_chunk->FightCols, p_chunk->FightCols, p_chunk->MaxFightCols);
    } else {
        // Older format: stored pointers are original runtime addresses.
        // Relocate them by computing the offset from old base to new allocation.
        a_off = ((ULONG)&p_chunk->AnimKeyFrames[0]) - addr1;
        ae_off = ((ULONG)&p_chunk->TheElements[0]) - addr2;
        af_off = ((ULONG)&p_chunk->FightCols[0]) - addr3;
        for (c0 = 0; c0 < p_chunk->MaxKeyFrames; c0++) {
            ULONG a;

            a = (ULONG)p_chunk->AnimKeyFrames[c0].NextFrame;
            a += a_off;
            if (p_chunk->AnimKeyFrames[c0].NextFrame)
                p_chunk->AnimKeyFrames[c0].NextFrame = (struct GameKeyFrame*)a;

            a = (ULONG)p_chunk->AnimKeyFrames[c0].PrevFrame;
            a += a_off;
            if (p_chunk->AnimKeyFrames[c0].PrevFrame)
                p_chunk->AnimKeyFrames[c0].PrevFrame = (struct GameKeyFrame*)a;

            a = (ULONG)p_chunk->AnimKeyFrames[c0].FirstElement;
            a += ae_off;
            p_chunk->AnimKeyFrames[c0].FirstElement = (struct GameKeyFrameElement*)a;

            a = (ULONG)p_chunk->AnimKeyFrames[c0].Fight;
            if (a != 0 && a < (ULONG)addr3) {
                a = 0;
                p_chunk->AnimKeyFrames[c0].Fight = 0;
            }
            a += af_off;

            if (p_chunk->AnimKeyFrames[c0].Fight) {
                struct GameFightCol* p_fight;
                ULONG offset;

                offset = (ULONG)p_chunk->AnimKeyFrames[c0].Fight;
                offset -= (ULONG)addr3;
                offset /= sizeof(struct GameFightCol);

                p_chunk->AnimKeyFrames[c0].Fight = (struct GameFightCol*)a;

                p_fight = (struct GameFightCol*)a;
                p_fight->Next = 0;

                while (p_fight->Next) {
                    offset = (ULONG)p_fight->Next;
                    offset -= (ULONG)addr3;
                    offset /= sizeof(struct GameFightCol);
                    if (offset > p_chunk->MaxFightCols) {
                        p_fight->Next = 0;
                    } else {
                        a = (ULONG)p_fight->Next;
                        a += af_off;
                        p_fight->Next = 0;
                        p_fight = p_fight->Next;
                    }
                }
            }
        }

        a_off = ((ULONG)&p_chunk->AnimKeyFrames[0]) - addr1;
        for (c0 = 0; c0 < p_chunk->MaxAnimFrames; c0++) {
            ULONG a;

            a = (ULONG)p_chunk->AnimList[c0];
            a += a_off;
            p_chunk->AnimList[c0] = (struct GameKeyFrame*)a;
        }
    }

    MemFree((void*)temp_mem);
    return (1);
}

// uc_orig: load_insert_a_multi_prim (fallen/Source/io.cpp)
SLONG load_insert_a_multi_prim(MFFileHandle handle)
{
    SLONG c0;
    SLONG save_type = 0;
    SLONG so, eo;

    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

        FileRead(handle, (UBYTE*)&so, sizeof(so));
        FileRead(handle, (UBYTE*)&eo, sizeof(eo));

        prim_multi_objects[next_prim_multi_object].StartObject = next_prim_object;
        prim_multi_objects[next_prim_multi_object].EndObject = next_prim_object + (eo - so);
        for (c0 = so; c0 < eo; c0++)
            read_a_prim(c0, handle, save_type);

        next_prim_multi_object++;
        return (next_prim_multi_object - 1);
    } else
        return (0);
}

// uc_orig: load_anim_system (fallen/Source/io.cpp)
SLONG load_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG type)
{
    SLONG c0, point;
    MFFileHandle handle;
    SLONG save_type = 0;
    CBYTE ext_name[100];
    SLONG count;
    UBYTE load;

    CBYTE fname[100];

    sprintf(fname, "%sdata\\%s", DATA_DIR, name);

    free_game_chunk(p_chunk);

    change_extension(fname, "all", ext_name);
    handle = FileOpen(ext_name);
    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

        if (save_type > 2) {

            FileRead(handle, (UBYTE*)&count, sizeof(count));

            for (c0 = 0; c0 < count; c0++) {
                switch (type) {
                case 0:
                    p_chunk->MultiObject[c0] = load_insert_a_multi_prim(handle);
                    break;
                case 1:
                    // people.all: always load Darci (slot 0), conditionally load others.
                    switch (c0) {
                    case 0: // darci
                        load = 1;
                        break;
                    case 1: // slag
                        load = !(DONT_load & (1 << PERSON_SLAG_TART));
                        break;
                    case 2: // fat slag
                        load = !(DONT_load & (1 << PERSON_SLAG_FATUGLY));
                        break;
                    case 3: // hostage
                        load = !(DONT_load & (1 << PERSON_HOSTAGE));
                        break;
                    default:
                        load = 1;
                        break;
                    }
                    if (load)
                        p_chunk->MultiObject[c0] = load_insert_a_multi_prim(handle);
                    else {
                        skip_load_a_multi_prim(handle);
                        p_chunk->MultiObject[c0] = 1;
                    }
                    break;
                case 2:
                    // thug.all: slots 0=rasta 1=grey 2=red 3=miller 4=cop 5=mib 6=tramp 7=civ variety
                    switch (c0) {
                    case 0: // rasta
                    case 1: // grey
                    case 2: // red
                    case 4: // cop
                    case 7: // civ variety
                        load = 1;
                        break;
                    case 3: // miller (mechanic)
                        load = !(DONT_load & (1 << PERSON_MECHANIC));
                        break;
                    case 5: // MIB
                        load = !(DONT_load & (1 << PERSON_MIB1));
                        break;
                    case 6: // tramp
                        load = !(DONT_load & (1 << PERSON_TRAMP));
                        break;
                    default:
                        load = 1;
                        break;
                    }
                    if (load)
                        p_chunk->MultiObject[c0] = load_insert_a_multi_prim(handle);
                    else {
                        skip_load_a_multi_prim(handle);
                        p_chunk->MultiObject[c0] = 1;
                    }
                    break;
                }
            }
            p_chunk->MultiObject[c0] = 0;
        } else {
            p_chunk->MultiObject[0] = load_insert_a_multi_prim(handle);
            p_chunk->MultiObject[1] = 0;
        }
        load_insert_game_chunk(handle, p_chunk);

        FileClose(handle);
        return (1);
    }
    return (0);
}

// uc_orig: load_append_game_chunk (fallen/Source/io.cpp)
SLONG load_append_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk, SLONG start_frame)
{
    SLONG save_type = 0, c0;
    SLONG temp;
    ULONG addr1, addr2, a_off, ae_off;
    ULONG af_off, addr3;
    UWORD check;

    struct GameKeyFrameElementBig* temp_mem;

    SLONG ElementCount;
    SWORD MaxPeopleTypes;
    SWORD MaxKeyFrames;
    SWORD MaxAnimFrames;
    SWORD MaxFightCols;
    SLONG MaxElements;

    ULONG error;

    FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

    ASSERT(save_type > 1);

    FileRead(handle, (UBYTE*)&ElementCount, sizeof(p_chunk->ElementCount));
    FileRead(handle, (UBYTE*)&MaxPeopleTypes, sizeof(p_chunk->MaxPeopleTypes));
    FileRead(handle, (UBYTE*)&MaxAnimFrames, sizeof(p_chunk->MaxAnimFrames));
    FileRead(handle, (UBYTE*)&MaxElements, sizeof(p_chunk->MaxElements));
    FileRead(handle, (UBYTE*)&MaxKeyFrames, sizeof(p_chunk->MaxKeyFrames));
    FileRead(handle, (UBYTE*)&MaxFightCols, sizeof(p_chunk->MaxFightCols));

    // Skip the PeopleTypes block — already loaded when the .all was first loaded.
    FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct BodyDef) * MaxPeopleTypes);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Append keyframes after the existing block.
    FileRead(handle, (UBYTE*)&addr1, sizeof(addr1));
    FileRead(handle, (UBYTE*)&p_chunk->AnimKeyFrames[p_chunk->MaxKeyFrames], sizeof(struct GameKeyFrame) * MaxKeyFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Append elements.
    FileRead(handle, (UBYTE*)&addr2, sizeof(addr2));
    FileRead(handle, (UBYTE*)&p_chunk->TheElements[p_chunk->MaxElements], sizeof(struct GameKeyFrameElement) * MaxElements);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Append anim list starting at start_frame.
    FileRead(handle, (UBYTE*)&p_chunk->AnimList[start_frame], sizeof(struct GameKeyFrame*) * MaxAnimFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    FileRead(handle, (UBYTE*)&addr3, sizeof(addr3));

    // Append fight collision volumes.
    FileRead(handle, (UBYTE*)&p_chunk->FightCols[p_chunk->MaxFightCols], sizeof(struct GameFightCol) * MaxFightCols);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    if (save_type > 4) {
        convert_keyframe_to_pointer(&p_chunk->AnimKeyFrames[p_chunk->MaxKeyFrames], &p_chunk->TheElements[p_chunk->MaxElements], &p_chunk->FightCols[p_chunk->MaxFightCols], MaxKeyFrames);
        convert_animlist_to_pointer(&p_chunk->AnimList[start_frame], &p_chunk->AnimKeyFrames[p_chunk->MaxKeyFrames], MaxAnimFrames);
        convert_fightcol_to_pointer(&p_chunk->FightCols[p_chunk->MaxFightCols], &p_chunk->FightCols[p_chunk->MaxFightCols], MaxFightCols);
    }

    p_chunk->MaxAnimFrames = start_frame + MaxAnimFrames;
    p_chunk->MaxElements += MaxElements;
    p_chunk->MaxKeyFrames += MaxKeyFrames;
    p_chunk->MaxFightCols += MaxFightCols;

    return (1);
}

// uc_orig: skip_a_prim (fallen/Source/io.cpp)
void skip_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type)
{
    SLONG c0;
    SLONG sf3, ef3, sf4, ef4, sp, ep;
    SLONG dp;

    if (handle != FILE_OPEN_ERROR) {
        FileSeek(handle, SEEK_MODE_CURRENT, 32); // skip object name

        FileRead(handle, (UBYTE*)&sp, sizeof(sp));
        FileRead(handle, (UBYTE*)&ep, sizeof(ep));

        ASSERT(ep - sp >= 0 && ep - sp < 10000);

        for (c0 = sp; c0 < ep; c0++) {
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimPoint));
        }

        FileRead(handle, (UBYTE*)&sf3, sizeof(sf3));
        FileRead(handle, (UBYTE*)&ef3, sizeof(ef3));
        ASSERT(ef3 - sf3 >= 0 && ef3 - sf3 < 10000);
        for (c0 = sf3; c0 < ef3; c0++) {
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimFace3));
        }

        FileRead(handle, (UBYTE*)&sf4, sizeof(sf4));
        FileRead(handle, (UBYTE*)&ef4, sizeof(ef4));
        ASSERT(ef4 - sf4 >= 0 && ef4 - sf4 < 10000);
        for (c0 = sf4; c0 < ef4; c0++) {
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimFace4));
        }
    }
}

// uc_orig: skip_load_a_multi_prim (fallen/Source/io.cpp)
void skip_load_a_multi_prim(MFFileHandle handle)
{
    SLONG c0;
    SLONG save_type = 0;
    SLONG so, eo;

    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

        FileRead(handle, (UBYTE*)&so, sizeof(so));
        FileRead(handle, (UBYTE*)&eo, sizeof(eo));

        for (c0 = so; c0 < eo; c0++)
            skip_a_prim(c0, handle, save_type);
    }
}

// uc_orig: append_anim_system (fallen/Source/io.cpp)
SLONG append_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG start_anim, SLONG load_mesh)
{
    SLONG c0, point;
    MFFileHandle handle;
    SLONG save_type = 0;
    CBYTE ext_name[100];
    SLONG count;

    CBYTE fname[100];

    sprintf(fname, "%sdata\\%s", DATA_DIR, name);

    change_extension(fname, "all", ext_name);
    handle = FileOpen(ext_name);
    if (handle != FILE_OPEN_ERROR) {
        SLONG start_ob = 0;

        while (p_chunk->MultiObject[start_ob]) {
            start_ob++;
        }

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));
        FileRead(handle, (UBYTE*)&count, sizeof(count));

        ASSERT(start_ob + count < 11);

        for (c0 = 0; c0 < count; c0++) {
            if (load_mesh)
                p_chunk->MultiObject[start_ob + c0] = load_insert_a_multi_prim(handle);
            else
                skip_load_a_multi_prim(handle);
        }
        if (c0 + start_ob < 10)
            p_chunk->MultiObject[c0 + start_ob] = 0;

        load_append_game_chunk(handle, p_chunk, start_anim);

        FileClose(handle);
        return (1);
    }
    return (0);
}
