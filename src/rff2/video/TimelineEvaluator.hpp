// Modified by GPT-5 on 2026-08-18

#pragma once

#include "TimelineParams.hpp"
#include "../attr/VidTimelineAttribute.h"

namespace merutilm::rff2 {
    class TimelineEvaluator final {
        VidTimelineAttribute timeline;
        bool activeShaderTracks = false;

    public:
        explicit TimelineEvaluator(const VidTimelineAttribute &timeline);

        [[nodiscard]] bool hasActiveShaderTracks() const { return activeShaderTracks; }

        void evaluate(float depth, float sec, const ShaderAttribute &base, ShaderAttribute &out) const;

        [[nodiscard]] TimelineDirtyMask diff(const ShaderAttribute &previous,
                                             const ShaderAttribute &next) const;
    };
}
