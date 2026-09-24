//
// Modified by GPT-6 on 2026-09-18, 2026-09-19, 2026-09-22, 2026-09-23, 2026-09-24
//

#pragma once
#include "PanelDrawing.hpp"
#include "PanelBackBuffer.hpp"
#include "WorkspaceTheme.hpp"
#include "AccessibleControl.hpp"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <windowsx.h>

namespace merutilm::rff2::workspace {
    class PanelDockHandle {
        static constexpr int noTarget = -1;
        static constexpr int outsideLeft = 0;
        static constexpr int outsideRight = 1;
        static constexpr int siblingRightHalf = 2;
        static constexpr int siblingLeftHalf = 3;

        HWND window = nullptr;
        HWND preview = nullptr;
        HFONT font = nullptr;
        float scale = 1;
        std::wstring label;
        PanelBackBuffer buffer;
        RECT area{};
        RECT neighbor{};
        POINT anchor{};
        bool pressed = false;
        bool dragging = false;
        int target = noTarget;
        std::function<bool()> begin;
        std::function<void(bool, bool)> dock;
        std::function<void(int)> tab;
        int px(int n) const {
            return int(n * scale + .5f);
        }
        POINT parentPoint(LPARAM l) const {
            POINT p{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
            MapWindowPoints(window, GetParent(window), &p, 1);
            return p;
        }
        void update(POINT point) {
            if (!dragging && (std::abs(point.x - anchor.x) >= GetSystemMetrics(SM_CXDRAG) ||
                              std::abs(point.y - anchor.y) >= GetSystemMetrics(SM_CYDRAG))) {
                dragging = true;
            }
            if (!dragging) {
                return;
            }
            const int nextTarget = dropTarget(area, neighbor, point, px(240));
            if (nextTarget != target) {
                target = nextTarget;
                if (target < 0) {
                    ShowWindow(preview, SW_HIDE);
                } else {
                    RECT hint = dropPreview(area, neighbor, target, px(240));
                    SetWindowPos(preview, HWND_TOP, hint.left, hint.top, hint.right - hint.left,
                                 hint.bottom - hint.top, SWP_NOACTIVATE | SWP_SHOWWINDOW);
                    InvalidateRect(preview, nullptr, FALSE);
                }
            }
            SetCursor(LoadCursor(nullptr, target < 0 ? IDC_NO : IDC_SIZEALL));
        }
        void finish(bool accept) {
            const int selectedTarget = accept && dragging ? target : noTarget;
            pressed = false;
            dragging = false;
            target = noTarget;
            ShowWindow(preview, SW_HIDE);
            if (GetCapture() == window) {
                ReleaseCapture();
            }
            InvalidateRect(window, nullptr, FALSE);
            if (selectedTarget >= 0) {
                const bool dockOnRight = selectedTarget == outsideRight || selectedTarget == siblingLeftHalf;
                const bool dockOutside = selectedTarget == outsideLeft || selectedTarget == outsideRight;
                dock(dockOnRight, dockOutside);
            }
        }
        void paint(HWND handle, HDC dc) {
            RECT bounds;
            GetClientRect(handle, &bounds);
            const auto theme = WorkspaceTheme::current();
            const bool feedback = handle == preview;
            PanelDrawing::fill(dc, bounds, feedback ? theme.selected : theme.field);
            if (feedback) {
                PanelDrawing::border(dc, bounds, theme.accent, std::max(2, px(2)));
                const wchar_t *hint = L"Drop to dock left";
                if (target >= siblingRightHalf) {
                    hint = L"Drop beside this panel";
                } else if (target == outsideRight) {
                    hint = L"Drop to dock right";
                }
                PanelDrawing::text(dc, hint, bounds, theme.selectedForeground, font, DT_CENTER);
            } else {
                for (int y : {10, 15, 20}) {
                    for (int x : {10, 15}) {
                        PanelDrawing::fill(dc, {px(x), px(y), px(x + 2), px(y + 2)}, theme.secondary);
                    }
                }
                bounds.left = px(26);
                bounds.right -= px(6);
                PanelDrawing::text(dc, label, bounds, theme.foreground, font);
                if (GetFocus() == window) {
                    GetClientRect(handle, &bounds);
                    PanelDrawing::border(dc, bounds, theme.accent, std::max(1, px(1)));
                }
            }
        }
        static LRESULT CALLBACK procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
            auto *self = reinterpret_cast<PanelDockHandle *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self =
                    static_cast<PanelDockHandle *>(reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
                SetWindowLongPtrW(handle, GWLP_USERDATA, LONG_PTR(self));
            }
            if (!self) {
                return DefWindowProcW(handle, message, wParam, lParam);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PRINTCLIENT:
                self->paint(handle, HDC(wParam));
                return 0;
            case WM_PAINT: {
                PAINTSTRUCT paintState;
                auto dc = BeginPaint(handle, &paintState);
                try {
                    RECT bounds;
                    GetClientRect(handle, &bounds);
                    if (handle == self->preview) {
                        self->paint(handle, dc);
                    } else if (auto back = self->buffer.begin(dc, bounds.right, bounds.bottom)) {
                        self->paint(handle, back);
                        self->buffer.present(dc);
                    }
                } catch (...) {
                    EndPaint(handle, &paintState);
                    throw;
                }
                EndPaint(handle, &paintState);
                return 0;
            }
            case WM_GETDLGCODE:
                return DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTALLKEYS;
            case WM_SETFOCUS:
                InvalidateRect(handle, nullptr, FALSE);
                return 0;
            case WM_KILLFOCUS:
                if (self->pressed) {
                    self->finish(false);
                }
                InvalidateRect(handle, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN:
                if (handle == self->preview || !self->begin()) {
                    return 0;
                }
                SetFocus(handle);
                self->anchor = self->parentPoint(lParam);
                self->pressed = true;
                SetCapture(handle);
                return 0;
            case WM_MOUSEMOVE:
                if (self->pressed && GetCapture() == handle) {
                    self->update(self->parentPoint(lParam));
                }
                return 0;
            case WM_LBUTTONUP:
                if (self->pressed) {
                    self->update(self->parentPoint(lParam));
                    self->finish(true);
                }
                return 0;
            case WM_CAPTURECHANGED:
            case WM_CANCELMODE:
                if (self->pressed) {
                    self->finish(false);
                }
                return 0;
            case WM_KEYDOWN:
                if (wParam == VK_ESCAPE) {
                    self->finish(false);
                    return 0;
                }
                if (wParam == VK_TAB) {
                    self->finish(false);
                    self->tab((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                    return 0;
                }
                if (!self->pressed && (wParam == VK_LEFT || wParam == VK_RIGHT) && self->begin()) {
                    self->dock(wParam == VK_RIGHT, (GetKeyState(VK_SHIFT) & 0x8000) == 0);
                    return 0;
                }
                break;
            case WM_SETCURSOR:
                if (LOWORD(lParam) == HTCLIENT) {
                    SetCursor(LoadCursor(nullptr, self->dragging && self->target < 0 ? IDC_NO : IDC_SIZEALL));
                    return TRUE;
                }
                break;
            }
            return DefWindowProcW(handle, message, wParam, lParam);
        }

      public:
        static int dropSide(RECT bounds, POINT point, int maximumWidth) {
            if (!PtInRect(&bounds, point)) {
                return noTarget;
            }
            const int width = std::min(maximumWidth, int(bounds.right - bounds.left) / 3);
            if (point.x < bounds.left + width) {
                return outsideLeft;
            }
            if (point.x >= bounds.right - width) {
                return outsideRight;
            }
            return noTarget;
        }
        static int dropTarget(RECT bounds, RECT sibling, POINT point, int maximumWidth) {
            if (!PtInRect(&bounds, point)) {
                return noTarget;
            }
            RECT nearby = sibling;
            InflateRect(&nearby, std::max(1, maximumWidth / 10), 0);
            if (!IsRectEmpty(&sibling) && PtInRect(&nearby, point)) {
                const bool siblingOnRight = sibling.left + sibling.right > bounds.left + bounds.right;
                const bool pointInRightHalf = point.x >= (sibling.left + sibling.right) / 2;
                if (pointInRightHalf == siblingOnRight) {
                    return siblingOnRight ? outsideRight : outsideLeft;
                }
                return siblingOnRight ? siblingLeftHalf : siblingRightHalf;
            }
            return dropSide(bounds, point, maximumWidth);
        }
        static RECT dropPreview(RECT bounds, RECT sibling, int target, int maximumWidth) {
            if (target >= siblingRightHalf) {
                const int middle = (sibling.left + sibling.right) / 2;
                if (target == siblingRightHalf) {
                    sibling.left = middle;
                } else {
                    sibling.right = middle;
                }
                return sibling;
            }
            const int width = std::min(maximumWidth, int(bounds.right - bounds.left) / 3);
            if (target == outsideRight) {
                bounds.left = bounds.right - width;
            } else {
                bounds.right = bounds.left + width;
            }
            return bounds;
        }
        PanelDockHandle(HWND parent, std::wstring name, std::function<bool()> begin,
                        std::function<void(bool, bool)> dock, std::function<void(int)> tab)
            : label(std::move(name)), begin(std::move(begin)), dock(std::move(dock)), tab(std::move(tab)) {
            WNDCLASSW cls{};
            cls.lpfnWndProc = procedure;
            cls.hInstance = GetModuleHandleW(nullptr);
            cls.lpszClassName = L"RFF.Workspace.DockHandle";
            cls.hCursor = LoadCursor(nullptr, IDC_SIZEALL);
            RegisterClassW(&cls);
            window = CreateWindowExW(0, cls.lpszClassName, UiLanguage::text(label).c_str(),
                                     WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS, 0, 0, 1, 1, parent, nullptr,
                                     cls.hInstance, this);
            preview = CreateWindowExW(WS_EX_NOACTIVATE, cls.lpszClassName, L"", WS_CHILD | WS_CLIPSIBLINGS, 0,
                                      0, 1, 1, parent, nullptr, cls.hInstance, this);
            AccessibleControl::describe(window, label,
                                        L"Drag to an edge or either half of the other panel. Escape cancels. "
                                        L"Left and Right dock outside; Shift docks inside.");
        }
        ~PanelDockHandle() {
            finish(false);
            DestroyWindow(preview);
            DestroyWindow(window);
        }
        HWND handle() const {
            return window;
        }
        void themeChanged() {
            InvalidateRect(window, nullptr, FALSE);
            InvalidateRect(preview, nullptr, FALSE);
        }
        void cancel() {
            finish(false);
        }
        void layout(RECT bounds, RECT workspace, HFONT value, float dpi, bool visible, RECT sibling = {}) {
            if (pressed) {
                finish(false);
            }
            font = value;
            scale = dpi;
            area = workspace;
            neighbor = sibling;
            SetWindowPos(window, HWND_TOP, bounds.left, bounds.top, std::max(1L, bounds.right - bounds.left),
                         std::max(1L, bounds.bottom - bounds.top), SWP_NOACTIVATE);
            ShowWindow(window, visible ? SW_SHOWNA : SW_HIDE);
            InvalidateRect(window, nullptr, FALSE);
        }
    };
} // namespace merutilm::rff2::workspace
