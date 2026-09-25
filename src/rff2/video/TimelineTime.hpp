// Modified by GPT-6 on 2026-09-26
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>
namespace merutilm::rff2 {
    struct TimelineTime {
        static double frameSeconds(uint64_t frame, double fps) { return double(frame) / fps; }
        static double elapsed(double origin, uint64_t started, uint64_t now) {
            return origin + double(now >= started ? now - started : 0) / 1000.0;
        }
        static std::wstring display(double seconds) {
            const auto ticks = static_cast<uint64_t>(std::llround(std::clamp(seconds, 0.0, 1e12) * 10));
            const auto days = ticks / 864000, hours = ticks / 36000 % 24, minutes = ticks / 600 % 60;
            const auto secs = ticks / 10 % 60, tenth = ticks % 10;
            if (days) return std::format(L"{}d {:02}:{:02}:{:02}.{}", days, hours, minutes, secs, tenth);
            if (hours) return std::format(L"{:02}:{:02}:{:02}.{}", hours, minutes, secs, tenth);
            return std::format(L"{:02}:{:02}.{}", minutes, secs, tenth);
        }
    };
}
