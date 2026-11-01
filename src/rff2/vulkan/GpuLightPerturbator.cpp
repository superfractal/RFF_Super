//
// Created by Merutilm on 8/24/26.
// Modified by Opus 5.5 on 2026-10-06, 2026-10-07, 2026-10-12, 2026-10-16, 2026-10-29
//

// Derived from Merutilm RFF 2.3.0 CPCIterate.hpp, ComputeShaderRenderManager.hpp and RFF2::fillIterationComputeShader, GPL-3.0; see NOTICE.
#include "GpuLightPerturbator.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>

#include "../../vulkan_helper/core/exception.hpp"
#include "../../vulkan_helper/core/vkh_core.hpp"
#include "../../vulkan_helper/impl/ShaderModule.hpp"
#include "../formula/Perturbator.h"

namespace merutilm::rff2 {
    namespace {
        // ---- Buffer layouts, each matching its struct or block in light_perturbation.glsl ----

        struct GpuParams {
            uint32_t extentX;
            uint32_t extentY;
            uint32_t selectionMethod;
            uint32_t levelCount;
            uint32_t compressionMethod;
            uint32_t absoluteMode;
            uint64_t maxIteration;
            uint64_t maxRefIteration;
            uint64_t mapperLength;
            double bailout2;
            uint32_t trapShapeKind;
            uint32_t trapSides;
            double trapCenterR;
            double trapCenterI;
            double trapRadius;
            double trapCos;
            double trapSin;
            double trapSectorCos;
            double trapSectorSin;
            double trapHalfCos;
            double trapHalfSin;
            double trapInnerX;
            double trapInnerY;
        };
        static_assert(sizeof(GpuParams) == 152);

        struct GpuPA {
            double anr;
            double ani;
            double bnr;
            double bni;
            double radius;
            uint64_t skip;
        };
        static_assert(sizeof(GpuPA) == 48);

        struct GpuPAF {
            float anr;
            float ani;
            float bnr;
            float bni;
            float radius;
            uint32_t pad;
            uint64_t skip;
        };
        static_assert(sizeof(GpuPAF) == 32);

        struct GpuMapper {
            uint32_t offset;
            uint32_t levels;
        };

        struct GpuPeriod {
            uint64_t period;
            uint64_t elements;
        };

        struct PushBatch {
            uint32_t batchSize;
            uint32_t ignoreMpa;
        };

        constexpr VkDeviceSize STATE_BYTES = 96;
        constexpr uint32_t WORK_GROUP_SIZE = 16;
        constexpr uint32_t MAX_BATCH_SIZE = 1u << 30;
        // Past this zoom a pixel's dc nears the bottom of float range.
        constexpr float SINGLE_PRECISION_ZOOM_LIMIT = 30.0f;
        constexpr VkBufferUsageFlags DEVICE_USAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        // Storage buffer bindings must not be empty, so an absent table still binds 16 bytes.
        VkDeviceSize bindable(const VkDeviceSize bytes) {
            return std::max<VkDeviceSize>(16, (bytes + 15) / 16 * 16);
        }

        void memoryBarrier(const VkCommandBuffer cbh, const VkAccessFlags srcAccess, const VkAccessFlags dstAccess,
                           const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage) {
            const VkMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = srcAccess,
                .dstAccessMask = dstAccess
            };
            vkCmdPipelineBarrier(cbh, srcStage, dstStage, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        }

        void bufferBarrier(const VkCommandBuffer cbh, const VkBuffer buffer, const VkAccessFlags srcAccess,
                           const VkAccessFlags dstAccess, const VkPipelineStageFlags srcStage,
                           const VkPipelineStageFlags dstStage) {
            const VkBufferMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = srcAccess,
                .dstAccessMask = dstAccess,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = buffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            };
            vkCmdPipelineBarrier(cbh, srcStage, dstStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        }

        // Doubles a batch that ends sooner than the target and halves one that takes more than twice as long.
        class BatchSizer {
            uint32_t multiplier = 1;

