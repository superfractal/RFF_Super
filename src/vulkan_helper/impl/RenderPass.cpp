//
// Created by Merutilm on 2025-07-11.
// Modified by GPT-6 on 2026-09-23
//

#include "RenderPass.hpp"

#include <algorithm>
#include <format>
#include <utility>

#include "../core/exception.hpp"
#include "../manage/RenderPassManager.hpp"

namespace merutilm::vkh {
    RenderPassImpl::RenderPassImpl(CoreRef core, RenderPassManager &&manager)
        : CoreHandler(core),
          attachments(std::move(manager->attachments)),
          preserveIndices(std::move(manager->preserveIndices)),
          attachmentReferences(std::move(manager->attachmentReferences)),
          subpassDependencies(std::move(manager->subpassDependencies)),
          subpassCount(manager->subpassCount) {
        RenderPassImpl::init();
    }

    RenderPassImpl::~RenderPassImpl() {
        RenderPassImpl::destroy();
    }

    void RenderPassImpl::init() {
        if (renderPass != VK_NULL_HANDLE) {
            throw exception_invalid_state("Render pass is already initialized");
        }
        const uint32_t subpasses = getSubpassCount();
        std::vector<VkSubpassDescription> subpassDescriptions(subpasses);

        const auto &dependencies = getSubpassDependencies();

        using enum RenderPassAttachmentType;
        for (uint32_t subpassIndex = 0; subpassIndex < subpasses; ++subpassIndex) {
            const auto &inputRef = getAttachmentReferences(subpassIndex, INPUT);
            const auto &colorRef = getAttachmentReferences(subpassIndex, COLOR);
            const auto &resolveRef = getAttachmentReferences(subpassIndex, RESOLVE);
            const auto &depthStencilRef = getAttachmentReferences(subpassIndex, DEPTH_STENCIL);
            if (!resolveRef.empty() && colorRef.size() != resolveRef.size()) {
                throw exception_init(
                    std::format("SUBPASS {}: the size of color attachment and resolve attachment doesn't match ",
                                subpassIndex));
            }
            subpassDescriptions[subpassIndex] = {
                .flags = 0,
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = static_cast<uint32_t>(inputRef.size()),
                .pInputAttachments = inputRef.data(),
                .colorAttachmentCount = static_cast<uint32_t>(colorRef.size()),
                .pColorAttachments = colorRef.data(),
                .pResolveAttachments = resolveRef.data(),
                .pDepthStencilAttachment = depthStencilRef.data(),
                .preserveAttachmentCount = getPreserveIndicesCount(subpassIndex),
                .pPreserveAttachments = getPreserveIndices(subpassIndex),
            };
        }
        const auto &renderAttachments = getAttachments();
        std::vector<VkAttachmentDescription> attachmentDescriptions(renderAttachments.size());
        std::ranges::transform(renderAttachments, attachmentDescriptions.begin(),
                               [](const RenderPassAttachment &attachment) { return attachment.attachment; });
        const VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
            .pAttachments = attachmentDescriptions.data(),
            .subpassCount = static_cast<uint32_t>(subpassDescriptions.size()),
            .pSubpasses = subpassDescriptions.data(),
            .dependencyCount = static_cast<uint32_t>(dependencies.size()),
            .pDependencies = dependencies.data(),
        };
        VkRenderPass createdRenderPass = VK_NULL_HANDLE;
        if (allocator::invoke(vkCreateRenderPass, core.getLogicalDevice().getLogicalDeviceHandle(),
                              &renderPassInfo, nullptr, &createdRenderPass) != VK_SUCCESS) {
            throw exception_init("Failed to create render pass!");
        }
        renderPass = createdRenderPass;
    }

    void RenderPassImpl::destroy() {
        if (renderPass == VK_NULL_HANDLE) {
            return;
        }
        allocator::invoke(vkDestroyRenderPass, core.getLogicalDevice().getLogicalDeviceHandle(), renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
}
