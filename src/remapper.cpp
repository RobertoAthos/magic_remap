#include "remapper.hpp"

#include <windows.h>

#include "logging.hpp"

namespace magic_remap {

namespace {

bool IsExtendedScan(USHORT vk) {
    // Lista das VKs que sempre são "extended" (E0).
    switch (vk) {
        case VK_LWIN:  case VK_RWIN:
        case VK_RMENU: case VK_RCONTROL:
        case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
        case VK_PRIOR:  case VK_NEXT:
        case VK_LEFT:   case VK_RIGHT: case VK_UP: case VK_DOWN:
        case VK_DIVIDE: case VK_NUMLOCK:
            return true;
        default:
            return false;
    }
}

USHORT ScanFor(USHORT vk) {
    return static_cast<USHORT>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
}

}  // namespace

Remapper::Remapper(const Config& cfg) : cfg_(cfg), enabled_(cfg.enabled) {}

void Remapper::Reconfigure(const Config& cfg) {
    cfg_ = cfg;
    enabled_ = cfg.enabled;
}

void Remapper::SetEnabled(bool enabled) { enabled_ = enabled; }

bool Remapper::ConsumePanicRequested() {
    bool v = panic_requested_;
    panic_requested_ = false;
    return v;
}

void Remapper::ReleaseAllSynth() {
    SyncMods({false, false, false, false});
}

void Remapper::SendKey(USHORT vk, USHORT scan, bool key_up, bool extended) {
    INPUT inp{};
    inp.type = INPUT_KEYBOARD;
    inp.ki.wVk = vk;
    inp.ki.wScan = scan;
    inp.ki.dwFlags = (key_up ? KEYEVENTF_KEYUP : 0) | (extended ? KEYEVENTF_EXTENDEDKEY : 0);
    SendInput(1, &inp, sizeof(INPUT));
}

void Remapper::SendVk(USHORT vk, bool key_up) {
    SendKey(vk, ScanFor(vk), key_up, IsExtendedScan(vk));
}

void Remapper::SyncMods(DesiredMods d) {
    auto adjust = [&](bool& cur, bool want, USHORT vk) {
        if (cur == want) return;
        SendVk(vk, /*key_up=*/cur);  // se cur=true, queremos liberar
        cur = want;
    };
    // Ordem: liberar primeiro o que vai sair, depois pressionar o que entra.
    // Isso evita combinações estranhas no meio (ex.: Ctrl+Alt brevemente).
    if (synth_shift_ && !d.shift) { SendVk(VK_LSHIFT, true); synth_shift_ = false; }
    if (synth_ctrl_  && !d.ctrl)  { SendVk(VK_LCONTROL, true); synth_ctrl_ = false; }
    if (synth_alt_   && !d.alt)   { SendVk(VK_LMENU, true); synth_alt_ = false; }
    if (synth_win_   && !d.win)   { SendVk(VK_LWIN, true); synth_win_ = false; }

    if (!synth_shift_ && d.shift) { SendVk(VK_LSHIFT, false); synth_shift_ = true; }
    if (!synth_ctrl_  && d.ctrl)  { SendVk(VK_LCONTROL, false); synth_ctrl_ = true; }
    if (!synth_alt_   && d.alt)   { SendVk(VK_LMENU, false); synth_alt_ = true; }
    if (!synth_win_   && d.win)   { SendVk(VK_LWIN, false); synth_win_ = true; }
}

void Remapper::Process(USHORT vk, USHORT scan, bool up, bool e0, bool is_apple) {
    // Pânico: Shift+Ctrl+Alt+F12 (em qualquer teclado) — verifica antes de qualquer remap.
    if (vk == VK_F12 && !up) {
        bool shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
        bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
        if (shift && ctrl && alt) {
            panic_requested_ = true;
            MR_LOG(L"PANIC combo detected — releasing all and quitting");
            return;
        }
        // Também aceitar combo Apple-only mesmo sem GetAsyncKeyState refletir Cmd/Opt:
        if (apple_shift_held_ && apple_ctrl_held_ &&
            (apple_opt_held_ || apple_cmd_held_)) {
            panic_requested_ = true;
            return;
        }
    }

    // Eventos de teclados não-Apple ou app desabilitado: passthrough verbatim.
    if (!is_apple || !enabled_) {
        SendKey(vk, scan, up, e0);
        return;
    }

    // ---- Apple: estado físico ----
    if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SHIFT) {
        apple_shift_held_ = !up;
        SendKey(vk, scan, up, e0);
        return;
    }
    if (vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL) {
        apple_ctrl_held_ = !up;
        if (cfg_.swap_ctrl_too) {
            // Ctrl→Win
            USHORT remap = (vk == VK_RCONTROL) ? VK_RWIN : VK_LWIN;
            SendKey(remap, ScanFor(remap), up, true);
        } else {
            SendKey(vk, scan, up, e0);
        }
        return;
    }
    if (vk == VK_LWIN || vk == VK_RWIN) {
        // Apple Cmd: lazy — não emite nada.
        if (up) {
            apple_cmd_held_ = false;
            // Liberar synth mods que estavam servindo Cmd.
            SyncMods({false, false, apple_shift_held_ && synth_shift_, false});
            // Note: synth_shift_ só é true se em algum momento tivermos sintetizado
            // Shift; mas Shift de Apple é passthrough, então normalmente isso é false.
        } else {
            apple_cmd_held_ = true;
        }
        return;
    }
    if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_MENU) {
        // Apple Option (= Alt físico): lazy — não emite Alt até justificar.
        if (up) {
            apple_opt_held_ = false;
            SyncMods({false, false, false, false});
        } else {
            apple_opt_held_ = true;
        }
        return;
    }

    // Não-modificador.
    if (apple_cmd_held_) {
        HandleCmdCombo(vk, scan, up, e0);
        return;
    }
    if (apple_opt_held_) {
        HandleOptCombo(vk, scan, up, e0);
        return;
    }

    // Sem modificador especial Apple: passthrough.
    SyncMods({false, false, false, false});
    SendKey(vk, scan, up, e0);
}

