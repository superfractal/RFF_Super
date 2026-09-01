//
// Created by Merutilm on 2025-09-08.
// Modified by Opus 5 on 2026-08-10
// Modified by GPT-5 on 2026-08-23.
//

#pragma once
#include "../configurator/PipelineConfigurator.hpp"
#include "../core/vkh.hpp"
#include "../executor/ScopedCommandBufferExecutor.hpp"
#include "../util/SwapchainUtils.hpp"

namespace merutilm::vkh {
    struct RendererAbstract : public Handler {
        EngineRef engine;
        WindowContextRef wc;

        uint32_t frameIndex = 0;
        // Set while executeOffscreen() records, so a concrete renderer can skip its present pass.
        bool offscreenPass = false;
        bool pipelineInitialized = false;
        std::vector<PipelineConfigurator> configurators = {};

        explicit RendererAbstract(EngineRef engine, const uint32_t windowContextIndex) : engine(engine),
            wc(engine.getWindowContext(windowContextIndex)) {
        }

        ~RendererAbstract() override = default;

        RendererAbstract(const RendererAbstract &windowContext) = delete;

        RendererAbstract(RendererAbstract &&) = delete;

        RendererAbstract &operator=(RendererAbstract &&) = delete;

        RendererAbstract &operator=(const RendererAbstract &) = delete;


        [[nodiscard]] uint32_t getFrameIndex() const {
            return frameIndex;
        }

        void finishPipelineInitialization() const {
            for (const auto &sp: configurators) {
                sp->pipelineInitialized();
            }
        };

        bool execute() {
            return SwapchainUtils::renderFrame(wc, &frameIndex, [this](const uint32_t swapchainImageIndex) {
                recordAndSubmit(swapchainImageIndex,
                                wc.getSyncObject().getSemaphore(frameIndex).getImageAvailable(),
                                wc.getSyncObject().getSemaphore(frameIndex).getRenderFinished());
            });
        }

        // Renders one frame without touching the swapchain: no image acquire, no present, and none of
        // renderFrame's isUnrenderable() early-out, which would otherwise leave the caller reading back
        // the previous frame without any way to tell that nothing was rendered.
        void executeOffscreen() {
            SwapchainUtils::changeFrameIndex(wc.core, &frameIndex);
            wc.getSyncObject().getFence(frameIndex).waitAndReset();
            offscreenPass = true;
            recordAndSubmit(UINT32_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE);
            offscreenPass = false;
        }

    private:
        void recordAndSubmit(const uint32_t swapchainImageIndex, const VkSemaphore imageAvailableSemaphore,
                             const VkSemaphore renderFinishedSemaphore) {
            if (frameIndex == 0) {
                for (auto &rc: wc.getRenderContexts()) {
                    rc->getConfigurator()->allFrameInitialized();
                }
            }
            DescriptorUpdateQueue queue = DescriptorUpdater::createQueue();
            const VkDevice device = wc.core.getLogicalDevice().getLogicalDeviceHandle();

            for (const auto &configurator: configurators) {
                configurator->updateQueue(queue, frameIndex);
            }

            DescriptorUpdater::write(device, queue);

            const VkFence fence = wc.getSyncObject().getFence(frameIndex).getFenceHandle();
            beforeCmdRender();
            ScopedCommandBufferExecutor executor(wc, frameIndex, fence, imageAvailableSemaphore,
                                                 renderFinishedSemaphore);
            cmdRender(swapchainImageIndex);
        }

        virtual void beforeCmdRender() = 0;

        virtual void cmdRender(uint32_t swapchainImageIndex) = 0;
    };

    using Renderer = std::unique_ptr<RendererAbstract>;
    using RendererPtr = RendererAbstract *;
    using RendererRef = RendererAbstract &;
}
