#include "remapper.hpp"

#include <windows.h>

#include "kb_hook.hpp"
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
    // Marca como nossa injeção: o hook ignora teclas com este dwExtraInfo.
    inp.ki.dwExtraInfo = KbHook::kInjectTag;
    SendInput(1, &inp, sizeof(INPUT));
}

void Remapper::SendVk(USHORT vk, bool key_up) {
    SendKey(vk, ScanFor(vk), key_up, IsExtendedScan(vk));
}

void Remapper::SendUnicode(wchar_t ch) {
    INPUT inp[2]{};
    inp[0].type = INPUT_KEYBOARD;
    inp[0].ki.wVk = 0;
    inp[0].ki.wScan = static_cast<WORD>(ch);
    inp[0].ki.dwFlags = KEYEVENTF_UNICODE;
    inp[0].ki.dwExtraInfo = KbHook::kInjectTag;
    inp[1].type = INPUT_KEYBOARD;
    inp[1].ki.wVk = 0;
    inp[1].ki.wScan = static_cast<WORD>(ch);
    inp[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    inp[1].ki.dwExtraInfo = KbHook::kInjectTag;
    SendInput(2, inp, sizeof(INPUT));
}

wchar_t Remapper::ComposeDead(PendingDead d, USHORT vk, bool shift) {
    if (vk == VK_SPACE) return DeadCharAlone(d);
    if (vk < 'A' || vk > 'Z') return 0;

    switch (d) {
        case PendingDead::Acute:
            switch (vk) {
                case 'A': return shift ? L'Á' : L'á';
                case 'E': return shift ? L'É' : L'é';
                case 'I': return shift ? L'Í' : L'í';
                case 'O': return shift ? L'Ó' : L'ó';
                case 'U': return shift ? L'Ú' : L'ú';
                case 'Y': return shift ? L'Ý' : L'ý';
                case 'C': return shift ? L'Ć' : L'ć';
            }
            break;
        case PendingDead::Grave:
            switch (vk) {
                case 'A': return shift ? L'À' : L'à';
                case 'E': return shift ? L'È' : L'è';
                case 'I': return shift ? L'Ì' : L'ì';
                case 'O': return shift ? L'Ò' : L'ò';
                case 'U': return shift ? L'Ù' : L'ù';
            }
            break;
        case PendingDead::Circumflex:
            switch (vk) {
                case 'A': return shift ? L'Â' : L'â';
                case 'E': return shift ? L'Ê' : L'ê';
                case 'I': return shift ? L'Î' : L'î';
                case 'O': return shift ? L'Ô' : L'ô';
                case 'U': return shift ? L'Û' : L'û';
            }
            break;
        case PendingDead::Tilde:
            switch (vk) {
                case 'A': return shift ? L'Ã' : L'ã';
                case 'N': return shift ? L'Ñ' : L'ñ';
                case 'O': return shift ? L'Õ' : L'õ';
            }
            break;
        case PendingDead::Diaeresis:
            switch (vk) {
                case 'A': return shift ? L'Ä' : L'ä';
                case 'E': return shift ? L'Ë' : L'ë';
                case 'I': return shift ? L'Ï' : L'ï';
                case 'O': return shift ? L'Ö' : L'ö';
                case 'U': return shift ? L'Ü' : L'ü';
                case 'Y': return shift ? L'Ÿ' : L'ÿ';
            }
            break;
        default:
            break;
    }
    return 0;
}

wchar_t Remapper::DeadCharAlone(PendingDead d) {
    switch (d) {
        case PendingDead::Acute:      return L'´';  // ´
        case PendingDead::Grave:      return L'`';
        case PendingDead::Circumflex: return L'^';
        case PendingDead::Tilde:      return L'~';
        case PendingDead::Diaeresis:  return L'¨';  // ¨
        default:                       return 0;
    }
}

void Remapper::SyncMods(DesiredMods d) {
    // Ordem: liberar primeiro o que vai sair, depois pressionar o que entra.
    if (synth_shift_ && !d.shift) { SendVk(VK_LSHIFT, true); synth_shift_ = false; }
    if (synth_ctrl_  && !d.ctrl)  { SendVk(VK_LCONTROL, true); synth_ctrl_ = false; }
    if (synth_alt_   && !d.alt)   { SendVk(VK_LMENU, true); synth_alt_ = false; }
    if (synth_win_   && !d.win)   { SendVk(VK_LWIN, true); synth_win_ = false; }

    if (!synth_shift_ && d.shift) { SendVk(VK_LSHIFT, false); synth_shift_ = true; }
    if (!synth_ctrl_  && d.ctrl)  { SendVk(VK_LCONTROL, false); synth_ctrl_ = true; }
    if (!synth_alt_   && d.alt)   { SendVk(VK_LMENU, false); synth_alt_ = true; }
    if (!synth_win_   && d.win)   { SendVk(VK_LWIN, false); synth_win_ = true; }
}

Remapper::Action Remapper::Process(USHORT vk, USHORT scan, bool up, bool e0, bool is_apple) {
    // Pânico: Shift+Ctrl+Alt+F12 (em qualquer teclado).
    if (vk == VK_F12 && !up) {
        bool shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
        bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
        if (shift && ctrl && alt) {
            panic_requested_ = true;
            MR_LOG(L"PANIC combo detectado");
            return Action::Suppress;
        }
        if (apple_shift_held_ && apple_ctrl_held_ &&
            (apple_opt_held_ || apple_cmd_held_)) {
            panic_requested_ = true;
            return Action::Suppress;
        }
    }

    // Não-Apple ou app desabilitado: deixa o Windows entregar normalmente.
    if (!is_apple || !enabled_) {
        return Action::PassThrough;
    }

    // ---- Apple: estado físico de modificadoras ----
    if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SHIFT) {
        apple_shift_held_ = !up;
        return Action::PassThrough;  // Shift Apple = Shift normal
    }
    if (vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL) {
        apple_ctrl_held_ = !up;
        if (cfg_.swap_ctrl_too) {
            // Ctrl→Win: suprime Ctrl, injeta Win.
            USHORT remap = (vk == VK_RCONTROL) ? VK_RWIN : VK_LWIN;
            SendKey(remap, ScanFor(remap), up, true);
            return Action::Suppress;
        }
        return Action::PassThrough;
    }
    if (vk == VK_LWIN || vk == VK_RWIN) {
        // Apple Cmd: suprime sempre. Lógica do remap acontece em HandleCmdCombo.
        if (up) {
            apple_cmd_held_ = false;
            // Libera Ctrl/Alt sintetizados pelo combo. Mantém Shift sintético
            // somente se ainda for de interesse.
            SyncMods({false, false, apple_shift_held_ && synth_shift_, false});
        } else {
            apple_cmd_held_ = true;
        }
        return Action::Suppress;
    }
    if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_MENU) {
        // Apple Option (= Alt físico): suprime; vira Alt só quando o combo justifica.
        if (up) {
            apple_opt_held_ = false;
            SyncMods({false, false, false, false});
        } else {
            apple_opt_held_ = true;
        }
        return Action::Suppress;
    }

    // Não-modificador.
    if (apple_cmd_held_) {
        HandleCmdCombo(vk, scan, up, e0);
        return Action::Suppress;
    }
    if (apple_opt_held_) {
        HandleOptCombo(vk, scan, up, e0);
        return Action::Suppress;
    }

    // Up de tecla previamente "comida" pela composição dead-key.
    if (up && active_remap_[vk & 0xFF] == kSuppress) {
        active_remap_[vk & 0xFF] = 0;
        return Action::Suppress;
    }

    // Down com dead-key pendente: tenta compor a vogal acentuada.
    if (!up && pending_dead_ != PendingDead::None) {
        PendingDead pd = pending_dead_;
        pending_dead_ = PendingDead::None;
        wchar_t composed = ComposeDead(pd, vk, apple_shift_held_);
        if (composed != 0) {
            SendUnicode(composed);
            active_remap_[vk & 0xFF] = kSuppress;
            return Action::Suppress;
        }
        // Tecla não combina: emite o acento sozinho e deixa a letra passar normal.
        wchar_t alone = DeadCharAlone(pd);
        if (alone) SendUnicode(alone);
    }

    // Sem modificador especial Apple: passthrough verbatim.
    SyncMods({false, false, false, false});
    return Action::PassThrough;
}

void Remapper::HandleCmdCombo(USHORT vk, USHORT scan, bool up, bool e0) {
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

    // Mac US dead-key combos: Option+E/I/N/U/` arma o acento; Option+C insere ç direto.
    switch (vk) {
        case 'E':
            pending_dead_ = PendingDead::Acute;
            track(kSuppress);
            return;
        case 'I':
            pending_dead_ = PendingDead::Circumflex;
            track(kSuppress);
            return;
        case 'N':
            pending_dead_ = PendingDead::Tilde;
            track(kSuppress);
            return;
        case 'U':
            pending_dead_ = PendingDead::Diaeresis;
            track(kSuppress);
            return;
        case VK_OEM_3:  // ` em layout US
            pending_dead_ = PendingDead::Grave;
            track(kSuppress);
            return;
        case 'C':
            SendUnicode(apple_shift_held_ ? L'Ç' : L'ç');
            track(kSuppress);
            return;
    }

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
