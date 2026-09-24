//
// Created by Merutilm on 2025-08-13.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>

#include "../handle/CoreHandler.hpp"

namespace merutilm::vkh {
    class SamplerImpl final : public CoreHandler {

        VkSampler sampler = VK_NULL_HANDLE;
        const VkSamplerCreateInfo samplerInfo;

    public:
        explicit SamplerImpl(CoreRef core, const VkSamplerCreateInfo &samplerInfo);

        ~SamplerImpl() override;

        SamplerImpl(const SamplerImpl &) = delete;
        SamplerImpl &operator=(const SamplerImpl &) = delete;
        SamplerImpl(SamplerImpl &&) = delete;
        SamplerImpl &operator=(SamplerImpl &&) = delete;

        [[nodiscard]] VkSampler getSamplerHandle() const { return sampler; }

    private:
        void init() override;

        void destroy() override;
    };

    using Sampler = std::unique_ptr<SamplerImpl>;
    using SamplerPtr = SamplerImpl *;
    using SamplerRef = SamplerImpl &;
}
