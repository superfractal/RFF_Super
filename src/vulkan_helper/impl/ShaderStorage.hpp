//
// Created by Merutilm on 2025-08-13.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>

#include "BufferObject.hpp"

namespace merutilm::vkh {
    class ShaderStorageImpl final : public BufferObjectAbstract {
    public:
        explicit ShaderStorageImpl(CoreRef core, HostDataObjectManager &&manager, BufferLock bufferLock,
                                   bool multiframeEnabled);

        ShaderStorageImpl(const ShaderStorageImpl &) = delete;

        ShaderStorageImpl &operator=(const ShaderStorageImpl &) = delete;

        ShaderStorageImpl(ShaderStorageImpl &&) = delete;

        ShaderStorageImpl &operator=(ShaderStorageImpl &&) = delete;
    };

    using ShaderStorage = std::unique_ptr<ShaderStorageImpl>;
    using ShaderStoragePtr = ShaderStorageImpl *;
    using ShaderStorageRef = ShaderStorageImpl &;
}
