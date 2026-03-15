

#include "game.h"

#include "inside2.h"
#include "memory.h"
#include "mav.h"

// struct 	DInsideRect	inside_rect[MAX_INSIDE_RECT];

UWORD next_inside_storey = 1;
UWORD next_inside_stair = 1;
SLONG next_inside_block = 1;

SLONG find_inside_room(SLONG inside, SLONG x, SLONG z)
{

    UBYTE* rooms;
    SWORD width;

    if (x >= inside_storeys[inside].MaxX || z >= inside_storeys[inside].MaxZ)
        return (0);
    if (x < inside_storeys[inside].MinX || z < inside_storeys[inside].MinZ)
        return (0);

    width = inside_storeys[inside].MaxX - inside_storeys[inside].MinX;
    rooms = &inside_block[inside_storeys[inside].InsideBlock];

    x -= inside_storeys[inside].MinX;
    z -= inside_storeys[inside].MinZ;

    z = z * width;
    return (rooms[x + z] & 0xf);
}

SLONG find_inside_flags(SLONG inside, SLONG x, SLONG z)
{

    UBYTE* rooms;
    SWORD width;

    if (x >= inside_storeys[inside].MaxX || z >= inside_storeys[inside].MaxZ)
        return (0);
    if (x < inside_storeys[inside].MinX || z < inside_storeys[inside].MinZ)
        return (0);

    width = inside_storeys[inside].MaxX - inside_storeys[inside].MinX;
    rooms = &inside_block[inside_storeys[inside].InsideBlock];

    x -= inside_storeys[inside].MinX;
    z -= inside_storeys[inside].MinZ;

    z = z * width;
    return (rooms[x + z]);
}

SLONG get_inside_alt(SLONG inside)
{
    /*
    #ifndef	PSX
            CBYTE	str[100];
            sprintf(str," Inside %d alt %d \n",inside,inside_storeys[inside].StoreyY);
            CONSOLE_text(str);
    #endif
    */
    return (inside_storeys[inside].StoreyY);
}

UBYTE slide_inside_stair = 0;
extern SLONG slide_door;

//
// mark's
//
SLONG person_slide_inside(
    SLONG inside,
    SLONG x1,
    SLONG y1,
    SLONG z1,
    SLONG* x2,
    SLONG* y2,
    SLONG* z2)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;

    SWORD mx = *x2 >> 16;
    SWORD mz = *z2 >> 16;
    SLONG ret_val = 0;

    slide_door = 0;
    slide_inside_stair = 0;

    //
    // All the room surrounding us.
    //

    UBYTE id_here = find_inside_flags(inside, mx, mz);
    UBYTE id_xs = find_inside_flags(inside, mx - 1, mz);
    UBYTE id_xl = find_inside_flags(inside, mx + 1, mz);
    UBYTE id_zs = find_inside_flags(inside, mx, mz - 1);
    UBYTE id_zl = find_inside_flags(inside, mx, mz + 1);

    if ((id_here & 0xf) == 0) {
        //
        // try to step outside of permitted area
        // return 1, to see if exited door
        //
        ret_val = 1;
    }
    //
    // The width of the walls.
    //

#define INSIDE_RADIUS (50 << 8)

    //
    // Work out which wall we must collide with.
    //

    UBYTE col = 0;

