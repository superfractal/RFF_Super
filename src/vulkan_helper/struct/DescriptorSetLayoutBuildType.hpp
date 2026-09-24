//
// Created by Merutilm on 2025-07-12.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <vulkan/vulkan.h>

namespace merutilm::vkh {
    struct DescriptorSetLayoutBuildType {
        VkDescriptorType type;
        VkShaderStageFlags stage;

        bool operator==(const DescriptorSetLayoutBuildType &) const = default;
    };
}
