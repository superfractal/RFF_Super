//
// Created by Opus 5.5 on 2026-10-16
// Modified by Opus 5.5 on 2026-10-29
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

#include "../attr/ShdPalOrbitTrapColoring.h"
#include "../attr/ShdPalOrbitTrapShape.h"
#include "../calc/dex.h"

namespace merutilm::rff2 {
    // The geometric trap the perturbators measure orbit points against; 2D take on GMT Fractals' geometric orbit trap, see NOTICE.
    struct TrapShapeParams {
        bool enabled = false;
        ShdPalOrbitTrapShape shape = ShdPalOrbitTrapShape::POINT;
        double centerReal = 0.0;
        double centerImag = 0.0;
        // Each kept only for the shapes that read it, so a value the shape ignores never asks for a recompute.
        double radius = 0.0;
        double angleDegrees = 0.0;
        uint32_t sides = 0;
        // The unit normal (cos, sin) of angleDegrees, exact at multiples of 90 degrees.
        double cosAngle = 1.0;
        double sinAngle = 0.0;
        // Polygon and Star: (cos, sin) of one sector's turn 2 pi / sides and of half of it, and the outline's
        // segment in the folded half sector, from the corner (radius, 0) to the inner corner (innerX, innerY).
        double sectorCos = 1.0;
        double sectorSin = 0.0;
        double halfCos = 1.0;
        double halfSin = 0.0;
        double innerX = 0.0;
        double innerY = 0.0;

        // A star's inner corners sit at this share of its radius.
        static constexpr double STAR_INNER_RATIO = 0.5;

        template<typename Palette>
        static TrapShapeParams of(const Palette &palette) {
            TrapShapeParams p;
            if (palette.orbitTrapColoring != ShdPalOrbitTrapColoring::SHAPE) return p;
            p.enabled = true;
            p.shape = palette.orbitTrapShape;
            p.centerReal = palette.orbitTrapCenterReal;
            p.centerImag = palette.orbitTrapCenterImag;
            if (orbitTrapShapeUsesRadius(p.shape)) p.radius = palette.orbitTrapRadius;
            if (orbitTrapShapeUsesAngle(p.shape)) p.angleDegrees = palette.orbitTrapAngle;
            if (orbitTrapShapeUsesSides(p.shape)) p.sides = std::clamp(palette.orbitTrapSides, ORBIT_TRAP_SIDES_MIN, ORBIT_TRAP_SIDES_MAX);
            unitOf(p.angleDegrees, p.cosAngle, p.sinAngle);
            if (p.sides != 0) {
                const double sector = 2.0 * std::numbers::pi / p.sides;
                p.sectorCos = std::cos(sector);
                p.sectorSin = std::sin(sector);
                p.halfCos = std::cos(sector / 2.0);
                p.halfSin = std::sin(sector / 2.0);
                // A polygon's inner corner is the middle of its edge, at the apothem.
                const double inner = p.shape == ShdPalOrbitTrapShape::STAR ? p.radius * STAR_INNER_RATIO
                                                                           : p.radius * p.halfCos;
                p.innerX = inner * p.halfCos;
                p.innerY = inner * p.halfSin;
            }
            return p;
        }

        // Equal when both are off, or when both measure the same trap.
        bool operator==(const TrapShapeParams &other) const {
            if (!enabled || !other.enabled) return enabled == other.enabled;
            return shape == other.shape && centerReal == other.centerReal && centerImag == other.centerImag &&
                   radius == other.radius && angleDegrees == other.angleDegrees && sides == other.sides;
        }

        static void unitOf(const double degrees, double &c, double &s) {
            const double r = std::remainder(degrees, 360.0);
            if (r == 0.0) { c = 1.0; s = 0.0; return; }
            if (r == 90.0) { c = 0.0; s = 1.0; return; }
            if (r == -90.0) { c = 0.0; s = -1.0; return; }
            if (r == 180.0 || r == -180.0) { c = -1.0; s = 0.0; return; }
            const double radians = r * (std::numbers::pi / 180.0);
            c = std::cos(radians);
            s = std::sin(radians);
        }
    };

    namespace orbit_trap_shape {
        inline double magnitude(const double r, const double i) {
            const double norm = r * r + i * i;
            // The plain square root unless the squares left double's normal range; then scaled first.
            if (norm >= 1.0e-300 && norm <= 1.0e300) return std::sqrt(norm);
            const double m = std::max(std::abs(r), std::abs(i));
            if (m == 0.0 || !std::isfinite(m)) return m;
            const double a = r / m;
            const double b = i / m;
            return m * std::sqrt(a * a + b * b);
        }

        inline dex magnitude(const dex &r, const dex &i) {
            dex root;
            dex::sqrt(&root, r * r + i * i);
            dex::normalize(&root);
            return root;
        }

        inline double absolute(const double v) { return std::abs(v); }

        inline dex absolute(const dex &v) { return v.sgn() < 0 ? -v : v; }

        // Both sides const: dex declares a non-const comparison it never defines.
        template<typename Num>
        bool less(const Num &a, const Num &b) { return a < b; }

        // arg(z - c) in [-pi, pi].
        inline double angle(const double dr, const double di) { return std::atan2(di, dr); }

