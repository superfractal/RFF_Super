//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-22
//

#pragma once
#include "WorkspaceComboDrawing.hpp"

namespace merutilm::rff2::workspace {
    struct WorkspaceEditDrawing {
        using Context = WorkspaceComboDrawing::Context;
        static void border(HWND window, HDC dc, const Context &context) {
            RECT windowBounds;
            GetWindowRect(window, &windowBounds);
            OffsetRect(&windowBounds, -windowBounds.left, -windowBounds.top);
            const auto &theme = *context.theme;
            const bool focused = GetFocus() == window;
            RECT clientBounds;
            GetClientRect(window, &clientBounds);
            POINT clientOrigin{};
            ClientToScreen(window, &clientOrigin);
            RECT screenBounds;
            GetWindowRect(window, &screenBounds);
            OffsetRect(&clientBounds, clientOrigin.x - screenBounds.left, clientOrigin.y - screenBounds.top);
            const int savedDc = SaveDC(dc);
            if (savedDc == 0) {
                return;
            }
            ExcludeClipRect(dc, clientBounds.left, clientBounds.top, clientBounds.right, clientBounds.bottom);
            PanelDrawing::fill(dc, windowBounds, theme.field);
            RestoreDC(dc, savedDc);
            HBRUSH borderBrush = CreateSolidBrush(GetPropW(window, L"RFF.Form.Invalid") ? theme.error
                                                  : focused                             ? theme.accent
                                                                                        : theme.track);
            FrameRect(dc, &windowBounds, borderBrush);
            DeleteObject(borderBrush);
            InflateRect(&windowBounds, -1, -1);
            borderBrush = CreateSolidBrush(focused ? theme.accent : theme.field);
            FrameRect(dc, &windowBounds, borderBrush);
            DeleteObject(borderBrush);
        }
        static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR subclassId, DWORD_PTR contextData) {
            const auto &context = *reinterpret_cast<const Context *>(contextData);
            if (message == WM_NCCALCSIZE) {
                DefSubclassProc(window, message, wParam, lParam);
                RECT &clientBounds = wParam ? reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam)->rgrc[0]
                                            : *reinterpret_cast<RECT *>(lParam);
                HDC dc = GetDC(window);
                const auto previousFont = SelectObject(dc, context.font);
                TEXTMETRICW metrics{};
                GetTextMetricsW(dc, &metrics);
                SelectObject(dc, previousFont);
                ReleaseDC(window, dc);
                const int height =
                    std::max(0, std::min(int(clientBounds.bottom - clientBounds.top), int(metrics.tmHeight)));
                clientBounds.top += (clientBounds.bottom - clientBounds.top - height) / 2;
                clientBounds.bottom = clientBounds.top + height;
                return 0;
            }
            if (message == WM_NCPAINT) {
                if (HDC dc = GetWindowDC(window)) {
                    border(window, dc, context);
                    ReleaseDC(window, dc);
                }
                return 0;
            }
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            if (message == WM_NCHITTEST && result == HTBORDER) {
                return HTCLIENT;
            }
            if (message == WM_SETFONT) {
                const int inset = int(6 * context.scale + .5f);
                SendMessageW(window, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(inset, inset));
                SetWindowPos(window, nullptr, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            }
            if (message == WM_PRINT && (lParam & PRF_NONCLIENT)) {
                border(window, reinterpret_cast<HDC>(wParam), context);
            }
            if (message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_ENABLE) {
                RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(window, procedure, subclassId);
            }
            return result;
        }
        static void attach(HWND window, const Context &context) {
            SetWindowSubclass(window, procedure, 5, reinterpret_cast<DWORD_PTR>(&context));
            SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(context.font), FALSE);
        }
    };
} // namespace merutilm::rff2::workspace
