//
// Created by Merutilm on 2025-05-04.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

namespace merutilm::rff2 {
    enum class FrtReuseReferenceMethod {
        /**
        * Reuse current reference
        */
        CURRENT_REFERENCE = 0,
        /**
         * Get the centered reference using its period and reuse this.
         */
        CENTERED_REFERENCE = 1,
        /**
         * Do not reuse reference and recalculate reference every perturbator.
         */
        DISABLED = 2
    };
}