        inline double angle(const dex &dr, const dex &di) {
            if (dr.is_zero() && di.is_zero()) return 0.0;
            // Both brought to the larger exponent first, so a pair below double's range keeps its direction.
            const int e = dr.is_zero() ? di.get_exp2() : di.is_zero() ? dr.get_exp2() : std::max(dr.get_exp2(), di.get_exp2());
            const double r = dr.is_zero() ? 0.0 : std::ldexp(dr.get_mantissa(), dr.get_exp2() - e);
            const double i = di.is_zero() ? 0.0 : std::ldexp(di.get_mantissa(), di.get_exp2() - e);
            return std::atan2(i, r);
        }

        inline bool positive(const double v) { return v > 0.0; }

        inline bool positive(const dex &v) { return v.sgn() > 0; }

        // The whole number of steps nearest x / step; 0 where the quotient leaves double's range, which only a far miss reaches.
        inline double nearestStep(const double x, const double step) {
            const double k = std::floor(x / step + 0.5);
            return std::isfinite(k) ? k : 0.0;
        }

        // |x - k step| for the k nearest, kept in Num so a hit on the center itself still resolves at any depth.
        template<typename Num>
        Num stepResidual(const Num &x, const double step) {
            return x - step * nearestStep(static_cast<double>(x), step);
        }

        // Polygon and Star: (u, v) folded into the half sector between the corner on the real axis and the inner corner,
        // then the distance to the outline's one segment there. The sector comes from the angle, and a point the rounding
        // left just past the inner corner's ray is mirrored back across it, so the fold is exact either way.
        template<typename Num>
        Num sectorDistance(const Num &u, const Num &v, const TrapShapeParams &p) {
            const double turns = angle(u, v) * p.sides / (2.0 * std::numbers::pi);
            const double k = std::floor(turns + 0.5);
            const double back = -k * 2.0 * std::numbers::pi / p.sides;
            const double c = std::cos(back);
            const double s = std::sin(back);
            Num x = u * c - v * s;
            Num y = u * s + v * c;
            if (!positive(y)) y = -y;
            if (const Num beyond = y * p.halfCos - x * p.halfSin; positive(beyond)) {
                const Num along = x * p.halfCos + y * p.halfSin;
                const Num mx = along * (2.0 * p.halfCos) - x;
                const Num my = along * (2.0 * p.halfSin) - y;
                x = mx;
                y = my;
            }
            const double ex = p.innerX - p.radius;
            const double ey = p.innerY;
            const Num ax = x - p.radius;
            const double t = std::clamp(static_cast<double>(ax * ex + y * ey) / (ex * ex + ey * ey), 0.0, 1.0);
            return magnitude(ax - ex * t, y - ey * t);
        }

        // The distance from z - c = (dr, di) to the trap; O(1) per orbit point.
        template<typename Num>
        Num distance(const Num &dr, const Num &di, const TrapShapeParams &p) {
            // Only the trap turns; the orbit is left as it is.
            const auto turned = [&](Num &u, Num &v) {
                u = dr * p.cosAngle + di * p.sinAngle;
                v = di * p.cosAngle - dr * p.sinAngle;
            };
            const auto nearer = [](const Num &a, const Num &b) { return less(b, a) ? b : a; };
            const bool sized = p.radius > 0.0;
            switch (p.shape) {
                case ShdPalOrbitTrapShape::POINT:
                    return magnitude(dr, di);
                case ShdPalOrbitTrapShape::CIRCLE:
                    return absolute(magnitude(dr, di) - p.radius);
                case ShdPalOrbitTrapShape::CROSS:
                case ShdPalOrbitTrapShape::GRID_LINES: {
                    Num u, v;
                    turned(u, v);
                    if (p.shape == ShdPalOrbitTrapShape::GRID_LINES && sized) {
                        return nearer(absolute(stepResidual(u, p.radius)), absolute(stepResidual(v, p.radius)));
                    }
                    return nearer(absolute(u), absolute(v));
                }
                case ShdPalOrbitTrapShape::GRID_POINTS: {
                    if (!sized) return magnitude(dr, di);
                    Num u, v;
                    turned(u, v);
                    return magnitude(stepResidual(u, p.radius), stepResidual(v, p.radius));
                }
                case ShdPalOrbitTrapShape::RINGS:
                    if (!sized) return magnitude(dr, di);
                    return absolute(stepResidual(magnitude(dr, di), p.radius));
                case ShdPalOrbitTrapShape::POLYGON:
                case ShdPalOrbitTrapShape::STAR: {
                    if (!sized || p.sides == 0) return magnitude(dr, di);
                    Num u, v;
                    turned(u, v);
                    return sectorDistance(u, v, p);
                }
                case ShdPalOrbitTrapShape::SPIRAL: {
                    const Num rho = magnitude(dr, di);
                    if (!sized) return rho;
                    Num u, v;
                    turned(u, v);
                    // Turns from the angle in [0, 1); the arm k + turns radii out is the nearest one along the radius.
                    double turns = angle(u, v) / (2.0 * std::numbers::pi);
                    if (turns < 0.0) turns += 1.0;
                    const double k = std::max(nearestStep(static_cast<double>(rho) / p.radius - turns, 1.0), 0.0);
                    // The arms all start at the center, so the center is never farther than rho.
                    return nearer(absolute(rho - p.radius * (k + turns)), rho);
                }
                case ShdPalOrbitTrapShape::LINE:
                default:
                    return absolute(dr * p.cosAngle + di * p.sinAngle);
            }
        }
    }
}
