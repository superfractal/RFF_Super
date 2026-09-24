//
// Created by Merutilm on 2025-07-13.
// Modified by GPT-6 on 2026-09-17, 2026-09-23
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "../handle/CoreHandler.hpp"
#include "../manage/PipelineLayoutManager.hpp"

namespace merutilm::vkh {
    class PipelineLayoutImpl final : public CoreHandler {

        VkPipelineLayout layout = VK_NULL_HANDLE;
        PipelineLayoutBuilder builders;
        uint32_t descriptorSetLayoutCount;

    public:
        explicit PipelineLayoutImpl(CoreRef core, PipelineLayoutManager &&pipelineLayoutManager);

        ~PipelineLayoutImpl() override;

        PipelineLayoutImpl(const PipelineLayoutImpl &) = delete;

        PipelineLayoutImpl &operator=(const PipelineLayoutImpl &) = delete;

        PipelineLayoutImpl(PipelineLayoutImpl &&) = delete;

        PipelineLayoutImpl &operator=(PipelineLayoutImpl &&) = delete;

        void cmdPush(VkCommandBuffer commandBuffer) const;

        [[nodiscard]] VkPipelineLayout getLayoutHandle() const { return layout; }

        [[nodiscard]] std::vector<DescriptorSetLayoutPtr> getDescriptorSetLayouts() const {
            std::vector<DescriptorSetLayoutPtr> layouts;
            layouts.reserve(descriptorSetLayoutCount);
            for (uint32_t index = 0; index < descriptorSetLayoutCount; ++index) {
                layouts.push_back(std::get<DescriptorSetLayoutPtr>(builders[index]));
            }
            return layouts;
        }

        [[nodiscard]] std::vector<PushConstantPtr> getPushConstants() const {
            std::vector<PushConstantPtr> pushConstants;
            pushConstants.reserve(builders.size() - descriptorSetLayoutCount);
            for (std::size_t index = descriptorSetLayoutCount; index < builders.size(); ++index) {
                pushConstants.push_back(std::get<PushConstantPtr>(builders[index]));
            }
            return pushConstants;
        }

        [[nodiscard]] PushConstantPtr getPushConstant(const uint32_t pushIndex) const {
            return std::get<PushConstantPtr>(builders.at(descriptorSetLayoutCount + pushIndex));
        }

    private:
        void init() override;

        void destroy() override;
    };

    
    using PipelineLayout = std::unique_ptr<PipelineLayoutImpl>;
    using PipelineLayoutPtr = PipelineLayoutImpl *;
    using PipelineLayoutRef = PipelineLayoutImpl &;
}
