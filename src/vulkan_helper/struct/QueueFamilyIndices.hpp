//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-22
//

#pragma once
#include "../core/vkh_base.hpp"

namespace merutilm::vkh {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsAndComputeFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete() const { return graphicsAndComputeFamily.has_value() && presentFamily.has_value(); }

    };
}
