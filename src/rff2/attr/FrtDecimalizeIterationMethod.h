//
// Created by Merutilm on 2025-05-04.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

namespace merutilm::rff2 {
    enum class FrtDecimalizeIterationMethod {
        /**
         * Do Not Use Decimal Iterations.
         */
        NONE = 0,
        /**
         * Use triangle inequality once.
         */
        LINEAR = 1,
        /**
         * Calculates <b>Sqrt(Linear)</b>.
         */
        SQUARE_ROOT = 2,
        /**
         * Calculates <b>Log(Linear + 1)</b>.
         */
        LOG = 3,
        /**
         * Calculates <b>Log(Log(Linear + 1) + 1)</b>.
         */
        LOG_LOG = 4
    };
}
