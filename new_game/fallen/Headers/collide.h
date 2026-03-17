#ifndef FALLEN_HEADERS_COLLIDE_H
#define FALLEN_HEADERS_COLLIDE_H

#include "pap.h"

//
// Defines
//
// claude-ai: Лимиты пулов коллизионных структур (статические массивы фиксированного размера).
// claude-ai:   MAX_COL_VECT      — максимум линейных барьеров (CollisionVect); PC: 10000 × ~20 байт ≈ 200KB
// claude-ai:   MAX_COL_VECT_LINK — максимум привязок барьеров к лорез-ячейкам; PC: 10000 × 4 байт ≈ 40KB
// claude-ai:   MAX_WALK_POOL     — максимум записей о проходимых поверхностях; PC: 30000 × 4 байт ≈ 120KB
// claude-ai: На PSX все лимиты значительно меньше из-за нехватки RAM.
#define MAX_COL_VECT_LINK 10000
#define MAX_COL_VECT 10000
#define MAX_WALK_POOL (30000)

#define DONT_INTERSECT 0
#define DO_INTERSECT 1
#define COLLINEAR 2

//
// Structures
//

// claude-ai: Привязка коллизионного барьера к лорез-ячейке карты. 4 байта.
// claude-ai: Хранится в col_vects_links[MAX_COL_VECT_LINK] — связный список для каждой PAP_Lo-ячейки.
// claude-ai:   Next       — индекс следующего элемента в цепочке (0 = конец)
// claude-ai:   VectIndex  — индекс в массиве col_vects[]
struct CollisionVectLink {
    UWORD Next; // linked list of collision vectors
    UWORD VectIndex;
};

// claude-ai: Линейный коллизионный барьер в 3D (стена, забор, лестница и т.д.). ~20 байт.
// claude-ai: Хранится в col_vects[MAX_COL_VECT=10000] — пул всех барьеров уровня.
// claude-ai:   X[2], Z[2] — начало и конец барьера в горизонтальной плоскости (мировые координаты)
// claude-ai:   Y[2]       — нижняя и верхняя высоты барьера (SWORD)
// claude-ai:   PrimType   — тип storey (стена, лестница, забор, и т.п.)
// claude-ai:   PrimExtra  — дополнительный параметр (зависит от PrimType)
// claude-ai:   Face       — индекс facet в системе зданий (-1 если не связан с фасетом)
struct CollisionVect {
    SLONG X[2];
    SWORD Y[2];
    SLONG Z[2];
    UBYTE PrimType; // A storey type...
    UBYTE PrimExtra;
    SWORD Face;
};

// claude-ai: Запись о проходимой поверхности (walkable face). 4 байта.
// claude-ai: Хранится в walk_links[MAX_WALK_POOL=30000] — пул для навигации персонажей.
// claude-ai:   Next — следующий элемент связного списка для данной PAP_Lo-ячейки
// claude-ai:   Face — индекс face (грани) в системе зданий, по которой можно ходить
struct WalkLink {
    UWORD Next;
    SWORD Face;
};
//
// Vars
//

extern struct CollisionVectLink col_vects_links[MAX_COL_VECT_LINK];
extern struct CollisionVect col_vects[MAX_COL_VECT];

extern UWORD next_col_vect;
extern UWORD next_col_vect_link;

//
// Functions
//

extern struct WalkLink walk_links[MAX_WALK_POOL];
extern UWORD next_walk_link;

extern SLONG get_fence_height(SLONG h);
extern UBYTE two4_line_intersection(SLONG x1, SLONG y1, SLONG x2, SLONG y2, SLONG x3, SLONG y3, SLONG x4, SLONG y4);
extern SLONG lines_intersect(SLONG x1, SLONG y1, SLONG x2, SLONG y2, SLONG x3, SLONG y3, SLONG x4, SLONG y4, SLONG* x, SLONG* y);
extern SLONG insert_collision_vect(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, UBYTE prim, UBYTE prim_extra, SWORD face);
extern void remove_collision_vect(UWORD vect);

