// env2.cpp
//
// ENV stuff #2

#include "game.h"
#include <MFStdLib.h>
#include "env.h"
#include "Interfac.h"
#include "menufont.h"


CBYTE inifile[_MAX_PATH];
CBYTE strbuf[_MAX_PATH];


void ENV_load(CBYTE* fname)
{
    GetCurrentDirectory(_MAX_PATH, inifile);
    if (inifile[strlen(inifile) - 1] != '\\')
        strcat(inifile, "\\");
    strcat(inifile, fname);

    TRACE("Full INI file pathname = %s\n", inifile);

    SLONG local = GetPrivateProfileInt("MuckyFoot", "local", 0, inifile);

    if (local) {
        strcpy(inifile, "c:\\fallen.ini");
        TRACE("Using local INI file %s\n", inifile);
    }
}

CBYTE* ENV_get_value_string(CBYTE* name, CBYTE* section)
{
    GetPrivateProfileString(section, name, "", strbuf, _MAX_PATH, inifile);
    TRACE("[%s] %s = \"%s\"\n", section, name, strbuf);
    return strbuf[0] ? strbuf : NULL;
}

SLONG ENV_get_value_number(CBYTE* name, SLONG def, CBYTE* section)
{
    SLONG val = GetPrivateProfileInt(section, name, def, inifile);
    TRACE("[%s] %s = %d\n", section, name, val);
    if (stricmp(section, "Secret"))
        ENV_set_value_number(name, val, section); // don't write out "psx" key
    return val;
}

void ENV_set_value_string(CBYTE* name, CBYTE* value, CBYTE* section)
{
    WritePrivateProfileString(section, name, value, inifile);
}

void ENV_set_value_number(CBYTE* name, SLONG value, CBYTE* section)
{
    sprintf(strbuf, "%d", value);
    WritePrivateProfileString(section, name, strbuf, inifile);
}

