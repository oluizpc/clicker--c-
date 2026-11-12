// Teste de precisão do CPS: roda as threads reais (worker.cpp) com uma camada de
// entrada falsa que só conta os cliques. Não clica nada de verdade.
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <chrono>
#include <cstdio>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "input.hpp"
#include "worker.hpp"

static std::atomic<uint64_t> g_clicks{0};

// --- Implementação falsa de input.hpp ---
MouseButton mouse_button_from(uint8_t) { return MouseButton::Left; }
const char* mouse_button_label(MouseButton) { return ""; }
void send_click(MouseButton) { g_clicks.fetch_add(1); }
// Posição falsa do cursor (relativa à origem) e maior afastamento visto.
static std::atomic<int> g_x{0}, g_y{0}, g_max_off{0}, g_moves{0};
void move_cursor(int dx, int dy) {
    if (dx == 0 && dy == 0) return;  // igual à versão real
    int x = g_x += dx, y = g_y += dy;
    int off = std::max(std::abs(x), std::abs(y));
    if (off > g_max_off) g_max_off = off;
    g_moves.fetch_add(1);
}
bool key_down(int) { return false; }
std::string foreground_title() { return "Minecraft 1.8.9"; }
std::vector<std::string> list_windows() { return {}; }
std::optional<int> first_key_down() { return std::nullopt; }
std::wstring to_wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }
std::string to_utf8(const std::wstring& w) { return std::string(w.begin(), w.end()); }
std::string to_lower(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = char(tolower(static_cast<unsigned char>(c)));
    return r;
}
std::string ellipsize(const std::string& s, size_t) { return s; }
std::string take_chars(const std::string& s, size_t) { return s; }

static int failures = 0;
static void check(bool ok, const char* what) {
    std::printf("  [%s] %s\n", ok ? "ok" : "FALHOU", what);
    if (!ok) ++failures;
}

int main() {
    Settings s;
    s.jitter = false;
    s.mode = 0;
    Shared sh(s);
    Workers w;
    w.start(sh);

    // 1) CPS real vs configurado.
    for (float cps : {10.0f, 20.0f, 50.0f, 100.0f}) {
        s.min_cps = s.max_cps = cps;
        sh.set(s);
        g_clicks = 0;
        sh.set_clicking(true);
        auto t0 = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        sh.set_clicking(false);
        double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        // O 1º clique sai no instante zero; os intervalos são (cliques - 1).
        double real = double(g_clicks.load() - 1) / dt;
        std::printf("CPS %5.1f -> real %6.2f (%+.1f%%)\n", cps, real, (real - cps) / cps * 100.0);
        check(std::abs(real - cps) / cps < 0.03, "dentro de 3%");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 2) Limite de cliques.
    s.min_cps = s.max_cps = 100.0f;
    s.click_limit_enabled = true;
    s.click_limit = 37;
    sh.set(s);
    g_clicks = 0;
    sh.set_clicking(true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::printf("Limite 37 -> %llu cliques, ativo=%d\n", (unsigned long long)g_clicks.load(),
                int(sh.clicking.load()));
    check(g_clicks.load() == 37 && !sh.clicking.load(), "para exatamente no limite");
    s.click_limit_enabled = false;

    // 3) Filtro de janela-alvo (janela em foco falsa = "Minecraft 1.8.9").
    s.target_window_only = true;
    s.target_window_title = "Roblox";
    sh.set(s);
    g_clicks = 0;
    sh.set_clicking(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    check(g_clicks.load() == 0, "janela errada: nenhum clique");
    s.target_window_title = "MINECRAFT";
    sh.set(s);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sh.set_clicking(false);
    check(g_clicks.load() > 30, "janela certa (sem diferenciar maiusculas): clica");
    s.target_window_only = false;

    // 4) Atraso ao iniciar.
    s.start_delay_ms = 400;
    sh.set(s);
    g_clicks = 0;
    sh.set_clicking(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    uint64_t during = g_clicks.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    sh.set_clicking(false);
    check(during == 0 && g_clicks.load() > 10, "espera o atraso antes de clicar");
    s.start_delay_ms = 0;

    // 5) Rajada.
    s.burst_clicks = 25;
    s.max_cps = 100.0f;
    sh.set(s);
    g_clicks = 0;
    sh.burst_request = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    check(g_clicks.load() == 25, "rajada de 25 cliques");

    // 6) Jitter: média fica entre mínimo e máximo.
    s.jitter = true;
    s.min_cps = 10.0f;
    s.max_cps = 20.0f;
    sh.set(s);
    g_clicks = 0;
    sh.set_clicking(true);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    sh.set_clicking(false);
    double avg = double(g_clicks.load()) / 3.0;
    std::printf("Jitter 10-20 -> media %.2f\n", avg);
    check(avg > 12.5 && avg < 17.5, "media perto de 15");
    check(g_moves.load() == 0, "sem jitter de posicao: cursor parado");

    // 7) Jitter de posição: fica dentro do raio e volta à origem ao parar.
    s.jitter = false;
    s.min_cps = s.max_cps = 100.0f;
    s.pos_jitter = true;
    s.pos_jitter_px = 4;
    sh.set(s);
    sh.set_clicking(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    int moves_during = g_moves.load();
    sh.set_clicking(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::printf("Jitter posicao 4px -> %d movimentos, afastamento max %d, final (%d,%d)\n",
                moves_during, g_max_off.load(), g_x.load(), g_y.load());
    check(moves_during > 20, "move o cursor a cada clique");
    check(g_max_off.load() <= 4, "nunca passa do raio");
    check(g_max_off.load() >= 3, "usa o raio todo");
    check(g_x.load() == 0 && g_y.load() == 0, "volta para a origem ao parar");

    sh.running = false;
    w.join();
    std::printf(failures ? "\n%d FALHA(S)\n" : "\nTudo certo.\n", failures);
    return failures ? 1 : 0;
}
