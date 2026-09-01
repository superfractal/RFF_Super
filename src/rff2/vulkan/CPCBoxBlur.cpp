//
// Created by Merutilm on 2025-08-28.
//
// Modified by Opus 5 on 2026-08-19, 2026-08-24
// Modified by GPT-5 on 2026-08-23.
//

#include "CPCBoxBlur.hpp"

#include <algorithm>
#include <cmath>

#include "SharedImageContextIndices.hpp"
#include "../../vulkan_helper/executor/ScopedCommandBufferExecutor.hpp"
#include "../../vulkan_helper/util/BarrierUtils.hpp"
#include "../constants/VulkanWindowConstants.hpp"

namespace merutilm::rff2 {
    void CPCBoxBlur::updateQueue(vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
        //no operation
    }

    /**
     * Set Gaussian blur using 3x box blur.
     * @param srcImage the source image to blur. when the gaussian blur starts, its layout must be <b>VK_IMAGE_LAYOUT_GENERAL</b>.
     * it can be used in fragment shader without any layout transition.
     * @param dstImage the destination of blurred image. previous image is discarded.
     */
    void CPCBoxBlur::setGaussianBlur(const vkh::MultiframeImageContext &srcImage,
                                     const vkh::MultiframeImageContext &dstImage) {
        setExtent(srcImage[0].extent);
        // Ping-pong, so an odd number of passes is what leaves the result in the destination.
        for (uint32_t i = 0; i < BOX_BLUR_PASS_COUNT; ++i) {
            if (i % 2 == 0) {
                setImages(i, srcImage, dstImage);
            } else {
                setImages(i, dstImage, srcImage);
            }
        }
    }


