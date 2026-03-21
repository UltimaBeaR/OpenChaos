// Remaining anim/save/load functions from io.cpp.
// First chunk (globals + map/prim loaders) migrated to new/assets/level_loader.cpp.
#include "game.h"
#include "..\headers\mapthing.h"
#include "pap.h"
#include "ob.h"
#include "supermap.h"
#include "io.h"
#include "eway.h"
#include "..\headers\inside2.h"
#include "memory.h"
#include "math.h"

void skip_load_a_multi_prim(MFFileHandle handle);

extern CBYTE texture_style_names[200][21];
extern void fix_style_names(void);
SLONG load_a_multi_prim(CBYTE* name);
void create_kline_bottle(void);
SLONG load_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG type = 0);

//---------------------------------------------------------------
SLONG key_frame_count, current_element;
SLONG x_centre, y_centre, z_centre;

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
                    //
                    // load the rest of the frames
                    //
                    for (; val < max_frames; val++) {
                        if (frames[val] == 0) {
                            frames[val] = index + 1;
                            index++;
                        }
                        //							else
                        //								ASSERT(0);
                    }
                } else if (string[0] == '-') {
                    //
                    // load a range of frames
                    //
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
                        //
                        // This records the order for frames, so if we need to know where 24 should be in the list you simply inquire at frames[24]
                        //
                        //
                        frames[val] = index + 1;
                        index++;
                    }
                }
            }
        } while (result >= 0);
        MF_Fclose(f_handle);
    } else {
        for (index = 0; index < max_frames; index++) {
            frames[index] = index + 1;
        }
    }
}

void invert_mult(struct Matrix33* mat, struct PrimPoint* pp)
{
    Matrix33 temp_mat;
    SLONG i, j;
    SLONG x, y, z;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            temp_mat.M[i][j] = mat->M[j][i];
        }

    //	LogText(" len before %d \n",SDIST3(pp->X,pp->Y,pp->Z));

    x = (pp->X * temp_mat.M[0][0]) + (pp->Y * temp_mat.M[0][1]) + (pp->Z * temp_mat.M[0][2]) >> 15;
    y = (pp->X * temp_mat.M[1][0]) + (pp->Y * temp_mat.M[1][1]) + (pp->Z * temp_mat.M[1][2]) >> 15;
    z = (pp->X * temp_mat.M[2][0]) + (pp->Y * temp_mat.M[2][1]) + (pp->Z * temp_mat.M[2][2]) >> 15;
    //	LogText(" len after %d \n",SDIST3(x,y,z));

    pp->X = x;
    pp->Y = y;
    pp->Z = z;
}

extern CBYTE* body_part_names[];
void sort_multi_object(struct KeyFrameChunk* the_chunk)
{
    SLONG c0, c1, c2,
        so, eo,
        sp, ep;
    struct KeyFrameElement* the_element;
    struct PrimObject* p_obj;
    struct KeyFrame* the_keyframe;
    SLONG multi;

    // LogText(" key frame count %d \n",key_frame_count);
    /*
          for(c0=0;c0<key_frame_count;c0++)
          {
                  the_keyframe=&the_chunk->KeyFrames[c0];
                  the_element	=	the_keyframe->FirstElement;
  //		LogText(" frame %d  dxyz (%d,%d,%d) elementcount %d\n",c0,the_keyframe->Dx,the_keyframe->Dy,the_keyframe->Dz,the_keyframe->ElementCount);
                  for(c1=0;c1<the_keyframe->ElementCount;c1++,the_element++)
                  {
                          the_element->OffsetX-=the_keyframe->Dx<<2;
  // 			the_element->OffsetY-=the_keyframe->Dy<<2;
                          the_element->OffsetZ-=the_keyframe->Dz<<2;
                          the_element->Parent=c0;
                  }
          }
          */
}

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

void make_compress_matrix(struct KeyFrameElement* the_element, struct Matrix33* matrix)
{
    ULONG encode;
    SLONG u, v, w;
    /*

            u=(((the_element->CMatrix.M[0]&CMAT0_MASK)<<2)>>22);
            v=(((the_element->CMatrix.M[0]&CMAT1_MASK)<<12)>>22);
            w=(((the_element->CMatrix.M[0]&CMAT2_MASK)<<22)>>22);


    */
    the_element->CMatrix.M[0] = ((((matrix->M[0][0] >> 6)) << 20) & CMAT0_MASK) + ((((matrix->M[0][1] >> 6)) << 10) & CMAT1_MASK) + ((((matrix->M[0][2] >> 6)) << 0) & CMAT2_MASK);
    the_element->CMatrix.M[1] = ((((matrix->M[1][0] >> 6)) << 20) & CMAT0_MASK) + ((((matrix->M[1][1] >> 6)) << 10) & CMAT1_MASK) + ((((matrix->M[1][2] >> 6)) << 0) & CMAT2_MASK);
    the_element->CMatrix.M[2] = ((((matrix->M[2][0] >> 6)) << 20) & CMAT0_MASK) + ((((matrix->M[2][1] >> 6)) << 10) & CMAT1_MASK) + ((((matrix->M[2][2] >> 6)) << 0) & CMAT2_MASK);
}

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

    //	*x=*x/len;
    //	*y=*y/len;
    //	*z=*z/len;
}

