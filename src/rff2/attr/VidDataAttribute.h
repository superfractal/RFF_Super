//
// Created by Merutilm on 2025-05-04.
// Modified by GPT-6 on 2026-09-08, 2026-09-23
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
    };
}
