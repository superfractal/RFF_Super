//
// Modified by GPT-6 on 2026-09-30
//

#pragma once
#include "AttributeFormModel.hpp"
#include <algorithm>
#include <cmath>

namespace merutilm::rff2::workspace {
    struct TimelineHoldForm {
        static constexpr int group = 6;
        static int add(VidTimelineAttribute &timeline, float depth) {
            if (!std::isfinite(depth)) return -1;
            auto &holds = timeline.holds;
            const auto found = std::ranges::find(holds, depth, &VidTimelineHold::depth);
            if (found != holds.end()) return int(found - holds.begin());
            if (holds.size() >= 65536) return -1;
            holds.push_back({depth, 2.f});
            return int(holds.size()) - 1;
        }
        static bool remove(VidTimelineAttribute &timeline, int index) {
            if (index < 0 || index >= int(timeline.holds.size())) return false;
            timeline.holds.erase(timeline.holds.begin() + index);
            return true;
        }
        static void fields(AttributeFormModel &model, int index, float minimumDepth, float maximumDepth) {
            model.numeric("hold.depth", group, L"Hold Keyframe",
                L"Keyframe position where zoom pauses. Playback travels from higher to lower keyframe numbers.",
                [index](auto &a) -> auto & { return a.video.timeline.holds.at(size_t(index)).depth; },
                minimumDepth, maximumDepth);
            model.numeric("hold.seconds", group, L"Hold Duration (s)",
                L"Pause zoom for this many seconds. 0 disables this hold. Color animation, constant rotation and audio continue.",
                [index](auto &a) -> auto & { return a.video.timeline.holds.at(size_t(index)).seconds; },
                0.f, 604800.f);
        }
    };
}