//************************************************************************************************
//************************************************************************************************

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
    UWORD frame_id[4501]; // more than 3000 frames, I don't think so.
    SLONG funny_fanny = 0;
    SLONG c2;

    //! JCL delete me
    //	jcl_setup();

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
                // LogText("read %d, pos %d\n",read_frame,frame_id[read_frame]);
                if (frame_id[read_frame] == 0) {
                    // skip this frame because it aint in list
                    //					LogText(" skipping %d \n",read_frame);

                } else {
                    // frame++;

                    frame = frame_id[read_frame] - 1;

                    if (frame == 234) {
                        //						ASSERT(0);
                    }

                    if (frame > last_frame)
                        last_frame = frame;
                    //					LogText(" read data into frame %d \n",frame);
                    the_key_frame = &the_chunk->KeyFrames[frame];
                    the_key_frame->ChunkID = 0;
                    the_key_frame->FrameID = frame;
                    the_key_frame->TweenStep = 4;
                    the_key_frame->ElementCount = the_chunk->ElementCount;
                    the_key_frame->FirstElement = &the_elements[current_element];
                    //					LogText(" Read a keyframe elecount %d \n",the_key_frame->ElementCount);
                    for (c0 = 0; c0 < the_key_frame->ElementCount;) {
                        // poo						LogText("element c0 %d out of %d \n",c0,the_key_frame->ElementCount);
                        fscanf(f_handle, "%s", temp_string); // Read the 'transform' bit.
                        if (!strcmp(temp_string, "transform")) {
                            read_object_name(f_handle, transform_name);
                            for (c2 = 0; c2 < strlen(transform_name); c2++) {
                                transform_name[c2] = tolower(transform_name[c2]);
                            }

                            if ((!strcmp(transform_name, "lfoot")) || (!strcmp(transform_name, "pivot")))
                                pivot = 1;
                            else
                                pivot = 0;

                            for (c1 = 0; c1 < the_key_frame->ElementCount; c1++) {
                                SLONG offset;
                                if (funny_fanny)
                                    offset = 0;
                                else
                                    offset = 2;

                                //								if(!memcmp(transform_name,prim_objects[prim_multi_objects[the_chunk->MultiObject].StartObject+c1].ObjectName,strlen(transform_name)-offset))
                                if (!memcmp(transform_name, prim_names[prim_multi_objects[the_chunk->MultiObject].StartObject + c1], strlen(transform_name) - offset))
                                    break;
                                //
                                // find the element to assign it to
                                //
                            }

                            fscanf(
                                f_handle,
                                "%f %f %f %f %f %f %f %f %f %f %f %f",
                                &fe_matrix[0][0], &fe_matrix[0][1], &fe_matrix[0][2],
                                &fe_matrix[1][0], &fe_matrix[1][1], &fe_matrix[1][2],
                                &fe_matrix[2][0], &fe_matrix[2][1], &fe_matrix[2][2],
                                &fe_offset_x, &fe_offset_y, &fe_offset_z);

                            //							normalise_max_matrix(fe_matrix,&fe_offset_x,&fe_offset_y,&fe_offset_z);

                            // this matrix has been fiddled so y=-z && z=y
                            /*
                            if(funny_fanny)
                            {
                                    fe_matrix[0][0]/=18.091;
                                    fe_matrix[0][1]/=18.091;
                                    fe_matrix[0][2]/=18.091;

                                    fe_matrix[1][0]/=18.091;
                                    fe_matrix[1][1]/=18.091;
                                    fe_matrix[1][2]/=18.091;

                                    fe_matrix[2][0]/=18.091;
                                    fe_matrix[2][1]/=18.091;
                                    fe_matrix[2][2]/=18.091;
                            } */

                            temp_matrix.M[0][0] = (SLONG)(fe_matrix[0][0] * (1 << 15));
                            temp_matrix.M[0][2] = (SLONG)(fe_matrix[1][0] * (1 << 15));
                            temp_matrix.M[0][1] = (SLONG)(fe_matrix[2][0] * (1 << 15)); //-ve md

                            temp_matrix.M[2][0] = (SLONG)(fe_matrix[0][1] * (1 << 15));
                            temp_matrix.M[2][2] = (SLONG)(fe_matrix[1][1] * (1 << 15));
                            temp_matrix.M[2][1] = (SLONG)(fe_matrix[2][1] * (1 << 15)); //-ve md

                            temp_matrix.M[1][0] = (SLONG)(fe_matrix[0][2] * (1 << 15));
                            temp_matrix.M[1][2] = (SLONG)(fe_matrix[1][2] * (1 << 15));
                            temp_matrix.M[1][1] = (SLONG)(fe_matrix[2][2] * (1 << 15)); // not -ve md
                            {
                                SLONG dx, dy;
                                for (dx = 0; dx < 3; dx++) {
                                    for (dy = 0; dy < 3; dy++) {
                                        /*
                                                                                                                        if(temp_matrix.M[dx][dy] == 32768)
                                                                                                                        {
                                                                                                                                temp_matrix.M[dx][dy] = 32767;
                                                                                                                        }

                                                                                                                        if(temp_matrix.M[dx][dy] == -32768)
                                                                                                                        {
                                                                                                                                temp_matrix.M[dx][dy] = -32767;
                                                                                                                        }
                                        */
                                        SATURATE(temp_matrix.M[dx][dy], -32767, 32767);

                                        //										ASSERT(temp_matrix.M[dx][dy] <32768 && temp_matrix.M[dx][dy]>=-32768);
                                    }
                                }
                            }

                            the_element = &the_elements[(current_element + c1)];
                            //					current_element++;
                            memcpy(&the_element->Matrix, &temp_matrix, sizeof(struct Matrix33));

                            make_compress_matrix(the_element, &temp_matrix);

                            /*
                            // Guy - Added scaling to multi objects.
                                                                    the_element->OffsetX	=	(SLONG)fe_offset_x;
                                                                    the_element->OffsetY	=	-(SLONG)fe_offset_z;
                                                                    the_element->OffsetZ	=	(SLONG)fe_offset_y;
                            */
                            the_element->OffsetX = (SLONG)((fe_offset_x * GAME_SCALE) / (GAME_SCALE_DIV * shrink_me));
                            the_element->OffsetY = (SLONG)((fe_offset_z * GAME_SCALE) / (GAME_SCALE_DIV * shrink_me)); // -ve md
                            the_element->OffsetZ = (SLONG)((fe_offset_y * GAME_SCALE) / (GAME_SCALE_DIV * shrink_me));
                            the_element->Next = current_element;

                            the_element->OffsetX -= x_centre;
                            the_element->OffsetY -= y_centre;
                            the_element->OffsetZ -= z_centre;

                            //						the_element->OffsetX>>=2; //TWEEN_OFFSET_SHIFT;
                            //						the_element->OffsetY>>=2; //TWEEN_OFFSET_SHIFT;
                            //						the_element->OffsetZ>>=2; //TWEEN_OFFSET_SHIFT;

                            //! jcl - delete me!
                            //							jcl_process_offset_check(the_element->OffsetX, the_element->OffsetZ, the_element->OffsetZ);

                            if (pivot) {
                                the_key_frame->Dx = the_element->OffsetX >> 2;
                                the_key_frame->Dy = the_element->OffsetY >> 2;
                                the_key_frame->Dz = the_element->OffsetZ >> 2;
                                //							LogText("PIVOT frame %d  dx %d dy %d dz %d \n",frame,the_key_frame->Dx,the_key_frame->Dy,the_key_frame->Dz);
                            }

                            c0++;
                        }
                    }
                    current_element += (the_key_frame->ElementCount);
                    the_element->Next = 0;
                }
            }
            //			LogText(" duff >%s<\n",temp_string);

        } while (result >= 0);
        MF_Fclose(f_handle);
        the_chunk->LastElement = &the_elements[current_element];
    }
    the_chunk->KeyFrameCount = last_frame;
    sort_multi_object(the_chunk);

    //! jcl delete me
    //	jcl_fini();
}

