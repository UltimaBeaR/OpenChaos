#include "engine/io/drive.h"
#include "engine/io/drive_globals.h"
#include <cstdio>
#include <cstring>
// Temporary: env.h not yet migrated
#include "fallen/Headers/env.h"
// Temporary: MF_Fopen/MF_Fclose from MFStdLib
#include "MFStdLib/Headers/MFStdLib.h"

// uc_orig: LocateCDROM (fallen/DDLibrary/Source/Drive.cpp)
void LocateCDROM(void)
{
    TexturesCD = ENV_get_value_number("textures", 1, "LocalInstall") ? false : true;
    SFX_CD = ENV_get_value_number("sfx", 1, "LocalInstall") ? false : true;
    MoviesCD = ENV_get_value_number("movies", 1, "LocalInstall") ? false : true;
    SpeechCD = ENV_get_value_number("speech", 1, "LocalInstall") ? false : true;

    if (!TexturesCD && !SFX_CD && !MoviesCD && !SpeechCD)
        return;

    char strings[256];

    if (!GetLogicalDriveStrings(255, strings)) {
        MessageBox(NULL, "Cannot locate system devices - serious internal error", NULL, MB_ICONERROR);
        exit(0);
    }

    for (;;) {
        char* sptr = strings;

        while (*sptr) {
            if (GetDriveType(sptr) == DRIVE_CDROM) {
                char filename[MAX_PATH];

                sprintf(filename, "%sclumps\\mib.txc", sptr);

                FILE* fd = MF_Fopen(filename, "rb");

                if (fd) {
                    MF_Fclose(fd);
                    strcpy(Path, sptr);
                    return;
                }
            }

            sptr += strlen(sptr) + 1;
        }

        if (MessageBox(NULL, "Cannot locate Urban Chaos CD-ROM", NULL, MB_ICONERROR | MB_RETRYCANCEL) == IDCANCEL) {
            break;
        }
    }
    exit(0);
}

// uc_orig: GetPath (fallen/DDLibrary/Source/Drive.cpp)
static inline char* GetPath(bool on_cd) { return on_cd ? Path : (char*)".\\"; }

// uc_orig: GetCDPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetCDPath(void) { return Path; }
// uc_orig: GetTexturePath (fallen/DDLibrary/Source/Drive.cpp)
char* GetTexturePath(void) { return GetPath(TexturesCD); }
// uc_orig: GetSFXPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetSFXPath(void) { return GetPath(SFX_CD); }
// uc_orig: GetMoviesPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetMoviesPath(void) { return GetPath(MoviesCD); }
// uc_orig: GetSpeechPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetSpeechPath(void) { return GetPath(SpeechCD); }
