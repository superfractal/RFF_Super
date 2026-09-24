//
// Modified by GPT-6 on 2026-09-14, 2026-09-21
//

#pragma once
#include "WorkspaceForm.hpp"
#include <memory>
#include <windows.h>

namespace merutilm::rff2 {
    class RenderScene;
}
namespace merutilm::rff2::workspace {
    class AppearanceEditTracker;
    class ComparisonWorkspace : public std::enable_shared_from_this<ComparisonWorkspace> {
        struct State;
        std::unique_ptr<State> state;

      public:
        ComparisonWorkspace(RenderScene &scene, HWND parent, HWND canvas,
                            std::shared_ptr<AppearanceEditTracker> edits = {});
        ~ComparisonWorkspace();
        WorkspaceForm form();
        void applyDpi(UINT dpi);
        void applyTheme();
        void update(bool suspended = false);
        bool active() const;
        void clear();
    };
} // namespace merutilm::rff2::workspace
