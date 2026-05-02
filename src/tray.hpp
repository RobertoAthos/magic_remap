#pragma once

#include <windows.h>
#include <shellapi.h>

namespace magic_remap {

class App;

class Tray {
public:
    explicit Tray(App& app);

    bool Create(HWND hwnd);
    void Destroy();

    void UpdateTooltip(bool enabled);

    // Mensagem custom para callback do tray.
    static constexpr UINT kCallbackMessage = WM_APP + 1;

    // Trata WM_TRAYICON (chamada do WndProc).
    void OnTrayMessage(LPARAM lParam);

private:
    void ShowMenu();

    App& app_;
    HWND hwnd_ = nullptr;
    NOTIFYICONDATAW nid_{};
};

}  // namespace magic_remap
