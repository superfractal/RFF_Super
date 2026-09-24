//
// Modified by GPT-6 on 2026-09-17, 2026-09-22
//

#pragma once
#include "MenuModel.hpp"
#include <functional>
#include <memory>

namespace merutilm::rff2 {
    class CustomMenu {
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        CustomMenu(HWND owner, std::function<MenuModel()> read, std::function<void(UINT)> dispatch);
        ~CustomMenu();
        CustomMenu(const CustomMenu &) = delete;
        CustomMenu &operator=(const CustomMenu &) = delete;
        bool filter(const MSG &message);
        void refresh();
        void layout(int width);
        int height() const;
        void cancel();
        static bool requested();
    };
} // namespace merutilm::rff2
