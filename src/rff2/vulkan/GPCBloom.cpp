//
// Created by Merutilm on 2025-08-30.
// Modified by GPT-5 on 2026-08-18
// Modified by Opus 5 on 2026-08-19, 2026-08-24
//

#include "GPCBloom.hpp"

#include "RCCDownsampleForBlur.hpp"
#include "RCC3.hpp"
#include "SharedDescriptorTemplate.hpp"
#include "GPCSlope.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../constants/VulkanWindowConstants.hpp"

namespace merutilm::rff2 {
    void GPCBloom::updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) {
        //no operation
    }

    void GPCBloom::setBloom(const ShdBloomAttribute &bloom, const ShdHdrAttribute &hdr) const {
        using namespace SharedDescriptorTemplate;
        auto &bloomDesc = getDescriptor(SET_BLOOM);
        auto &bloomUBO = *bloomDesc.get<vkh::Uniform>(0, DescBloom::BINDING_UBO_BLOOM);
        auto &bloomUBOHost = bloomUBO.getHostObject();

        const bool lockAfterUpdate = wc.getAttachmentIndex() ==
                                     Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX;
        if (bloomUBO.isLocked()) {
            bloomUBO.unlock(wc.getCommandPool());
        }

        bloomUBOHost.set<float>(DescBloom::TARGET_BLOOM_THRESHOLD, bloom.threshold);
        bloomUBOHost.set<float>(DescBloom::TARGET_BLOOM_RADIUS, bloom.radius);
        bloomUBOHost.set<float>(DescBloom::TARGET_BLOOM_SOFTNESS, bloom.softness);
        bloomUBOHost.set<float>(DescBloom::TARGET_BLOOM_INTENSITY, bloom.intensity);
        bloomUBOHost.set<float>(DescBloom::TARGET_BLOOM_HDR, hdr.use ? 1.0f : 0.0f);
        bloomUBOHost.set<float>(DescBloom::TARGET_BLOOM_HEADROOM, std::max(hdr.headroom, 1e-3f));
        bloomUBOHost.set<float>(DescBloom::TARGET_BLOOM_LINEAR_ADD, bloom.linearAdd ? 1.0f : 0.0f);
        bloomUBO.update();
        if (lockAfterUpdate) {
            bloomUBO.lock(wc.getCommandPool());
        }

        writeDescriptorMF([&bloomDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            bloomDesc.queue(queue, frameIndex, {}, {DescBloom::BINDING_UBO_BLOOM});
        });
    }

    void GPCBloom::setBloomDynamic(const ShdBloomAttribute &bloom, const ShdHdrAttribute &hdr) const {
        using namespace SharedDescriptorTemplate;
        const auto &bloomUBO = *getDescriptor(SET_BLOOM).get<vkh::Uniform>(0, DescBloom::BINDING_UBO_BLOOM);
        auto &host = bloomUBO.getHostObject();
        host.set<float>(DescBloom::TARGET_BLOOM_THRESHOLD, bloom.threshold);
        host.set<float>(DescBloom::TARGET_BLOOM_RADIUS, bloom.radius);
        host.set<float>(DescBloom::TARGET_BLOOM_SOFTNESS, bloom.softness);
        host.set<float>(DescBloom::TARGET_BLOOM_INTENSITY, bloom.intensity);
        host.set<float>(DescBloom::TARGET_BLOOM_HDR, hdr.use ? 1.0f : 0.0f);
        host.set<float>(DescBloom::TARGET_BLOOM_HEADROOM, std::max(hdr.headroom, 1e-3f));
        host.set<float>(DescBloom::TARGET_BLOOM_LINEAR_ADD, bloom.linearAdd ? 1.0f : 0.0f);
        bloomUBO.update();
    }

    void GPCBloom::pipelineInitialized() {
        //noop
    }

    void GPCBloom::renderContextRefreshed() {
        using namespace SharedDescriptorTemplate;
        auto &sic = wc.getSharedImageContext();
        auto &bloomDesc = getDescriptor(SET_BLOOM_CANVAS);

        switch (wc.getAttachmentIndex()) {
            case Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX: {
                bloomDesc.get<vkh::CombinedImageSampler>(0, BINDING_BLOOM_CANVAS_ORIGINAL)->setImageContextMF(
                    sic.getImageContextMF(
                        SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY));
                bloomDesc.get<vkh::CombinedImageSampler>(0, BINDING_BLOOM_CANVAS_BLURRED)->setImageContextMF(
                    sic.getImageContextMF(
                        SharedImageContextIndices::MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_SECONDARY)
                );
                break;
            }
            case Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX: {
                bloomDesc.get<vkh::CombinedImageSampler>(0, BINDING_BLOOM_CANVAS_ORIGINAL)->setImageContextMF(
                    sic.getImageContextMF(
                        SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY));
                bloomDesc.get<vkh::CombinedImageSampler>(0, BINDING_BLOOM_CANVAS_BLURRED)->setImageContextMF(
                    sic.getImageContextMF(
                        SharedImageContextIndices::MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_SECONDARY)
                );
                break;
            }
            default: {
                //noop
            }
        }


        writeDescriptorMF([&bloomDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            bloomDesc.queue(queue, frameIndex, {}, {BINDING_BLOOM_CANVAS_ORIGINAL, BINDING_BLOOM_CANVAS_BLURRED});
        });
    }


    void GPCBloom::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //no operation
    }

    void GPCBloom::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;

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
                .unnormalizedCoordinates = VK_FALSE,
            });
        auto descManager = vkh::factory::create<vkh::DescriptorManager>();

        descManager->appendCombinedImgSampler(BINDING_BLOOM_CANVAS_ORIGINAL,
                                                        VK_SHADER_STAGE_FRAGMENT_BIT,
                                                        vkh::factory::create<vkh::CombinedImageSampler>(
                                                            wc.core, sampler, true));
        descManager->appendCombinedImgSampler(BINDING_BLOOM_CANVAS_BLURRED,
                                                        VK_SHADER_STAGE_FRAGMENT_BIT,
                                                        vkh::factory::create<vkh::CombinedImageSampler>(
                                                            wc.core, sampler, true));
        appendUniqueDescriptor(SET_BLOOM_CANVAS, descriptors, std::move(descManager));
        appendDescriptor<DescBloom>(SET_BLOOM, descriptors);
    }
}
