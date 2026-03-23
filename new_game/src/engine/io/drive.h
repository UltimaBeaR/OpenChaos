#ifndef ENGINE_IO_DRIVE_H
#define ENGINE_IO_DRIVE_H

// CD-ROM / asset path resolution.
// Locates the game CD and provides paths for textures, SFX, movies, speech.

// uc_orig: LocateCDROM (fallen/DDLibrary/Source/Drive.cpp)
void LocateCDROM(void);

// uc_orig: GetCDPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetCDPath(void);
// uc_orig: GetTexturePath (fallen/DDLibrary/Source/Drive.cpp)
char* GetTexturePath(void);
// uc_orig: GetSFXPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetSFXPath(void);
// uc_orig: GetMoviesPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetMoviesPath(void);
// uc_orig: GetSpeechPath (fallen/DDLibrary/Source/Drive.cpp)
char* GetSpeechPath(void);

#endif // ENGINE_IO_DRIVE_H
