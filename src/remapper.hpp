#pragma once

#include <windows.h>

#include <array>

#include "config.hpp"

namespace magic_remap {

class Remapper {
public:
    enum class Action {
        PassThrough,  // hook deixa a tecla original seguir
        Suppress,     // hook bloqueia a tecla original (e nós já injetamos o que queríamos)
    };

    explicit Remapper(const Config& cfg);

    void Reconfigure(const Config& cfg);
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled_; }

    // Recebe um evento bruto vindo do hook. Pode chamar SendInput para
    // sintetizar a forma remapeada. Retorna Suppress se a tecla original
    // deve ser bloqueada, ou PassThrough se deve seguir o caminho normal.
    //   vk:        Virtual-Key
    //   scan_code: scancode
    //   is_key_up: tecla foi solta?
    //   is_e0:     tecla estendida?
    //   is_apple:  origem é o Magic Keyboard?
    Action Process(USHORT vk, USHORT scan_code, bool is_key_up, bool is_e0, bool is_apple);

    // True se a combinação de pânico (Shift+Ctrl+Alt+F12) foi detectada.
    // Limpa a flag.
    bool ConsumePanicRequested();

    // Garante que toda modificadora sintetizada seja liberada (chamar no shutdown).
    void ReleaseAllSynth();

private:
    struct DesiredMods { bool ctrl, alt, shift, win; };

    // Estados de "dead key" no estilo macOS US: Option+E arma agudo, próxima
    // vogal recebe acento; Option+I=circumflexo; Option+N=til; Option+U=trema;
    // Option+`=grave. Option+C insere ç direto (sem ser dead key).
    enum class PendingDead { None, Acute, Grave, Circumflex, Tilde, Diaeresis };

    void SyncMods(DesiredMods desired);
    void SendKey(USHORT vk, USHORT scan, bool key_up, bool extended);
    void SendVk(USHORT vk, bool key_up);  // helper que resolve scancode
    void SendUnicode(wchar_t ch);          // injeta um codepoint Unicode

    void HandleCmdCombo(USHORT vk, USHORT scan, bool key_up, bool e0);
    void HandleOptCombo(USHORT vk, USHORT scan, bool key_up, bool e0);

    // Helpers de dead-key.
    static wchar_t ComposeDead(PendingDead d, USHORT vk, bool shift);
    static wchar_t DeadCharAlone(PendingDead d);

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

    PendingDead pending_dead_ = PendingDead::None;
};

}  // namespace magic_remap
