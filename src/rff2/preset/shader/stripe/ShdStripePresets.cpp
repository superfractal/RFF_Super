//
// Created by Merutilm on 2025-05-28.
// Modified by GPT-6 on 2026-09-23
//

#include "ShdStripePresets.h"

namespace merutilm::rff2 {

    std::string ShdStripePresets::Disabled::getName() const {
        return "Disabled";
    }

    ShdStripeAttribute ShdStripePresets::Disabled::genStripe() const {
        return ShdStripeAttribute{
            .stripeType = ShdStripeType::NONE,
            .firstInterval = 10,
            .secondInterval = 50,
            .opacity = 1,
            .offset = 0,
            .animationSpeed = 0
        };
    }

    std::string ShdStripePresets::SlowAnimated::getName() const {
        return "Slow Animated";
    }

    ShdStripeAttribute ShdStripePresets::SlowAnimated::genStripe() const {
        return ShdStripeAttribute{
            .stripeType = ShdStripeType::SINGLE_DIRECTION,
            .firstInterval = 10,
            .secondInterval = 50,
            .opacity = 1,
            .offset = 0,
            .animationSpeed = 0.5f
        };
    }

    std::string ShdStripePresets::FastAnimated::getName() const {
        return "Fast Animated";
    }

    ShdStripeAttribute ShdStripePresets::FastAnimated::genStripe() const {
        return ShdStripeAttribute{
            .stripeType = ShdStripeType::SINGLE_DIRECTION,
            .firstInterval = 100,
            .secondInterval = 500,
            .opacity = 1,
            .offset = 0,
            .animationSpeed = 5
        };
    }

    std::string ShdStripePresets::Smooth::getName() const {
        return "Smooth";
    }

    ShdStripeAttribute ShdStripePresets::Smooth::genStripe() const {
        return ShdStripeAttribute{
            .stripeType = ShdStripeType::SMOOTH,
            .firstInterval = 1,
            .secondInterval = 1,
            .opacity = 1,
            .offset = 0,
            .animationSpeed = 0.25f
        };
    }

    std::string ShdStripePresets::SmoothTranslucent::getName() const {
        return "Smooth Translucent";
    }

    ShdStripeAttribute ShdStripePresets::SmoothTranslucent::genStripe() const {
        return ShdStripeAttribute{
            .stripeType = ShdStripeType::SQUARED,
            .firstInterval = 1,
            .secondInterval = 1,
            .opacity = 0.5f,
            .offset = 0,
            .animationSpeed = 1
        };
    }
}
