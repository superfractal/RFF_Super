//
// Modified by GPT-6 on 2026-09-14, 2026-09-21
//

#pragma once

namespace merutilm::rff2 {
    struct PreviewAnimationClock {
        bool paused = false;
        double offset = 0.0;
        double held = 0.0;
        float now(float wallTime) const {
            return float(paused ? held : double(wallTime) - offset);
        }
        void setPaused(bool pauseRequested, float wallTime) {
            if (pauseRequested == paused) {
                return;
            }
            if (pauseRequested) {
                held = now(wallTime);
            } else {
                offset = double(wallTime) - held;
            }
            paused = pauseRequested;
        }
    };
} // namespace merutilm::rff2