#define INSIDE_COL_XS (1 << 0)
#define INSIDE_COL_XL (1 << 1)
#define INSIDE_COL_ZS (1 << 2)
#define INSIDE_COL_ZL (1 << 3)

    if ((id_here & 0xf) != (id_xs & 0xf)) {
        col |= INSIDE_COL_XS;
    }
    if ((id_here & 0xf) != (id_xl & 0xf)) {
        col |= INSIDE_COL_XL;
    }
    if ((id_here & 0xf) != (id_zs & 0xf)) {
        col |= INSIDE_COL_ZS;
    }
    if ((id_here & 0xf) != (id_zl & 0xf)) {
        col |= INSIDE_COL_ZL;
    }

    //
    // Collide with each edge of the mapsquare you are in that doesn't lead to
    // the same room.
    //

    if (col & INSIDE_COL_XS) {
        if ((*x2 & 0xffff) < INSIDE_RADIUS) {
            //
            // Is there a door here?
            //

            if (id_here & FLAG_DOOR_LEFT) {
                //
                // Collide with a door frame. An elipse at each end of the mapsquare.
                //

                if ((*z2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dz = 0x10000 - dz;
                    dz >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dz <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= dx;
                    *z2 |= 0x10000 - dz;
                } else {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dz >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dz <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= dx;
                    *z2 |= dz;
                }
            } else {
                //
                // Collide with a wall.
                //

                *x2 &= ~0xffff;
                *x2 |= INSIDE_RADIUS;
            }
        }
    }

    if (col & INSIDE_COL_XL) {
        if ((*x2 & 0xffff) > 0x10000 - INSIDE_RADIUS) {
            //
            // Is there a door here?
            //

            if (id_xl & FLAG_DOOR_LEFT) {
                //
                // Collide with a door frame. An elipse at each end of the mapsquare.
                //

                if ((*z2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx = 0x10000 - dx;
                    dz = 0x10000 - dz;
                    dz >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dz <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= 0x10000 - dx;
                    *z2 |= 0x10000 - dz;
                } else {
                    dx = *x2 & 0xffff;
                    dx = 0x10000 - dx;
                    dz = *z2 & 0xffff;
                    dz >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dz <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= 0x10000 - dx;
                    *z2 |= dz;
                }
            } else {
                *x2 &= ~0xffff;
                *x2 |= 0x10000 - INSIDE_RADIUS;
            }
        }
    }

    if (col & INSIDE_COL_ZS) {
        if ((*z2 & 0xffff) < INSIDE_RADIUS) {
            //
            // Is there a door here?
            //

            if (id_here & FLAG_DOOR_UP) {
                //
                // Collide with a door frame. An elipse at each end of the mapsquare.
                //

                if ((*x2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx = 0x10000 - dx;
                    dx >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dx <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= 0x10000 - dx;
                    *z2 |= dz;
                } else {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dx <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= dx;
                    *z2 |= dz;
                }
            } else {
                *z2 &= ~0xffff;
                *z2 |= INSIDE_RADIUS;
            }
        }
    }

    if (col & INSIDE_COL_ZL) {
        if ((*z2 & 0xffff) > 0x10000 - INSIDE_RADIUS) {
            //
            // Is there a door here?
            //

            if (id_zl & FLAG_DOOR_UP) {
                //
                // Collide with a door frame. An elipse at each end of the mapsquare.
                //

                if ((*x2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx = 0x10000 - dx;
                    dz = 0x10000 - dz;
                    dx >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dx <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= 0x10000 - dx;
                    *z2 |= 0x10000 - dz;
                } else {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dz = 0x10000 - dz;
                    dx >>= 1; // To make it an elipse.

                    dist = QDIST2(dx, dz); // dx and dz are both > 0

                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }

                    dx <<= 1; // Go back out of 'ellpise space'

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    *x2 |= dx;
                    *z2 |= 0x10000 - dz;
                }
            } else {
                *z2 &= ~0xffff;
                *z2 |= 0x10000 - INSIDE_RADIUS;
            }
        }
    }

    //
    // Sliding against sticky-outy corner bits.
    //

    if (!(col & (INSIDE_COL_XS | INSIDE_COL_ZS))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx - 1, mz - 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;

            dist = QDIST2(dx, dz);

            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }

            *x2 &= ~0xffff;
            *z2 &= ~0xffff;

            *x2 |= dx;
            *z2 |= dz;
        }
    }

    if (!(col & (INSIDE_COL_XS | INSIDE_COL_ZL))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx - 1, mz + 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;
            dz = 0x10000 - dz;

            dist = QDIST2(dx, dz);

            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }

            *x2 &= ~0xffff;
            *z2 &= ~0xffff;

            *x2 |= dx;
            *z2 |= 0x10000 - dz;
        }
    }

    if (!(col & (INSIDE_COL_XL | INSIDE_COL_ZS))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx + 1, mz - 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;
            dx = 0x10000 - dx;

            dist = QDIST2(dx, dz);

            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }

            *x2 &= ~0xffff;
            *z2 &= ~0xffff;

            *x2 |= 0x10000 - dx;
            *z2 |= dz;
        }
    }

    if (!(col & (INSIDE_COL_XL | INSIDE_COL_ZL))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx + 1, mz + 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;
            dx = 0x10000 - dx;
            dz = 0x10000 - dz;

            dist = QDIST2(dx, dz);

            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }

            *x2 &= ~0xffff;
            *z2 &= ~0xffff;

            *x2 |= 0x10000 - dx;
            *z2 |= 0x10000 - dz;
        }
    }

    //
    // I wander what we are meant to be returning!
    //

    {
        UBYTE id_here = find_inside_flags(inside, *x2 >> 16, *z2 >> 16);
        if (id_here & FLAG_INSIDE_STAIR) {
            slide_inside_stair = id_here & FLAG_INSIDE_STAIR;
        }
    }

    return (ret_val);
}

