//
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../attr/VidTimelineAttribute.h"
#include <filesystem>
#include <functional>
#include <opencv2/core/mat.hpp>

namespace merutilm::rff2 {
    struct TimelineAiBundle {
        struct Result {
            std::filesystem::path directory;
            uint32_t completedPages = 0;
            bool cancelled = false;
            std::string error;
        };
        static Result save(const std::filesystem::path &parent, const VidTimelineAttribute &timeline,
                           const std::string &prompt, int side, uint32_t frameCount,
                           const std::function<cv::Mat(uint32_t)> &renderPage,
                           const std::function<bool()> &cancelled,
                           const std::function<void(uint32_t, uint32_t)> &progress);
    };
}
