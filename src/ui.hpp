// Pequenos componentes reaproveitados pela interface (equivalentes aos do egui).
#pragma once

#include <algorithm>
#include <string>

#include "imgui.h"
#include "theme.hpp"

namespace ui {

// Texto com tamanho e cor próprios.
void text(const std::string& s, float size, ImVec4 color, ImFont* font = nullptr);
void text_wrapped(const std::string& s, float size, ImVec4 color);

// Espaço vertical extra de `px` (além do espaçamento normal entre itens).
void space(float px);

// Largura natural de um botão com esse rótulo.
float button_width(const char* label);

// Botão de seleção estilo "pílula" (selectable_label do egui).
bool pill(const Palette& p, const char* label, bool selected);

// Botão preenchido com uma cor (texto em on_accent).
bool filled_button(const Palette& p, const char* label, ImVec4 fill, ImVec2 size = ImVec2(0, 0),
                   float font_size = 0.0f, float rounding = -1.0f);

// Logo "D" num quadrado âmbar + nome ao lado.
void logo_title(const Palette& p, float box, float rounding, float logo_font, const char* title,
                float title_size);

// Bolinha colorida + texto (pílula de status).
void dot_label(const char* label, ImVec4 color, float size);

// Coloca o próximo item na mesma linha se couber; senão quebra a linha.
void same_line_if_fits(float next_w);

// Desenha texto alinhado num ponto: ax/ay = 0 (início), 0.5 (centro), 1 (fim).
void draw_text(ImDrawList* dl, ImVec2 pos, float ax, float ay, const char* s, float size, ImU32 col);

// Moldura de cartão (superfície + borda fina). Use dentro de card(...).
bool begin_card(const char* id, ImVec4 fill, ImVec4 border, float width, float padding,
                float rounding);
void end_card(float min_h);

// Cartão de seção com título e altura mínima opcional.
template <class F>
void section(const Palette& p, const char* title, float width, float min_h, F&& body) {
    begin_card(title, p.surface, p.stroke, width, S(14), S(10));
    text(title, 15.0f, p.text);
    space(4);
    body();
    end_card(min_h);
    space(2);
}

}  // namespace ui