/*
SLONG	find_stair_routes(UWORD inside,UBYTE x,UBYTE z,UWORD *up,UWORD *down)
{
        SLONG	stair;
//	ASSERT(0);

        stair=inside_storeys[inside].StairCaseHead;
        while(stair)
        {
                UBYTE	sx,sz;
                UBYTE	dir;

                sx=inside_stairs[stair].X;
                sz=inside_stairs[stair].Z;

                dir=GET_STAIR_DIR(inside_stairs[stair].Flags);


                if(x==sx && z==sz)
                {
                        *up=inside_stairs[stair].UpInside;
                        *down=inside_stairs[stair].DownInside;
                        return(1);
                }

                switch(dir)
                {
                        case	0:
                                //n
                                sz--;
                                break;
                        case	1:
                                //e
                                sx++;
                                break;
                        case	2:
                                //s
                                sz++;
                                break;
                        case	3:
                                //w
                                sx--;
                                break;

                }

                if(x==sx && z==sz)
                {
                        *up=inside_stairs[stair].UpInside;
                        *down=inside_stairs[stair].DownInside;
                        return(1);
                }
                stair=inside_stairs[stair].NextStairs;
        }
        return(0);
}
*/
UWORD find_stair_in(SLONG mx, SLONG mz, SLONG* rdx, SLONG* rdz, UWORD inside)
{
    SLONG dx, dz;
    SLONG stair;
    SLONG x_ok, z_ok;
    //	ASSERT(0);

    stair = inside_storeys[inside].StairCaseHead;
    while (stair) {
        UBYTE sx, sz;
        UBYTE dir;

        sx = inside_stairs[stair].X;
        sz = inside_stairs[stair].Z;

        dir = GET_STAIR_DIR(inside_stairs[stair].Flags);

        dx = 0;
        dz = 0;

        switch (dir) {
        case 0:
            dz = -2;
            dx = 1;
            // n
            break;
        case 1:
            dx = 2;
            dz = 1;
            // e
            break;
        case 2:
            dz = 2;
            dx = -1;
            // s
            break;
        case 3:
            dx = -2;
            dz = -1;
            // w
            break;
        }

        x_ok = 0;
        z_ok = 0;
        if (dx > 0) {
            if (mx >= sx && mx < sx + dx) {
                x_ok = 1;
            }
        } else {
            if (mx >= sx + dx && mx < sx) {
                x_ok = 1;
            }
        }

        if (x_ok) {
            if (dz > 0) {
                if (mz >= sz && mz < sz + dz) {
                    z_ok = 1;
                }
            } else {
                if (mz >= sz + dz && mz < sz) {
                    z_ok = 1;
                }
            }

            if (x_ok && z_ok) {
                *rdx = dx;
                *rdz = dz;
                return (stair);
            }
        }
        stair = inside_stairs[stair].NextStairs;
    }
    return (0);
}

