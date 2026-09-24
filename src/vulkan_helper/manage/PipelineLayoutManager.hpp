//
// Created by Merutilm on 2025-07-18.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "../impl/PushConstant.hpp"
#include "../impl/DescriptorSetLayout.hpp"

namespace merutilm::vkh {

    using PipelineLayoutBuildType = std::variant<DescriptorSetLayoutPtr, PushConstantPtr>;
    using PipelineLayoutBuilder = std::vector<PipelineLayoutBuildType>;

    struct PipelineLayoutManagerImpl {
        PipelineLayoutBuilder builders;
        uint32_t descriptorSetLayoutCount = 0;

        PipelineLayoutManagerImpl() = default;
        ~PipelineLayoutManagerImpl() = default;

        PipelineLayoutManagerImpl(const PipelineLayoutManagerImpl &) = delete;
        PipelineLayoutManagerImpl &operator=(const PipelineLayoutManagerImpl &) = delete;
        PipelineLayoutManagerImpl(PipelineLayoutManagerImpl &&) = delete;
        PipelineLayoutManagerImpl &operator=(PipelineLayoutManagerImpl &&) = delete;

        bool operator==(const PipelineLayoutManagerImpl &) const = default;

        void appendDescriptorSetLayout(DescriptorSetLayoutPtr descriptorSetLayout) {
            builders.emplace_back(descriptorSetLayout);
            ++descriptorSetLayoutCount;
        }

        void appendPushConstantManager(PushConstantPtr pushConstant) {
            builders.emplace_back(pushConstant);
        }
    };

    using PipelineLayoutManager = std::unique_ptr<PipelineLayoutManagerImpl>;
    using PipelineLayoutManagerPtr = PipelineLayoutManagerImpl *;
    using PipelineLayoutManagerRef = PipelineLayoutManagerImpl &;
}
