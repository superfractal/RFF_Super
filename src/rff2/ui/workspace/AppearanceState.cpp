//
// Modified by GPT-6 on 2026-09-14, 2026-09-21
//

#include "AppearanceState.hpp"
#include "../../io/ConfigIO.h"
#include "../../io/ShaderPresetIO.h"
#include "../../preset/shader/palette/ShdPalettePresets.h"
#include "../../preset/shader/slope/ShdSlopePresets.h"
#include "../../preset/shader/stripe/ShdStripePresets.h"
#include "../../preset/shader/color/ShdColorPresets.h"
#include "../../preset/shader/fog/ShdFogPresets.h"
#include "../../preset/shader/bloom/ShdBloomPresets.h"
#include <sstream>

namespace merutilm::rff2::workspace {
    ShaderAttribute AppearanceState::defaults() {
        ShaderAttribute result{};
        result.palette = ShdPalettePresets::Classic1().genPalette();
        result.slope = ShdSlopePresets::Disabled().genSlope();
        result.stripe = ShdStripePresets::Disabled().genStripe();
        result.color = ShdColorPresets::Disabled().genColor();
        result.fog = ShdFogPresets::Disabled().genFog();
        result.bloom = BloomPresets::Disabled().genBloom();
        return result;
    }
    void AppearanceState::keepMotion(const ShaderAttribute &from, ShaderAttribute &to) {
        to.camera = from.camera;
        const auto &palette = from.palette;
        auto &target = to.palette;
        target.animationSpeed = palette.animationSpeed;
        target.animationMode = palette.animationMode;
        target.animationFlowAmount = palette.animationFlowAmount;
        target.animationFlowScale = palette.animationFlowScale;
        target.animationFlowSpeed = palette.animationFlowSpeed;
        target.animationFlowSwirl = palette.animationFlowSwirl;
        target.staticColorIterations = palette.staticColorIterations;
        target.staticColorTolerance = palette.staticColorTolerance;
    }
    std::string AppearanceState::key(const ShaderAttribute &shader) {
        Attribute sample{.fractal = {.center = fp_complex("0", "0", -30)}};
        sample.shader = shader;
        keepMotion(ShaderAttribute{}, sample.shader);
        std::ostringstream output(std::ios::out | std::ios::binary);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        ConfigIO::write(output, sample, 0, 0);
        // Include realized recipe colors as well as their serialized recipe identity.
        for (const auto &color : sample.shader.palette.colors) {
            for (int i = 0; i < 4; ++i) {
                output.write(reinterpret_cast<const char *>(&color[i]), sizeof(float));
            }
        }
        return std::move(output).str();
    }
} // namespace merutilm::rff2::workspace
