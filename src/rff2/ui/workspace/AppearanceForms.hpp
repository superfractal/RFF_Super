//
// Modified by GPT-6 on 2026-09-14, 2026-09-23, 2026-09-25
//

#pragma once
#include <functional>
#include <limits>

#include "WorkspaceForm.hpp"
#include "../../attr/Attribute.h"

namespace merutilm::rff2::workspace {
    using AttributeGetter = std::function<Attribute &()>;
    using LayerMove = std::function<void(int, int)>;
    inline constexpr float formMaximum = std::numeric_limits<float>::max();
    inline constexpr float formPositive = std::numeric_limits<float>::denorm_min();

    WorkspaceForm lightingForm(AttributeGetter attribute, std::function<void()> changed);
    WorkspaceForm textureForm(AttributeGetter attribute, std::function<void()> changed, LayerMove onMove = {});
    WorkspaceForm patternForm(AttributeGetter attribute, std::function<void()> changed, LayerMove onMove = {});
    WorkspaceForm warpStripeForm(AttributeGetter attribute, std::function<void()> changed);
    WorkspaceForm finishingForm(AttributeGetter attribute, std::function<void()> changed);
    WorkspaceForm materialEffectsForm(AttributeGetter attribute, std::function<void()> changed, LayerMove onMove = {});
    WorkspaceForm paletteForm(AttributeGetter attribute, std::function<void()> changed);
    WorkspaceForm previewPerformanceForm(AttributeGetter attribute, std::function<void()> changed);
}
