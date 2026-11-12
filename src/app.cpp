#include "app.hpp"

#include <algorithm>
#include <cfloat>
#include <cstdio>

#include "imgui.h"
#include "imgui_stdlib.h"
#include "input.hpp"
#include "ui.hpp"
#include "version.h"


namespace {

// Título da própria janela do app, para removê-la da lista de alvos.
const char* const SELF_TITLE = "DzkLP - Clicker";

// Verde de "ativo" usado na pílula de status.
constexpr ImVec4 GREEN = rgb(0x3F, 0xC5, 0x6B);

bool is_blank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }

// Lista as janelas abertas, sem a própria janela do app nem títulos vazios.
std::vector<std::string> window_choices() {
    std::vector<std::string> out;
    for (auto& t : list_windows()) {
        if (!is_blank(t) && t != SELF_TITLE) out.push_back(t);
    }
    return out;
}

// Palavras que indicam uma janela de Minecraft (jogo, clientes ou launchers).
const char* const MINECRAFT_HINTS[] = {
    "minecraft", "lunar",  "badlion",  "feather",   "labymod",    "salwyrr",
    "pvp lounge", "pvplounge", "forge", "fabric",   "optifine",   "rlcraft",
    "tlauncher", "multimc", "prism",   "atlauncher", "curseforge", "modrinth",
    "technic",   "ftb",     "polymc",  "legacylauncher",
};

// Heurística simples: o título parece ser de algo ligado ao Minecraft?
bool minecraft_like(const std::string& title) {
    std::string t = to_lower(title);
    for (const char* h : MINECRAFT_HINTS) {
        if (t.find(h) != std::string::npos) return true;
    }
    return false;
}

struct Badge {
    const char* label;
    ImVec4 color;
};

// Rótulo e cor do "selo" do cliente, a partir do título da janela.
Badge client_badge(const std::string& title) {
    std::string t = to_lower(title);
    auto has = [&](const char* k) { return t.find(k) != std::string::npos; };
    if (has("lunar")) return {"Lunar", rgb(0x5B, 0xA8, 0xFF)};
    if (has("badlion")) return {"Badlion", rgb(0xE0, 0x5A, 0x2B)};
    if (has("feather")) return {"Feather", rgb(0x8E, 0xD1, 0xFF)};
    if (has("labymod")) return {"LabyMod", rgb(0xB1, 0x7A, 0xF0)};
    if (has("salwyrr")) return {"Salwyrr", rgb(0xF0, 0x7A, 0xC0)};
    if (has("pvp lounge") || has("pvplounge")) return {"PvP Lounge", GREEN};
    if (has("optifine")) return {"OptiFine", rgb(0x7F, 0xB0, 0x5A)};
    if (has("forge")) return {"Forge", rgb(0x8A, 0x9A, 0xB0)};
    if (has("fabric")) return {"Fabric", rgb(0xC9, 0xB4, 0x8F)};
    if (has("tlauncher") || has("multimc") || has("polymc") || has("prism") || has("atlauncher") ||
        has("curseforge") || has("modrinth") || has("technic") || has("ftb") ||
        has("legacylauncher"))
        return {"Launcher", rgb(0x9A, 0xA0, 0xB0)};
    return {"Minecraft", rgb(0x5F, 0xB8, 0x5F)};
}

