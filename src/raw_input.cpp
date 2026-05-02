#include "raw_input.hpp"

#include <windows.h>

#include <vector>

#include "device_filter.hpp"
#include "logging.hpp"
#include "remapper.hpp"

namespace magic_remap {

RawInput::RawInput(DeviceFilter& filter, Remapper& remapper)
    : filter_(filter), remapper_(remapper) {}

bool RawInput::Register(HWND hwnd) {
    hwnd_ = hwnd;
    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01;  // Generic Desktop
    rid.usUsage     = 0x06;  // Keyboard
    // INPUTSINK: recebe mesmo sem foco. NOLEGACY: suprime entrega original ao sistema.
    // DEVNOTIFY: recebe WM_INPUT_DEVICE_CHANGE.
    // V2: NOLEGACY removido (saturava SIQ e congelava o sistema).
    // Suprimir/injetar agora é responsabilidade do WH_KEYBOARD_LL hook.
    // Raw Input fica apenas para identificar dispositivos (logging diagnóstico
    // e WM_INPUT_DEVICE_CHANGE para invalidar cache do filtro).
    rid.dwFlags    = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    rid.hwndTarget = hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        DWORD err = GetLastError();
        MR_LOGF(L"RegisterRawInputDevices failed err=%lu", err);
        return false;
    }
    MR_LOG(L"Raw Input registered (NOLEGACY)");
    return true;
}

void RawInput::Unregister() {
    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x06;
    rid.dwFlags     = RIDEV_REMOVE;
    rid.hwndTarget  = nullptr;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    MR_LOG(L"Raw Input unregistered");
}

void RawInput::HandleWmInput(LPARAM lParam) {
    UINT size = 0;
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size,
                        sizeof(RAWINPUTHEADER)) != 0) {
        return;
    }
    if (size == 0 || size > 1024) return;

    BYTE buffer[1024];
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer, &size,
                        sizeof(RAWINPUTHEADER)) != size) {
        return;
    }

    auto* raw = reinterpret_cast<RAWINPUT*>(buffer);
    if (raw->header.dwType != RIM_TYPEKEYBOARD) return;

    const RAWKEYBOARD& kb = raw->data.keyboard;

    // Microsoft envia uns "fake" key events (VK 0xFF) durante AltGr e similares — ignore.
    if (kb.VKey == 0xFF) return;

    USHORT vk    = kb.VKey;
    USHORT scan  = kb.MakeCode;
    bool   up    = (kb.Flags & RI_KEY_BREAK) != 0;
    bool   e0    = (kb.Flags & RI_KEY_E0) != 0;

    // Distinguir L/R para Shift, Ctrl, Alt usando scancode + E0:
    // - Shift: VK genérico VK_SHIFT, scan 0x2A=Left, 0x36=Right.
    // - Ctrl/Alt: distinção via E0.
    if (vk == VK_SHIFT) {
        vk = (scan == 0x36) ? VK_RSHIFT : VK_LSHIFT;
    } else if (vk == VK_CONTROL) {
        vk = e0 ? VK_RCONTROL : VK_LCONTROL;
    } else if (vk == VK_MENU) {
        vk = e0 ? VK_RMENU : VK_LMENU;
    }

    // V2: apenas atualiza/loga o dispositivo. O remapping é feito pelo hook.
    bool is_apple = filter_.IsApple(raw->header.hDevice);
    (void)vk; (void)scan; (void)up; (void)e0; (void)is_apple;
}

}  // namespace magic_remap
