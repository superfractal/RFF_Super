//
// Created by Merutilm on 2025-08-10.
// Modified by GPT-6 on 2026-09-20
// Modified by Opus 5.5 on 2026-10-13
//

#pragma once
#include <cstdint>

namespace merutilm::rff2::Constants::VulkanWindow {
    static constexpr uint32_t MAIN_WINDOW_ATTACHMENT_INDEX = 0;
    static constexpr uint32_t VIDEO_WINDOW_ATTACHMENT_INDEX = 1;
    static constexpr uint32_t VIDEO_PREPARATION_WINDOW_ATTACHMENT_INDEX = 2;
    // The SDR video warm-up at start-up. Kept apart from VIDEO_WINDOW so a warm-up that a skip left
    // compiling never holds the context the video window and the timeline preview need.
    static constexpr uint32_t VIDEO_WARMUP_WINDOW_ATTACHMENT_INDEX = 3;
}