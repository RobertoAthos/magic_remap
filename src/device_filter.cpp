#include "device_filter.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdlib>
#include <cwchar>
#include <string>

#include "logging.hpp"

namespace magic_remap {

namespace {

// Caminho de device do Raw Input parece com:
//   \\?\HID#VID_05AC&PID_0267&MI_01#7&1234abcd&0&0000#{...}
// Extraímos o VID em hex.
unsigned short ExtractVid(const std::wstring& device_name) {
    auto pos = device_name.find(L"VID_");
    if (pos == std::wstring::npos) return 0;
    pos += 4;
    if (pos + 4 > device_name.size()) return 0;
    wchar_t hex[5] = {device_name[pos], device_name[pos + 1],
                      device_name[pos + 2], device_name[pos + 3], 0};
    return static_cast<unsigned short>(wcstoul(hex, nullptr, 16));
}

std::wstring GetRawInputDeviceName(HANDLE hDevice) {
    UINT size = 0;
    if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, nullptr, &size) != 0 || size == 0) {
        return {};
    }
    std::wstring name(size, L'\0');
    UINT got = GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, name.data(), &size);
    if (got == static_cast<UINT>(-1)) return {};
    // Trim trailing NULs
    while (!name.empty() && name.back() == L'\0') name.pop_back();
    return name;
}

}  // namespace

DeviceFilter::DeviceFilter(std::vector<unsigned short> apple_vids)
    : apple_vids_(std::move(apple_vids)) {}

void DeviceFilter::InvalidateCache() {
    cache_.clear();
}

void DeviceFilter::SetVendorIds(std::vector<unsigned short> vids) {
    apple_vids_ = std::move(vids);
    cache_.clear();
}

bool DeviceFilter::IsApple(HANDLE hDevice) {
    if (!hDevice) return false;

    auto it = cache_.find(hDevice);
    if (it != cache_.end()) return it->second;

    std::wstring name = GetRawInputDeviceName(hDevice);
    unsigned short vid = ExtractVid(name);
    bool is_apple = std::find(apple_vids_.begin(), apple_vids_.end(), vid) != apple_vids_.end();

    MR_LOGF(L"device hDevice=%p VID=%04X apple=%d name=%.80ls",
            hDevice, vid, is_apple ? 1 : 0, name.c_str());

    cache_.emplace(hDevice, is_apple);
    return is_apple;
}

}  // namespace magic_remap
