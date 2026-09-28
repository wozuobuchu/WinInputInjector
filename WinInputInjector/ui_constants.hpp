#ifndef _UI_CONSTANTS_HPP
#define _UI_CONSTANTS_HPP

#pragma once

#include <cstdint>

namespace ui {
    static inline constexpr int UI_WIDTH = 900;
    static inline constexpr int UI_HEIGHT = 600;
    // Layout dimensions are logical pixels at 96 DPI.
    static inline constexpr int UI_MIN_WIDTH = 720;
    static inline constexpr int UI_MIN_HEIGHT = 480;
    static inline constexpr int UI_MARGIN = 12;
    static inline constexpr int UI_GAP = 8;
    static inline constexpr int UI_FONT_HEIGHT = 16;
    static inline constexpr int UI_HEADER_HEIGHT = 24;
    static inline constexpr int UI_CONTROL_HEIGHT = 28;
    static inline constexpr int UI_NUMBER_WIDTH = 80;
    static inline constexpr int UI_STATUS_FONT_HEIGHT = 14;
    static inline constexpr int UI_TEXT_LABEL_WIDTH = 160;
    static inline constexpr int UI_GAP_LABEL_WIDTH = 64;
    static inline constexpr int UI_CHUNK_LABEL_WIDTH = 48;
    static inline constexpr int UI_STATUS_LINES = 2;
    static inline constexpr int UI_PROGRESS_HEIGHT = 6;
    static inline constexpr int UNICODE_INPUT_MODE = 0;
    static inline constexpr int INPUT_TEXT_LIMIT = 100'000; // UTF-16 code units.
    static inline constexpr int DEFAULT_INTERVAL_US = 1000;

    static inline HWND g_hwndInput = NULL;
    static inline HWND g_hwndTextLabel = NULL;
    static inline HWND g_hwndHint = NULL;
    static inline HWND g_hwndProgress = NULL;
    static inline HWND g_hwndIntervalLabel = NULL;
    static inline HWND g_hwndIntervalInput = NULL;
    static inline HWND g_hwndStatus = NULL;
    static inline HWND g_hwndStatusDetail = NULL;
    static inline HWND g_hwndTooltip = NULL;
    static inline HWND g_hwndChunkLabel = NULL;
    static inline HWND g_hwndChunkInput = NULL;
    static inline HFONT g_hFont = NULL;
    static inline HFONT g_hInputFont = NULL;
    static inline HFONT g_hStatusFont = NULL;
    static inline UINT g_dpi = USER_DEFAULT_SCREEN_DPI;

    static inline constexpr int64_t IDC_INPUT = 2001;
    static inline constexpr int64_t IDC_PROGRESS = 2005;
    static inline constexpr int64_t IDC_INTERVAL_LABEL = 2006;
    static inline constexpr int64_t IDC_INTERVAL_INPUT = 2007;
    static inline constexpr int64_t IDC_STATUS = 2008;
    static inline constexpr int64_t IDC_CHUNK_LABEL = 2009;
    static inline constexpr int64_t IDC_CHUNK_INPUT = 2010;
    static inline constexpr int64_t IDC_TEXT_LABEL = 2011;
    static inline constexpr int64_t IDC_HINT = 2012;
    static inline constexpr int64_t IDC_STATUS_DETAIL = 2013;
} // namespace ui

#endif // !_UI_CONSTANTS_HPP
