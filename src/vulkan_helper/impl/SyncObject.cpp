//
// Created by Merutilm on 2025-07-14.
// Modified by GPT-6 on 2026-09-23
//

#include "SyncObject.hpp"
#include "../core/factory.hpp"
#include "../core/exception.hpp"

namespace merutilm::vkh {
    SyncObjectImpl::SyncObjectImpl(CoreRef core) : CoreHandler(core) {
        SyncObjectImpl::init();
    }

    SyncObjectImpl::~SyncObjectImpl() {
        SyncObjectImpl::destroy();
    }

    void SyncObjectImpl::init() {
        if (!fences.empty() || !semaphores.empty()) {
            throw exception_invalid_state("Sync objects are already initialized");
        }
        const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();
        std::vector<Fence> createdFences;
        std::vector<Semaphore> createdSemaphores;
        createdFences.reserve(maxFramesInFlight);
        createdSemaphores.reserve(maxFramesInFlight);
        for (uint32_t frameIndex = 0; frameIndex < maxFramesInFlight; ++frameIndex) {
            createdFences.push_back(factory::create<Fence>(core));
            createdSemaphores.push_back(factory::create<Semaphore>(core));
        }
        fences.swap(createdFences);
        semaphores.swap(createdSemaphores);
    }

    void SyncObjectImpl::destroy() {
        semaphores.clear();
        fences.clear();
    }
}
