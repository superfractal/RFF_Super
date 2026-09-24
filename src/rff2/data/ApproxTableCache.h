//
// Created by Merutilm on 2025-05-23.
// Modified by Sonnet 5 on 2026-07-06
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <vector>

#include "../mrthy/DeepPA.h"
#include "../mrthy/LightPA.h"

namespace merutilm::rff2 {
    struct ApproxTableCache {
        std::vector<std::vector<LightPA>> lightTable = {};
        std::vector<std::vector<DeepPA>> deepTable = {};
    };
}
