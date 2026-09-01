//
// Created by Merutilm on 2025-07-14.
// Modified by GPT-5 on 2026-08-23.
//

#include "Framebuffer.hpp"

namespace merutilm::vkh {
    FramebufferImpl::FramebufferImpl(CoreRef core, RenderPassRef renderPass,
                             const VkExtent2D extent) : CoreHandler(core), renderPass(renderPass), extent(extent) {
        FramebufferImpl::init();
    }

    FramebufferImpl::~FramebufferImpl() {
        FramebufferImpl::destroy();
    }

    void FramebufferImpl::init() {
        const auto [width, height] = extent;
        auto & attachments = renderPass.getAttachments();
        const size_t imageCount = attachments.empty() ? 0 : attachments.front().imageContext.size();
        if (imageCount == 0 || std::ranges::any_of(attachments, [imageCount](const RenderPassAttachment &attachment) {
                return attachment.imageContext.size() != imageCount;
            })) {
            throw exception_init("Framebuffer attachments have inconsistent image counts");
        }
        framebuffer.resize(imageCount);

        for (uint32_t i = 0; i < imageCount; ++i) {
            auto attachmentWriteImageViews = std::vector<VkImageView>(attachments.size());
            std::ranges::transform(attachments, attachmentWriteImageViews.begin(), [i](const RenderPassAttachment &v) {
                return v.imageContext[i].imageView;
            });



            const VkFramebufferCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderPass = renderPass.getRenderPassHandle(),
                .attachmentCount = static_cast<uint32_t>(attachmentWriteImageViews.size()),
                .pAttachments = attachmentWriteImageViews.data(),
                .width = width,
                .height = height,
                .layers = 1
            };


            if (allocator::invoke(vkCreateFramebuffer, core.getLogicalDevice().getLogicalDeviceHandle(), &createInfo, nullptr,
                                    &framebuffer[i]) != VK_SUCCESS) {
                throw exception_init("Failed to create framebuffer");
            }
        }
    }

    void FramebufferImpl::destroy() {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        for (const VkFramebuffer handle: framebuffer) {
            allocator::invoke(vkDestroyFramebuffer, device, handle, nullptr);
        }
    }


}
