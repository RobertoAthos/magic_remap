#pragma once

#include <string>
#include <string_view>

namespace magic_remap::log {

void Init(bool verbose);
void Shutdown();

void Write(std::wstring_view msg);

template <typename... Args>
void Format(const wchar_t* fmt, Args... args) {
    wchar_t buf[1024];
    int n = _snwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args...);
    if (n > 0) Write(std::wstring_view(buf, static_cast<size_t>(n)));
}

}  // namespace magic_remap::log

#define MR_LOG(msg)        ::magic_remap::log::Write(L"[MR] " msg)
#define MR_LOGF(fmt, ...)  ::magic_remap::log::Format(L"[MR] " fmt, __VA_ARGS__)
