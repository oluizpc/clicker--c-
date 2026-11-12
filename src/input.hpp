// Camada de entrada: envio de cliques, leitura de teclas e janelas abertas.
// Tudo isolado aqui para o resto do app não depender diretamente da WinAPI.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class MouseButton { Left, Right, Middle };

MouseButton mouse_button_from(uint8_t v);
const char* mouse_button_label(MouseButton b);

void send_click(MouseButton btn);

// true se a tecla do virtual-key code está pressionada agora.
bool key_down(int vk);

// Título (UTF-8) da janela atualmente em foco (para o filtro de janela-alvo).
std::string foreground_title();

// Títulos (UTF-8) das janelas visíveis abertas, ordenados e sem repetição.
std::vector<std::string> list_windows();

// Primeira tecla pressionada no momento (para capturar a hotkey).
// Ignora o botão esquerdo do mouse (usado para clicar no campo).
std::optional<int> first_key_down();

// Utilitários de texto UTF-8.
std::wstring to_wide(const std::string& s);
std::string to_utf8(const std::wstring& w);
std::string to_lower(const std::string& s);              // minúsculas com acentos
std::string ellipsize(const std::string& s, size_t max);  // corta e põe "…"
std::string take_chars(const std::string& s, size_t max); // corta sem "…"