SLONG load_anim_mesh(CBYTE* fname, float scale)
{
    SLONG ele_count;
    ele_count = load_a_multi_prim(fname); // ele_count bug

    return (ele_count);
}

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
        //		if(ele_count>15)
        //			ele_count=15;
        // md change
        //		the_chunk->ElementCount	=	prim_multi_objects[the_chunk->MultiObject].EndObject-prim_multi_objects[the_chunk->MultiObject].StartObject;
        the_chunk->ElementCount = ele_count; // prim_multi_objects[the_chunk->MultiObject].EndObject-prim_multi_objects[the_chunk->MultiObject].StartObject;

        // Fudgy bit for centering.
        {
            SLONG multi;
            extern SLONG x_centre,
                y_centre,
                z_centre;
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

/*
fucked one
LCTI obj 122 has f4 6 f3 31
LCTI obj 123 has f4 0 f3 16
LCTI obj 124 has f4 2 f3 8
LCTI obj 125 has f4 3 f3 6
LCTI obj 126 has f4 17 f3 77
LCTI obj 127 has f4 1 f3 11
LCTI obj 128 has f4 1 f3 14
LCTI obj 129 has f4 4 f3 6
LCTI obj 130 has f4 2 f3 9
LCTI obj 131 has f4 2 f3 12
LCTI obj 132 has f4 3 f3 8
LCTI obj 133 has f4 7 f3 20
LCTI obj 134 has f4 1 f3 14
LCTI obj 135 has f4 2 f3 8
LCTI obj 136 has f4 3 f3 6
LCTI obj 137 has f4 0 f3 0


fucked one
LCTI obj 122 has f4 6 f3 31
LCTI obj 123 has f4 0 f3 16
LCTI obj 124 has f4 2 f3 8
LCTI obj 125 has f4 3 f3 6
LCTI obj 126 has f4 17 f3 77
LCTI obj 127 has f4 1 f3 11
LCTI obj 128 has f4 1 f3 14
LCTI obj 129 has f4 4 f3 6
LCTI obj 130 has f4 2 f3 9
LCTI obj 131 has f4 2 f3 12
LCTI obj 132 has f4 3 f3 8
LCTI obj 133 has f4 7 f3 20
LCTI obj 134 has f4 1 f3 14
LCTI obj 135 has f4 2 f3 8
LCTI obj 136 has f4 3 f3 6
LCTI obj 137 has f4 0 f3 0

fucked but right sex
LCTI obj 122 has f4 6 f3 31
LCTI obj 123 has f4 0 f3 16
LCTI obj 124 has f4 2 f3 8
LCTI obj 125 has f4 3 f3 6
LCTI obj 126 has f4 17 f3 77
LCTI obj 127 has f4 1 f3 11
LCTI obj 128 has f4 1 f3 14
LCTI obj 129 has f4 4 f3 6
LCTI obj 130 has f4 2 f3 9
LCTI obj 131 has f4 2 f3 12
LCTI obj 132 has f4 3 f3 8
LCTI obj 133 has f4 7 f3 20
LCTI obj 134 has f4 1 f3 14
LCTI obj 135 has f4 2 f3 8
LCTI obj 136 has f4 3 f3 6
LCTI obj 137 has f4 0 f3 0

right but wrong sex
LCTI obj 122 has f4 6 f3 31
LCTI obj 123 has f4 0 f3 16
LCTI obj 124 has f4 1 f3 10
LCTI obj 125 has f4 3 f3 6
LCTI obj 126 has f4 18 f3 75
LCTI obj 127 has f4 0 f3 13
LCTI obj 128 has f4 0 f3 16
LCTI obj 129 has f4 4 f3 6
LCTI obj 130 has f4 2 f3 9
LCTI obj 131 has f4 2 f3 12
LCTI obj 132 has f4 3 f3 8
LCTI obj 133 has f4 5 f3 24
LCTI obj 134 has f4 1 f3 14
LCTI obj 135 has f4 2 f3 8
LCTI obj 136 has f4 3 f3 6
LCTI obj 137 has f4 0 f3 0

*/

// claude-ai: read_a_prim() — reads one sub-object block from an open .moj or .all handle.
// claude-ai: Used by load_a_multi_prim() and load_insert_a_multi_prim() for each body-part sub-mesh.
// claude-ai: Binary block layout within the containing file:
// claude-ai:   [32 bytes] char name[]          — sub-object name, stored in prim_names[next_prim_object]
// claude-ai:   [4 bytes]  SLONG sp             — file-relative StartPoint index
// claude-ai:   [4 bytes]  SLONG ep             — file-relative EndPoint index
// claude-ai:   [(ep-sp) × sizeof(PrimPoint)] vertices  (SWORD XYZ) or OldPrimPoint (SLONG XYZ) if save_type<=3
// claude-ai:   [4 bytes]  SLONG sf3 / [4 bytes] SLONG ef3
// claude-ai:   [(ef3-sf3) × sizeof(PrimFace3)] triangle faces; point indices rebased by dp
// claude-ai:   [4 bytes]  SLONG sf4 / [4 bytes] SLONG ef4
// claude-ai:   [(ef4-sf4) × sizeof(PrimFace4)] quad faces; point indices rebased by dp
// claude-ai:     dp = next_prim_point - sp  (translates file-relative indices to global-array indices)
// claude-ai: FACE_FLAG_WALKABLE is cleared on all quad faces of animated mesh sub-objects.
void read_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type)
{
    SLONG c0;
    SLONG sf3, ef3, sf4, ef4, sp, ep;
    SLONG dp;

    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&prim_names[next_prim_object], 32); // sizeof(prim_objects[0].ObjectName));

        //		LogText(" name %s becomes prim %d\n",prim_objects[next_prim_object].ObjectName,next_prim_object);

        FileRead(handle, (UBYTE*)&sp, sizeof(sp));
        FileRead(handle, (UBYTE*)&ep, sizeof(ep));

        for (c0 = sp; c0 < ep; c0++) {
            if (save_type > 3) {
                FileRead(handle, (UBYTE*)&prim_points[next_prim_point + c0 - sp], sizeof(struct PrimPoint));
            } else {
                struct OldPrimPoint pp;
                FileRead(handle, (UBYTE*)&pp, sizeof(struct OldPrimPoint));

                prim_points[next_prim_point + c0 - sp].X = (SWORD)pp.X;
                prim_points[next_prim_point + c0 - sp].Y = (SWORD)pp.Y;
                prim_points[next_prim_point + c0 - sp].Z = (SWORD)pp.Z;
            }
        }

        dp = next_prim_point - sp; // was 10 but is now 50 so need to add 40 to all point indexs

        FileRead(handle, (UBYTE*)&sf3, sizeof(sf3));
        FileRead(handle, (UBYTE*)&ef3, sizeof(ef3));
        for (c0 = sf3; c0 < ef3; c0++) {
            FileRead(handle, (UBYTE*)&prim_faces3[next_prim_face3 + c0 - sf3], sizeof(struct PrimFace3));
            prim_faces3[next_prim_face3 + c0 - sf3].Points[0] += dp;
            prim_faces3[next_prim_face3 + c0 - sf3].Points[1] += dp;
            prim_faces3[next_prim_face3 + c0 - sf3].Points[2] += dp;
        }

        FileRead(handle, (UBYTE*)&sf4, sizeof(sf4));
        FileRead(handle, (UBYTE*)&ef4, sizeof(ef4));
        for (c0 = sf4; c0 < ef4; c0++) {
            FileRead(handle, (UBYTE*)&prim_faces4[next_prim_face4 + c0 - sf4], sizeof(struct PrimFace4));
            prim_faces4[next_prim_face4 + c0 - sf4].Points[0] += dp;
            prim_faces4[next_prim_face4 + c0 - sf4].Points[1] += dp;
            prim_faces4[next_prim_face4 + c0 - sf4].Points[2] += dp;
            prim_faces4[next_prim_face4 + c0 - sf4].Points[3] += dp;

            prim_faces4[next_prim_face4 + c0 - sf4].FaceFlags &= ~FACE_FLAG_WALKABLE;
        }

        prim_objects[next_prim_object].StartPoint = next_prim_point;
        prim_objects[next_prim_object].EndPoint = next_prim_point + ep - sp;

        prim_objects[next_prim_object].StartFace3 = next_prim_face3;
        prim_objects[next_prim_object].EndFace3 = next_prim_face3 + ef3 - sf3;

        prim_objects[next_prim_object].StartFace4 = next_prim_face4;
        prim_objects[next_prim_object].EndFace4 = next_prim_face4 + ef4 - sf4;

        next_prim_point += ep - sp;
        next_prim_face3 += ef3 - sf3;
        next_prim_face4 += ef4 - sf4;

        next_prim_object++;
    }
}

