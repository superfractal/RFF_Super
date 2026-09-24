//
// Modified by GPT-6 on 2026-09-14, 2026-09-22
//

#pragma once
#include <algorithm>
#include <windows.h>

namespace merutilm::rff2 {
    struct UiDpi {
        static UINT forWindow(HWND window) {
            using QueryWindowDpi = UINT(WINAPI*)(HWND);
            static const auto query = reinterpret_cast<QueryWindowDpi>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
            if (query && window) {
                if (const UINT dpi = query(window)) {
                    return dpi;
                }
            }
            const HDC dc = GetDC(window);
            const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
            if (dc) {
                ReleaseDC(window, dc);
            }
            return dpi > 0 ? UINT(dpi) : 96;
        }
        static int pixels(int value, UINT dpi) {
            return MulDiv(value, int(dpi), 96);
        }
        static bool adjustWindowRect(RECT& rectangle, DWORD style, DWORD extendedStyle, UINT dpi) {
            using AdjustWindowForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
            static const auto adjust = reinterpret_cast<AdjustWindowForDpi>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi"));
            if (adjust) {
                return adjust(&rectangle, style, FALSE, extendedStyle, dpi) != FALSE;
            }
            return AdjustWindowRectEx(&rectangle, style, FALSE, extendedStyle) != FALSE;
        }
        static int metric(int index, UINT dpi) {
            using QueryMetricForDpi = int(WINAPI*)(int, UINT);
            static const auto query = reinterpret_cast<QueryMetricForDpi>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi"));
            if (query) {
                return query(index, dpi);
            }
            return MulDiv(GetSystemMetrics(index), int(dpi), int(forWindow(nullptr)));
        }
        class AwarenessScope {
            using SetContext = HANDLE(WINAPI*)(HANDLE);
            SetContext setContext = reinterpret_cast<SetContext>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
            HANDLE previous = nullptr;
        public:
            explicit AwarenessScope(bool perMonitor) {
                if (setContext) {
                    const HANDLE context = perMonitor ? DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
                                                      : DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
                    previous = setContext(context);
                }
            }
            ~AwarenessScope() {
                if (previous) {
                    setContext(previous);
                }
            }
            AwarenessScope(const AwarenessScope&) = delete;
            AwarenessScope& operator=(const AwarenessScope&) = delete;
        };
    };
}
