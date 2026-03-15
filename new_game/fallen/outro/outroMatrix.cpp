#include "always.h"
#include "matrix.h"

void MATRIX_scale(float matrix[9], float mulby)
{
    matrix[0] *= mulby;
    matrix[1] *= mulby;
    matrix[2] *= mulby;
    matrix[3] *= mulby;
    matrix[4] *= mulby;
    matrix[5] *= mulby;
    matrix[6] *= mulby;
    matrix[7] *= mulby;
    matrix[8] *= mulby;
}