extern SLONG get_point_dist_from_col_vect(SLONG vect, SLONG x, SLONG z, SLONG* ret_x, SLONG* ret_z, SLONG new_dist);
extern SLONG check_vect_circle(SLONG m_dx, SLONG m_dy, SLONG m_dz, Thing* p_thing, SLONG radius);
// claude-ai: move_thing       — полное движение с коллизиями для CLASS_PERSON.
// claude-ai:                    Вызывает slide_along + collide_against_things + plant_feet.
// claude-ai:                    Возвращает флаги столкновений (ULONG).
// claude-ai: move_thing_quick — телепортация без коллизий (мгновенное смещение).
// claude-ai:                    Используется для ragdoll, катсцен, respawn.
extern ULONG move_thing(SLONG m_dx, SLONG m_dy, SLONG m_dz, Thing* p_thing);
extern ULONG move_thing_quick(SLONG dx, SLONG dy, SLONG dz, Thing* p_thing);
extern SLONG check_vect_vect(SLONG m_dx, SLONG m_dy, SLONG m_dz, Thing* p_thing, SLONG scale);
extern SLONG find_face_for_this_pos(SLONG x, SLONG y, SLONG z, SLONG* ret_y, SLONG ignore_faces_of_this_building, UBYTE flag);
extern SLONG dist_to_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b);
extern void correct_pos_for_ladder(struct DFacet* p_facet, SLONG* px, SLONG* pz, SLONG* angle, SLONG scale);
extern SLONG height_above_anything(Thing* p_person, SLONG body_part, SWORD* onface);
extern SLONG plant_feet(Thing* p_person);

//
// Early-out tests for box-box circle-box collision. These functions
// return TRUE if the primitives don't collide for certain.
//

void box_box_early_out(
    SLONG box1_mid_x,
    SLONG box1_mid_z,
    SLONG box1_min_x,
    SLONG box1_min_z,
    SLONG box1_max_x,
    SLONG box1_max_z,
    SLONG box1_yaw,
    SLONG box2_mid_x,
    SLONG box2_mid_z,
    SLONG box2_min_x,
    SLONG box2_min_z,
    SLONG box2_max_x,
    SLONG box2_max_z,
    SLONG box2_yaw);

void box_circle_early_out(
    SLONG box1_mid_x,
    SLONG box1_mid_z,
    SLONG box1_min_x,
    SLONG box1_min_z,
    SLONG box1_max_x,
    SLONG box1_max_z,
    SLONG box1_yaw,
    SLONG cx,
    SLONG cz,
    SLONG cradius);

//
// Slides the given vector against a circle.
// Returns TRUE if a collision occurred.
//

SLONG slide_around_circle(
    SLONG cx,
    SLONG cz,
    SLONG cradius,

    SLONG x1,
    SLONG z1,
    SLONG* x2,
    SLONG* z2);

//
// Slides the given vector against a box. Returns TRUE if
// a collision occurred.
//

SLONG slide_around_box(
    SLONG box_mid_x,
    SLONG box_mid_z,
    SLONG box_min_x,
    SLONG box_min_z,
    SLONG box_max_x,
    SLONG box_max_z,
    SLONG box_yaw,
    SLONG radius, // How far from the box the vector should be from.

    SLONG x1, // The start of the vector is ignored! So you can use this
    SLONG z1, // function to slide a circle around the box too.
    SLONG* x2,
    SLONG* z2);

//
// Returns TRUE if a collision vector intersects a rotated box
//

SLONG collide_box(
    SLONG midx,
    SLONG midy,
    SLONG midz,
    SLONG minx, SLONG minz,
    SLONG maxx, SLONG maxz,
    SLONG yaw);

//
// Returns TRUE if the given line intersects the rotated bounding box.
//

SLONG collide_box_with_line(
    SLONG midx,
    SLONG midz,
    SLONG minx, SLONG minz,
    SLONG maxx, SLONG maxz,
    SLONG yaw,
    SLONG lx1,
    SLONG lz1,
    SLONG lx2,
    SLONG lz2);