// extern	struct	PrimMultiObject	prim_multi_objects[];

// claude-ai: ============================================================
// claude-ai: load_a_multi_prim() — loads one .moj file (multi-prim container)
// claude-ai: ============================================================
// claude-ai: A .MOJ file contains a group of prim sub-objects that together form one
// claude-ai: animated character or vehicle mesh variant. Each sub-object is one body part
// claude-ai: (e.g. torso, left arm, right arm) or one car panel.
// claude-ai: Multiple .MOJ files can belong to one character (e.g. skin variants stored as
// claude-ai: separate .MOJ files, loaded and registered in prim_multi_objects[]).
// claude-ai:
// claude-ai: .MOJ FILE BINARY LAYOUT:
// claude-ai:   [4 bytes]  SLONG save_type            — version (3 or 4 observed)
// claude-ai:   [4 bytes]  SLONG so                   — original StartObject index (file-relative)
// claude-ai:   [4 bytes]  SLONG eo                   — original EndObject index (file-relative)
// claude-ai:   for each object in [so, eo):
// claude-ai:     read_a_prim() block:
// claude-ai:       [32 bytes] char name[]             — sub-object name (body part label)
// claude-ai:       [4 bytes]  SLONG sp                — StartPoint (file-relative)
// claude-ai:       [4 bytes]  SLONG ep                — EndPoint
// claude-ai:       [ep-sp * sizeof(PrimPoint)] vertices
// claude-ai:       [4 bytes]  SLONG sf3               — StartFace3
// claude-ai:       [4 bytes]  SLONG ef3               — EndFace3
// claude-ai:       [(ef3-sf3) * sizeof(PrimFace3)] triangular faces
// claude-ai:       [4 bytes]  SLONG sf4               — StartFace4
// claude-ai:       [4 bytes]  SLONG ef4               — EndFace4
// claude-ai:       [(ef4-sf4) * sizeof(PrimFace4)] quad faces
// claude-ai:
// claude-ai: After loading, prim_multi_objects[next_prim_multi_object] is filled with
// claude-ai: {StartObject=next_prim_object, EndObject=next_prim_object+(eo-so)}.
// claude-ai: Returns next_prim_multi_object index (or 0 on failure/file-not-found).
// claude-ai:
// claude-ai: The body part NAME strings in the sub-objects are critical — sort_multi_object()
// claude-ai: in io.cpp uses them to match keyframe elements to mesh sub-objects by name.
// claude-ai: ============================================================
SLONG load_a_multi_prim(CBYTE* name)
{
    SLONG c0;
    MFFileHandle handle;
    SLONG save_type = 0;
    SLONG so, eo;
    CBYTE ext_name[80];

    change_extension(name, "moj", ext_name);
    //	sprintf(ext_name,"data/%s",ext_name);
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

extern void add_point(SLONG x, SLONG y, SLONG z);
extern struct PrimFace4* create_a_quad(UWORD p1, UWORD p0, UWORD p3, UWORD p2, SWORD texture_style, SWORD texture_piece);
extern SLONG build_prim_object(SLONG sp, SLONG sf3, SLONG sf4);
extern void save_prim_asc(UWORD prim, UWORD version);

void create_kline_bottle(void)
{
    float x, y, z, u, v;
    float sqrt_2, a = 1.0; // what the fuck should a be
    struct PrimFace4* p_f4;

    float step = PI / 10.0; // low poly version

    return; // switch it off for now

    /*
            SLONG	sp[10000],count=0,inner_count=0;
            SLONG	c0,c1;
            SLONG	sf3,sf4,stp;

            stp=next_prim_point;
            sf3=next_prim_face3;
            sf4=next_prim_face4;

            sqrt_2=sqrt(2.0);


            for(u=-1.0*PI;u<(1.0*PI)+(step/2.0);u+=step,count++)
            {
                    sp[count]=next_prim_point;
                    for(v=-1.0*PI;v<(1.0*PI)+(step/2.0);v+=step)
                    {
    //			x = cos(u)*(cos(u/2.0)*(sqrt_2+cos(v))+(sin(u/2.0)*sin(v)*cos(v)));
    //			y = sin(u)*(cos(u/2.0)*(sqrt_2+cos(v))+(sin(u/2.0)*sin(v)*cos(v)));
    //			z = -1.0*sin(u/2.0)*(sqrt_2+cos(v))+cos(u/2.0)*sin(v)*cos(v);

                              x = (a+cos(u/2.0)*sin(v)-sin(u/2.0)*sin(2.0*v))*cos(u);

                              y = (a+cos(u/2.0)*sin(v)-sin(u/2.0)*sin(2.0*v))*sin(u);

                              z = sin(u/2.0)*sin(v)+cos(u/2.0)*sin(2.0*v);


                            add_point((SLONG)(x*200.0),(SLONG)(y*200.0),(SLONG)(z*200.0));
                    }
            }

            for(c0=0;c0<count-1;c0++)
            for(c1=0;c1<count-1;c1++)
            {
    //		if((c0+c1)&1)
                    {
                            p_f4=create_a_quad(sp[c0]+c1,sp[c0]+c1+1,sp[c0+1]+c1,sp[c0+1]+c1+1,0,0);
                            p_f4->DrawFlags=1+(1<<6); //POLY_G; gourad
                            p_f4->Col=((c0+c1)&1)*50+50;
                    }
            }

            build_prim_object(stp,sf3,sf4);
            save_prim_asc(next_prim_object-1,0);
            */
}

// claude-ai: load_palette() — loads a 256-entry RGB palette file into ENGINE_palette[].
// claude-ai: Binary layout: 256 × 3 bytes (R, G, B), one byte per channel, no alpha.
// claude-ai: Used for 8-bit paletted rendering on PC (DirectX palettised surface).
// claude-ai: On error, fills ENGINE_palette[] with random colours (visible glitch = file missing).
// claude-ai: In new game (true-colour rendering) this entire system is irrelevant.
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

//
// no meshes
//

// mesh

//

extern void write_a_prim(SLONG prim, MFFileHandle handle);

SLONG save_insert_a_multi_prim(MFFileHandle handle, SLONG multi)
{
    SLONG c0, point;
    CBYTE file_name[64];
    SLONG save_type = 4;
    SLONG so, eo;
    CBYTE ext_name[80];
    return (0);
}

SLONG save_insert_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk)
{
    SLONG save_type = 5;
    SLONG temp;
    UWORD check = 666;
    return (1);
}

