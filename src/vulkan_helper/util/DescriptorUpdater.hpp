//
// Created by Merutilm on 2025-08-15.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "../context/DescriptorUpdateContext.hpp"
#include "../core/allocator.hpp"

namespace merutilm::vkh {
    struct DescriptorUpdater {
        DescriptorUpdater() = delete;

        static DescriptorUpdateQueue createQueue() {
            return {};
        }

        static void write(const VkDevice device, const DescriptorUpdateQueue &queue) {
            std::vector<VkWriteDescriptorSet> writeDescriptorSets(queue.size());
            std::ranges::transform(
                queue, writeDescriptorSets.begin(),
                [](const DescriptorUpdateContext &context) { return context.writeSet; });

            allocator::invoke(vkUpdateDescriptorSets, device,
                              static_cast<uint32_t>(writeDescriptorSets.size()),
                              writeDescriptorSets.data(), 0, nullptr);
        }
    };
}
