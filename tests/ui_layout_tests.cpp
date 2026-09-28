#include "../WinInputInjector/framework.hpp"
#include "../WinInputInjector/ui.hpp"
#include <iostream>
#include <stdexcept>

// Own an unshown native window; never inject into another application.
void require(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
RECT bounds(HWND window, HWND child) {
    RECT r{};
    GetWindowRect(child, &r);
    MapWindowPoints(HWND_DESKTOP, window, reinterpret_cast<POINT*>(&r), 2);
    return r;
}
std::wstring text_of(HWND control) {
    std::wstring text(GetWindowTextLengthW(control) + 1, L'\0');
    text.resize(GetWindowTextW(control, text.data(), static_cast<int>(text.size())));
    return text;
}
void check_text_fits(HWND control, HFONT font, bool multiline) {
    RECT available{};
    GetClientRect(control, &available);
    HDC dc = GetDC(control);
    const auto old = SelectObject(dc, font);
    RECT measured{0, 0, available.right, 0};
    const std::wstring text = text_of(control);
    DrawTextW(dc, text.c_str(), -1, &measured, DT_CALCRECT | DT_NOPREFIX |
        (multiline ? DT_WORDBREAK : DT_SINGLELINE));
    SelectObject(dc, old);
    ReleaseDC(control, dc);
    require(measured.right <= available.right && measured.bottom <= available.bottom, "text clipped");
}
void check_tooltip(HWND window, UINT_PTR id, const wchar_t* expected) {
    NMTTDISPINFOW info{};
    info.hdr = {ui::g_hwndTooltip, id, TTN_GETDISPINFOW};
    SendMessageW(window, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&info));
    require(info.lpszText && std::wstring(info.lpszText).find(expected) != std::wstring::npos, "tooltip content");
}
int run_checks() {
    using namespace ui;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HWND window = register_main_ui(GetModuleHandleW(nullptr));
    require(window != nullptr, "window creation");
    require(GetInputText().empty() && text_of(g_hwndStatus) == L"Ready", "initial state");
    require(!(GetWindowLongPtrW(window, GWL_EXSTYLE) & WS_EX_CLIENTEDGE), "main window edge");
    for (int removed : {2002, 2003, 2004}) require(!GetDlgItem(window, removed), "removed control exists");
    int tab_stops = 0;
    for (HWND child = GetWindow(window, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
        if (GetWindowLongPtrW(child, GWL_STYLE) & WS_TABSTOP) ++tab_stops;
    require(tab_stops == 3, "only text and two parameters are tab stops");
    require(SendMessageW(g_hwndTooltip, TTM_GETTOOLCOUNT, 0, 0) == 5, "tooltip registrations");
    check_tooltip(window, 1, L"0-1000000 us");
    check_tooltip(window, reinterpret_cast<UINT_PTR>(g_hwndIntervalInput), L"one batch");
    SetWindowTextW(g_hwndIntervalInput, L"0");
    require(!IsWindowEnabled(g_hwndChunkInput), "zero disables chunk");
    check_tooltip(window, 2, L"Ignored when Gap is 0");
    SetWindowTextW(g_hwndIntervalInput, L"99999999");
    require(NormalizeInterval() == 1000000, "gap clamp");
    SetWindowTextW(g_hwndIntervalInput, L"1000");
    SetWindowTextW(g_hwndChunkInput, L"99999");
    require(NormalizeChunkSize() == 32768, "chunk clamp");
    SetWindowTextW(g_hwndChunkInput, L"");
    require(NormalizeChunkSize() == 128, "chunk default");
    std::wstring long_text;
    for (int i = 0; i < 10000; ++i) long_text += L"中文Code\t\r\nX";
    SetWindowTextW(g_hwndInput, long_text.c_str());
    require(GetInputText() == long_text, "100000-unit source text");

    const auto running = ResultStatus({InjectionStatus::Running}, 42);
    const auto completed = ResultStatus({InjectionStatus::Completed, 100000, 100000}, 100);
    const auto empty = ResultStatus({InjectionStatus::Completed}, 100);
    const auto cancelled = ResultStatus({InjectionStatus::Cancelled, 42000, 100000}, 42);
    const auto failed = ResultStatus({InjectionStatus::Failed, 99999, 100000, {200000, 199999, MAXDWORD}}, 99);
    const auto no_error = ResultStatus({InjectionStatus::Failed, 99999, 100000, {200000, 199999, 0}}, 99);
    const auto zero = ResultStatus({InjectionStatus::Failed, 0, 100000, {200000, 0, 5}}, 0);
    require(running.text == L"Sending \u00b7 42%" && completed.text == L"Sent to Windows", "sending/completed captions");
    require(empty.text == L"No text" && cancelled.text == L"Cancelled \u00b7 42%", "empty/cancelled captions");
    require(failed.text == L"Failed \u00b7 99%" && failed.detail.find(L"4294967295") != std::wstring::npos, "failure code");
    require(no_error.detail.find(L"Windows provided no error code") != std::wstring::npos, "missing error code");
    require(zero.percent == 0 && zero.detail.find(L"0/200000 events") != std::wstring::npos, "zero-send report");

    for (UINT dpi : {96u, 120u, 144u, 192u}) {
        for (const POINT size : {POINT{720,480}, POINT{900,600}, POINT{1920,1080}, POINT{900,600}}) {
            RECT suggested{0, 0, MulDiv(size.x, dpi, 96), MulDiv(size.y, dpi, 96)};
            SendMessageW(window, WM_DPICHANGED, MAKELONG(dpi, dpi), reinterpret_cast<LPARAM>(&suggested));
            SetStatus(window, {});
            const int normal_bottom = bounds(window, g_hwndInput).bottom;
            const auto progress_rect = bounds(window, g_hwndProgress);
            for (const auto& state : {StatusPresentation{}, running, completed, empty, cancelled, failed, no_error, zero, running}) {
                SetStatus(window, state);
                RECT client{};
                GetClientRect(window, &client);
                for (HWND control : {g_hwndTextLabel, g_hwndHint, g_hwndInput, g_hwndIntervalLabel,
                    g_hwndIntervalInput, g_hwndChunkLabel, g_hwndChunkInput, g_hwndStatus, g_hwndProgress}) {
                    RECT r = bounds(window, control);
                    require(r.left >= 0 && r.top >= 0 && r.right <= client.right && r.bottom <= client.bottom, "control clipped");
                    require(r.right > r.left && r.bottom > r.top, "empty control");
                }
                RECT text = bounds(window, g_hwndInput), gap = bounds(window, g_hwndIntervalInput),
                    chunk = bounds(window, g_hwndChunkInput), status = bounds(window, g_hwndStatus),
                    progress = bounds(window, g_hwndProgress), detail = bounds(window, g_hwndStatusDetail);
                require(text.bottom + Scale(UI_GAP) == gap.top, "editor toolbar spacing");
                require(gap.top == chunk.top && chunk.top == status.top && chunk.right + Scale(UI_GAP) == status.left, "toolbar alignment");
                require(gap.right - gap.left == Scale(80) && chunk.right - chunk.left == Scale(80), "number widths");
                require(progress.bottom - progress.top == Scale(6) && progress.top == progress_rect.top, "anchored progress");
                const bool expanded = !state.detail.empty();
                require(((GetWindowLongPtrW(g_hwndStatusDetail, GWL_STYLE) & WS_VISIBLE) != 0) == expanded, "detail visibility");
                require(text.bottom == normal_bottom - (expanded ? StatusHeight() + Scale(UI_GAP) : 0), "expand/collapse editor height");
                if (expanded) {
                    require(detail.top == gap.bottom + Scale(UI_GAP) && detail.bottom + Scale(UI_GAP) == progress.top, "detail spacing");
                    check_text_fits(g_hwndStatusDetail, g_hStatusFont, true);
                }
                check_text_fits(g_hwndStatus, g_hStatusFont, false);
                check_text_fits(g_hwndHint, g_hFont, false);
                require(text_of(g_hwndStatus) == state.text, "status text persists across layout");
                check_tooltip(window, reinterpret_cast<UINT_PTR>(g_hwndStatus), state.tooltip.c_str());
            }
            LOGFONTW font{};
            GetObjectW(g_hFont, sizeof(font), &font);
            require(font.lfHeight == -Scale(16) && wcscmp(font.lfFaceName, L"Segoe UI") == 0, "UI font");
            GetObjectW(g_hInputFont, sizeof(font), &font);
            require(font.lfHeight == -Scale(16) && wcscmp(font.lfFaceName, L"Consolas") == 0, "editor font");
            GetObjectW(g_hStatusFont, sizeof(font), &font);
            require(font.lfHeight == -Scale(14) && wcscmp(font.lfFaceName, L"Segoe UI") == 0, "status font");
        }
    }
    DestroyWindow(window);
    std::cout << "PASS: four DPIs, window sizes, compact statuses, failure details, expand/collapse, tooltips, 100k text, parameter correction\n";
    return 0;
}
int main() {
    try { return run_checks(); }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
