#ifndef _UI_CONSTANTS_HPP
#define _UI_CONSTANTS_HPP

#pragma once

#include <cstdint>

namespace ui {
    static inline constexpr int UI_WIDTH = 900;
    static inline constexpr int UI_HEIGHT = 600;
    static inline constexpr int INPUT_TEXT_LIMIT = 100'000; // UTF-16 code units.

    static inline HWND g_hwndInput = NULL;
    static inline HWND g_hwndMode = NULL;
    static inline HWND g_hwndSubmit = NULL;
    static inline HWND g_hwndClear = NULL;
    static inline HWND g_hwndProgress = NULL;
    static inline HWND g_hwndIntervalLabel = NULL;
    static inline HWND g_hwndIntervalInput = NULL;
    static inline HFONT g_hFont = NULL;

    static inline constexpr int64_t IDC_INPUT = 2001;
    static inline constexpr int64_t IDC_MODE = 2002;
    static inline constexpr int64_t IDC_SUBMIT = 2003;
    static inline constexpr int64_t IDC_CLEAR = 2004;
    static inline constexpr int64_t IDC_PROGRESS = 2005;
    static inline constexpr int64_t IDC_INTERVAL_LABEL = 2006;
    static inline constexpr int64_t IDC_INTERVAL_INPUT = 2007;
} // namespace ui

#endif // !_UI_CONSTANTS_HPP
