//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18, 2026-08-31
// Modified by Opus 5 on 2026-08-31
//

#pragma once
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

#include "VidKeyInterpolation.h"

namespace merutilm::rff2 {
    // One key of an animated parameter. The axis is depth - the keyframe number, which the zoom
    // ratio follows one to one - and not time, so a key stays on the view it was placed on however
    // the speed around it is later changed.
    struct VidTimelineKey {
        float depth;
        float value;
        // Used by color tracks only; a scalar track leaves it alone.
        glm::vec4 color;
        // How this key reaches the next one, the next one being the key below it in depth.
        VidKeyInterpolation out;
    };

    struct VidTimelineTrack {
        // VidTimelineTarget, kept as its raw number: a track a later version writes still reads
        // back and saves out unchanged on a build that does not know what it drives.
        uint16_t targetId;
        bool enabled;
        // Held in descending depth, which is playback order.
        std::vector<VidTimelineKey> keys;
    };

    // A pause: the zoom stands still at this depth for this many seconds. Speed is never allowed to
    // reach zero - the time integral would not converge and the export would never end - so a hold
    // is how a standstill is written.
    struct VidTimelineHold {
        float depth;
        float seconds;
    };

    struct VidTimelineAttribute {
        // Keys per track, and holds, the editor offers. The format itself has no such limit.
        static constexpr uint32_t MAX_KEYS_PER_TRACK = 1024;
        // Speed may come arbitrarily close to a standstill but never reach one.
        static constexpr float MIN_SPEED = 1e-4f;

        // False leaves the export exactly as it is without a timeline: one constant Zoom Speed.
        bool enabled = true;
        std::vector<VidTimelineTrack> tracks;
        std::vector<VidTimelineHold> holds;
        // Keyframe count the editor's length readout assumes - the top of its depth axis. An export
        // always counts the folder it is given; this exists only so the panel can show a length
        // before a folder is chosen. Held as a float because the axis it stands for is one.
        float estimateKeyframes = 100.0f;
    };
}
