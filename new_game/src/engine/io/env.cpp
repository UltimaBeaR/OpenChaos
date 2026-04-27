// INI configuration file reader/writer.
// Replaces Win32 GetPrivateProfileInt/String/WritePrivateProfileString
// with a simple cross-platform INI parser.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/io/env.h"
#include "engine/io/file.h"
#include "engine/io/oc_config.h"

// --- Simple INI parser (internal) ---

// Trim leading/trailing whitespace in place. Returns pointer into the same buffer.
static char* ini_trim(char* s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
    return s;
}

// Read a string value from an INI file. Returns true if found.
static bool ini_read_string(const char* filepath, const char* section, const char* key,
    char* out, int out_size)
{
    FILE* f = fopen_ci(filepath, "r");
    if (!f) {
        out[0] = '\0';
        return false;
    }

    bool in_section = false;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        char* trimmed = ini_trim(line);

        // Skip empty lines and comments
        if (!*trimmed || *trimmed == ';' || *trimmed == '#')
            continue;

        // Section header
        if (*trimmed == '[') {
            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                in_section = (oc_stricmp(trimmed + 1, section) == 0);
            }
            continue;
        }

        if (!in_section)
            continue;

        // Key=Value
        char* eq = strchr(trimmed, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char* k = ini_trim(trimmed);
        char* v = ini_trim(eq + 1);

        if (oc_stricmp(k, key) == 0) {
            strncpy(out, v, out_size - 1);
            out[out_size - 1] = '\0';
            fclose(f);
            return true;
        }
    }

    fclose(f);
    out[0] = '\0';
    return false;
}

// Read a string value from an in-memory INI string. Returns true if found.
static bool ini_read_string_mem(const char* ini_data, const char* section, const char* key,
    char* out, int out_size)
{
    if (!ini_data) {
        out[0] = '\0';
        return false;
    }

    bool in_section = false;
    const char* p = ini_data;

    while (*p) {
        // Extract one line.
        const char* line_end = p;
        while (*line_end && *line_end != '\n')
            line_end++;

        char line[512];
        int len = (int)(line_end - p);
        if (len >= (int)sizeof(line))
            len = (int)sizeof(line) - 1;
        memcpy(line, p, len);
        line[len] = '\0';

        p = *line_end ? line_end + 1 : line_end;

        char* trimmed = ini_trim(line);
        if (!*trimmed || *trimmed == ';' || *trimmed == '#')
            continue;

        if (*trimmed == '[') {
            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                in_section = (oc_stricmp(trimmed + 1, section) == 0);
            }
            continue;
        }

        if (!in_section)
            continue;

        char* eq = strchr(trimmed, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char* k = ini_trim(trimmed);
        char* v = ini_trim(eq + 1);

        if (oc_stricmp(k, key) == 0) {
            strncpy(out, v, out_size - 1);
            out[out_size - 1] = '\0';
            return true;
        }
    }

    out[0] = '\0';
    return false;
}

// Read an integer value from an INI file. Returns def if not found.
static int ini_read_int(const char* filepath, const char* section, const char* key, int def)
{
    char buf[64];
    if (ini_read_string(filepath, section, key, buf, sizeof(buf)) && buf[0])
        return atoi(buf);
    return def;
}


// --- Public API ---

// uc_orig: ENV_load (fallen/Source/env2.cpp)
void ENV_load(CBYTE* fname)
{
    OC_CONFIG_load(fname);
}

// uc_orig: ENV_get_value_number (fallen/Source/env2.cpp)
SLONG ENV_get_value_number(CBYTE* name, SLONG def, CBYTE* section)
{
    return OC_CONFIG_get_int(section, name, def);
}

// uc_orig: ENV_set_value_number (fallen/Source/env2.cpp)
void ENV_set_value_number(CBYTE* name, SLONG value, CBYTE* section)
{
    OC_CONFIG_set_int(section, name, value);
}

// --- Public INI access (cross-platform replacement for Win32 GetPrivateProfile*) ---

int INI_get_int(const char* filepath, const char* section, const char* key, int def)
{
    return ini_read_int(filepath, section, key, def);
}

bool INI_get_string(const char* filepath, const char* section, const char* key, char* out, int out_size)
{
    return ini_read_string(filepath, section, key, out, out_size);
}

// Reads all key=value pairs from a section into a double-null-terminated buffer.
// Each entry is "key=value\0", terminated by an extra "\0".
bool INI_get_section(const char* filepath, const char* section, char* out, int out_size)
{
    FILE* f = fopen_ci(filepath, "r");
    if (!f) {
        out[0] = '\0';
        out[1] = '\0';
        return false;
    }

    bool in_section = false;
    char line[512];
    int pos = 0;

    while (fgets(line, sizeof(line), f)) {
        char* trimmed = ini_trim(line);
        if (!*trimmed || *trimmed == ';' || *trimmed == '#')
            continue;

        if (*trimmed == '[') {
            if (in_section)
                break;
            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                in_section = (oc_stricmp(trimmed + 1, section) == 0);
            }
            continue;
        }

        if (!in_section)
            continue;

        int len = (int)strlen(trimmed);
        if (pos + len + 2 > out_size)
            break;
        memcpy(out + pos, trimmed, len);
        pos += len;
        out[pos++] = '\0';
    }

    out[pos] = '\0';
    fclose(f);
    return in_section;
}
