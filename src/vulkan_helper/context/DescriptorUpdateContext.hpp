//
// Created by Merutilm on 2025-08-01.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <list>
#include <vulkan/vulkan.h>

namespace merutilm::vkh {
    struct DescriptorUpdateContext {
        VkWriteDescriptorSet writeSet;
        VkDescriptorBufferInfo bufferInfo;
        VkDescriptorImageInfo imageInfo;
    };

    using DescriptorUpdateQueue = std::list<DescriptorUpdateContext>;
}