        public:
            [[nodiscard]] uint32_t size() const {
                return static_cast<uint32_t>(std::min<uint64_t>(
                    static_cast<uint64_t>(GpuLightPerturbator::INIT_BATCH_SIZE) * multiplier, MAX_BATCH_SIZE));
            }

            void adapt(const float seconds, const float targetSeconds) {
                if (seconds < targetSeconds &&
                    static_cast<uint64_t>(GpuLightPerturbator::INIT_BATCH_SIZE) * multiplier < MAX_BATCH_SIZE) {
                    multiplier *= 2;
                }
                if (seconds > targetSeconds * 2) {
                    multiplier = std::max<uint32_t>(1, multiplier / 2);
                }
            }

            void reset() {
                multiplier = 1;
            }
        };
    }

    // ---- Step 1: what the shaders read, laid out on the host ----

    struct GpuLightPerturbator::HostData {
        struct Upload {
            Binding binding;
            const void *data;
            VkDeviceSize bytes;
        };

        GpuParams params{};
        std::vector<double> orbit;
        std::vector<GpuPA> table;
        std::vector<GpuMapper> mapper;
        std::vector<GpuPeriod> periods;
        const std::vector<double> *dc = nullptr;
        // Filled only when the single-precision shader runs.
        bool single = false;
        std::vector<float> orbitFloat;
        std::vector<GpuPAF> tableFloat;
        std::vector<float> dcFloat;

        HostData(const LightMandelbrotPerturbator &perturbator, const FractalAttribute &calc,
                 const RndComputeShaderAttribute &settings, const uint16_t width, const uint16_t height,
                 const std::vector<double> &pixelOffsets) : dc(&pixelOffsets) {
            const LightMandelbrotReference &reference = *perturbator.getReference();
            params = {
                .extentX = width,
                .extentY = height,
                .selectionMethod = 0,
                .levelCount = 0,
                .compressionMethod = 0,
                .absoluteMode = calc.absoluteIterationMode ? 1u : 0u,
                .maxIteration = calc.maxIteration,
                .maxRefIteration = reference.longestPeriod(),
                .mapperLength = 0,
                // Squared in double for the reason LightMandelbrotPerturbator::iterate gives.
                .bailout2 = static_cast<double>(calc.bailout) * static_cast<double>(calc.bailout)
            };
            if (const TrapShapeParams &shape = perturbator.trapShape; shape.enabled) {
                params.trapShapeKind = static_cast<uint32_t>(shape.shape);
                params.trapCenterR = shape.centerReal;
                params.trapCenterI = shape.centerImag;
                params.trapRadius = shape.radius;
                params.trapCos = shape.cosAngle;
                params.trapSin = shape.sinAngle;
                params.trapSides = shape.sides;
                params.trapSectorCos = shape.sectorCos;
                params.trapSectorSin = shape.sectorSin;
                params.trapHalfCos = shape.halfCos;
                params.trapHalfSin = shape.halfSin;
                params.trapInnerX = shape.innerX;
                params.trapInnerY = shape.innerY;
            }
            copyOrbit(reference);
            if (const LightMPATable *mpa = perturbator.getTableOrNull(); mpa != nullptr && mpa->mpaPeriod != nullptr) {
                flattenTable(*mpa);
            }
            single = settings.singlePrecision && calc.logZoom <= SINGLE_PRECISION_ZOOM_LIMIT && narrowToFloat();
        }

        // Each binding's source; the state and result bindings are filled on the GPU instead.
        [[nodiscard]] std::array<Upload, 9> uploads() const {
            return {
                Upload{PARAMS, &params, sizeof(GpuParams)},
                {ORBIT, orbit.data(), orbit.size() * sizeof(double)},
                {TABLE, table.data(), table.size() * sizeof(GpuPA)},
                {MAPPER, mapper.data(), mapper.size() * sizeof(GpuMapper)},
                {PERIODS, periods.data(), periods.size() * sizeof(GpuPeriod)},
                {DC, dc->data(), dc->size() * sizeof(double)},
                {ORBIT_FLOAT, orbitFloat.data(), orbitFloat.size() * sizeof(float)},
                {TABLE_FLOAT, tableFloat.data(), tableFloat.size() * sizeof(GpuPAF)},
                {DC_FLOAT, dcFloat.data(), dcFloat.size() * sizeof(float)},
            };
        }

