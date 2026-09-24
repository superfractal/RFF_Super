//
// Modified by GPT-6 on 2026-09-14, 2026-09-21
//

#pragma once
#include <windows.h>
#include <oleacc.h>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace merutilm::rff2::workspace {
    struct AccessibleItem {
        long id = 0;
        std::wstring name, help, action;
        RECT bounds{};
        long role = ROLE_SYSTEM_PUSHBUTTON, state = STATE_SYSTEM_FOCUSABLE;
        HWND native = nullptr;
        std::wstring value;
        bool writable = false;
    };
    class AccessibleItems {
        struct State;
        struct Provider;
        std::shared_ptr<State> state;
        Provider *provider = nullptr;
        std::vector<AccessibleItem> previous;
        std::wstring previousName;

      public:
        AccessibleItems(HWND window, std::function<std::wstring()> name,
                        std::function<std::vector<AccessibleItem>()> items,
                        std::function<bool(long, bool)> operate,
                        std::function<bool(long, std::wstring_view)> write = {});
        ~AccessibleItems();
        AccessibleItems(const AccessibleItems &) = delete;
        AccessibleItems &operator=(const AccessibleItems &) = delete;
        LRESULT object(WPARAM flags) const;
        void changed();
    };
} // namespace merutilm::rff2::workspace
