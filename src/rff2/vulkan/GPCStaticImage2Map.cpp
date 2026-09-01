//
// Created by Merutilm on 2025-09-09.
// Modified by Opus 5 on 2026-08-24, 2026-08-26
//

#include "GPCStaticImage2Map.hpp"

#include "SharedDescriptorTemplate.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../../vulkan_helper/util/BufferImageContextUtils.hpp"
#include "../../vulkan_helper/core/logger.hpp"
#include "../io/RFFStaticMapBinary.h"
#include "opencv2/imgproc.hpp"

namespace merutilm::rff2 {
    namespace {
        // The keyframe images are read from disk exactly as they were written, so what arrives here
        // is whatever the folder holds: 8-bit or 16-bit, one, three or four channels, any size, and
        // a file that failed to decode arrives empty. The upload below reads width x height x 8
        // bytes out of it as 16-bit BGRA, so anything narrower than that has to be widened first or
        // the copy runs off the end of the picture. Channel order is left as OpenCV lays it out,
        // which is the order the shader's own swizzle expects.
        bool toColorBGRA16(const cv::Mat &src, cv::Mat &out) {
            if (src.empty() || src.cols <= 0 || src.rows <= 0 || src.data == nullptr) {
                return false;
            }
            cv::Mat wide = src;
            if (wide.depth() != CV_16U) {
                // 8-bit levels are stretched over the full 16-bit range rather than left in its
                // bottom 1/257th, which would come out as a nearly black frame.
                wide.convertTo(wide, CV_16U, wide.depth() == CV_8U ? 257.0 : 1.0);
            }
            switch (wide.channels()) {
                case 1:
                    cv::cvtColor(wide, out, cv::COLOR_GRAY2BGRA);
                    break;
                case 3:
                    cv::cvtColor(wide, out, cv::COLOR_BGR2BGRA);
                    break;
                case 4:
                    out = wide;
                    break;
                default:
                    return false;
            }
            // A copy taken by the row is only whole when the rows sit end to end.
            if (!out.isContinuous()) {
                out = out.clone();
            }
            return true;
        }

        // Stands in for a keyframe image that cannot be read, so the sampler still has an image
        // behind it and the frame comes out black rather than taking the renderer down.
        vkh::ImageContext uploadColorBGRA16(const vkh::CoreRef core, const vkh::CommandPoolRef commandPool,
                                            const cv::Mat &image) {
            cv::Mat color;
            if (!toColorBGRA16(image, color)) {
                vkh::logger::w_log(L"ERROR : Cannot read the keyframe image");
                constexpr std::array<uint16_t, 4> black = {0, 0, 0, 0xFFFF};
                return vkh::BufferImageContextUtils::imageFromByteColorArray(
                    core, commandPool, VK_FORMAT_R16G16B16A16_UNORM, 1, 1, 4, 16, false,
                    reinterpret_cast<const std::byte *>(black.data()));
            }
            return vkh::BufferImageContextUtils::imageFromByteColorArray(
                core, commandPool, VK_FORMAT_R16G16B16A16_UNORM, static_cast<uint32_t>(color.cols),
                static_cast<uint32_t>(color.rows), 4, 16, false,
                reinterpret_cast<const std::byte *>(color.data));
        }
    }

    void GPCStaticImage2Map::updateQueue(vkh::DescriptorUpdateQueue &queue, uint32_t frameIndex) {
        //noop
    }

    void GPCStaticImage2Map::pipelineInitialized() {
        //noop
    }

    void GPCStaticImage2Map::renderContextRefreshed() {
        //noop
    }

    void GPCStaticImage2Map::setImages(const cv::Mat &normal, const cv::Mat &zoomed) const {
        // Each image is measured by its own size: the two keyframes are separate files and need not
        // agree, and reading the second one through the first one's dimensions runs past its end.
        const auto n = uploadColorBGRA16(wc.core, wc.getCommandPool(), normal);
        const auto z = uploadColorBGRA16(wc.core, wc.getCommandPool(), zoomed);
        auto &imageDesc = getDescriptor(SET_IMAGES);
        imageDesc.get<vkh::CombinedImageSampler>(0, BINDING_IMAGES_NORMAL)->setUniqueImageContext(n);
        imageDesc.get<vkh::CombinedImageSampler>(0, BINDING_IMAGES_ZOOMED)->setUniqueImageContext(z);
        writeDescriptorMF([&imageDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            imageDesc.queue(queue, frameIndex, {}, {BINDING_IMAGES_NORMAL, BINDING_IMAGES_ZOOMED});
        });
    }

    void GPCStaticImage2Map::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        //noop
    }

    void GPCStaticImage2Map::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
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

        descManager->appendCombinedImgSampler(BINDING_IMAGES_NORMAL,
                                              VK_SHADER_STAGE_FRAGMENT_BIT,
                                              vkh::factory::create<vkh::CombinedImageSampler>(
                                                  wc.core, sampler, false));
        descManager->appendCombinedImgSampler(BINDING_IMAGES_ZOOMED,
                                              VK_SHADER_STAGE_FRAGMENT_BIT,
                                              vkh::factory::create<vkh::CombinedImageSampler>(
                                                  wc.core, sampler, false));
        appendUniqueDescriptor(SET_IMAGES, descriptors, std::move(descManager));
        appendDescriptor<SharedDescriptorTemplate::DescVideo>(SET_VIDEO, descriptors);
        appendDescriptor<SharedDescriptorTemplate::DescBloom>(SET_BLOOM, descriptors);
    }
}
