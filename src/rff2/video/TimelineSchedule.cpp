//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18, 2026-08-23
// Modified by Opus 5 on 2026-08-25, 2026-08-31
//

#include "TimelineSchedule.hpp"

#include <algorithm>
#include <cmath>

namespace merutilm::rff2 {

    namespace {
        float clampSpeed(const float speed) {
            return std::isfinite(speed)
                       ? std::max(speed, VidTimelineAttribute::MIN_SPEED)
                       : VidTimelineAttribute::MIN_SPEED;
        }

        // Catmull-Rom through the two keys around the sample, with the keys outside them as the
        // tangents. It overshoots by nature, so the caller holds the result inside the key range.
        float catmullRom(const float p0, const float p1, const float p2, const float p3, const float u) {
            const float u2 = u * u;
            const float u3 = u2 * u;
            return 0.5f * (2.0f * p1 + (-p0 + p2) * u + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * u2 +
                           (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * u3);
        }
    }

    const VidTimelineTrack *TimelineSchedule::findTrack(const VidTimelineAttribute &timeline, const uint16_t targetId) {
        for (const auto &track: timeline.tracks) {
            if (track.targetId == targetId) {
                return &track;
            }
        }
        return nullptr;
    }

    float TimelineSchedule::evaluateTrack(const VidTimelineTrack &track, const float depth, const float fallback) {
        const auto &keys = track.keys;
        if (keys.empty()) {
            return fallback;
        }
        float lo = keys.front().value;
        float hi = lo;
        for (const auto &key: keys) {
            lo = std::min(lo, key.value);
            hi = std::max(hi, key.value);
        }
        return evaluateTrack(track, depth, fallback, lo, hi);
    }

    float TimelineSchedule::evaluateTrack(const VidTimelineTrack &track, const float depth, const float fallback,
                                          const float minValue, const float maxValue) {
        const auto &keys = track.keys;
        if (keys.empty()) {
            return std::clamp(fallback, minValue, maxValue);
        }
        if (keys.size() == 1) {
            return std::clamp(keys.front().value, minValue, maxValue);
        }
        // Outside the outermost keys the value is held, never extrapolated.
        if (depth >= keys.front().depth) {
            return std::clamp(keys.front().value, minValue, maxValue);
        }
        if (depth <= keys.back().depth) {
            return std::clamp(keys.back().value, minValue, maxValue);
        }

        // A segment runs from its own key down to, but not including, the next key's depth, so a
        // sample landing exactly on a key belongs to the segment that key starts. Including the
        // lower end here made STEP hand back the key before it for one more sample.
        // Keys run in descending depth, so the ones at or above this depth are a prefix of them:
        // the segment wanted is the last of that prefix, found without walking every key.
        const size_t below = static_cast<size_t>(std::ranges::distance(
            keys.begin(), std::ranges::partition_point(keys, [depth](const VidTimelineKey &key) {
                return key.depth >= depth;
            })));
        const size_t i = below == 0 ? 0 : std::min(below - 1, keys.size() - 2);
        const float d0 = keys[i].depth;
        const float d1 = keys[i + 1].depth;
        const float v0 = keys[i].value;
        const float v1 = keys[i + 1].value;
        const float span = d0 - d1;
        const float u = span > 0.0f ? std::clamp((d0 - depth) / span, 0.0f, 1.0f) : 0.0f;

        switch (keys[i].out) {
            case VidKeyInterpolation::LINEAR:
                return std::clamp(v0 + (v1 - v0) * u, minValue, maxValue);
            case VidKeyInterpolation::SMOOTH: {
                // Zero slope at both keys, so a curve built of these has no corner at a key.
                const float s = u * u * (3.0f - 2.0f * u);
                return std::clamp(v0 + (v1 - v0) * s, minValue, maxValue);
            }
            case VidKeyInterpolation::CUBIC: {
                const float pm = i > 0 ? keys[i - 1].value : v0;
                const float pp = i + 2 < keys.size() ? keys[i + 2].value : v1;
                return std::clamp(catmullRom(pm, v0, v1, pp, u), minValue, maxValue);
            }
            case VidKeyInterpolation::STEP:
            default:
                return std::clamp(v0, minValue, maxValue);
        }
    }

