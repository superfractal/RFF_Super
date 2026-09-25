// Modified by GPT-6 on 2026-09-25
#pragma once
#include "../attr/VidAudioAttribute.h"

namespace merutilm::rff2 {
    struct AudioClipEdit {
        static bool apply(VidAudioAttribute &audio, const VidAudioClip &original, int edge, int64_t delta) {
            if (delta < -VidAudioAttribute::maximumTime || delta > VidAudioAttribute::maximumTime) return false;
            auto candidate = audio;
            auto found = std::ranges::find(candidate.clips, original.id, &VidAudioClip::id);
            if (found == candidate.clips.end()) return false;
            *found = original;
            if (edge < 0) {
                found->start += delta;
                found->in += delta;
            } else if (edge > 0) {
                found->out += delta;
            } else {
                found->start += delta;
            }
            if (found->in < 0 || found->out <= found->in) return false;
            const auto duration = found->duration();
            found->fadeIn = std::min(found->fadeIn, duration);
            found->fadeOut = std::min(found->fadeOut, duration - found->fadeIn);
            if (!candidate.valid()) return false;
            audio = std::move(candidate);
            return true;
        }
    };
}
