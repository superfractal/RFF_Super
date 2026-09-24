//
// Created by Merutilm on 2025-07-14.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include "../context/ImageContext.hpp"

namespace merutilm::vkh {
    struct RenderPassAttachment {
        VkAttachmentDescription attachment;
        MultiframeImageContext imageContext;
    };
}
