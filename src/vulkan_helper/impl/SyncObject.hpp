//
// Created by Merutilm on 2025-07-14.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <vector>

#include "Fence.hpp"
#include "Semaphore.hpp"
#include "../handle/CoreHandler.hpp"

namespace merutilm::vkh {
    class SyncObjectImpl final : public CoreHandler {
        std::vector<Fence> fences = {};
        std::vector<Semaphore> semaphores = {};
    public:
        explicit SyncObjectImpl(CoreRef core);

        ~SyncObjectImpl() override;

        SyncObjectImpl(const SyncObjectImpl &) = delete;

        SyncObjectImpl &operator=(const SyncObjectImpl &) = delete;

        SyncObjectImpl(SyncObjectImpl &&) = delete;

        SyncObjectImpl &operator=(SyncObjectImpl &&) = delete;

        [[nodiscard]] SemaphoreRef getSemaphore(const uint32_t frameIndex) const {
            return *semaphores[frameIndex];
        }

        [[nodiscard]] FenceRef getFence(const uint32_t frameIndex) const { return *fences[frameIndex]; }

    private:
        void init() override;

        void destroy() override;
    };

    using SyncObject = std::unique_ptr<SyncObjectImpl>;
    using SyncObjectPtr = SyncObjectImpl *;
    using SyncObjectRef = SyncObjectImpl &;
}
