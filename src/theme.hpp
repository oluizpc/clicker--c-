// Identidade visual: painel em ardósia com um único acento âmbar.
// Tudo que é cor/espaçamento/raio vive aqui, para a UI ficar coesa.
#pragma once

#include "imgui.h"

struct Palette {
    ImVec4 bg, surface, surface2, stroke, text, muted, accent, on_accent, danger;
};

Palette palette(bool dark);

// Aplica cores, espaçamentos e escala de DPI ao estilo do ImGui.
void install_theme(const Palette& p, float dpi_scale);

// Carrega as fontes do Windows (Segoe UI + Consolas). Chamar uma vez.
void load_fonts();
ImFont* mono_font();

// Escala de DPI atual: use S(px) em toda medida fixa de layout.
float dpi_scale();
inline float S(float px) { return px * dpi_scale(); }

constexpr ImVec4 rgb(int r, int g, int b) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}
inline ImVec4 with_alpha(ImVec4 c, float a) { return ImVec4(c.x, c.y, c.z, c.w * a); }
inline ImU32 u32(ImVec4 c) { return ImGui::ColorConvertFloat4ToU32(c); }
