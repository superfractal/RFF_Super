//
// Created by Opus 5.5 on 2026-10-10.
//

#pragma once

namespace merutilm::rff2 {
    // How the finished video frames are resized to the chosen output size; the picture itself is rendered at the keyframe size.
    enum class VidUpscaleFilter {
        // These numbers are written to the configuration stream.
        LANCZOS = 0,
        BICUBIC = 1,
        NEAREST = 2
    };
}
