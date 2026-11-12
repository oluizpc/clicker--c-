#include "config.hpp"

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

using nlohmann::json;
namespace fs = std::filesystem;

// Mesmo formato de JSON da versão Rust: o autoclicker_config.json antigo continua valendo.
static void to_json(json& j, const Settings& s) {
    j = json{
        {"min_cps", s.min_cps},
        {"max_cps", s.max_cps},
        {"jitter", s.jitter},
        {"button", s.button},
        {"mode", s.mode},
        {"hotkey_vk", s.hotkey_vk},
        {"hotkey_name", s.hotkey_name},
        {"start_delay_ms", s.start_delay_ms},
        {"burst_clicks", s.burst_clicks},
        {"click_limit_enabled", s.click_limit_enabled},
        {"click_limit", s.click_limit},
        {"target_window_only", s.target_window_only},
        {"target_window_title", s.target_window_title},
        {"dark_mode", s.dark_mode},
        {"minimize_to_tray", s.minimize_to_tray},
    };
}

// Campos ausentes caem no valor padrão (mais tolerante que falhar o arquivo todo).
static Settings settings_from_json(const json& j) {
    Settings d;
    if (!j.is_object()) return d;
    Settings s;
    s.min_cps = j.value("min_cps", d.min_cps);
    s.max_cps = j.value("max_cps", d.max_cps);
    s.jitter = j.value("jitter", d.jitter);
    s.button = j.value("button", d.button);
    s.mode = j.value("mode", d.mode);
    s.hotkey_vk = j.value("hotkey_vk", d.hotkey_vk);
    s.hotkey_name = j.value("hotkey_name", d.hotkey_name);
    s.start_delay_ms = j.value("start_delay_ms", d.start_delay_ms);
    s.burst_clicks = j.value("burst_clicks", d.burst_clicks);
    s.click_limit_enabled = j.value("click_limit_enabled", d.click_limit_enabled);
    s.click_limit = j.value("click_limit", d.click_limit);
    s.target_window_only = j.value("target_window_only", d.target_window_only);
    s.target_window_title = j.value("target_window_title", d.target_window_title);
    s.dark_mode = j.value("dark_mode", d.dark_mode);
    s.minimize_to_tray = j.value("minimize_to_tray", d.minimize_to_tray);
    if (s.button > 2) s.button = 0;
    if (s.mode > 2) s.mode = 0;
    return s;
}

// Salva ao lado do executável; cai para o diretório atual se necessário.
static fs::path config_path() {
    wchar_t buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(std::size(buf)));
    fs::path dir = (n > 0 && n < std::size(buf)) ? fs::path(buf).parent_path() : fs::path(L".");
    return dir / L"autoclicker_config.json";
}

Config Config::load() {
    Config cfg;
    std::ifstream f(config_path(), std::ios::binary);
    if (!f) return cfg;
    std::stringstream ss;
    ss << f.rdbuf();
    try {
        json j = json::parse(ss.str());
        if (j.contains("active")) cfg.active = settings_from_json(j["active"]);
        if (j.contains("window") && j["window"].is_object()) {
            const json& w = j["window"];
            cfg.window.x = w.value("x", 0);
            cfg.window.y = w.value("y", 0);
            cfg.window.w = w.value("w", 0);
            cfg.window.h = w.value("h", 0);
            cfg.window.maximized = w.value("maximized", false);
        }
        if (j.contains("profiles") && j["profiles"].is_object()) {
            for (auto& [name, val] : j["profiles"].items()) {
                cfg.profiles[name] = settings_from_json(val);
            }
        }
    } catch (...) {
        return Config{};
    }
    return cfg;
}

void Config::save() const {
    json profs = json::object();
    for (const auto& [name, s] : profiles) profs[name] = s;
    json j = {{"active", active}, {"profiles", profs}};
    if (window.valid()) {
        j["window"] = {{"x", window.x}, {"y", window.y}, {"w", window.w}, {"h", window.h},
                       {"maximized", window.maximized}};
    }
    std::ofstream f(config_path(), std::ios::binary | std::ios::trunc);
    if (f) f << j.dump(2);
}
