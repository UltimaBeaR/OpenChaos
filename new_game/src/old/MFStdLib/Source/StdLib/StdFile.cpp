// StdFile.cpp
// Guy Simmons, 18th December 1997.

// File I/O functions moved to engine/io/file.cpp (Stage 4).

#include <MFStdLib.h>

//---------------------------------------------------------------
// TraceText remains here — belongs to Host section, not file I/O.
// Will be moved when Host section is migrated.

void TraceText(CBYTE* fmt, ...)
{
    CBYTE message[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    OutputDebugString(message);
}
