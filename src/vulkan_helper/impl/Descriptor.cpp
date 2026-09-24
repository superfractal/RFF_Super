//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-22, 2026-09-23
//

#include "Descriptor.hpp"


namespace merutilm::vkh {
    DescriptorImpl::DescriptorImpl(CoreRef core, DescriptorSetLayoutRef descriptorSetLayout,
                                   std::vector<DescriptorManager> &&descriptorManager) : CoreHandler(core),
        descriptorSetLayout(descriptorSetLayout),
        data([&descriptorManager] {
            std::vector<std::vector<DescriptorType>> descriptorTypes(descriptorManager.size());
            std::transform(
                std::make_move_iterator(descriptorManager.begin()),
                std::make_move_iterator(descriptorManager.end()),
                descriptorTypes.begin(), [](DescriptorManager &&manager) {
                return std::move(manager->data);
            });
            return descriptorTypes;
        }()) {
        DescriptorImpl::init();
    }

    DescriptorImpl::~DescriptorImpl() {
        DescriptorImpl::destroy();
    }



    void DescriptorImpl::queue(DescriptorUpdateQueue &updateQueue, const uint32_t frameIndex,
                               DescIndexPicker &&descIndices,
                               DescIndexPicker &&bindings) const {
        if (descIndices.empty()) {
            const uint32_t descriptorCount = getDescriptorCount();
            descIndices = std::vector<uint32_t>(descriptorCount);
            std::iota(descIndices.begin(), descIndices.end(), 0);
        }


        if (bindings.empty()) {
            const uint32_t elementCount = getDescriptorElements();
            bindings = std::vector<uint32_t>(elementCount);
            std::iota(bindings.begin(), bindings.end(), 0);
        }

        auto selectedBindings = std::move(bindings);
        std::ranges::sort(selectedBindings);
        const auto duplicates = std::ranges::unique(selectedBindings);
        selectedBindings.erase(duplicates.begin(), duplicates.end());

        updateIndices(updateQueue, frameIndex, std::move(descIndices), selectedBindings);
    }

    void DescriptorImpl::updateIndices(DescriptorUpdateQueue &updateQueue, const uint32_t frameIndex,
                                       const std::vector<uint32_t> &descIndices,
                                       const std::vector<uint32_t> &bindings) const {
        const auto queueBufferWrite = [&](BufferObjectAbstract &buffer, const VkDescriptorType descriptorType,
                                          const uint32_t descriptorIndex, const uint32_t binding) {
            updateQueue.push_back({
                .bufferInfo = VkDescriptorBufferInfo{
                    .buffer = buffer.isMultiframe() ? buffer.getBufferContextMF(frameIndex).buffer
                                                    : buffer.getBufferContext().buffer,
                    .offset = 0,
                    .range = buffer.getHostObject().getTotalSizeByte()
                },
            });
            updateQueue.back().writeSet = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descriptorSets[frameIndex][descriptorIndex],
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = descriptorType,
                .pImageInfo = nullptr,
                .pBufferInfo = &updateQueue.back().bufferInfo,
                .pTexelBufferView = nullptr,
            };
        };

