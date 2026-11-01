//
// Created by Merutilm on 2025-07-11.
// Modified by Fable 5.1 on 2026-09-06
// Modified by GPT-6 on 2026-09-20, 2026-09-22, 2026-09-23
// Modified by Opus 5.5 on 2026-10-13
//

#pragma once

#include "../manage/PipelineManager.hpp"
#include "PipelinePreparation.hpp"
#include "../handle/WindowContextHandler.hpp"

namespace merutilm::vkh {
    struct PipelineAbstract : public WindowContextHandler {
        std::shared_ptr<PipelinePreparation::Job> preparation;
        VkPipeline pipeline = VK_NULL_HANDLE;
        PipelineLayoutRef pipelineLayout;
        const std::vector<DescriptorPtr> descriptors;
        const std::vector<ShaderModulePtr> shaderModules;
        // Specialization constants the pipeline was built with; word i is constant_id i.
        std::vector<uint32_t> specialization;
        std::vector<VkSpecializationMapEntry> specializationMap;
        VkSpecializationInfo specializationInfo = {};

        explicit PipelineAbstract(WindowContextRef wc, PipelineLayoutRef pipelineLayout,
                                  PipelineManager &&pipelineManager)
            : WindowContextHandler(wc),
              pipelineLayout(pipelineLayout),
              descriptors(std::move(pipelineManager->descriptors)),
              shaderModules(std::move(pipelineManager->shaderModules)),
              specialization(std::move(pipelineManager->specialization)) {
        }

        ~PipelineAbstract() override = default;

        PipelineAbstract(const PipelineAbstract &) = delete;

        PipelineAbstract &operator=(const PipelineAbstract &) = delete;

        PipelineAbstract(PipelineAbstract &&) = delete;

        PipelineAbstract &operator=(PipelineAbstract &&) = delete;

        virtual void cmdBindAll(VkCommandBuffer cbh, uint32_t frameIndex, DescIndexPicker &&descIndices = {}) const = 0;



        [[nodiscard]] DescriptorRef getDescriptor(const uint32_t setIndex) const {
            return *descriptors[setIndex];
        }

        [[nodiscard]] PipelineLayoutRef getLayout() const { return pipelineLayout; }


        [[nodiscard]] std::span<const ShaderModulePtr> getShaderModules() const {
            return shaderModules;
        }

        // The stage's specialization block, or nullptr with no constants. Rebuilt here so init()
        // reads the words respecialize() may have replaced; the pointers stay valid as long as this does.
        [[nodiscard]] const VkSpecializationInfo *getSpecializationInfo() {
            if (specialization.empty()) {
                return nullptr;
            }
            specializationMap.resize(specialization.size());
            for (uint32_t i = 0; i < specialization.size(); ++i) {
                specializationMap[i] = {i, i * static_cast<uint32_t>(sizeof(uint32_t)), sizeof(uint32_t)};
            }
            specializationInfo = {
                .mapEntryCount = static_cast<uint32_t>(specializationMap.size()),
                .pMapEntries = specializationMap.data(),
                .dataSize = specialization.size() * sizeof(uint32_t),
                .pData = specialization.data()
            };
            return &specializationInfo;
        }

        // Rebuilds the pipeline with new constants. A pipeline is immutable once created, so the
        // device is drained and the old one destroyed first; unchanged words cost nothing.
        void respecialize(std::vector<uint32_t> &&data) {
            finishPreparation();
            if (data == specialization && pipeline != VK_NULL_HANDLE) {
                return;
            }
            wc.core.getLogicalDevice().waitDeviceIdle();
            destroy();
            specialization = std::move(data);
            init();
        }


        // Lets a skip leave this pipeline compiling, inside a PipelinePreparation::DeferralScope only.
        bool allowDeferral() const {
            if (!preparation || !PipelinePreparation::DeferralScope::open) {
                return false;
            }
            preparation->deferrable = true;
            return true;
        }

        // True from a skip until the pipeline is collected; `pipeline` must not be bound meanwhile.
        [[nodiscard]] bool isDeferred() const {
            return preparation && preparation->deferred;
        }

        // Takes a deferred pipeline once its compile is done, rethrowing a failure; false while it still runs.
        bool collectDeferred() {
            if (!isDeferred() ||
                preparation->result.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                return false;
            }
            finishPreparation();
            return true;
        }

        // Waits for a deferred pipeline with the preparation window up, then takes it.
        void awaitDeferred() {
            if (!isDeferred()) {
                return;
            }
            PipelinePreparation::await(preparation);
            finishPreparation();
        }

        [[nodiscard]] std::vector<VkDescriptorSet> enumerateDescriptorSets(const uint32_t frameIndex, DescIndexPicker &&descIndices = {}) const {
            std::vector<VkDescriptorSet> descriptorSets(descriptors.size());

            if (!descIndices.empty()) {
                safe_array::check_size_equal(descriptors.size(), descIndices.size(), "Descriptor Index");
            } else {
                descIndices = std::vector<uint32_t>(descriptors.size(), 0);
            }

            for (uint32_t descriptorIndex = 0; descriptorIndex < descriptors.size(); ++descriptorIndex) {
                descriptorSets[descriptorIndex] =
                    descriptors[descriptorIndex]->getDescriptorSetHandle(frameIndex, descIndices[descriptorIndex]);
            }
            return descriptorSets;
        }

    protected:
        template<class Create> void prepare(Create create) {
            const auto& device = wc.core.getPhysicalDevice().getPhysicalDeviceProperties();
            preparation = PipelinePreparation::enqueue(wc.getWindow().getWindowHandle(), shaderModules.back()->getFilename(),
                std::to_string(device.vendorID) + ":" + std::to_string(device.deviceID) + ":" + std::to_string(device.driverVersion), std::move(create));
        }
        void finishPreparation() {
            if (preparation) {
                traceBlockingWait("finishPreparation");
                auto pending = std::move(preparation);
                pending->result.get();
            }
        }
        // Notes a wait that will actually block on a compile, for the skip trace.
        void traceBlockingWait(const char *where) const noexcept {
            try {
                if (preparation &&
                    preparation->result.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                    PipelinePreparation::trace(std::string(where) + " blocks on " + preparation->progress.shader);
                }
            } catch (...) {
            }
        }
        void waitPreparation() const noexcept {
            if (preparation) {
                traceBlockingWait("destructor");
                preparation->result.wait();
            }
        }

        void destroy() override {
            if (pipeline == VK_NULL_HANDLE) {
                return;
            }
            allocator::invoke(vkDestroyPipeline, wc.core.getLogicalDevice().getLogicalDeviceHandle(), pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
    };

    using Pipeline = std::unique_ptr<PipelineAbstract>;
    using PipelinePtr = PipelineAbstract *;
    using PipelineRef = PipelineAbstract &;
}
