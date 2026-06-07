#include "assets/xlat_str.h"
#include "assets/xlat_str_globals.h"
#include "engine/io/env.h" // INI_get_string (config.ini [Game] language=)
#include "engine/io/file.h"

#include <algorithm> // std::sort
#include <cctype> // tolower
#include <filesystem> // directory scan for the language fallback
#include <string>
#include <vector>

// Byte offset into xlat_ptr where control remapping strings start.
// uc_orig: REMAP_OFFSET (fallen/Source/xlat_str.cpp)
#define REMAP_OFFSET (230)

// Reset MBCS state between strings.
// uc_orig: mbcs_reset (fallen/Source/xlat_str.cpp)
static void mbcs_reset()
{
    previous_byte = 0;
}

// Advance one character (not byte) in an MBCS string.
// On PC, always advances by one byte since MBCS is disabled.
// uc_orig: mbcs_inc_char (fallen/Source/xlat_str.cpp)
static CBYTE* mbcs_inc_char(CBYTE*& c)
{
    c++;
    return c;
}

// Return the previous character pointer without modifying c.
// uc_orig: mbcs_prev_char (fallen/Source/xlat_str.cpp)
static CBYTE* mbcs_prev_char(CBYTE* c, CBYTE* base)
{
    return c - 1;
}

// MBCS-safe strchr: finds first occurrence of c in str.
// uc_orig: mbcs_strchr (fallen/Source/xlat_str.cpp)
static CBYTE* mbcs_strchr(CBYTE* str, CBYTE c)
{
    CBYTE *scan = str, *end = str + strlen(str);
    while (scan < end) {
        if (*scan == c)
            return scan;
        mbcs_inc_char(scan);
    }
    return 0;
}

// MBCS-safe strspnp: skips characters found in badlist, returns first non-match.
// uc_orig: mbcs_strspnp (fallen/Source/xlat_str.cpp)
static CBYTE* mbcs_strspnp(CBYTE* str, CBYTE* badlist)
{
    while (*str && mbcs_strchr(badlist, *str))
        mbcs_inc_char(str);
    return str;
}

// Returns the remapped control string for the given remap ID (e.g. XCTL_JUMP).
// uc_orig: XLAT_remap (fallen/Source/xlat_str.cpp)
static CBYTE* XLAT_remap(UBYTE remap_id)
{
    return xlat_ptr[REMAP_OFFSET + remap_id];
}

// uc_orig: XLAT_str_ptr (fallen/Source/xlat_str.cpp)
CBYTE* XLAT_str_ptr(SLONG string_id)
{
    CBYTE* xlated = xlat_ptr[string_id];
    if ((xlat_upto == xlat_set) || (!xlat_upto) || !xlated) {
        return "missing language file. get t:\\lang-english.txt and stick it in your fallen\\text directory";
    }
    return xlated;
}

// Returns the translated string for string_id, expanding any control token
// substitutions (byte value 1 followed by a remap ID).
// uc_orig: XLAT_str (fallen/Source/xlat_str.cpp)
CBYTE* XLAT_str(SLONG string_id, CBYTE* xlat_dest)
{
    CBYTE* xlated = xlat_ptr[string_id];
    CBYTE *ptr, *ptr2, *buff;
    UWORD ofs;

    if (!xlat_dest)
        xlat_dest = xlat_buffer;

    buff = xlat_dest;

    if ((xlat_upto == xlat_set) || (!xlat_upto) || !xlated) {
        ASSERT(UC_FALSE);
        return "missing language file. get t:\\lang-english.txt and stick it in your fallen\\text directory";
    }

    memset(xlat_buffer, 0, _MAX_PATH + 100);

    while (ptr = mbcs_strchr(xlated, 1)) {
        ofs = ptr - xlated;
        if (ofs) {
            strncpy(xlat_dest, xlated, ofs);
            xlat_dest += ofs;
        }
        ptr++;
        ptr2 = XLAT_remap(*ptr);
        strcpy(xlat_dest, ptr2);
        xlat_dest += strlen(ptr2);
        xlated = ptr + 1;
    }

    strcpy(xlat_dest, xlated);

    return buff;
}

