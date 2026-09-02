//
// Created by Merutilm on 2025-08-15.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-16, 2026-08-21.
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-08, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-20, 2026-08-22, 2026-08-29
// Modified by ox-alpha on 2026-08-22.
// Modified by Fable 5.1 on 2026-09-02
//

#include "GPCSlope.hpp"

#include "RCC1.hpp"
#include "SharedDescriptorTemplate.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../constants/VulkanWindowConstants.hpp"

namespace merutilm::rff2 {
    void GPCSlope::updateQueue(vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {

        //no operation
    }


    void GPCSlope::setSlope(const ShdSlopeAttribute &slope) const {
        using namespace SharedDescriptorTemplate;
        auto &slopeDesc = getDescriptor(SET_SLOPE);
        const auto &slopeUBO = slopeDesc.get<vkh::Uniform>(0, DescSlope::BINDING_UBO_SLOPE);
        auto &slopeUBOHost = slopeUBO->getHostObject();
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_DEPTH, slope.depth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_REFLECTION_RATIO, slope.reflectionRatio);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_OPACITY, slope.opacity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_ZENITH, slope.zenith);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_AZIMUTH, slope.azimuth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_INTENSITY, slope.specularIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_POWER, slope.specularPower);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_INTENSITY, slope.rimIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_POWER, slope.rimPower);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_BRIGHTNESS, slope.brightness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GAMMA, slope.gamma);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_COLOR_R, slope.rimColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_COLOR_G, slope.rimColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_COLOR_B, slope.rimColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_COLOR_R, slope.specularColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_COLOR_G, slope.specularColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_COLOR_B, slope.specularColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_AO_INTENSITY, slope.aoIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_AMBIENT_INTENSITY, slope.ambientIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SKY_COLOR_R, slope.skyColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SKY_COLOR_G, slope.skyColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SKY_COLOR_B, slope.skyColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GROUND_COLOR_R, slope.groundColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GROUND_COLOR_G, slope.groundColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GROUND_COLOR_B, slope.groundColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_LINK, slope.specularIndependent ? 0.0f : 1.0f);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_ZENITH, slope.specularZenith);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_AZIMUTH, slope.specularAzimuth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_ANISOTROPY, slope.specularAnisotropy);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_ANISOTROPY_ANGLE, slope.specularAnisotropyAngle);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_MACRO_RELIEF, slope.macroRelief);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_MACRO_RADIUS, slope.macroRadius);
        // Carried as a float like specular_link; the shader rounds it back.
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SHADING_BLEND,
                                static_cast<float>(static_cast<int32_t>(slope.shadingBlend)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RELIEF_RESPONSE, slope.reliefResponse);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_TERMINATOR_SOFTNESS, slope.terminatorSoftness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_HIGHLIGHT_KNEE, slope.highlightKnee);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_LIGHT_BLEND,
                                static_cast<float>(static_cast<int32_t>(slope.lightBlend)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_LUMA_AMOUNT, slope.lumaAmount);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_TINT_RESPONSE, slope.tintResponse);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SHADOW_CHROMA, slope.shadowChroma);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_TINT_BLEND,
                                static_cast<float>(static_cast<int32_t>(slope.tintBlend)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_FILL_INTENSITY, slope.fillIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_FILL_ZENITH, slope.fillZenith);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_FILL_AZIMUTH, slope.fillAzimuth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_INTENSITY, slope.glossIntensity);
        // Carried as a float like specular_link; the shader rounds it back.
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_SOURCE,
                                static_cast<float>(static_cast<int32_t>(slope.glossSource)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_BANDS, slope.glossBands);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_SHARPNESS, slope.glossSharpness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_PHASE, slope.glossPhase);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_COLOR_R, slope.glossColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_COLOR_G, slope.glossColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_COLOR_B, slope.glossColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_RELIEF, slope.glossRelief);
        slopeUBO->update();
    }

    void GPCSlope::pipelineInitialized() {
        using namespace SharedDescriptorTemplate;
        writeDescriptorMF([this](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            getDescriptor(SET_SLOPE).queue(queue, frameIndex, {}, {DescSlope::BINDING_UBO_SLOPE});
        });
    }

    void GPCSlope::renderContextRefreshed() {
        auto &sic = wc.getSharedImageContext();
        auto &inputDesc = getDescriptor(SET_PREV_RESULT);

        switch (wc.getAttachmentIndex()) {
            case Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX: {
                const auto &input = sic.getImageContextMF(SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY);
                inputDesc.get<vkh::CombinedImageSampler>(0, BINDING_PREV_RESULT_SAMPLER)->setImageContextMF(input);
                break;
            }
            case Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX: {
                const auto &input = sic.getImageContextMF(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY);
                inputDesc.get<vkh::CombinedImageSampler>(0, BINDING_PREV_RESULT_SAMPLER)->setImageContextMF(input);
                break;
            }
            default: {
                //noop
            }
        }

        writeDescriptorMF([&inputDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            inputDesc.queue(queue, frameIndex, {}, {BINDING_PREV_RESULT_SAMPLER});
        });

    }

    void GPCSlope::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //noop
    }

    void GPCSlope::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;
        auto descManager = vkh::factory::create<vkh::DescriptorManager>();
        vkh::SamplerRef sampler = pickFromGlobalRepository<vkh::GlobalSamplerRepo, vkh::SamplerRef>(
            VkSamplerCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .mipLodBias = 0,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 0,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0,
                .maxLod = 0,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                .unnormalizedCoordinates = VK_TRUE
            });
        descManager->appendCombinedImgSampler(BINDING_PREV_RESULT_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, vkh::factory::create<vkh::CombinedImageSampler>(wc.core, sampler, true));

        appendUniqueDescriptor(SET_PREV_RESULT, descriptors, std::move(descManager));
        appendDescriptor<DescIteration>(SET_ITERATION, descriptors);
        appendDescriptor<DescSlope>(SET_SLOPE, descriptors);
    }
}
