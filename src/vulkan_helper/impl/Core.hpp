//
// Created by Merutilm on 2025-07-13.
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-10-25
//

#pragma once
#include "Instance.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDeviceLoader.hpp"

namespace merutilm::vkh {
    class CoreImpl final : public Handler {
        Instance instance = nullptr;
        PhysicalDeviceLoader physicalDevice = nullptr;
        LogicalDevice logicalDevice = nullptr;

    public:
        explicit CoreImpl();

        ~CoreImpl() override;

        CoreImpl(const CoreImpl &) = delete;

        CoreImpl &operator=(const CoreImpl &) = delete;

        CoreImpl(CoreImpl &&) = delete;

        CoreImpl &operator=(CoreImpl &&) = delete;

        [[nodiscard]] InstanceRef getInstance() const {
            return *instance;
        }

        [[nodiscard]] PhysicalDeviceLoaderRef getPhysicalDevice() const {
            return *physicalDevice;
        }

        [[nodiscard]] LogicalDeviceRef getLogicalDevice() const {
            return *logicalDevice;
        }

    private:
        void init() override;

        void destroy() override;
    };

    using Core = std::unique_ptr<CoreImpl>;
    using CorePtr = CoreImpl *;
    using CoreRef = CoreImpl &;
}