// Reads one line from an open language file into txt, stripping CR/LF.
// uc_orig: XLAT_load_string (fallen/Source/xlat_str.cpp)
static CBYTE* XLAT_load_string(MFFileHandle& file, CBYTE* txt)
{
    CBYTE *ptr = txt, *temp;
    UWORD emergency_bail_out_for_martins_machine = 2000;

    mbcs_reset();

    *ptr = 0;
    while (emergency_bail_out_for_martins_machine--) {
        if (FileRead(file, ptr, 1) == FILE_READ_ERROR) {
            *ptr = 0;
            return txt;
        };
        if (*ptr == 10)
            break;
        ptr++;
    }

    temp = mbcs_prev_char(ptr, txt);
    if (*temp == 13)
        *temp = 0;
    else
        *ptr = 0;
    return txt;
}

// Replaces "|N" control tokens in txt with single-byte substitution sequences.
// Token format: '|' followed by decimal digits -> byte 1 + remap ID byte.
// uc_orig: XLAT_compress_tokens (fallen/Source/xlat_str.cpp)
static void XLAT_compress_tokens(CBYTE* txt)
{
    CBYTE *test, *scan;
    CBYTE buff[10];
    UWORD skip;
    while (scan = mbcs_strchr(txt, '|')) {
        test = scan;
        test = mbcs_strspnp(mbcs_inc_char(test), "0123456789");
        if (test) {
            skip = test - scan;
            strncpy(buff, scan + 1, skip - 1);
            *(buff + skip - 1) = 0;
            *scan = 1;
            scan++;
            *scan = atoi(buff);
            memmove(scan + 1, test, strlen(test) + 1);
            txt = scan + 1;
        } else {
            *scan = 1;
            scan++;
            *scan = atoi(scan + 1);
            *(scan + 1) = 0;
        }
        txt++;
    }
}

// Writes a string directly into the xlat pool at the given offset index.
// Used by XLAT_init to set default control name strings.
// uc_orig: XLAT_poke (fallen/Source/xlat_str.cpp)
static void XLAT_poke(SLONG offset, CBYTE* str)
{
    strcpy(xlat_upto, str);
    xlat_ptr[offset] = xlat_upto;
    xlat_upto += strlen(str) + 1;
}

// Loads a language file and populates the translation pointer table.
// uc_orig: XLAT_load (fallen/Source/xlat_str.cpp)
void XLAT_load(CBYTE* fn)
{
    MFFileHandle handle;
    CBYTE* txt;
    UWORD id = 0;
    UWORD emergency_bail_out_for_martins_machine = 2000;

    xlat_upto = xlat_set;
    memset(xlat_ptr, 0, sizeof(xlat_ptr));
    memset(xlat_set, 0, sizeof(xlat_set));

    if (!FileExists(fn)) {
        ASSERT(UC_FALSE);
        return;
    }

    handle = FileOpen(fn);

    do {
        txt = XLAT_load_string(handle, xlat_upto);
        if (*txt) {
            XLAT_compress_tokens(xlat_upto);
            xlat_ptr[id++] = xlat_upto;
            xlat_upto += strlen(xlat_upto) + 1;
        }
    } while (*txt && emergency_bail_out_for_martins_machine--);

    FileClose(handle);
}

// Fills in default key name strings for control remapping
// (e.g. XCTL_JUMP -> "space"). Must be called after XLAT_load.
// uc_orig: XLAT_init (fallen/Source/xlat_str.cpp)
void XLAT_init()
{
    ASSERT(xlat_upto);
    XLAT_poke(REMAP_OFFSET + XCTL_JUMP, "space");
    XLAT_poke(REMAP_OFFSET + XCTL_PUNCH, "Z");
    XLAT_poke(REMAP_OFFSET + XCTL_KICK, "X");
    XLAT_poke(REMAP_OFFSET + XCTL_ACTION, "C");
    XLAT_poke(REMAP_OFFSET + XCTL_LEFT, "left");
    XLAT_poke(REMAP_OFFSET + XCTL_RIGHT, "right");
    XLAT_poke(REMAP_OFFSET + XCTL_START, "start");
    XLAT_poke(REMAP_OFFSET + XCTL_SELECT, "select");
    XLAT_poke(REMAP_OFFSET + XCTL_SPACE, "space");
    XLAT_poke(REMAP_OFFSET + XCTL_ENTER, "enter");
    XLAT_poke(REMAP_OFFSET + XCTL_CAM_FIRST, "A");
    XLAT_poke(REMAP_OFFSET + XCTL_RUN, "V");
    XLAT_poke(REMAP_OFFSET + XCTL_CAM_HIGH, "shoulder");
    XLAT_poke(REMAP_OFFSET + XCTL_CAM_LOW, "shoulder");
    XLAT_poke(REMAP_OFFSET + XCTL_CAM_ESC, "esc");
    XLAT_poke(REMAP_OFFSET + XCTL_CAM_CONTINUE, "space");
}

