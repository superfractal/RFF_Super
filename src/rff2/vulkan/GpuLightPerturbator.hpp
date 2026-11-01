//
// Created by Merutilm on 8/24/26.
// Modified by Opus 5.5 on 2026-10-06, 2026-10-07, 2026-10-12, 2026-10-16
//

// Derived from Merutilm RFF 2.3.0 CPCIterate.hpp, ComputeShaderRenderManager.hpp and RFF2::fillIterationComputeShader, GPL-3.0; see NOTICE.
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include "../../vulkan_helper/impl/Core.hpp"
#include "../../vulkan_helper/impl/CommandPool.hpp"
#include "../../vulkan_helper/context/BufferContext.hpp"
#include "../attr/FractalAttribute.h"
#include "../attr/RndComputeShaderAttribute.h"
#include "../data/Matrix.h"
#include "../data/OrbitTrapMatrix.h"
#include "../formula/LightMandelbrotPerturbator.h"
#include "../parallel/ParallelRenderState.h"

namespace merutilm::rff2 {
    // Iterates a LightMandelbrotPerturbator on the GPU in batches; owns its pipelines so the per-resize renderer rebuild cannot pull them away.
    // A render builds HostData, uploads it into Buffers, runs batches, and finishes each pixel's smoothing on the host.
    class GpuLightPerturbator final {
    public:
        struct Progress {
            uint32_t batch;
            uint32_t batchSize;
            uint64_t unfinished;
            float seconds;
        };

        static constexpr uint32_t INIT_BATCH_SIZE = 16;

        explicit GpuLightPerturbator(vkh::CoreRef core);

        ~GpuLightPerturbator();

        GpuLightPerturbator(const GpuLightPerturbator &) = delete;

        GpuLightPerturbator &operator=(const GpuLightPerturbator &) = delete;

        GpuLightPerturbator(GpuLightPerturbator &&) = delete;

        GpuLightPerturbator &operator=(GpuLightPerturbator &&) = delete;

        // The shader needs 64-bit integers and doubles.
        static bool isSupported(vkh::CoreRef core);

        // A compressed reference or table is indexed in ways the shader does not reproduce.
        static bool accepts(const LightMandelbrotPerturbator &perturbator);

        // dc holds x then y per pixel with the perturbator's offset added, NaN for a sky pixel; returns false when interrupted, throws when the GPU cannot run it.
        // escapeAngles, when given, receives each escaping pixel's angle of z and stays 0 elsewhere; traps likewise receives each pixel's OrbitTrap.
        bool render(const ParallelRenderState &state, const LightMandelbrotPerturbator &perturbator,
                    const FractalAttribute &calc, const RndComputeShaderAttribute &settings, uint16_t width,
                    uint16_t height, const std::vector<double> &dc, Matrix<double> &output,
                    Matrix<float> *escapeAngles, OrbitTrapMatrix *traps,
                    std::atomic<int> &finishedPixels, const std::function<void(const Progress &)> &progress);

    private:
        // Descriptor bindings, matching light_perturbation.glsl.
        enum Binding : uint32_t {
            PARAMS = 0,
            ORBIT = 1,
            TABLE = 2,
            MAPPER = 3,
            PERIODS = 4,
            DC = 5,
            STATE = 6,
            RESULT = 7,
            ORBIT_FLOAT = 8,
            TABLE_FLOAT = 9,
            DC_FLOAT = 10,
            BINDING_COUNT = 11
        };

        // Pixel status, matching STATUS_* in light_perturbation.glsl.
        enum Status : uint32_t {
            RUNNING = 0,
            FINISHED = 1,
            INTERIOR = 2,
            SKY = 3,
            INTERPOLATED = 4
        };

        // Must match PixelResult in light_perturbation.glsl.
        struct PixelResult {
            double pd;
            double cd;
            uint64_t count;
            uint32_t status;
            float escapeAngle;
            float trapCross;
            float trapPoint;
            float trapSecond;
            float trapIteration;
            float trapShape;
            float trapShapeIteration;
            float trapShapeAngle;
            float trapShapeStatus;
        };
        static_assert(sizeof(PixelResult) == 64);

        struct HostData;
        struct Buffers;

        vkh::CoreRef core;
        vkh::CommandPool commandPool = nullptr;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline doublePipeline = VK_NULL_HANDLE;
        VkPipeline singlePipeline = VK_NULL_HANDLE;
        VkPipeline isolatedPipeline = VK_NULL_HANDLE;

        void destroy() noexcept;

        VkPipeline createPipeline(const char *shaderName) const;

        vkh::BufferContext createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible) const;

        void destroyBuffer(vkh::BufferContext &buffer) const noexcept;

        // Step 2: allocates every binding, stages the host data and points the descriptor set at the buffers.
        void createBuffers(const HostData &data, uint32_t pixelCount, Buffers &buffers) const;

        // Step 3: records one batch; the first also uploads the host data and clears the pixel states.
        void recordBatch(const Buffers &buffers, bool first, bool single, uint32_t batchSize, bool ignoreMpa,
                         bool interpolateIsolatedPixels, uint32_t width, uint32_t height) const;

        void submitAndWait() const;
    };
}
