//
// Created by Fable 5.1 on 2026-09-06
//

#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "../attr/ShdPaletteAttribute.h"
#include "../attr/ShdPatternAttribute.h"
#include "../attr/ShdStripeAttribute.h"
#include "../attr/ShdTextureAttribute.h"
#include "../attr/ShdWarpAttribute.h"

namespace merutilm::rff2 {
    // The modes vk_iteration_palette.frag and vk_2_map_iter_stripe.comp switch on, packed the way
    // both declare their specialization constants: word i is constant_id i. Each is also written
    // to its uniform as before, so a pipeline built without these is what it always was; with them,
    // the switch arms a mode never takes are not compiled. Anything true here that the uniform
    // says is off costs only the unspecialized code, so every flag is set whenever it might be on.
    struct ShaderModeSpecialization {
        uint32_t smoothing = 0;
        uint32_t stripeType = 0;
        uint32_t animationMode = 0;
        bool decor = false;
        bool freeze = false;
        bool dither = false;

        bool operator==(const ShaderModeSpecialization &) const = default;

        [[nodiscard]] std::vector<uint32_t> words() const {
            return {1u, smoothing, stripeType, animationMode, decor ? 1u : 0u, freeze ? 1u : 0u, dither ? 1u : 0u};
        }

        // The palette's packed mode word: low 8 bits = smoothing method, bit 8 = interpolation
        // space, bit 9 = cycle curve, bits 10-13 = iteration coloring. The one place it is packed,
        // so the uniform and the constant cannot disagree.
        static uint32_t smoothingWord(const ShdPaletteAttribute &palette) {
            return (static_cast<uint32_t>(palette.colorSmoothing) & 0xFFu) |
                   (static_cast<uint32_t>(palette.colorInterpolation) << 8) |
                   (static_cast<uint32_t>(palette.cycleCurve) << 9) |
                   ((static_cast<uint32_t>(palette.iterationColoring) & 0xFu) << 10);
        }

        void setPalette(const ShdPaletteAttribute &palette) {
            smoothing = smoothingWord(palette);
            animationMode = static_cast<uint32_t>(palette.animationMode);
            freeze = !palette.staticColorIterations.empty();
        }

        // The word alone: the timeline rewrites it without touching the rest of the palette.
        void setPaletteDynamic(const ShdPaletteAttribute &palette) {
            smoothing = smoothingWord(palette);
        }

        void setStripe(const ShdStripeAttribute &stripe) {
            stripeType = static_cast<uint32_t>(stripe.stripeType);
        }

        template<size_t N>
        void setTextures(const std::array<ShdTextureAttribute, N> &textures) {
            anyTexture = false;
            for (const auto &t: textures) {
                anyTexture = anyTexture || (t.enabled && t.opacity > 0.0f);
            }
            decor = anyTexture || anyPattern || warpOn;
        }

        template<size_t N>
        void setPattern(const std::array<ShdPatternAttribute, N> &patterns) {
            anyPattern = false;
            for (const auto &p: patterns) {
                anyPattern = anyPattern || (p.enabled && p.opacity > 0.0f);
            }
            decor = anyTexture || anyPattern || warpOn;
        }

        void setWarp(const ShdWarpAttribute &warp) {
            warpOn = warp.enabled && warp.amount != 0.0f;
            decor = anyTexture || anyPattern || warpOn;
        }

    private:
        bool anyTexture = false;
        bool anyPattern = false;
        bool warpOn = false;
    };
}
