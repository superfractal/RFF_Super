//
// Modified by GPT-6 on 2026-09-20, 2026-09-22
//

#pragma once
#include "NativeDialogs.hpp"
#include "UiDpi.hpp"
#include "workspace/WorkspaceButton.hpp"
#include <array>

namespace merutilm::rff2 {
    class UnsavedChangesDialog {
        workspace::WorkspaceTheme theme = workspace::WorkspaceTheme::current();
        HBRUSH background = CreateSolidBrush(theme.background);
        HFONT font = nullptr;
        HFONT headingFont = nullptr;
        HWND heading = nullptr;
        HWND description = nullptr;
        std::array<HWND, 3> buttons{};
        UINT dpi = 96;

        int px(int value) const {
            return UiDpi::pixels(value, dpi);
        }

        void layout(HWND window) {
            if (font) {
                DeleteObject(font);
            }
            if (headingFont) {
                DeleteObject(headingFont);
            }
            font = CreateFontW(-px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                               UiLanguage::fontFace());
            headingFont = CreateFontW(-px(20), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH, UiLanguage::fontFace());
            const int width = px(520), inset = px(24), gap = px(12), buttonHeight = px(36);
            const HDC dc = GetDC(window);
            const auto previous = SelectObject(dc, headingFont);
            RECT headingBounds{0, 0, width - inset * 2, 0};
            const auto title = UiLanguage::text(L"Save changes before continuing?");
            DrawTextW(dc, title.c_str(), -1, &headingBounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(dc, font);
            RECT descriptionBounds{0, 0, width - inset * 2, 0};
            const auto detail = UiLanguage::text(
                L"Save includes your pending edits. Discarding changes cannot be undone.\nCancel keeps this document open.");
            DrawTextW(dc, detail.c_str(), -1, &descriptionBounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(dc, previous);
            ReleaseDC(window, dc);
            const int descriptionTop = inset + headingBounds.bottom + gap;
            const int buttonTop = descriptionTop + descriptionBounds.bottom + px(28);
            RECT frame{0, 0, width, buttonTop + buttonHeight + inset};
            UiDpi::adjustWindowRect(frame, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
                                    DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)), dpi);
            SetWindowPos(window, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(heading, nullptr, inset, inset, width - inset * 2, headingBounds.bottom,
                         SWP_NOZORDER);
            SetWindowPos(description, nullptr, inset, descriptionTop, width - inset * 2,
                         descriptionBounds.bottom, SWP_NOZORDER);
            SendMessageW(heading, WM_SETFONT, reinterpret_cast<WPARAM>(headingFont), TRUE);
            SendMessageW(description, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            const int buttonWidth = (width - inset * 2 - gap * 2) / 3;
            for (size_t i = 0; i < buttons.size(); ++i) {
                SetWindowPos(buttons[i], nullptr, inset + int(i) * (buttonWidth + gap), buttonTop,
                             buttonWidth, buttonHeight, SWP_NOZORDER);
                SendMessageW(buttons[i], WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            }
            InvalidateRect(window, nullptr, TRUE);
        }

        static INT_PTR CALLBACK procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
            auto *self = reinterpret_cast<UnsavedChangesDialog *>(GetWindowLongPtrW(window, DWLP_USER));
            if (message == WM_INITDIALOG) {
                self = reinterpret_cast<UnsavedChangesDialog *>(lParam);
                SetWindowLongPtrW(window, DWLP_USER, lParam);
                self->dpi = UiDpi::forWindow(window);
                SetWindowTextW(window, UiLanguage::label(L"Unsaved changes"));
                applyDarkWindowFrame(window);
                const auto instance = GetModuleHandleW(nullptr);
                self->heading =
                    CreateWindowW(L"STATIC", UiLanguage::label(L"Save changes before continuing?"),
                                  WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 0, 0, window, nullptr,
                                  instance, nullptr);
                self->description = CreateWindowW(
                    L"STATIC",
                    UiLanguage::label(
                        L"Save includes your pending edits. Discarding changes cannot be undone.\nCancel keeps this document open."),
                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 0, 0, window, nullptr, instance,
                    nullptr);
                const std::array<int, 3> ids{IDYES, IDNO, IDCANCEL};
                const std::array<const wchar_t *, 3> labels{L"Save", L"Discard changes", L"Cancel"};
                for (size_t i = 0; i < ids.size(); ++i) {
                    self->buttons[i] =
                        CreateWindowW(L"BUTTON", UiLanguage::label(labels[i]),
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0, window,
                                      reinterpret_cast<HMENU>(INT_PTR(ids[i])), instance, nullptr);
                    workspace::WorkspaceButton::attach(self->buttons[i], i == 0);
                }
                self->layout(window);
                MONITORINFO monitor{sizeof(monitor)};
                GetMonitorInfoW(MonitorFromWindow(GetParent(window), MONITOR_DEFAULTTONEAREST), &monitor);
                RECT anchor = monitor.rcWork, bounds{};
                if (IsWindowVisible(GetParent(window)) && !IsIconic(GetParent(window))) {
                    GetWindowRect(GetParent(window), &anchor);
                }
                GetWindowRect(window, &bounds);
                const LONG width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
                const LONG x = std::clamp((anchor.left + anchor.right - width) / 2, monitor.rcWork.left,
                                          std::max(monitor.rcWork.left, monitor.rcWork.right - width));
                const LONG y = std::clamp((anchor.top + anchor.bottom - height) / 2, monitor.rcWork.top,
                                          std::max(monitor.rcWork.top, monitor.rcWork.bottom - height));
                SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                SendMessageW(window, DM_SETDEFID, IDCANCEL, 0);
                SetFocus(self->buttons[2]);
                return FALSE;
            }
            if (!self) {
                return FALSE;
            }
            switch (message) {
            case WM_COMMAND:
                if (LOWORD(wParam) == IDYES || LOWORD(wParam) == IDNO || LOWORD(wParam) == IDCANCEL) {
                    EndDialog(window, LOWORD(wParam));
                    return TRUE;
                }
                break;
            case WM_CLOSE:
                EndDialog(window, IDCANCEL);
                return TRUE;
            case WM_CTLCOLORDLG:
            case WM_CTLCOLORSTATIC:
                SetTextColor(reinterpret_cast<HDC>(wParam), reinterpret_cast<HWND>(lParam) == self->heading
                                                                ? self->theme.foreground
                                                                : self->theme.secondary);
                SetBkColor(reinterpret_cast<HDC>(wParam), self->theme.background);
                return reinterpret_cast<INT_PTR>(self->background);
            case WM_DRAWITEM:
                workspace::WorkspaceButton::draw(*reinterpret_cast<DRAWITEMSTRUCT *>(lParam),
                                                 {&self->theme, self->font, float(self->dpi) / 96});
                return TRUE;
            case WM_DPICHANGED: {
                self->dpi = HIWORD(wParam);
                const RECT &suggested = *reinterpret_cast<RECT *>(lParam);
                SetWindowPos(window, nullptr, suggested.left, suggested.top, 0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                self->layout(window);
                return TRUE;
            }
            }
            return FALSE;
        }

      public:
        ~UnsavedChangesDialog() {
            if (font) {
                DeleteObject(font);
            }
            if (headingFont) {
                DeleteObject(headingFont);
            }
            DeleteObject(background);
        }

        static int show(HWND owner) {
            const NativeDialogs::Session session;
            UnsavedChangesDialog dialog;
            struct Template {
                DLGTEMPLATE dialog;
                WORD menu = 0;
                WORD windowClass = 0;
                WORD title = 0;
            } definition{};
            definition.dialog.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
            definition.dialog.dwExtendedStyle = WS_EX_CONTROLPARENT;
            definition.dialog.cx = 320;
            definition.dialog.cy = 140;
            const auto result = DialogBoxIndirectParamW(GetModuleHandleW(nullptr), &definition.dialog,
                                                        NativeDialogs::owner(owner), procedure,
                                                        reinterpret_cast<LPARAM>(&dialog));
            return result == IDYES || result == IDNO ? int(result) : IDCANCEL;
        }
    };
}
