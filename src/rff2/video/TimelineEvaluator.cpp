// Modified by GPT-5 on 2026-08-18
// Modified by Opus 5 on 2026-08-21, 2026-08-24, 2026-08-25

#include "TimelineEvaluator.hpp"

#include <algorithm>
#include <cmath>

#include "../attr/VidTimelineTarget.h"

namespace merutilm::rff2 {
    namespace {
        struct Segment {
            size_t index = 0;
            float u = 0.0f;
        };

        // A segment holds the depths from its own key down to, but not including, the next key's:
        // a sample landing exactly on a key belongs to the segment that key starts. Including the
        // lower end here made STEP hand back the key before it for one more sample.
        Segment locateSegment(const std::vector<VidTimelineKey> &keys, const float depth) {
            for (size_t i = 0; i + 1 < keys.size(); ++i) {
                if (keys[i].depth >= depth && depth > keys[i + 1].depth) {
                    const float span = keys[i].depth - keys[i + 1].depth;
                    return {i, span > 0.0f ? std::clamp((keys[i].depth - depth) / span, 0.0f, 1.0f) : 0.0f};
                }
            }
            return {};
        }

        float cubic(const float p0, const float p1, const float p2, const float p3, const float u) {
            const float u2 = u * u;
            const float u3 = u2 * u;
            return 0.5f * (2.0f * p1 + (-p0 + p2) * u +
                           (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * u2 +
                           (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * u3);
        }

        float curveRatio(const VidKeyInterpolation interpolation, const float u) {
            if (interpolation == VidKeyInterpolation::SMOOTH) {
                return u * u * (3.0f - 2.0f * u);
            }
            return u;
        }

        float evaluateValue(const VidTimelineTrack &track, const TimelineParamDesc &param, const float depth,
                            const float fallback) {
            const auto &keys = track.keys;
            if (keys.empty()) {
                return fallback;
            }
            if (keys.size() == 1 || depth >= keys.front().depth) {
                return std::clamp(std::isfinite(keys.front().value) ? keys.front().value : fallback,
                                  param.minValue, param.maxValue);
            }
            if (depth <= keys.back().depth) {
                return std::clamp(std::isfinite(keys.back().value) ? keys.back().value : fallback,
                                  param.minValue, param.maxValue);
            }
            const auto [index, u] = locateSegment(keys, depth);
            const float a = std::isfinite(keys[index].value) ? keys[index].value : fallback;
            const float b = std::isfinite(keys[index + 1].value) ? keys[index + 1].value : fallback;
            if (param.kind == TimelineParamKind::BOOL || param.kind == TimelineParamKind::ENUM ||
                keys[index].out == VidKeyInterpolation::STEP) {
                return std::clamp(a, param.minValue, param.maxValue);
            }
            float value = 0.0f;
            if (keys[index].out == VidKeyInterpolation::CUBIC) {
                const float before = index > 0 && std::isfinite(keys[index - 1].value)
                                         ? keys[index - 1].value
                                         : a;
                const float after = index + 2 < keys.size() && std::isfinite(keys[index + 2].value)
                                        ? keys[index + 2].value
                                        : b;
                value = cubic(before, a, b, after, u);
            } else {
                value = std::lerp(a, b, curveRatio(keys[index].out, u));
            }
            return std::clamp(std::isfinite(value) ? value : fallback, param.minValue, param.maxValue);
        }

        // IEC 61966-2-1 sRGB EOTF, acknowledged in NOTICE. A plain 2.2 power parts from it near black.
        float srgbToLinearChannel(const float c) {
            const float s = std::max(c, 0.0f);
            return s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
        }

        float linearToSrgbChannel(const float c) {
            const float s = std::clamp(c, 0.0f, 1.0f);
            return s <= 0.0031308f ? s * 12.92f : 1.055f * std::pow(s, 1.0f / 2.4f) - 0.055f;
        }

        glm::vec3 srgbToLinear(const glm::vec4 &color) {
            return {
                srgbToLinearChannel(color.r),
                srgbToLinearChannel(color.g),
                srgbToLinearChannel(color.b)
            };
        }

        // Matrices from Björn Ottosson, "A perceptual color space for image processing" (2020), published as MIT / public domain.
        glm::vec3 linearToOklab(const glm::vec3 &color) {
            const float l = 0.4122214708f * color.r + 0.5363325363f * color.g + 0.0514459929f * color.b;
            const float m = 0.2119034982f * color.r + 0.6806995451f * color.g + 0.1073969566f * color.b;
            const float s = 0.0883024619f * color.r + 0.2817188376f * color.g + 0.6299787005f * color.b;
            const float lc = std::cbrt(std::max(l, 0.0f));
            const float mc = std::cbrt(std::max(m, 0.0f));
            const float sc = std::cbrt(std::max(s, 0.0f));
            return {
                0.2104542553f * lc + 0.7936177850f * mc - 0.0040720468f * sc,
                1.9779984951f * lc - 2.4285922050f * mc + 0.4505937099f * sc,
                0.0259040371f * lc + 0.7827717662f * mc - 0.8086757660f * sc
            };
        }

        glm::vec3 oklabToLinear(const glm::vec3 &lab) {
            const float lc = lab.x + 0.3963377774f * lab.y + 0.2158037573f * lab.z;
            const float mc = lab.x - 0.1055613458f * lab.y - 0.0638541728f * lab.z;
            const float sc = lab.x - 0.0894841775f * lab.y - 1.2914855480f * lab.z;
            const float l = lc * lc * lc;
            const float m = mc * mc * mc;
            const float s = sc * sc * sc;
            return {
                4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
                -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
                -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s
            };
        }

        glm::vec4 fromOklab(const glm::vec3 &lab, const float alpha) {
            const glm::vec3 linear = oklabToLinear(lab);
            return {
                linearToSrgbChannel(linear.r),
                linearToSrgbChannel(linear.g),
                linearToSrgbChannel(linear.b),
                std::clamp(alpha, 0.0f, 1.0f)
            };
        }

        glm::vec4 evaluateColor(const VidTimelineTrack &track, const float depth, const glm::vec4 &fallback) {
            const auto &keys = track.keys;
            if (keys.empty()) {
                return fallback;
            }
            if (keys.size() == 1 || depth >= keys.front().depth) {
                return glm::clamp(keys.front().color, glm::vec4(0.0f), glm::vec4(1.0f));
            }
            if (depth <= keys.back().depth) {
                return glm::clamp(keys.back().color, glm::vec4(0.0f), glm::vec4(1.0f));
            }
            const auto [index, u] = locateSegment(keys, depth);
            if (keys[index].out == VidKeyInterpolation::STEP) {
                return glm::clamp(keys[index].color, glm::vec4(0.0f), glm::vec4(1.0f));
            }
            const glm::vec3 a = linearToOklab(srgbToLinear(keys[index].color));
            const glm::vec3 b = linearToOklab(srgbToLinear(keys[index + 1].color));
            glm::vec3 lab = {};
            float alpha = 0.0f;
            if (keys[index].out == VidKeyInterpolation::CUBIC) {
                const glm::vec3 before = index > 0 ? linearToOklab(srgbToLinear(keys[index - 1].color)) : a;
                const glm::vec3 after = index + 2 < keys.size()
                                            ? linearToOklab(srgbToLinear(keys[index + 2].color))
                                            : b;
                for (int component = 0; component < 3; ++component) {
                    lab[component] = cubic(before[component], a[component], b[component], after[component], u);
                }
                const float beforeAlpha = index > 0 ? keys[index - 1].color.a : keys[index].color.a;
                const float afterAlpha = index + 2 < keys.size() ? keys[index + 2].color.a : keys[index + 1].color.a;
                alpha = cubic(beforeAlpha, keys[index].color.a, keys[index + 1].color.a, afterAlpha, u);
            } else {
                const float ratio = curveRatio(keys[index].out, u);
                lab = glm::mix(a, b, ratio);
                alpha = std::lerp(keys[index].color.a, keys[index + 1].color.a, ratio);
            }
            return fromOklab(lab, alpha);
        }

        bool colorsDiffer(const glm::vec4 &a, const glm::vec4 &b) {
            return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
        }
    }

    TimelineEvaluator::TimelineEvaluator(const VidTimelineAttribute &source) : timeline(source) {
        if (!timeline.enabled) {
            return;
        }
        for (const auto &track: timeline.tracks) {
            if (track.enabled && !track.keys.empty() &&
                track.targetId != vidTimelineTargetId(VidTimelineTarget::SPEED) &&
                TimelineParams::find(track.targetId) != nullptr) {
                activeShaderTracks = true;
                break;
            }
        }
    }

    void TimelineEvaluator::evaluate(const float depth, const float sec, const ShaderAttribute &base,
                                     ShaderAttribute &out) const {
        (void) sec;
        out = base;
        if (!activeShaderTracks) {
            return;
        }
        for (const auto &track: timeline.tracks) {
            if (!track.enabled || track.keys.empty()) {
                continue;
            }
            const TimelineParamDesc *param = TimelineParams::find(track.targetId);
            if (param == nullptr || param->cost != TimelineApplyCost::CHEAP) {
                continue;
            }
            if (param->kind == TimelineParamKind::COLOR) {
                param->setColor(out, evaluateColor(track, depth, param->getColor(base)));
            } else {
                param->setValue(out, evaluateValue(track, *param, depth, param->getValue(base)));
            }
        }
    }

    TimelineDirtyMask TimelineEvaluator::diff(const ShaderAttribute &previous, const ShaderAttribute &next) const {
        TimelineDirtyMask dirty = TimelineDirtyMask::NONE;
        for (const auto &param: TimelineParams::all()) {
            const bool changed = param.kind == TimelineParamKind::COLOR
                                     ? colorsDiffer(param.getColor(previous), param.getColor(next))
                                     : param.getValue(previous) != param.getValue(next);
            if (changed) {
                dirty |= param.dirty;
            }
        }
        return dirty;
    }
}
