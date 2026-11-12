// Configuração persistida: settings atuais + perfis nomeados, salvos em JSON.
#pragma once

#include <cstdint>
#include <map>
#include <string>

struct Settings {
    float min_cps = 12.0f;
    float max_cps = 16.0f;
    bool jitter = true;
    // 0 = esquerdo, 1 = direito, 2 = meio
    uint8_t button = 0;
    // 0 = toggle (liga/desliga), 1 = hold (segurar hotkey), 2 = segurar botão esquerdo do mouse
    uint8_t mode = 0;
    int hotkey_vk = 0x75;  // F6
    std::string hotkey_name = "F6";
    uint64_t start_delay_ms = 0;
    uint32_t burst_clicks = 25;
    bool click_limit_enabled = false;
    uint64_t click_limit = 1000;
    bool target_window_only = false;
    std::string target_window_title = "Roblox";
    bool dark_mode = true;
    // Ao minimizar, some da barra de tarefas e fica como ícone perto do relógio.
    bool minimize_to_tray = true;
    // Jitter de posição: a cada clique o cursor vai para um ponto aleatório a até
    // N pixels do lugar original (e volta ao parar).
    bool pos_jitter = false;
    int pos_jitter_px = 3;

    bool operator==(const Settings&) const = default;
};

// Tamanho/posição da janela (pixels de tela, "posição restaurada" do Windows).
// Fica fora de Settings para carregar um perfil não mexer na janela.
struct WindowState {
    int x = 0, y = 0, w = 0, h = 0;
    bool maximized = false;
    bool valid() const { return w > 0 && h > 0; }
};

struct Config {
    Settings active;
    WindowState window;
    // std::map mantém os nomes em ordem alfabética (como o BTreeMap da versão Rust).
    std::map<std::string, Settings> profiles;

    static Config load();
    void save() const;
};
