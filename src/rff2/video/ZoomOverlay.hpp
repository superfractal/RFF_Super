//
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-23
//

#pragma once

#include <cstdint>
#include <memory>
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

        static std::string format(double logZoom, uint32_t decimalPlaces = 6);

        void apply(cv::Mat &image, double logZoom, const VidZoomOverlayAttribute &style,
                   VidHdrTransfer transfer = VidHdrTransfer::SDR);
        void paint(HDC dc, RECT image, double logZoom, const VidZoomOverlayAttribute &style);

        [[nodiscard]] std::wstring status() const;
        [[nodiscard]] cv::Rect bounds() const;
    };
}