//
// Slides the given vector against the sausage.  Returns TRUE
// if a slide-along occured.
//

SLONG slide_around_sausage(
    SLONG sx1,
    SLONG sz1,
    SLONG sx2,
    SLONG sz2,
    SLONG sradius,

    SLONG x1,
    SLONG z1,
    SLONG* x2,
    SLONG* z2);

//
// Returns the colvect of a nearby ladder that the thing can mount.
// Makes the thing mount the given ladder... Returns TRUE if it did.
//

SLONG find_nearby_ladder_colvect(Thing* p_thing);
SLONG find_nearby_ladder_colvect_radius(SLONG mid_x, SLONG mid_z, SLONG radius);
SLONG mount_ladder(Thing* p_thing, SLONG col);

//
// Returns which side of the colvect you are on.
//

SLONG which_side(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b);

//
// Returns the height of the floor at (x,z).
//

SLONG calc_height_at(SLONG x, SLONG z);

//
// Returns the height of the map at (x,z)... i.e. if (x,z) is inside a building
// then it returns the MAV_height at the top of the building, otherwise
// the call is equivalent to calc_height_at()
//

SLONG calc_map_height_at(SLONG x, SLONG z);

//
// Given a movement vector, this function changes it to slide
// along any col-vects it may encounter. If it find a colvect
// onto a walkable face, then it doesn't slide, it just returns
// that col-vect.
//

extern SLONG last_slide_colvect; // The last colvect you slid along, or NULL if you didn't slide.

#define SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT (-0x50)

#define SLIDE_ALONG_FLAG_CRAWL (1 << 0)
#define SLIDE_ALONG_FLAG_CARRYING (1 << 1)
#define SLIDE_ALONG_FLAG_JUMPING (1 << 2)

// claude-ai: Ключевая функция скольжения персонажа вдоль стен.
// claude-ai: Изменяет вектор движения (x1,y1,z1)→(*x2,*y2,*z2) так, чтобы он не пересекал col_vects.
// claude-ai: ВАЖНО: НЕТ отскока — только скольжение вдоль поверхности (проекция вектора).
// claude-ai:   extra_wall_height — делает стены выше/ниже (предотвращает падение сквозь стены)
// claude-ai:   radius            — радиус капсулы персонажа
// claude-ai:   flags             — SLIDE_ALONG_FLAG_CRAWL/CARRYING/JUMPING
// claude-ai: Глобал actual_sliding — TRUE если было скольжение в этом вызове.
// claude-ai: Глобал last_slide_colvect — индекс барьера, о который скользили (или NULL).
// claude-ai: Глобал fence_colvect — если нашли проходимый барьер (walkable face), он здесь.
SLONG slide_along(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG extra_wall_height, // Makes walls seem higher/lower so that a falling person won't fall through walls.
    SLONG radius, ULONG flags = 0);

//
// Slides the movement vector around things- If the person hits a
// moving vehicle- he'll die. Returns TRUE if a collision occurs.
//

SLONG collide_against_things(
    Thing* p_thing,
    SLONG radius,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2);

//
// Slides the given movement vector around OB_jects.
// Returns TRUE if a collision occurs.
//

SLONG collide_against_objects(
    Thing* p_thing,
    SLONG radius,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2);

//
// Returns TRUE if there is a los from 1 to 2.
// If there isn't a los, then it fills in los_failure_[x|y|z] with
// the last coordinate from point 1 to 2 that can be seen from point 1.
//

extern SLONG los_failure_x;
extern SLONG los_failure_y;
extern SLONG los_failure_z;
extern SLONG los_failure_dfacet; // The dfacet that caused the LOS to fail.

#define LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG (1 << 0)
#define LOS_FLAG_IGNORE_PRIMS (1 << 1)
#define LOS_FLAG_IGNORE_UNDERGROUND_CHECK (1 << 2)
#define LOS_FLAG_INCLUDE_CARS (1 << 3) // Only relevant if you don't ignore prims.

