//
// Created by Merutilm on 2025-09-05.
// Modified by GPT-5 on 2026-08-23
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-23
//

#include "GPCPresent.hpp"
#include <algorithm>
#include <cstddef>
#include <utility>

#include "SharedImageContextIndices.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../constants/VulkanWindowConstants.hpp"

namespace merutilm::rff2 {
    namespace {
        bool srgbAttachment(const VkFormat format) {
            switch (format) {
                case VK_FORMAT_R8_SRGB:
                case VK_FORMAT_R8G8_SRGB:
                case VK_FORMAT_R8G8B8_SRGB:
                case VK_FORMAT_B8G8R8_SRGB:
                case VK_FORMAT_R8G8B8A8_SRGB:
                case VK_FORMAT_B8G8R8A8_SRGB:
                case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
                    return true;
                default:
                    return false;
            }
        }
    }

    void GPCPresent::updateQueue(vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
        //no operation
        const auto secondaryImageIndex = wc.getAttachmentIndex() == Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX ?
            SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY : SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY;
        auto frameImages = wc.getSharedImageContext().getImageContextMF(secondaryImageIndex);
        if (heldFrame >= 0) {
            const auto heldImage = frameImages.at(static_cast<size_t>(heldFrame));
            std::fill(frameImages.begin(), frameImages.end(), heldImage);
        }
        auto &presentDescriptor = getDescriptor(SET_PRESENT);
        presentDescriptor.get<vkh::CombinedImageSampler>(0, BINDING_PRESENT_SAMPLER)->setImageContextMF(frameImages);
        presentDescriptor.queue(queue, frameIndex, {}, {BINDING_PRESENT_SAMPLER});
    }


    void GPCPresent::setRescaledResolution(const glm::uvec2 &newResolution) const {
        auto &presentDescriptor = getDescriptor(SET_PRESENT);
        auto &resolutionUniform = *presentDescriptor.get<vkh::Uniform>(0, BINDING_PRESENT_UBO);
        auto &resolutionHost = resolutionUniform.getHostObject();
        resolutionHost.set<glm::uvec2>(TARGET_PRESENT_UBO_EXTENT, newResolution);
        resolutionHost.set<uint32_t>(TARGET_PRESENT_UBO_SRGB,
                                     srgbAttachment(wc.getSwapchain().getImageFormat()) ? 1u : 0u);
        resolutionUniform.update();
    }

    void GPCPresent::pipelineInitialized() {
        writeDescriptorMF([this](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            getDescriptor(SET_PRESENT).queue(queue, frameIndex, {}, {BINDING_PRESENT_UBO});
        });
    }


    void GPCPresent::renderContextRefreshed() {
        auto &sharedImages = wc.getSharedImageContext();
        auto &presentDescriptor = getDescriptor(SET_PRESENT);

        switch (wc.getAttachmentIndex()) {
            case Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX: {
                presentDescriptor.get<vkh::CombinedImageSampler>(0, BINDING_PRESENT_SAMPLER)->
                        setImageContextMF(sharedImages.getImageContextMF(SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY));
                break;
            }
            case Constants::VulkanWindow::VIDEO_PREPARATION_WINDOW_ATTACHMENT_INDEX:
            case Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX: {
                presentDescriptor.get<vkh::CombinedImageSampler>(0, BINDING_PRESENT_SAMPLER)->
                        setImageContextMF(sharedImages.getImageContextMF(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY));
                break;
            }
            default: {
                //noop
            }
        }

        writeDescriptorMF([&presentDescriptor](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            presentDescriptor.queue(queue, frameIndex, {}, {BINDING_PRESENT_SAMPLER});
        });
    }


    void GPCPresent::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //noop
        auto manager = vkh::factory::create<vkh::HostDataObjectManager>();
        manager->reserve<glm::vec4>(0);
        zoomPush = vkh::factory::create<vkh::PushConstant>(VK_SHADER_STAGE_FRAGMENT_BIT, std::move(manager));
        setZoomTransform(1.0f, 0.0f, 0.0f);
        pipelineLayoutManager.appendPushConstantManager(zoomPush.get());
    }

    void GPCPresent::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        vkh::SamplerRef sampler = pickFromGlobalRepository<vkh::GlobalSamplerRepo, vkh::SamplerRef>(
            VkSamplerCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .mipLodBias = 0,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 0,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0,
                .maxLod = 0,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                .unnormalizedCoordinates = VK_FALSE
            });
        auto descManager = vkh::factory::create<vkh::DescriptorManager>();
        auto combinedSampler = vkh::factory::create<vkh::CombinedImageSampler>(wc.core, sampler, true);
        descManager->appendCombinedImgSampler(BINDING_PRESENT_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                        std::move(combinedSampler));
        auto uboManager = vkh::factory::create<vkh::HostDataObjectManager>();
        uboManager->reserve<glm::uvec2>(TARGET_PRESENT_UBO_EXTENT);
        uboManager->reserve<uint32_t>(TARGET_PRESENT_UBO_SRGB, 4);
        descManager->appendUBO(BINDING_PRESENT_UBO, VK_SHADER_STAGE_FRAGMENT_BIT,
                               vkh::factory::create<vkh::Uniform>(wc.core, std::move(uboManager),
                                                                  vkh::BufferLock::LOCK_UNLOCK, false));

        appendUniqueDescriptor(SET_PRESENT, descriptors, std::move(descManager));
    }
}
