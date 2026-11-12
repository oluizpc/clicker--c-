#include "worker.hpp"

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <random>

#include "input.hpp"

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

using Clock = std::chrono::steady_clock;
using std::chrono::microseconds;

// Sleep fino: usa o timer de alta resolução do Windows 10+ (precisão de ~0,5 ms).
// O Sleep() comum do Windows arredonda para 1 a 15 ms, o que estragaria CPS altos.
static void sleep_us(int64_t us) {
    thread_local HANDLE timer = [] {
        HANDLE t = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                          TIMER_ALL_ACCESS);
        if (!t) t = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
        return t;
    }();
    if (!timer) {
        Sleep(static_cast<DWORD>(std::max<int64_t>(1, us / 1000)));
        return;
    }
    LARGE_INTEGER due;
    due.QuadPart = -us * 10;  // unidades de 100 ns, negativo = relativo
    SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
    WaitForSingleObject(timer, INFINITE);
}

// Espera de alta precisão: dorme a maior parte e faz spin nos últimos ~1,5 ms
// para o CPS real bater com o configurado, mesmo em taxas altas. Acorda cedo
// se o clique for desligado (resposta instantânea).
static void sleep_interruptible(Shared& sh, Clock::duration dur) {
    const auto end = Clock::now() + dur;
    for (;;) {
        if (!sh.clicking.load() || !sh.running.load()) break;
        auto now = Clock::now();
        if (now >= end) break;
        if (end - now > microseconds(1500)) {
            sleep_us(500);  // ainda falta bastante: dorme sem gastar CPU
        } else {
            YieldProcessor();  // reta final: spin para precisão sub-milissegundo
        }
    }
}

static Clock::duration secs(float s) {
    return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(s));
}

// Thread que lê a hotkey e liga/desliga o clique conforme o modo.
static void hotkey_loop(Shared& sh) {
    bool prev_down = false;
    while (sh.running.load()) {
        auto [vk, mode] = sh.hotkey_and_mode();
        if (mode == 1 || mode == 2) {
            // Hold: ativo enquanto a hotkey (ou o botão esquerdo, VK_LBUTTON) estiver pressionada.
            bool down = key_down(mode == 2 ? VK_LBUTTON : vk);
            if (down != sh.clicking.load()) sh.set_clicking(down);
            prev_down = down;
        } else {
            // Toggle: inverte na borda de subida (tecla recém-pressionada).
            bool down = key_down(vk);
            if (down && !prev_down) sh.set_clicking(!sh.clicking.load());
            prev_down = down;
        }
        // Polling curto para o modo "segurar mouse" responder rápido.
        sleep_us(2000);
    }
}

// Jitter de posição: guarda o deslocamento atual em relação ao ponto de origem
// (onde o cursor estava) para nunca "andar" além do raio e voltar ao parar.
struct PosJitter {
    int ox = 0, oy = 0;

    // Leva o cursor para um ponto aleatório dentro do raio (quadrado de lado 2r+1).
    void jump(int r, std::mt19937& rng) {
        std::uniform_int_distribution<int> d(-r, r);
        int tx = d(rng), ty = d(rng);
        move_cursor(tx - ox, ty - oy);
        ox = tx;
        oy = ty;
    }
    // Volta para a origem.
    void reset() {
        move_cursor(-ox, -oy);
        ox = oy = 0;
    }
};

// Thread que efetivamente envia os cliques.
static void clicker_loop(Shared& sh) {
    std::mt19937 rng{std::random_device{}()};
    PosJitter pj;

    while (sh.running.load()) {
        // Rajada avulsa (botão "Disparar"), independente do toggle.
        if (sh.burst_request.exchange(false)) {
            Settings s = sh.get();
            MouseButton btn = mouse_button_from(s.button);
            auto gap_us = static_cast<int64_t>(1'000'000.0f / std::max(s.max_cps, 1.0f));
            for (uint32_t i = 0; i < s.burst_clicks && sh.running.load(); ++i) {
                send_click(btn);
                sleep_us(gap_us);
            }
        }

        if (!sh.clicking.load()) {
            pj.reset();
            sleep_us(5000);
            continue;
        }

        // Início de uma ativação: aplica delay e zera o contador.
        if (sh.reset.exchange(false)) {
            sh.clicks_done.store(0);
            uint64_t delay = sh.get().start_delay_ms;
            if (delay > 0) sleep_interruptible(sh, std::chrono::milliseconds(delay));
        }

        Settings s = sh.get();

        // Filtro de janela-alvo.
        if (s.target_window_only && s.target_window_title.find_first_not_of(" \t") != std::string::npos) {
            std::string title = to_lower(foreground_title());
            if (title.find(to_lower(s.target_window_title)) == std::string::npos) {
                pj.reset();
                sleep_us(20000);
                continue;
            }
        }

        // Limite de cliques.
        if (s.click_limit_enabled && sh.clicks_done.load() >= s.click_limit) {
            sh.set_clicking(false);
            continue;
        }

        // CPS alvo (com jitter opcional).
        float cps = s.min_cps;
        if (s.jitter && s.max_cps > s.min_cps) {
            cps = std::uniform_real_distribution<float>(s.min_cps, s.max_cps)(rng);
        }
        cps = std::max(cps, 0.5f);

        if (s.pos_jitter) {
            pj.jump(std::clamp(s.pos_jitter_px, 1, 20), rng);
        } else {
            pj.reset();
        }
        send_click(mouse_button_from(s.button));
        sh.clicks_done.fetch_add(1);
        sh.cps_now.store(cps);

        sleep_interruptible(sh, secs(1.0f / cps));
    }
    pj.reset();
}

void Workers::start(Shared& shared) {
    hotkey_ = std::thread(hotkey_loop, std::ref(shared));
    clicker_ = std::thread(clicker_loop, std::ref(shared));
}

void Workers::join() {
    if (hotkey_.joinable()) hotkey_.join();
    if (clicker_.joinable()) clicker_.join();
}
