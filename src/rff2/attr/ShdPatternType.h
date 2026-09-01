//
// Created by Opus 5 on 2026-08-07
//

#pragma once

namespace merutilm::rff2 {
    // Shape drawn by the procedural pattern layer. Every one is periodic over a unit UV cell, so it
    // stays seamless where the cycle wraps.
    enum class ShdPatternType {
        STRIPES = 0,
        CHECKER = 1,
        GRID = 2,
        DOTS = 3,
        DIAMOND = 4,
        HONEYCOMB = 5,
        WAVES = 6,
        CLOUD = 7
    };
}
