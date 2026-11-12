// Ponto de entrada: sobe as threads, cria a janela Win32 + Direct3D 11 e roda a GUI.
#include <windows.h>
#include <d3d11.h>
#include <timeapi.h>

#include <algorithm>

#include "app.hpp"
#include "config.hpp"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "theme.hpp"
#include "tray.hpp"
#include "worker.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {

constexpr int kWidth = 960, kHeight = 600;        // tamanho inicial (área cliente)
constexpr int kMinWidth = 680, kMinHeight = 460;  // tamanho mínimo
constexpr int kIconId = 1;                        // ícone em res/app.rc

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
IDXGISwapChain* g_swap = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;
bool g_occluded = false;
UINT g_resize_w = 0, g_resize_h = 0;
float g_dpi = 1.0f;
Shared* g_shared = nullptr;
WindowState g_closing_state;  // lido no WM_CLOSE, antes da janela ser destruída

// Texto do ícone da bandeja (aparece ao passar o mouse).
const wchar_t* tray_tip(bool active) {
    return active ? L"DzkLP - Clicker (ativo)" : L"DzkLP - Clicker (parado)";
}

// Esconde a janela e deixa só o ícone na bandeja.
void send_to_tray(HWND hwnd) {
    ShowWindow(hwnd, SW_HIDE);
    tray::show(tray_tip(g_shared->clicking.load()));
}

// Traz a janela de volta da bandeja.
void restore_from_tray(HWND hwnd) {
    tray::hide();
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
}

void create_rtv() {
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }
}

void release_rtv() {
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
}

bool create_device(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL got;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels,
                                               2, D3D11_SDK_VERSION, &sd, &g_swap, &g_device, &got,
                                               &g_context);
    if (hr == DXGI_ERROR_UNSUPPORTED) {  // sem GPU compatível: usa o renderizador por software
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
                                           D3D11_SDK_VERSION, &sd, &g_swap, &g_device, &got,
                                           &g_context);
    }
    if (FAILED(hr)) return false;
    create_rtv();
    return true;
}

void cleanup_device() {
    release_rtv();
    if (g_swap) g_swap->Release();
    if (g_context) g_context->Release();
    if (g_device) g_device->Release();
    g_swap = nullptr;
    g_context = nullptr;
    g_device = nullptr;
}

// Lê o tamanho/posição atuais (a posição "restaurada", mesmo se maximizada ou na bandeja).
WindowState read_window_state(HWND hwnd) {
    WindowState ws;
    WINDOWPLACEMENT wp{};
    wp.length = sizeof(wp);
    if (!GetWindowPlacement(hwnd, &wp)) return ws;
    const RECT& r = wp.rcNormalPosition;
    ws.x = r.left;
    ws.y = r.top;
    ws.w = r.right - r.left;
    ws.h = r.bottom - r.top;
    ws.maximized = wp.showCmd == SW_SHOWMAXIMIZED ||
                   (wp.showCmd == SW_SHOWMINIMIZED && (wp.flags & WPF_RESTORETOMAXIMIZED)) ||
                   (!IsWindowVisible(hwnd) && (wp.flags & WPF_RESTORETOMAXIMIZED));
    return ws;
}

