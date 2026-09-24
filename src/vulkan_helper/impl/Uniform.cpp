//
// Created by Merutilm on 2025-07-15.
// Modified by GPT-6 on 2026-09-23
//

#include "Uniform.hpp"
#include <utility>

namespace merutilm::vkh {
    UniformImpl::UniformImpl(CoreRef core, HostDataObjectManager &&manager,
                             const BufferLock bufferLock, const bool multiframeEnabled)
        : BufferObjectAbstract(core, std::move(manager), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               bufferLock, multiframeEnabled) {
    }
}
