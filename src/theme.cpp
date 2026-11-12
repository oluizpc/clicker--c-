#include "theme.hpp"

#include <windows.h>

#include <string>

static float g_scale = 1.0f;
static ImFont* g_mono = nullptr;

float dpi_scale() { return g_scale; }
ImFont* mono_font() { return g_mono; }

Palette palette(bool dark) {
    if (dark) {
        return Palette{
            rgb(0x14, 0x16, 0x1B), rgb(0x1C, 0x1F, 0x27), rgb(0x24, 0x28, 0x33),
            rgb(0x2E, 0x33, 0x40), rgb(0xE7, 0xE9, 0xF0), rgb(0x86, 0x8D, 0xA0),
            rgb(0xF5, 0xA5, 0x24), rgb(0x19, 0x14, 0x05), rgb(0xE4, 0x5A, 0x4A),
        };
    }
    return Palette{
        rgb(0xF2, 0xF3, 0xF6), rgb(0xFF, 0xFF, 0xFF), rgb(0xE9, 0xEC, 0xF1),
        rgb(0xD6, 0xDA, 0xE2), rgb(0x1A, 0x1D, 0x24), rgb(0x6A, 0x71, 0x80),
        rgb(0xD6, 0x84, 0x00), rgb(0xFF, 0xF7, 0xEA), rgb(0xC6, 0x3B, 0x2D),
    };
}

static ImVec4 mix(ImVec4 a, ImVec4 b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
                  a.w + (b.w - a.w) * t);
}

void install_theme(const Palette& p, float scale) {
    g_scale = scale;
    ImGuiStyle st;  // parte do padrão e reescala do zero (evita escalar duas vezes)
    ImGui::StyleColorsDark(&st);

    st.WindowPadding = ImVec2(16, 16);
    st.FramePadding = ImVec2(10, 5);
    st.ItemSpacing = ImVec2(8, 8);
    st.ItemInnerSpacing = ImVec2(8, 6);
    st.CellPadding = ImVec2(6, 5);
    st.WindowRounding = 0;
    st.ChildRounding = 10;
    st.FrameRounding = 7;
    st.PopupRounding = 8;
    st.GrabRounding = 5;
    st.ScrollbarRounding = 6;
    st.ScrollbarSize = 10;
    st.GrabMinSize = 12;
    st.WindowBorderSize = 0;
    st.ChildBorderSize = 1;
    st.PopupBorderSize = 1;
    st.FrameBorderSize = 1;
    st.SeparatorTextBorderSize = 1;

    ImVec4* c = st.Colors;
    const ImVec4 clear(0, 0, 0, 0);
    c[ImGuiCol_Text] = p.text;
    c[ImGuiCol_TextDisabled] = p.muted;
    c[ImGuiCol_WindowBg] = p.bg;
    c[ImGuiCol_ChildBg] = clear;
    c[ImGuiCol_PopupBg] = p.surface;
    c[ImGuiCol_Border] = p.stroke;
    c[ImGuiCol_BorderShadow] = clear;
    c[ImGuiCol_FrameBg] = p.surface2;
    c[ImGuiCol_FrameBgHovered] = mix(p.surface2, p.accent, 0.10f);
    c[ImGuiCol_FrameBgActive] = mix(p.surface2, p.accent, 0.18f);
    c[ImGuiCol_ScrollbarBg] = clear;
    c[ImGuiCol_ScrollbarGrab] = p.stroke;
    c[ImGuiCol_ScrollbarGrabHovered] = p.muted;
    c[ImGuiCol_ScrollbarGrabActive] = p.accent;
    c[ImGuiCol_CheckMark] = p.accent;
    c[ImGuiCol_SliderGrab] = p.accent;
    c[ImGuiCol_SliderGrabActive] = p.accent;
    c[ImGuiCol_Button] = p.surface2;
    c[ImGuiCol_ButtonHovered] = mix(p.surface2, p.accent, 0.18f);
    c[ImGuiCol_ButtonActive] = p.accent;
    c[ImGuiCol_Header] = with_alpha(p.accent, 0.35f);
    c[ImGuiCol_HeaderHovered] = mix(p.surface2, p.accent, 0.12f);
    c[ImGuiCol_HeaderActive] = with_alpha(p.accent, 0.55f);
    c[ImGuiCol_Separator] = p.stroke;
    c[ImGuiCol_SeparatorHovered] = p.stroke;
    c[ImGuiCol_SeparatorActive] = p.stroke;
    c[ImGuiCol_TextSelectedBg] = with_alpha(p.accent, 0.35f);
    c[ImGuiCol_NavCursor] = p.accent;
    c[ImGuiCol_ResizeGrip] = clear;
    c[ImGuiCol_ResizeGripHovered] = clear;
    c[ImGuiCol_ResizeGripActive] = clear;

    st.ScaleAllSizes(scale);
    st.FontSizeBase = 14.0f;
    st.FontScaleDpi = scale;
    ImGui::GetStyle() = st;
}

static bool file_exists(const char* path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

void load_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    char win[MAX_PATH];
    UINT n = GetWindowsDirectoryA(win, MAX_PATH);
    std::string fonts = (n > 0 ? std::string(win, n) : std::string("C:\\Windows")) + "\\Fonts\\";

    // Fonte principal: Segoe UI (tem todos os acentos do português).
    std::string ui = fonts + "segoeui.ttf";
    if (file_exists(ui.c_str())) {
        io.Fonts->AddFontFromFileTTF(ui.c_str(), 14.0f);
        // Símbolos que faltarem na Segoe UI vêm da Segoe UI Symbol.
        std::string sym = fonts + "seguisym.ttf";
        if (file_exists(sym.c_str())) {
            ImFontConfig cfg;
            cfg.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(sym.c_str(), 14.0f, &cfg);
        }
    } else {
        io.Fonts->AddFontDefault();
    }

    // Monoespaçada para os números do card de status.
    std::string mono = fonts + "consola.ttf";
    g_mono = file_exists(mono.c_str()) ? io.Fonts->AddFontFromFileTTF(mono.c_str(), 14.0f)
                                       : nullptr;
}
