//
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-23, 2026-09-26, 2026-09-30
//

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <windows.h>
#include <opencv2/core.hpp>

#include "../attr/VidZoomOverlayAttribute.h"
#include "../attr/VidHdrTransfer.h"

namespace merutilm::rff2 {
    class ZoomOverlay {
        struct Impl;
        std::unique_ptr<Impl> impl;

    public:
        ZoomOverlay();
        ~ZoomOverlay();

        ZoomOverlay(const ZoomOverlay &) = delete;
        ZoomOverlay &operator=(const ZoomOverlay &) = delete;

        static VidZoomOverlayAttribute zoomStyle(VidZoomOverlayAttribute value, double seconds = 0) {
            value.visible = value.visibleAt(seconds);
            value.showMaxIteration = false;
            return value;
        }
        static VidZoomOverlayAttribute iterationStyle(VidZoomOverlayAttribute value, double seconds = 0) {
            value.visible = value.visibleAt(seconds);
            value.showMaxIteration = value.visible;
            value.visible = false;
            value.custom = true;
            return value;
        }

        static std::string format(double logZoom, uint32_t decimalPlaces = 6);
        static std::string label(double logZoom, const VidZoomOverlayAttribute &style,
                                 std::optional<uint64_t> maxIteration);

        void apply(cv::Mat &image, double logZoom, const VidZoomOverlayAttribute &style,
                   VidHdrTransfer transfer = VidHdrTransfer::SDR,
                   std::optional<uint64_t> maxIteration = {});
        void paint(HDC dc, RECT image, double logZoom, const VidZoomOverlayAttribute &style,
                   std::optional<uint64_t> maxIteration = {});

        [[nodiscard]] std::wstring status() const;
        [[nodiscard]] cv::Rect bounds() const;
    };
}
