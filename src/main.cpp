#include <windows.h>
#include <objbase.h>

#include <memory>

#include "app.hpp"
#include "logging.hpp"

namespace {

constexpr const wchar_t* kWindowClass = L"MagicRemap.Window";
constexpr const wchar_t* kMutexName   = L"Global\\MagicRemap.SingleInstance";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    auto* app = reinterpret_cast<magic_remap::App*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!app) return DefWindowProcW(hwnd, msg, wParam, lParam);
    return app->HandleMessage(hwnd, msg, wParam, lParam);
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // Single-instance.
    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    DWORD mutex_err = GetLastError();
    if (!mutex || mutex_err == ERROR_ALREADY_EXISTS) {
        if (mutex) CloseHandle(mutex);
        MessageBoxW(nullptr, L"MagicRemap já está rodando.", L"MagicRemap",
                    MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&wc)) {
        return 1;
    }

    auto app = std::make_unique<magic_remap::App>();

    HWND hwnd = CreateWindowExW(
        0, kWindowClass, L"MagicRemap",
        0, 0, 0, 0, 0,
        HWND_MESSAGE,  // message-only window
        nullptr, hInstance, app.get());
    if (!hwnd) {
        return 1;
    }

    if (!app->Init(hwnd, hInstance)) {
        DestroyWindow(hwnd);
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return static_cast<int>(msg.wParam);
}
