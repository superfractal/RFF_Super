//
// Modified by GPT-6 on 2026-09-17, 2026-09-22
//

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

namespace merutilm::rff2 {
    struct MenuAccessibleNode {
        int id = 0, parent = -1;
        std::vector<int> children;
        std::wstring name, accessKey, shortcut;
        RECT bounds{};
        bool menu = false, bar = false, enabled = true, focused = false, expanded = false, expandable = false,
             checkable = false, checked = false;
    };
    class MenuAccessibility {
        struct State;
        class Provider;
        std::shared_ptr<State> state;

      public:
        static constexpr UINT ActionMessage = WM_APP + 0x361;
        enum Action { Focus, Invoke, Expand, Collapse };
        explicit MenuAccessibility(HWND window);
        ~MenuAccessibility();
        LRESULT object(WPARAM flags, LPARAM objectId);
        void update(std::vector<MenuAccessibleNode> nodes, ULONG generation);
    };
} // namespace merutilm::rff2
