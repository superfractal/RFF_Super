//
// Created by Merutilm on 2025-05-28.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#pragma once
#include "../../Presets.h"
#include "../../../attr/ShdSlopeAttribute.h"


namespace merutilm::rff2::ShdSlopePresets {

    struct Disabled final : public Presets::ShaderPresets::SlopePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdSlopeAttribute genSlope() const override;
    };

    struct Normal1 final : public Presets::ShaderPresets::SlopePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdSlopeAttribute genSlope() const override;
    };

    struct Normal2 final : public Presets::ShaderPresets::SlopePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdSlopeAttribute genSlope() const override;
    };
}
