#include "input.hpp"

#include <windows.h>

#include <algorithm>

MouseButton mouse_button_from(uint8_t v) {
    switch (v) {
        case 1: return MouseButton::Right;
        case 2: return MouseButton::Middle;
        default: return MouseButton::Left;
    }
}

const char* mouse_button_label(MouseButton b) {
    switch (b) {
        case MouseButton::Right: return "Direito";
        case MouseButton::Middle: return "Meio";
        default: return "Esquerdo";
    }
}

static INPUT mouse_input(DWORD flags) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = flags;
    return in;
}

void send_click(MouseButton btn) {
    DWORD down = MOUSEEVENTF_LEFTDOWN, up = MOUSEEVENTF_LEFTUP;
    if (btn == MouseButton::Right) {
        down = MOUSEEVENTF_RIGHTDOWN;
        up = MOUSEEVENTF_RIGHTUP;
    } else if (btn == MouseButton::Middle) {
        down = MOUSEEVENTF_MIDDLEDOWN;
        up = MOUSEEVENTF_MIDDLEUP;
    }
    INPUT inputs[2] = {mouse_input(down), mouse_input(up)};
    SendInput(2, inputs, sizeof(INPUT));
}

void move_cursor(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    INPUT in = mouse_input(MOUSEEVENTF_MOVE);
    in.mi.dx = dx;
    in.mi.dy = dy;
    SendInput(1, &in, sizeof(INPUT));
}

bool key_down(int vk) {
    return (static_cast<unsigned short>(GetAsyncKeyState(vk)) & 0x8000) != 0;
}

std::string foreground_title() {
    HWND hwnd = GetForegroundWindow();
    wchar_t buf[512];
    int len = GetWindowTextW(hwnd, buf, 512);
    if (len <= 0) return {};
    return to_utf8(std::wstring(buf, static_cast<size_t>(len)));
}

static BOOL CALLBACK enum_cb(HWND hwnd, LPARAM lparam) {
    if (IsWindowVisible(hwnd)) {
        int len = GetWindowTextLengthW(hwnd);
        if (len > 0) {
            std::wstring buf(static_cast<size_t>(len) + 1, L'\0');
            int n = GetWindowTextW(hwnd, buf.data(), len + 1);
            if (n > 0) {
                buf.resize(static_cast<size_t>(n));
                reinterpret_cast<std::vector<std::string>*>(lparam)->push_back(to_utf8(buf));
            }
        }
    }
    return TRUE;
}

std::vector<std::string> list_windows() {
    std::vector<std::string> titles;
    EnumWindows(enum_cb, reinterpret_cast<LPARAM>(&titles));
    std::sort(titles.begin(), titles.end());
    titles.erase(std::unique(titles.begin(), titles.end()), titles.end());
    return titles;
}

std::optional<int> first_key_down() {
    for (int vk = 0x02; vk <= 0xFE; ++vk) {
        if (key_down(vk)) return vk;
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Texto
// ---------------------------------------------------------------------------
std::wstring to_wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0,
                                nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr,
                        nullptr);
    return s;
}

std::string to_lower(const std::string& s) {
    std::wstring w = to_wide(s);
    if (!w.empty()) CharLowerBuffW(w.data(), static_cast<DWORD>(w.size()));
    return to_utf8(w);
}

std::string take_chars(const std::string& s, size_t max) {
    std::wstring w = to_wide(s);
    if (w.size() <= max) return s;
    w.resize(max);
    return to_utf8(w);
}

std::string ellipsize(const std::string& s, size_t max) {
    std::wstring w = to_wide(s);
    if (w.size() <= max) return s;
    w.resize(max > 0 ? max - 1 : 0);
    w.push_back(L'…');
    return to_utf8(w);
}
