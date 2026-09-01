//
// Created by Opus 5 on 2026-08-17.
//

#pragma once

namespace merutilm::rff2 {
    // How the lit / shadow tint meets the shaded color. Values are the shader's mode numbers.
    // MULTIPLY is what earlier versions did, and stays the default so a file written by one of
    // those looks as it did.
    enum class ShdSlopeTintBlend {
        MULTIPLY = 0,
        OKLAB = 1
    };
}
