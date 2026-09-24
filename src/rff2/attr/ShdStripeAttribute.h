//
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include "ShdStripeType.h"

namespace merutilm::rff2 {
    struct ShdStripeAttribute {
        ShdStripeType stripeType;
        float firstInterval;
        float secondInterval;
        float opacity;
        float offset;
        float animationSpeed;
    };
}
