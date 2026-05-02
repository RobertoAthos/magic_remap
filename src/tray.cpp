#include "tray.hpp"

#include <windows.h>
#include <shellapi.h>

#include "app.hpp"
#include "autostart.hpp"
#include "logging.hpp"
#include "resource.h"

namespace magic_remap {

Tray::Tray(App& app) : app_(app) {}

bool Tray::Create(HWND hwnd) {
    hwnd_ = hwnd;

    nid_.cbSize           = sizeof(nid_);
    nid_.hWnd             = hwnd;
    nid_.uID              = 1;
    nid_.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = kCallbackMessage;

    // Tenta carregar o ícone embutido; se não houver, usa o padrão de aplicação.
    HMODULE me = GetModuleHandleW(nullptr);
    nid_.hIcon = static_cast<HICON>(LoadImageW(me, MAKEINTRESOURCEW(IDI_APPICON),
                                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    if (!nid_.hIcon) {
        nid_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    UpdateTooltip(true);

    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        MR_LOGF(L"Shell_NotifyIcon NIM_ADD failed err=%lu", GetLastError());
        return false;
    }
    nid_.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);
    return true;
}

void Tray::Destroy() {
    if (hwnd_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        hwnd_ = nullptr;
    }
}

void Tray::UpdateTooltip(bool enabled) {
    const wchar_t* status = enabled ? L"ativo" : L"desabilitado";
    swprintf_s(nid_.szTip, L"MagicRemap (%s) — clique direito para opções", status);
    if (hwnd_) {
        nid_.uFlags = NIF_TIP;
        Shell_NotifyIconW(NIM_MODIFY, &nid_);
        nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    }
}

void Tray::OnTrayMessage(LPARAM lParam) {
    // Com NOTIFYICON_VERSION_4, lParam carrega o evento no LOWORD.
    UINT msg = LOWORD(lParam);
    if (msg == WM_RBUTTONUP || msg == WM_CONTEXTMENU || msg == WM_LBUTTONUP) {
        ShowMenu();
    }
}

void Tray::ShowMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    bool enabled    = app_.IsEnabled();
    bool autostart  = autostart::IsEnabled();

    AppendMenuW(menu, MF_STRING | (enabled   ? MF_CHECKED : 0), IDM_TRAY_ENABLED,   L"Habilitado");
    AppendMenuW(menu, MF_STRING | (autostart ? MF_CHECKED : 0), IDM_TRAY_AUTOSTART, L"Iniciar com Windows");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_TRAY_RELOAD,      L"Recarregar config");
    AppendMenuW(menu, MF_STRING, IDM_TRAY_OPEN_CONFIG, L"Abrir config.ini");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_TRAY_ABOUT, L"Sobre");
    AppendMenuW(menu, MF_STRING, IDM_TRAY_QUIT,  L"Sair");

    POINT pt;
    GetCursorPos(&pt);

    // Necessário antes de TrackPopupMenu para o menu fechar corretamente em clique fora.
    SetForegroundWindow(hwnd_);

    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);

    DestroyMenu(menu);
}

}  // namespace magic_remap
