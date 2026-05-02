#include "autostart.hpp"

#include <windows.h>

#include <string>

namespace magic_remap::autostart {

namespace {

constexpr const wchar_t* kRunKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr const wchar_t* kValueName = L"MagicRemap";

std::wstring CurrentExePath() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return {};
    // Quote it so paths with spaces survive the registry parser.
    std::wstring quoted;
    quoted.reserve(n + 2);
    quoted.push_back(L'"');
    quoted.append(buf, n);
    quoted.push_back(L'"');
    return quoted;
}

}  // namespace

bool IsEnabled() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }
    wchar_t buf[1024];
    DWORD type = 0;
    DWORD size = sizeof(buf);
    LONG rc = RegQueryValueExW(key, kValueName, nullptr, &type,
                               reinterpret_cast<BYTE*>(buf), &size);
    RegCloseKey(key);
    if (rc != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return false;

    std::wstring expected = CurrentExePath();
    return _wcsicmp(buf, expected.c_str()) == 0;
}

bool Set(bool enable) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    LONG rc;
    if (enable) {
        std::wstring exe = CurrentExePath();
        rc = RegSetValueExW(key, kValueName, 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(exe.c_str()),
                            static_cast<DWORD>((exe.size() + 1) * sizeof(wchar_t)));
    } else {
        rc = RegDeleteValueW(key, kValueName);
        if (rc == ERROR_FILE_NOT_FOUND) rc = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}

}  // namespace magic_remap::autostart
