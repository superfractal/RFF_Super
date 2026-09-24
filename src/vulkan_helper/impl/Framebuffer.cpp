//
// Created by Merutilm on 2025-07-14.
// Modified by GPT-5 on 2026-08-23
// Modified by GPT-6 on 2026-09-23
//

#include "Framebuffer.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>
#include "../core/exception.hpp"

namespace merutilm::vkh {
    FramebufferImpl::FramebufferImpl(CoreRef core, RenderPassRef renderPass,
                                     const VkExtent2D extent)
        : CoreHandler(core), renderPass(renderPass), extent(extent) {
        FramebufferImpl::init();
    }

    FramebufferImpl::~FramebufferImpl() {
        FramebufferImpl::destroy();
    }

    void FramebufferImpl::init() {
        if (!framebuffer.empty()) {
            throw exception_invalid_state("Framebuffer is already initialized");
        }
        const auto [width, height] = extent;
        const auto &attachments = renderPass.getAttachments();
        const size_t imageCount = attachments.empty() ? 0 : attachments.front().imageContext.size();
        if (imageCount == 0 || std::ranges::any_of(attachments, [imageCount](const RenderPassAttachment &attachment) {
                return attachment.imageContext.size() != imageCount;
            })) {
            throw exception_init("Framebuffer attachments have inconsistent image counts");
        }
        framebuffer.resize(imageCount);

        try {
            for (size_t imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
                std::vector<VkImageView> imageViews(attachments.size());
                std::ranges::transform(attachments, imageViews.begin(),
                                       [imageIndex](const RenderPassAttachment &attachment) {
                                           return attachment.imageContext[imageIndex].imageView;
                                       });

                const VkFramebufferCreateInfo createInfo = {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .renderPass = renderPass.getRenderPassHandle(),
                    .attachmentCount = static_cast<uint32_t>(imageViews.size()),
                    .pAttachments = imageViews.data(),
                    .width = width,
                    .height = height,
                    .layers = 1
                };

                VkFramebuffer createdFramebuffer = VK_NULL_HANDLE;
                if (allocator::invoke(vkCreateFramebuffer, core.getLogicalDevice().getLogicalDeviceHandle(),
                                      &createInfo, nullptr, &createdFramebuffer) != VK_SUCCESS) {
                    throw exception_init("Failed to create framebuffer");
                }
                framebuffer[imageIndex] = createdFramebuffer;
            }
        } catch (...) {
            FramebufferImpl::destroy();
            throw;
        }
    }

    void FramebufferImpl::destroy() {
        if (framebuffer.empty()) {
            return;
        }
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        for (const VkFramebuffer handle : framebuffer) {
            if (handle != VK_NULL_HANDLE) {
                allocator::invoke(vkDestroyFramebuffer, device, handle, nullptr);
            }
        }
        framebuffer.clear();
    }
}
