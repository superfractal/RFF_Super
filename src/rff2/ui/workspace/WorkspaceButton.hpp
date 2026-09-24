//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-19, 2026-09-22
//

#pragma once
#include "WorkspaceComboDrawing.hpp"
#include "PanelBackBuffer.hpp"

namespace merutilm::rff2::workspace {
    struct WorkspaceButton {
        using Context = WorkspaceComboDrawing::Context;
        static void draw(const DRAWITEMSTRUCT &target, const Context &context) {
            PanelBackBuffer buffer;
            auto item = target;
            const auto bufferDc = buffer.begin(target.hDC, target.rcItem.right, target.rcItem.bottom);
            if (bufferDc) {
                item.hDC = bufferDc;
            }
            const auto &theme = *context.theme;
            const bool disabled = (item.itemState & ODS_DISABLED) != 0;
            const bool pressed = (item.itemState & ODS_SELECTED) != 0;
            const bool focused = GetFocus() == item.hwndItem;
            const bool hover = GetPropW(item.hwndItem, L"RFF.Button.Hover") != nullptr;
            const bool primary = GetPropW(item.hwndItem, L"RFF.Button.Primary") != nullptr;
            PanelDrawing::fill(item.hDC, item.rcItem, theme.background);
            const auto faceColor = !disabled && (pressed || primary) ? theme.selected : theme.field;
            const auto borderColor = !disabled && (focused || hover || primary) ? theme.accent : theme.track;
            PanelDrawing::rounded(item.hDC, item.rcItem, faceColor, borderColor, 2);
            std::wstring label(GetWindowTextLengthW(item.hwndItem) + 1, L'\0');
            GetWindowTextW(item.hwndItem, label.data(), int(label.size()));
            label.resize(wcslen(label.c_str()));
            for (size_t escapePosition = 0;
                 (escapePosition = label.find(L"&&", escapePosition)) != std::wstring::npos;
                 ++escapePosition) {
                label.erase(escapePosition, 1);
            }
            RECT textBounds = item.rcItem;
            const int padding = int(8 * context.scale + .5f);
            textBounds.left += padding;
            textBounds.right -= padding;
            const auto textColor = disabled             ? theme.secondary
                                   : pressed || primary ? theme.selectedForeground
                                                        : theme.foreground;
            PanelDrawing::text(item.hDC, label, textBounds, textColor, context.font, DT_CENTER);
            if (focused) {
                RECT focusBounds = item.rcItem;
                InflateRect(&focusBounds, -3, -3);
                DrawFocusRect(item.hDC, &focusBounds);
            }
            if (bufferDc) {
                buffer.present(target.hDC);
            }
        }
        static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR subclassId, DWORD_PTR) {
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_MOUSEMOVE && !GetPropW(window, L"RFF.Button.Hover")) {
                SetPropW(window, L"RFF.Button.Hover", reinterpret_cast<HANDLE>(1));
                TRACKMOUSEEVENT trackingRequest{sizeof(trackingRequest), TME_LEAVE, window, 0};
                TrackMouseEvent(&trackingRequest);
                InvalidateRect(window, nullptr, FALSE);
            }
            if (message == WM_MOUSELEAVE) {
                RemovePropW(window, L"RFF.Button.Hover");
                InvalidateRect(window, nullptr, FALSE);
            }
            if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
                InvalidateRect(window, nullptr, FALSE);
            }
            if (message == WM_NCDESTROY) {
                RemovePropW(window, L"RFF.Button.Hover");
                RemovePropW(window, L"RFF.Button.Primary");
                RemoveWindowSubclass(window, procedure, subclassId);
            }
            return DefSubclassProc(window, message, wParam, lParam);
        }
        static void attach(HWND window, bool primary = false) {
            UiLanguage::caption(window);
            SetWindowLongPtrW(window, GWL_STYLE,
                              (GetWindowLongPtrW(window, GWL_STYLE) & ~BS_TYPEMASK) | BS_OWNERDRAW);
            if (primary) {
                SetPropW(window, L"RFF.Button.Primary", reinterpret_cast<HANDLE>(1));
            }
            SetWindowSubclass(window, procedure, 4, 0);
        }
    };
} // namespace merutilm::rff2::workspace
