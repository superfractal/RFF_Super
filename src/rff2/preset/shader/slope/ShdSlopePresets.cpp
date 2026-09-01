//
// Created by Merutilm on 2025-05-28.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-08
//
#include "ShdSlopePresets.h"



namespace merutilm::rff2 {

    std::string ShdSlopePresets::Disabled::getName() const {
        return "Disabled";
    }

    ShdSlopeAttribute ShdSlopePresets::Disabled::genSlope() const {
        return ShdSlopeAttribute{0, 0, 1.0f, 60, 135, 0.0f, 32.0f, 0.0f, 3.0f, 1.0f, 1.0f};
    }

    std::string ShdSlopePresets::Normal1::getName() const {
        return "Normal 1";
    }

    ShdSlopeAttribute ShdSlopePresets::Normal1::genSlope() const {
        // Inherit every field from Normal2 so all unspecified items match the normal
        // settings, then override only the intended differences.
        ShdSlopeAttribute attr = Normal2().genSlope();
        attr.specularIntensity = 0.0f; // Specular disabled
        attr.aoIntensity = 0.5f;       // AO Intensity 0.5
        attr.ambientIntensity = 0.0f;  // Keep Normal 1's flat ambient (don't inherit Normal 2's)
        attr.brightness = 1.65f;       // Tone mapping (slope) brightness
        attr.gamma = 2.0f;             // Tone mapping (slope) gamma
        return attr;
    }

    std::string ShdSlopePresets::Normal2::getName() const {
        return "Normal 2";
    }

    ShdSlopeAttribute ShdSlopePresets::Normal2::genSlope() const {
        // depth is in human-friendly units now (100 == reference relief); the shader folds in
        // the 1e5 gain that previously forced depth to be ~1e7. See DEPTH_GAIN in vk_slope.frag.
        ShdSlopeAttribute attr = ShdSlopeAttribute{1000.0f, 0.5f, 1.0f, 60, 135, 0.15f, 100.0f, 0.0f, 3.0f, 2.25f, 3.0f};
        attr.aoIntensity = 0.5f;        // AO Intensity 0.5
        attr.ambientIntensity = 0.75f;  // Hemisphere ambient 0.75
        return attr;
    }
}
