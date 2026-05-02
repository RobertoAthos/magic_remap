#include "kb_hook.hpp"

#include "logging.hpp"

namespace magic_remap {

namespace {
// O hook é global por processo. Guardamos a única instância aqui.
KbHook* g_instance = nullptr;
}  // namespace

KbHook::KbHook(Remapper& remapper) : remapper_(remapper) {}

KbHook::~KbHook() { Uninstall(); }

bool KbHook::Install() {
    if (g_instance) {
        MR_LOG(L"KbHook::Install — instância já existe");
        return false;
    }
    g_instance = this;
    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, Proc, GetModuleHandleW(nullptr), 0);
    if (!hook_) {
        DWORD err = GetLastError();
        MR_LOGF(L"SetWindowsHookExW(WH_KEYBOARD_LL) falhou err=%lu", err);
        g_instance = nullptr;
        return false;
    }
    MR_LOG(L"WH_KEYBOARD_LL hook instalado");
    return true;
}

void KbHook::Uninstall() {
    if (hook_) {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
        MR_LOG(L"WH_KEYBOARD_LL hook removido");
    }
    if (g_instance == this) g_instance = nullptr;
}

LRESULT CALLBACK KbHook::Proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION || !g_instance) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    auto* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

    // Ignora nossas próprias injeções para evitar loop.
    if (p->dwExtraInfo == kInjectTag) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    bool   up   = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
    bool   e0   = (p->flags & LLKHF_EXTENDED) != 0;
    USHORT vk   = static_cast<USHORT>(p->vkCode);
    USHORT scan = static_cast<USHORT>(p->scanCode);

    // V2: assume que toda tecla vem do Apple Magic Keyboard. Funciona perfeito
    // se o usuário só usa o MK. Para multi-teclado, futuro V2.1 correlaciona
    // com Raw Input por scancode/timing.
    Remapper::Action act =
        g_instance->remapper_.Process(vk, scan, up, e0, /*is_apple=*/true);

    // Tecla de pânico: se Process detectou, posta WM_CLOSE para a janela do app.
    if (g_instance->remapper_.ConsumePanicRequested() && g_instance->panic_hwnd_) {
        PostMessageW(g_instance->panic_hwnd_, WM_CLOSE, 0, 0);
    }

    if (act == Remapper::Action::Suppress) {
        return 1;  // suprime — não entrega ao próximo hook nem aos apps.
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

}  // namespace magic_remap
