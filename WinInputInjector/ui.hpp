#ifndef _UI_HPP_
#define _UI_HPP_

#include "resource.hpp"
#include <Windows.h>
#include <CommCtrl.h>

#include <algorithm>
#include <cwchar>
#include <string>

#include "header.hpp"

#include "ui_constants.hpp"

#pragma comment(lib, "Comctl32.lib")

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace ui {
    inline std::wstring GetInputText() {
        if (!g_hwndInput) return L"";
        int len = GetWindowTextLengthW(g_hwndInput);
        if (len == 0) return L"";
        std::wstring buf(len, L'\0');
        GetWindowTextW(g_hwndInput, &buf[0], len + 1);
        return buf;
    }

    enum class InputMode : int {
        SendUnicodeInput = 0,
        SimulateKeyboard = 1,
    };
    inline int GetSelectedMode() {
        if (!g_hwndMode) return 0;
        return (int)SendMessageW(g_hwndMode, CB_GETCURSEL, 0, 0);
    }

    inline int GetInterval() {
        if (!g_hwndIntervalInput) return 0;
        wchar_t buf[32] = {0};
        GetWindowTextW(g_hwndIntervalInput, buf, 32);
        // wcstoll saturates on overflow, so oversized values still reach the clamp.
        return static_cast<int>(std::clamp(std::wcstoll(buf, nullptr, 10), 0LL, 1000000LL));
    }

    inline void SetProgress(int percent) {
        if (!g_hwndProgress) return;
        percent = std::clamp(percent, 0, 100);
        SendMessageW(g_hwndProgress, PBM_SETPOS, percent, 0);
    }

    inline VOID CALLBACK ProgressTimerProc(HWND, UINT, UINT_PTR, DWORD) {
        SetProgress(static_cast<int>(InjectThread::get_progress()));
    }

    inline VOID CALLBACK CheckReadyTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD) {
        if (InjectThread::check_ready()) {
            KillTimer(hwnd, 1);
            KillTimer(hwnd, 2);
            SetProgress(static_cast<int>(InjectThread::get_progress()));
            EnableWindow(g_hwndSubmit, TRUE);
        }
    }

    inline void SubmitInjection(HWND hwnd) {
        if (InjectThread::check_ready()) {
            InjectThread::set_input_text(GetInputText());
            if (InjectThread::launch_injection(GetSelectedMode(), GetInterval())) {
                EnableWindow(g_hwndSubmit, FALSE);
                SetTimer(hwnd, 1, 50, ProgressTimerProc);
                SetTimer(hwnd, 2, 200, CheckReadyTimerProc);
            }
        }
    }

    inline VOID CALLBACK KeyboardTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD) {
        static rawinput::LowLatencyKeyboard::KeyEvent ev_buffer[rawinput::LowLatencyKeyboard::kQueueCapacity];
        const size_t count = rawinput::LowLatencyKeyboard::pop_events(ev_buffer);
        for (size_t i = 0; i < count; ++i) {
            const auto& ev = ev_buffer[i];
            if (ev.vkey == VK_F2 && ev.down == 1) {
                SubmitInjection(hwnd);
                break;
            }
        }
    }

    inline void UpdateLayout(HWND hwnd) {
        RECT rc;
        if (!GetClientRect(hwnd, &rc) || IsIconic(hwnd)) return;
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        int margin = 20;
        int progressHeight = 25;
        int buttonHeight = 35;
        int controlGap = 10;

        MoveWindow(g_hwndProgress, margin, height - margin - progressHeight, width - 2 * margin, progressHeight, TRUE);

        int bottomRowY = height - margin - progressHeight - margin - buttonHeight;

        int comboWidth = 150;
        MoveWindow(g_hwndMode, margin, bottomRowY + (buttonHeight - 30) / 2, comboWidth, 200, TRUE);

        int labelWidth = 95;
        int intervalWidth = 70;
        int currentX = margin + comboWidth + controlGap;

        MoveWindow(g_hwndIntervalLabel, currentX, bottomRowY, labelWidth, buttonHeight, TRUE);
        currentX += labelWidth + (controlGap / 2);

        MoveWindow(g_hwndIntervalInput, currentX, bottomRowY + (buttonHeight - 30) / 2, intervalWidth, 30, TRUE);

        int buttonWidth = 100;
        MoveWindow(g_hwndSubmit, width - margin - buttonWidth * 2 - controlGap, bottomRowY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(g_hwndClear, width - margin - buttonWidth, bottomRowY, buttonWidth, buttonHeight, TRUE);

        int inputHeight = bottomRowY - margin - margin;
        if (inputHeight < 0) inputHeight = 0;
        MoveWindow(g_hwndInput, margin, margin, width - 2 * margin, inputHeight, TRUE);

        InvalidateRect(hwnd, NULL, TRUE);
    }

    LRESULT CALLBACK windowproc_main(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        static bool inSizeMove = false;

        switch (uMsg) {

            case WM_SYSCOMMAND: {
                if (wParam == SC_CLOSE) {
                    PostQuitMessage(0);
                    shared_data::sts_.request_stop();
                    return 0;
                }
                break;
            }

            case WM_GETMINMAXINFO: {
                LPMINMAXINFO lpMinMaxInfo = (LPMINMAXINFO)lParam;
                lpMinMaxInfo->ptMinTrackSize.x = 720;
                lpMinMaxInfo->ptMinTrackSize.y = 480;
                return 0;
            }

            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wParam;
                HWND hwndCtrl = (HWND)lParam;
                if (hwndCtrl == g_hwndIntervalLabel) {
                    SetBkMode(hdc, TRANSPARENT);
                    return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
                }
                break;
            }

            case WM_CREATE: {
                g_hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

                g_hwndInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Your text, press F2 to submit...", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd, (HMENU)IDC_INPUT, NULL, NULL);
                SendMessageW(g_hwndInput, EM_SETLIMITTEXT, INPUT_TEXT_LIMIT, 0);

                g_hwndMode = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hwnd, (HMENU)IDC_MODE, NULL, NULL);
                SendMessageW(g_hwndMode, CB_ADDSTRING, 0, (LPARAM)L"SendUnicodeInput");
                // SendMessageW(g_hwndMode, CB_ADDSTRING, 0, (LPARAM)L"SimulateKeyboard");
                SendMessageW(g_hwndMode, CB_SETCURSEL, 0, 0);

                g_hwndSubmit = CreateWindowExW(0, L"BUTTON", L"Submit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, (HMENU)IDC_SUBMIT, NULL, NULL);

                g_hwndClear = CreateWindowExW(0, L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, (HMENU)IDC_CLEAR, NULL, NULL);

                g_hwndProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 0, 0, 0, 0, hwnd, (HMENU)IDC_PROGRESS, NULL, NULL);

                g_hwndIntervalLabel = CreateWindowExW(0, L"STATIC", L"Interval (us):", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_RIGHT, 0, 0, 0, 0, hwnd, (HMENU)IDC_INTERVAL_LABEL, NULL, NULL);
                g_hwndIntervalInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd, (HMENU)IDC_INTERVAL_INPUT, NULL, NULL);

                SendMessageW(g_hwndInput, WM_SETFONT, (WPARAM)g_hFont, TRUE);
                SendMessageW(g_hwndMode, WM_SETFONT, (WPARAM)g_hFont, TRUE);
                SendMessageW(g_hwndSubmit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
                SendMessageW(g_hwndClear, WM_SETFONT, (WPARAM)g_hFont, TRUE);
                SendMessageW(g_hwndIntervalLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);
                SendMessageW(g_hwndIntervalInput, WM_SETFONT, (WPARAM)g_hFont, TRUE);

                SetTimer(hwnd, 3, 10, KeyboardTimerProc);

                return 0;
            }

            case WM_COMMAND: {
                int cmd = LOWORD(wParam);
                switch (cmd) {
                    case IDC_CLEAR: {
                        SetWindowTextW(g_hwndInput, L"");
                        break;
                    }

                    case IDC_SUBMIT: {
                        SubmitInjection(hwnd);
                        break;
                    }

                    default: {
                        break;
                    }
                }
                return 0;
            }

            case WM_ENTERSIZEMOVE: {
                inSizeMove = true;
                return 0;
            }

            case WM_EXITSIZEMOVE: {
                inSizeMove = false;
                UpdateLayout(hwnd);
                return 0;
            }

            case WM_SIZE: {
                // Creation, maximize and restore do not enter the sizing loop.
                if (wParam != SIZE_MINIMIZED && !inSizeMove) {
                    UpdateLayout(hwnd);
                }
                return 0;
            }

            case WM_DESTROY: {
                if (g_hFont) DeleteObject(g_hFont);
                PostQuitMessage(0);
                shared_data::sts_.request_stop();
                return 0;
            }

            default: {
                break;
            }
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    HWND register_main_ui(_In_ HINSTANCE hInstance) {
        (void)InjectThread::getInstance();

        INITCOMMONCONTROLSEX icex{sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&icex);

        WNDCLASSEX wndclass_main{};
        wndclass_main.cbSize = sizeof(WNDCLASSEX);
        wndclass_main.lpfnWndProc = windowproc_main;
        wndclass_main.hInstance = hInstance;
        wndclass_main.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WININPUTINJECTOR));
        wndclass_main.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass_main.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wndclass_main.lpszClassName = TEXT("MainUIWindowClass");
        wndclass_main.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

        if (!RegisterClassEx(&wndclass_main)) {
            shared_data::sts_.request_stop();
            return nullptr;
        }

        HWND hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            wndclass_main.lpszClassName,
            TEXT("WinInputInjector"),
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT,
            UI_WIDTH, UI_HEIGHT,
            NULL,
            NULL,
            hInstance,
            NULL
        );

        if (hwnd == NULL) {
            shared_data::sts_.request_stop();
            return nullptr;
        }

        return hwnd;
    }

} // namespace ui

#endif // !_UI_HPP_
