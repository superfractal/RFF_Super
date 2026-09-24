//
// Created by Merutilm on 2025-07-15.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>

#include "BufferObject.hpp"

namespace merutilm::vkh {
    class UniformImpl final : public BufferObjectAbstract {
    public:
        explicit UniformImpl(CoreRef core, HostDataObjectManager &&manager, BufferLock bufferLock,
                             bool multiframeEnabled);

        UniformImpl(const UniformImpl &) = delete;

        UniformImpl &operator=(const UniformImpl &) = delete;

        UniformImpl(UniformImpl &&) = delete;

        UniformImpl &operator=(UniformImpl &&) = delete;
    };

    using Uniform = std::unique_ptr<UniformImpl>;
    using UniformPtr = UniformImpl *;
    using UniformRef = UniformImpl &;
}
