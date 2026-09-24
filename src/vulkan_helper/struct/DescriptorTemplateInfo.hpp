//
// Created by Merutilm on 2025-07-19.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "../manage/DescriptorManager.hpp"

namespace merutilm::vkh {
    struct DescriptorTemplateInfo {
        uint32_t id;
        std::function<std::vector<DescriptorManager>(CoreRef)> descriptorGenerator;
    };
}
