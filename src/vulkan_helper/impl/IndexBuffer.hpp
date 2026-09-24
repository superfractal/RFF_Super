//
// Created by Merutilm on 2025-07-18.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>

#include "BufferObject.hpp"

namespace merutilm::vkh {
    class IndexBufferImpl final : public BufferObjectAbstract {
    public:
        explicit IndexBufferImpl(CoreRef core, HostDataObjectManager &&manager, BufferLock bufferLock,
                                 bool multiframeEnabled);

        IndexBufferImpl(const IndexBufferImpl &) = delete;

        IndexBufferImpl &operator=(const IndexBufferImpl &) = delete;

        IndexBufferImpl(IndexBufferImpl &&) = delete;

        IndexBufferImpl &operator=(IndexBufferImpl &&) = delete;
    };

    using IndexBuffer = std::unique_ptr<IndexBufferImpl>;
    using IndexBufferPtr = IndexBufferImpl *;
    using IndexBufferRef = IndexBufferImpl &;
}
