//
// Created by Opus 5.5 on 2026-10-12
// Modified by Opus 5.5 on 2026-10-16
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "OrbitTrapShape.h"
#include "../calc/dex.h"

namespace merutilm::rff2 {
    // Orbit trap statistics of one pixel, kept as log2 so deep-zoom distances stay in float range.
    struct OrbitTrap {
        // The planes a trap occupies in RenderScene's buffers, in the order of the fields below.
        static constexpr uint32_t PLANE_COUNT = 4;
        // All eight planes, kept only while Orbit Trap Coloring is Shape so the shape fields cost nothing otherwise.
        static constexpr uint32_t SHAPE_PLANE_COUNT = 8;
        // shapeStatus values; a pixel the shape never sampled stays NO_DATA rather than holding a stand-in distance.
        static constexpr float STATUS_NO_DATA = 0.0f;
        static constexpr float STATUS_VALID = 1.0f;
        // Stands in for a distance the orbit never sampled.
        static constexpr float FAR_LOG2 = 64.0f;
        // The log2 floor of an exact hit, so a zero distance stays finite.
        static constexpr double HIT_LOG2 = -1.0e6;
        // The wrap of the stored iteration, below float's 2^24 exact-integer limit.
        static constexpr uint64_t ITERATION_WRAP = 1ull << 22;

        // Nearest approach of an orbit point to the real or imaginary axis (the cross trap).
        float crossLog2 = FAR_LOG2;
        // Nearest and second nearest approach to the origin (the point trap).
        float pointLog2 = FAR_LOG2;
        float secondLog2 = FAR_LOG2;
        // One plus the wrapped iteration of the nearest approach to the origin; 0 marks a pixel without trap data.
        float iteration = 0.0f;
        // Nearest approach to the Orbit Trap Shape, one plus its wrapped iteration, arg(z - c) there, and the status.
        float shapeLog2 = FAR_LOG2;
        float shapeIteration = 0.0f;
        float shapeAngle = 0.0f;
        float shapeStatus = STATUS_NO_DATA;

        static constexpr uint32_t planeCountFor(const bool shape) {
            return shape ? SHAPE_PLANE_COUNT : PLANE_COUNT;
        }
    };

    // Takes the minima over the orbit points a perturbator actually evaluates; MPA-skipped interiors are not sampled.
    template<typename Num>
    class OrbitTrapAccumulator {
        Num cross{};
        Num point{};
        Num second{};
        uint64_t nearest = 0;
        bool sampled = false;
        bool sampledSecond = false;
        // The shape trap, measured only when one is given.
        const TrapShapeParams *shape = nullptr;
        Num shapeMin{};
        Num shapeR{};
        Num shapeI{};
        uint64_t shapeNearest = 0;
        bool shapeSampled = false;

        static double log2Of(const double v) {
            return v > 0.0 ? std::log2(v) : OrbitTrap::HIT_LOG2;
        }

        static double log2Of(const dex &v) {
            if (v.is_zero()) return OrbitTrap::HIT_LOG2;
            return static_cast<double>(v.get_exp2()) + std::log2(std::abs(v.get_mantissa()));
        }

        // Both sides const: dex declares a non-const comparison it never defines.
        static bool less(const Num &a, const Num &b) {
            return a < b;
        }

        static float stored(const double log2) {
            return static_cast<float>(std::max(log2, OrbitTrap::HIT_LOG2));
        }

    public:
        OrbitTrapAccumulator() = default;

        // shapeParams, when enabled, adds the shape trap; it must outlive the accumulator.
        explicit OrbitTrapAccumulator(const TrapShapeParams *shapeParams)
            : shape(shapeParams != nullptr && shapeParams->enabled ? shapeParams : nullptr) {
        }

        [[nodiscard]] bool measuresShape() const { return shape != nullptr; }

        // One orbit point inside the bailout as (Re z - Re c, Im z - Im c); only called when measuresShape().
        void sampleShape(const Num &dr, const Num &di, const uint64_t iteration) {
            const Num d = orbit_trap_shape::distance(dr, di, *shape);
            if (shapeSampled && !less(d, shapeMin)) return;
            shapeMin = d;
            shapeR = dr;
            shapeI = di;
            shapeNearest = iteration;
            shapeSampled = true;
        }

        // crossDistance is min(|Re z|, |Im z|) and pointNorm is |z|^2 of one orbit point inside the bailout.
        void sample(const Num &crossDistance, const Num &pointNorm, const uint64_t iteration) {
            if (!sampled) {
                cross = crossDistance;
                point = pointNorm;
                nearest = iteration;
                sampled = true;
                return;
            }
            if (less(crossDistance, cross)) cross = crossDistance;
            if (less(pointNorm, point)) {
                second = point;
                sampledSecond = true;
                point = pointNorm;
                nearest = iteration;
            } else if (!sampledSecond || less(pointNorm, second)) {
                second = pointNorm;
                sampledSecond = true;
            }
        }

        OrbitTrap result() const {
            OrbitTrap trap;
            trap.iteration = static_cast<float>(nearest % OrbitTrap::ITERATION_WRAP + 1);
            if (sampled) {
                trap.crossLog2 = stored(log2Of(cross));
                trap.pointLog2 = stored(0.5 * log2Of(point));
            }
            if (sampledSecond) {
                trap.secondLog2 = stored(0.5 * log2Of(second));
            }
            if (shapeSampled) {
                trap.shapeLog2 = stored(log2Of(shapeMin));
                trap.shapeIteration = static_cast<float>(shapeNearest % OrbitTrap::ITERATION_WRAP + 1);
                trap.shapeAngle = static_cast<float>(orbit_trap_shape::angle(shapeR, shapeI));
                trap.shapeStatus = OrbitTrap::STATUS_VALID;
            }
            return trap;
        }
    };
}
