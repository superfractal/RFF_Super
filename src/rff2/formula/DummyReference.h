//
// Created by AI; exact creation date unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#pragma once
#include "MandelbrotReference.h"
#include <vector>
#include "../calc/fp_complex.h"

namespace merutilm::rff2 {
    struct DummyReference : public MandelbrotReference {
        DummyReference() 
            : MandelbrotReference(
                fp_complex("0", "0", 0), 
                {}, 
                {}, 
                fp_complex("0", "0", 0), 
                fp_complex("0", "0", 0)) {}

        size_t length() const override { return 0; }
        uint64_t longestPeriod() const override { return 0; }
    };
}