    private:
        // x then y per reference iteration, up to and including the longest period.
        void copyOrbit(const LightMandelbrotReference &reference) {
            const uint64_t maxRefIteration = reference.longestPeriod();
            orbit.resize(static_cast<size_t>(maxRefIteration + 1) * 2);
            for (uint64_t i = 0; i <= maxRefIteration; ++i) {
                orbit[i * 2] = reference.refReal[i];
                orbit[i * 2 + 1] = reference.refImag[i];
            }
        }

        // The table's per-slot vectors become one array, with the mapper giving each slot's offset and length.
        void flattenTable(const LightMPATable &mpa) {
            const auto &slots = mpa.tableRef.lightTable;
            if (slots.size() >= std::numeric_limits<uint32_t>::max()) {
                throw vkh::exception_invalid_args("The MPA table is too long for the GPU perturbation");
            }
            mapper.resize(slots.size());
            for (size_t i = 0; i < slots.size(); ++i) {
                if (table.size() + slots[i].size() >= static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
                    throw vkh::exception_invalid_args("The MPA table is too large for the GPU perturbation");
                }
                mapper[i] = {static_cast<uint32_t>(table.size()), static_cast<uint32_t>(slots[i].size())};
                for (const LightPA &pa: slots[i]) {
                    table.push_back({pa.anr, pa.ani, pa.bnr, pa.bni, pa.radius, pa.skip});
                }
            }
            const auto &tablePeriod = mpa.mpaPeriod->tablePeriod;
            const auto &tableElements = mpa.mpaPeriod->tableElements;
            for (size_t i = 0; i < tablePeriod.size(); ++i) {
                periods.push_back({tablePeriod[i], tableElements[i]});
            }
            params.selectionMethod = static_cast<uint32_t>(mpa.mpaSettings.mpaSelectionMethod);
            params.compressionMethod =
                    mpa.mpaSettings.mpaCompressionMethod == FrtMPACompressionMethod::NO_COMPRESSION ? 0u : 1u;
            params.levelCount = static_cast<uint32_t>(periods.size());
            params.mapperLength = mapper.size();
        }

        // Fills the float copies; false, leaving them empty, when a table coefficient is beyond float range.
        bool narrowToFloat() {
            const auto fits = [](const double v) { return std::isfinite(static_cast<float>(v)); };
            tableFloat.reserve(table.size());
            for (const GpuPA &pa: table) {
                if (!fits(pa.anr) || !fits(pa.ani) || !fits(pa.bnr) || !fits(pa.bni)) {
                    tableFloat.clear();
                    return false;
                }
                tableFloat.push_back({static_cast<float>(pa.anr), static_cast<float>(pa.ani),
                                      static_cast<float>(pa.bnr), static_cast<float>(pa.bni),
                                      static_cast<float>(pa.radius), 0, pa.skip});
            }
            orbitFloat.assign(orbit.begin(), orbit.end());
            dcFloat.assign(dc->begin(), dc->end());
            return true;
        }
    };

    // ---- Step 2: the device buffers of one render ----

    // Every buffer lives only for this render, so a long orbit does not keep its VRAM afterwards.
    struct GpuLightPerturbator::Buffers {
        const GpuLightPerturbator &owner;
        std::array<vkh::BufferContext, BINDING_COUNT> device{};
        vkh::BufferContext staging{};
        vkh::BufferContext readback{};
        std::array<VkDeviceSize, 9> stagingOffsets{};
        std::array<HostData::Upload, 9> uploads{};
        VkDeviceSize resultBytes = 0;

        explicit Buffers(const GpuLightPerturbator &owner) : owner(owner) {
        }

        ~Buffers() {
            for (auto &buffer: device) {
                owner.destroyBuffer(buffer);
            }
            owner.destroyBuffer(staging);
            owner.destroyBuffer(readback);
        }

        Buffers(const Buffers &) = delete;

        Buffers &operator=(const Buffers &) = delete;
    };

