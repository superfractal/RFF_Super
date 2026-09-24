//
// Modified by GPT-6 on 2026-09-16, 2026-09-17, 2026-09-23
//

#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace merutilm::rff2 {
    // Serialized IDs remain fixed; palette coordinates and output encoding are not compositing effects.
    enum class ShdLayer : uint32_t {
        BAND_LINE = 1, TEXTURE_1 = 2, TEXTURE_2 = 3, TEXTURE_3 = 4, TEXTURE_4 = 5,
        PATTERN_1 = 6, PATTERN_2 = 7, PATTERN_3 = 8, PATTERN_4 = 9, STRIPE = 10,
        SURFACE = 11, METAL = 12, SIGIL = 13, PHONK = 14, FROST = 15, SEA = 16,
        PRINT = 17, SURFACE_COLOR = 18, EFFECT_1 = 19, EFFECT_2 = 20, EFFECT_3 = 21,
        EFFECT_4 = 22, COLOR = 23, FOG = 24, BLOOM = 25, TONE_MAP = 26,
        PRINT_FINISH = 27, VHS = 28, MONO = 29
    };

    struct ShdLayerOrder {
        static constexpr uint32_t COUNT = 29;
        static constexpr std::array<ShdLayer, COUNT> defaults() {
            std::array<ShdLayer, COUNT> result{};
            for (uint32_t i = 0; i < COUNT; ++i) {
                result[i] = ShdLayer(i + 1);
            }
            return result;
        }

        bool enabled = false;
        std::array<ShdLayer, COUNT> layers = defaults();
        uint32_t hiddenMask = 0;

        bool visible(ShdLayer layer) const {
            const auto id = uint32_t(layer);
            return id >= 1 && id <= COUNT && (hiddenMask & (1u << (id - 1))) == 0;
        }

        void setVisible(ShdLayer layer, bool value) {
            const auto id = uint32_t(layer);
            if (id < 1 || id > COUNT) {
                return;
            }

            const auto bit = 1u << (id - 1);
            hiddenMask = value ? hiddenMask & ~bit : hiddenMask | bit;
            enabled = true;
        }

        bool valid() const {
            if (hiddenMask & ~((1u << COUNT) - 1)) {
                return false;
            }

            std::array<bool, COUNT> seen{};
            for (const auto layer : layers) {
                const uint32_t id = uint32_t(layer);
                if (id == 0 || id > COUNT || seen[id - 1]) {
                    return false;
                }
                seen[id - 1] = true;
            }
            return true;
        }

        bool move(uint32_t from, uint32_t to) {
            if (from >= COUNT || to >= COUNT || from == to) {
                return false;
            }

            if (from < to) {
                std::rotate(layers.begin() + from, layers.begin() + from + 1, layers.begin() + to + 1);
            } else {
                std::rotate(layers.begin() + to, layers.begin() + from, layers.begin() + from + 1);
            }
            enabled = true;
            return true;
        }

        bool operator==(const ShdLayerOrder&) const = default;
    };
}
