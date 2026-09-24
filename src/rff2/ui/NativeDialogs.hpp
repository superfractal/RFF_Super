//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-22
//

#pragma once
#include "UiLanguage.hpp"
#include "../constants/Win32Constants.hpp"
#include <atomic>
#include <commdlg.h>

namespace merutilm::rff2 {
    class NativeDialogs {
        inline static std::atomic<unsigned> modalCount{0};

        static HWND ownRoot(HWND window) {
            DWORD process = 0;
            if (!window || !IsWindow(window) || !GetWindowThreadProcessId(window, &process) ||
                process != GetCurrentProcessId()) {
                return nullptr;
            }
            return GetAncestor(window, GA_ROOT);
        }

      public:
        struct Session {
            HWND previousFocus = GetFocus();
            Session() {
                modalCount.fetch_add(1, std::memory_order_relaxed);
            }
            ~Session() {
                if (previousFocus && IsWindow(previousFocus) && IsWindowEnabled(previousFocus) &&
                    GetWindowThreadProcessId(previousFocus, nullptr) == GetCurrentThreadId() &&
                    GetFocus() != previousFocus) {
                    SetFocus(previousFocus);
                }
                modalCount.fetch_sub(1, std::memory_order_relaxed);
            }
            Session(const Session &) = delete;
            Session &operator=(const Session &) = delete;
        };

        static HWND mainWindow() {
            HWND found = nullptr;
            EnumWindows(
                [](HWND window, LPARAM data) -> BOOL {
                    DWORD process = 0;
                    GetWindowThreadProcessId(window, &process);
                    wchar_t name[64]{};
                    if (process == GetCurrentProcessId() && GetClassNameW(window, name, 64) &&
                        std::wcscmp(name, Constants::Win32::CLASS_MASTER_WINDOW) == 0) {
                        *reinterpret_cast<HWND *>(data) = window;
                        return FALSE;
                    }
                    return TRUE;
                },
                reinterpret_cast<LPARAM>(&found));
            return found;
        }

        static HWND owner(HWND preferred = nullptr) {
            if (const HWND root = ownRoot(preferred)) {
                return root;
            }
            if (const HWND active = ownRoot(GetActiveWindow())) {
                return active;
            }
            return mainWindow();
        }

        static bool isOpen() {
            return modalCount.load(std::memory_order_relaxed) != 0;
        }

        static int message(HWND preferred, LPCWSTR text, LPCWSTR title, UINT flags) {
            const Session session;
            return MessageBoxW(owner(preferred), UiLanguage::text(text ? text : L"").c_str(),
                               UiLanguage::text(title ? title : L"").c_str(), flags);
        }

        static int message(HWND preferred, LPCSTR text, LPCSTR title, UINT flags) {
            const Session session;
            return MessageBoxW(owner(preferred), UiLanguage::utf8(text ? text : "").c_str(),
                               UiLanguage::utf8(title ? title : "").c_str(), flags);
        }

        static BOOL chooseColor(CHOOSECOLORW *dialog) {
            dialog->hwndOwner = owner(dialog->hwndOwner);
            const Session session;
            return ChooseColorW(dialog);
        }

        static BOOL openFile(OPENFILENAMEW *dialog) {
            dialog->hwndOwner = owner(dialog->hwndOwner);
            const Session session;
            const auto originalTitle = dialog->lpstrTitle;
            const auto title = UiLanguage::text(originalTitle ? originalTitle : L"");
            if (originalTitle) {
                dialog->lpstrTitle = title.c_str();
            }
            const BOOL result = GetOpenFileNameW(dialog);
            dialog->lpstrTitle = originalTitle;
            return result;
        }
    };
} // namespace merutilm::rff2
