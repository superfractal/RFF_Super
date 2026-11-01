//
// Created by Merutilm on 2025-05-04.
// Modified by GPT-6 on 2026-09-08, 2026-09-23
// Modified by Opus 5.5 on 2026-10-10
//

#pragma once
#include <cstdint>

namespace merutilm::rff2 {
    struct VidDataAttribute {
        float defaultZoomIncrement;
        bool isStatic;

        bool cameraPadding = false;
        uint32_t cameraScale = 4;
        uint32_t sourceScale = 1; // Runtime coverage from the keyframe folder; never serialized into config files.
        uint32_t keyframeFirst = 0; // Runtime keyframe file range; 0 picks the lowest file, never serialized.
        uint32_t keyframeLast = 0; // Runtime keyframe file range; 0 runs to the first gap, never serialized.
    };
}
