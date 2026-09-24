//
// Modified by GPT-6 on 2026-09-18, 2026-09-21, 2026-09-22
//

#include "TimelineWindow.hpp"
#include <algorithm>

namespace merutilm::rff2 {
    void TimelineWindow::applyLayoutPreset(int previewPercent, bool showPreview, bool showTracks) {
        if (exporting) {
            return;
        }
        dockSplitter->cancel();
        dockState.previewPercent = std::clamp(previewPercent, 10, 90);
        dockState.collapsed = !showTracks;
        dockState.previewHidden = !showPreview && showTracks;
        layoutWorkspaceDock();
        saveWorkspaceDock();
    }

}
