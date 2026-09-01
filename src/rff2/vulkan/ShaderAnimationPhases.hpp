//
// Created by Opus 5 on 2026-08-26.
//

#pragma once
#include <array>

#include "SharedDescriptorTemplate.hpp"
#include "../attr/ShdPaletteAttribute.h"
#include "../attr/ShdPatternAttribute.h"
#include "../attr/ShdStripeAttribute.h"
#include "../attr/ShdTextureAttribute.h"
#include "../attr/ShdWarpAttribute.h"

namespace merutilm::rff2 {
    // Every animated quantity the iteration and stripe passes draw with is a rate. The phase such a
    // rate has reached cannot be recovered from the rate alone - speed * elapsed answers for the new
    // speed as though it had always been in effect - so the shaders are handed the phase itself,
    // added up here. Changing a speed then carries on from where the animation stands instead of
    // jumping by elapsed * (new - old).
    struct ShaderAnimationPhases {
        // One rate and the phase it has run up.
        struct Axis {
            double phase = 0.0;
            float speed = 0.0f;
        };

        struct Scroll {
            Axis u;
            Axis v;
        };

        Axis palette = {};
        Axis flow = {};
        Axis stripe = {};
        std::array<Scroll, TEXTURE_LAYER_COUNT> texture = {};
        std::array<Scroll, PATTERN_LAYER_COUNT> pattern = {};
        Scroll warp = {};

        // A clock that went backwards, or forward by this much at once, was seeked rather than
        // played: nothing ran across the gap to add up, so the phases are re-derived from the
        // speeds in effect at the instant landed on. A frame step never reaches this.
        static constexpr float SEEK_SECONDS = 1.0f;

        [[nodiscard]] bool isSeek(const float now) const {
            return !started || now < since || now - since >= SEEK_SECONDS;
        }

        // Brings every phase up to `now` under the speeds in effect until then. Call it before any
        // speed is replaced, and once per frame before the phases are read.
        void advanceTo(const float now) {
            if (!started) {
                started = true;
                since = now;
                return;
            }
            const double elapsed = static_cast<double>(now) - static_cast<double>(since);
            since = now;
            forEachAxis([elapsed](Axis &axis) { axis.phase += static_cast<double>(axis.speed) * elapsed; });
        }

        // Where an instant is landed on rather than reached: the phase is what a constant run at
        // the current speeds would have reached by then.
        void seekTo(const float now) {
            started = true;
            since = now;
            forEachAxis([now](Axis &axis) { axis.phase = static_cast<double>(axis.speed) * now; });
        }

        void setPaletteSpeeds(const ShdPaletteAttribute &attribute) {
            palette.speed = attribute.animationSpeed;
            flow.speed = attribute.animationFlowSpeed;
        }

        void setStripeSpeed(const ShdStripeAttribute &attribute) {
            stripe.speed = attribute.animationSpeed;
        }

        void setTextureSpeed(const uint32_t layer, const ShdTextureAttribute &attribute) {
            texture[layer].u.speed = attribute.scrollU;
            texture[layer].v.speed = attribute.scrollV;
        }

        void setPatternSpeed(const uint32_t layer, const ShdPatternAttribute &attribute) {
            pattern[layer].u.speed = attribute.scrollU;
            pattern[layer].v.speed = attribute.scrollV;
        }

        void setWarpSpeed(const ShdWarpAttribute &attribute) {
            warp.u.speed = attribute.scrollU;
            warp.v.speed = attribute.scrollV;
        }

        // Puts every phase, and the clock itself, into the shared time uniform. One pipeline per
        // window context does this once a frame: the descriptor is shared, so the stripe pass reads
        // what the pass that owns these wrote.
        void writeTimeUniform(const vkh::UniformImpl &timeUBO, const uint32_t frameIndex, const float now) const {
            using namespace SharedDescriptorTemplate;
            auto &host = timeUBO.getHostObject();
            host.set<float>(DescTime::TARGET_TIME_CURRENT, now);
            host.set<float>(DescTime::TARGET_TIME_PALETTE_PHASE, static_cast<float>(palette.phase));
            host.set<float>(DescTime::TARGET_TIME_FLOW_PHASE, static_cast<float>(flow.phase));
            host.set<float>(DescTime::TARGET_TIME_STRIPE_PHASE, static_cast<float>(stripe.phase));
            for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
                host.set<float>(DescTime::TARGET_TIME_TEXTURE_SCROLL + layer * 2,
                                static_cast<float>(texture[layer].u.phase));
                host.set<float>(DescTime::TARGET_TIME_TEXTURE_SCROLL + layer * 2 + 1,
                                static_cast<float>(texture[layer].v.phase));
            }
            for (uint32_t layer = 0; layer < PATTERN_LAYER_COUNT; ++layer) {
                host.set<float>(DescTime::TARGET_TIME_PATTERN_SCROLL + layer * 2,
                                static_cast<float>(pattern[layer].u.phase));
                host.set<float>(DescTime::TARGET_TIME_PATTERN_SCROLL + layer * 2 + 1,
                                static_cast<float>(pattern[layer].v.phase));
            }
            host.set<float>(DescTime::TARGET_TIME_WARP_SCROLL, static_cast<float>(warp.u.phase));
            host.set<float>(DescTime::TARGET_TIME_WARP_SCROLL + 1, static_cast<float>(warp.v.phase));
            timeUBO.updateMF(frameIndex);
        }

    private:
        bool started = false;
        float since = 0.0f;

        template<typename F> requires std::is_invocable_r_v<void, F, Axis &>
        void forEachAxis(F &&action) {
            action(palette);
            action(flow);
            action(stripe);
            for (auto &[u, v]: texture) {
                action(u);
                action(v);
            }
            for (auto &[u, v]: pattern) {
                action(u);
                action(v);
            }
            action(warp.u);
            action(warp.v);
        }
    };
}
