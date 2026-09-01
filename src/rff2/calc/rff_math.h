//
// Created by Merutilm on 2025-05-09.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-25
//

#pragma once
#include <cmath>
#include <random>

namespace merutilm::rff2 {
    struct rff_math {

        inline static auto rd = std::random_device();
        inline static auto gen = std::mt19937(rd());

        // Maps a 32-bit mt19937 draw to [0, 1). Fixed arithmetic (not std::uniform_*_distribution,
        // whose algorithm is implementation-defined) so palette recipes reproduce across toolchains.
        static constexpr double UINT32_TO_UNIT = 1.0 / 4294967296.0;

        static double hypot_approx(double x, double y) {
            x = fabs(x);
            y = fabs(y);
            const double min = std::min(x, y);
            const double max = std::max(x, y);

            if (min == 0) {
                return max;
            }
            if (max == 0) {
                return 0;
            }

            return max + 0.428 * min / max * min;
        }
        // Reseeds the shared generator for deterministic palette regeneration from a saved recipe.
        static void reseed(const uint32_t seed) {
            gen = std::mt19937(seed);
        }
        // Lends the shared generator out and takes it back, so a regeneration that has to reseed it
        // does not also decide what every later random draw comes out as.
        static std::mt19937 captureState() {
            return gen;
        }

        static void restoreState(const std::mt19937 &state) {
            gen = state;
        }
        // Draws a fresh non-deterministic seed to record alongside a generated palette recipe.
        static uint32_t randomSeed() {
            return rd();
        }
        static float random_i() {
            return static_cast<float>(static_cast<uint32_t>(gen()) >> 24);
        }
        static float random_f() {
            return static_cast<float>(static_cast<uint32_t>(gen()) * UINT32_TO_UNIT);
        }

        static double random_d() {
            return static_cast<uint32_t>(gen()) * UINT32_TO_UNIT;
        }
    };
}
