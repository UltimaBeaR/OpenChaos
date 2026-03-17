//
// The camera.
//

#ifndef FALLEN_OUTRO_CAM_H
#define FALLEN_OUTRO_CAM_H

//
// The position of the camera. Only read from these vars.
//

extern float CAM_x;
extern float CAM_y;
extern float CAM_z;
extern float CAM_yaw;
extern float CAM_pitch;
extern float CAM_lens;
extern float CAM_matrix[9];

//
// Initialises the cameras. There is a free floating camera and
// a camera that always looks at a focus point.
//

void CAM_init(void);

//
// Processes the camera using the keyboard.
//

void CAM_process(void);

#endif // FALLEN_OUTRO_CAM_H
