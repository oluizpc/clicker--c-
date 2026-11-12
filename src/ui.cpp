#include "ui.hpp"

#include <cfloat>
#include <cmath>

namespace ui {

void text(const std::string& s, float size, ImVec4 color, ImFont* font) {
    ImGui::PushFont(font, size);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(s.c_str(), s.c_str() + s.size());
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void text_wrapped(const std::string& s, float size, ImVec4 color) {
    ImGui::PushTextWrapPos(0.0f);
    text(s, size, color);
    ImGui::PopTextWrapPos();
}

void space(float px) {
    float h = S(px) - ImGui::GetStyle().ItemSpacing.y;
    if (h > 0) ImGui::Dummy(ImVec2(0, h));
}

float button_width(const char* label) {
    return ImGui::CalcTextSize(label, nullptr, true).x + ImGui::GetStyle().FramePadding.x * 2;
}

bool pill(const Palette& p, const char* label, bool selected) {
    ImVec4 bg = selected ? with_alpha(p.accent, 0.30f) : ImVec4(0, 0, 0, 0);
    ImVec4 hover = selected ? with_alpha(p.accent, 0.38f) : p.surface2;
    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, with_alpha(p.accent, 0.50f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    bool r = ImGui::Button(label);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    return r;
}

bool filled_button(const Palette& p, const char* label, ImVec4 fill, ImVec2 size, float font_size,
                   float rounding) {
    ImVec4 hover(std::min(fill.x * 1.08f, 1.0f), std::min(fill.y * 1.08f, 1.0f),
                 std::min(fill.z * 1.08f, 1.0f), fill.w);
    ImGui::PushStyleColor(ImGuiCol_Button, fill);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, fill);
    ImGui::PushStyleColor(ImGuiCol_Text, p.on_accent);
    ImGui::PushStyleColor(ImGuiCol_Border, fill);
    if (rounding >= 0) ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    if (font_size > 0) ImGui::PushFont(nullptr, font_size);
    bool r = ImGui::Button(label, size);
    if (font_size > 0) ImGui::PopFont();
    if (rounding >= 0) ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    return r;
}

void draw_text(ImDrawList* dl, ImVec2 pos, float ax, float ay, const char* s, float size, ImU32 col) {
    ImFont* font = ImGui::GetFont();
    ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, s);
    dl->AddText(font, size, ImVec2(std::floor(pos.x - ts.x * ax + 0.5f), std::floor(pos.y - ts.y * ay + 0.5f)), col, s);
}

void logo_title(const Palette& p, float box, float rounding, float logo_font, const char* title,
                float title_size) {
    ImVec2 o = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(o, ImVec2(o.x + box, o.y + box), u32(p.accent), rounding);
    draw_text(dl, ImVec2(o.x + box * 0.5f, o.y + box * 0.5f), 0.5f, 0.5f, "D", S(logo_font),
              u32(p.on_accent));
    draw_text(dl, ImVec2(o.x + box + S(10), o.y + box * 0.5f), 0.0f, 0.5f, title, S(title_size),
              u32(p.text));
    ImGui::Dummy(ImVec2(box, box));
}

void dot_label(const char* label, ImVec4 color, float size) {
    ImVec2 o = ImGui::GetCursorScreenPos();
    float h = S(size) * 1.35f;
    float r = S(size) * 0.30f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(ImVec2(o.x + r + S(1), o.y + h * 0.5f), r, u32(color));
    draw_text(dl, ImVec2(o.x + r * 2 + S(7), o.y + h * 0.5f), 0.0f, 0.5f, label, S(size), u32(color));
    ImGui::Dummy(ImVec2(S(80), h));
}

void same_line_if_fits(float next_w) {
    float sp = ImGui::GetStyle().ItemSpacing.x;
    float right = ImGui::GetItemRectMax().x + sp + next_w;
    float limit = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    if (right <= limit) ImGui::SameLine();
}

static float g_card_padding = 0;

bool begin_card(const char* id, ImVec4 fill, ImVec4 border, float width, float padding,
                float rounding) {
    g_card_padding = padding;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, fill);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    bool r = ImGui::BeginChild(id, ImVec2(width, 0),
                               ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY |
                                   ImGuiChildFlags_AlwaysUseWindowPadding,
                               ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    return r;
}

void end_card(float min_h) {
    if (min_h > 0) {
        float need = g_card_padding + min_h - ImGui::GetCursorPosY();
        if (need > 1) ImGui::Dummy(ImVec2(0, need));
    }
    ImGui::EndChild();
}

}  // namespace ui
