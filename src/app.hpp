#pragma once

#include <windows.h>

#include <memory>

#include "config.hpp"
#include "device_filter.hpp"
#include "raw_input.hpp"
#include "remapper.hpp"
#include "tray.hpp"

namespace magic_remap {

class App {
public:
    App();
    ~App();

    bool Init(HWND hwnd, HINSTANCE hInstance);
    void Shutdown();

    // Roteador de mensagens. Retorna o LRESULT a ser devolvido pelo WndProc;
    // se não tratou, retorna o resultado de DefWindowProc.
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Comandos do menu do tray.
    bool IsEnabled() const { return remapper_->IsEnabled(); }
    void SetEnabled(bool v);
    void ReloadConfig();
    void OpenConfig();
    void ShowAbout();
    void RequestQuit();

private:
    HWND      hwnd_ = nullptr;
    HINSTANCE hInst_ = nullptr;
    Config    config_;

    std::unique_ptr<DeviceFilter> filter_;
    std::unique_ptr<Remapper>     remapper_;
    std::unique_ptr<RawInput>     raw_input_;
    std::unique_ptr<Tray>         tray_;
};

}  // namespace magic_remap
