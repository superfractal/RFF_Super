//
// Modified by GPT-6 on 2026-09-19, 2026-09-21, 2026-09-23
//

#include "../attr/VidAudioAttribute.h"

namespace merutilm::rff2 {
    bool VidAudioAttribute::valid() const {
        if (!std::isfinite(gain) || gain < 0 || gain > 4 || clips.size() > maximumClips) {
            return false;
        }
        std::unordered_set<uint64_t> ids;
        std::vector<const VidAudioClip *> clipsByStart;
        for (const auto &clip : clips) {
            if (!clip.id || !ids.insert(clip.id).second || clip.path.empty() ||
                clip.path.size() > maximumPathBytes || clip.path.find('\0') != std::string::npos) {
                return false;
            }

            // Validate source endpoints before subtracting them to calculate duration.
            if (clip.sourceDuration <= 0 || clip.sourceDuration > maximumTime || clip.in < 0 ||
                clip.out <= clip.in || clip.out > clip.sourceDuration) {
                return false;
            }
            const int64_t duration = clip.duration();
            if (clip.start < 0 || clip.start > maximumTime - duration) {
                return false;
            }
            if (clip.fadeIn < 0 || clip.fadeOut < 0 || clip.fadeIn > duration ||
                clip.fadeOut > duration - clip.fadeIn) {
                return false;
            }
            if (!std::isfinite(clip.gain) || clip.gain < 0 || clip.gain > 4) {
                return false;
            }
            clipsByStart.push_back(&clip);
        }

        std::ranges::sort(clipsByStart, {}, &VidAudioClip::start);
        for (size_t i = 1; i < clipsByStart.size(); ++i) {
            if (clipsByStart[i]->start < clipsByStart[i - 1]->start + clipsByStart[i - 1]->duration()) {
                return false;
            }
        }
        return true;
    }
}
