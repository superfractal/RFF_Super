//
// Modified by GPT-6 on 2026-09-14, 2026-09-18, 2026-09-19, 2026-09-23
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace merutilm::rff2::workspace {
struct TimelineItems {
    static constexpr long frames = 1;
    static constexpr long load = 2;
    static constexpr long save = 3;
    static constexpr long exportVideo = 4;
    static constexpr long theme = 5;
    static constexpr long fullscreen = 6;
    static constexpr long play = 7;
    static constexpr long pause = 8;
    static constexpr long stop = 9;
    static constexpr long loop = 10;
    static constexpr long distance = 11;
    static constexpr long keyframe = 12;
    static constexpr long zoom = 13;
    static constexpr long time = 14;
    static constexpr long viewZoom = 15;
    static constexpr long controls = 16;
    static constexpr long overlay = 17;
    static constexpr long ai = 18;
    static constexpr long divider = 50;
    static constexpr long toggle = 51;
    static constexpr long editor = 52;
    static constexpr long tracks = 1000;
    static constexpr long keys = 100000;

    std::unordered_map<uint64_t, long> keyIds;
    std::vector<std::pair<uint16_t, int>> keyTargets;

    static long track(uint16_t target) {
        return tracks + target;
    }

    long key(uint16_t target, int index) {
        const uint64_t packedKey = (uint64_t(target) << 32) | uint32_t(index);
        if (const auto found = keyIds.find(packedKey); found != keyIds.end()) {
            return found->second;
        }
        if (keyTargets.size() >= size_t(std::numeric_limits<long>::max() - keys)) {
            throw std::length_error("Too many accessible timeline keys");
        }

        const long id = keys + long(keyTargets.size());
        keyTargets.emplace_back(target, index);
        keyIds.emplace(packedKey, id);
        return id;
    }

    static bool isTrack(long id) {
        return id >= tracks && id < tracks + 65536;
    }

    static bool isKey(long id) {
        return id >= keys;
    }

    uint16_t target(long id) const {
        if (isKey(id)) {
            return keyTargets.at(size_t(id - keys)).first;
        }
        return uint16_t(id - tracks);
    }

    int index(long id) const {
        return keyTargets.at(size_t(id - keys)).second;
    }
};
} // namespace merutilm::rff2::workspace
