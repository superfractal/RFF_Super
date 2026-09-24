//
// Created by Merutilm on 2025-07-18.
// Modified by Opus 5 on 2026-08-21, 2026-09-03
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <cstddef>

namespace merutilm::vkh {
    struct BoostHasher {
        BoostHasher() = delete;

        // The seed mixing is the boost::hash_combine form, used under the Boost Software License 1.0;
        // see NOTICE. 0x9e3779b9 is the golden-ratio fraction.
        static void hash(const size_t currentHashValue, size_t *seed) {
            *seed ^= currentHashValue + 0x9e3779b9 + (*seed << 6) + (*seed >> 2);
        }
    };
}
