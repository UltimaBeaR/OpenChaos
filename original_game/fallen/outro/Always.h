#ifndef _ALWAYS_
#define _ALWAYS_

//
// Turn off the warning about [] arrays at the end of structures.
//

#pragma warning( disable : 4200 )


typedef		signed		long			SLONG;
typedef		unsigned	long			ULONG;
typedef		signed		short	int		SWORD;
typedef		unsigned	short	int		UWORD;
typedef		signed		char   			SBYTE;
typedef		unsigned 	char			UBYTE;
typedef					char			CBYTE;


//
// Standard libraries most often used.
//

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//
// Constants.
//

#define PI         (3.14159265F)
#undef INFINITY
#define INFINITY   (0x7fffffff)


//
// Useful defines.
//

#define MIN(mnx,mny)  		(((mnx) < (mny)) ? (mnx) : (mny))
#define MAX(mnx,mny)  		(((mnx) > (mny)) ? (mnx) : (mny))
#define MAX3(mnx,mny,mnz)	(((mnx) > (mny)) ? MAX(mnx,mnz) : MAX(mny,mnz))
#define MIN3(mnx,mny,mnz)	(((mnx) < (mny)) ? MIN(mnx,mnz) : MIN(mny,mnz))
#define MIN4(a,b,c,d)		(MIN(MIN(a,b), MIN(c,d)))
#define MAX4(a,b,c,d)		(MAX(MAX(a,b), MAX(c,d)))
#define SWAP(x,y)	   		{SLONG   SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_UB(x,y)   		{UBYTE   SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_UW(x,y)   		{UWORD   SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_FL(x,y)   		{float   SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_DB(x,y)   		{double  SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_HF(x,y)   		{Pointhf SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_P3(x,y)		{Point3d SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_VD(x,y)		{void   *SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_UWP(x,y)		{UWORD  *SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define SWAP_UBP(x,y)		{UBYTE  *SWAP_spare; SWAP_spare = x; x = y; y = SWAP_spare;}
#define WITHIN(x,a,b)  		((x) >= (a) && (x) <= (b))
#define SATURATE(x,a,b)		{if ((x) < (a)) (x) = (a); if ((x) > (b)) (x) = (b);}
#define SIGN(x)		   		(((x)) ? (((x) > 0) ? 1 : -1) : 0)
#define QLEN(x,y) 	   		(((x) > (y)) ? (x) + ((y) >> 1) : (y) + ((x) >> 1))
#define XOR(a,b)  	   		((!(a) && (b)) || ((a) && !(b)))
#define SQRT16(x)  			((sqrt((double) (x)) * 256))
//#define SHOW(a)			{TRACE(#a" = %d\n", (a));}
//#define SHOW_X(a)			{TRACE(#a" = 0x%x\n", (a));}
//#define SHOW_FL(a)		{TRACE(#a" = %f\n", (a));}


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

//
// Error checking macros
// 

#ifndef NDEBUG

void OS_string(CBYTE *fmt, ...);

#define ASSERT(x) {assert(x);}
#define TRACE	  OS_string
#define VERIFY(x) ASSERT(x)
#else
#define ASSERT(x) {__assume(x);}
#define TRACE	  
#define VERIFY(x) x

//
// In release build, get rid of the x == 1; warning message otherwise
// VERIFY generates it all the time!
//

#pragma warning( disable : 4553 )
#endif

static inline int ftol(float f)
{
	return (int)f;
}

// Globally useful structures.
//

typedef struct {float x, y, z;} Point3d;
typedef struct {float x, y;   } Point2d;
typedef struct {float x, z;   } Pointhf;
typedef struct {float u, v;   } Pointuv;

typedef struct
{
	float yaw;
	float pitch;
	float roll;

} Direction;

typedef struct
{
	UBYTE r;
	UBYTE g;
	UBYTE b;
	
} Colour;

//
// Fast approximation to sqrt(x*x + y*y)
//

static inline float qdist2(float x, float y)
{
	float ans;

	ASSERT(x >= 0.0F);
	ASSERT(y >= 0.0F);

	if (x > y)
	{
		ans = x + y * 0.5F;
	}
	else
	{
		ans = y + x * 0.5F;
	}

	return ans;
}


//
// Fast approximation to sqrt(x*x + y*y + z*z), x,y and z must
// be positive.
//

static inline float qdist3(float x, float y, float z)
{
	float ans;

	ASSERT(x >= 0.0F);
	ASSERT(y >= 0.0F);
	ASSERT(z >= 0.0F);

	if (x > y)
	{
		if (x > z)
		{
			//
			// x is the biggeset.
			//

			ans = x + (y + z) * 0.2941F;

			return ans;
		}
	}
	else
	{
		if (y > z)
		{
			//
			// y is the biggeset.
			//

			ans = y + (x + z) * 0.2941F;

			return ans;
		}
	}

	//
	// z is the biggeset.
	//

	ans = z + (x + y) * 0.2941F;

	return ans;
}


//
// A floating point number between 0 and 1.0F
// 

static inline float frand(void)
{
	SLONG irand = rand();
	float ans   = float(irand) * (1.0F / float(RAND_MAX));

	return ans;
}



#endif
