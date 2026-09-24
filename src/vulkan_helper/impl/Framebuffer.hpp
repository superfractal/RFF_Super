//
// Created by Merutilm on 2025-07-14.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>
#include <vector>

#include "RenderPass.hpp"
#include "../handle/CoreHandler.hpp"

namespace merutilm::vkh {
    class FramebufferImpl final : public CoreHandler {
        std::vector<VkFramebuffer> framebuffer = {};
        RenderPassRef renderPass;
        VkExtent2D extent;

    public:
        explicit FramebufferImpl(CoreRef core, RenderPassRef renderPass, VkExtent2D extent);

        ~FramebufferImpl() override;

        FramebufferImpl(const FramebufferImpl &) = delete;

        FramebufferImpl &operator=(const FramebufferImpl &) = delete;

        FramebufferImpl(FramebufferImpl &&) = delete;

        FramebufferImpl &operator=(FramebufferImpl &&) = delete;


        [[nodiscard]] VkFramebuffer getFramebufferHandle(uint32_t imageIndex) const {
            return framebuffer[imageIndex];
        }

        [[nodiscard]] const VkExtent2D &getExtent() const { return extent; }

    private:
        void init() override;

        void destroy() override;
    };
    using Framebuffer = std::unique_ptr<FramebufferImpl>;
    using FramebufferPtr = FramebufferImpl *;
    using FramebufferRef = FramebufferImpl &;
}
