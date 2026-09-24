//
// Modified by GPT-6 on 2026-09-14, 2026-09-22, 2026-09-23
//

#pragma once
#include "PanelDrawing.hpp"
#include "PanelBackBuffer.hpp"
#include "WorkspaceTheme.hpp"
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <functional>
#include <utility>
#include <windowsx.h>

namespace merutilm::rff2::workspace {
    class PaneSplitter {
        HWND window = nullptr;
        PanelBackBuffer buffer;
        bool rightEdge = false;
        bool hovered = false;
        bool horizontal = false;
        enum class Interaction { NONE, POINTER, KEYBOARD };
        Interaction interaction = Interaction::NONE;
        int anchor = 0;
        int keyboardDelta = 0;
        float scale = 1;
        std::function<bool()> begin;
        std::function<void(int)> change;
        std::function<void(bool)> finish;
        std::function<void()> reset;
        std::function<void(int)> tab;

        int parentCoordinate(LPARAM point) const {
            POINT position{GET_X_LPARAM(point), GET_Y_LPARAM(point)};
            MapWindowPoints(window, GetParent(window), &position, 1);
            return horizontal ? position.y : position.x;
        }
        void end(bool cancel) {
            if (interaction == Interaction::NONE) {
                return;
            }
            interaction = Interaction::NONE;
            if (GetCapture() == window) {
                ReleaseCapture();
            }
            finish(cancel);
            InvalidateRect(window, nullptr, FALSE);
        }
        void paint(HDC dc) const {
            RECT bounds;
            GetClientRect(window, &bounds);
            const auto theme = WorkspaceTheme::current();
            const bool focused = GetFocus() == window;
            PanelDrawing::fill(dc, bounds, focused ? theme.selected : theme.background);
            const int thickness = hovered || focused || interaction != Interaction::NONE ? 2 : 1;
            RECT line = bounds;
            if (horizontal) {
                if (rightEdge) {
                    line.top = std::max(0L, bounds.bottom - thickness);
                } else {
                    line.bottom = thickness;
                }
            } else if (rightEdge) {
                line.left = std::max(0L, bounds.right - thickness);
            } else {
                line.right = thickness;
            }
            PanelDrawing::fill(dc, line, thickness > 1 ? theme.accent : settingsTheme().sectionFrame);
        }
        static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
            auto *self = reinterpret_cast<PaneSplitter *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<PaneSplitter *>(reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
                self->window = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PRINTCLIENT:
                self->paint(reinterpret_cast<HDC>(wParam));
                return 0;
            case WM_PAINT: {
                PAINTSTRUCT paintState;
                const auto targetDc = BeginPaint(hwnd, &paintState);
                RECT bounds;
                GetClientRect(hwnd, &bounds);
                if (const auto dc = self->buffer.begin(targetDc, bounds.right, bounds.bottom)) {
                    self->paint(dc);
                    self->buffer.present(targetDc);
                }
                EndPaint(hwnd, &paintState);
                return 0;
            }
            case WM_SETCURSOR:
                SetCursor(LoadCursor(nullptr, self->horizontal ? IDC_SIZENS : IDC_SIZEWE));
                return TRUE;
            case WM_GETDLGCODE:
                return DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_WANTTAB;
            case WM_SETFOCUS:
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_KILLFOCUS:
                self->end(false);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN:
                self->end(false);
                if (self->begin()) {
                    self->interaction = Interaction::POINTER;
                    self->anchor = self->parentCoordinate(lParam);
                    SetCapture(hwnd);
                }
                return 0;
            case WM_LBUTTONDBLCLK:
                self->end(true);
                self->reset();
                return 0;
            case WM_MOUSEMOVE:
                if (!self->hovered) {
                    self->hovered = true;
                    TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
                    TrackMouseEvent(&track);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                if (self->interaction == Interaction::POINTER) {
                    self->change(self->parentCoordinate(lParam) - self->anchor);
                }
                return 0;
            case WM_MOUSELEAVE:
                self->hovered = false;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_LBUTTONUP:
                if (self->interaction == Interaction::POINTER) {
                    self->change(self->parentCoordinate(lParam) - self->anchor);
                    self->end(false);
                }
                return 0;
            case WM_CAPTURECHANGED:
                if (self->interaction == Interaction::POINTER) {
                    self->end(true);
                }
                return 0;
            case WM_CANCELMODE:
                self->end(true);
                return 0;
            case WM_KEYDOWN:
                if (wParam == VK_ESCAPE) {
                    self->end(true);
                    return 0;
                }
                if (wParam == VK_TAB) {
                    self->end(false);
                    self->tab(GetKeyState(VK_SHIFT) < 0 ? -1 : 1);
                    return 0;
                }
                if (wParam == VK_HOME) {
                    self->end(true);
                    self->reset();
                    return 0;
                }
                if ((!self->horizontal && (wParam == VK_LEFT || wParam == VK_RIGHT)) ||
                    (self->horizontal && (wParam == VK_UP || wParam == VK_DOWN))) {
                    if (self->interaction == Interaction::NONE && self->begin()) {
                        self->interaction = Interaction::KEYBOARD;
                        self->keyboardDelta = 0;
                    }
                    if (self->interaction == Interaction::KEYBOARD) {
                        const int direction = wParam == VK_LEFT || wParam == VK_UP ? -1 : 1;
                        const int step = GetKeyState(VK_CONTROL) < 0 ? 1 : 8;
                        self->keyboardDelta += direction * step;
                        self->change(int(std::round(self->keyboardDelta * self->scale)));
                    }
                    return 0;
                }
                break;
            case WM_KEYUP:
                if ((wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN) &&
                    self->interaction == Interaction::KEYBOARD) {
                    self->end(false);
                }
                return 0;
            case WM_NCDESTROY:
                self->interaction = Interaction::NONE;
                self->window = nullptr;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                break;
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

      public:
        PaneSplitter(HWND parent, bool rightEdge, const wchar_t *name, std::function<bool()> begin,
                     std::function<void(int)> change, std::function<void(bool)> finish,
                     std::function<void()> reset, std::function<void(int)> tab, bool horizontal = false)
            : rightEdge(rightEdge), horizontal(horizontal), begin(std::move(begin)),
              change(std::move(change)), finish(std::move(finish)), reset(std::move(reset)),
              tab(std::move(tab)) {
            WNDCLASSW type{};
            type.hInstance = GetModuleHandleW(nullptr);
            type.lpfnWndProc = procedure;
            type.lpszClassName = L"RFF.Workspace.Splitter";
            type.style = CS_DBLCLKS;
            RegisterClassW(&type);
            CreateWindowExW(0, type.lpszClassName, name, WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS, 0, 0, 1, 1,
                            parent, nullptr, type.hInstance, this);
        }
        ~PaneSplitter() {
            interaction = Interaction::NONE;
            if (window) {
                DestroyWindow(window);
            }
        }
        PaneSplitter(const PaneSplitter &) = delete;
        PaneSplitter &operator=(const PaneSplitter &) = delete;
        HWND handle() const {
            return window;
        }
        static bool handleCapturedShortcut(const MSG &message) {
            if (message.message != WM_KEYDOWN || message.wParam != VK_ESCAPE) {
                return false;
            }
            const HWND captured = GetCapture();
            wchar_t type[64]{};
            if (!captured || !GetClassNameW(captured, type, 64) || wcscmp(type, L"RFF.Workspace.Splitter")) {
                return false;
            }
            auto *self = reinterpret_cast<PaneSplitter *>(GetWindowLongPtrW(captured, GWLP_USERDATA));
            if (!self) {
                return false;
            }
            self->cancel();
            return true;
        }
        void cancel() {
            end(true);
        }
        void layout(RECT bounds, float value) {
            scale = value;
            const bool visible = bounds.right > bounds.left && bounds.bottom > bounds.top;
            if (!visible) {
                end(true);
            }
            SetWindowPos(window, HWND_TOP, bounds.left, bounds.top, std::max(1L, bounds.right - bounds.left),
                         std::max(1L, bounds.bottom - bounds.top),
                         SWP_NOACTIVATE | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
            InvalidateRect(window, nullptr, FALSE);
        }
    };
} // namespace merutilm::rff2::workspace
