#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "engine/io/env.h"
#include "engine/io/env_globals.h"

// uc_orig: ENV_load (fallen/Source/env2.cpp)
void ENV_load(CBYTE* fname)
{
    GetCurrentDirectory(_MAX_PATH, env_inifile);
    if (env_inifile[strlen(env_inifile) - 1] != '\\' && env_inifile[strlen(env_inifile) - 1] != '/')
        strcat(env_inifile, "/");
    strcat(env_inifile, fname);

    SLONG local = GetPrivateProfileInt("MuckyFoot", "local", 0, env_inifile);

    if (local) {
        // uc-abs-path: was "c:\fallen.ini"
        strcpy(env_inifile, "fallen.ini");
    }
}

// uc_orig: ENV_get_value_string (fallen/Source/env2.cpp)
CBYTE* ENV_get_value_string(CBYTE* name, CBYTE* section)
{
    GetPrivateProfileString(section, name, "", env_strbuf, _MAX_PATH, env_inifile);
    return env_strbuf[0] ? env_strbuf : NULL;
}

// uc_orig: ENV_get_value_number (fallen/Source/env2.cpp)
SLONG ENV_get_value_number(CBYTE* name, SLONG def, CBYTE* section)
{
    SLONG val = GetPrivateProfileInt(section, name, def, env_inifile);
    if (stricmp(section, "Secret"))
        ENV_set_value_number(name, val, section); // don't write out "psx" key
    return val;
}

// uc_orig: ENV_set_value_string (fallen/Source/env2.cpp)
void ENV_set_value_string(CBYTE* name, CBYTE* value, CBYTE* section)
{
    WritePrivateProfileString(section, name, value, env_inifile);
}

// uc_orig: ENV_set_value_number (fallen/Source/env2.cpp)
void ENV_set_value_number(CBYTE* name, SLONG value, CBYTE* section)
{
    sprintf(env_strbuf, "%d", value);
    WritePrivateProfileString(section, name, env_strbuf, env_inifile);
}