SLONG save_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name)
{
    SLONG c0, point;
    MFFileHandle handle;
    CBYTE file_name[64];
    SLONG save_type = 5;
    SLONG so, eo;
    CBYTE ext_name[80];
    SLONG count;
    return (0);
}
// claude-ai: load_insert_game_chunk() — reads the GameKeyFrameChunk section from a .all file.
// claude-ai: This is the animation data half of the .all bundle (the mesh half is the .moj blocks).
// claude-ai: Memory is MemAlloc'd here for each sub-array (AnimKeyFrames, AnimList, TheElements,
// claude-ai: FightCols, PeopleTypes). Sizes come from MaxKeyFrames/MaxElements/etc. fields.
// claude-ai: POINTER FIXUP: addr1/addr2/addr3 are original save-time runtime addresses of each array.
// claude-ai:   Each stored pointer value is: original_runtime_addr → rebased to new allocation.
// claude-ai:   a_off = new_ptr - addr1 added to each stored NextFrame/PrevFrame/AnimList entry.
// claude-ai: save_type > 4 uses convert_*_to_pointer() helpers; older files do manual fixup.
// claude-ai: ULTRA_COMPRESSED_ANIMATIONS: compile-time flag to use compressed matrix format (PSX).
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

    //	if(p_chunk->MaxAnimFrames)
    //		p_chunk->TweakSpeeds=(UBYTE*)MemAlloc(p_chunk->MaxAnimFrames);

    //	if(save_type>=3)
    //	{
    //		FileRead(handle,(UBYTE*)&p_chunk->TweakSpeeds[0],p_chunk->MaxAnimFrames);
    //		FileRead(handle,&check,2);
    //		ASSERT(check==666);
    //
    //	}

    //
    // Load the people types
    //
    // FileRead(handle,(UBYTE*)&temp,sizeof(temp));
    FileRead(handle, (UBYTE*)&p_chunk->PeopleTypes[0], sizeof(struct BodyDef) * p_chunk->MaxPeopleTypes);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);
    //
    // Load the keyframe linked lists
    //
    FileRead(handle, (UBYTE*)&addr1, sizeof(addr1));
    FileRead(handle, (UBYTE*)&p_chunk->AnimKeyFrames[0], sizeof(struct GameKeyFrame) * p_chunk->MaxKeyFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    //
    // Load the anim elements
    //

    FileRead(handle, (UBYTE*)&addr2, sizeof(addr2));
    FileRead(handle, (UBYTE*)&p_chunk->TheElements[0], sizeof(struct GameKeyFrameElement) * p_chunk->MaxElements);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    //
    // Load the animlist
    //
    //	FileRead(handle,(UBYTE*)&temp,sizeof(temp));
    FileRead(handle, (UBYTE*)&p_chunk->AnimList[0], sizeof(struct GameKeyFrame*) * p_chunk->MaxAnimFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    FileRead(handle, (UBYTE*)&addr3, sizeof(addr3));

    FileRead(handle, (UBYTE*)&p_chunk->FightCols[0], sizeof(struct GameFightCol) * p_chunk->MaxFightCols);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    if (save_type < 3) {
        SLONG c0;
        //
        // convert anim speeds to step sizes rather than step counts
        //

        for (c0 = 0; c0 < p_chunk->MaxKeyFrames; c0++) {
            p_chunk->AnimKeyFrames[c0].TweenStep = (256 / (p_chunk->AnimKeyFrames[c0].TweenStep + 1)) >> 1;
        }
    }

    if (save_type > 4) {
        extern void convert_keyframe_to_pointer(GameKeyFrame * p, GameKeyFrameElement * p_ele, GameFightCol * p_fight, SLONG count);
        extern void convert_animlist_to_pointer(GameKeyFrame * *p, GameKeyFrame * p_anim, SLONG count);
        extern void convert_fightcol_to_pointer(GameFightCol * p, GameFightCol * p_fight, SLONG count);

        convert_keyframe_to_pointer(p_chunk->AnimKeyFrames, p_chunk->TheElements, p_chunk->FightCols, p_chunk->MaxKeyFrames);
        convert_animlist_to_pointer(p_chunk->AnimList, p_chunk->AnimKeyFrames, p_chunk->MaxAnimFrames);
        convert_fightcol_to_pointer(p_chunk->FightCols, p_chunk->FightCols, p_chunk->MaxFightCols);
    } else {

        // was at 100 now at 10, a_off =-90 so we take 90 off each stored address

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

            //		p_chunk->AnimKeyFrames[c0].Fight=0;

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
                        p_fight->Next = 0; //(struct GameFightCol*)a;
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

// claude-ai: load_insert_a_multi_prim() — reads one .moj block from an open .all file handle.
// claude-ai: Difference from load_a_multi_prim(): takes an already-open handle (not a filename).
// claude-ai: The .all file embeds multiple .moj blocks sequentially; this function reads one.
// claude-ai: Same binary format as the standalone .moj file: save_type + so + eo + N×read_a_prim().
// claude-ai: Returns new next_prim_multi_object-1 index on success, 0 on error.
SLONG load_insert_a_multi_prim(MFFileHandle handle)
{
    SLONG c0;
    SLONG save_type = 0;
    SLONG so, eo;
    //	CBYTE			ext_name[80];

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

extern ULONG DONT_load;
// claude-ai: ============================================================
// claude-ai: load_anim_system() — loads a complete .all animation bundle
// claude-ai: ============================================================
// claude-ai: A .all file contains a complete animated character/creature bundle:
// claude-ai:   - One or more .moj multi-prim mesh variants (skin types, LOD levels)
// claude-ai:   - One GameKeyFrameChunk with all animation keyframe data
// claude-ai:
// claude-ai: Called for:
// claude-ai:   - anim_chunk[0] = people (darci.all / thug.all)
// claude-ai:   - anim_chunk[1] = bats, anim_chunk[2] = gargoyles
// claude-ai:   - anim_chunk[3] = balrog, anim_chunk[4] = bane
// claude-ai:   - animNNN.all   — animated world objects (doors, switches, fans, etc.)
// claude-ai:
// claude-ai: .ALL FILE BINARY LAYOUT:
// claude-ai:   [4 bytes]  SLONG save_type            — file format version (2, 3, 4, or 5)
// claude-ai:   if save_type > 2:
// claude-ai:     [4 bytes] SLONG count               — number of .moj mesh chunks
// claude-ai:     [count × load_insert_a_multi_prim() blocks]
// claude-ai:   else (old format):
// claude-ai:     [1 × load_insert_a_multi_prim() block]
// claude-ai:   [load_insert_game_chunk() block]:
// claude-ai:     [4 bytes] SLONG save_type           — game chunk version (must be > 1)
// claude-ai:     [4 bytes] SLONG ElementCount        — body part count (15 for people)
// claude-ai:     [2 bytes] SWORD MaxPeopleTypes       — number of person skin variants
// claude-ai:     [2 bytes] SWORD MaxAnimFrames        — total named animation slots
// claude-ai:     [4 bytes] SLONG MaxElements          — total keyframe element records
// claude-ai:     [2 bytes] SWORD MaxKeyFrames         — total keyframe records
// claude-ai:     [2 bytes] SWORD MaxFightCols          — fight collision volume count
// claude-ai:     [MaxPeopleTypes * sizeof(BodyDef)] PeopleTypes[] — skin→body part mapping
// claude-ai:     [2 bytes] UWORD check (must == 666)
// claude-ai:     [4 bytes] ULONG addr1 (original runtime address — used to fix pointers)
// claude-ai:     [MaxKeyFrames * sizeof(GameKeyFrame)] AnimKeyFrames[]
// claude-ai:     [2 bytes] UWORD check (must == 666)
// claude-ai:     [4 bytes] ULONG addr2
// claude-ai:     [MaxElements * sizeof(GameKeyFrameElement)] TheElements[]
// claude-ai:       GameKeyFrameElement: matrix (compressed 3×3) + OffsetX/Y/Z (body part delta)
// claude-ai:     [2 bytes] UWORD check (must == 666)
// claude-ai:     [MaxAnimFrames * sizeof(GameKeyFrame*)] AnimList[]  — named anim frame pointers
// claude-ai:     [2 bytes] UWORD check (must == 666)
// claude-ai:     [4 bytes] ULONG addr3
// claude-ai:     [MaxFightCols * sizeof(GameFightCol)] FightCols[]
// claude-ai:     [2 bytes] UWORD check (must == 666)
// claude-ai:
// claude-ai: POINTER FIXUP: stored pointers (NextFrame, PrevFrame, FirstElement, Fight, AnimList)
// claude-ai:   are original runtime addresses. After loading, offsets are computed from addr1/addr2/addr3
// claude-ai:   and all pointers are relocated to their new addresses (a_off, ae_off, af_off).
// claude-ai:   save_type > 4 uses convert_keyframe_to_pointer() which does this more cleanly.
// claude-ai:
// claude-ai: The 'type' parameter selects selective skin loading:
// claude-ai:   type=0 → load all variants
// claude-ai:   type=1 → person.all — Darci always, others only if not in DONT_load bitmask
// claude-ai:   type=2 → thug.all   — selective by character class (cop, rasta, MIB, mechanic, tramp)
// claude-ai: ============================================================
// claude-ai: load_anim_system() — top-level loader for a .all animation bundle file.
// claude-ai: Entry point called from load_anim_prim_object() and from character init code.
// claude-ai: Parameters:
// claude-ai:   p_chunk — destination GameKeyFrameChunk (from anim_chunk[] array)
// claude-ai:   name    — base filename, e.g. "anim003.all" or "people.all"
// claude-ai:   type    — 0=creature/world object, 1=people (controls which skin variants to load)
// claude-ai:
// claude-ai: Constructs full path: DATA_DIR + "data\" + name → changes extension to .all
// claude-ai: Calls free_game_chunk() first to release any previously loaded data.
// claude-ai:
// claude-ai: type==1 (people): DONT_load bitmask controls which mesh variants to skip.
// claude-ai:   Bit positions correspond to PERSON_* enum values (PERSON_SLAG_TART etc.).
// claude-ai:   When DONT_load has the bit set, skip loading the matching skin variant .moj block.
// claude-ai:   This allows the game to save memory by not loading unused character skins.
// claude-ai: p_chunk->MultiObject[] array holds indices into prim_multi_objects[]; null-terminated.
SLONG load_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG type)
{
    SLONG c0, point;
    MFFileHandle handle;
    //	CBYTE			file_name[64];
    SLONG save_type = 0;
    //	SLONG			so,eo;
    CBYTE ext_name[100];
    SLONG count;
    UBYTE load;

    CBYTE fname[100];

    sprintf(fname, "%sdata\\%s", DATA_DIR, name);

    extern void free_game_chunk(GameKeyFrameChunk * the_chunk);
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
                    // darci
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
                    // thug   // 0 rasta 1 grey 2 red 3 miller 4 cop 5 mib 6 tramp 7 civ variety

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
    } else {
    }
    return (0);
}
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

    //
    //
    //
    FileRead(handle, (UBYTE*)&ElementCount, sizeof(p_chunk->ElementCount));
    FileRead(handle, (UBYTE*)&MaxPeopleTypes, sizeof(p_chunk->MaxPeopleTypes));
    FileRead(handle, (UBYTE*)&MaxAnimFrames, sizeof(p_chunk->MaxAnimFrames));
    FileRead(handle, (UBYTE*)&MaxElements, sizeof(p_chunk->MaxElements));
    FileRead(handle, (UBYTE*)&MaxKeyFrames, sizeof(p_chunk->MaxKeyFrames));
    FileRead(handle, (UBYTE*)&MaxFightCols, sizeof(p_chunk->MaxFightCols));

    //
    // memory should have been allocated over when the .all was loaded
    //

    //
    // Load the people types
    //
    // FileRead(handle,(UBYTE*)&temp,sizeof(temp));
    //	FileRead(handle,(UBYTE*)&p_chunk->PeopleTypes[0],sizeof(struct BodyDef)*p_chunk->MaxPeopleTypes);
    FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct BodyDef) * MaxPeopleTypes);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);
    //
    // Load the keyframe linked lists
    //
    FileRead(handle, (UBYTE*)&addr1, sizeof(addr1));

    FileRead(handle, (UBYTE*)&p_chunk->AnimKeyFrames[p_chunk->MaxKeyFrames], sizeof(struct GameKeyFrame) * MaxKeyFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    //
    // Load the anim elements
    //

    FileRead(handle, (UBYTE*)&addr2, sizeof(addr2));
    FileRead(handle, (UBYTE*)&p_chunk->TheElements[p_chunk->MaxElements], sizeof(struct GameKeyFrameElement) * MaxElements);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    //
    // Load the animlist
    //
    FileRead(handle, (UBYTE*)&p_chunk->AnimList[start_frame], sizeof(struct GameKeyFrame*) * MaxAnimFrames);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    FileRead(handle, (UBYTE*)&addr3, sizeof(addr3));

    FileRead(handle, (UBYTE*)&p_chunk->FightCols[p_chunk->MaxFightCols], sizeof(struct GameFightCol) * MaxFightCols);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    if (save_type > 4) {
        extern void convert_keyframe_to_pointer(GameKeyFrame * p, GameKeyFrameElement * p_ele, GameFightCol * p_fight, SLONG count);
        extern void convert_animlist_to_pointer(GameKeyFrame * *p, GameKeyFrame * p_anim, SLONG count);
        extern void convert_fightcol_to_pointer(GameFightCol * p, GameFightCol * p_fight, SLONG count);

        convert_keyframe_to_pointer(&p_chunk->AnimKeyFrames[p_chunk->MaxKeyFrames], &p_chunk->TheElements[p_chunk->MaxElements], &p_chunk->FightCols[p_chunk->MaxFightCols], MaxKeyFrames);
        convert_animlist_to_pointer(&p_chunk->AnimList[start_frame], &p_chunk->AnimKeyFrames[p_chunk->MaxKeyFrames], MaxAnimFrames);
        convert_fightcol_to_pointer(&p_chunk->FightCols[p_chunk->MaxFightCols], &p_chunk->FightCols[p_chunk->MaxFightCols], MaxFightCols);
    }

    //	p_chunk->ElementCount+=ElementCount;
    p_chunk->MaxAnimFrames = start_frame + MaxAnimFrames;
    p_chunk->MaxElements += MaxElements;
    p_chunk->MaxKeyFrames += MaxKeyFrames;
    p_chunk->MaxFightCols += MaxFightCols;

    return (1);
}

