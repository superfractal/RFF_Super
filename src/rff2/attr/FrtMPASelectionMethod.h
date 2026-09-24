//
// Created by Merutilm on 2025-05-04.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

namespace merutilm::rff2 {
    enum class FrtMPASelectionMethod {
        /**
        * Checks the lowest-level MPA first, increases the level until not valid.
        */
        LOWEST = 0,
        /**
         * Checks the highest-level MPA first. Decreases the level if not valid.
         */
        HIGHEST = 1
    };
}