        for (const uint32_t descIndex: descIndices) {
            for (const uint32_t binding: bindings) {
                const auto &raw = getRaw(descIndex, binding);
                if (std::holds_alternative<Uniform>(raw)) {
                    queueBufferWrite(*std::get<Uniform>(raw), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                     descIndex, binding);
                } else if (std::holds_alternative<ShaderStorage>(raw)) {
                    queueBufferWrite(*std::get<ShaderStorage>(raw), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                     descIndex, binding);
                } else if (std::holds_alternative<CombinedImageSampler>(raw)) {
                    auto &tex = std::get<CombinedImageSampler>(raw);

                    updateQueue.push_back({
                        .imageInfo = VkDescriptorImageInfo{
                            .sampler = tex->getSampler().getSamplerHandle(),
                            .imageView = tex->isMultiframe() ? tex->getImageContextMF()[frameIndex].mipmappedImageView : tex->getImageContext().mipmappedImageView,
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        }
                    });

                    updateQueue.back().writeSet = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstSet = descriptorSets[frameIndex][descIndex],
                        .dstBinding = binding,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &updateQueue.back().imageInfo,
                        .pBufferInfo = nullptr,
                        .pTexelBufferView = nullptr,
                    };
                } else if (std::holds_alternative<InputAttachment>(raw)) {
                    const auto &[ctx] = std::get<InputAttachment>(raw);
                    updateQueue.push_back({
                        .imageInfo = VkDescriptorImageInfo{
                            .sampler = VK_NULL_HANDLE,
                            .imageView = ctx[frameIndex].mipmappedImageView,
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                        }
                    });

                    updateQueue.back().writeSet = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstSet = descriptorSets[frameIndex][descIndex],
                        .dstBinding = binding,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                        .pImageInfo = &updateQueue.back().imageInfo,
                        .pBufferInfo = nullptr,
                        .pTexelBufferView = nullptr,
                    };
                } else if (std::holds_alternative<StorageImage>(raw)) {
                    const auto &[ctx] = std::get<StorageImage>(raw);
                    updateQueue.push_back({
                        .imageInfo = VkDescriptorImageInfo{
                            .sampler = VK_NULL_HANDLE,
                            .imageView = ctx[frameIndex].mipmappedImageView,
                            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
                        }
                    });

                    updateQueue.back().writeSet = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = nullptr,
                        .dstSet = descriptorSets[frameIndex][descIndex],
                        .dstBinding = binding,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .pImageInfo = &updateQueue.back().imageInfo,
                        .pBufferInfo = nullptr,
                        .pTexelBufferView = nullptr,
                    };
                }
            }
        }
    }


    void DescriptorImpl::init() {
        const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();
        const uint32_t ubo = getElementCount<Uniform>();
        const uint32_t ssbo = getElementCount<ShaderStorage>();
        const uint32_t sampler = getElementCount<CombinedImageSampler>();
        const uint32_t inputAttachment = getElementCount<InputAttachment>();
        const uint32_t storageImage = getElementCount<StorageImage>();
        const uint32_t elements = getDescriptorElements();

        const uint32_t descriptorCount = getDescriptorCount();
        std::vector<VkDescriptorPoolSize> sizes = {};

        if (ubo > 0) {
            sizes.push_back({
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = ubo * descriptorCount
            });
        }
        if (ssbo > 0) {
            sizes.push_back({
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = ssbo * descriptorCount
            });
        }

        if (sampler > 0) {
            sizes.push_back({
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = sampler * descriptorCount
            });
        }
        if (inputAttachment > 0) {
            sizes.push_back({
                .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount = inputAttachment * descriptorCount
            });
        }
        if (storageImage > 0) {
            sizes.push_back({
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = storageImage * descriptorCount
            });
        }

        const VkDescriptorPoolCreateInfo descriptorPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = elements * descriptorCount,
            .poolSizeCount = static_cast<uint32_t>(sizes.size()),
            .pPoolSizes = sizes.data()
        };
        descriptorPools.resize(maxFramesInFlight);

        for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
            if (allocator::invoke(vkCreateDescriptorPool, core.getLogicalDevice().getLogicalDeviceHandle(), &descriptorPoolInfo, nullptr,
                                       &descriptorPools[i]) != VK_SUCCESS) {
                throw exception_init("Failed to create descriptor pool!");
            }
        }

        std::vector layouts(descriptorCount, descriptorSetLayout.getLayoutHandle());

        descriptorSets.resize(maxFramesInFlight);

        for (int i = 0; i < maxFramesInFlight; ++i) {
            descriptorSets[i].resize(descriptorCount);

            if (const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = descriptorPools[i],
                .descriptorSetCount = descriptorCount,
                .pSetLayouts = layouts.data()
            }; allocator::invoke(vkAllocateDescriptorSets, core.getLogicalDevice().getLogicalDeviceHandle(), &descriptorSetAllocateInfo,
                                        descriptorSets[i].data()) != VK_SUCCESS) {
                throw exception_init("Failed to allocate descriptor sets!");
            }
        }
    }

    void DescriptorImpl::destroy() {
        if (getDescriptorElements() == 0) {
            return;
        }
        const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();

        for (int i = 0; i < maxFramesInFlight; ++i) {
            allocator::invoke(vkDestroyDescriptorPool, core.getLogicalDevice().getLogicalDeviceHandle(), descriptorPools[i], nullptr);
        }
    }
}
