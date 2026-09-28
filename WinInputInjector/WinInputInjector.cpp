#include "framework.hpp"
#include "ui.hpp"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    rawinput::LowLatencyKeyboardLifetimeGuard keyboard_lifetime;
    if (!keyboard_lifetime.started()) {
        MessageBoxW(nullptr, L"Failed to initialize keyboard input.", L"WinInputInjector", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = ui::register_main_ui(hInstance);

    if (shared_data::sts_.stop_requested() || (hwnd == nullptr)) { shared_data::sts_.request_stop(); return 1; }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    SetFocus(ui::g_hwndInput);

    MSG msg{ 0 };
    while (GetMessage(&msg, NULL, 0, 0) && (!shared_data::sts_.stop_requested())) {
        // Use dialog navigation only for Tab; Enter remains an editor newline,
        // never a default-button action. F2 is handled by the global input listener.
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_TAB &&
            (msg.hwnd == hwnd || IsChild(hwnd, msg.hwnd))) {
            ui::MoveTabFocus(hwnd, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
