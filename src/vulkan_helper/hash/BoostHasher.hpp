//
// Created by Merutilm on 2025-07-18.
// Modified by Opus 5 on 2026-08-21
//

#pragma once
namespace merutilm::vkh {
    struct BoostHasher {
        BoostHasher() = delete;

        // The seed mixing is the well-known boost::hash_combine form; 0x9e3779b9 is the golden-ratio fraction.
        static void hash(const size_t currHashValue, size_t *seed) {
            *seed ^= currHashValue + 0x9e3779b9 + (*seed << 6) + (*seed >> 2);
        }
    };
}