void skip_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type)
{
    SLONG c0;
    SLONG sf3, ef3, sf4, ef4, sp, ep;
    SLONG dp;

    if (handle != FILE_OPEN_ERROR) {
        //		FileRead(handle,(UBYTE*)&prim_names[next_prim_object],32);//sizeof(prim_objects[0].ObjectName));
        FileSeek(handle, SEEK_MODE_CURRENT, 32);

        //		LogText(" name %s becomes prim %d\n",prim_objects[next_prim_object].ObjectName,next_prim_object);

        FileRead(handle, (UBYTE*)&sp, sizeof(sp));
        FileRead(handle, (UBYTE*)&ep, sizeof(ep));

        ASSERT(ep - sp >= 0 && ep - sp < 10000);

        for (c0 = sp; c0 < ep; c0++) {
            //			FileRead(handle,(UBYTE*)&prim_points[next_prim_point+c0-sp],sizeof(struct PrimPoint));
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimPoint));
        }

        FileRead(handle, (UBYTE*)&sf3, sizeof(sf3));
        FileRead(handle, (UBYTE*)&ef3, sizeof(ef3));
        ASSERT(ef3 - sf3 >= 0 && ef3 - sf3 < 10000);
        for (c0 = sf3; c0 < ef3; c0++) {
            //			FileRead(handle,(UBYTE*)&prim_faces3[next_prim_face3+c0-sf3],sizeof(struct PrimFace3));
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimFace3));
        }

        FileRead(handle, (UBYTE*)&sf4, sizeof(sf4));
        FileRead(handle, (UBYTE*)&ef4, sizeof(ef4));
        ASSERT(ef4 - sf4 >= 0 && ef4 - sf4 < 10000);
        for (c0 = sf4; c0 < ef4; c0++) {
            //			FileRead(handle,(UBYTE*)&prim_faces4[next_prim_face4+c0-sf4],sizeof(struct PrimFace4));
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimFace4));
        }
    }
}

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

SLONG append_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG start_anim, SLONG load_mesh)
{
    SLONG c0, point;
    MFFileHandle handle;
    //	CBYTE			file_name[64];
    SLONG save_type = 0;
    //	SLONG			so,eo;
    CBYTE ext_name[100];
    SLONG count;

    CBYTE fname[100];

    sprintf(fname, "%sdata\\%s", DATA_DIR, name);

    extern void free_game_chunk(GameKeyFrameChunk * the_chunk);
    //	free_game_chunk(p_chunk);

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
            //			DebugText(" next_prim_point %d primface3 %d primface4 %d   load ANIMSYSTEM part %d \n",next_prim_point,next_prim_face3,next_prim_face4,c0);
        }
        if (c0 + start_ob < 10)
            p_chunk->MultiObject[c0 + start_ob] = 0;

        load_append_game_chunk(handle, p_chunk, start_anim);

        FileClose(handle);
        return (1);
    } else {
    }
    return (0);
}