// ---------------------------------------------------------------------------
// Language file resolution (config.ini [Game] language=, with a fallback scan)
// ---------------------------------------------------------------------------

// Language files live in this directory as lang_<name>.txt (e.g.
// text/lang_english.txt). The original game records the chosen one in config.ini.
#define LANG_DIR "text"
#define LANG_FILE_PREFIX "lang_"
#define LANG_FILE_SUFFIX ".txt"
// Preferred default when config.ini gives no usable language.
#define LANG_DEFAULT_FILENAME "lang_english.txt"
// config.ini location + key (the original game's, relative to the resource root).
#define LANG_INI_FILE "config.ini"
#define LANG_INI_SECTION "Game"
#define LANG_INI_KEY "language"

// Resolved language path (relative to resource root), set by
// XLAT_resolve_language_file(), read by XLAT_language_file().
static char xlat_language_path[_MAX_PATH] = { 0 };

// Lowercase copy of a string (ASCII).
static std::string to_lower(std::string s)
{
    for (char& c : s)
        c = (char)tolower((unsigned char)c);
    return s;
}

// True if name matches lang_*.txt (case-insensitive) with a non-empty middle.
static bool is_language_filename(const std::string& name)
{
    const std::string lower = to_lower(name);
    const std::string prefix = LANG_FILE_PREFIX;
    const std::string suffix = LANG_FILE_SUFFIX;
    return lower.size() > prefix.size() + suffix.size()
        && lower.compare(0, prefix.size(), prefix) == 0
        && lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const char* XLAT_language_file(void)
{
    return xlat_language_path;
}

bool XLAT_resolve_language_file(void)
{
    // 1. Honour config.ini [Game] language= — the original game's mechanism and
    //    what every "change language" guide for the retail game edits. The path
    //    is relative to the resource root and may use backslashes; FileExists and
    //    XLAT_load normalise those.
    char ini_path[_MAX_PATH];
    if (INI_get_string(LANG_INI_FILE, LANG_INI_SECTION, LANG_INI_KEY, ini_path, (int)sizeof(ini_path))
        && ini_path[0] != '\0'
        && FileExists((CBYTE*)ini_path)) {
        strncpy(xlat_language_path, ini_path, sizeof(xlat_language_path) - 1);
        xlat_language_path[sizeof(xlat_language_path) - 1] = '\0';
        return true;
    }

    // 2. Fallback: scan text/ for any lang_*.txt. Prefer English; otherwise the
    //    first by name so the choice is stable across runs.
    //    NOTE: the file layer (FileExists/FileOpen) has no directory-listing API,
    //    so we enumerate with std::filesystem directly. That means the scan is
    //    relative to the current working directory and does NOT prepend the file
    //    layer's base path (cBasePath). This is correct only because the base path
    //    is always empty (FileSetBasePath("") at startup, never changed), so cwd
    //    and base-path roots coincide. If a non-empty base path is ever
    //    introduced, prepend it to LANG_DIR here (or add a base-path-aware
    //    directory-list helper to the file layer and use it).
    std::vector<std::string> langs;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(LANG_DIR, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec))
            continue;
        std::string name = it->path().filename().string();
        if (is_language_filename(name))
            langs.push_back(name);
    }

    if (langs.empty())
        return false;

    std::sort(langs.begin(), langs.end());
    std::string chosen = langs.front();
    for (const std::string& n : langs) {
        if (to_lower(n) == LANG_DEFAULT_FILENAME) {
            chosen = n;
            break;
        }
    }

    snprintf(xlat_language_path, sizeof(xlat_language_path), "%s/%s", LANG_DIR, chosen.c_str());
    return true;
}
