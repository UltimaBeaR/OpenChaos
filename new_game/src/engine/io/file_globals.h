#ifndef ENGINE_IO_FILE_GLOBALS_H
#define ENGINE_IO_FILE_GLOBALS_H

#include "engine/core/types.h"

// uc_orig: MAX_LENGTH_OF_BASE_PATH (MFStdLib/Source/StdLib/StdFile.cpp)
#define MAX_LENGTH_OF_BASE_PATH 64
// uc_orig: MAX_LENGTH_OF_FULL_NAME (MFStdLib/Source/StdLib/StdFile.cpp)
#define MAX_LENGTH_OF_FULL_NAME (MAX_LENGTH_OF_BASE_PATH + 128)

// uc_orig: cBasePath (MFStdLib/Source/StdLib/StdFile.cpp)
extern CBYTE cBasePath[MAX_LENGTH_OF_BASE_PATH + 1];
// uc_orig: cTempFilename (MFStdLib/Source/StdLib/StdFile.cpp)
extern CBYTE cTempFilename[MAX_LENGTH_OF_FULL_NAME + 1];

#endif // ENGINE_IO_FILE_GLOBALS_H
