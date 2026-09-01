//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
// Modified by GPT-5 on 2026-09-01
//

#include "CallbackDebug.hpp"

#include <cstring>
#include <string>
#include <windows.h>

#include "../../vulkan_helper/core/logger.hpp"

namespace merutilm::rff2 {
    namespace {
        void copyToClipboard(const std::wstring &text) {
            if (!OpenClipboard(nullptr)) {
                return;
            }
            EmptyClipboard();
            const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
            if (const HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
                if (void *const target = GlobalLock(handle)) {
                    std::memcpy(target, text.c_str(), bytes);
                    GlobalUnlock(handle);
                    // The clipboard owns the block from here; it only stays ours if it refused it.
                    if (SetClipboardData(CF_UNICODETEXT, handle) == nullptr) {
                        GlobalFree(handle);
                    }
                } else {
                    GlobalFree(handle);
                }
            }
            CloseClipboard();
        }

        // A dump runs to a screenful, and what is wanted from one is usually to paste it somewhere.
        // So it goes three ways at once: the console keeps it to scroll back to, the clipboard makes
        // it paste-able, and the box is what the click itself answers.
        void present(const wchar_t *title, const std::wstring &text) {
            vkh::logger::w_log(L"{}\n{}", title, text);
            copyToClipboard(text);
            MessageBoxW(nullptr, text.c_str(), title, MB_OK | MB_ICONINFORMATION);
        }

    }

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackDebug::DUMP_SCENE_STATE = [
            ](const SettingsMenu &, const RenderScene &scene) {
        present(L"Scene State", scene.dumpState());
    };

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackDebug::SHOW_PASS_TIMES = [
            ](const SettingsMenu &, const RenderScene &scene) {
        present(L"GPU Pass Times", scene.getPassTimingReport());
    };

}
