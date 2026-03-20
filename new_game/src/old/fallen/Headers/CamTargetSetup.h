//	CameraSetup.h
//	Matthew Rosenfeld, 1st October 1998.

#ifndef FALLEN_HEADERS_CAMTARGETSETUP_H
#define FALLEN_HEADERS_CAMTARGETSETUP_H

#include "Mission.h"

//---------------------------------------------------------------

void do_camtarget_setup(EventPoint* ep);
CBYTE* get_camtarget_message(EventPoint* ep, CBYTE* msg);

//---------------------------------------------------------------

#endif // FALLEN_HEADERS_CAMTARGETSETUP_H