SLONG there_is_a_los_things(Thing* p_person_a, Thing* p_person_b, SLONG los_flags);

SLONG there_is_a_los(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2,
    SLONG los_flag);

//
// An FOV calculation. Returns TRUE if I can see him, given
// that I am looking in the given direction.
//

SLONG in_my_fov(
    SLONG me_x, SLONG me_z,
    SLONG him_x, SLONG him_z,
    SLONG im_looking_x,
    SLONG im_looking_z);

//
// Returns the nearest person of the given type that I can see.
// 'see' means in FOV and LOS.
//

THING_INDEX find_nearby_person(
    THING_INDEX me,
    UWORD person_type_bits, // A bit for each person type (1 << person_type)
    SLONG max_range); // 256 => One map block.

//
// It doesn't check that (x,z) is a point on the face or not!
//

// SLONG calc_height_on_face(SLONG x,SLONG z,SLONG face);

//
// Looks for any colvect that intersects the given line segment.
// If it find a colvect it returns it, otherwise it returns NULL.
// If there is more than one colvect intersected, it prefers to
// return ladders and fences.
//

SLONG find_intersected_colvect(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG y);

//
// An collision routine for othogonal walls only.  It collides a thin vector
// against a thick orthogonal sausage shape.
//

SLONG collide_against_sausage(
    SLONG sx1, SLONG sz1,
    SLONG sx2, SLONG sz2,
    SLONG swidth,

    SLONG vx1, SLONG vz1,
    SLONG vx2, SLONG vz2,

    SLONG* slide_x,
    SLONG* slide_z);

//
// Useful mathsy stuff...
//
// A 'line' below actually means 'line _segment_'
//

void signed_dist_to_line_with_normal(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b,
    SLONG* dist,
    SLONG* vec_x,
    SLONG* vec_z,
    SLONG* on);

void nearest_point_on_line(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b,
    SLONG* ret_x,
    SLONG* ret_z);

SLONG nearest_point_on_line_and_dist(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b,
    SLONG* ret_x,
    SLONG* ret_z);

SLONG nearest_point_on_line_and_dist_and_along(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b,
    SLONG* ret_x,
    SLONG* ret_z,
    SLONG* along);

SLONG distance_to_line(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b);

//
// Inserts additional STOREY_TYPE_JUST_COLLISION facets around the map
// to stop Darci going where she aughtn't.
//

void insert_collision_facets(void);

//
// Creates a shockwave that damages people and knocks them down.
//

void create_shockwave(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG radius,
    SLONG maxdamage, // Remember- a person's maximum health is 200
    Thing* p_aggressor, ULONG just_people = 0); // Who caused the shockwave or NULL if you don't know

//
// Flags fences with a see-through texture as being FACET_FLAG_SEETHROUGH
//

void COLLIDE_find_seethrough_fences(void);

// ========================================================
//
// FASTNAV CALCULATION
//
// ========================================================

//
// Fast slide-along collision detection. These functions tell you id a
// person movement vector does not need full collision detection if it
// is wholly contained within the given square.
//

//
// The fastnav bits array.
//

typedef UBYTE COLLIDE_Fastnavrow[PAP_SIZE_HI >> 3];

extern COLLIDE_Fastnavrow* COLLIDE_fastnav;

//
// Calculates the fastnav bits array.
//

void COLLIDE_calc_fastnav_bits(void);

//
// Returns TRUE if you can fastnav in the given square.
//

#define COLLIDE_can_i_fastnav(x, z) (!(WITHIN((x), 0, PAP_SIZE_HI - 1) || !WITHIN((z), 0, PAP_SIZE_HI - 1) || (COLLIDE_fastnav[x][(z) >> 3] & (1 << ((z) & 0x7)))))

//
// Draws a debug cross over each fastnav square near the given place
//

void COLLIDE_debug_fastnav(
    SLONG world_x, // 8-bits per mapsquare.
    SLONG world_z);

#endif // FALLEN_HEADERS_COLLIDE_H
