//
// Created by Merutilm on 2025-05-04.
// Modified by GPT-6 on 2026-09-08, 2026-09-23
// Modified by Opus 5.5 on 2026-10-10, 2026-11-05
//

#pragma once
#include <cstdint>

#include "FrtPanoramaLayout.h"
#include "FrtProjectionMethod.h"

namespace merutilm::rff2 {
    // The 360 projection a keyframe folder was calculated in, read from its own .rfc.
    struct VidKeyframeProjection {
        FrtProjectionMethod projection = FrtProjectionMethod::PLANAR;
        float yaw = 0.0f; // Radians.
        float pitch = 0.0f; // Radians.
        float halfFovTangent = 1.0f;
        float limit = 1.0f; // Plane radius, in horizon radii, the projection folds at.
        FrtPanoramaLayout layout = FrtPanoramaLayout::GROUND;
    };

    struct VidDataAttribute {
        float defaultZoomIncrement;
        bool isStatic;

        bool cameraPadding = false;
        uint32_t cameraScale = 4;
        uint32_t sourceScale = 1; // Runtime coverage from the keyframe folder; never serialized into config files.
        uint32_t keyframeFirst = 0; // Runtime keyframe file range; 0 picks the lowest file, never serialized.
        uint32_t keyframeLast = 0; // Runtime keyframe file range; 0 runs to the first gap, never serialized.
        VidKeyframeProjection keyframeProjection; // Runtime, from the keyframe folder; never serialized.
    };
}
