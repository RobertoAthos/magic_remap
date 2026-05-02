#include "config.hpp"

#include <windows.h>
#include <shlobj.h>

#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <string>

namespace magic_remap {

namespace {

bool ReadBool(const wchar_t* section, const wchar_t* key, bool def, const wchar_t* path) {
    return GetPrivateProfileIntW(section, key, def ? 1 : 0, path) != 0;
}

void WriteBool(const wchar_t* section, const wchar_t* key, bool v, const wchar_t* path) {
    WritePrivateProfileStringW(section, key, v ? L"true" : L"false", path);
}

std::wstring ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* def,
                        const wchar_t* path) {
    wchar_t buf[1024];
    GetPrivateProfileStringW(section, key, def, buf, _countof(buf), path);
    return std::wstring(buf);
}

std::vector<unsigned short> ParseVidList(const std::wstring& csv) {
    std::vector<unsigned short> out;
    size_t i = 0;
    while (i < csv.size()) {
        // skip separators / whitespace
        while (i < csv.size() && (csv[i] == L',' || csv[i] == L' ' || csv[i] == L'\t')) ++i;
        if (i >= csv.size()) break;
        wchar_t* end = nullptr;
        unsigned long v = wcstoul(csv.c_str() + i, &end, 16);
        if (v != 0 && v <= 0xFFFF) {
            out.push_back(static_cast<unsigned short>(v));
        }
        if (!end || end == csv.c_str() + i) break;
        i = static_cast<size_t>(end - csv.c_str());
    }
    if (out.empty()) out.push_back(0x05AC);
    return out;
}

std::wstring FormatVidList(const std::vector<unsigned short>& vids) {
    std::wstring s;
    wchar_t buf[8];
    for (size_t i = 0; i < vids.size(); ++i) {
        if (i) s += L',';
        _snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%04X", vids[i]);
        s += buf;
    }
    return s;
}

void EnsureDirectory(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return;
    std::wstring dir = path.substr(0, pos);
    SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
}

void WriteDefaultsTo(const std::wstring& path) {
    Config def;
    def.path = path;
    EnsureDirectory(path);
    Save(def);
}

}  // namespace

std::wstring DefaultConfigPath() {
    PWSTR roaming = nullptr;
    std::wstring path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming))) {
        path = roaming;
        CoTaskMemFree(roaming);
    } else {
        path = L".";
    }
    path += L"\\MagicRemap\\config.ini";
    return path;
}

Config Load() {
    Config cfg;
    cfg.path = DefaultConfigPath();

    DWORD attr = GetFileAttributesW(cfg.path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        WriteDefaultsTo(cfg.path);
        return cfg;
    }

    const wchar_t* p = cfg.path.c_str();

    cfg.enabled       = ReadBool(L"General", L"enabled",       true,  p);
    cfg.autostart     = ReadBool(L"General", L"autostart",     false, p);
    cfg.swap_ctrl_too = ReadBool(L"General", L"swap_ctrl_too", false, p);
    cfg.verbose       = ReadBool(L"General", L"verbose",       false, p);

    cfg.cmd_tab          = ReadBool(L"Shortcuts", L"cmd_tab",          true, p);
    cfg.cmd_q            = ReadBool(L"Shortcuts", L"cmd_q",            true, p);
    cfg.cmd_space        = ReadBool(L"Shortcuts", L"cmd_space",        true, p);
    cfg.cmd_arrows       = ReadBool(L"Shortcuts", L"cmd_arrows",       true, p);
    cfg.option_arrows    = ReadBool(L"Shortcuts", L"option_arrows",    true, p);
    cfg.cmd_backspace    = ReadBool(L"Shortcuts", L"cmd_backspace",    true, p);
    cfg.option_backspace = ReadBool(L"Shortcuts", L"option_backspace", true, p);
    cfg.cmd_shift_screenshot = ReadBool(L"Shortcuts", L"cmd_shift_screenshot", true, p);

    std::wstring csv = ReadString(L"Devices", L"vendor_ids", L"05AC,004C", p);
    cfg.vendor_ids = ParseVidList(csv);

    return cfg;
}

void Save(const Config& cfg) {
    EnsureDirectory(cfg.path);
    const wchar_t* p = cfg.path.c_str();

    WriteBool(L"General", L"enabled",       cfg.enabled,       p);
    WriteBool(L"General", L"autostart",     cfg.autostart,     p);
    WriteBool(L"General", L"swap_ctrl_too", cfg.swap_ctrl_too, p);
    WriteBool(L"General", L"verbose",       cfg.verbose,       p);

    WriteBool(L"Shortcuts", L"cmd_tab",          cfg.cmd_tab,          p);
    WriteBool(L"Shortcuts", L"cmd_q",            cfg.cmd_q,            p);
    WriteBool(L"Shortcuts", L"cmd_space",        cfg.cmd_space,        p);
    WriteBool(L"Shortcuts", L"cmd_arrows",       cfg.cmd_arrows,       p);
    WriteBool(L"Shortcuts", L"option_arrows",    cfg.option_arrows,    p);
    WriteBool(L"Shortcuts", L"cmd_backspace",    cfg.cmd_backspace,    p);
    WriteBool(L"Shortcuts", L"option_backspace", cfg.option_backspace, p);
    WriteBool(L"Shortcuts", L"cmd_shift_screenshot", cfg.cmd_shift_screenshot, p);

    WritePrivateProfileStringW(L"Devices", L"vendor_ids",
                               FormatVidList(cfg.vendor_ids).c_str(), p);
}

}  // namespace magic_remap
