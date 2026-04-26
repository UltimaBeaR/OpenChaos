// OpenChaos config system — reads/writes openchaos/config.json.
// See engine/io/oc_config.h for the public API and devlog for design notes.

#include "engine/io/oc_config.h"
#include "engine/io/env.h"  // INI_get_string (for config.ini migration)

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using json = nlohmann::json;
namespace fs = std::filesystem;

static json        g_config;
static std::string g_config_path;
static std::string g_str_return_buf; // backing store for OC_CONFIG_get_string

static std::string to_lower(const char* s)
{
    std::string r = s;
    for (char& c : r) c = (char)std::tolower((unsigned char)c);
    return r;
}

static void config_save()
{
    std::ofstream f(g_config_path);
    if (f)
        f << g_config.dump(4) << '\n';
    else
        fprintf(stderr, "oc_config: could not write %s\n", g_config_path.c_str());
}

// Build g_config from hardcoded defaults, then override with config.ini values.
// Called only on first run (no config.json on disk). Writes ALL known keys.
static void build_defaults_and_migrate(const char* ini_path)
{
    // Hardcoded defaults — all sections, all keys.
    g_config = {
        {"audio", {
            {"ambient_volume", 127},
            {"music_volume",   127},
            {"fx_volume",      127}
        }},
        {"keyboard", {
            {"keyboard_left",       203},
            {"keyboard_right",      205},
            {"keyboard_forward",    200},
            {"keyboard_back",       208},
            {"keyboard_punch",       44},
            {"keyboard_kick",        45},
            {"keyboard_action",      46},
            {"keyboard_run",         47},
            {"keyboard_jump",        57},
            {"keyboard_start",       15},
            {"keyboard_select",      28},
            {"keyboard_camera",     207},
            {"keyboard_cam_left",   211},
            {"keyboard_cam_right",  209},
            {"keyboard_1stperson",   30}
        }},
        {"render", {
            {"detail_shadows",           1},
            {"detail_puddles",           1},
            {"detail_dirt",              1},
            {"detail_mist",              1},
            {"detail_rain",              1},
            {"detail_skyline",           1},
            {"detail_crinkles",          1},
            {"detail_stars",             1},
            {"detail_moon_reflection",   1},
            {"detail_people_reflection", 1},
            {"detail_filter",            1},
            {"detail_perspective",       1},
            {"Adami_lighting",           1},
            {"video_truecolour",         1},
            {"draw_distance",           22},
            {"max_frame_rate",          30}
        }},
        {"game", {
            {"scanner_follows", 1},
            {"language",        "text\\lang_english.txt"}
        }},
        {"movie", {
            {"play_movie", 1}
        }},
        {"openchaos", {
            {"fullscreen",      0},
            {"windowed_width",  640},
            {"windowed_height", 480},
            {"vsync",           1},
            {"render_scale",    1.0},
            {"aa_enable",       1},
            {"crt_enable",      1},
            {"fov_multiplier",  1.0}
        }}
    };

    // Migrate integer value from config.ini (only if the key actually exists there).
    auto try_ini_int = [&](const char* sec_json, const char* key_json,
                           const char* sec_ini,  const char* key_ini) {
        char buf[64];
        if (INI_get_string(ini_path, sec_ini, key_ini, buf, sizeof(buf)) && buf[0])
            g_config[sec_json][key_json] = atoi(buf);
    };
    auto try_ini_str = [&](const char* sec_json, const char* key_json,
                           const char* sec_ini,  const char* key_ini) {
        char buf[256];
        if (INI_get_string(ini_path, sec_ini, key_ini, buf, sizeof(buf)) && buf[0])
            g_config[sec_json][key_json] = std::string(buf);
    };

    // [Audio]
    try_ini_int("audio", "ambient_volume", "Audio", "ambient_volume");
    try_ini_int("audio", "music_volume",   "Audio", "music_volume");
    try_ini_int("audio", "fx_volume",      "Audio", "fx_volume");

    // [Game]
    try_ini_str("game", "language",        "Game", "language");
    try_ini_int("game", "scanner_follows", "Game", "scanner_follows");

    // [Render]
    try_ini_int("render", "detail_shadows",          "Render", "detail_shadows");
    try_ini_int("render", "detail_puddles",           "Render", "detail_puddles");
    try_ini_int("render", "detail_dirt",              "Render", "detail_dirt");
    try_ini_int("render", "detail_mist",              "Render", "detail_mist");
    try_ini_int("render", "detail_rain",              "Render", "detail_rain");
    try_ini_int("render", "detail_skyline",           "Render", "detail_skyline");
    try_ini_int("render", "detail_crinkles",          "Render", "detail_crinkles");
    try_ini_int("render", "detail_stars",             "Render", "detail_stars");
    try_ini_int("render", "detail_moon_reflection",   "Render", "detail_moon_reflection");
    try_ini_int("render", "detail_people_reflection", "Render", "detail_people_reflection");
    try_ini_int("render", "detail_filter",            "Render", "detail_filter");
    try_ini_int("render", "detail_perspective",       "Render", "detail_perspective");
    try_ini_int("render", "Adami_lighting",           "Render", "Adami_lighting");
    try_ini_int("render", "video_truecolour",         "Render", "video_truecolour");
    try_ini_int("render", "draw_distance",            "Render", "draw_distance");
    try_ini_int("render", "max_frame_rate",           "Render", "max_frame_rate");

    // [Movie]
    try_ini_int("movie", "play_movie", "Movie", "play_movie");

    // [Keyboard] — DirectInput scan codes, compatible with our keyboard system
    try_ini_int("keyboard", "keyboard_left",       "Keyboard", "keyboard_left");
    try_ini_int("keyboard", "keyboard_right",      "Keyboard", "keyboard_right");
    try_ini_int("keyboard", "keyboard_forward",    "Keyboard", "keyboard_forward");
    try_ini_int("keyboard", "keyboard_back",       "Keyboard", "keyboard_back");
    try_ini_int("keyboard", "keyboard_punch",      "Keyboard", "keyboard_punch");
    try_ini_int("keyboard", "keyboard_kick",       "Keyboard", "keyboard_kick");
    try_ini_int("keyboard", "keyboard_action",     "Keyboard", "keyboard_action");
    try_ini_int("keyboard", "keyboard_run",        "Keyboard", "keyboard_run");
    try_ini_int("keyboard", "keyboard_jump",       "Keyboard", "keyboard_jump");
    try_ini_int("keyboard", "keyboard_start",      "Keyboard", "keyboard_start");
    try_ini_int("keyboard", "keyboard_select",     "Keyboard", "keyboard_select");
    try_ini_int("keyboard", "keyboard_camera",     "Keyboard", "keyboard_camera");
    try_ini_int("keyboard", "keyboard_cam_left",   "Keyboard", "keyboard_cam_left");
    try_ini_int("keyboard", "keyboard_cam_right",  "Keyboard", "keyboard_cam_right");
    try_ini_int("keyboard", "keyboard_1stperson",  "Keyboard", "keyboard_1stperson");

    // [Joypad] intentionally NOT migrated — old DirectInput indices are
    // incompatible with our SDL3 button mapping. Gamepad bindings are hardcoded.

    // [openchaos] has no config.ini counterpart — hardcoded defaults only.
}

