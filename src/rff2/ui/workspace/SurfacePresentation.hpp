//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-19, 2026-09-23, 2026-09-24
//

#pragma once
#include "SurfaceParameterRegistry.hpp"

namespace merutilm::rff2::workspace {
    inline std::wstring_view displayLabel(const SurfaceParameter &parameter) {
        struct Label {
            std::string_view id;
            std::wstring_view text;
        };

        static constexpr Label labels[] = {
            {"surface.seaGlow", L"Emission"},
            {"surface.styleDetailLight", L"Detail Brightness"},
            {"surface.styleDetailSuppress", L"Dense Detail Suppression"},
            {"surface.styleDetailThreshold", L"Density Threshold"},
            {"surface.seaThreshold", L"Emission Threshold"},
            {"surface.seaBody", L"Body Light"},
            {"surface.seaParticles", L"Particle Amount"},
            {"surface.seaParticleSize", L"Particle Size"},
            {"surface.seaBalance", L"Emission Color Balance"},
            {"surface.styleRimStrength", L"Detail Rim Light"},
            {"surface.styleRimWidth", L"Rim Width"},
            {"surface.filmStrength", L"Iridescence"},
            {"surface.filmHue", L"Film Hue"},
            {"surface.prismWidth", L"Prism Width"},
            {"surface.prismSpread", L"Prism Spread"},
            {"surface.sparkleStrength", L"Glint Strength"},
            {"surface.styleGlintSize", L"Glint Size"},
            {"surface.ukiyoColors", L"Color Count"},
            {"surface.ukiyoFlatness", L"Flatness"},
            {"surface.ukiyoFoam", L"Wave Foam"},
            {"surface.ukiyoGrain", L"Woodcut Grain"},
            {"surface.vhsNoise", L"VHS Noise"},
            {"surface.filmGrain", L"Film Grain"},
            {"surface.pixelMix", L"Nearest Mix"},
            {"surface.pixelSize", L"Pixel Size"},
            {"surface.chromaticShift", L"Chromatic Shift"},
            {"surface.paletteColorMix", L"Palette Amount"}
        };

        for (const auto &label : labels) {
            if (label.id == parameter.id) {
                return label.text;
            }
        }
        return parameter.label;
    }
}
