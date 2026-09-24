//
// Modified by GPT-6 on 2026-09-19, 2026-09-21, 2026-09-23
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace merutilm::rff2 {
    struct VidAudioClip {
        uint64_t id = 0;
        std::string path;
        int64_t sourceDuration = 0;
        int64_t start = 0;
        int64_t in = 0;
        int64_t out = 0;
        int64_t fadeIn = 0;
        int64_t fadeOut = 0;
        float gain = 1;
        bool muted = false;
        int64_t duration() const {
            return out - in;
        }
        bool operator==(const VidAudioClip &) const = default;
    };

    struct VidAudioAttribute {
        // All audio times are integer microseconds from video time zero or the source file's start.
        static constexpr int64_t ticksPerSecond = 1000000;
        static constexpr int64_t maximumTime = 7 * 24 * 60 * 60 * ticksPerSecond;
        static constexpr uint32_t maximumClips = 1024;
        static constexpr uint32_t maximumPathBytes = 32768;
        bool exportEnabled = true;
        float gain = 1;
        std::vector<VidAudioClip> clips;
        bool operator==(const VidAudioAttribute &) const = default;

        bool valid() const;
    };
}
