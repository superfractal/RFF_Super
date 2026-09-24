//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once

#include <string_view>
#include <windows.h>

namespace merutilm::rff2::workspace {
    struct AccessibleControl {
        static bool describe(HWND window, std::wstring_view name, std::wstring_view help,
                             std::wstring_view automationId = {});
        static bool validation(HWND window, std::wstring_view error);
    };
}
