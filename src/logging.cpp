#include "logging.hpp"

#include <windows.h>
#include <shlobj.h>

#include <cstdio>
#include <mutex>
#include <string>

namespace magic_remap::log {

namespace {

bool g_verbose = false;
HANDLE g_file = INVALID_HANDLE_VALUE;
std::mutex g_mu;

constexpr LONGLONG kMaxLogBytes = 1 * 1024 * 1024;  // 1 MB

std::wstring AppDataDir() {
    PWSTR roaming = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming))) {
        std::wstring path(roaming);
        CoTaskMemFree(roaming);
        path += L"\\MagicRemap";
        CreateDirectoryW(path.c_str(), nullptr);
        return path;
    }
    return L".";
}

void RotateIfNeeded(const std::wstring& path) {
    LARGE_INTEGER size{};
    if (g_file == INVALID_HANDLE_VALUE) return;
    if (!GetFileSizeEx(g_file, &size) || size.QuadPart < kMaxLogBytes) return;

    CloseHandle(g_file);
    g_file = INVALID_HANDLE_VALUE;

    std::wstring backup = path + L".old";
    DeleteFileW(backup.c_str());
    MoveFileW(path.c_str(), backup.c_str());

    g_file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

}  // namespace

void Init(bool verbose) {
    g_verbose = verbose;
    if (!verbose) return;

    std::wstring path = AppDataDir() + L"\\magic_remap.log";
    g_file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

void Shutdown() {
    std::lock_guard lk(g_mu);
    if (g_file != INVALID_HANDLE_VALUE) {
        CloseHandle(g_file);
        g_file = INVALID_HANDLE_VALUE;
    }
}

void Write(std::wstring_view msg) {
    std::wstring line;
    line.reserve(msg.size() + 32);

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t ts[32];
    _snwprintf_s(ts, _countof(ts), _TRUNCATE, L"%02d:%02d:%02d.%03d ",
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    line.append(ts);
    line.append(msg);
    line.push_back(L'\n');

    OutputDebugStringW(line.c_str());

    if (!g_verbose) return;

    std::lock_guard lk(g_mu);
    if (g_file == INVALID_HANDLE_VALUE) return;

    // UTF-8 conversion for the file
    int n = WideCharToMultiByte(CP_UTF8, 0, line.data(), static_cast<int>(line.size()),
                                nullptr, 0, nullptr, nullptr);
    if (n <= 0) return;
    std::string utf8(static_cast<size_t>(n), 0);
    WideCharToMultiByte(CP_UTF8, 0, line.data(), static_cast<int>(line.size()),
                        utf8.data(), n, nullptr, nullptr);
    DWORD written = 0;
    WriteFile(g_file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);

    static int counter = 0;
    if (++counter % 64 == 0) {
        std::wstring path = AppDataDir() + L"\\magic_remap.log";
        RotateIfNeeded(path);
    }
}

}  // namespace magic_remap::log
