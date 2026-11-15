//
// Created by Opus 5.5 on 2026-11-07
// Modified by Opus 5.5 on 2026-11-14
//

#pragma once
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace merutilm::rff2 {
    // z and D = dcMax * dz/dc where an orbit escaped, for the dz/dc relief; valid stays false for any other orbit.
    struct EscapeDerivative {
        double zr = 0.0;
        double zi = 0.0;
        double dr = 0.0;
        double di = 0.0;
        bool valid = false;

        // One pixel's packed gradient: two float32 words, the GPU reading it as one vec2.
        using Packed = uint64_t;
        static constexpr uint32_t FLOATS_PER_PIXEL = 2;

        // A pixel without dz/dc data; Slope falls back to its iteration Sobel there.
        static constexpr Packed EMPTY = 0;

        // One dc step, divided by the dcMax that D is scaled by: the pixel to the right (x) and the row below (y).
        struct PixelStep {
            double xr = 0.0;
            double xi = 0.0;
            double yr = 0.0;
            double yi = 0.0;
        };

        // The smooth iteration's gradient in pixels, x right and y one row up, as float32: log2 of its length low, angle + 4 high.
        // Half floats stepped the angle by 0.004 rad, which drew contour lines through every effect that reads it; float32 steps it by 5e-7.
        // The +4 keeps the high word in [0.86, 7.15], so a packed pixel is never EMPTY.
        [[nodiscard]] static Packed pack(const EscapeDerivative &d, const PixelStep &step) {
            if (!d.valid) return EMPTY;
            const double z2 = d.zr * d.zr + d.zi * d.zi;
            const double lnR = 0.5 * std::log(z2);
            if (!(lnR > 0.0) || !std::isfinite(lnR)) return EMPTY;
            // q = D / z; the c-gradient of ln|z| is (Re q, -Im q), so that of the smooth iteration n + 1 - log2(ln|z|) is -(Re q, -Im q) / (ln2 lnR).
            const double qr = (d.dr * d.zr + d.di * d.zi) / z2;
            const double qi = (d.di * d.zr - d.dr * d.zi) / z2;
            const double s = -1.0 / (std::numbers::ln2 * lnR);
            const double gx = s * (step.xr * qr - step.xi * qi);
            const double gyDown = s * (step.yr * qr - step.yi * qi);
            const double gy = -gyDown;
            const double length = std::hypot(gx, gy);
            if (!(length > 0.0) || !std::isfinite(length)) return EMPTY;
            const double logLength = std::log2(length);
            if (!std::isfinite(logLength)) return EMPTY;
            const auto low = std::bit_cast<uint32_t>(static_cast<float>(logLength));
            const auto high = std::bit_cast<uint32_t>(static_cast<float>(std::atan2(gy, gx) + 4.0));
            return static_cast<Packed>(low) | static_cast<Packed>(high) << 32;
        }
    };
}