// Mostra a janela no tamanho/posição salvos. Se o lugar salvo não estiver mais em
// nenhum monitor (ex.: segundo monitor desconectado), usa a posição padrão.
void show_with_saved_state(HWND hwnd, const WindowState& ws) {
    if (ws.valid()) {
        RECT r{ws.x, ws.y, ws.x + ws.w, ws.y + ws.h};
        // Exige que pelo menos a barra de título (topo da janela) esteja visível em algum monitor.
        RECT title{r.left + 40, r.top, r.right - 40, r.top + 30};
        if (title.right > title.left && MonitorFromRect(&title, MONITOR_DEFAULTTONULL)) {
            WINDOWPLACEMENT wp{};
            wp.length = sizeof(wp);
            wp.showCmd = ws.maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
            wp.rcNormalPosition = r;
            SetWindowPlacement(hwnd, &wp);
            return;
        }
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
}

LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return TRUE;

    switch (msg) {
        case WM_SIZE:
            if (wp == SIZE_MINIMIZED) {
                if (g_shared && g_shared->get().minimize_to_tray) send_to_tray(hwnd);
            } else {
                g_resize_w = LOWORD(lp);
                g_resize_h = HIWORD(lp);
            }
            return 0;
        case tray::kCallbackMsg:
            switch (LOWORD(lp)) {
                case WM_LBUTTONUP:
                case WM_LBUTTONDBLCLK:
                    restore_from_tray(hwnd);
                    break;
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU: {
                    // Iniciar/Parar só faz sentido no modo liga/desliga.
                    bool toggle_mode = g_shared->get().mode == 0;
                    switch (tray::popup_menu(toggle_mode, g_shared->clicking.load())) {
                        case tray::kOpen: restore_from_tray(hwnd); break;
                        case tray::kToggle: g_shared->ui_toggle(); break;
                        case tray::kExit: PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
                        default: break;
                    }
                    break;
                }
            }
            return 0;
        case WM_GETMINMAXINFO: {
            RECT r{0, 0, LONG(kMinWidth * g_dpi), LONG(kMinHeight * g_dpi)};
            AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, 0);
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
            mmi->ptMinTrackSize.x = r.right - r.left;
            mmi->ptMinTrackSize.y = r.bottom - r.top;
            return 0;
        }
        case WM_DPICHANGED: {
            g_dpi = HIWORD(wp) / 96.0f;
            const RECT* r = reinterpret_cast<RECT*>(lp);
            SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_SYSCOMMAND:
            if ((wp & 0xfff0) == SC_KEYMENU) return 0;  // Alt não abre o menu da janela
            break;
        case WM_CLOSE:
            g_closing_state = read_window_state(hwnd);
            break;  // segue para DefWindowProc, que destrói a janela
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    // Explorer reiniciou: o ícone some, então coloca de novo.
    if (msg == tray::taskbar_created_msg() && tray::visible()) {
        tray::hide();
        tray::show(tray_tip(g_shared->clicking.load()));
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

int main() {
    // Timer do sistema em 1 ms: deixa as esperas curtas das threads mais precisas.
    timeBeginPeriod(1);
    ImGui_ImplWin32_EnableDpiAwareness();

    Config config = Config::load();
    Shared shared(config.active);
    Workers workers;
    workers.start(shared);
    g_shared = &shared;

    HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(kIconId), IMAGE_ICON,
                                             GetSystemMetrics(SM_CXICON),
                                             GetSystemMetrics(SM_CYICON), 0));
    wc.hIconSm = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(kIconId), IMAGE_ICON,
                                               GetSystemMetrics(SM_CXSMICON),
                                               GetSystemMetrics(SM_CYSMICON), 0));
    wc.lpszClassName = L"DzkLPClicker";
    RegisterClassExW(&wc);

    // Janela já no tamanho certo para o DPI do monitor principal.
    g_dpi = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
    RECT r{0, 0, LONG(kWidth * g_dpi), LONG(kHeight * g_dpi)};
    AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, 0);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"DzkLP - Clicker", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                                nullptr, nullptr, inst, nullptr);
    g_dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    tray::init(hwnd, wc.hIconSm ? wc.hIconSm : wc.hIcon);
    tray::taskbar_created_msg();

    if (!create_device(hwnd)) {
        MessageBoxW(nullptr, L"Não foi possível iniciar o Direct3D 11.", L"DzkLP - Clicker",
                    MB_ICONERROR);
        shared.running.store(false);
        workers.join();
        return 1;
    }
    show_with_saved_state(hwnd, config.window);
    UpdateWindow(hwnd);
    g_dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);  // pode ter aberto em outro monitor

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // nada de imgui.ini ao lado do exe
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    load_fonts();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);

    App app(shared, config);

    const float clear[4] = {0, 0, 0, 1};
    int extra_frames = 2;  // alguns frames extras após cada evento (layout/animações assentam)
    bool done = false;
    while (!done) {
        // Espera por eventos (não gasta CPU parado), mas acorda periodicamente
        // para atualizar o status/CPS.
        DWORD timeout = extra_frames > 0 ? 0 : app.idle_timeout_ms();
        MsgWaitForMultipleObjectsEx(0, nullptr, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

        MSG msg;
        bool had_msg = false;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
            had_msg = true;
        }
        if (done) break;
        extra_frames = had_msg ? 3 : std::max(extra_frames - 1, 0);

        // Na bandeja ou minimizado: não desenha nada, só mantém o texto do ícone em dia.
        if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
            static bool last_active = false;
            bool active = shared.clicking.load();
            if (active != last_active) {
                tray::set_tip(tray_tip(active));
                last_active = active;
            }
            extra_frames = 0;
            continue;
        }

        if (g_occluded && g_swap->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_occluded = false;

        if (g_resize_w != 0 && g_resize_h != 0) {
            release_rtv();
            g_swap->ResizeBuffers(0, g_resize_w, g_resize_h, DXGI_FORMAT_UNKNOWN, 0);
            g_resize_w = g_resize_h = 0;
            create_rtv();
        }

        app.before_frame(g_dpi);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.frame();
        ImGui::Render();

        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = g_swap->Present(1, 0);  // com vsync
        g_occluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    app.set_window_state(g_closing_state.valid() ? g_closing_state : read_window_state(hwnd));
    tray::hide();
    app.on_exit();
    workers.join();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    cleanup_device();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, inst);
    timeEndPeriod(1);
    return 0;
}
