//
// Modified by GPT-6 on 2026-09-17, 2026-09-23
//

#pragma once

#include "WorkspaceTheme.hpp"

#include <functional>
#include <memory>
#include <string_view>

namespace merutilm::rff2::workspace {
    struct WorkspaceContentContext {
        HWND parent;
        HFONT font;
        float scale;
        const WorkspaceTheme& theme;
        std::function<void()> changed;
        std::function<bool()> editable;
        std::function<void(bool)> history;
        std::function<void(int)> leaveFocus;
    };

    class WorkspaceContent {
    public:
        virtual ~WorkspaceContent() = default;
        virtual void layout(int width, int height) = 0;
        virtual void refresh() = 0;
        virtual void metrics(HFONT font, float scale) = 0;
        virtual void themeChanged() = 0;
        virtual void focus(std::string_view field = {}) = 0;
    };
}