// Linha de janela estilo "card": selo colorido, título e rótulo do cliente à direita.
// Retorna 0 (nada), 1 (clique) ou 2 (duplo clique).
int window_row(const Palette& p, const std::string& title, bool selected) {
    Badge b = client_badge(title);
    float w = ImGui::GetContentRegionAvail().x;
    float h = S(30);
    ImGui::PushID(title.c_str());
    bool clicked = ImGui::InvisibleButton("row", ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();
    bool dbl = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    ImGui::PopID();

    ImVec2 a = ImGui::GetItemRectMin(), z = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec4 bg = selected ? p.surface2 : hovered ? with_alpha(p.surface2, 0.55f) : p.surface;
    dl->AddRectFilled(a, z, u32(bg), S(7));
    if (selected) dl->AddRectFilled(a, ImVec2(a.x + S(3), z.y), u32(p.accent), S(2));

    float cy = (a.y + z.y) * 0.5f;
    float dot_x = a.x + S(16);
    dl->AddCircleFilled(ImVec2(dot_x, cy), S(4), u32(b.color));

    float label_w = ImGui::GetFont()->CalcTextSizeA(S(11.5f), FLT_MAX, 0, b.label).x;
    dl->PushClipRect(a, ImVec2(z.x - label_w - S(18), z.y), true);
    ui::draw_text(dl, ImVec2(dot_x + S(12), cy), 0, 0.5f, take_chars(title, 60).c_str(), S(13.5f),
                  u32(p.text));
    dl->PopClipRect();
    ui::draw_text(dl, ImVec2(z.x - S(10), cy), 1, 0.5f, b.label, S(11.5f), u32(p.muted));
    return dbl ? 2 : clicked ? 1 : 0;
}

// Item do menu lateral: faixa clicável de largura total, acento quando ativo.
bool nav_item(const Palette& p, const char* label, bool selected) {
    float w = ImGui::GetContentRegionAvail().x;
    float h = S(36);
    bool clicked = ImGui::InvisibleButton(label, ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();
    ImVec2 a = ImGui::GetItemRectMin(), z = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (selected) {
        dl->AddRectFilled(a, z, u32(with_alpha(p.accent, 0.16f)), S(8));
        dl->AddRectFilled(ImVec2(a.x, a.y + S(7)), ImVec2(a.x + S(3), z.y - S(7)), u32(p.accent), S(2));
    } else if (hovered) {
        dl->AddRectFilled(a, z, u32(p.surface2), S(8));
    }
    ui::draw_text(dl, ImVec2(a.x + S(14), (a.y + z.y) * 0.5f), 0, 0.5f, label, S(14.5f),
                  u32(selected ? p.accent : p.text));
    return clicked;
}

// Nome legível para um virtual-key code do Windows.
std::string vk_name(int vk) {
    switch (vk) {
        case 0x01: return "Mouse Esq";
        case 0x02: return "Mouse Dir";
        case 0x04: return "Mouse Meio";
        case 0x05: return "Mouse 4";
        case 0x06: return "Mouse 5";
        case 0x08: return "Backspace";
        case 0x09: return "Tab";
        case 0x0D: return "Enter";
        case 0x10: return "Shift";
        case 0x11: return "Ctrl";
        case 0x12: return "Alt";
        case 0x14: return "CapsLock";
        case 0x1B: return "Esc";
        case 0x20: return "Espaço";
        case 0x21: return "PageUp";
        case 0x22: return "PageDown";
        case 0x23: return "End";
        case 0x24: return "Home";
        case 0x25: return "Seta Esq";
        case 0x26: return "Seta Cima";
        case 0x27: return "Seta Dir";
        case 0x28: return "Seta Baixo";
        case 0x2D: return "Insert";
        case 0x2E: return "Delete";
        case 0xA0: return "Shift Esq";
        case 0xA1: return "Shift Dir";
        case 0xA2: return "Ctrl Esq";
        case 0xA3: return "Ctrl Dir";
        case 0xA4: return "Alt Esq";
        case 0xA5: return "Alt Dir";
        default: break;
    }
    char buf[32];
    if ((vk >= 0x30 && vk <= 0x39) || (vk >= 0x41 && vk <= 0x5A)) return std::string(1, char(vk));
    if (vk >= 0x60 && vk <= 0x69) {
        std::snprintf(buf, sizeof buf, "Num %d", vk - 0x60);
    } else if (vk >= 0x70 && vk <= 0x87) {
        std::snprintf(buf, sizeof buf, "F%d", vk - 0x6F);
    } else {
        std::snprintf(buf, sizeof buf, "Tecla 0x%02X", vk);
    }
    return buf;
}

// Linha "valor grande + legenda" do card de status.
void stat(ImVec4 fg, ImVec4 sub, const std::string& value, const char* caption) {
    ImGui::BeginGroup();
    ui::text(value, 22.0f, fg, mono_font());
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - S(6));
    ui::text(caption, 11.0f, sub);
    ImGui::EndGroup();
}

std::string fmt1(float v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f", v);
    return buf;
}

const char* EM_DASH = "\xE2\x80\x94";  // "—"

// Cartão de status: fica âmbar e ganha destaque quando está clicando.
// Retorna true se o botão Iniciar/Parar foi clicado (modo toggle).
bool hero(const Palette& p, bool active, float cps, float cps_meas, uint64_t clicks,
          const Settings& s, float width) {
    ImVec4 fill = active ? p.accent : p.surface;
    ImVec4 fg = active ? p.on_accent : p.text;
    ImVec4 sub = active ? p.on_accent : p.muted;
    bool toggled = false;

    ui::begin_card("##hero", fill, active ? p.accent : p.stroke, width, S(16), S(12));
    float inner_w = ImGui::GetContentRegionAvail().x;
    float top = ImGui::GetCursorPosY();

    ImGui::BeginGroup();
    ui::text(active ? "Atacando" : "Pronto", 22.0f, fg);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - S(4));
    const char* caption = s.mode == 1 ? "Modo segurar tecla"
                          : s.mode == 2 ? "Modo segurar mouse"
                                        : "Modo liga/desliga";
    ui::text(caption, 12.0f, sub);
    if (s.target_window_only && !is_blank(s.target_window_title)) {
        ui::text("Janela: " + ellipsize(s.target_window_title, 44), 12.0f, sub);
    } else {
        ui::text("Janela: qualquer", 12.0f, sub);
    }
    ImGui::EndGroup();
    float left_h = ImGui::GetItemRectSize().y;
    float after_left = ImGui::GetCursorPosY();

    // À direita, centralizado na altura do bloco da esquerda.
    if (s.mode == 0) {
        ImVec2 bsz(S(120), S(42));
        ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().WindowPadding.x + inner_w - bsz.x,
                                   top + (left_h - bsz.y) * 0.5f));
        if (ui::filled_button(p, active ? "Parar" : "Iniciar", active ? p.danger : p.accent, bsz,
                              15.0f, S(9)))
            toggled = true;
    } else {
        const char* hint = s.mode == 1 ? "segure a tecla" : "segure o mouse";
        ImGui::PushFont(nullptr, 12.0f);
        ImVec2 ts = ImGui::CalcTextSize(hint);
        ImGui::PopFont();
        ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().WindowPadding.x + inner_w - ts.x,
                                   top + (left_h - ts.y) * 0.5f));
        ui::text(hint, 12.0f, sub);
    }
    ImGui::SetCursorPosY(after_left);

    ui::space(4);
    stat(fg, sub, active ? fmt1(cps) : EM_DASH, "CPS alvo");
    ImGui::SameLine(0, S(32));
    stat(fg, sub, active ? fmt1(cps_meas) : EM_DASH, "CPS real");
    ImGui::SameLine(0, S(32));
    stat(fg, sub, std::to_string(clicks), "cliques");

    ui::end_card(0);
    return toggled;
}

