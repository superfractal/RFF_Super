//
// Created by Merutilm on 2025-09-05.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include "../core/vkh_base.hpp"

namespace merutilm::vkh {
    struct BufferInitInfo {
        VkDeviceSize size;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
    };
}
