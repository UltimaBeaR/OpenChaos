// OpenChaos config system — reads/writes openchaos/config.json.
// See engine/io/oc_config.h for the public API and devlog for design notes.

#include "engine/io/oc_config.h"
#include "engine/io/env.h" // INI_get_string (for config.ini migration)

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <cctype>
#include <cstdio>
#include <cstdlib>

using json = nlohmann::json;
namespace fs = std::filesystem;

static json g_config;
static std::string g_config_path;
static std::string to_lower(const char* s)
{
    std::string r = s;
    for (char& c : r)
        c = (char)std::tolower((unsigned char)c);
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
        { "audio", { { "ambient_volume", 127 }, { "music_volume", 127 }, { "fx_volume", 127 } } },
        { "video", { { "detail_shadows", true }, { "detail_puddles", true }, { "detail_dirt", true }, { "detail_mist", true }, { "detail_rain", true }, { "detail_skyline", true }, { "detail_crinkles", true }, { "detail_stars", true }, { "detail_moon_reflection", true }, { "detail_people_reflection", true }, { "detail_filter", true }, { "detail_perspective", true }, { "fullscreen", true }, { "windowed_maximized", false }, { "windowed_width", 640 }, { "windowed_height", 480 }, { "vsync", true }, { "render_scale", 1.0 }, { "antialiasing", true }, { "crt_effect", true } } },
        { "game", { { "scanner_follows", true } } },
        { "movie", { { "play_movie", true } } },
        // Stick deadzones as a fraction 0..1 of full deflection (center→edge).
        // gameplay = in-game movement/aim deadzone (raw 8192 = 0.25, unchanged).
        // menu = menu-navigation virtual-direction threshold (was raw 4096;
        // raised to 0.25 so controller drift doesn't auto-scroll menus).
        { "gamepad", { { "gameplay_stick_deadzone", 0.25 }, { "menu_stick_deadzone", 0.25 } } }
    };

    // Migrate integer value from config.ini (only if the key actually exists there).
    auto try_ini_int = [&](const char* sec_json, const char* key_json,
                           const char* sec_ini, const char* key_ini) {
        char buf[64];
        if (INI_get_string(ini_path, sec_ini, key_ini, buf, sizeof(buf)) && buf[0])
            g_config[sec_json][key_json] = atoi(buf);
    };
    // Same but stores as JSON boolean (true/false) instead of integer.
    auto try_ini_bool = [&](const char* sec_json, const char* key_json,
                            const char* sec_ini, const char* key_ini) {
        char buf[64];
        if (INI_get_string(ini_path, sec_ini, key_ini, buf, sizeof(buf)) && buf[0])
            g_config[sec_json][key_json] = (atoi(buf) != 0);
    };
    auto try_ini_str = [&](const char* sec_json, const char* key_json,
                           const char* sec_ini, const char* key_ini) {
        char buf[256];
        if (INI_get_string(ini_path, sec_ini, key_ini, buf, sizeof(buf)) && buf[0])
            g_config[sec_json][key_json] = std::string(buf);
    };

    // [Audio]
    try_ini_int("audio", "ambient_volume", "Audio", "ambient_volume");
    try_ini_int("audio", "music_volume", "Audio", "music_volume");
    try_ini_int("audio", "fx_volume", "Audio", "fx_volume");

    // [Game]
    try_ini_bool("game", "scanner_follows", "Game", "scanner_follows");

    // [Render] (config.ini section; stored under "video" in config.json)
    try_ini_bool("video", "detail_shadows", "Render", "detail_shadows");
    try_ini_bool("video", "detail_puddles", "Render", "detail_puddles");
    try_ini_bool("video", "detail_dirt", "Render", "detail_dirt");
    try_ini_bool("video", "detail_mist", "Render", "detail_mist");
    try_ini_bool("video", "detail_rain", "Render", "detail_rain");
    try_ini_bool("video", "detail_skyline", "Render", "detail_skyline");
    try_ini_bool("video", "detail_crinkles", "Render", "detail_crinkles");
    try_ini_bool("video", "detail_stars", "Render", "detail_stars");
    try_ini_bool("video", "detail_moon_reflection", "Render", "detail_moon_reflection");
    try_ini_bool("video", "detail_people_reflection", "Render", "detail_people_reflection");
    try_ini_bool("video", "detail_filter", "Render", "detail_filter");
    try_ini_bool("video", "detail_perspective", "Render", "detail_perspective");

    // [Movie]
    try_ini_bool("movie", "play_movie", "Movie", "play_movie");

    // [Keyboard] intentionally NOT migrated — custom key rebinding was removed;
    // keyboard bindings are hardcoded in the action map (act_*.h). [Joypad] is
    // likewise not migrated (old DirectInput indices don't map to SDL3).
}

