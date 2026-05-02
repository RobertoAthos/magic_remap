#pragma once

#include <string>
#include <vector>

namespace magic_remap {

struct Config {
    // [General]
    bool enabled        = true;
    bool autostart      = false;
    bool swap_ctrl_too  = false;
    bool verbose        = false;

    // [Shortcuts]
    bool cmd_tab        = true;
    bool cmd_q          = true;
    bool cmd_space      = true;
    bool cmd_arrows     = true;
    bool option_arrows  = true;
    bool cmd_backspace  = true;
    bool option_backspace = true;

    // [Devices] — VIDs HID a tratar como Apple (lista de uint16 em hex).
    std::vector<unsigned short> vendor_ids = {0x05AC, 0x004C};

    // Caminho do INI (resolvido em Load()).
    std::wstring path;
};

// Caminho default: %APPDATA%\MagicRemap\config.ini
std::wstring DefaultConfigPath();

// Carrega de DefaultConfigPath(). Cria com defaults se não existir.
Config Load();

// Salva de volta para cfg.path.
void Save(const Config& cfg);

}  // namespace magic_remap
