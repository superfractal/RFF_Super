//
// Modified by GPT-6 on 2026-09-18, 2026-09-19, 2026-09-23, 2026-09-26, 2026-09-30, 2026-10-01
//

#include "TimelineWindow.hpp"
#include "NativeDialogs.hpp"
#include "workspace/TimelineOverlayForm.hpp"
#include "../video/ZoomOverlay.hpp"
#include <commdlg.h>

namespace merutilm::rff2 {
    VidZoomOverlayAttribute &TimelineWindow::selectedOverlay() {
        return editingIterationOverlay ? attribute.video.timeline.maxIterationOverlay : attribute.video.timeline.zoomOverlay;
    }

    void TimelineWindow::refreshOverlaySettings() {
        refreshInspector();
    }

    void TimelineWindow::commitOverlay() {
        attribute.video.timeline.zoomOverlay.showMaxIteration = attribute.video.timeline.maxIterationOverlay.visible;
        attribute.video.animation.showText = attribute.video.timeline.zoomOverlay.visible;
        recordUndoStep();
        if (sourceAttribute) {
            sourceAttribute->video.timeline.zoomOverlay = attribute.video.timeline.zoomOverlay;
            sourceAttribute->video.timeline.maxIterationOverlay = attribute.video.timeline.maxIterationOverlay;
            sourceAttribute->video.timeline.interpolateMaxIteration = attribute.video.timeline.interpolateMaxIteration;
            sourceAttribute->video.animation.showText = attribute.video.animation.showText;
            rememberWorkspaceSource();
        }
        PostMessageW(window, WM_APP + 0x266, 0, 0);
        accessibilityDirty = true;
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::openOverlaySettings() {
        showInspectorSection(2);
    }

    void TimelineWindow::chooseOverlayFont(bool iteration) {
        editingIterationOverlay = iteration;
        if (exporting) {
            return;
        }
        const auto &current = selectedOverlay();
        LOGFONTW font{};
        const auto name = workspace::overlayFontName(current.family);
        wcsncpy_s(font.lfFaceName, name.c_str(), _TRUNCATE);
        font.lfHeight = -24;
        font.lfWeight = current.style & 1 ? FW_BOLD : FW_NORMAL;
        font.lfItalic = (current.style & 2) != 0;
        CHOOSEFONTW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = NativeDialogs::owner(window);
        dialog.lpLogFont = &font;
        dialog.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_TTONLY | CF_SCALABLEONLY | CF_NOSIZESEL;
        NativeDialogs::Session session;
        if (!ChooseFontW(&dialog)) {
            return;
        }
        auto &value = selectedOverlay();
        const auto family = workspace::overlayFontName(std::wstring(font.lfFaceName));
        if (family.empty()) {
            return;
        }
        value.family = family;
        value.style = (font.lfWeight >= FW_BOLD ? 1 : 0) | (font.lfItalic ? 2 : 0);
        value.custom = true;
        lastUndoStep = 0;
        commitOverlay();
    }

    void TimelineWindow::fitOverlayInsideFrame(bool iteration) {
        editingIterationOverlay = iteration;
        auto &renderer = iteration ? iterationOverlayRenderer : overlayRenderer;
        if (exporting || !renderer || IsRectEmpty(&overlayImageRect)) {
            return;
        }
        auto &value = selectedOverlay();
        if (!value.visibleAt(publishedPreviewSeconds)) {
            NativeDialogs::message(window, L"Move the playhead into the display interval before fitting the position.", L"Zoom Overlay", MB_OK);
            return;
        }
        const bool wasCustom = value.custom;
        value.custom = true;
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        const auto bounds = renderer->bounds();
        const float width = float(overlayImageRect.right - overlayImageRect.left);
        const float height = float(overlayImageRect.bottom - overlayImageRect.top);
        if (bounds.width > width || bounds.height > height) {
            value.custom = wasCustom;
            InvalidateRect(window, nullptr, FALSE);
            NativeDialogs::message(window, L"Reduce the font size to fit the overlay inside the video frame.", L"Zoom Overlay", MB_OK);
            return;
        }
        value.x = std::clamp(value.x + (std::clamp(bounds.x, 0, int(width) - bounds.width) - bounds.x) / width,
                             0.f, 1.f);
        value.y = std::clamp(value.y + (std::clamp(bounds.y, 0, int(height) - bounds.height) - bounds.y) / height,
                             0.f, 1.f);
        lastUndoStep = 0;
        commitOverlay();
    }

    void TimelineWindow::resetOverlayAppearance(bool iteration) {
        editingIterationOverlay = iteration;
        if (exporting) {
            return;
        }
        auto &value = selectedOverlay();
        VidZoomOverlayAttribute defaults;
        defaults.visible = value.visible;
        defaults.limitDisplayTime = value.limitDisplayTime;
        defaults.displayStart = value.displayStart;
        defaults.displayEnd = value.displayEnd;
        defaults.showMaxIteration = value.showMaxIteration;
        defaults.anchor = value.anchor;
        defaults.x = value.x;
        defaults.y = value.y;
        defaults.custom = true;
        value = std::move(defaults);
        lastUndoStep = 0;
        commitOverlay();
    }

    void TimelineWindow::resetOverlayPosition(bool iteration) {
        editingIterationOverlay = iteration;
        if (exporting) {
            return;
        }
        auto &value = selectedOverlay();
        value.anchor = 0;
        value.x = .02f;
        value.y = iteration ? .065f : .02f;
        value.custom = true;
        lastUndoStep = 0;
        commitOverlay();
    }
}