    // ---- Construction and teardown ----

    GpuLightPerturbator::GpuLightPerturbator(vkh::CoreRef core) : core(core) {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        try {
            commandPool = std::make_unique<vkh::CommandPoolImpl>(core);
            const VkCommandBufferAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = commandPool->getCommandPoolHandle(),
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1
            };
            if (vkh::allocator::invoke(vkAllocateCommandBuffers, device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
                throw vkh::exception_init("Failed to allocate the GPU perturbation command buffer!");
            }

            constexpr VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0
            };
            if (vkh::allocator::invoke(vkCreateFence, device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
                throw vkh::exception_init("Failed to create the GPU perturbation fence!");
            }

            std::array<VkDescriptorSetLayoutBinding, BINDING_COUNT> bindings{};
            for (uint32_t i = 0; i < BINDING_COUNT; ++i) {
                bindings[i] = {
                    .binding = i,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr
                };
            }
            const VkDescriptorSetLayoutCreateInfo layoutInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = BINDING_COUNT,
                .pBindings = bindings.data()
            };
            if (vkh::allocator::invoke(vkCreateDescriptorSetLayout, device, &layoutInfo, nullptr, &setLayout) !=
                VK_SUCCESS) {
                throw vkh::exception_init("Failed to create the GPU perturbation descriptor layout!");
            }

            const VkDescriptorPoolSize poolSize = {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = BINDING_COUNT
            };
            const VkDescriptorPoolCreateInfo poolInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = &poolSize
            };
            if (vkh::allocator::invoke(vkCreateDescriptorPool, device, &poolInfo, nullptr, &descriptorPool) !=
                VK_SUCCESS) {
                throw vkh::exception_init("Failed to create the GPU perturbation descriptor pool!");
            }
            const VkDescriptorSetAllocateInfo setInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &setLayout
            };
            if (vkh::allocator::invoke(vkAllocateDescriptorSets, device, &setInfo, &descriptorSet) != VK_SUCCESS) {
                throw vkh::exception_init("Failed to allocate the GPU perturbation descriptor set!");
            }

            const VkPushConstantRange pushRange = {
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .offset = 0,
                .size = sizeof(PushBatch)
            };
            const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = 1,
                .pSetLayouts = &setLayout,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &pushRange
            };
            if (vkh::allocator::invoke(vkCreatePipelineLayout, device, &pipelineLayoutInfo, nullptr,
                                       &pipelineLayout) != VK_SUCCESS) {
                throw vkh::exception_init("Failed to create the GPU perturbation pipeline layout!");
            }

            doublePipeline = createPipeline("vk_light_perturbation.comp");
            singlePipeline = createPipeline("vk_light_perturbation_single.comp");
            isolatedPipeline = createPipeline("vk_light_perturbation_isolated.comp");
        } catch (...) {
            destroy();
            throw;
        }
    }

    GpuLightPerturbator::~GpuLightPerturbator() {
        destroy();
    }

    void GpuLightPerturbator::destroy() noexcept {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        for (VkPipeline *pipeline: {&doublePipeline, &singlePipeline, &isolatedPipeline}) {
            if (*pipeline != VK_NULL_HANDLE) {
                vkh::allocator::invoke(vkDestroyPipeline, device, *pipeline, nullptr);
                *pipeline = VK_NULL_HANDLE;
            }
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkh::allocator::invoke(vkDestroyPipelineLayout, device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (descriptorPool != VK_NULL_HANDLE) {
            vkh::allocator::invoke(vkDestroyDescriptorPool, device, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
            descriptorSet = VK_NULL_HANDLE;
        }
        if (setLayout != VK_NULL_HANDLE) {
            vkh::allocator::invoke(vkDestroyDescriptorSetLayout, device, setLayout, nullptr);
            setLayout = VK_NULL_HANDLE;
        }
        if (fence != VK_NULL_HANDLE) {
            vkh::allocator::invoke(vkDestroyFence, device, fence, nullptr);
            fence = VK_NULL_HANDLE;
        }
        if (commandBuffer != VK_NULL_HANDLE) {
            vkh::allocator::invoke(vkFreeCommandBuffers, device, commandPool->getCommandPoolHandle(), 1,
                                   &commandBuffer);
            commandBuffer = VK_NULL_HANDLE;
        }
        commandPool = nullptr;
    }

    // ---- Capability checks ----

    bool GpuLightPerturbator::isSupported(vkh::CoreRef core) {
        const auto &features = core.getPhysicalDevice().getPhysicalDeviceFeatures();
        return features.shaderInt64 == VK_TRUE && features.shaderFloat64 == VK_TRUE;
    }

    bool GpuLightPerturbator::accepts(const LightMandelbrotPerturbator &perturbator) {
        const LightMandelbrotReference *reference = perturbator.getReference();
        if (reference == nullptr || !reference->compressor.empty()) {
            return false;
        }
        const uint64_t maxRefIteration = reference->longestPeriod();
        if (maxRefIteration >= std::numeric_limits<uint32_t>::max() ||
            reference->refReal.size() <= maxRefIteration || reference->refImag.size() <= maxRefIteration) {
            return false;
        }
        const LightMPATable *table = perturbator.getTableOrNull();
        return table == nullptr || table->pulledMPACompressor.empty();
    }

    // ---- Vulkan helpers ----

    VkPipeline GpuLightPerturbator::createPipeline(const char *shaderName) const {
        const vkh::ShaderModuleImpl shader(core, shaderName);
        const VkComputePipelineCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = shader.getShaderModuleHandle(),
                .pName = "main",
                .pSpecializationInfo = nullptr
            },
            .layout = pipelineLayout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };
        VkPipeline pipeline = VK_NULL_HANDLE;
        if (vkh::allocator::invoke(vkCreateComputePipelines, core.getLogicalDevice().getLogicalDeviceHandle(),
                                   core.getLogicalDevice().getPipelineCacheHandle(), 1, &info, nullptr,
                                   &pipeline) != VK_SUCCESS) {
            throw vkh::exception_init(std::string("Failed to create the pipeline of ") + shaderName);
        }
        return pipeline;
    }

    vkh::BufferContext GpuLightPerturbator::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage,
                                                         const bool hostVisible) const {
        if (!hostVisible) {
            return vkh::BufferContext::createContext(core, {
                                                         .size = size, .usage = usage,
                                                         .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                                     });
        }
        // Cached host memory makes the per-batch readback scan fast; plain coherent memory is the fallback.
        vkh::BufferContext buffer{};
        try {
            buffer = vkh::BufferContext::createContext(core, {
                                                           .size = size, .usage = usage,
                                                           .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                                         VK_MEMORY_PROPERTY_HOST_CACHED_BIT
                                                       });
        } catch (const vkh::exception &) {
            buffer = vkh::BufferContext::createContext(core, {
                                                           .size = size, .usage = usage,
                                                           .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                                       });
        }
        try {
            vkh::BufferContext::mapMemory(core, buffer);
        } catch (...) {
            vkh::BufferContext::destroyContext(core, buffer);
            throw;
        }
        return buffer;
    }

    void GpuLightPerturbator::destroyBuffer(vkh::BufferContext &buffer) const noexcept {
        if (buffer.buffer == VK_NULL_HANDLE) {
            return;
        }
        if (buffer.mappedMemory != nullptr) {
            vkh::BufferContext::unmapMemory(core, buffer);
        }
        vkh::BufferContext::destroyContext(core, buffer);
        buffer = {};
    }

    void GpuLightPerturbator::submitAndWait() const {
        const VkDevice device = core.getLogicalDevice().getLogicalDeviceHandle();
        if (const VkResult result = vkEndCommandBuffer(commandBuffer); result != VK_SUCCESS) {
            throw vkh::exception_invalid_state(std::string("Failed to end the GPU perturbation commands! ") +
                                               string_VkResult(result));
        }
        const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };
        if (const VkResult result = core.getLogicalDevice().queueSubmitResult(1, &submitInfo, fence);
            result != VK_SUCCESS) {
            throw vkh::exception_invalid_state(std::string("Failed to submit the GPU perturbation! ") +
                                               string_VkResult(result));
        }
        // Waited outside the allocator lock, which a batch of up to a few hundred milliseconds would otherwise hold.
        const VkResult waitResult = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        if (waitResult != VK_SUCCESS) {
            core.getLogicalDevice().waitDeviceIdle();
            throw vkh::exception_invalid_state(std::string("Failed to wait for the GPU perturbation! ") +
                                               string_VkResult(waitResult));
        }
        vkh::allocator::invoke(vkResetFences, device, 1, &fence);
    }

    // ---- Step 2 ----

    void GpuLightPerturbator::createBuffers(const HostData &data, const uint32_t pixelCount, Buffers &buffers) const {
        buffers.uploads = data.uploads();
        VkDeviceSize stagingSize = 0;
        for (size_t i = 0; i < buffers.uploads.size(); ++i) {
            const auto &upload = buffers.uploads[i];
            buffers.device[upload.binding] = createBuffer(bindable(upload.bytes), DEVICE_USAGE, false);
            buffers.stagingOffsets[i] = stagingSize;
            stagingSize += bindable(upload.bytes);
        }
        buffers.resultBytes = static_cast<VkDeviceSize>(pixelCount) * sizeof(PixelResult);
        buffers.device[STATE] = createBuffer(static_cast<VkDeviceSize>(pixelCount) * STATE_BYTES, DEVICE_USAGE, false);
        buffers.device[RESULT] = createBuffer(buffers.resultBytes, DEVICE_USAGE, false);
        buffers.staging = createBuffer(stagingSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        buffers.readback = createBuffer(buffers.resultBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);

        for (size_t i = 0; i < buffers.uploads.size(); ++i) {
            if (const auto &upload = buffers.uploads[i]; upload.bytes != 0) {
                std::memcpy(buffers.staging.mappedMemory + buffers.stagingOffsets[i], upload.data, upload.bytes);
            }
        }

        std::array<VkDescriptorBufferInfo, BINDING_COUNT> bufferInfos{};
        std::array<VkWriteDescriptorSet, BINDING_COUNT> writes{};
        for (uint32_t i = 0; i < BINDING_COUNT; ++i) {
            bufferInfos[i] = {.buffer = buffers.device[i].buffer, .offset = 0, .range = VK_WHOLE_SIZE};
            writes[i] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descriptorSet,
                .dstBinding = i,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &bufferInfos[i],
                .pTexelBufferView = nullptr
            };
        }
        vkh::allocator::invoke(vkUpdateDescriptorSets, core.getLogicalDevice().getLogicalDeviceHandle(),
                               BINDING_COUNT, writes.data(), 0, nullptr);
    }

    // ---- Step 3 ----

    void GpuLightPerturbator::recordBatch(const Buffers &buffers, const bool first, const bool single,
                                          const uint32_t batchSize, const bool ignoreMpa,
                                          const bool interpolateIsolatedPixels, const uint32_t width,
                                          const uint32_t height) const {
        if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS) {
            throw vkh::exception_invalid_state("Failed to reset the GPU perturbation commands!");
        }
        constexpr VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw vkh::exception_invalid_state("Failed to begin the GPU perturbation commands!");
        }

        if (first) {
            for (size_t i = 0; i < buffers.uploads.size(); ++i) {
                const auto &upload = buffers.uploads[i];
                const VkBufferCopy region = {
                    .srcOffset = buffers.stagingOffsets[i], .dstOffset = 0, .size = bindable(upload.bytes)
                };
                vkCmdCopyBuffer(commandBuffer, buffers.staging.buffer, buffers.device[upload.binding].buffer, 1,
                                &region);
            }
            vkCmdFillBuffer(commandBuffer, buffers.device[STATE].buffer, 0, VK_WHOLE_SIZE, 0);
            memoryBarrier(commandBuffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        } else {
            // A fence wait orders the host only, so the previous batch's state writes still need making visible here.
            memoryBarrier(commandBuffer, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT,
                          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }

        const uint32_t groupsX = (width + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
        const uint32_t groupsY = (height + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
        const PushBatch push = {.batchSize = batchSize, .ignoreMpa = ignoreMpa ? 1u : 0u};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet,
                                0, nullptr);
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushBatch), &push);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, single ? singlePipeline : doublePipeline);
        vkCmdDispatch(commandBuffer, groupsX, groupsY, 1);

        if (interpolateIsolatedPixels) {
            memoryBarrier(commandBuffer, VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, isolatedPipeline);
            vkCmdDispatch(commandBuffer, groupsX, groupsY, 1);
        }

        const VkBuffer resultBuffer = buffers.device[RESULT].buffer;
        bufferBarrier(commandBuffer, resultBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        const VkBufferCopy readbackRegion = {.srcOffset = 0, .dstOffset = 0, .size = buffers.resultBytes};
        vkCmdCopyBuffer(commandBuffer, resultBuffer, buffers.readback.buffer, 1, &readbackRegion);
        bufferBarrier(commandBuffer, buffers.readback.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);
    }

    // ---- The whole render ----

    bool GpuLightPerturbator::render(const ParallelRenderState &state, const LightMandelbrotPerturbator &perturbator,
                                     const FractalAttribute &calc, const RndComputeShaderAttribute &settings,
                                     const uint16_t width, const uint16_t height, const std::vector<double> &dc,
                                     Matrix<double> &output, Matrix<float> *escapeAngles, OrbitTrapMatrix *traps,
                                     std::atomic<int> &finishedPixels,
                                     const std::function<void(const Progress &)> &progress) {
        const uint32_t pixelCount = static_cast<uint32_t>(width) * height;
        if (pixelCount == 0 || dc.size() != static_cast<size_t>(pixelCount) * 2 || output.getLength() != pixelCount ||
            (escapeAngles != nullptr && escapeAngles->getLength() != pixelCount) ||
            (traps != nullptr && traps->getLength() != pixelCount)) {
            throw vkh::exception_invalid_args("GPU perturbation input does not match the iteration buffer");
        }

        const HostData data(perturbator, calc, settings, width, height, dc);
        Buffers buffers(*this);
        createBuffers(data, pixelCount, buffers);
        const auto *results = reinterpret_cast<const PixelResult *>(buffers.readback.mappedMemory);

        // Step 4: the value LightMandelbrotPerturbator::iterate would return for a finished pixel.
        const auto finishedValue = [&calc](const PixelResult &r) {
            if (calc.absoluteIterationMode) {
                return static_cast<double>(r.count);
            }
            if (r.count >= calc.maxIteration) {
                return static_cast<double>(calc.maxIteration);
            }
            return Perturbator::getQuadraticDoubleValueIteration(r.count, std::sqrt(r.pd), std::sqrt(r.cd),
                                                                 calc.decimalizeIterationMethod, calc.bailout);
        };
        // The same bailout test the CPU loop ends on; only an escaping pixel has an angle worth keeping.
        const double bailout2 = static_cast<double>(calc.bailout) * static_cast<double>(calc.bailout);
        const auto finishedAngle = [bailout2](const PixelResult &r) {
            return r.cd > bailout2 ? r.escapeAngle : 0.0f;
        };
        // The in-bounds neighbors, all of them finished by the time an interpolated pixel reads them.
        const auto neighborsOf = [width, height](const uint32_t index) {
            const uint32_t x = index % width;
            const uint32_t y = index / width;
            return std::array{std::pair{x != 0, index - 1}, std::pair{x + 1u < width, index + 1},
                              std::pair{y + 1u < height, index + width}, std::pair{y != 0, index - width}};
        };
        const auto neighborMean = [&](const uint32_t index) {
            double sum = 0;
            uint32_t count = 0;
            for (const auto [inBounds, neighbor]: neighborsOf(index)) {
                if (inBounds) {
                    sum += output.loadRelaxed(neighbor);
                    ++count;
                }
            }
            return count == 0 ? static_cast<double>(calc.maxIteration) : sum / count;
        };
        // Angles wrap at pi, so the neighbors are averaged as unit vectors rather than as numbers; 0 marks a neighbor with no angle.
        const auto neighborAngleMean = [&](const uint32_t index) {
            double sumCos = 0;
            double sumSin = 0;
            for (const auto [inBounds, neighbor]: neighborsOf(index)) {
                if (const float angle = inBounds ? escapeAngles->loadRelaxed(neighbor) : 0.0f; angle != 0.0f) {
                    sumCos += std::cos(angle);
                    sumSin += std::sin(angle);
                }
            }
            return sumCos == 0 && sumSin == 0 ? 0.0f : static_cast<float>(std::atan2(sumSin, sumCos));
        };

        const auto trapOf = [](const PixelResult &r) {
            return OrbitTrap{r.trapCross, r.trapPoint, r.trapSecond, r.trapIteration,
                             r.trapShape, r.trapShapeIteration, r.trapShapeAngle, r.trapShapeStatus};
        };
        // A trap has no mean worth taking, so an interpolated pixel copies its first in-bounds neighbor's.
        const auto neighborTrap = [&](const uint32_t index) {
            for (const auto [inBounds, neighbor]: neighborsOf(index)) {
                if (inBounds) return traps->loadRelaxed(neighbor);
            }
            return OrbitTrap{0.0f, 0.0f, 0.0f, 0.0f};
        };

        std::vector<uint8_t> done(pixelCount, 0);
        std::vector<uint32_t> interpolated;
        uint64_t unfinished = pixelCount;
        const auto finish = [&](const uint32_t index, const double value, const float angle, const OrbitTrap &trap) {
            output.storeRelaxed(index, value);
            if (escapeAngles != nullptr) {
                escapeAngles->storeRelaxed(index, angle);
            }
            if (traps != nullptr) {
                traps->storeRelaxed(index, trap);
            }
            done[index] = 1;
            --unfinished;
            ++finishedPixels;
        };

        BatchSizer sizer;
        bool ignoreMpa = settings.ignoreMpa;
        for (uint32_t batch = 0; unfinished > settings.allowedUnfinishedPixels; ++batch) {
            if (state.interruptRequested()) {
                return false;
            }
            const uint32_t batchSize = sizer.size();
            const auto begin = std::chrono::high_resolution_clock::now();
            recordBatch(buffers, batch == 0, data.single, batchSize, ignoreMpa, settings.interpolateIsolatedPixels,
                        width, height);
            submitAndWait();
            const float seconds = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - begin).
                    count();

            interpolated.clear();
            for (uint32_t index = 0; index < pixelCount; ++index) {
                if (done[index]) {
                    continue;
                }
                const PixelResult &r = results[index];
                switch (r.status) {
                    case FINISHED: finish(index, finishedValue(r), finishedAngle(r), trapOf(r));
                        break;
                    case INTERIOR: finish(index, static_cast<double>(calc.maxIteration), 0.0f, trapOf(r));
                        break;
                    case SKY: finish(index, static_cast<double>(calc.maxIteration), 0.0f, OrbitTrap{0.0f, 0.0f, 0.0f, 0.0f});
                        break;
                    case INTERPOLATED: interpolated.push_back(index);
                        break;
                    default:
                        // Still iterating: shows the count reached so far, as the original batches did.
                        output.storeRelaxed(index, static_cast<double>(r.count));
                        break;
                }
            }
            for (const uint32_t index: interpolated) {
                finish(index, neighborMean(index), escapeAngles != nullptr ? neighborAngleMean(index) : 0.0f,
                       traps != nullptr ? neighborTrap(index) : OrbitTrap{});
            }

            if (progress) {
                progress({.batch = batch, .batchSize = batchSize, .unfinished = unfinished, .seconds = seconds});
            }

            sizer.adapt(seconds, settings.batchSeconds);
            const bool nextIgnoreMpa = settings.ignoreMpa ||
                                       (settings.stopMpaAfterBatches != 0 && batch > settings.stopMpaAfterBatches);
            if (nextIgnoreMpa != ignoreMpa) {
                sizer.reset();
                ignoreMpa = nextIgnoreMpa;
            }
        }
        return true;
    }
}
