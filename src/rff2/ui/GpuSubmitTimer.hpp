//
// Created by Opus 5.5 on 2026-10-04.
//

#pragma once
#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <vector>

#include "GpuPassTimer.hpp"

namespace merutilm::rff2 {
    // GPU time of each queue submission in a video frame, the unit Windows' GPU watchdog (TDR) times out.
    struct GpuSubmitTimer {
        static constexpr uint32_t MAX_SEGMENTS = 256;

        struct Stats {
            uint64_t frames = 0;
            uint32_t lastSegments = 0;
            double lastLongestMs = 0.0;
            std::string lastLongestLabel;
            double lastFrameMs = 0.0;
            double worstMs = 0.0;
            std::string worstLabel;
            uint64_t worstFrame = 0;
            double longestSumMs = 0.0;
        };

        explicit GpuSubmitTimer(vkh::CoreRef core) : core(core) {
            const auto &props = core.getPhysicalDevice().getPhysicalDeviceProperties();
            timestampPeriod = props.limits.timestampPeriod;
            const uint32_t validBits = GpuPassTimer::queueTimestampValidBits(core);
            timestampMask = validBits >= 64 ? ~0ull : (1ull << validBits) - 1ull;
            supported = timestampPeriod > 0.0f && props.limits.timestampComputeAndGraphics == VK_TRUE &&
                        validBits > 0;
            labels.resize(MAX_SEGMENTS);
        }

        ~GpuSubmitTimer() {
            if (pool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(core.getLogicalDevice().getLogicalDeviceHandle(), pool, nullptr);
            }
        }

        GpuSubmitTimer(const GpuSubmitTimer &) = delete;

        GpuSubmitTimer &operator=(const GpuSubmitTimer &) = delete;

        GpuSubmitTimer(GpuSubmitTimer &&) = delete;

        GpuSubmitTimer &operator=(GpuSubmitTimer &&) = delete;

        // The query pool is only made once timing is asked for, so a renderer that never logs pays nothing.
        void setEnabled(const bool on) {
            enabled = on && supported;
            if (enabled && pool == VK_NULL_HANDLE) {
                const VkQueryPoolCreateInfo info{
                    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queryType = VK_QUERY_TYPE_TIMESTAMP,
                    .queryCount = MAX_SEGMENTS * 2,
                    .pipelineStatistics = 0,
                };
                if (vkCreateQueryPool(core.getLogicalDevice().getLogicalDeviceHandle(), &info, nullptr, &pool) !=
                    VK_SUCCESS) {
                    pool = VK_NULL_HANDLE;
                    enabled = false;
                }
            }
        }

        [[nodiscard]] bool isEnabled() const { return enabled; }

        [[nodiscard]] bool isSupported() const { return supported; }

        // First command of the frame's first submission.
        void cmdBeginFrame(const VkCommandBuffer cbh) {
            segments = 0;
            if (!enabled) {
                return;
            }
            vkCmdResetQueryPool(cbh, pool, 0, MAX_SEGMENTS * 2);
            cmdBeginSegment(cbh);
        }

        void cmdBeginSegment(const VkCommandBuffer cbh) {
            if (!enabled || segments >= MAX_SEGMENTS) {
                return;
            }
            vkCmdWriteTimestamp(cbh, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, segments * 2);
        }

        // Last command of a submission; the label names the pass that closes it.
        void cmdEndSegment(const VkCommandBuffer cbh, std::string label) {
            if (!enabled || segments >= MAX_SEGMENTS) {
                return;
            }
            vkCmdWriteTimestamp(cbh, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, segments * 2 + 1);
            labels[segments] = std::move(label);
            ++segments;
        }

        // Call after the fence of the frame's last submission has been waited on.
        void collect() {
            if (!enabled || segments == 0) {
                return;
            }
            const uint32_t count = segments;
            segments = 0;
            std::vector<uint64_t> stamps(count * 2);
            if (vkGetQueryPoolResults(core.getLogicalDevice().getLogicalDeviceHandle(), pool, 0, count * 2,
                                      sizeof(uint64_t) * stamps.size(), stamps.data(), sizeof(uint64_t),
                                      VK_QUERY_RESULT_64_BIT) != VK_SUCCESS) {
                return;
            }
            double longest = 0.0;
            double total = 0.0;
            uint32_t longestIndex = 0;
            for (uint32_t i = 0; i < count; ++i) {
                const uint64_t delta = ((stamps[i * 2 + 1] & timestampMask) - (stamps[i * 2] & timestampMask)) &
                                       timestampMask;
                const double ms = static_cast<double>(delta) * timestampPeriod / 1e6;
                total += ms;
                if (ms > longest) {
                    longest = ms;
                    longestIndex = i;
                }
            }
            ++stats.frames;
            stats.lastSegments = count;
            stats.lastLongestMs = longest;
            stats.lastLongestLabel = labels[longestIndex];
            stats.lastFrameMs = total;
            stats.longestSumMs += longest;
            if (longest > stats.worstMs) {
                stats.worstMs = longest;
                stats.worstLabel = labels[longestIndex];
                stats.worstFrame = stats.frames;
            }
        }

        [[nodiscard]] const Stats &getStats() const { return stats; }

    private:
        vkh::CoreRef core;
        VkQueryPool pool = VK_NULL_HANDLE;
        uint64_t timestampMask = 0;
        float timestampPeriod = 0.0f;
        bool supported = false;
        bool enabled = false;
        uint32_t segments = 0;
        std::vector<std::string> labels;
        Stats stats;
    };
}
