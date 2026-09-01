//
// Created by Opus 5 on 2026-08-13
//

#pragma once

namespace merutilm::rff2 {
    // Which layer of the texture stack the settings window is editing. UI state only: the stack
    // itself is an array, and every layer renders whether or not it is the one on screen.
    enum class ShdTextureLayerSelection {
        LAYER_1 = 0,
        LAYER_2 = 1,
        LAYER_3 = 2,
        LAYER_4 = 3
    };
}
