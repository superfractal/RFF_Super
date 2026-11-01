//
// Created by Opus 5.5 on 2026-10-08
//

#pragma once

#include <cstdint>

namespace merutilm::rff2 {
    // What the Studio environment reflection shows. Values are the shader's source numbers; a texture layer is read as an equirectangular panorama.
    enum class ShdStudioEnvironmentSource {
        TWO_COLOR = 0,
        GRADIENT = 1,
        TEXTURE_LAYER_1 = 2,
        TEXTURE_LAYER_2 = 3,
        TEXTURE_LAYER_3 = 4,
        TEXTURE_LAYER_4 = 5
    };

    // Bit of the texture layer the source reads, so that layer's image stays loaded while the layer itself paints nothing.
    constexpr uint32_t environmentSourceLayerMask(const ShdStudioEnvironmentSource source) {
        const auto first = static_cast<uint32_t>(ShdStudioEnvironmentSource::TEXTURE_LAYER_1);
        const auto value = static_cast<uint32_t>(source);
        return value >= first ? 1u << (value - first) : 0u;
    }
}
