//
// Created by Merutilm on 2025-07-08.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include "../handle/Handler.hpp"
#include "ValidationLayer.hpp"

namespace merutilm::vkh {
    class InstanceImpl final : public Handler {
        VkInstance instance = VK_NULL_HANDLE;
        ValidationLayer validationLayer;

    public:
        InstanceImpl();

        ~InstanceImpl() override;

        InstanceImpl(const InstanceImpl &) = delete;

        InstanceImpl &operator=(const InstanceImpl &) = delete;

        InstanceImpl(InstanceImpl &&) = delete;

        InstanceImpl &operator=(InstanceImpl &&) = delete;

        [[nodiscard]] VkInstance getInstanceHandle() const { return instance; }

    private:
        void init() override;

        void createInstance();

        void destroy() override;
    };

    using Instance = std::unique_ptr<InstanceImpl>;
    using InstancePtr = InstanceImpl *;
    using InstanceRef = InstanceImpl &;
}
