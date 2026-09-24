//
// Created by Merutilm on 2025-07-18.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <vector>

#include "BoostHasher.hpp"

namespace merutilm::vkh {

    template<typename T, typename Hasher = std::hash<T>>
    struct VectorHasher {
        using is_transparent = void;

        size_t operator()(const std::vector<T> &values) const {
            return operator()(std::span<const T>(values));
        }

        size_t operator()(const std::span<const T> &values) const {
            size_t seed = 0;
            for (size_t index = 0; index < values.size(); ++index) {
                BoostHasher::hash(Hasher{}(values[index]), &seed);
            }
            return seed;
        }
    };
}
