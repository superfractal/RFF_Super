//
// Created by Opus 5 on 2026-08-10.
// Modified by Opus 5 on 2026-08-26, 2026-08-31
//

#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "../../vulkan_helper/core/vkh.hpp"

namespace merutilm::rff2 {
    // Per-pass GPU timing for the video export chain, via timestamp queries.
    // Safe to leave in: queueImage already fully waits on the fence, so collect() never adds a stall.
    struct GpuPassTimer {
        static constexpr uint32_t MAX_MARKS = 16;

        // One set of marks per slot. A renderer with frames in flight passes its frame index, so a
        // frame still on the GPU keeps its own timestamps instead of having them reset and rewritten
        // by the frame being recorded behind it. A caller that submits one frame at a time and waits
        // for it - the video export - leaves this at one.
        explicit GpuPassTimer(vkh::CoreRef core, const uint32_t slotCount = 1) : core(core) {
            slots = slotCount == 0 ? 1 : slotCount;
            pending.assign(slots, 0);
            const auto &props = core.getPhysicalDevice().getPhysicalDeviceProperties();
            timestampPeriod = props.limits.timestampPeriod;
            const uint32_t validBits = queueTimestampValidBits(core);
            // Only the low bits of a timestamp carry a value; a queue that writes none of them
            // supports no timing at all, and the rest above them are not the counter.
            timestampMask = validBits >= 64 ? ~0ull : (1ull << validBits) - 1ull;
            supported = timestampPeriod > 0.0f && props.limits.timestampComputeAndGraphics == VK_TRUE &&
                        validBits > 0;
            if (!supported) {
                return;
            }
            const VkQueryPoolCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queryType = VK_QUERY_TYPE_TIMESTAMP,
                .queryCount = MAX_MARKS * slots,
                .pipelineStatistics = 0,
            };
            if (vkCreateQueryPool(core.getLogicalDevice().getLogicalDeviceHandle(), &info, nullptr, &pool)
                != VK_SUCCESS) {
                supported = false;
                pool = VK_NULL_HANDLE;
            }
        }

        ~GpuPassTimer() {
            if (pool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(core.getLogicalDevice().getLogicalDeviceHandle(), pool, nullptr);
            }
        }

        GpuPassTimer(const GpuPassTimer &) = delete;

        GpuPassTimer &operator=(const GpuPassTimer &) = delete;

        GpuPassTimer(GpuPassTimer &&) = delete;

        GpuPassTimer &operator=(GpuPassTimer &&) = delete;

        [[nodiscard]] bool isSupported() const { return supported; }

        // Must be the first command recorded in the frame.
        void cmdReset(const VkCommandBuffer cbh, const uint32_t slot = 0) {
            if (!supported || slot >= slots) return;
            vkCmdResetQueryPool(cbh, pool, slot * MAX_MARKS, MAX_MARKS);
            pending[slot] = 0;
        }

        // Records "everything up to here is done". Call once after each pass, in a fixed order.
        void cmdMark(const VkCommandBuffer cbh, const std::string &label, const uint32_t slot = 0) {
            if (!supported || slot >= slots || pending[slot] >= MAX_MARKS) return;
            if (labels.size() < static_cast<size_t>(pending[slot]) + 1) {
                labels.emplace_back(label);
                totals.emplace_back(0.0);
            }
            vkCmdWriteTimestamp(cbh, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool,
                                slot * MAX_MARKS + pending[slot]);
            ++pending[slot];
        }

        // Call after the fence of the frame that wrote this slot has been waited on.
        void collect(const uint32_t slot = 0) {
            if (!supported || slot >= slots || pending[slot] < 2) return;
            const uint32_t count = pending[slot];
            std::array<uint64_t, MAX_MARKS> stamps{};
            if (vkGetQueryPoolResults(core.getLogicalDevice().getLogicalDeviceHandle(), pool,
                                      slot * MAX_MARKS, count,
                                      sizeof(uint64_t) * count, stamps.data(), sizeof(uint64_t),
                                      VK_QUERY_RESULT_64_BIT) != VK_SUCCESS) {
                return;
            }
            for (uint32_t i = 1; i < count; ++i) {
                // Masked before and after the subtraction, so a counter that wrapped inside its own
                // width still reads as the short interval it was rather than a whole period.
                const uint64_t delta = (stamps[i] & timestampMask) - (stamps[i - 1] & timestampMask);
                totals[i] += static_cast<double>(delta & timestampMask) * timestampPeriod / 1e6;
            }
            ++frames;
        }

        // Drops what has been gathered so far, so a measurement starts from the frames that follow
        // rather than from an average of everything the window has ever drawn.
        void clear() {
            std::ranges::fill(totals, 0.0);
            frames = 0;
        }

        [[nodiscard]] std::wstring report() const {
            if (!supported) {
                return L"  (timestamp queries unsupported on this device)\n";
            }
            if (frames == 0) {
                return L"  (no samples)\n";
            }
            std::wstring out;
            double sum = 0.0;
            for (uint32_t i = 1; i < labels.size(); ++i) sum += totals[i];
            for (uint32_t i = 1; i < labels.size(); ++i) {
                const double perFrame = totals[i] / static_cast<double>(frames);
                const std::wstring name(labels[i].begin(), labels[i].end());
                out += std::format(L"  {:<22}{:8.2f} ms/sample  ({:5.1f}%)\n",
                                   name, perFrame, sum > 0.0 ? totals[i] / sum * 100.0 : 0.0);
            }
            out += std::format(L"  {:<22}{:8.2f} ms/sample\n", L"[sum]",
                               sum / static_cast<double>(frames));
            return out;
        }

    private:
        // Valid bits belong to the queue family the work is submitted on, not to the device.
        static uint32_t queueTimestampValidBits(vkh::CoreRef core) {
            const auto &indices = core.getPhysicalDevice().getQueueFamilyIndices();
            if (!indices.graphicsAndComputeFamily.has_value()) {
                return 0;
            }
            const VkPhysicalDevice physicalDevice = core.getPhysicalDevice().getPhysicalDeviceHandle();
            uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
            std::vector<VkQueueFamilyProperties> families(count);
            if (count == 0) {
                return 0;
            }
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());
            const uint32_t family = *indices.graphicsAndComputeFamily;
            return family < count ? families[family].timestampValidBits : 0;
        }

        vkh::CoreRef core;
        VkQueryPool pool = VK_NULL_HANDLE;
        uint64_t timestampMask = 0;
        float timestampPeriod = 0.0f;
        bool supported = false;
        uint32_t slots = 1;
        std::vector<uint32_t> pending;
        uint64_t frames = 0;
        std::vector<std::string> labels;
        std::vector<double> totals;
    };
}
