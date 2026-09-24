//
// Created by Merutilm on 2025-07-15.
// Modified by GPT-6 on 2026-09-23
//

#include "VertexBuffer.hpp"

#include "../manage/HostDataObjectManager.hpp"
#include "../struct/Vertex.hpp"
#include <cstddef>
#include <format>
#include <utility>

namespace merutilm::vkh {
    VertexBufferImpl::VertexBufferImpl(CoreRef core, HostDataObjectManager &&manager,
                                       const BufferLock bufferLock, const bool multiframeEnabled)
        : BufferObjectAbstract(core, std::move(manager), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               bufferLock, multiframeEnabled) {
        VertexBufferImpl::init();
    }

    void VertexBufferImpl::init() {
        const auto &host = getHostObject();
        const uint32_t objectCount = host.getObjectCount();
        for (uint32_t bindingIndex = 0; bindingIndex < objectCount; ++bindingIndex) {
            const uint32_t size = host.sizes[bindingIndex];
            const uint32_t offset = host.offsets[bindingIndex];
            if (const uint32_t vertexCount = host.elements[bindingIndex];
                size != sizeof(Vertex) * vertexCount) {
                throw exception_invalid_args(std::format("size {} and {} is not match", size,
                                                         sizeof(Vertex) * vertexCount));
            }
            bindingDescriptions.emplace_back(bindingIndex, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
            vertexInputAttributeDescriptions.emplace_back(0, bindingIndex, getFormat<decltype(Vertex::position)>(),
                                                          offset + offsetof(Vertex, position));
            vertexInputAttributeDescriptions.emplace_back(1, bindingIndex, getFormat<decltype(Vertex::color)>(),
                                                          offset + offsetof(Vertex, color));
            vertexInputAttributeDescriptions.emplace_back(2, bindingIndex, getFormat<decltype(Vertex::texcoord)>(),
                                                          offset + offsetof(Vertex, texcoord));
        }
    }
}