void OC_CONFIG_load(const char* ini_path)
{
    fs::path dir  = fs::path("openchaos");
    fs::path path = dir / "config.json";
    g_config_path = path.string();

    if (fs::exists(path)) {
        std::ifstream f(path);
        if (f) {
            try {
                g_config = json::parse(f);
                return; // loaded successfully — missing keys use hardcoded defaults in get_*
            } catch (const std::exception& e) {
                fprintf(stderr, "oc_config: corrupt config.json (%s), rebuilding\n", e.what());
            }
        }
    }

    // First run or corrupt file: build from defaults + migrate from config.ini.
    build_defaults_and_migrate(ini_path);

    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec)
        fprintf(stderr, "oc_config: could not create openchaos/ directory: %s\n",
                ec.message().c_str());

    config_save();
}

int OC_CONFIG_get_int(const char* section, const char* key, int def)
{
    std::string sec = to_lower(section);
    auto it = g_config.find(sec);
    if (it != g_config.end()) {
        auto jt = it->find(key);
        if (jt != it->end() && jt->is_number_integer())
            return jt->get<int>();
        if (jt != it->end() && jt->is_number())
            return (int)jt->get<double>();
    }
    return def;
}

float OC_CONFIG_get_float(const char* section, const char* key, float def)
{
    std::string sec = to_lower(section);
    auto it = g_config.find(sec);
    if (it != g_config.end()) {
        auto jt = it->find(key);
        if (jt != it->end() && jt->is_number())
            return (float)jt->get<double>();
    }
    return def;
}

const char* OC_CONFIG_get_string(const char* section, const char* key, const char* def)
{
    std::string sec = to_lower(section);
    auto it = g_config.find(sec);
    if (it != g_config.end()) {
        auto jt = it->find(key);
        if (jt != it->end() && jt->is_string()) {
            g_str_return_buf = jt->get<std::string>();
            return g_str_return_buf.c_str();
        }
    }
    return def;
}

void OC_CONFIG_set_int(const char* section, const char* key, int value)
{
    g_config[to_lower(section)][key] = value;
    config_save();
}

void OC_CONFIG_set_string(const char* section, const char* key, const char* value)
{
    g_config[to_lower(section)][key] = std::string(value);
    config_save();
}
