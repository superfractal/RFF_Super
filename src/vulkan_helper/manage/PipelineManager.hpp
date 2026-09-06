//
// Created by Merutilm on 2025-07-11.
// Modified by Fable 5.1 on 2026-09-06
//

#pragma once
#include "../core/vkh_base.hpp"
#include "../impl/Descriptor.hpp"
#include "../impl/PipelineLayout.hpp"
#include "../impl/ShaderModule.hpp"

namespace merutilm::vkh {
    struct PipelineManagerImpl {
        PipelineLayoutRef layout;
        std::vector<DescriptorPtr> descriptors = {};
        std::vector<ShaderModulePtr> shaderModules = {};
        // Specialization constants, one 32-bit word per constant_id starting at 0. Empty means none.
        std::vector<uint32_t> specialization = {};

        explicit PipelineManagerImpl(PipelineLayoutRef layout) : layout(layout) {
        }


        void attachShader(ShaderModulePtr shaderStage) {
            shaderModules.emplace_back(shaderStage);
        }

        void attachDescriptor(std::vector<DescriptorPtr> &&descriptor) { descriptors = std::move(descriptor); }

        void attachSpecialization(std::vector<uint32_t> &&data) { specialization = std::move(data); }

    };

    using PipelineManager = std::unique_ptr<PipelineManagerImpl>;
    using PipelineManagerPtr = PipelineManagerImpl *;
    using PipelineManagerRef = PipelineManagerImpl &;
}
