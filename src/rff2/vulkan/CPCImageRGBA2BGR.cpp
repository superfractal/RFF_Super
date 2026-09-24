//
// Created by Merutilm on 2025-09-10.
// Modified by Opus 5 on 2026-08-10, 2026-08-19
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-09-23
//

#include "CPCImageRGBA2BGR.hpp"

#include <limits>
#include <stdexcept>

#include "SharedImageContextIndices.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"


namespace merutilm::rff2 {
    void CPCImageRGBA2BGR::updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) {
        //noop
    }

    void CPCImageRGBA2BGR::pipelineInitialized() {
        //noop
    }

    void CPCImageRGBA2BGR::renderContextRefreshed() {
        using namespace SharedImageContextIndices;
        auto &prevImg = *getDescriptor(SET_INFO).get<vkh::CombinedImageSampler>(0, BINDING_PREV_IMAGE_SAMPLER);
        prevImg.setImageContextMF(wc.getSharedImageContext().getImageContextMF(MF_VIDEO_RENDER_IMAGE_SECONDARY));
        applyOutputSize();
    }

    void CPCImageRGBA2BGR::setDownsample(const uint32_t factor) {
        const uint32_t clamped = std::max<uint32_t>(1, factor);
        if (clamped == downsample) {
            return;
        }
        downsample = clamped;
        applyOutputSize();
    }

    void CPCImageRGBA2BGR::setHdr(const bool use) {
        if (use == hdr) {
            return;
        }
        hdr = use;
        applyOutputSize();
    }

    void CPCImageRGBA2BGR::applyOutputSize() {
        auto &desc = getDescriptor(SET_INFO);
        const auto &prevImg = *desc.get<vkh::CombinedImageSampler>(0, BINDING_PREV_IMAGE_SAMPLER);
        const auto &srcExtent = prevImg.getImageContextMF()[0].extent;
        const VkExtent2D nextExtent{
            std::max<uint32_t>(1, srcExtent.width / downsample),
            std::max<uint32_t>(1, srcExtent.height / downsample)
        };
        const uint64_t pixels = uint64_t{nextExtent.width} * nextExtent.height;
        const uint64_t maxShaderInt = std::numeric_limits<int32_t>::max();
        const uint64_t maxHostBytes = std::numeric_limits<uint32_t>::max();
        if (pixels > maxShaderInt ||
            (!hdr && pixels * 3 + 3 > maxShaderInt)) {
            throw std::overflow_error("Video output exceeds compute shader index limits");
        }
        // Keep the SDR extra word used by the existing buffer layout.
        const uint64_t words = hdr ? pixels * 2 : pixels * 3 / 4 + 1;
        if (words > maxHostBytes / sizeof(uint32_t)) {
            throw std::overflow_error("Video output exceeds the host buffer size limit");
        }
        outputExtent = nextExtent;

        auto &ssbo = *desc.get<vkh::ShaderStorage>(0, BINDING_OUTPUT_SSBO);
        // Two words per pixel for rgba64le; the BGR24 path packs three bytes into every word instead.
        ssbo.getHostObject().resizeAndClear<uint32_t>(TARGET_OUTPUT_SSBO_DATA,
                                                      static_cast<uint32_t>(words));
        ssbo.reloadBuffer();
        // lock() copies the staging mapping, so the cleared host words must be in it first.
        ssbo.upload();
        ssbo.lock(wc.getCommandPool());

        const auto &ubo = *desc.get<vkh::Uniform>(0, BINDING_OUTPUT_EXTENT_UBO);
        ubo.getHostObject().set<glm::uvec2>(TARGET_OUTPUT_EXTENT_UBO_EXTENT,
                                            glm::uvec2(outputExtent.width, outputExtent.height));
        ubo.getHostObject().set<uint32_t>(TARGET_OUTPUT_EXTENT_UBO_HDR, hdr ? 1u : 0u);
        ubo.update();

        // One invocation per destination pixel; the shader averages the source region it covers.
        setExtent(outputExtent);
        writeDescriptorMF(
            [&desc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                desc.queue(queue, frameIndex, {},
                           {BINDING_PREV_IMAGE_SAMPLER, BINDING_OUTPUT_SSBO, BINDING_OUTPUT_EXTENT_UBO});
            });
    }

    const vkh::BufferContext &CPCImageRGBA2BGR::getBufferContext(const uint32_t frameIndex) const {
        using namespace SharedImageContextIndices;
        return getDescriptor(SET_INFO).get<vkh::ShaderStorage>(0, BINDING_OUTPUT_SSBO)->getBufferContextMF()[
            frameIndex];
    }

    void CPCImageRGBA2BGR::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //noop
    }

    void CPCImageRGBA2BGR::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
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
        descManager->appendCombinedImgSampler(BINDING_PREV_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, vkh::factory::create<vkh::CombinedImageSampler>(wc.core, sampler, true));

        auto hdm = vkh::factory::create<vkh::HostDataObjectManager>();
        hdm->reserveArray<uint32_t>(TARGET_OUTPUT_SSBO_DATA, 1);

        descManager->appendSSBO(BINDING_OUTPUT_SSBO, VK_SHADER_STAGE_COMPUTE_BIT,
                                vkh::factory::create<vkh::ShaderStorage>(
                                    wc.core, std::move(hdm), vkh::BufferLock::LOCK_UNLOCK, true));

        auto uboManager = vkh::factory::create<vkh::HostDataObjectManager>();
        uboManager->reserve<glm::uvec2>(TARGET_OUTPUT_EXTENT_UBO_EXTENT);
        uboManager->reserve<uint32_t>(TARGET_OUTPUT_EXTENT_UBO_HDR);
        descManager->appendUBO(BINDING_OUTPUT_EXTENT_UBO, VK_SHADER_STAGE_COMPUTE_BIT,
                               vkh::factory::create<vkh::Uniform>(wc.core, std::move(uboManager),
                                                                  vkh::BufferLock::LOCK_UNLOCK, false));

        appendUniqueDescriptor(SET_INFO, descriptors, std::move(descManager));
    }
}
