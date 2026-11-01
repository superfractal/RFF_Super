//
// Created by Merutilm on 2025-07-13.
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-10-25
//

#include "Core.hpp"

#include "../core/factory.hpp"

namespace merutilm::vkh {

    CoreImpl::CoreImpl() {
        CoreImpl::init();
    }

    CoreImpl::~CoreImpl() {
        CoreImpl::destroy();
    }


    void CoreImpl::init() {
        instance = factory::create<Instance>();
        physicalDevice = factory::create<PhysicalDeviceLoader>(*instance);
        logicalDevice = factory::create<LogicalDevice>(*physicalDevice);
    }


    void CoreImpl::destroy() {
        logicalDevice = nullptr;
        physicalDevice = nullptr;
        instance = nullptr;
    }
}