void Remapper::HandleCmdCombo(USHORT vk, USHORT scan, bool up, bool e0) {
    // Liberação: olha o que mapeamos no down, replica o release.
    if (up) {
        USHORT remapped = active_remap_[vk & 0xFF];
        active_remap_[vk & 0xFF] = 0;
        if (remapped == 0) return;
        if (remapped == kSuppress) return;  // combo atômico, release suprimido
        SendKey(remapped, ScanFor(remapped), true, IsExtendedScan(remapped));
        return;
    }

    auto track = [&](USHORT emitted) { active_remap_[vk & 0xFF] = emitted; };

    switch (vk) {
        case VK_TAB:
            if (cfg_.cmd_tab) {
                SyncMods({false, true, apple_shift_held_, false});  // Alt[+Shift]
                SendVk(VK_TAB, false);
                track(VK_TAB);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk(VK_TAB, false);
                track(VK_TAB);
            }
            return;

        case 'Q':
            if (cfg_.cmd_q) {
                SyncMods({false, true, false, false});  // Alt+F4 (atômico)
                SendVk(VK_F4, false);
                SendVk(VK_F4, true);
                track(kSuppress);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk('Q', false);
                track('Q');
            }
            return;

        case VK_SPACE:
            if (cfg_.cmd_space) {
                // Tap Win key alone (abre Iniciar/Search).
                SyncMods({false, false, false, false});
                SendVk(VK_LWIN, false);
                SendVk(VK_LWIN, true);
                track(kSuppress);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk(VK_SPACE, false);
                track(VK_SPACE);
            }
            return;

        case VK_LEFT:
            if (cfg_.cmd_arrows) {
                SyncMods({false, false, apple_shift_held_, false});
                SendVk(VK_HOME, false);
                track(VK_HOME);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk(VK_LEFT, false);
                track(VK_LEFT);
            }
            return;

        case VK_RIGHT:
            if (cfg_.cmd_arrows) {
                SyncMods({false, false, apple_shift_held_, false});
                SendVk(VK_END, false);
                track(VK_END);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk(VK_RIGHT, false);
                track(VK_RIGHT);
            }
            return;

        case VK_UP:
            if (cfg_.cmd_arrows) {
                SyncMods({true, false, apple_shift_held_, false});  // Ctrl+Home
                SendVk(VK_HOME, false);
                track(VK_HOME);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk(VK_UP, false);
                track(VK_UP);
            }
            return;

        case VK_DOWN:
            if (cfg_.cmd_arrows) {
                SyncMods({true, false, apple_shift_held_, false});  // Ctrl+End
                SendVk(VK_END, false);
                track(VK_END);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk(VK_DOWN, false);
                track(VK_DOWN);
            }
            return;

        case VK_BACK:
            if (cfg_.cmd_backspace) {
                // Selecionar até o início da linha (Shift+Home) e Delete (atômico).
                SyncMods({false, false, true, false});
                SendVk(VK_HOME, false);
                SendVk(VK_HOME, true);
                SyncMods({false, false, false, false});
                SendVk(VK_DELETE, false);
                SendVk(VK_DELETE, true);
                track(kSuppress);
            } else {
                SyncMods({true, false, apple_shift_held_, false});
                SendVk(VK_BACK, false);
                track(VK_BACK);
            }
            return;

        default:
            // Default: Cmd→Ctrl
            SyncMods({true, false, apple_shift_held_, false});
            SendKey(vk, scan, false, e0);
            track(vk);
            return;
    }
}

void Remapper::HandleOptCombo(USHORT vk, USHORT scan, bool up, bool e0) {
    if (up) {
        USHORT remapped = active_remap_[vk & 0xFF];
        active_remap_[vk & 0xFF] = 0;
        if (remapped == 0) return;
        if (remapped == kSuppress) return;
        SendKey(remapped, ScanFor(remapped), true, IsExtendedScan(remapped));
        return;
    }

    auto track = [&](USHORT emitted) { active_remap_[vk & 0xFF] = emitted; };

    switch (vk) {
        case VK_LEFT:
        case VK_RIGHT:
            if (cfg_.option_arrows) {
                SyncMods({true, false, apple_shift_held_, false});  // Ctrl+arrow
                SendKey(vk, scan, false, e0);
                track(vk);
                return;
            }
            break;

        case VK_BACK:
            if (cfg_.option_backspace) {
                SyncMods({true, false, apple_shift_held_, false});  // Ctrl+Backspace
                SendVk(VK_BACK, false);
                track(VK_BACK);
                return;
            }
            break;

        default:
            break;
    }

    // Fallback: passthrough como Alt+key (comportamento default da Apple Option).
    SyncMods({false, true, apple_shift_held_, false});
    SendKey(vk, scan, false, e0);
    track(vk);
}

}  // namespace magic_remap
