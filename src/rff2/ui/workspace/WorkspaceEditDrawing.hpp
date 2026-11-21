//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-22
// Modified by Opus 5.5 on 2026-11-15
//

#pragma once
#include "WorkspaceComboDrawing.hpp"
#include <algorithm>
#include <string>

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
            underlineError(window, dc, clientBounds, windowBounds.bottom - 1, context);
        }
        // A form stores the characters an error is about as RFF.Form.ErrorStart and ErrorLength, each plus one.
        static void underlineError(HWND window, HDC dc, const RECT &client, int bottom, const Context &context) {
            const auto first = INT_PTR(GetPropW(window, L"RFF.Form.ErrorStart"));
            if (!first) {
                return;
            }
            const int length = GetWindowTextLengthW(window);
            const int start = std::min(int(first) - 1, std::max(0, length - 1));
            const int end = std::min(length, start + std::max(1, int(INT_PTR(GetPropW(window, L"RFF.Form.ErrorLength"))) - 1));
            if (length == 0) {
                return;
            }
            // EM_POSFROMCHAR has no answer past the last character, so the end there is the last one's right edge.
            const auto x = [&](int index) -> int {
                if (index < length) {
                    return int(short(LOWORD(SendMessageW(window, EM_POSFROMCHAR, index, 0))));
                }
                wchar_t last[2]{};
                std::wstring text(length + 1, L'\0');
                GetWindowTextW(window, text.data(), length + 1);
                last[0] = text[length - 1];
                const HDC measure = GetDC(window);
                const auto previous = SelectObject(measure, reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0)));
                SIZE size{};
                GetTextExtentPoint32W(measure, last, 1, &size);
                SelectObject(measure, previous);
                ReleaseDC(window, measure);
                return int(short(LOWORD(SendMessageW(window, EM_POSFROMCHAR, length - 1, 0)))) + size.cx;
            };
            const int left = std::max(int(client.left), int(client.left) + x(start));
            const int right = std::min(int(client.right), int(client.left) + x(end));
            const int top = client.bottom;
            const int thickness = std::min(std::max(1, int(2 * context.scale + .5f)), bottom - top);
            if (right <= left || thickness <= 0) {
                return;
            }
            RECT line{left, top, right, top + thickness};
            const HBRUSH brush = CreateSolidBrush(context.theme->error);
            FillRect(dc, &line, brush);
            DeleteObject(brush);
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
            // The text can scroll under an underline; the frame is redrawn after anything that may scroll it.
            if (GetPropW(window, L"RFF.Form.ErrorStart") &&
                (message == WM_KEYDOWN || message == WM_CHAR || message == WM_LBUTTONUP ||
                 (message == WM_MOUSEMOVE && (wParam & MK_LBUTTON)) || message == EM_SETSEL ||
                 message == EM_REPLACESEL || message == EM_SCROLLCARET || message == WM_SETTEXT)) {
                if (HDC dc = GetWindowDC(window)) {
                    border(window, dc, context);
                    ReleaseDC(window, dc);
                }
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
