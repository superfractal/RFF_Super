//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-19, 2026-09-20, 2026-09-22, 2026-09-24, 2026-09-25
// Modified by Opus 5.5 on 2026-09-26
//

#pragma once
#include "PanelDrawing.hpp"
#include "PanelBackBuffer.hpp"
#include "WorkspaceTheme.hpp"
#include <algorithm>
#include <commctrl.h>
#include <string>

namespace merutilm::rff2::workspace {
    struct WorkspaceComboDrawing {
        struct Context {
            const WorkspaceTheme *theme = nullptr;
            HFONT font = nullptr;
            float scale = 1;
        };
        static LRESULT CALLBACK listProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                               UINT_PTR subclassId, DWORD_PTR contextData) {
            const auto &context = *reinterpret_cast<const Context *>(contextData);
            if (message == WM_PAINT) {
                PAINTSTRUCT paintState;
                const auto target = BeginPaint(window, &paintState);
                RECT bounds;
                GetClientRect(window, &bounds);
                PanelBackBuffer buffer;
                try {
                    const auto memory = buffer.begin(target, bounds.right, bounds.bottom);
                    const auto dc = memory ? memory : target;
                    PanelDrawing::fill(dc, bounds, context.theme->field);
                    DefSubclassProc(window, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
                    if (memory) {
                        buffer.present(target);
                    }
                } catch (...) {
                    EndPaint(window, &paintState);
                    throw;
                }
                EndPaint(window, &paintState);
                return 0;
            }
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(window, listProcedure, subclassId);
            }
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            if (message == WM_WINDOWPOSCHANGED &&
                (reinterpret_cast<WINDOWPOS *>(lParam)->flags & SWP_SHOWWINDOW)) {
                RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            return result;
        }
        static void face(HWND window, HDC dc, const Context &context) {
            const auto &theme = *context.theme;
            RECT bounds;
            GetClientRect(window, &bounds);
            const bool focused = GetFocus() == window;
            PanelDrawing::rounded(dc, bounds, theme.field, focused ? theme.accent : theme.track, 2);
            const int inset = int(8 * context.scale + .5f);
            const int arrowWidth = int(24 * context.scale + .5f);
            const int selectedIndex = static_cast<int>(SendMessageW(window, CB_GETCURSEL, 0, 0));
            const auto labelLength = SendMessageW(window, CB_GETLBTEXTLEN, selectedIndex, 0);
            if (labelLength != CB_ERR) {
                std::wstring label(static_cast<size_t>(labelLength) + 1, L'\0');
                SendMessageW(window, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(label.data()));
                label.resize(static_cast<size_t>(labelLength));
                RECT textBounds = bounds;
                textBounds.left += inset;
                textBounds.right -= arrowWidth;
                PanelDrawing::text(dc, label, textBounds,
                                   IsWindowEnabled(window) ? theme.foreground : theme.secondary,
                                   context.font);
            }
            const int centerX = bounds.right - arrowWidth / 2, centerY = (bounds.top + bounds.bottom) / 2;
            const int half = int(3 * context.scale + .5f);
            const auto pen = CreatePen(PS_SOLID, std::max(1, int(context.scale + .5f)), theme.secondary);
            const auto previousPen = SelectObject(dc, pen);
            MoveToEx(dc, centerX - half, centerY - half / 2, nullptr);
            LineTo(dc, centerX, centerY + half / 2);
            LineTo(dc, centerX + half, centerY - half / 2);
            SelectObject(dc, previousPen);
            DeleteObject(pen);
        }
        // Windows' slide-open animation shows each newly uncovered strip unpainted (white) for a frame, so the list opens without it.
        static LRESULT withoutSlideAnimation(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
            BOOL animation = FALSE;
            if (!SystemParametersInfoW(SPI_GETCOMBOBOXANIMATION, 0, &animation, 0) || !animation) {
                return DefSubclassProc(window, message, wParam, lParam);
            }
            SystemParametersInfoW(SPI_SETCOMBOBOXANIMATION, 0, reinterpret_cast<PVOID>(FALSE), 0);
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            SystemParametersInfoW(SPI_SETCOMBOBOXANIMATION, 0, reinterpret_cast<PVOID>(TRUE), 0);
            return result;
        }
        static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR subclassId, DWORD_PTR contextData) {
            const auto &context = *reinterpret_cast<const Context *>(contextData);
            if (message == WM_MOUSEWHEEL && !SendMessageW(window, CB_GETDROPPEDSTATE, 0, 0)) {
                if (const auto parent = GetParent(window)) {
                    SendMessageW(parent, message, wParam, lParam);
                }
                return 0;
            }
            if (message == WM_CTLCOLORLISTBOX) {
                const auto drawingDc = reinterpret_cast<HDC>(wParam);
                SetTextColor(drawingDc, context.theme->foreground);
                SetBkColor(drawingDc, context.theme->field);
                SetDCBrushColor(drawingDc, context.theme->field);
                return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
            }
            if (message == WM_PAINT) {
                PAINTSTRUCT paintState;
                const HDC targetDc = BeginPaint(window, &paintState);
                RECT bounds;
                GetClientRect(window, &bounds);
                PanelBackBuffer buffer;
                try {
                    const auto drawingDc = buffer.begin(targetDc, bounds.right, bounds.bottom);
                    face(window, drawingDc ? drawingDc : targetDc, context);
                    if (drawingDc) {
                        buffer.present(targetDc);
                    }
                } catch (...) {
                    EndPaint(window, &paintState);
                    throw;
                }
                EndPaint(window, &paintState);
                return 0;
            }
            if (message == WM_PRINTCLIENT || (message == WM_PRINT && (lParam & PRF_CLIENT))) {
                face(window, reinterpret_cast<HDC>(wParam), context);
                return 0;
            }
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_NCDESTROY) {
                COMBOBOXINFO info{sizeof(info)};
                if (GetComboBoxInfo(window, &info) && IsWindow(info.hwndList)) {
                    RemoveWindowSubclass(info.hwndList, listProcedure, 1);
                }
                RemoveWindowSubclass(window, procedure, subclassId);
            }
            const bool opensList = message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK || message == CB_SHOWDROPDOWN ||
                                   ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
                                    (wParam == VK_F4 || wParam == VK_DOWN || wParam == VK_UP));
            const auto result = opensList ? withoutSlideAnimation(window, message, wParam, lParam)
                                          : DefSubclassProc(window, message, wParam, lParam);
            if (message == WM_SETFOCUS || message == WM_KILLFOCUS || message == CB_SETCURSEL ||
                message == WM_ENABLE) {
                InvalidateRect(window, nullptr, FALSE);
            }
            return result;
        }
        static void attach(HWND window, Context &context) {
            SetWindowSubclass(window, procedure, 1, reinterpret_cast<DWORD_PTR>(&context));
            COMBOBOXINFO info{sizeof(info)};
            if (GetComboBoxInfo(window, &info) && IsWindow(info.hwndList)) {
                SetWindowSubclass(info.hwndList, listProcedure, 1, reinterpret_cast<DWORD_PTR>(&context));
            }
        }
        static void applyMetrics(HWND window, const Context &context, int faceHeight = 0) {
            SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(context.font), FALSE);
            const int height = std::max(1, int(20 * context.scale + .5f));
            SendMessageW(window, CB_SETITEMHEIGHT, 0, height);
            SendMessageW(window, CB_SETITEMHEIGHT, WPARAM(-1), height);
            if (faceHeight > 0) {
                RECT bounds;
                GetWindowRect(window, &bounds);
                const int currentFaceHeight = int(SendMessageW(window, CB_GETITEMHEIGHT, WPARAM(-1), 0));
                SendMessageW(window, CB_SETITEMHEIGHT, WPARAM(-1),
                             std::max(1, currentFaceHeight + faceHeight - int(bounds.bottom - bounds.top)));
            }
            InvalidateRect(window, nullptr, FALSE);
        }
        static void draw(const DRAWITEMSTRUCT &item, const Context &context, int inset) {
            if (item.itemState & ODS_COMBOBOXEDIT) {
                face(item.hwndItem, item.hDC, context);
                return;
            }
            const auto &theme = *context.theme;
            const auto font = context.font;
            const bool selected = (item.itemState & ODS_SELECTED) != 0;
            PanelDrawing::fill(item.hDC, item.rcItem, selected ? theme.selected : theme.field);
            int itemIndex = static_cast<int>(item.itemID);
            if (itemIndex < 0) {
                itemIndex = static_cast<int>(SendMessageW(item.hwndItem, CB_GETCURSEL, 0, 0));
            }
            const auto labelLength = SendMessageW(item.hwndItem, CB_GETLBTEXTLEN, itemIndex, 0);
            if (labelLength != CB_ERR) {
                std::wstring label(static_cast<size_t>(labelLength) + 1, L'\0');
                SendMessageW(item.hwndItem, CB_GETLBTEXT, itemIndex, reinterpret_cast<LPARAM>(label.data()));
                label.resize(static_cast<size_t>(labelLength));
                RECT bounds = item.rcItem;
                bounds.left += inset;
                bounds.right -= inset;
                PanelDrawing::text(item.hDC, label, bounds,
                                   selected ? theme.selectedForeground : theme.foreground, font);
            }
            if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT)) {
                RECT focusBounds = item.rcItem;
                InflateRect(&focusBounds, -1, -1);
                DrawFocusRect(item.hDC, &focusBounds);
            }
        }
    };
} // namespace merutilm::rff2::workspace
