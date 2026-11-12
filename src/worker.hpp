// Estado compartilhado e as threads de fundo: hotkey e clique.
#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <utility>

#include "config.hpp"

class Shared {
public:
    explicit Shared(Settings s) : settings_(std::move(s)) {}

    Settings get() const {
        std::lock_guard<std::mutex> lk(m_);
        return settings_;
    }
    void set(const Settings& s) {
        std::lock_guard<std::mutex> lk(m_);
        settings_ = s;
    }
    // Só o necessário para a thread de hotkey (evita copiar strings a cada 2 ms).
    std::pair<int, uint8_t> hotkey_and_mode() const {
        std::lock_guard<std::mutex> lk(m_);
        return {settings_.hotkey_vk, settings_.mode};
    }

    // Liga/desliga pelo botão da interface (usado só no modo toggle).
    void ui_toggle() { set_clicking(!clicking.load()); }
    void set_clicking(bool on) {
        bool was = clicking.exchange(on);
        if (on && !was) reset.store(true);
    }

    // true enquanto o clique automático está ativo.
    std::atomic<bool> clicking{false};
    // mantém as threads vivas; vira false ao fechar.
    std::atomic<bool> running{true};
    // pedido de rajada (burst) de uma vez só.
    std::atomic<bool> burst_request{false};
    // cliques já enviados na ativação atual.
    std::atomic<uint64_t> clicks_done{0};
    // CPS alvo calculado no último clique (para exibir).
    std::atomic<float> cps_now{0.0f};
    // sinaliza que a ativação acabou de começar (resetar contador/delay).
    std::atomic<bool> reset{false};

private:
    mutable std::mutex m_;
    Settings settings_;
};

// Sobe as threads de hotkey e de clique; join() espera elas terminarem
// depois que `running` vira false.
class Workers {
public:
    void start(Shared& shared);
    void join();

private:
    std::thread hotkey_;
    std::thread clicker_;
};
