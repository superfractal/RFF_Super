//
// Created by Merutilm on 2025-05-31.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#pragma once
#include "../Presets.h"

namespace merutilm::rff2::ResolutionPresets {
    struct L1 final : public Presets::ResolutionPreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] std::array<int, 2> genResolution() const override;
    };

    struct L2 final : public Presets::ResolutionPreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] std::array<int, 2> genResolution() const override;
    };

    struct L3 final : public Presets::ResolutionPreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] std::array<int, 2> genResolution() const override;
    };

    struct L4 final : public Presets::ResolutionPreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] std::array<int, 2> genResolution() const override;
    };

    struct L5 final : public Presets::ResolutionPreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] std::array<int, 2> genResolution() const override;
    };

    struct L6 final : public Presets::ResolutionPreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] std::array<int, 2> genResolution() const override;
    };
}
