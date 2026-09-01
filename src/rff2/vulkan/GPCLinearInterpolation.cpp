//
// Created by Merutilm on 2025-08-31.
// Modified by Opus 5 on 2026-08-15, 2026-08-19, 2026-08-31
//

#include "GPCLinearInterpolation.hpp"

#include "RCC4.hpp"
#include "SharedDescriptorTemplate.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../constants/VulkanWindowConstants.hpp"

namespace merutilm::rff2 {
    void GPCLinearInterpolation::updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) {
        //noop
    }

    void GPCLinearInterpolation::setLinearInterpolation(const bool use) const {
        using namespace SharedDescriptorTemplate;
        auto &interDesc = getDescriptor(SET_LINEAR_INTERPOLATION);
        const auto &interUBO = *interDesc.get<vkh::Uniform>(
            0, DescLinearInterpolation::BINDING_UBO_LINEAR_INTERPOLATION);
        auto &interUBOHost = interUBO.getHostObject();
        interUBOHost.set<bool>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_USE, use);
        interUBO.update();
    }

    void GPCLinearInterpolation::setDither(const bool use) const {
        using namespace SharedDescriptorTemplate;
        auto &interDesc = getDescriptor(SET_LINEAR_INTERPOLATION);
        const auto &interUBO = *interDesc.get<vkh::Uniform>(
            0, DescLinearInterpolation::BINDING_UBO_LINEAR_INTERPOLATION);
        auto &interUBOHost = interUBO.getHostObject();
        interUBOHost.set<bool>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_DITHER, use);
        interUBO.update();
    }

    void GPCLinearInterpolation::setToneMap(const ShdHdrAttribute &hdr, const VidHdrTransfer transfer,
                                            const float peakNits) const {
        using namespace SharedDescriptorTemplate;
        auto &interDesc = getDescriptor(SET_LINEAR_INTERPOLATION);
        const auto &interUBO = *interDesc.get<vkh::Uniform>(
            0, DescLinearInterpolation::BINDING_UBO_LINEAR_INTERPOLATION);
        auto &interUBOHost = interUBO.getHostObject();
        // Only the float chain carries light above white, so an HDR curve without it would encode nothing extra.
        const VidHdrTransfer effective = hdr.use ? transfer : VidHdrTransfer::SDR;
        interUBOHost.set<bool>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_HDR, hdr.use);
        interUBOHost.set<float>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_EXPOSURE, hdr.exposure);
        interUBOHost.set<float>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_HEADROOM,
                                std::max(hdr.headroom, 1e-3f));
        interUBOHost.set<uint32_t>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_TONE_MAP,
                                   static_cast<uint32_t>(hdr.method));
        interUBOHost.set<uint32_t>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_TRANSFER,
                                   static_cast<uint32_t>(effective));
        interUBOHost.set<float>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_PEAK_NITS,
                                std::max(peakNits, 1.0f));
        interUBO.update();
    }

    void GPCLinearInterpolation::pipelineInitialized() {
        using namespace SharedDescriptorTemplate;
        writeDescriptorMF([this](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            getDescriptor(SET_LINEAR_INTERPOLATION).queue(queue, frameIndex, {}, {DescBloom::BINDING_UBO_BLOOM});
        });
    }

    void GPCLinearInterpolation::renderContextRefreshed() {
        auto &sic = wc.getSharedImageContext();
        auto &samplerDesc = getDescriptor(SET_PREV_RESULT);
        switch (wc.getAttachmentIndex()) {
            case Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX: {
                const auto &sample = sic.getImageContextMF(SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY);
                samplerDesc.get<vkh::CombinedImageSampler>(0, BINDING_PREV_RESULT_SAMPLER)->
                        setImageContextMF(sample);
                break;
            }
            case Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX: {
                const auto &sample = sic.getImageContextMF(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY);
                samplerDesc.get<vkh::CombinedImageSampler>(0, BINDING_PREV_RESULT_SAMPLER)->
                        setImageContextMF(sample);
                break;
            }
            default: {
                //noop
            }
        }


        writeDescriptorMF([&samplerDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            samplerDesc.queue(queue, frameIndex, {}, {BINDING_PREV_RESULT_SAMPLER});
        });
    }

    void GPCLinearInterpolation::configurePushConstant(
        vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //noop
    }

    void GPCLinearInterpolation::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;
        vkh::SamplerRef sampler = pickFromGlobalRepository<vkh::GlobalSamplerRepo, vkh::SamplerRef>(
            VkSamplerCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
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

        descManager->appendCombinedImgSampler(BINDING_PREV_RESULT_SAMPLER,
                                                        VK_SHADER_STAGE_FRAGMENT_BIT,
                                                        vkh::factory::create<vkh::CombinedImageSampler>(
                                                            wc.core, sampler, true));
        appendUniqueDescriptor(SET_PREV_RESULT, descriptors, std::move(descManager));
        appendDescriptor<DescLinearInterpolation>(SET_LINEAR_INTERPOLATION, descriptors);
    }
}
