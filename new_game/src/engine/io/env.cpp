// INI configuration file reader/writer.
// Replaces Win32 GetPrivateProfileInt/String/WritePrivateProfileString
// with a simple cross-platform INI parser.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/io/env.h"
#include "engine/io/env_globals.h"

// --- Simple INI parser (internal) ---

// Trim leading/trailing whitespace in place. Returns pointer into the same buffer.
static char* ini_trim(char* s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return s;
}

// Read a string value from an INI file. Returns true if found.
static bool ini_read_string(const char* filepath, const char* section, const char* key,
                            char* out, int out_size)
{
    FILE* f = fopen(filepath, "r");
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

// Read an integer value from an INI file. Returns def if not found.
static int ini_read_int(const char* filepath, const char* section, const char* key, int def)
{
    char buf[64];
    if (ini_read_string(filepath, section, key, buf, sizeof(buf)) && buf[0])
        return atoi(buf);
    return def;
}

// Write a string value to an INI file. Creates the file/section/key as needed.
// Rewrites the entire file to update or insert the key.
static void ini_write_string(const char* filepath, const char* section, const char* key,
                             const char* value)
{
    // Read entire file into memory.
    FILE* f = fopen(filepath, "r");

    // Generous buffer — INI files are small.
    static char filebuf[32768];
    int filelen = 0;

    if (f) {
        filelen = (int)fread(filebuf, 1, sizeof(filebuf) - 1, f);
        fclose(f);
    }
    filebuf[filelen] = '\0';

    // Parse and rebuild, replacing the key if found.
    static char outbuf[32768];
    int outlen = 0;
    bool key_written = false;
    bool in_target_section = false;
    bool section_found = false;

    char* p = filebuf;
    while (*p) {
        // Read one line.
        char* linestart = p;
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        int linelen = (int)(p - linestart);

        // Copy line into temp buffer for parsing.
        char line[512];
        int copylen = linelen < (int)sizeof(line) - 1 ? linelen : (int)sizeof(line) - 1;
        memcpy(line, linestart, copylen);
        line[copylen] = '\0';

        char* trimmed = ini_trim(line);

        if (*trimmed == '[') {
            // If we were in the target section and didn't write the key yet, insert it.
            if (in_target_section && !key_written) {
                outlen += sprintf(outbuf + outlen, "%s=%s\n", key, value);
                key_written = true;
            }

            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                in_target_section = (oc_stricmp(trimmed + 1, section) == 0);
                if (in_target_section) section_found = true;
            }

            // Copy original line.
            memcpy(outbuf + outlen, linestart, linelen);
            outlen += linelen;
            continue;
        }

        if (in_target_section && *trimmed && *trimmed != ';' && *trimmed != '#') {
            char* eq = strchr(trimmed, '=');
            if (eq) {
                *eq = '\0';
                char* k = ini_trim(trimmed);
                if (oc_stricmp(k, key) == 0) {
                    // Replace this line.
                    outlen += sprintf(outbuf + outlen, "%s=%s\n", key, value);
                    key_written = true;
                    continue;
                }
            }
        }

        // Copy original line.
        memcpy(outbuf + outlen, linestart, linelen);
        outlen += linelen;
    }

    // If key not written yet, append section + key.
    if (!key_written) {
        if (in_target_section) {
            // We were at end of file still in the target section.
            outlen += sprintf(outbuf + outlen, "%s=%s\n", key, value);
        } else {
            if (!section_found) {
                // Add newline if file doesn't end with one.
                if (outlen > 0 && outbuf[outlen - 1] != '\n')
                    outbuf[outlen++] = '\n';
                outlen += sprintf(outbuf + outlen, "[%s]\n", section);
            }
            outlen += sprintf(outbuf + outlen, "%s=%s\n", key, value);
        }
    }

    f = fopen(filepath, "w");
    if (f) {
        fwrite(outbuf, 1, outlen, f);
        fclose(f);
    }
}

// --- Public API ---

// uc_orig: ENV_load (fallen/Source/env2.cpp)
void ENV_load(CBYTE* fname)
{
    oc_getcwd(env_inifile, ENV_MAX_PATH);
    size_t len = strlen(env_inifile);
    if (len > 0 && env_inifile[len - 1] != '\\' && env_inifile[len - 1] != '/')
        strcat(env_inifile, "/");
    strcat(env_inifile, fname);

    SLONG local = ini_read_int(env_inifile, "MuckyFoot", "local", 0);

    if (local) {
        // uc-abs-path: was "c:\fallen.ini"
        strcpy(env_inifile, "fallen.ini");
    }
}

// uc_orig: ENV_get_value_string (fallen/Source/env2.cpp)
CBYTE* ENV_get_value_string(CBYTE* name, CBYTE* section)
{
    ini_read_string(env_inifile, section, name, env_strbuf, ENV_MAX_PATH);
    return env_strbuf[0] ? env_strbuf : NULL;
}

// uc_orig: ENV_get_value_number (fallen/Source/env2.cpp)
SLONG ENV_get_value_number(CBYTE* name, SLONG def, CBYTE* section)
{
    SLONG val = ini_read_int(env_inifile, section, name, def);
    if (oc_stricmp(section, "Secret"))
        ENV_set_value_number(name, val, section); // don't write out "psx" key
    return val;
}

// uc_orig: ENV_set_value_string (fallen/Source/env2.cpp)
void ENV_set_value_string(CBYTE* name, CBYTE* value, CBYTE* section)
{
    ini_write_string(env_inifile, section, name, value);
}

// uc_orig: ENV_set_value_number (fallen/Source/env2.cpp)
void ENV_set_value_number(CBYTE* name, SLONG value, CBYTE* section)
{
    sprintf(env_strbuf, "%d", value);
    ini_write_string(env_inifile, section, name, env_strbuf);
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
    FILE* f = fopen(filepath, "r");
    if (!f) { out[0] = '\0'; out[1] = '\0'; return false; }

    bool in_section = false;
    char line[512];
    int pos = 0;

    while (fgets(line, sizeof(line), f)) {
        char* trimmed = ini_trim(line);
        if (!*trimmed || *trimmed == ';' || *trimmed == '#') continue;

        if (*trimmed == '[') {
            if (in_section) break;
            char* end = strchr(trimmed, ']');
            if (end) { *end = '\0'; in_section = (oc_stricmp(trimmed + 1, section) == 0); }
            continue;
        }

        if (!in_section) continue;

        int len = (int)strlen(trimmed);
        if (pos + len + 2 > out_size) break;
        memcpy(out + pos, trimmed, len);
        pos += len;
        out[pos++] = '\0';
    }

    out[pos] = '\0';
    fclose(f);
    return in_section;
}
