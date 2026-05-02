#pragma once

#include <windows.h>

#include "remapper.hpp"

namespace magic_remap {

// Low-level keyboard hook (WH_KEYBOARD_LL). Substitui o RIDEV_NOLEGACY do
// Raw Input — pode suprimir teclas individualmente sem saturar a System
// Input Queue. Windows mata o hook automaticamente após timeout (LowLevelHooks-
// Timeout, default ~300ms), então um bug não congela o sistema permanentemente.
class KbHook {
public:
    // Tag mágico que marcamos em dwExtraInfo das nossas próprias injeções,
    // para o hook ignorá-las e não criar loop.
    static constexpr ULONG_PTR kInjectTag = 0x4D52454DULL;  // 'MREM'

    explicit KbHook(Remapper& remapper);
    ~KbHook();

    // Define a janela que receberá WM_CLOSE quando a tecla de pânico disparar.
    void SetPanicTarget(HWND hwnd) { panic_hwnd_ = hwnd; }

    bool Install();
    void Uninstall();

private:
    static LRESULT CALLBACK Proc(int nCode, WPARAM wParam, LPARAM lParam);

    Remapper& remapper_;
    HHOOK     hook_       = nullptr;
    HWND      panic_hwnd_ = nullptr;
};

}  // namespace magic_remap
