//
// Created by Merutilm on 2025-08-13.
// Modified by GPT-6 on 2026-09-23
//

#include "ShaderStorage.hpp"
#include <utility>

namespace merutilm::vkh {
    ShaderStorageImpl::ShaderStorageImpl(CoreRef core, HostDataObjectManager &&manager,
                                        const BufferLock bufferLock, const bool multiframeEnabled)
        : BufferObjectAbstract(core, std::move(manager), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               bufferLock, multiframeEnabled) {
    }
}
