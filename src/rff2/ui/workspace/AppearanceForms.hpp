//
// Modified by GPT-6 on 2026-09-14, 2026-09-23, 2026-09-25
// Modified by Opus 5.5 on 2026-10-07, 2026-10-09, 2026-10-12
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
    // pickFocus arms a canvas pick for Focus Depth and runs its argument once the click has landed; empty leaves the action out.
    WorkspaceForm finishingForm(AttributeGetter attribute, std::function<void()> changed,
                                std::function<void(std::function<void()>)> pickFocus = {});
    WorkspaceForm materialEffectsForm(AttributeGetter attribute, std::function<void()> changed, LayerMove onMove = {});
    WorkspaceForm paletteForm(AttributeGetter attribute, std::function<void()> changed);
    // Settings that belong to no other panel, installed as workspace OTHER_WORKSPACE.
    WorkspaceForm otherForm(AttributeGetter attribute, std::function<void()> changed);
    // Orbit Trap coloring, installed as workspace ORBIT_TRAP_WORKSPACE.
    WorkspaceForm orbitTrapForm(AttributeGetter attribute, std::function<void()> changed);
    WorkspaceForm previewPerformanceForm(AttributeGetter attribute, std::function<void()> changed);
}
