#include "tray.hpp"

#include <shellapi.h>

#include <cwchar>
#include <iterator>

namespace tray {

static NOTIFYICONDATAW g_nid{};
static bool g_visible = false;

void init(HWND hwnd, HICON icon) {
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = kCallbackMsg;
    g_nid.hIcon = icon;
}

static void copy_tip(const wchar_t* tip) {
    wcsncpy(g_nid.szTip, tip, std::size(g_nid.szTip) - 1);
    g_nid.szTip[std::size(g_nid.szTip) - 1] = L'\0';
}

void show(const wchar_t* tip) {
    copy_tip(tip);
    if (g_visible) {
        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    } else {
        g_visible = Shell_NotifyIconW(NIM_ADD, &g_nid) != FALSE;
    }
}

void hide() {
    if (!g_visible) return;
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    g_visible = false;
}

void set_tip(const wchar_t* tip) {
    if (!g_visible) return;
    copy_tip(tip);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

bool visible() { return g_visible; }

UINT taskbar_created_msg() {
    static UINT msg = RegisterWindowMessageW(L"TaskbarCreated");
    return msg;
}

UINT popup_menu(bool show_toggle, bool active) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kOpen, L"Abrir");
    if (show_toggle) AppendMenuW(menu, MF_STRING, kToggle, active ? L"Parar" : L"Iniciar");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kExit, L"Sair");
    SetMenuDefaultItem(menu, kOpen, FALSE);

    POINT pt;
    GetCursorPos(&pt);
    // Sem isso o menu não fecha ao clicar fora (comportamento documentado do Windows).
    SetForegroundWindow(g_nid.hWnd);
    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0,
                              g_nid.hWnd, nullptr);
    PostMessageW(g_nid.hWnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
    return cmd;
}

}  // namespace tray
