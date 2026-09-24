//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-16, 2026-09-19, 2026-09-20, 2026-09-23, 2026-09-24
//

#pragma once
#include "SurfaceParameterRegistry.hpp"
#include "SurfaceColorRegistry.hpp"

namespace merutilm::rff2::workspace {
    inline int changedSurfaceControls(const ShdSlopeAttribute &slope, Category category) {
        const auto defaults = surfaceDefaults();
        int count = 0;
        for (const auto &parameter : surfaceParameters()) {
            if (!paletteLineControl(parameter.id) && parameter.category() == category &&
                parameter.value(slope) != parameter.value(defaults)) {
                ++count;
            }
        }
        for (const auto &color : surfaceColors()) {
            if (!paletteLineControl(color.id) && color.category == category &&
                slope.*color.member != defaults.*color.member) {
                ++count;
            }
        }
        return count;
    }

    struct SurfaceEffectState {
        bool base = false;
        bool added = false;
        bool studio = false;
        std::wstring_view explanation;
        std::wstring_view label() const {
            if (!explanation.empty()) return explanation;
            if (!studio) return L"Studio off";
            if (base && added) return L"Base + added";
            if (base) return L"Base";
            if (added) return L"Added";
            return L"Available";
        }
    };

    inline SurfaceEffectState effectState(const ShdSlopeAttribute &slope, Category category) {
        const int style = static_cast<int>(slope.surfaceStyle);
        SurfaceEffectState result{false, false, slope.studio.use};
        switch (category) {
            case Category::COLOR:
                result.base = style != 0;
                result.added = slope.layerCommon > 0;
                break;
            case Category::REFLECTION:
                result.base = slope.studio.use;
                result.added = slope.layerMetal > 0;
                break;
            case Category::FILM:
                result.base = style == 1;
                result.added = slope.layerMetal > 0;
                break;
            case Category::CONTOUR:
                result.base = style == 2;
                result.added = slope.layerSigil > 0;
                break;
            case Category::EMISSION:
                result.base = style == 5;
                result.added = slope.layerSea > 0;
                break;
            case Category::FLAME:
                result.base = style == 3 || style == 4;
                result.added = slope.layerPhonk > 0 || slope.layerFrost > 0;
                break;
            case Category::PRINT:
                result.base = style == 6 && slope.studio.use;
                result.added = slope.layerPrint > 0 || slope.layerQuantize > 0;
                result.studio = slope.studio.use || slope.layerQuantize > 0;
                break;
            case Category::NOISE:
                result.added = slope.layerVhs > 0 || slope.layerMono > 0;
                result.studio = true;
                break;
            case Category::RELIEF:
                result.base = slope.depth > 0 && slope.opacity > 0;
                result.studio = result.base;
                result.explanation = slope.depth <= 0 ? L"Depth is zero" :
                                     slope.opacity <= 0 ? L"Opacity is zero" :
                                     slope.studio.use ? L"Studio lighting" : L"Legacy lighting";
                break;
            case Category::MIX:
                result.added = slope.layerCommon > 0 || slope.layerMetal > 0 || slope.layerSigil > 0 ||
                               slope.layerPhonk > 0 || slope.layerFrost > 0 || slope.layerSea > 0 ||
                               slope.layerPrint > 0;
                break;
        }
        return result;
    }
}
