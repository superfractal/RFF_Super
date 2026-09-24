//
// Created by Merutilm on 2025-08-31.
// Modified by Opus 5 on 2026-08-15, 2026-08-19, 2026-08-31
// Modified by GPT-6 on 2026-09-10, 2026-09-13, 2026-09-16, 2026-09-19, 2026-09-20, 2026-09-23
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

    void GPCLinearInterpolation::setSurface(const ShdSlopeAttribute &slope) const {
        using namespace SharedDescriptorTemplate;
        const auto &ubo = *getDescriptor(SET_LINEAR_INTERPOLATION).get<vkh::Uniform>(0, DescLinearInterpolation::BINDING_UBO_LINEAR_INTERPOLATION);
        auto &host = ubo.getHostObject();
        host.set<float>(DescLinearInterpolation::TARGET_DARK_STYLE, float(slope.surfaceStyle));
        host.set<float>(DescLinearInterpolation::TARGET_DARK_STRENGTH, 1.0f);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_VHS, slope.vhsNoise);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_CHROMA, slope.chromaticShift);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_PIXEL_MIX, slope.pixelMix);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_PIXEL_SIZE, slope.pixelSize);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_GRAIN, slope.filmGrain);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_SCALE, slope.grungeScale);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_DAMAGE, slope.styleDamage);
        host.set<float>(DescLinearInterpolation::TARGET_DARK_MONOCHROME, slope.styleMonochrome);
        host.set<float>(DescLinearInterpolation::TARGET_UKIYO_COLORS, slope.ukiyoColors);
        host.set<float>(DescLinearInterpolation::TARGET_UKIYO_FLATNESS, slope.ukiyoFlatness);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_INK_R, slope.printInk.r);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_INK_G, slope.printInk.g);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_INK_B, slope.printInk.b);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_INDIGO_R, slope.printIndigo.r);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_INDIGO_G, slope.printIndigo.g);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_INDIGO_B, slope.printIndigo.b);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_ASAGI_R, slope.printAsagi.r);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_ASAGI_G, slope.printAsagi.g);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_ASAGI_B, slope.printAsagi.b);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_BLUE_R, slope.printBlue.r);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_BLUE_G, slope.printBlue.g);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_BLUE_B, slope.printBlue.b);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_FOAM_R, slope.printFoam.r);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_FOAM_G, slope.printFoam.g);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_FOAM_B, slope.printFoam.b);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_PAPER_R, slope.printPaper.r);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_PAPER_G, slope.printPaper.g);
        host.set<float>(DescLinearInterpolation::TARGET_PRINT_PAPER_B, slope.printPaper.b);
        host.set<float>(DescLinearInterpolation::TARGET_LAYER_QUANTIZE, slope.layerQuantize);
        host.set<float>(DescLinearInterpolation::TARGET_LAYER_VHS, slope.layerVhs);
        host.set<float>(DescLinearInterpolation::TARGET_LAYER_MONO, slope.layerMono);
        ubo.update();
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
                                            const float peakNits, const bool sceneLinear) const {
        using namespace SharedDescriptorTemplate;
        auto &interDesc = getDescriptor(SET_LINEAR_INTERPOLATION);
        const auto &interUBO = *interDesc.get<vkh::Uniform>(
            0, DescLinearInterpolation::BINDING_UBO_LINEAR_INTERPOLATION);
        auto &interUBOHost = interUBO.getHostObject();
        // Only the float chain carries light above white, so an HDR curve without it would encode nothing extra.
        const VidHdrTransfer effective = hdr.use ? transfer : VidHdrTransfer::SDR;
        interUBOHost.set<bool>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_HDR, hdr.use);
        interUBOHost.set<float>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_SCENE_LINEAR, sceneLinear ? 1.0f : 0.0f);
        interUBOHost.set<float>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_EXPOSURE, hdr.exposure);
        interUBOHost.set<float>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_HEADROOM,
                                std::max(hdr.headroom, 1e-3f));
        interUBOHost.set<uint32_t>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_TONE_MAP,
                                   static_cast<uint32_t>(hdr.method));
        interUBOHost.set<uint32_t>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_TRANSFER,
                                   static_cast<uint32_t>(effective));
        interUBOHost.set<float>(DescLinearInterpolation::TARGET_LINEAR_INTERPOLATION_PEAK_NITS,
                                hdr.isMfr() && effective == VidHdrTransfer::SDR ? std::clamp(hdr.mfrPeakNits, 100.0f, 10000.0f) : std::max(peakNits, 1.0f));
        interUBO.update();
    }

    void GPCLinearInterpolation::pipelineInitialized() {
        using namespace SharedDescriptorTemplate;
        writeDescriptorMF([this](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            getDescriptor(SET_LINEAR_INTERPOLATION).queue(queue, frameIndex, {}, {DescLinearInterpolation::BINDING_UBO_LINEAR_INTERPOLATION});
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
            case Constants::VulkanWindow::VIDEO_PREPARATION_WINDOW_ATTACHMENT_INDEX:
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
        ShaderLayerControl::configure(layerPush, pipelineLayoutManager);
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
        appendDescriptor<DescIteration>(2, descriptors);
    }
}
