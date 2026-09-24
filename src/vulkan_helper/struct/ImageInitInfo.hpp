//
// Created by Merutilm on 2025-07-14.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include "../core/vkh_base.hpp"

namespace merutilm::vkh {
    struct ImageInitInfo {
        VkImageType imageType;
        VkImageViewType imageViewType;
        VkFormat imageFormat;
        VkExtent3D extent;
        bool useMipmap;
        uint32_t arrayLayers;
        VkSampleCountFlagBits samples;
        VkImageTiling imageTiling;
        VkImageUsageFlags usage;
        VkImageLayout initialLayout;
        VkMemoryPropertyFlags properties;
    };
}