    TimelineSchedule TimelineSchedule::create(const VidTimelineAttribute &timeline, const float startDepth,
                                              const float endDepth, const float fallbackSpeed) {
        TimelineSchedule s;
        s.startDepth = startDepth;
        s.endDepth = std::min(endDepth, startDepth);
        s.uniformSpeed = clampSpeed(fallbackSpeed);

        const VidTimelineTrack *speed = timeline.enabled
                                            ? findTrack(timeline, vidTimelineTargetId(VidTimelineTarget::SPEED))
                                            : nullptr;
        const bool hasKeys = speed != nullptr && speed->enabled && !speed->keys.empty();
        bool hasHolds = false;
        if (timeline.enabled) {
            for (const auto &[depth, seconds]: timeline.holds) {
                if (seconds > 0.0f && depth <= s.startDepth && depth >= s.endDepth) {
                    hasHolds = true;
                    break;
                }
            }
        }

        const float span = s.startDepth - s.endDepth;
        if ((!hasKeys && !hasHolds) || span <= 0.0f) {
            // Nothing to integrate: one constant speed, which is what the export always did.
            s.uniform = true;
            s.totalSeconds = span > 0.0f ? span / s.uniformSpeed : 0.0f;
            return s;
        }

        s.uniform = false;
        if (hasKeys) {
            s.speedTrack = *speed;
            s.hasSpeedTrack = true;
        }

        const double requested = static_cast<double>(span) * SAMPLES_PER_KEYFRAME;
        const size_t sampleCount = std::clamp<size_t>(static_cast<size_t>(std::ceil(requested)), 1, MAX_SAMPLES);
        s.depthStep = span / static_cast<float>(sampleCount);
        s.times.resize(sampleCount + 1);
        s.times[0] = 0.0f;

        // Trapezoid rule on 1/speed. The time a step of depth takes is that step divided by the
        // speed over it, so it is the reciprocal that has to be averaged across the step, not the
        // speed: averaging the speed itself would make a slow stretch look faster than it runs.
        double accumulated = 0.0;
        double previousInverse = 1.0 / static_cast<double>(s.speedAt(s.startDepth));
        for (size_t i = 1; i <= sampleCount; ++i) {
            const float depth = s.startDepth - static_cast<float>(i) * s.depthStep;
            const double inverse = 1.0 / static_cast<double>(s.speedAt(depth));
            accumulated += static_cast<double>(s.depthStep) * (previousInverse + inverse) * 0.5;
            s.times[i] = static_cast<float>(accumulated);
            previousInverse = inverse;
        }

        // A hold enters the mapping as pure time: its depth is reached at the moment the integral
        // says, and nothing moves for as long as the hold lasts.
        std::vector<VidTimelineHold> sorted;
        for (const auto &hold: timeline.holds) {
            if (hold.seconds > 0.0f && hold.depth <= s.startDepth && hold.depth >= s.endDepth) {
                sorted.push_back(hold);
            }
        }
        std::ranges::sort(sorted, [](const VidTimelineHold &a, const VidTimelineHold &b) {
            return a.depth > b.depth;
        });
        float held = 0.0f;
        for (const auto &[depth, seconds]: sorted) {
            s.holds.push_back({.depth = depth, .seconds = seconds, .startTime = s.integralAt(depth) + held});
            held += seconds;
        }

        s.totalSeconds = static_cast<float>(accumulated) + held;
        return s;
    }

    float TimelineSchedule::speedAt(const float depth) const {
        if (!hasSpeedTrack) {
            return uniformSpeed;
        }
        return clampSpeed(evaluateTrack(speedTrack, depth, uniformSpeed));
    }

    uint64_t TimelineSchedule::totalFrames(const float fps) const {
        if (fps <= 0.0f || totalSeconds <= 0.0f) {
            return 0;
        }
        return static_cast<uint64_t>(std::ceil(static_cast<double>(totalSeconds) * static_cast<double>(fps)));
    }

    float TimelineSchedule::integralAt(const float depth) const {
        if (uniform || times.empty()) {
            return std::max(startDepth - depth, 0.0f) / uniformSpeed;
        }
        const auto last = static_cast<float>(times.size() - 1);
        const float position = std::clamp((startDepth - depth) / depthStep, 0.0f, last);
        const auto index = static_cast<size_t>(position);
        if (index + 1 >= times.size()) {
            return times.back();
        }
        const float frac = position - static_cast<float>(index);
        return times[index] + (times[index + 1] - times[index]) * frac;
    }

    float TimelineSchedule::invertIntegral(const float sec) const {
        if (uniform || times.empty()) {
            return startDepth - sec * uniformSpeed;
        }
        if (sec <= 0.0f) {
            return startDepth;
        }
        if (sec >= times.back()) {
            return endDepth;
        }
        // times only increases, so the segment holding this instant is one search away.
        const auto upper = std::ranges::upper_bound(times, sec);
        const auto index = static_cast<size_t>(std::distance(times.begin(), upper)) - 1;
        const float t0 = times[index];
        const float t1 = times[index + 1];
        const float frac = t1 > t0 ? (sec - t0) / (t1 - t0) : 0.0f;
        return startDepth - (static_cast<float>(index) + frac) * depthStep;
    }

    float TimelineSchedule::timeAt(const float depth) const {
        float held = 0.0f;
        for (const auto &hold: holds) {
            if (hold.depth > depth) {
                held += hold.seconds;
            }
        }
        return integralAt(depth) + held;
    }

    float TimelineSchedule::depthAt(const float sec) const {
        if (sec <= 0.0f) {
            return startDepth;
        }
        if (sec >= totalSeconds) {
            return endDepth;
        }
        if (uniform) {
            return startDepth - sec * uniformSpeed;
        }
        float held = 0.0f;
        for (const auto &hold: holds) {
            if (sec < hold.startTime) {
                break;
            }
            if (sec < hold.startTime + hold.seconds) {
                return hold.depth;
            }
            held += hold.seconds;
        }
        return std::clamp(invertIntegral(sec - held), endDepth, startDepth);
    }
}
