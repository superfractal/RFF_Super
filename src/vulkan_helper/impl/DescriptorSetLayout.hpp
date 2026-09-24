//
// Created by Merutilm on 2025-07-12.
// Modified by GPT-6 on 2026-09-22, 2026-09-23
//

#pragma once

#include <memory>

#include "Core.hpp"
#include "../handle/CoreHandler.hpp"
#include "../manage/DescriptorManager.hpp"


namespace merutilm::vkh {
    class DescriptorSetLayoutImpl final : public CoreHandler {
        DescriptorSetLayoutBuilder layoutBuilder;
        VkDescriptorSetLayout layout = nullptr;

    public:
        explicit DescriptorSetLayoutImpl(CoreRef core, const DescriptorSetLayoutBuilder &layoutBuilder);

        ~DescriptorSetLayoutImpl() override;

        DescriptorSetLayoutImpl(const DescriptorSetLayoutImpl &) = delete;

        DescriptorSetLayoutImpl &operator=(const DescriptorSetLayoutImpl &) = delete;

        DescriptorSetLayoutImpl(DescriptorSetLayoutImpl &&) = delete;

        DescriptorSetLayoutImpl &operator=(DescriptorSetLayoutImpl &&) = delete;

        [[nodiscard]] VkDescriptorSetLayout getLayoutHandle() const { return layout; }


    private:
        void init() override;

        void destroy() override;
    };

    
    using DescriptorSetLayout = std::unique_ptr<DescriptorSetLayoutImpl>;
    using DescriptorSetLayoutPtr = DescriptorSetLayoutImpl *;
    using DescriptorSetLayoutRef = DescriptorSetLayoutImpl &;
}
