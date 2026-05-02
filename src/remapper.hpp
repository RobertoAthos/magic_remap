#pragma once

#include <windows.h>

#include <array>

#include "config.hpp"

namespace magic_remap {

class Remapper {
public:
    explicit Remapper(const Config& cfg);

    void Reconfigure(const Config& cfg);
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled_; }

    // Recebe um evento bruto. Sintetiza os eventos resultantes via SendInput.
    //   vk:        Virtual-Key da Raw Input
    //   scan_code: scancode (RAWKEYBOARD::MakeCode)
    //   is_key_up: RI_KEY_BREAK
    //   is_e0:     RI_KEY_E0 (tecla estendida)
    //   is_apple:  origem é o Magic Keyboard?
    void Process(USHORT vk, USHORT scan_code, bool is_key_up, bool is_e0, bool is_apple);

    // True se a combinação de pânico (Shift+Ctrl+Alt+F12) foi detectada.
    // Limpa a flag.
    bool ConsumePanicRequested();

    // Garante que toda modificadora sintetizada seja liberada (chamar no shutdown).
    void ReleaseAllSynth();

private:
    struct DesiredMods { bool ctrl, alt, shift, win; };

    void SyncMods(DesiredMods desired);
    void SendKey(USHORT vk, USHORT scan, bool key_up, bool extended);
    void SendVk(USHORT vk, bool key_up);  // helper que resolve scancode

    void HandleCmdCombo(USHORT vk, USHORT scan, bool key_up, bool e0);
    void HandleOptCombo(USHORT vk, USHORT scan, bool key_up, bool e0);

    bool ShiftHeld() const { return apple_shift_held_; }

    Config cfg_;
    bool enabled_ = true;

    // Estado físico das teclas Apple
    bool apple_cmd_held_   = false;
    bool apple_opt_held_   = false;
    bool apple_shift_held_ = false;
    bool apple_ctrl_held_  = false;

    // Modificadoras que sintetizamos (e que devemos liberar depois)
    bool synth_ctrl_  = false;
    bool synth_alt_   = false;
    bool synth_shift_ = false;
    bool synth_win_   = false;

    // active_remap_[vk_original] = vk_emitido (ou kSuppress).
    // Usado para casar key-up com a forma remapeada que enviamos no key-down.
    static constexpr USHORT kSuppress = 0xFFFF;
    std::array<USHORT, 256> active_remap_{};

    bool panic_requested_ = false;
};

}  // namespace magic_remap
