//
// Created by Merutilm on 2025-05-27.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-20, 2026-08-31
//

#pragma once

#include "../../Presets.h"
#include "../../../attr/ShdPaletteAttribute.h"


namespace merutilm::rff2::ShdPalettePresets {


    struct LongRandom64 final : public Presets::ShaderPresets::PalettePreset {
        std::vector<glm::vec4> baseColors;

        LongRandom64() = default;
        explicit LongRandom64(std::vector<glm::vec4> colors) : baseColors(std::move(colors)) {}

        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;

        // Recipe-eligible only when the base colors are random (empty); a caller-supplied base
        // cannot be reproduced from a seed, so those must fall back to raw storage.
        [[nodiscard]] int32_t getPaletteRecipeId() const override { return baseColors.empty() ? 1 : -1; }
    };

    struct LongRandom64_2 final : public Presets::ShaderPresets::PalettePreset {
        std::vector<glm::vec4> baseColors;

        LongRandom64_2() = default;
        explicit LongRandom64_2(std::vector<glm::vec4> colors) : baseColors(std::move(colors)) {}

        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;

        [[nodiscard]] int32_t getPaletteRecipeId() const override { return baseColors.empty() ? 2 : -1; }
    };

    struct RandomSmooth final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct Classic1 final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct Classic2 final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct ArcticAurora final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct Azure final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct Cinematic final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct CrimsonMagma final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct DeepSpace final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct Desert final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct ElectricDreams final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct Flame final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyBerry final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyCyber final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyFire final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyForest final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyIce final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyMetal final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyNeon final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyOcean final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossyPastel final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct GlossySunset final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct LongRainbow7 final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;

        [[nodiscard]] int32_t getPaletteRecipeId() const override { return 3; }
    };

    struct MidnightNeon final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct MistyForest final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct PastelDream final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct Rainbow final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    struct VolcanicAsh final : public Presets::ShaderPresets::PalettePreset {
        [[nodiscard]] std::string getName() const override;

        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    // Not registered in the Settings menu; used programmatically (e.g. random startup palette).
    struct FromColors final : public Presets::ShaderPresets::PalettePreset {
        std::vector<glm::vec4> colors;
        explicit FromColors(std::vector<glm::vec4> c) : colors(std::move(c)) {}

        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] ShdPaletteAttribute genPalette() const override;
    };

    // Regenerates the color array for a recipe-eligible preset id (see getPaletteRecipeId) using
    // the saved seed. Returns empty for an unknown id. Reproducible within the same binary.
    std::vector<glm::vec4> regenerateRecipeColors(int32_t recipeId, uint32_t seed);
}
