#pragma once

#include <windows.h>

namespace magic_remap {

class DeviceFilter;
class Remapper;

class RawInput {
public:
    RawInput(DeviceFilter& filter, Remapper& remapper);

    // Registra Raw Input para teclados na janela hwnd com NOLEGACY.
    // Retorna false se a chamada do sistema falhar.
    bool Register(HWND hwnd);

    // Cancela registro (RIDEV_REMOVE).
    void Unregister();

    // Processa WM_INPUT. Lê o RAWKEYBOARD, consulta DeviceFilter e despacha
    // para Remapper::Process.
    void HandleWmInput(LPARAM lParam);

private:
    DeviceFilter& filter_;
    Remapper& remapper_;
    HWND hwnd_ = nullptr;
};

}  // namespace magic_remap
