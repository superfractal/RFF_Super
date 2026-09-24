//
// Modified by GPT-6 on 2026-09-14, 2026-09-17, 2026-09-20, 2026-09-22
//

#pragma once
#include "PanelBackBuffer.hpp"
#include "PanelDrawing.hpp"
#include "WorkspaceTheme.hpp"
#include "WorkspaceText.hpp"
#include "AccessibleControl.hpp"
#include <algorithm>
#include <commctrl.h>
#include <functional>
#include <string>

namespace merutilm::rff2::workspace {
    class SearchField {
      public:
        enum class Navigation { SUBMIT, RESULTS, PREVIOUS, NEXT, ESCAPE };

      private:
        HWND window = nullptr;
        HWND input = nullptr;
        HWND clear = nullptr;
        HWND cue = nullptr;
        HFONT font;
        const WorkspaceTheme &theme;
        float scale;
        HBRUSH fieldBrush = nullptr;
        PanelBackBuffer buffer;
        PanelBackBuffer cueBuffer;
        std::function<void(std::wstring)> changed;
        std::function<void(Navigation)> navigate;
        bool clearHovered = false;

        int px(int value) const {
            return int(value * scale + .5f);
        }
        bool empty() const {
            return GetWindowTextLengthW(input) == 0;
        }
        bool focused() const {
            return GetFocus() == input || GetFocus() == clear;
        }

        void invalidate() {
            InvalidateRect(window, nullptr, FALSE);
            InvalidateRect(clear, nullptr, FALSE);
            InvalidateRect(cue, nullptr, FALSE);
        }

        void arrange() {
            if (!input || !clear) {
                return;
            }
            if (empty()) {
                clearHovered = false;
                if (GetFocus() == clear) {
                    SetFocus(input);
                }
            }
            RECT bounds;
            GetClientRect(window, &bounds);
            HDC dc = GetDC(input);
            const auto previousFont = SelectObject(dc, font);
            TEXTMETRICW metrics{};
            GetTextMetricsW(dc, &metrics);
            SelectObject(dc, previousFont);
            ReleaseDC(input, dc);
            const int trailingWidth = empty() && bounds.right >= px(240) ? px(64) : px(36);
            SetWindowPos(input, nullptr, px(32), (bounds.bottom - metrics.tmHeight) / 2,
                         std::max(1, int(bounds.right) - px(32) - trailingWidth), metrics.tmHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(cue, HWND_TOP, px(32) + px(2), (bounds.bottom - metrics.tmHeight) / 2,
                         std::max(1, int(bounds.right) - px(34) - trailingWidth), metrics.tmHeight,
                         SWP_NOACTIVATE);
            ShowWindow(cue, empty() ? SW_SHOWNA : SW_HIDE);
            SetWindowPos(clear, nullptr, bounds.right - px(30), px(2), px(28), bounds.bottom - px(4),
                         SWP_NOZORDER | SWP_NOACTIVATE);
            ShowWindow(clear, empty() ? SW_HIDE : SW_SHOWNA);
            invalidate();
        }

        void paint(HDC dc) const {
            RECT bounds;
            GetClientRect(window, &bounds);
            PanelDrawing::fill(dc, bounds, theme.background);
            PanelDrawing::rounded(dc, bounds, theme.field, focused() ? theme.accent : theme.track, px(2));
            const int centerX = px(14), centerY = bounds.bottom / 2 - px(1), radius = px(4);
            const auto pen = CreatePen(PS_SOLID, std::max(1, px(1)), theme.secondary);
            const auto previousPen = SelectObject(dc, pen);
            const auto previousBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
            Ellipse(dc, centerX - radius, centerY - radius, centerX + radius + 1, centerY + radius + 1);
            MoveToEx(dc, centerX + radius - 1, centerY + radius - 1, nullptr);
            LineTo(dc, centerX + px(8), centerY + px(8));
            SelectObject(dc, previousBrush);
            SelectObject(dc, previousPen);
            DeleteObject(pen);
            if (empty() && bounds.right >= px(240)) {
                PanelDrawing::text(dc, uiText(TextKey::SearchShortcut),
                                   {bounds.right - px(60), 0, bounds.right - px(10), bounds.bottom},
                                   theme.secondary, font, DT_RIGHT);
            }
        }

        void paintCue(HDC dc) const {
            RECT bounds;
            GetClientRect(cue, &bounds);
            PanelDrawing::fill(dc, bounds, theme.field);
            PanelDrawing::text(dc, uiText(TextKey::SearchPlaceholder), bounds, theme.secondary, font);
        }

        static LRESULT CALLBACK cueProcedure(HWND control, UINT message, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR subclassId, DWORD_PTR contextData) {
            auto &self = *reinterpret_cast<SearchField *>(contextData);
            if (message == WM_NCHITTEST) {
                return HTTRANSPARENT;
            }
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_PRINTCLIENT || (message == WM_PRINT && (lParam & PRF_CLIENT))) {
                self.paintCue(reinterpret_cast<HDC>(wParam));
                return 0;
            }
            if (message == WM_PAINT) {
                PAINTSTRUCT paintState;
                const auto targetDc = BeginPaint(control, &paintState);
                try {
                    RECT bounds;
                    GetClientRect(control, &bounds);
                    if (auto dc = self.cueBuffer.begin(targetDc, bounds.right, bounds.bottom)) {
                        self.paintCue(dc);
                        self.cueBuffer.present(targetDc);
                    }
                } catch (...) {
                    EndPaint(control, &paintState);
                    throw;
                }
                EndPaint(control, &paintState);
                return 0;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(control, cueProcedure, subclassId);
            }
            return DefSubclassProc(control, message, wParam, lParam);
        }

