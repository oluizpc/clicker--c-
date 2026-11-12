// Interface gráfica (Dear ImGui): sidebar de navegação + páginas.
// Ao abrir, uma tela inicial pede para escolher a janela onde o clicker atua.
#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "config.hpp"
#include "theme.hpp"
#include "worker.hpp"

class App {
public:
    App(Shared& shared, Config config);

    // Chamar antes de ImGui::NewFrame(): aplica troca de tema/DPI pendente.
    void before_frame(float dpi_scale);
    // Desenha a interface inteira (entre NewFrame e Render).
    void frame();
    // Ao fechar: para as threads e salva.
    void on_exit();
    // Tamanho/posição da janela, gravado junto no próximo salvamento.
    void set_window_state(const WindowState& w) { config_.window = w; }

    // Quanto tempo o loop pode esperar por eventos antes de redesenhar (ms).
    unsigned idle_timeout_ms() const;

private:
    enum class Screen { Picker, Main };
    enum class Page { Clicker, Profiles, Appearance };

    void persist();
    void confirm_window(bool scoped);
    void open_picker();

    void picker_screen();
    void main_screen();
    void sidebar(const Settings& s, bool active, bool& go_picker);
    void clicker_page(Settings& s, bool active, float cps_now, uint64_t clicks, float width,
                      bool& toggle_req);
    void profiles_page(Settings& s, float width);
    void appearance_page(Settings& s, float width, bool& theme_changed);

    Shared& shared_;
    Config config_;
    Palette pal_;
    float scale_ = 0.0f;
    bool theme_dirty_ = true;

    std::string new_profile_name_;
    std::string status_;
    bool capturing_ = false;
    Page page_ = Page::Clicker;

    // Tela inicial de seleção de janela.
    Screen screen_ = Screen::Picker;
    std::vector<std::string> win_list_;
    std::string win_filter_;
    std::optional<std::string> picked_;
    bool mc_only_ = true;

    // Medição de CPS real (cliques por segundo observados).
    float meas_cps_ = 0.0f;
    std::chrono::steady_clock::time_point meas_t_;
    uint64_t meas_clicks_ = 0;
};