// Seção "Velocidade" da página do clicker.
void clicker_speed(const Palette& p, Settings& s, float width, float min_h) {
    ui::section(p, "Velocidade", width, min_h, [&] {
        // Slider adaptativo: encolhe em colunas estreitas, cresce em largas.
        float sw = std::clamp(ImGui::GetContentRegionAvail().x - S(110), S(90), S(230));
        ImGui::SetNextItemWidth(sw);
        ImGui::SliderFloat("CPS mínimo", &s.min_cps, 1.0f, 100.0f, "%.1f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::BeginDisabled(!s.jitter);
        ImGui::SetNextItemWidth(sw);
        ImGui::SliderFloat("CPS máximo", &s.max_cps, 1.0f, 100.0f, "%.1f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndDisabled();
        if (s.max_cps < s.min_cps) s.max_cps = s.min_cps;
        ui::space(4);
        ImGui::Checkbox("Variar entre mínimo e máximo", &s.jitter);
    });
}

// Seção "Acionamento" da página do clicker.
void clicker_trigger(const Palette& p, Settings& s, bool& capturing, float width, float min_h) {
    ui::section(p, "Acionamento", width, min_h, [&] {
        if (!ImGui::BeginTable("acion", 2, ImGuiTableFlags_None)) return;
        ImGui::TableSetupColumn("k", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Botão");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, S(150)));
        if (ImGui::BeginCombo("##btn", mouse_button_label(mouse_button_from(s.button)))) {
            const char* names[] = {"Esquerdo", "Direito", "Meio"};
            for (uint8_t i = 0; i < 3; ++i) {
                if (ImGui::Selectable(names[i], s.button == i)) s.button = i;
            }
            ImGui::EndCombo();
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Modo");
        ImGui::TableNextColumn();
        {
            const char* modes[] = {"Toggle", "Segurar tecla", "Segurar mouse"};
            for (uint8_t i = 0; i < 3; ++i) {
                if (i > 0) ui::same_line_if_fits(ui::button_width(modes[i]));
                if (ui::pill(p, modes[i], s.mode == i)) s.mode = i;
            }
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Tecla");
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(s.mode == 2);
        if (capturing) {
            if (ui::filled_button(p, "aperte uma tecla\xE2\x80\xA6##key", p.accent)) capturing = false;
            ui::same_line_if_fits(S(70));
            ImGui::AlignTextToFramePadding();
            ui::text("(Esc cancela)", 11.0f, p.muted);
        } else {
            std::string label = vk_name(s.hotkey_vk) + "##key";
            if (ImGui::Button(label.c_str())) capturing = true;
        }
        ImGui::EndDisabled();

        ImGui::EndTable();
    });
}

// Seção "Avançado" da página do clicker.
void clicker_advanced(const Palette& p, Settings& s, Shared& shared, float width, float min_h) {
    ui::section(p, "Avançado", width, min_h, [&] {
        const ImGuiSliderFlags clamp = ImGuiSliderFlags_AlwaysClamp;
        float dw = S(110);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Atraso ao iniciar");
        ImGui::SameLine();
        uint64_t dmin = 0, dmax = 10'000;
        ImGui::SetNextItemWidth(dw);
        ImGui::DragScalar("##delay", ImGuiDataType_U64, &s.start_delay_ms, 10.0f, &dmin, &dmax,
                          "%llu ms", clamp);

        ImGui::Checkbox("Parar após", &s.click_limit_enabled);
        ImGui::SameLine();
        ImGui::BeginDisabled(!s.click_limit_enabled);
        uint64_t lmin = 1, lmax = 1'000'000;
        ImGui::SetNextItemWidth(dw);
        ImGui::DragScalar("##limit", ImGuiDataType_U64, &s.click_limit, 10.0f, &lmin, &lmax,
                          "%llu cliques", clamp);
        ImGui::EndDisabled();

        ImGui::Checkbox("Só na janela-alvo", &s.target_window_only);
        if (s.target_window_only && !is_blank(s.target_window_title)) {
            ui::text("Alvo: " + ellipsize(s.target_window_title, 40), 11.0f, p.muted);
        }
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Rajada");
        ImGui::SameLine();
        uint32_t bmin = 1, bmax = 1000;
        ImGui::SetNextItemWidth(dw);
        ImGui::DragScalar("##burst", ImGuiDataType_U32, &s.burst_clicks, 1.0f, &bmin, &bmax,
                          "%u cliques", clamp);
        ui::same_line_if_fits(ui::button_width("Disparar"));
        if (ImGui::Button("Disparar")) shared.burst_request.store(true);
    });
}

}  // namespace

// ===========================================================================

App::App(Shared& shared, Config config)
    : shared_(shared), config_(std::move(config)), pal_(palette(config_.active.dark_mode)) {
    win_list_ = window_choices();
    std::string saved = config_.active.target_window_title;
    if (std::find(win_list_.begin(), win_list_.end(), saved) != win_list_.end()) picked_ = saved;
    meas_t_ = std::chrono::steady_clock::now();
}

void App::before_frame(float dpi) {
    if (theme_dirty_ || dpi != scale_) {
        scale_ = dpi;
        install_theme(pal_, scale_);
        theme_dirty_ = false;
    }
}

unsigned App::idle_timeout_ms() const {
    if (capturing_) return 0;
    if (screen_ == Screen::Picker) return 200;
    return 100;
}

void App::persist() {
    config_.active = shared_.get();
    config_.save();
}

void App::on_exit() {
    shared_.running.store(false);
    persist();
}

// Aplica a escolha da tela inicial e entra no painel.
void App::confirm_window(bool scoped) {
    Settings s = shared_.get();
    s.target_window_only = scoped;
    if (scoped && picked_) s.target_window_title = *picked_;
    shared_.set(s);
    persist();
    status_ = scoped ? "Clicker vai atuar em: " + s.target_window_title
                     : "Clicker vai atuar em qualquer janela.";
    screen_ = Screen::Main;
}

// Volta para a tela de seleção e atualiza a lista de janelas.
void App::open_picker() {
    win_list_ = window_choices();
    std::string current = shared_.get().target_window_title;
    picked_.reset();
    if (std::find(win_list_.begin(), win_list_.end(), current) != win_list_.end()) picked_ = current;
    screen_ = Screen::Picker;
}

void App::frame() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (screen_ == Screen::Picker) {
        picker_screen();
    } else {
        main_screen();
    }
    ImGui::End();
}

// ---------------------------------------------------------------------------
// Tela inicial: escolher a janela onde o clicker vai funcionar.
// ---------------------------------------------------------------------------
void App::picker_screen() {
    const Palette& p = pal_;
    float avail = ImGui::GetContentRegionAvail().x;
    float col_w = std::min(avail - S(32), S(560));
    float pad = std::max((avail - col_w) * 0.5f, 0.0f);

    ImGui::SetCursorPos(ImVec2(pad, 0));
    ImGui::BeginChild("col", ImVec2(col_w, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
    // Largura útil (desconta a barra de rolagem, se aparecer): base dos alinhamentos à direita.
    col_w = ImGui::GetContentRegionAvail().x;
    ui::space(28);

    ui::logo_title(p, S(34), S(10), 22.0f, "DzkLP Clicker", 20.0f);
    ui::space(10);

    ui::text("Onde o clicker vai funcionar?", 22.0f, p.text);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - S(4));
    ui::text_wrapped(
        "Escolha a janela em que os cliques serão enviados. "
        "Você pode trocar a qualquer momento depois.",
        13.0f, p.muted);
    ui::space(10);

    // Filtro de categoria: só Minecraft ou todas as janelas.
    if (ui::pill(p, "Só Minecraft", mc_only_)) mc_only_ = true;
    ImGui::SameLine();
    if (ui::pill(p, "Todas as janelas", !mc_only_)) mc_only_ = false;
    ImGui::SameLine(col_w - ui::button_width("Atualizar"));
    if (ImGui::Button("Atualizar")) win_list_ = window_choices();
    ui::space(2);

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##filter", "filtrar pelo título\xE2\x80\xA6", &win_filter_);
    ui::space(4);

    std::string filter = to_lower(win_filter_);
    std::vector<std::string> matches;
    for (auto& t : win_list_) {
        if (mc_only_ && !minecraft_like(t)) continue;
        if (!filter.empty() && to_lower(t).find(filter) == std::string::npos) continue;
        matches.push_back(t);
    }

    char count[64];
    std::snprintf(count, sizeof count, "%zu janela%s%s", matches.size(),
                  matches.size() == 1 ? "" : "s", mc_only_ ? " de Minecraft" : "");
    ui::text(count, 12.0f, p.muted);

    // Lista em cartão com rolagem própria.
    bool confirm_now = false;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, p.surface);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, S(10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(S(14), S(14)));
    // Altura da lista: até 300 px, encolhendo para o rodapé sempre caber na janela.
    float footer = ImGui::GetFrameHeight() + S(44);
    float list_h = std::clamp(ImGui::GetContentRegionAvail().y - footer, S(120), S(300) + S(28));
    ImGui::BeginChild("list", ImVec2(0, list_h),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(S(8), S(4)));
    if (matches.empty()) {
        ui::text_wrapped(mc_only_ ? "Nenhuma janela de Minecraft encontrada. Abra o jogo/launcher e "
                                    "clique em Atualizar, ou mude para \"Todas as janelas\"."
                                  : "Nenhuma janela encontrada. Abra o programa-alvo e clique em "
                                    "Atualizar.",
                         12.0f, p.muted);
    }
    for (auto& title : matches) {
        bool selected = picked_ && *picked_ == title;
        int r = window_row(p, title, selected);
        if (r >= 1) picked_ = title;
        if (r == 2) confirm_now = true;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ui::space(10);

    // Rodapé: "qualquer janela" à esquerda, "Continuar" à direita.
    if (ImGui::Button("Usar em qualquer janela")) {
        confirm_now = false;
        confirm_window(false);
    }
    float left_end = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x;
    float cont_w = ui::button_width("Continuar");
    if (picked_) {
        std::string short_t = take_chars(*picked_, 32);
        ImGui::PushFont(nullptr, 12.0f);
        float tw = ImGui::CalcTextSize(short_t.c_str()).x;
        ImGui::PopFont();
        float x = col_w - cont_w - S(10) - tw;
        if (x > left_end + S(10)) {
            ImGui::SameLine(x);
            ImGui::AlignTextToFramePadding();
            ui::text(short_t, 12.0f, p.muted);
        }
    }
    ImGui::SameLine(col_w - cont_w);
    ImGui::BeginDisabled(!picked_);
    if (ui::filled_button(p, "Continuar", p.accent)) confirm_now = true;
    ImGui::EndDisabled();
    ui::space(10);

    ImGui::EndChild();

    if (confirm_now && screen_ == Screen::Picker) confirm_window(true);
}

// ---------------------------------------------------------------------------
// Tela principal: menu lateral + página escolhida.
// ---------------------------------------------------------------------------
void App::main_screen() {
    const Palette& p = pal_;
    bool active = shared_.clicking.load();
    uint64_t clicks = shared_.clicks_done.load();
    float cps_now = shared_.cps_now.load();

    // CPS real: cliques observados por segundo, atualizado a cada ~0,5s.
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - meas_t_).count();
    if (dt >= 0.5f) {
        uint64_t dc = clicks >= meas_clicks_ ? clicks - meas_clicks_ : 0;
        meas_cps_ = active ? float(dc) / dt : 0.0f;
        meas_clicks_ = clicks;
        meas_t_ = now;
    }
    if (!active) meas_cps_ = 0.0f;

    Settings s = shared_.get();
    const Settings before = s;
    bool theme_changed = false, go_picker = false, toggle_req = false;

    // Captura de hotkey: aguarda a próxima tecla pressionada.
    if (capturing_) {
        if (auto vk = first_key_down()) {
            if (*vk != 0x1B) {
                s.hotkey_vk = *vk;
                s.hotkey_name = vk_name(*vk);
            }
            capturing_ = false;
        }
    }

    sidebar(s, active, go_picker);
    ImGui::SameLine(0, 0);

    // Conteúdo com rolagem.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(S(16), 0));
    ImGui::BeginChild("content", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    {
        float avail = ImGui::GetContentRegionAvail().x;
        // Ocupa a largura toda, travando só em telas ultrawide.
        float col_w = std::min(avail, S(1500));
        float pad = std::max((avail - col_w) * 0.5f, 0.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
        ImGui::BeginGroup();
        ui::space(14);
        switch (page_) {
            case Page::Clicker:
                clicker_page(s, active, cps_now, clicks, col_w, toggle_req);
                break;
            case Page::Profiles:
                profiles_page(s, col_w);
                break;
            case Page::Appearance:
                appearance_page(s, col_w, theme_changed);
                break;
        }
        ui::space(8);
        ImGui::EndGroup();
    }
    ImGui::EndChild();

    if (theme_changed) {
        pal_ = palette(s.dark_mode);
        theme_dirty_ = true;  // aplicado antes do próximo frame
    }
    if (s != before) {
        shared_.set(s);
        persist();
    }
    if (toggle_req) shared_.ui_toggle();
    if (go_picker) open_picker();
}

void App::sidebar(const Settings& s, bool active, bool& go_picker) {
    const Palette& p = pal_;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, p.surface);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(S(12), S(14)));
    ImGui::BeginChild("nav", ImVec2(S(214), 0),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // Cabeçalho: logo + nome + pílula de status.
    ui::logo_title(p, S(30), S(8), 19.0f, "DzkLP", 18.0f);
    ui::dot_label(active ? "Ativo" : "Parado", active ? GREEN : p.muted, 12.0f);
    ui::space(10);

    // Itens de navegação. Novos módulos entram aqui.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(S(8), S(2)));
    if (nav_item(p, "AutoClicker", page_ == Page::Clicker)) page_ = Page::Clicker;
    if (nav_item(p, "Perfis", page_ == Page::Profiles)) page_ = Page::Profiles;
    if (nav_item(p, "Aparência", page_ == Page::Appearance)) page_ = Page::Appearance;
    ImGui::PopStyleVar();

    // Rodapé fixo embaixo: alvo atual, trocar janela e versão.
    std::string alvo = s.target_window_only && !is_blank(s.target_window_title)
                           ? "Alvo: " + ellipsize(s.target_window_title, 22)
                           : std::string("Alvo: qualquer janela");
    ImGui::PushFont(nullptr, 11.0f);
    float lh = ImGui::GetTextLineHeight();
    ImGui::PopFont();
    float sp = ImGui::GetStyle().ItemSpacing.y;
    float footer_h = lh + sp + S(30) + sp + S(4) + lh;
    float y = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - footer_h;
    if (y > ImGui::GetCursorPosY()) ImGui::SetCursorPosY(y);

    ui::text(alvo, 11.0f, p.muted);
    if (ImGui::Button("Trocar janela", ImVec2(-FLT_MIN, S(30)))) go_picker = true;
    ui::space(4);
    ui::text("v" APP_VERSION, 11.0f, p.muted);

    ImGui::EndChild();
}

void App::clicker_page(Settings& s, bool active, float cps_now, uint64_t clicks, float width,
                       bool& toggle_req) {
    const Palette& p = pal_;
    if (hero(p, active, cps_now, meas_cps_, clicks, s, width)) toggle_req = true;
    ui::space(10);

    // Layout responsivo: 3 colunas (largo), 2+1 (médio) ou 1 (estreito).
    // Em telas grandes os cards esticam para preencher a altura.
    float sp = ImGui::GetStyle().ItemSpacing.x;
    float fill_h = std::clamp(ImGui::GetContentRegionAvail().y - S(60), S(230), S(460));
    auto columns = [&](int n, auto&& draw) {
        float cw = (width - sp * float(n - 1)) / float(n);
        ImVec2 start = ImGui::GetCursorPos();
        float max_y = start.y;
        for (int i = 0; i < n; ++i) {
            ImGui::SetCursorPos(ImVec2(start.x + float(i) * (cw + sp), start.y));
            ImGui::BeginGroup();
            draw(i, cw);
            ImGui::EndGroup();
            max_y = std::max(max_y, ImGui::GetItemRectMax().y - ImGui::GetWindowPos().y +
                                        ImGui::GetScrollY());
        }
        ImGui::SetCursorPos(ImVec2(start.x, max_y));
        ImGui::Dummy(ImVec2(0, 0));
    };

    if (width >= S(840)) {
        columns(3, [&](int i, float cw) {
            if (i == 0) clicker_speed(p, s, cw, fill_h);
            if (i == 1) clicker_trigger(p, s, capturing_, cw, fill_h);
            if (i == 2) clicker_advanced(p, s, shared_, cw, fill_h);
        });
    } else if (width >= S(540)) {
        // Aqui o "Avançado" fica embaixo: sem esticar, para ele continuar visível.
        columns(2, [&](int i, float cw) {
            if (i == 0) clicker_speed(p, s, cw, 0);
            if (i == 1) clicker_trigger(p, s, capturing_, cw, 0);
        });
        clicker_advanced(p, s, shared_, width, 0);
    } else {
        clicker_speed(p, s, width, 0);
        clicker_trigger(p, s, capturing_, width, 0);
        clicker_advanced(p, s, shared_, width, 0);
    }

    std::string hint = s.mode == 1   ? "Segure " + s.hotkey_name + " para clicar; solte para parar."
                       : s.mode == 2 ? "Segure o botão esquerdo do mouse para clicar; solte para parar."
                                     : "Aperte " + s.hotkey_name + " para ligar e desligar.";
    ui::text(hint, 12.0f, p.muted);
    if (!status_.empty()) ui::text(status_, 12.0f, p.accent);
}

// Página de perfis (salvar/carregar/excluir configurações nomeadas).
void App::profiles_page(Settings& s, float width) {
    const Palette& p = pal_;
    ui::text("Perfis", 20.0f, p.text);
    ui::space(6);

    ui::section(p, "Salvar configuração atual", width, 0, [&] {
        ImGui::SetNextItemWidth(S(200));
        bool enter = ImGui::InputTextWithHint("##pname", "nome do perfil", &new_profile_name_,
                                              ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Salvar") || enter) {
            std::string name = new_profile_name_;
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            if (!name.empty()) {
                config_.profiles[name] = s;
                new_profile_name_.clear();
                status_ = "Perfil \"" + name + "\" salvo.";
                persist();
            }
        }
    });

    ui::section(p, "Perfis salvos", width, 0, [&] {
        if (config_.profiles.empty()) {
            ui::text_wrapped("Nenhum perfil ainda. Salve a configuração atual para reusar depois.",
                             12.0f, p.muted);
        }
        std::string to_remove;
        for (auto& [name, prof] : config_.profiles) {
            ImGui::PushID(name.c_str());
            if (ImGui::Button("Carregar")) {
                s = prof;
                status_ = "Perfil \"" + name + "\" carregado.";
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, p.danger);
            if (ImGui::Button("Excluir")) to_remove = name;
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name.c_str());
            ImGui::PopID();
        }
        if (!to_remove.empty()) {
            config_.profiles.erase(to_remove);
            persist();
        }
    });
}

void App::appearance_page(Settings& s, float width, bool& theme_changed) {
    const Palette& p = pal_;
    ui::text("Aparência", 20.0f, p.text);
    ui::space(6);
    ui::section(p, "Tema", width, 0, [&] {
        if (ui::pill(p, "Claro", !s.dark_mode) && s.dark_mode) {
            s.dark_mode = false;
            theme_changed = true;
        }
        ImGui::SameLine();
        if (ui::pill(p, "Escuro", s.dark_mode) && !s.dark_mode) {
            s.dark_mode = true;
            theme_changed = true;
        }
    });
    ui::section(p, "Janela", width, 0, [&] {
        ImGui::Checkbox("Minimizar para a bandeja", &s.minimize_to_tray);
        ui::text_wrapped(
            "Ao minimizar, o app sai da barra de tarefas e fica como ícone perto do relógio. "
            "Clique no ícone para voltar; botão direito para iniciar/parar ou sair.",
            12.0f, p.muted);
    });
}
