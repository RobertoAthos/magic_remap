#include "app.hpp"

#include <windows.h>
#include <shellapi.h>

#include "autostart.hpp"
#include "logging.hpp"
#include "resource.h"

namespace magic_remap {

App::App() = default;
App::~App() = default;

bool App::Init(HWND hwnd, HINSTANCE hInstance) {
    hwnd_ = hwnd;
    hInst_ = hInstance;

    config_ = Load();
    log::Init(config_.verbose);
    MR_LOGF(L"Config carregada de %ls", config_.path.c_str());

    filter_    = std::make_unique<DeviceFilter>(config_.vendor_ids);
    remapper_  = std::make_unique<Remapper>(config_);
    raw_input_ = std::make_unique<RawInput>(*filter_, *remapper_);
    tray_      = std::make_unique<Tray>(*this);

    if (!raw_input_->Register(hwnd)) {
        MessageBoxW(hwnd,
                    L"Falha ao registrar Raw Input. O app não pode prosseguir.",
                    L"MagicRemap", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!tray_->Create(hwnd)) {
        return false;
    }
    tray_->UpdateTooltip(remapper_->IsEnabled());

    // Sincroniza o estado de autostart com o config (não força mudança se o
    // usuário já configurou manualmente — só aplica se config diz e atualmente
    // não está ativo).
    if (config_.autostart && !autostart::IsEnabled()) {
        autostart::Set(true);
    }

    MR_LOG(L"App inicializado");
    return true;
}

void App::Shutdown() {
    if (remapper_) remapper_->ReleaseAllSynth();
    if (raw_input_) raw_input_->Unregister();
    if (tray_) tray_->Destroy();
    log::Shutdown();
}

void App::SetEnabled(bool v) {
    remapper_->SetEnabled(v);
    tray_->UpdateTooltip(v);
    if (!v) remapper_->ReleaseAllSynth();
    config_.enabled = v;
    Save(config_);
}

void App::ReloadConfig() {
    config_ = Load();
    remapper_->Reconfigure(config_);
    filter_->SetVendorIds(config_.vendor_ids);
    tray_->UpdateTooltip(remapper_->IsEnabled());
    MR_LOG(L"Config recarregada");
}

void App::OpenConfig() {
    ShellExecuteW(hwnd_, L"open", config_.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void App::ShowAbout() {
    MessageBoxW(hwnd_,
        L"MagicRemap 1.0.0\n\n"
        L"Remapeia o Apple Magic Keyboard no Windows.\n"
        L"Licença MIT.\n\n"
        L"Tecla de pânico: Shift+Ctrl+Alt+F12",
        L"Sobre", MB_OK | MB_ICONINFORMATION);
}

void App::RequestQuit() {
    PostMessageW(hwnd_, WM_CLOSE, 0, 0);
}

LRESULT App::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INPUT:
            raw_input_->HandleWmInput(lParam);
            if (remapper_->ConsumePanicRequested()) {
                MR_LOG(L"Panic — encerrando");
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;

        case WM_INPUT_DEVICE_CHANGE:
            filter_->InvalidateCache();
            return 0;

        case Tray::kCallbackMessage:
            tray_->OnTrayMessage(lParam);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_TRAY_ENABLED:
                    SetEnabled(!IsEnabled());
                    return 0;
                case IDM_TRAY_AUTOSTART: {
                    bool was = autostart::IsEnabled();
                    autostart::Set(!was);
                    config_.autostart = !was;
                    Save(config_);
                    return 0;
                }
                case IDM_TRAY_RELOAD:
                    ReloadConfig();
                    return 0;
                case IDM_TRAY_OPEN_CONFIG:
                    OpenConfig();
                    return 0;
                case IDM_TRAY_ABOUT:
                    ShowAbout();
                    return 0;
                case IDM_TRAY_QUIT:
                    RequestQuit();
                    return 0;
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            Shutdown();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace magic_remap
