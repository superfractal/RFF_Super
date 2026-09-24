//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-22, 2026-09-23, 2026-09-24
//

#pragma once
#include <windows.h>
#include <commctrl.h>
#include <functional>
#include <cmath>
#include <cerrno>
#include <cwchar>
#include <cwctype>
#include <string>
#include "AccessibleControl.hpp"

namespace merutilm::rff2::workspace {
    class NumericField {
        HWND window = nullptr;
        std::function<bool(float)> accept;
        std::function<void()> finished;
        std::function<void(int)> advance;
        std::function<void()> validationChanged;
        std::wstring validationHint;
        std::wstring initialText;
        bool invalid = false;
        bool editing = false;
        void finish(bool commit) {
            if (!editing) {
                return;
            }
            if (commit && !submit()) {
                return;
            }
            editing = false;
            ShowWindow(window, SW_HIDE);
            AccessibleControl::validation(window, L"");
            if (finished) {
                finished();
            }
        }
        void publishValidation() {
            AccessibleControl::validation(window, invalid ? validationHint : L"");
            if (validationChanged) {
                validationChanged();
            }
        }
        bool submit() {
            const std::wstring inputText = text();
            if (inputText == initialText) {
                invalid = false;
                publishValidation();
                return true;
            }
            if (inputText.find_first_not_of(L"0123456789+-.eE \t\r\n") != std::wstring::npos) {
                invalid = true;
                publishValidation();
                return false;
            }
            wchar_t *parseEnd = nullptr;
            errno = 0;
            float parsedValue = std::wcstof(inputText.c_str(), &parseEnd);
            const bool parsedAnyNumber = parseEnd != inputText.c_str();
            while (parseEnd && std::iswspace(*parseEnd)) {
                ++parseEnd;
            }
            invalid = !parsedAnyNumber || !parseEnd || *parseEnd != L'\0' || errno == ERANGE ||
                      !std::isfinite(parsedValue) || !accept(parsedValue);
            InvalidateRect(window, nullptr, FALSE);
            publishValidation();
            return !invalid;
        }
        static LRESULT CALLBACK procedure(HWND editWindow, UINT message, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR, DWORD_PTR contextData) {
            auto &self = *reinterpret_cast<NumericField *>(contextData);
            if (message == WM_KEYDOWN && (wParam == VK_RETURN || wParam == VK_ESCAPE || wParam == VK_TAB)) {
                self.finish(wParam != VK_ESCAPE);
                if (!self.editing) {
                    SetFocus(GetParent(editWindow));
                    if (wParam == VK_TAB && self.advance) {
                        self.advance((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                    }
                }
                return 0;
            }
            if (message == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE || wParam == VK_TAB)) {
                return 0;
            }
            if (message == WM_KILLFOCUS) {
                self.finish(true);
                return DefSubclassProc(editWindow, message, wParam, lParam);
            }
            if (message == WM_GETDLGCODE) {
                return DLGC_WANTALLKEYS | DLGC_WANTCHARS;
            }
            return DefSubclassProc(editWindow, message, wParam, lParam);
        }

      public:
        NumericField(HWND parent, HFONT font, std::function<void(int)> next = {},
                     std::function<void()> validation = {})
            : advance(std::move(next)), validationChanged(std::move(validation)) {
            window = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                     WS_CHILD | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 0, 0, 1, 1, parent,
                                     nullptr, GetModuleHandleW(nullptr), nullptr);
            SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            SendMessageW(window, EM_SETLIMITTEXT, 64, 0);
            SetWindowSubclass(window, procedure, 1, reinterpret_cast<DWORD_PTR>(this));
        }
        NumericField(const NumericField &) = delete;
        NumericField &operator=(const NumericField &) = delete;
        ~NumericField() {
            editing = false;
            if (IsWindow(window)) {
                RemoveWindowSubclass(window, procedure, 1);
                DestroyWindow(window);
            }
        }
        void open(RECT rect, std::wstring displayedValue, std::function<bool(float)> setter, std::function<void()> close,
                  std::wstring hint = L"Enter a finite number.", std::wstring name = L"Parameter value",
                  std::wstring automationId = L"surface.value") {
            finish(false);
            accept = std::move(setter);
            finished = std::move(close);
            invalid = false;
            editing = true;
            validationHint = std::move(hint);
            AccessibleControl::describe(window, name, validationHint, automationId);
            AccessibleControl::validation(window, L"");
            SetWindowTextW(window, displayedValue.c_str());
            initialText = std::move(displayedValue);
            SetWindowPos(window, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                         SWP_NOZORDER | SWP_SHOWWINDOW);
            SetFocus(window);
            SendMessageW(window, EM_SETSEL, 0, -1);
        }
        void cancel() {
            finish(false);
        }
        bool commit() {
            finish(true);
            return !editing;
        }
        bool hasPending() const {
            return editing && text() != initialText;
        }
        bool isInvalid() const {
            return editing && invalid;
        }
        bool isEditing() const {
            return editing;
        }
        std::wstring text() const {
            const int textLength = GetWindowTextLengthW(window);
            std::wstring content(textLength + 1, L'\0');
            GetWindowTextW(window, content.data(), textLength + 1);
            content.resize(textLength);
            return content;
        }
        std::wstring_view errorMessage() const {
            return isInvalid() ? std::wstring_view(validationHint) : std::wstring_view{};
        }
        HWND handle() const {
            return window;
        }
    };
} // namespace merutilm::rff2::workspace
