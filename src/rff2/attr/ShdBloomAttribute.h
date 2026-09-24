//
// Created by Merutilm on 2025-05-04.
// Modified by Opus 5 on 2026-08-24
// Modified by GPT-6 on 2026-09-23
//

#pragma once

namespace merutilm::rff2 {
    struct ShdBloomAttribute {
        float threshold;
        float radius;
        float softness;
        float intensity;

        // Adds the glow in proportion to light rather than on the encoded values, as the slope pass does. Off is every earlier version's sum.
        bool linearAdd = false;
    };
}
