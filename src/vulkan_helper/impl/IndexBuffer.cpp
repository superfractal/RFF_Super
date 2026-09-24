//
// Created by Merutilm on 2025-07-18.
// Modified by GPT-6 on 2026-09-23
//

#include "IndexBuffer.hpp"
#include <utility>

namespace merutilm::vkh {
    IndexBufferImpl::IndexBufferImpl(CoreRef core, HostDataObjectManager &&manager,
                                     const BufferLock bufferLock, const bool multiframeEnabled)
        : BufferObjectAbstract(core, std::move(manager), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                               bufferLock, multiframeEnabled) {
    }
}
