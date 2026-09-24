//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18, 2026-08-23
// Modified by Opus 5 on 2026-08-25, 2026-08-31
// Modified by GPT-6 on 2026-09-23
//

#include "TimelineSchedule.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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
        TimelineSchedule schedule;
        schedule.startDepth = startDepth;
        schedule.endDepth = std::min(endDepth, startDepth);
        schedule.uniformSpeed = clampSpeed(fallbackSpeed);

        const VidTimelineTrack *activeSpeedTrack = timeline.enabled
                                                       ? findTrack(timeline, vidTimelineTargetId(VidTimelineTarget::SPEED))
                                                       : nullptr;
        const bool hasSpeedKeys = activeSpeedTrack != nullptr && activeSpeedTrack->enabled &&
                                  !activeSpeedTrack->keys.empty();
        bool hasTimedHolds = false;
        if (timeline.enabled) {
            for (const auto &[depth, seconds]: timeline.holds) {
                if (seconds > 0.0f && depth <= schedule.startDepth && depth >= schedule.endDepth) {
                    hasTimedHolds = true;
                    break;
                }
            }
        }

        const float depthSpan = schedule.startDepth - schedule.endDepth;
        if ((!hasSpeedKeys && !hasTimedHolds) || depthSpan <= 0.0f) {
            // Nothing to integrate: one constant speed, which is what the export always did.
            schedule.uniform = true;
            schedule.totalSeconds = depthSpan > 0.0f ? depthSpan / schedule.uniformSpeed : 0.0f;
            return schedule;
        }

        schedule.uniform = false;
        if (hasSpeedKeys) {
            schedule.speedTrack = *activeSpeedTrack;
            schedule.hasSpeedTrack = true;
            schedule.speedMinimum = schedule.speedMaximum = schedule.speedTrack.keys.front().value;
            for (const auto &key : schedule.speedTrack.keys) {
                schedule.speedMinimum = std::min(schedule.speedMinimum, key.value);
                schedule.speedMaximum = std::max(schedule.speedMaximum, key.value);
            }
        }

        const double requestedSamples = static_cast<double>(depthSpan) * SAMPLES_PER_KEYFRAME;
        const size_t sampleCount = std::clamp<size_t>(static_cast<size_t>(std::ceil(requestedSamples)), 1, MAX_SAMPLES);
        schedule.depthStep = depthSpan / static_cast<float>(sampleCount);
        schedule.times.resize(sampleCount + 1);
        schedule.times[0] = 0.0f;

        const auto inverseSpeed = [&schedule](double depth) {
            return 1.0 / static_cast<double>(schedule.speedAt(static_cast<float>(depth)));
        };
        const auto integratePiece = [&](double upper, double lower) {
            const double width = upper - lower;
            if (width <= 0.0) {
                return 0.0;
            }
            VidKeyInterpolation mode = VidKeyInterpolation::STEP;
            if (schedule.hasSpeedTrack) {
                const float middle = static_cast<float>((upper + lower) * 0.5);
                const auto &keys = schedule.speedTrack.keys;
                const size_t below = static_cast<size_t>(std::ranges::distance(
                    keys.begin(), std::ranges::partition_point(keys, [middle](const VidTimelineKey &key) {
                        return key.depth >= middle;
                    })));
                if (below > 0 && below < keys.size()) {
                    mode = keys[below - 1].out;
                }
            }
            if (mode == VidKeyInterpolation::STEP) {
                return width * inverseSpeed((upper + lower) * 0.5);
            }
            if (mode == VidKeyInterpolation::LINEAR) {
                const double highSpeed = 1.0 / inverseSpeed(upper);
                const double lowSpeed = 1.0 / inverseSpeed(lower);
                const double difference = lowSpeed - highSpeed;
                return difference == 0.0 ? width / highSpeed
                                         : width * std::log1p(difference / highSpeed) / difference;
            }
            const auto simpson = [](double high, double low, double first, double middle, double last) {
                return (high - low) * (first + 4.0 * middle + last) / 6.0;
            };
            const auto refine = [&](auto &&self, double high, double low, double first, double middle,
                                    double last, double whole, int remaining) -> double {
                const double center = (high + low) * 0.5;
                const double leftMiddle = (high + center) * 0.5;
                const double rightMiddle = (center + low) * 0.5;
                const double left = simpson(high, center, first, inverseSpeed(leftMiddle), middle);
                const double right = simpson(center, low, middle, inverseSpeed(rightMiddle), last);
                const double combined = left + right;
                if (remaining == 0 || std::abs(combined - whole) <= 15.0 * (1e-10 + 1e-8 * std::abs(combined))) {
                    return combined + (combined - whole) / 15.0;
                }
                return self(self, high, center, first, inverseSpeed(leftMiddle), middle, left, remaining - 1) +
                       self(self, center, low, middle, inverseSpeed(rightMiddle), last, right, remaining - 1);
            };
            const double highValue = inverseSpeed(upper);
            const double lowValue = inverseSpeed(lower);
            const double midValue = inverseSpeed((upper + lower) * 0.5);
            return refine(refine, upper, lower, highValue, midValue, lowValue,
                          simpson(upper, lower, highValue, midValue, lowValue), 14);
        };
        double integratedSeconds = 0.0;
        float previousDepth = schedule.startDepth;
        size_t nextKey = 0;
        for (size_t i = 1; i <= sampleCount; ++i) {
            const float depth = i == sampleCount ? schedule.endDepth
                                                  : schedule.startDepth - static_cast<float>(i) * schedule.depthStep;
            double upper = previousDepth;
            if (schedule.hasSpeedTrack) {
                const auto &keys = schedule.speedTrack.keys;
                while (nextKey < keys.size() && keys[nextKey].depth >= upper) {
                    ++nextKey;
                }
                while (nextKey < keys.size() && keys[nextKey].depth > depth) {
                    integratedSeconds += integratePiece(upper, keys[nextKey].depth);
                    upper = keys[nextKey].depth;
                    ++nextKey;
                }
            }
            integratedSeconds += integratePiece(upper, depth);
            schedule.times[i] = static_cast<float>(integratedSeconds);
            previousDepth = depth;
        }

        // A hold enters the mapping as pure time: its depth is reached at the moment the integral
        // says, and nothing moves for as long as the hold lasts.
        std::vector<VidTimelineHold> sortedHolds;
        for (const auto &hold: timeline.holds) {
            if (hold.seconds > 0.0f && hold.depth <= schedule.startDepth && hold.depth >= schedule.endDepth) {
                sortedHolds.push_back(hold);
            }
        }
        std::ranges::sort(sortedHolds, [](const VidTimelineHold &a, const VidTimelineHold &b) {
            return a.depth > b.depth;
        });
        float heldSeconds = 0.0f;
        for (const auto &[depth, seconds]: sortedHolds) {
            schedule.holds.push_back({.depth = depth, .seconds = seconds,
                                      .startTime = schedule.integralAt(depth) + heldSeconds});
            heldSeconds += seconds;
        }

        schedule.totalSeconds = static_cast<float>(integratedSeconds) + heldSeconds;
        return schedule;
    }

    float TimelineSchedule::speedAt(const float depth) const {
        if (!hasSpeedTrack) {
            return uniformSpeed;
        }
        return clampSpeed(evaluateTrack(speedTrack, depth, uniformSpeed, speedMinimum, speedMaximum));
    }

    uint64_t TimelineSchedule::totalFrames(const float fps) const {
        if (!std::isfinite(fps) || fps <= 0.0f || !std::isfinite(totalSeconds) || totalSeconds <= 0.0f) {
            return 0;
        }
        const double frames = std::ceil(static_cast<double>(totalSeconds) * static_cast<double>(fps));
        const double firstOutOfRange = std::ldexp(1.0, std::numeric_limits<uint64_t>::digits);
        return frames >= firstOutOfRange ? std::numeric_limits<uint64_t>::max()
                                         : static_cast<uint64_t>(frames);
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
