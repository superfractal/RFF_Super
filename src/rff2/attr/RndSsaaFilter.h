//
// Created by Opus 5.5 on 2026-10-07.
//

#pragma once

namespace merutilm::rff2 {
    // How Supersampling's samples are combined into one output pixel.
    enum class RndSsaaFilter {
        // The plain mean of the sRGB-encoded samples, which every build before this one drew.
        BOX_GAMMA = 0,
        // A Mitchell-Netravali filter applied to the samples in linear light.
        MITCHELL_LINEAR = 1
    };
}
