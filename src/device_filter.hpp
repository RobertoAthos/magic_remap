#pragma once

#include <windows.h>

#include <unordered_map>
#include <vector>

namespace magic_remap {

class DeviceFilter {
public:
    explicit DeviceFilter(std::vector<unsigned short> apple_vids);

    // True se hDevice corresponde a um teclado Apple (VID na whitelist).
    // hDevice pode ser nullptr (input vindo de "ninguém" — eventos sintetizados
    // ou de fontes sem identidade); nesse caso retorna false.
    bool IsApple(HANDLE hDevice);

    // Limpa o cache. Chamar em WM_INPUT_DEVICE_CHANGE.
    void InvalidateCache();

    void SetVendorIds(std::vector<unsigned short> vids);

private:
    std::vector<unsigned short> apple_vids_;
    std::unordered_map<HANDLE, bool> cache_;
};

}  // namespace magic_remap
