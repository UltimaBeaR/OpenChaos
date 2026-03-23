#ifndef UI_CUTSCENES_OUTRO_OUTRO_MATRIX_H
#define UI_CUTSCENES_OUTRO_OUTRO_MATRIX_H

#include "fallen/outro/always.h" // Temporary: outro uses its own type definitions
#include "fallen/outro/Matrix.h" // Temporary: matrix functions not yet migrated to new/

// uc_orig: MATRIX_scale (fallen/outro/outroMatrix.cpp)
// Scales all 9 elements of a 3x3 matrix by mulby.
void MATRIX_scale(float matrix[9], float mulby);

#endif // UI_CUTSCENES_OUTRO_OUTRO_MATRIX_H