    void CPCBoxBlur::cmdGaussianBlur(const uint32_t frameIndex, const uint32_t blurTargetDescIndex) {
        const VkCommandBuffer cbh = wc.getCommandBuffer().getCommandBufferHandle(frameIndex);
        auto &blurDesc = getDescriptor(SET_BLUR_IMAGE);

        auto ctxGetter = [&blurDesc, &frameIndex](const uint32_t descIndex, const uint32_t binding) {
            return blurDesc.get<vkh::StorageImage>(descIndex, binding).ctx[frameIndex];
        };

        vkh::BarrierUtils::cmdImageMemoryBarrier(cbh, ctxGetter(0, BINDING_BLUR_IMAGE_DST).image, 0,
                                                 VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                                 VK_IMAGE_LAYOUT_GENERAL, 0, 1,
                                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        for (uint32_t i = 0; i < BOX_BLUR_PASS_COUNT; ++i) {
            if (i > 0) {
                vkh::BarrierUtils::cmdSynchronizeImageWriteToRead(
                    cbh, ctxGetter(i - 1, BINDING_BLUR_IMAGE_DST).image, VK_IMAGE_LAYOUT_GENERAL, 0, 1,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            }
            cmdRender(cbh, frameIndex, {i, radiusDescIndex(blurTargetDescIndex, i)});
        }
    }


    void CPCBoxBlur::initSize() const {
        auto &desc = getDescriptor(SET_BLUR_IMAGE);
        const uint32_t maxFramesInFlight = wc.core.getPhysicalDevice().getMaxFramesInFlight();
        for (uint32_t i = 0; i < BOX_BLUR_PASS_COUNT; ++i) {
            desc.get<vkh::StorageImage>(i, BINDING_BLUR_IMAGE_SRC).ctx = std::vector<vkh::ImageContext>(
                maxFramesInFlight);
            desc.get<vkh::StorageImage>(i, BINDING_BLUR_IMAGE_DST).ctx = std::vector<vkh::ImageContext>(
                maxFramesInFlight);
        }
    }

    void CPCBoxBlur::setImages(const uint32_t descIndex, const vkh::MultiframeImageContext &srcImage,
                               const vkh::MultiframeImageContext &dstImage) const {
        auto &desc = getDescriptor(SET_BLUR_IMAGE);


        writeDescriptorMF(
            [&desc, &srcImage, &dstImage, &descIndex](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                desc.get<vkh::StorageImage>(descIndex, BINDING_BLUR_IMAGE_SRC).ctx[frameIndex] = srcImage[frameIndex];
                desc.get<vkh::StorageImage>(descIndex, BINDING_BLUR_IMAGE_DST).ctx[frameIndex] = dstImage[frameIndex];
                desc.queue(queue, frameIndex, {descIndex}, {BINDING_BLUR_IMAGE_SRC, BINDING_BLUR_IMAGE_DST});
            });
    }


    uint32_t CPCBoxBlur::radiusDescIndex(const uint32_t blurTargetDescIndex, const uint32_t pass) {
        // The last pass carries the finished picture back across the ping-pong, so it blurs nothing.
        if (pass + 1 == BOX_BLUR_PASS_COUNT) {
            return DESC_INDEX_BLUR_COPY;
        }
        return blurTargetDescIndex * 2 + pass % 2;
    }

    void CPCBoxBlur::setBlurInfo(const uint32_t blurTargetDescIndex, const float blurSize) const {
        auto &desc = getDescriptor(SET_BLUR_RADIUS);

        const float safeBlurSize = std::isfinite(blurSize) ? std::clamp(blurSize, 0.0f, 1.0f) : 0.0f;
        for (uint32_t axis = 0; axis < 2; ++axis) {
            const uint32_t descIndex = blurTargetDescIndex * 2 + axis;
            const auto &ubo = *desc.get<vkh::Uniform>(descIndex, BINDING_BLUR_RADIUS_UBO);
            ubo.getHostObject().set<float>(TARGET_BLUR_UBO_BLUR_SIZE, safeBlurSize);
            ubo.getHostObject().set<uint32_t>(TARGET_BLUR_UBO_AXIS, axis);
            ubo.update();

            writeDescriptorMF(
                [&desc, descIndex](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
                    desc.queue(queue, frameIndex, {descIndex}, {BINDING_BLUR_RADIUS_UBO});
                });
        }
    }

    void CPCBoxBlur::pipelineInitialized() {
        auto &desc = getDescriptor(SET_BLUR_RADIUS);
        const auto &copyUBO = *desc.get<vkh::Uniform>(DESC_INDEX_BLUR_COPY, BINDING_BLUR_RADIUS_UBO);
        copyUBO.getHostObject().set<float>(TARGET_BLUR_UBO_BLUR_SIZE, 0.0f);
        copyUBO.getHostObject().set<uint32_t>(TARGET_BLUR_UBO_AXIS, 0u);
        copyUBO.update();
        // Every radius set is bound by name from the renderers, so none may reach a dispatch unwritten.
        writeDescriptorMF([&desc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            desc.queue(queue, frameIndex, {}, {BINDING_BLUR_RADIUS_UBO});
        });
    }

    void CPCBoxBlur::renderContextRefreshed() {
        using namespace SharedImageContextIndices;
        auto &sic = wc.getSharedImageContext();
        switch (wc.getAttachmentIndex()) {
            case Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX: {
                setGaussianBlur(sic.getImageContextMF(MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                                sic.getImageContextMF(MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_SECONDARY));
                break;
            }
            case Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX: {
                setGaussianBlur(sic.getImageContextMF(MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_PRIMARY),
                                sic.getImageContextMF(MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_SECONDARY));
                break;
            }
            default: {
                //noop
            }
        }
    }


    void CPCBoxBlur::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //no operation
    }

    void CPCBoxBlur::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        auto imgDesc = std::vector<vkh::DescriptorManager>(BOX_BLUR_PASS_COUNT);
        for (uint32_t i = 0; i < BOX_BLUR_PASS_COUNT; ++i) {
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendStorageImage(BINDING_BLUR_IMAGE_SRC, VK_SHADER_STAGE_COMPUTE_BIT);
            descManager->appendStorageImage(BINDING_BLUR_IMAGE_DST, VK_SHADER_STAGE_COMPUTE_BIT);
            imgDesc[i] = std::move(descManager);
        }

        auto radDesc = std::vector<vkh::DescriptorManager>(DESC_COUNT_BLUR_TARGET);
        for (uint32_t i = 0; i < DESC_COUNT_BLUR_TARGET; ++i) {
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<float>(TARGET_BLUR_UBO_BLUR_SIZE);
            bufferManager->reserve<uint32_t>(TARGET_BLUR_UBO_AXIS);
            auto descUBO = vkh::factory::create<vkh::Uniform>(wc.core, std::move(bufferManager),
                                                              vkh::BufferLock::LOCK_UNLOCK, false);
            descManager->appendUBO(BINDING_BLUR_RADIUS_UBO, VK_SHADER_STAGE_COMPUTE_BIT, std::move(descUBO));
            radDesc[i] = std::move(descManager);
        }

        appendUniqueDescriptor(SET_BLUR_IMAGE, descriptors, std::move(imgDesc));
        appendUniqueDescriptor(SET_BLUR_RADIUS, descriptors, std::move(radDesc));
    }
}
