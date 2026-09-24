//
// Modified by GPT-6 on 2026-09-18, 2026-09-23
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace merutilm::rff2 {
    struct SmoothZoomMotion {
        struct View {
            double scale = 1.0;
            double x = 0.0;
            double y = 0.0;
        };

        static constexpr double duration = 0.22;

        static double progress(double seconds) {
            const double t = std::clamp(seconds / duration, 0.0, 1.0);
            return t * t * (3.0 - 2.0 * t);
        }

        static double sourceScale(double delta, double seconds, bool widerSource) {
            return std::pow(10.0,
                            widerSource ? delta * (1.0 - progress(seconds)) : -delta * progress(seconds));
        }

        static double sourceOffset(double anchor, double scale) {
            return anchor * (1.0 - scale);
        }

        static float previewClarity(float savedClarity, uint32_t width, uint32_t height, uint32_t coverage) {
            const float longest = static_cast<float>(std::max({width, height, 1u}));
            return std::min({savedClarity, 0.5f, 640.0f / (longest * std::max(1u, coverage))});
        }

        static double sourcePixel(uint32_t pixel, uint32_t fullSize, uint32_t previewSize) {
            if (fullSize == previewSize) {
                return pixel;
            }
            return (pixel + 0.5) * fullSize / previewSize - 0.5;
        }

        static uint32_t previewPixel(uint32_t pixel, uint32_t previewSize, uint32_t fullSize) {
            return static_cast<uint32_t>(std::min<uint64_t>(
                previewSize - 1,
                (uint64_t(pixel) * 2 + 1) * previewSize / (2 * fullSize)));
        }

        static View relative(View view, View source) {
            return {view.scale / source.scale, (view.x - source.x) / source.scale, (view.y - source.y) / source.scale};
        }

        static bool covered(View view) {
            constexpr double tolerance = 1e-9;
            return view.scale > 0 && view.x >= -tolerance && view.y >= -tolerance &&
                view.x + view.scale <= 1.0 + tolerance && view.y + view.scale <= 1.0 + tolerance;
        }

        static View interpolate(View from, View to, double seconds) {
            const double p = progress(seconds);
            if (p >= 1.0) {
                return to;
            }
            const double scale = std::exp(std::lerp(std::log(from.scale), std::log(to.scale), p));
            const double weight = std::abs(to.scale - from.scale) > 1e-12
                                      ? (scale - from.scale) / (to.scale - from.scale)
                                      : p;
            return {scale, std::lerp(from.x, to.x, weight), std::lerp(from.y, to.y, weight)};
        }

        static View enclosing(View first, View second) {
            const double left = std::min(first.x, second.x);
            const double top = std::min(first.y, second.y);
            const double right = std::max(first.x + first.scale, second.x + second.scale);
            const double bottom = std::max(first.y + first.scale, second.y + second.scale);
            const double scale = std::max(right - left, bottom - top) * 1.2;
            return {scale, (left + right - scale) * 0.5, (top + bottom - scale) * 0.5};
        }
    };
}
