//
// Created by Merutilm on 2025-05-28.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-31
//

#pragma once
#include <string>
#include <array>
#include "../attr/FrtMPAAttribute.h"
#include "../attr/FrtReferenceCompAttribute.h"
#include "../attr/RenderAttribute.h"
#include "../attr/ShdPaletteAttribute.h"
#include "../attr/ShdStripeAttribute.h"
#include "../attr/ShdSlopeAttribute.h"
#include "../attr/ShdColorAttribute.h"
#include "../attr/ShdFogAttribute.h"
#include "../attr/ShdBloomAttribute.h"
#include "../attr/ShaderAttribute.h"


namespace merutilm::rff2 {
    struct Preset {
        virtual ~Preset() = default;

        virtual std::string getName() const = 0;
    };


    namespace Presets {
        struct CalculationPreset : public Preset {
            ~CalculationPreset() override = default;

            virtual FrtMPAAttribute genMPA() const = 0;

            virtual FrtReferenceCompAttribute genReferenceCompression() const = 0;
        };

        struct RenderPreset : public Preset {
            ~RenderPreset() override = default;

            virtual RenderAttribute genRender() const = 0;
        };

        struct ResolutionPreset : public Preset {
            ~ResolutionPreset() override = default;

            virtual std::array<int, 2> genResolution() const = 0;
        };

        struct ShaderPreset : public Preset {
            ~ShaderPreset() override = default;
        };

        namespace ShaderPresets {
            // Every shader section at once, for a look that is kept as a whole file rather than
            // built out of the sections below.
            struct FullShaderPreset : public ShaderPreset {
                ~FullShaderPreset() override = default;

                virtual ShaderAttribute genShader() const = 0;
            };

            struct PalettePreset : public ShaderPreset {
                ~PalettePreset() override = default;

                virtual ShdPaletteAttribute genPalette() const = 0;

                // Stable id used to store/regenerate huge generated palettes as {id, seed} instead
                // of dumping the color array. -1 = not recipe-eligible (palette saved as raw data).
                [[nodiscard]] virtual int32_t getPaletteRecipeId() const { return -1; }
            };

            struct StripePreset : public ShaderPreset {
                ~StripePreset() override = default;

                virtual ShdStripeAttribute genStripe() const = 0;
            };

            struct SlopePreset : public ShaderPreset {
                ~SlopePreset() override = default;

                virtual ShdSlopeAttribute genSlope() const = 0;
            };

            struct ColorPreset : public ShaderPreset {
                ~ColorPreset() override = default;

                virtual ShdColorAttribute genColor() const = 0;
            };

            struct FogPreset : public ShaderPreset {
                ~FogPreset() override = default;

                virtual ShdFogAttribute genFog() const = 0;
            };

            struct BloomPreset : public ShaderPreset {
                ~BloomPreset() override = default;

                virtual ShdBloomAttribute genBloom() const = 0;
            };
        }
    }
}
