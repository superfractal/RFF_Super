//
// Created by Merutilm on 2025-07-10.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../core/vkh_base.hpp"
#include "../core/safe_array.hpp"
#include "../hash/DescriptorSetLayoutBuildTypeHasher.hpp"
#include "../hash/VectorHasher.hpp"
#include "../impl/CombinedImageSampler.hpp"
#include "../impl/ShaderStorage.hpp"
#include "../impl/Uniform.hpp"
#include "../struct/DescriptorSetLayoutBuildType.hpp"
#include "../struct/InputAttachment.hpp"
#include "../struct/StorageImage.hpp"

namespace merutilm::vkh {
    using DescriptorSetLayoutBuilder = std::vector<DescriptorSetLayoutBuildType>;
    using DescriptorSetLayoutBuilderHasher =
        VectorHasher<DescriptorSetLayoutBuildType, DescriptorSetLayoutBuildTypeHasher>;

    using DescriptorType = std::variant<Uniform, ShaderStorage, CombinedImageSampler, InputAttachment, StorageImage>;

    struct DescriptorManagerImpl {
        std::vector<DescriptorType> data = {};
        DescriptorSetLayoutBuilder layoutBuilder = {};

        explicit DescriptorManagerImpl() = default;

        ~DescriptorManagerImpl() = default;

        DescriptorManagerImpl(const DescriptorManagerImpl &) = delete;

        DescriptorManagerImpl &operator=(const DescriptorManagerImpl &) = delete;

        DescriptorManagerImpl(DescriptorManagerImpl &&) noexcept = delete;

        DescriptorManagerImpl &operator=(DescriptorManagerImpl &&) noexcept = delete;

        void appendUBO(const uint32_t bindingExpected, const VkShaderStageFlags useStage,
                       Uniform &&ubo) {
            checkNextBinding(bindingExpected, "Descriptor UBO add");
            data.emplace_back(std::move(ubo));
            layoutBuilder.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, useStage);
        }

        void appendSSBO(const uint32_t bindingExpected, const VkShaderStageFlags useStage,
                        ShaderStorage &&ssbo) {
            checkNextBinding(bindingExpected, "Descriptor SSBO add");
            data.emplace_back(std::move(ssbo));
            layoutBuilder.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, useStage);
        }

        void appendCombinedImgSampler(const uint32_t bindingExpected, const VkShaderStageFlags useStage,
                                      CombinedImageSampler &&sampler) {
            checkNextBinding(bindingExpected, "Descriptor Sampler add");
            data.emplace_back(std::move(sampler));
            layoutBuilder.emplace_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, useStage);
        }

        void appendInputAttachment(const uint32_t bindingExpected, const VkShaderStageFlags useStage) {
            checkNextBinding(bindingExpected, "Descriptor Input Attachment add");
            data.emplace_back(InputAttachment{});
            layoutBuilder.emplace_back(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, useStage);
        }

        void appendStorageImage(const uint32_t bindingExpected, const VkShaderStageFlags useStage) {
            checkNextBinding(bindingExpected, "Descriptor Image2D add");
            data.emplace_back(StorageImage{});
            layoutBuilder.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, useStage);
        }

    private:
        void checkNextBinding(const uint32_t bindingExpected, const std::string &operation) const {
            safe_array::check_index_equal(bindingExpected, static_cast<uint32_t>(data.size()), operation);
        }
    };

    using DescriptorManager = std::unique_ptr<DescriptorManagerImpl>;
    using DescriptorManagerPtr = DescriptorManagerImpl *;
    using DescriptorManagerRef = DescriptorManagerImpl &;
}
