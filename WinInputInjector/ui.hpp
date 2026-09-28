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
    struct StatusPresentation {
        InjectionStatus kind = InjectionStatus::Idle;
        std::wstring text = L"Ready";
        std::wstring tooltip = L"Focus the target input, then press F2. Sending to Windows does not verify the target text.";
        std::wstring detail;
        int percent = 0;
    };
    inline StatusPresentation g_status;
    inline std::wstring g_runningTooltip;
    inline constexpr wchar_t GAP_TOOLTIP[] = L"Gap between completed batches: 0-1000000 us. Default: 1000 us (1 ms).\n0 sends the full text in one batch and ignores Chunk.";
    inline constexpr wchar_t CHUNK_TOOLTIP[] = L"Characters per batch: 1-32768. Default: 128. A UTF-16 surrogate pair counts as one character.\nIgnored when Gap is 0.";
    inline void UpdateLayout(HWND hwnd);

    inline int Scale(int value) { return MulDiv(value, static_cast<int>(g_dpi), USER_DEFAULT_SCREEN_DPI); }

    inline int StatusHeight() {
        // Font line boxes do not scale to exact integer multiples at fractional DPI.
        const HDC dc = GetDC(g_hwndStatusDetail);
        const HGDIOBJ previous = SelectObject(dc, g_hStatusFont);
        TEXTMETRICW metrics{};
        const bool measured = GetTextMetricsW(dc, &metrics) != FALSE;
        SelectObject(dc, previous);
        ReleaseDC(g_hwndStatusDetail, dc);
        return UI_STATUS_LINES * (measured ? metrics.tmHeight : Scale(UI_STATUS_FONT_HEIGHT + UI_GAP));
    }

    inline void UpdateFonts() {
        const HFONT previous = g_hFont;
        const HFONT previous_input = g_hInputFont;
        const HFONT previous_status = g_hStatusFont;
        g_hFont = CreateFontW(-Scale(UI_FONT_HEIGHT), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        g_hInputFont = CreateFontW(-Scale(UI_FONT_HEIGHT), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH, L"Consolas");
        g_hStatusFont = CreateFontW(-Scale(UI_STATUS_FONT_HEIGHT), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        for (HWND control : {g_hwndTextLabel, g_hwndHint, g_hwndIntervalLabel,
            g_hwndIntervalInput, g_hwndChunkLabel, g_hwndChunkInput}) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), TRUE);
        }
        SendMessageW(g_hwndInput, WM_SETFONT, reinterpret_cast<WPARAM>(g_hInputFont), TRUE);
        for (HWND control : {g_hwndStatus, g_hwndStatusDetail, g_hwndTooltip})
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_hStatusFont), TRUE);
        SendMessageW(g_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, Scale(420));
        if (previous) DeleteObject(previous);
        if (previous_input) DeleteObject(previous_input);
        if (previous_status) DeleteObject(previous_status);
    }

    inline std::wstring GetInputText() {
        if (!g_hwndInput) return L"";
        int len = GetWindowTextLengthW(g_hwndInput);
        if (len == 0) return L"";
        std::wstring buf(len, L'\0');
        GetWindowTextW(g_hwndInput, &buf[0], len + 1);
        return buf;
    }

    inline int GetInterval() {
        if (!g_hwndIntervalInput) return DEFAULT_INTERVAL_US;
        wchar_t buf[32] = {0};
        GetWindowTextW(g_hwndIntervalInput, buf, 32);
        // wcstoll saturates on overflow, so oversized values still reach the clamp.
        return static_cast<int>(std::clamp(std::wcstoll(buf, nullptr, 10), 0LL, 1000000LL));
    }

    inline void MoveTabFocus(HWND hwnd, bool previous) {
        // Multiline EDIT requests all keys, so IsDialogMessage alone eats Tab.
        const HWND next = GetNextDlgTabItem(hwnd, GetFocus(), previous);
        if (!next) return;
        SetFocus(next);
        if (next == g_hwndIntervalInput || next == g_hwndChunkInput)
            SendMessageW(next, EM_SETSEL, 0, -1);
    }

    inline int NormalizeInterval() {
        const int interval = GetInterval();
        SetWindowTextW(g_hwndIntervalInput, std::to_wstring(interval).c_str());
        return interval;
    }

    inline void SetProgress(int percent) {
        if (!g_hwndProgress) return;
        percent = std::clamp(percent, 0, 100);
        SendMessageW(g_hwndProgress, PBM_SETPOS, percent, 0);
    }

    inline int NormalizeChunkSize() {
        if (!g_hwndChunkInput) return injection_settings::DEFAULT_CHUNK_SIZE;
        const int length = GetWindowTextLengthW(g_hwndChunkInput);
        std::wstring value(static_cast<size_t>(length) + 1, L'\0');
        value.resize(GetWindowTextW(g_hwndChunkInput, value.data(), length + 1));
        const int chunk_size = injection_settings::parse_chunk_size(value);
        SetWindowTextW(g_hwndChunkInput, std::to_wstring(chunk_size).c_str());
        return chunk_size;
    }

    inline void UpdateChunkEnabled() {
        const BOOL enabled = GetInterval() > 0;
        EnableWindow(g_hwndChunkInput, enabled);
        EnableWindow(g_hwndChunkLabel, enabled);
    }

    inline StatusPresentation ResultStatus(const InjectionReport& report, int percent) {
        StatusPresentation result;
        result.kind = report.status;
        result.percent = std::clamp(percent, 0, 100);
        const std::wstring position = std::to_wstring(report.completed_units) + L"/" +
            std::to_wstring(report.total_units) + L" UTF-16 units";
        const std::wstring suffix = L" \u00b7 " + std::to_wstring(result.percent) + L"%";
        switch (report.status) {
            case InjectionStatus::Running:
                result.text = L"Sending" + suffix;
                result.tooltip = g_runningTooltip;
                break;
            case InjectionStatus::Completed:
                result.text = report.total_units == 0 ? L"No text" : L"Sent to Windows";
                result.tooltip = report.total_units == 0 ? L"No text to send." :
                    L"Sent " + position + L" to Windows. Target text has not been verified.";
                break;
            case InjectionStatus::Cancelled:
                result.text = L"Cancelled" + suffix;
                result.tooltip = L"Cancelled after " + position + L". Target text has not been verified.";
                break;
            case InjectionStatus::Failed:
                result.text = L"Failed" + suffix;
                result.detail = L"Sent " + position + L"; last send " +
                    std::to_wstring(report.last_send.sent_events) + L"/" +
                    std::to_wstring(report.last_send.requested_events) + L" events.\n" +
                    (report.last_send.error_code ? L"Error " + std::to_wstring(report.last_send.error_code) + L"." :
                        L"Windows provided no error code.");
                result.tooltip = result.detail + L"\nStopped without retrying. Target text has not been verified.";
                break;
            default:
                break;
        }
        return result;
    }

    inline void SetStatus(HWND hwnd, const StatusPresentation& status) {
        const bool layout_changed = g_status.detail.empty() != status.detail.empty();
        if (g_status.tooltip != status.tooltip) SendMessageW(g_hwndTooltip, TTM_POP, 0, 0);
        if (g_status.text != status.text) SetWindowTextW(g_hwndStatus, status.text.c_str());
        if (g_status.detail != status.detail) SetWindowTextW(g_hwndStatusDetail, status.detail.c_str());
        g_status = status;
        SetProgress(status.percent);
        ShowWindow(g_hwndStatusDetail, status.detail.empty() ? SW_HIDE : SW_SHOWNA);
        InvalidateRect(g_hwndStatus, nullptr, TRUE);
        if (layout_changed) UpdateLayout(hwnd);
    }

    inline VOID CALLBACK ProgressTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD) {
        if (g_status.kind == InjectionStatus::Running)
            SetStatus(hwnd, ResultStatus({InjectionStatus::Running}, static_cast<int>(InjectThread::get_progress())));
    }

    inline VOID CALLBACK CheckReadyTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD) {
        if (InjectThread::check_ready()) {
            KillTimer(hwnd, 1);
            KillTimer(hwnd, 2);
            SetStatus(hwnd, ResultStatus(InjectThread::get_report(), static_cast<int>(InjectThread::get_progress())));
        }
    }

    inline void StartInjection(HWND hwnd) {
        if (InjectThread::check_ready()) {
            InjectThread::set_input_text(GetInputText());
            const int interval = NormalizeInterval();
            // A disabled chunk value is retained verbatim while zero interval ignores it.
            const int chunk_size = interval > 0 ? NormalizeChunkSize() : injection_settings::DEFAULT_CHUNK_SIZE;
            if (InjectThread::launch_injection(UNICODE_INPUT_MODE, interval, chunk_size)) {
                g_runningTooltip = interval == 0 ?
                    L"Sending in one batch (0 us); target may miss characters." :
                    L"Sending up to " + std::to_wstring(chunk_size) + L" characters per chunk; gap " +
                        std::to_wstring(interval) + L" us.";
                g_runningTooltip += L" Sending to Windows does not verify the target text.";
                SetStatus(hwnd, ResultStatus({InjectionStatus::Running}, 0));
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
            if (ev.vkey == VK_F2 && ev.down == 1 && InjectThread::check_ready(ev.received_at)) {
                StartInjection(hwnd);
                break;
            }
        }
    }

    inline void CreateTooltips(HWND hwnd) {
        g_hwndTooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, hwnd, nullptr, nullptr, nullptr);
        // Child tools cover enabled edits/status; parent rectangles also cover
        // static labels and the disabled Chunk edit, which cannot receive mouse input.
        for (HWND control : {g_hwndIntervalInput, g_hwndChunkInput, g_hwndStatus}) {
            TOOLINFOW tool{sizeof(tool)};
            tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            tool.hwnd = hwnd;
            tool.uId = reinterpret_cast<UINT_PTR>(control);
            tool.lpszText = LPSTR_TEXTCALLBACKW;
            SendMessageW(g_hwndTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        }
        for (UINT_PTR id : {1u, 2u}) {
            TOOLINFOW tool{sizeof(tool)};
            tool.uFlags = TTF_SUBCLASS;
            tool.hwnd = hwnd;
            tool.uId = id;
            tool.lpszText = LPSTR_TEXTCALLBACKW;
            SendMessageW(g_hwndTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        }
    }

    inline void UpdateTooltipRect(HWND hwnd, UINT_PTR id, HWND label, HWND input) {
        TOOLINFOW tool{sizeof(tool)};
        tool.hwnd = hwnd;
        tool.uId = id;
        RECT input_rect{};
        GetWindowRect(label, &tool.rect);
        GetWindowRect(input, &input_rect);
        tool.rect.right = input_rect.right;
        MapWindowPoints(HWND_DESKTOP, hwnd, reinterpret_cast<POINT*>(&tool.rect), 2);
        SendMessageW(g_hwndTooltip, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&tool));
    }

    inline void UpdateLayout(HWND hwnd) {
        RECT rc;
        if (!GetClientRect(hwnd, &rc) || IsIconic(hwnd)) return;
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        const int margin = Scale(UI_MARGIN);
        const int gap = Scale(UI_GAP);
        const int contentWidth = width - 2 * margin;
        const int controlHeight = Scale(UI_CONTROL_HEIGHT);
        const int numberWidth = Scale(UI_NUMBER_WIDTH);
        const int progressY = height - margin - Scale(UI_PROGRESS_HEIGHT);
        const int detailHeight = g_status.detail.empty() ? 0 : StatusHeight();
        const int detailY = progressY - gap - detailHeight;
        const int toolbarY = progressY - gap - controlHeight - (detailHeight ? detailHeight + gap : 0);
        const int inputY = margin + Scale(UI_HEADER_HEIGHT) + gap;

        const int hintOffset = Scale(UI_TEXT_LABEL_WIDTH) + gap;
        MoveWindow(g_hwndTextLabel, margin, margin, Scale(UI_TEXT_LABEL_WIDTH), Scale(UI_HEADER_HEIGHT), TRUE);
        MoveWindow(g_hwndHint, margin + hintOffset, margin, contentWidth - hintOffset, Scale(UI_HEADER_HEIGHT), TRUE);
        MoveWindow(g_hwndInput, margin, inputY, contentWidth, (std::max)(0, toolbarY - gap - inputY), TRUE);

        int x = margin;
        MoveWindow(g_hwndIntervalLabel, x, toolbarY, Scale(UI_GAP_LABEL_WIDTH), controlHeight, TRUE);
        x += Scale(UI_GAP_LABEL_WIDTH) + gap;
        MoveWindow(g_hwndIntervalInput, x, toolbarY, numberWidth, controlHeight, TRUE);
        x += numberWidth + gap;
        MoveWindow(g_hwndChunkLabel, x, toolbarY, Scale(UI_CHUNK_LABEL_WIDTH), controlHeight, TRUE);
        x += Scale(UI_CHUNK_LABEL_WIDTH) + gap;
        MoveWindow(g_hwndChunkInput, x, toolbarY, numberWidth, controlHeight, TRUE);
        x += numberWidth + gap;
        MoveWindow(g_hwndStatus, x, toolbarY, width - margin - x, controlHeight, TRUE);
        MoveWindow(g_hwndStatusDetail, margin, detailY, contentWidth, detailHeight, TRUE);
        MoveWindow(g_hwndProgress, margin, progressY, contentWidth, Scale(UI_PROGRESS_HEIGHT), TRUE);
        UpdateTooltipRect(hwnd, 1, g_hwndIntervalLabel, g_hwndIntervalInput);
        UpdateTooltipRect(hwnd, 2, g_hwndChunkLabel, g_hwndChunkInput);
        // Right-aligned and wrapped static text must repaint when shrinking too.
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
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
                lpMinMaxInfo->ptMinTrackSize.x = Scale(UI_MIN_WIDTH);
                lpMinMaxInfo->ptMinTrackSize.y = Scale(UI_MIN_HEIGHT);
                return 0;
            }

            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wParam;
                HWND hwndCtrl = (HWND)lParam;
                if (hwndCtrl == g_hwndIntervalLabel || hwndCtrl == g_hwndStatus || hwndCtrl == g_hwndChunkLabel ||
                    hwndCtrl == g_hwndTextLabel || hwndCtrl == g_hwndHint || hwndCtrl == g_hwndStatusDetail) {
                    SetBkMode(hdc, TRANSPARENT);
                    if (hwndCtrl == g_hwndStatus)
                        SetTextColor(hdc, GetSysColor(g_status.kind == InjectionStatus::Failed ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
                    return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
                }
                break;
            }

            case WM_CREATE: {
                g_dpi = GetDpiForWindow(hwnd);
                g_hwndTextLabel = CreateWindowExW(0, L"STATIC", L"Text \u00b7 Unicode", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_TEXT_LABEL, NULL, NULL);
                g_hwndHint = CreateWindowExW(0, L"STATIC", L"Focus target window, then press F2", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_RIGHT,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_HINT, NULL, NULL);
                g_hwndInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                    ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_INPUT, NULL, NULL);
                SendMessageW(g_hwndInput, EM_SETLIMITTEXT, INPUT_TEXT_LIMIT, 0);

                // Creation order defines Tab: text -> gap -> chunk -> text.
                g_hwndIntervalLabel = CreateWindowExW(0, L"STATIC", L"Gap (us)", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_INTERVAL_LABEL, NULL, NULL);
                g_hwndIntervalInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", std::to_wstring(DEFAULT_INTERVAL_US).c_str(),
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_INTERVAL_INPUT, NULL, NULL);
                g_hwndChunkLabel = CreateWindowExW(0, L"STATIC", L"Chunk", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_CHUNK_LABEL, NULL, NULL);
                g_hwndChunkInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", std::to_wstring(injection_settings::DEFAULT_CHUNK_SIZE).c_str(),
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_CHUNK_INPUT, NULL, NULL);
                SendMessageW(g_hwndChunkInput, EM_SETLIMITTEXT, 32, 0);
                g_status = {};
                g_hwndStatus = CreateWindowExW(0, L"STATIC", L"Ready", WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE | SS_NOTIFY,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_STATUS, NULL, NULL);
                g_hwndStatusDetail = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_LEFT | SS_NOPREFIX,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_STATUS_DETAIL, NULL, NULL);
                g_hwndProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                    0, 0, 0, 0, hwnd, (HMENU)IDC_PROGRESS, NULL, NULL);
                CreateTooltips(hwnd);
                UpdateFonts();
                UpdateChunkEnabled();
                SetTimer(hwnd, 3, 10, KeyboardTimerProc);

                return 0;
            }

            case WM_COMMAND: {
                int cmd = LOWORD(wParam);
                switch (cmd) {
                    case IDC_INTERVAL_INPUT: {
                        if (HIWORD(wParam) == EN_CHANGE) UpdateChunkEnabled();
                        if (HIWORD(wParam) == EN_KILLFOCUS) NormalizeInterval();
                        break;
                    }
                    case IDC_CHUNK_INPUT: {
                        if (HIWORD(wParam) == EN_KILLFOCUS) NormalizeChunkSize();
                        break;
                    }
                    default: {
                        break;
                    }
                }
                return 0;
            }

            case WM_NOTIFY: {
                const auto* header = reinterpret_cast<NMHDR*>(lParam);
                if (header->hwndFrom == g_hwndTooltip && header->code == TTN_GETDISPINFOW) {
                    auto* info = reinterpret_cast<NMTTDISPINFOW*>(lParam);
                    const UINT_PTR id = header->idFrom;
                    const wchar_t* text = g_status.tooltip.c_str();
                    if (id == 1 || id == reinterpret_cast<UINT_PTR>(g_hwndIntervalInput)) text = GAP_TOOLTIP;
                    if (id == 2 || id == reinterpret_cast<UINT_PTR>(g_hwndChunkInput)) text = CHUNK_TOOLTIP;
                    info->lpszText = const_cast<wchar_t*>(text);
                    return 0;
                }
                break;
            }

            case WM_ENTERSIZEMOVE: {
                inSizeMove = true;
                return 0;
            }

            case WM_DPICHANGED: {
                g_dpi = HIWORD(wParam);
                UpdateFonts();
                const RECT& suggested = *reinterpret_cast<const RECT*>(lParam);
                SetWindowPos(hwnd, nullptr, suggested.left, suggested.top,
                    suggested.right - suggested.left, suggested.bottom - suggested.top,
                    SWP_NOACTIVATE | SWP_NOZORDER);
                UpdateLayout(hwnd);
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
                if (g_hInputFont) DeleteObject(g_hInputFont);
                if (g_hStatusFont) DeleteObject(g_hStatusFont);
                g_hFont = g_hInputFont = g_hStatusFont = nullptr;
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

        g_dpi = GetDpiForSystem();
        HWND hwnd = CreateWindowEx(
            0,
            wndclass_main.lpszClassName,
            TEXT("WinInputInjector"),
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT,
            Scale(UI_WIDTH), Scale(UI_HEIGHT),
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
