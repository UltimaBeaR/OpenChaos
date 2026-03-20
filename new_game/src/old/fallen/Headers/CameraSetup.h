//	CameraSetup.h
//	Matthew Rosenfeld, 30th September 1998.

#ifndef FALLEN_HEADERS_CAMERASETUP_H
#define FALLEN_HEADERS_CAMERASETUP_H

#include "Mission.h"

//---------------------------------------------------------------

void do_camera_setup(EventPoint* ep);
CBYTE* get_camera_message(EventPoint* ep, CBYTE* msg);

//---------------------------------------------------------------

#endif // FALLEN_HEADERS_CAMERASETUP_H