SLONG find_stair_y(Thing* p_person, SLONG* y1, SLONG x, SLONG y, SLONG z, UWORD* new_floor)
{
    SLONG dx, dz;
    SLONG stair;
    SLONG off_x, off_z;
    SLONG mx, mz;
    SLONG s, t;
    SLONG new_y;
    SLONG can_move = 1;

    MSG_add(" INDOORS INDEX %d  INDEX NEXT %d \n", INDOORS_INDEX, INDOORS_INDEX_NEXT);
    INDOORS_INDEX_NEXT = 0;
    INDOORS_INDEX_FADE = 255;

    *new_floor = 0;

    mx = x >> 8;
    mz = z >> 8;

    stair = find_stair_in(mx, mz, &dx, &dz, p_person->Genus.Person->InsideIndex);

    if (stair) {
        off_x = x - (inside_stairs[stair].X << 8);
        off_z = z - (inside_stairs[stair].Z << 8);

        //		MSG_add(" find stair y dx %d dz %d off_x %d off_z %d \n",dx,dz,off_x,off_z);

        //
        // do this super quick way of getting s & t without muls or divs
        //

        if (abs(dx) == 2) {
            off_x >>= 1;
        }
        if (abs(dz) == 2) {
            off_z >>= 1;
        }

        s = abs(off_x);
        t = abs(off_z);
        if (abs(dx) == 2) {
            SWAP(s, t);
        }
        //		MSG_add("ox %d oz %d s %d t %d \n",off_x,off_z,s,t);

        //
        // given s and t are we over a step
        //

        if (inside_stairs[stair].UpInside) {
            if (s < 128 && t > 64) {
                if (t > 192) {
                    new_y = 256;
                } else {
                    new_y = (t - 64) >> 3;
                    new_y <<= 4;
                }
                new_y += inside_storeys[INDOORS_INDEX].StoreyY;

                MSG_add(" inside  new_y = %d old y %d \n", new_y, y);
                if (abs(new_y - y) < 96) {
                    //
                    // go up the stairs
                    //

                    if (t > 128) {
                        //
                        // ping up the stairs
                        //
                        MSG_add(" NEW FLOOR up %d prev %d\n", inside_stairs[stair].UpInside, INDOORS_INDEX);
                        *new_floor = inside_stairs[stair].UpInside;
                        MSG_add(" upthe stairs is index %d \n", inside_stairs[stair].UpInside);

                        INDOORS_INDEX_FADE = 0;
                        INDOORS_INDEX_NEXT = INDOORS_INDEX;
                        INDOORS_ROOM_NEXT = find_inside_room(INDOORS_INDEX_NEXT, x >> 8, z >> 8);
                    }
                    *y1 = new_y;
                    return (1);
                } else {
                    can_move = 0;
                }
            }
        }
        if (inside_stairs[stair].DownInside) {
            if (s < 128 && t > 64) {
                if (t > 192) {
                    new_y = 0;
                } else {
                    new_y = (t - 64) >> 4;
                    new_y <<= 5;
                    new_y -= 256;
                }
                new_y += inside_storeys[INDOORS_INDEX].StoreyY;
                if (abs(new_y - y) < 96) {
                    //
                    // go down the stairs
                    //

                    if (t < 128) {
                        //
                        // ping down the stairs
                        //
                        *new_floor = inside_stairs[stair].DownInside;
                        MSG_add(" NEW FLOOR down %d prev %d\n", inside_stairs[stair].DownInside, INDOORS_INDEX);
                    } else {

                        if (t < 192) {
                            INDOORS_INDEX_FADE = MIN(((t - 128) << 2), 255);

                            INDOORS_INDEX_NEXT = inside_stairs[stair].DownInside;
                            INDOORS_ROOM_NEXT = find_inside_room(INDOORS_INDEX_NEXT, x >> 8, z >> 8);
                        } else {
                            INDOORS_INDEX_NEXT = 0;
                            INDOORS_ROOM_NEXT = 0;
                            INDOORS_INDEX_FADE = 255; // MIN(((t)<<1),255);
                        }
                    }
                    *y1 = new_y;
                    return (1);
                } else {
                    can_move = 0;
                }
            }
        }
    }

    new_y = inside_storeys[INDOORS_INDEX].StoreyY;
    *y1 = new_y;
    return (can_move);
}
/*
void	stair_teleport_bodge(Thing *p_person)
{
        SLONG	x,z;
        UWORD	up,down;
        SLONG	found;

        x=p_person->WorldPos.X>>16;
        z=p_person->WorldPos.Z>>16;

        found=find_stair_routes(p_person->Genus.Person->InsideIndex,x,z,&up,&down);



        if(found)
        {
//		ASSERT(0);
                if(up&&(slide_inside_stair&FLAG_INSIDE_STAIR_UP))
                {
                        MSG_add(" go upstairs from %d to %d \n",INDOORS_INDEX,up);
                        INDOORS_INDEX=up;
                }
                if(down&&(slide_inside_stair&FLAG_INSIDE_STAIR_DOWN))
                {
                        MSG_add(" go upstairs from %d to %d \n",INDOORS_INDEX,up);
                        INDOORS_INDEX=down;
                }
//		if(down)
//			INDOORS_INDEX=down;

                p_person->Genus.Person->InsideIndex=INDOORS_INDEX;
                INDOORS_ROOM=find_inside_room(INDOORS_INDEX,x,z);
                p_person->WorldPos.Y=get_inside_alt(INDOORS_INDEX);

        }

}
*/