void OC_CONFIG_load(const char* ini_path)
{
    fs::path dir = fs::path("open_chaos");
    fs::path path = dir / "config.json";
    g_config_path = path.string();

    if (fs::exists(path)) {
        std::ifstream f(path);
        if (f) {
            try {
                g_config = json::parse(f);
                // Normalise known bool fields: old files stored them as integer 0/1.
                static const struct {
                    const char* sec;
                    const char* key;
                } bool_fields[] = {
                    { "video", "detail_shadows" },
                    { "video", "detail_puddles" },
                    { "video", "detail_dirt" },
                    { "video", "detail_mist" },
                    { "video", "detail_rain" },
                    { "video", "detail_skyline" },
                    { "video", "detail_crinkles" },
                    { "video", "detail_stars" },
                    { "video", "detail_moon_reflection" },
                    { "video", "detail_people_reflection" },
                    { "video", "detail_filter" },
                    { "video", "detail_perspective" },
                    { "video", "fullscreen" },
                    { "video", "windowed_maximized" },
                    { "video", "vsync" },
                    { "video", "antialiasing" },
                    { "video", "crt_effect" },
                    { "game", "scanner_follows" },
                    { "movie", "play_movie" },
                };
                bool upgraded = false;
                for (auto& bf : bool_fields) {
                    auto sit = g_config.find(bf.sec);
                    if (sit == g_config.end())
                        continue;
                    auto kit = sit->find(bf.key);
                    if (kit == sit->end() || !kit->is_number_integer())
                        continue;
                    *kit = (kit->get<int>() != 0);
                    upgraded = true;
                }
                if (upgraded)
                    config_save();
                return;
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
    if (it == g_config.end())
        return def;
    auto jt = it->find(key);
    if (jt == it->end())
        return def;
    if (jt->is_boolean())
        return jt->get<bool>() ? 1 : 0;
    if (jt->is_number_integer())
        return jt->get<int>();
    if (jt->is_number())
        return (int)jt->get<double>();
    // Bad value — reset to hardcoded default and save.
    fprintf(stderr, "oc_config: bad value for %s.%s, resetting to %d\n", sec.c_str(), key, def);
    *jt = def;
    config_save();
    return def;
}

float OC_CONFIG_get_float(const char* section, const char* key, float def)
{
    std::string sec = to_lower(section);
    auto it = g_config.find(sec);
    if (it == g_config.end())
        return def;
    auto jt = it->find(key);
    if (jt == it->end())
        return def;
    if (jt->is_number())
        return (float)jt->get<double>();
    fprintf(stderr, "oc_config: bad value for %s.%s, resetting to %g\n", sec.c_str(), key, (double)def);
    *jt = def;
    config_save();
    return def;
}

void OC_CONFIG_set_int(const char* section, const char* key, int value)
{
    std::string sec = to_lower(section);
    // Preserve JSON type: if the key is already stored as bool, keep it bool.
    auto it = g_config.find(sec);
    if (it != g_config.end()) {
        auto jt = it->find(key);
        if (jt != it->end() && jt->is_boolean()) {
            g_config[sec][key] = (value != 0);
            config_save();
            return;
        }
    }
    g_config[sec][key] = value;
    config_save();
}