        void paintClear(HDC dc, UINT state) const {
            RECT bounds;
            GetClientRect(clear, &bounds);
            PanelDrawing::fill(dc, bounds,
                               clearHovered || (state & ODS_SELECTED) ? theme.selected : theme.field);
            if (GetFocus() == clear) {
                RECT focusBounds = bounds;
                InflateRect(&focusBounds, -px(2), -px(2));
                PanelDrawing::rounded(dc, focusBounds, theme.field, theme.accent, px(2));
            }
            const int centerX = bounds.right / 2, centerY = bounds.bottom / 2, radius = px(4);
            const bool selected = (clearHovered || (state & ODS_SELECTED)) && GetFocus() != clear;
            const auto pen = CreatePen(PS_SOLID, std::max(1, px(1)),
                                       selected ? theme.selectedForeground : theme.foreground);
            const auto previousPen = SelectObject(dc, pen);
            MoveToEx(dc, centerX - radius, centerY - radius, nullptr);
            LineTo(dc, centerX + radius + 1, centerY + radius + 1);
            MoveToEx(dc, centerX + radius, centerY - radius, nullptr);
            LineTo(dc, centerX - radius - 1, centerY + radius + 1);
            SelectObject(dc, previousPen);
            DeleteObject(pen);
        }

        void clearInput() {
            SetFocus(input);
            SetWindowTextW(input, L"");
        }

