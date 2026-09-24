//
// Created by Merutilm on 2025-07-18.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>
#include <string>

#include "../core/vkh_base.hpp"
#include "GraphicsPipelineConfigurator.hpp"
#include "../handle/WindowContextHandler.hpp"
#include "../impl/IndexBuffer.hpp"
#include "../impl/VertexBuffer.hpp"

namespace merutilm::vkh {
    class GeneralGraphicsPipelineConfigurator : public GraphicsPipelineConfigurator {
        VertexBuffer vertexBuffer = {};
        IndexBuffer indexBuffer = nullptr;

    public:
        explicit GeneralGraphicsPipelineConfigurator(EngineRef engine, const uint32_t windowContextIndex,
                                                     const uint32_t renderContextIndex,
                                                     const uint32_t primarySubpassIndex,
                                                     const std::string &vertName,
                                                     const std::string &fragName)
            : GraphicsPipelineConfigurator(engine, windowContextIndex, renderContextIndex,
                                           primarySubpassIndex, vertName, fragName) {
        }

        ~GeneralGraphicsPipelineConfigurator() override = default;

        GeneralGraphicsPipelineConfigurator(const GeneralGraphicsPipelineConfigurator &) = delete;

        GeneralGraphicsPipelineConfigurator(GeneralGraphicsPipelineConfigurator &&) = delete;

        GeneralGraphicsPipelineConfigurator &operator=(const GeneralGraphicsPipelineConfigurator &) = delete;

        GeneralGraphicsPipelineConfigurator &operator=(GeneralGraphicsPipelineConfigurator &&) = delete;

        void configure() override;

        [[nodiscard]] VertexBufferRef getVertexBuffer() const override {
            return *vertexBuffer;
        }

        [[nodiscard]] IndexBufferRef getIndexBuffer() const override {
            return *indexBuffer;
        }

    };
}
