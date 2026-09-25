//
// Modified by GPT-6 on 2026-09-23, 2026-09-26
//

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "TimelineEvaluator.hpp"
#include "TimelineSchedule.hpp"

namespace merutilm::rff2 {
    class TimelineAnimationPhases {
    public:
        static constexpr size_t AXIS_COUNT = 3 + 2 * TEXTURE_LAYER_COUNT + 2 * PATTERN_LAYER_COUNT + 2;
        using Values = std::array<double, AXIS_COUNT>;

        TimelineAnimationPhases(const TimelineSchedule &sourceSchedule, const VidTimelineAttribute &timeline,
                                const ShaderAttribute &sourceBase)
            : schedule(sourceSchedule), evaluator(timeline), base(sourceBase) {
            boundaries.push_back(0.0);
            boundaries.push_back(schedule.getTotalSeconds());
            if (timeline.enabled) {
                for (const auto &track : timeline.tracks) {
                    if (!track.enabled) continue;
                    for (const auto &key : track.keys) boundaries.push_back(schedule.timeAt(key.depth));
                }
                for (const auto &hold : timeline.holds) {
                    if (hold.seconds <= 0.0f || hold.depth > schedule.getStartDepth() ||
                        hold.depth < schedule.getEndDepth()) continue;
                    const double start = schedule.timeAt(hold.depth);
                    boundaries.push_back(start);
                    boundaries.push_back(start + hold.seconds);
                }
            }
            std::ranges::sort(boundaries);
            boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
        }

        [[nodiscard]] static Values ratesOf(const ShaderAttribute &shader) {
            Values rates{};
            size_t index = 0;
            rates[index++] = shader.palette.animationSpeed;
            rates[index++] = shader.palette.animationFlowSpeed;
            rates[index++] = shader.stripe.animationSpeed;
            for (const auto &layer : shader.textures) {
                rates[index++] = layer.scrollU;
                rates[index++] = layer.scrollV;
            }
            for (const auto &layer : shader.patterns) {
                rates[index++] = layer.scrollU;
                rates[index++] = layer.scrollV;
            }
            rates[index++] = shader.warp.scrollU;
            rates[index] = shader.warp.scrollV;
            return rates;
        }

        [[nodiscard]] const Values &at(const double seconds) {
            if (!std::isfinite(seconds)) return phases;
            if (seconds < position || position < 0.0) {
                phases.fill(0.0);
                position = 0.0;
            }
            const double end = seconds;
            double start = position;
            auto boundary = std::ranges::upper_bound(boundaries, start);
            while (boundary != boundaries.end() && *boundary < end) {
                add(integrate(start, *boundary, 10));
                start = *boundary++;
            }
            if (start != end) add(integrate(start, end, 10));
            position = end;
            return phases;
        }

    private:
        TimelineSchedule schedule;
        TimelineEvaluator evaluator;
        ShaderAttribute base;
        std::vector<double> boundaries;
        Values phases{};
        double position = 0.0;

        [[nodiscard]] Values rateAt(const double seconds) const {
            ShaderAttribute shader;
            evaluator.evaluate(schedule.depthAt(seconds),
                               seconds, base, shader);
            return ratesOf(shader);
        }

        [[nodiscard]] Values gauss(const double start, const double end, const bool fourPoints) const {
            constexpr std::array<double, 2> twoNodes{0.5773502691896257, -0.5773502691896257};
            constexpr std::array<double, 4> fourNodes{0.8611363115940526, 0.3399810435848563,
                                                      -0.3399810435848563, -0.8611363115940526};
            constexpr std::array<double, 4> fourWeights{0.3478548451374538, 0.6521451548625461,
                                                        0.6521451548625461, 0.3478548451374538};
            const double middle = (start + end) * 0.5;
            const double half = (end - start) * 0.5;
            Values result{};
            const size_t count = fourPoints ? fourNodes.size() : twoNodes.size();
            for (size_t point = 0; point < count; ++point) {
                const double node = fourPoints ? fourNodes[point] : twoNodes[point];
                const double weight = fourPoints ? fourWeights[point] : 1.0;
                const Values rates = rateAt(middle + half * node);
                for (size_t axis = 0; axis < AXIS_COUNT; ++axis) result[axis] += rates[axis] * weight * half;
            }
            return result;
        }

        [[nodiscard]] Values integrate(const double start, const double end, const int remaining) const {
            const Values coarse = gauss(start, end, false);
            const Values fine = gauss(start, end, true);
            bool accurate = true;
            for (size_t axis = 0; axis < AXIS_COUNT; ++axis) {
                if (std::abs(fine[axis] - coarse[axis]) > 1e-7 * (1.0 + std::abs(fine[axis]))) {
                    accurate = false;
                    break;
                }
            }
            if (accurate || remaining == 0) return fine;
            const double middle = (start + end) * 0.5;
            Values left = integrate(start, middle, remaining - 1);
            const Values right = integrate(middle, end, remaining - 1);
            for (size_t axis = 0; axis < AXIS_COUNT; ++axis) left[axis] += right[axis];
            return left;
        }

        void add(const Values &values) {
            for (size_t axis = 0; axis < AXIS_COUNT; ++axis) phases[axis] += values[axis];
        }
    };
}