        static LRESULT CALLBACK inputProcedure(HWND control, UINT message, WPARAM wParam, LPARAM lParam,
                                               UINT_PTR subclassId, DWORD_PTR contextData) {
            auto &self = *reinterpret_cast<SearchField *>(contextData);
            if (message == WM_KEYDOWN) {
                if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'A' || wParam == 'F')) {
                    SendMessageW(control, EM_SETSEL, 0, -1);
                    return 0;
                }
                if (wParam == VK_RETURN) {
                    self.navigate(Navigation::SUBMIT);
                    return 0;
                }
                if (wParam == VK_DOWN) {
                    self.navigate(Navigation::RESULTS);
                    return 0;
                }
                if (wParam == VK_ESCAPE) {
                    self.clearInput();
                    self.navigate(Navigation::ESCAPE);
                    return 0;
                }
                if (wParam == VK_TAB) {
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        self.navigate(Navigation::PREVIOUS);
                    } else if (!self.empty()) {
                        SetFocus(self.clear);
                    } else {
                        self.navigate(Navigation::NEXT);
                    }
                    return 0;
                }
            }
            if (message == WM_CHAR && (wParam == VK_RETURN || wParam == VK_TAB || wParam == VK_ESCAPE ||
                                       wParam == 1 || wParam == 6)) {
                return 0;
            }
            if (message == WM_GETDLGCODE) {
                return DefSubclassProc(control, message, wParam, lParam) | DLGC_WANTTAB | DLGC_WANTARROWS;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(control, inputProcedure, subclassId);
            }
            const auto result = DefSubclassProc(control, message, wParam, lParam);
            if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
                self.invalidate();
            }
            if ((message == WM_LBUTTONUP || message == WM_KEYUP || message == WM_SETTEXT) && self.empty()) {
                InvalidateRect(control, nullptr, FALSE);
            }
            return result;
        }

        static LRESULT CALLBACK clearProcedure(HWND control, UINT message, WPARAM wParam, LPARAM lParam,
                                               UINT_PTR subclassId, DWORD_PTR contextData) {
            auto &self = *reinterpret_cast<SearchField *>(contextData);
            if (message == WM_KEYDOWN) {
                if (wParam == VK_TAB) {
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        SetFocus(self.input);
                    } else {
                        self.navigate(Navigation::NEXT);
                    }
                    return 0;
                }
                if (wParam == VK_RETURN || wParam == VK_ESCAPE) {
                    self.clearInput();
                    return 0;
                }
            }
            if (message == WM_CHAR && (wParam == VK_RETURN || wParam == VK_TAB || wParam == VK_ESCAPE)) {
                return 0;
            }
            if (message == WM_GETDLGCODE) {
                return DLGC_BUTTON | DLGC_WANTTAB;
            }
            if (message == WM_MOUSEMOVE) {
                self.clearHovered = true;
                self.invalidate();
                TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, control, 0};
                TrackMouseEvent(&tracking);
            }
            if (message == WM_MOUSELEAVE) {
                self.clearHovered = false;
                self.invalidate();
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(control, clearProcedure, subclassId);
            }
            const auto result = DefSubclassProc(control, message, wParam, lParam);
            if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
                self.invalidate();
            }
            return result;
        }

        static LRESULT CALLBACK procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
            auto *self = reinterpret_cast<SearchField *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<SearchField *>(reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
                self->window = handle;
                SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(handle, message, wParam, lParam);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PRINT: {
                const auto result = DefWindowProcW(handle, message, wParam, lParam);
                if ((lParam & PRF_CHILDREN) && self->empty()) {
                    RECT bounds;
                    GetWindowRect(self->cue, &bounds);
                    MapWindowPoints(nullptr, handle, reinterpret_cast<POINT *>(&bounds), 2);
                    const auto dc = reinterpret_cast<HDC>(wParam);
                    const int savedDc = SaveDC(dc);
                    if (savedDc != 0) {
                        try {
                            OffsetViewportOrgEx(dc, bounds.left, bounds.top, nullptr);
                            self->paintCue(dc);
                        } catch (...) {
                            RestoreDC(dc, savedDc);
                            throw;
                        }
                        RestoreDC(dc, savedDc);
                    }
                }
                return result;
            }
            case WM_SIZE:
                self->arrange();
                return 0;
            case WM_SETFOCUS:
            case WM_LBUTTONDOWN:
                SetFocus(self->input);
                return 0;
            case WM_CTLCOLOREDIT:
                SetTextColor(reinterpret_cast<HDC>(wParam), self->theme.foreground);
                SetBkColor(reinterpret_cast<HDC>(wParam), self->theme.field);
                return reinterpret_cast<LRESULT>(self->fieldBrush);
            case WM_DRAWITEM: {
                const auto *item = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
                if (item->hwndItem == self->clear) {
                    self->paintClear(item->hDC, item->itemState);
                    return TRUE;
                }
                break;
            }
            case WM_COMMAND:
                if (reinterpret_cast<HWND>(lParam) == self->input && HIWORD(wParam) == EN_CHANGE) {
                    const int textLength = GetWindowTextLengthW(self->input);
                    std::wstring inputText(textLength + 1, L'\0');
                    GetWindowTextW(self->input, inputText.data(), textLength + 1);
                    inputText.resize(textLength);
                    self->arrange();
                    self->changed(std::move(inputText));
                    return 0;
                }
                if (reinterpret_cast<HWND>(lParam) == self->clear && HIWORD(wParam) == BN_CLICKED) {
                    self->clearInput();
                    return 0;
                }
                break;
            case WM_PRINTCLIENT:
                self->paint(reinterpret_cast<HDC>(wParam));
                return 0;
            case WM_PAINT: {
                PAINTSTRUCT paintState;
                const HDC targetDc = BeginPaint(handle, &paintState);
                try {
                    RECT bounds;
                    GetClientRect(handle, &bounds);
                    if (HDC dc = self->buffer.begin(targetDc, bounds.right, bounds.bottom)) {
                        self->paint(dc);
                        self->buffer.present(targetDc);
                    }
                } catch (...) {
                    EndPaint(handle, &paintState);
                    throw;
                }
                EndPaint(handle, &paintState);
                return 0;
            }
            }
            return DefWindowProcW(handle, message, wParam, lParam);
        }

      public:
        SearchField(HWND parent, HFONT font, const WorkspaceTheme &theme, float scale,
                    std::function<void(std::wstring)> changed, std::function<void(Navigation)> navigate)
            : font(font), theme(theme), scale(scale), changed(std::move(changed)),
              navigate(std::move(navigate)) {
            fieldBrush = CreateSolidBrush(theme.field);
            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = procedure;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = L"RFF.Workspace.Search";
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&windowClass);
            window = CreateWindowExW(0, windowClass.lpszClassName, uiText(TextKey::SearchPlaceholder),
                                     WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 1, 1, parent, nullptr,
                                     windowClass.hInstance, this);
            input = CreateWindowExW(0, L"EDIT", L"",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | ES_AUTOHSCROLL, 0,
                                    0, 1, 1, window, nullptr, windowClass.hInstance, nullptr);
            cue = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 1, 1,
                                  window, nullptr, windowClass.hInstance, nullptr);
            clear = CreateWindowExW(0, L"BUTTON", uiText(TextKey::ClearSearch),
                                    WS_CHILD | WS_TABSTOP | BS_OWNERDRAW, 0, 0, 1, 1, window, nullptr,
                                    windowClass.hInstance, nullptr);
            SendMessageW(input, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            SendMessageW(input, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
            SendMessageW(input, EM_SETLIMITTEXT, 512, 0);
            SetWindowSubclass(input, inputProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
            SetWindowSubclass(cue, cueProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
            SetWindowSubclass(clear, clearProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
            AccessibleControl::describe(
                input, L"Search settings",
                L"Enter a setting name. Press Enter for results or Escape to clear the search.",
                L"workspace.search");
            AccessibleControl::describe(clear, uiText(TextKey::ClearSearch),
                                        L"Remove the current settings search.", L"workspace.search.clear");
        }
        ~SearchField() {
            if (IsWindow(window)) {
                DestroyWindow(window);
            }
            DeleteObject(fieldBrush);
        }
        SearchField(const SearchField &) = delete;
        SearchField &operator=(const SearchField &) = delete;
        HWND editor() const {
            return input;
        }
        void applyMetrics(HFONT replacement, float newScale) {
            font = replacement;
            scale = newScale;
            SendMessageW(input, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            arrange();
        }
        void layout(RECT bounds) {
            SetWindowPos(window, nullptr, bounds.left, bounds.top, bounds.right - bounds.left,
                         bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        void applyTheme() {
            if (const auto replacement = CreateSolidBrush(theme.field)) {
                DeleteObject(fieldBrush);
                fieldBrush = replacement;
            }
            InvalidateRect(input, nullptr, FALSE);
            invalidate();
        }
    };
} // namespace merutilm::rff2::workspace
