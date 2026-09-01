//
// Created by Merutilm on 2025-05-08.
// Modified by Sonnet 5 on 2026-07-06
// Modified by GPT-5 on 2026-07-11
// Modified by Opus 5 on 2026-08-06
//

#pragma once
#include <cmath>
#include "../attr/FrtDecimalizeIterationMethod.h"
#include "../constants/Constants.hpp"

namespace merutilm::rff2 {
    struct Perturbator {

        virtual ~Perturbator() = default;

        static int logZoomToExp10(float logZoom);

        static double getDoubleValueIteration(uint64_t iteration, double prevIterDistance,
                                              double currIterDistance,
                                              const FrtDecimalizeIterationMethod &decimalizeIterationMethod, float
                                              bailout);
        static double getQuadraticDoubleValueIteration(uint64_t iteration, double prevIterDistance,
                                                       double currIterDistance,
                                                       const FrtDecimalizeIterationMethod &decimalizeIterationMethod,
                                                       float bailout);

        static double getPotentialDoubleValueIteration(uint64_t iteration, double prevIterDistance,
                                                       double currIterDistance,
                                                       const FrtDecimalizeIterationMethod &decimalizeIterationMethod,
                                                       float bailout);

    private:
        static double applyIterationRatio(double iteration, double ratio,
                                          const FrtDecimalizeIterationMethod &decimalizeIterationMethod);
    };

    inline int Perturbator::logZoomToExp10(const float logZoom) {
        return -static_cast<int>(logZoom) - Constants::Fractal::EXP10_ADDITION;
    }

    inline double Perturbator::getDoubleValueIteration(const uint64_t iteration, const double prevIterDistance,
        const double currIterDistance, const FrtDecimalizeIterationMethod &decimalizeIterationMethod, const float bailout) {
        // prevIterDistance = p
        // currIterDistance = c
        // bailout = b
        //
        // a = b - p (p < b)
        // b = c - b (c > b)
        // 0 dec 1 decimal value
        // a : b ratio
        // ratio = a / (a + b) = (b - p) / (c - p)

        if (prevIterDistance == currIterDistance) {
            return static_cast<double>(iteration);
        }
        const double ratio = (bailout - prevIterDistance) / (currIterDistance - prevIterDistance);
        return applyIterationRatio(iteration, ratio, decimalizeIterationMethod);
    }

    inline double Perturbator::getQuadraticDoubleValueIteration(const uint64_t iteration,
        const double prevIterDistance, const double currIterDistance,
        const FrtDecimalizeIterationMethod &decimalizeIterationMethod, const float bailout) {
        if (decimalizeIterationMethod == FrtDecimalizeIterationMethod::NONE) {
            return static_cast<double>(iteration);
        }

        const double logBailout = std::log(static_cast<double>(bailout));
        if (bailout > 1.0f && currIterDistance > 1.0 && logBailout > 0.0) {
            if (const double potentialScale = std::log(currIterDistance) / logBailout;
                std::isfinite(potentialScale) && potentialScale > 0.0) {
                // Already the continuous smooth iteration. Re-splitting it at the integer boundary
                // and pushing the fraction back through applyIterationRatio remaps every step
                // separately: the slope of the LOG_LOG map is 0.52 at the top of a step and 2.08 at
                // the bottom of the next, so the gradient kinks 4x at every integer. An animated
                // palette crossing those kinks speeds up and slows down once per iteration, which
                // reads as the whole image breathing.
                if (const double smooth = static_cast<double>(iteration) + 1.0 - std::log2(potentialScale);
                    std::isfinite(smooth)) {
                    return smooth;
                }
            }
        }

        // Fallback for a bailout too small for the potential: the crude linear ratio is what the
        // decimalize remap was designed for, so it still applies here.
        if (prevIterDistance == currIterDistance) {
            return static_cast<double>(iteration);
        }
        const double ratio = (bailout - prevIterDistance) / (currIterDistance - prevIterDistance);
        return applyIterationRatio(static_cast<double>(iteration), ratio, decimalizeIterationMethod);
    }

    // Escape-potential smoothing for formulas whose degree is not fixed at 2. Linear interpolation
    // between the last two magnitudes collapses to ~0 whenever a step overshoots the bailout (with
    // bailout 128 a typical escape lands near |z|^2, giving a ratio around 1e-5), which quantizes
    // the iteration count to integers and makes an animated palette snap band to band.
    inline double Perturbator::getPotentialDoubleValueIteration(const uint64_t iteration,
        const double prevIterDistance, const double currIterDistance,
        const FrtDecimalizeIterationMethod &decimalizeIterationMethod, const float bailout) {
        if (decimalizeIterationMethod == FrtDecimalizeIterationMethod::NONE) {
            return static_cast<double>(iteration);
        }

        const double logBailout = std::log(static_cast<double>(bailout));
        const double logCurrent = std::log(currIterDistance);

        if (bailout > 1.0f && currIterDistance > 1.0 && logBailout > 0.0) {
            // |z| grows like |z|^degree once it escapes, so the last two log-magnitudes give the
            // degree directly. Fall back to the quadratic case when the previous step is too small
            // for the estimate to mean anything.
            double degree = 2.0;
            if (prevIterDistance > 1.0) {
                if (const double d = logCurrent / std::log(prevIterDistance);
                    std::isfinite(d) && d > 1.0 && d <= 64.0) {
                    degree = d;
                }
            }
            // Returned as-is for the same reason as the quadratic case: remapping the fraction per
            // integer step would kink the gradient at every boundary.
            if (const double potentialScale = logCurrent / logBailout;
                std::isfinite(potentialScale) && potentialScale > 0.0) {
                if (const double smooth = static_cast<double>(iteration) + 1.0
                                          - std::log(potentialScale) / std::log(degree);
                    std::isfinite(smooth)) {
                    return smooth;
                }
            }
        }

        if (prevIterDistance == currIterDistance) {
            return static_cast<double>(iteration);
        }
        const double ratio = (bailout - prevIterDistance) / (currIterDistance - prevIterDistance);
        return applyIterationRatio(static_cast<double>(iteration), ratio, decimalizeIterationMethod);
    }

    inline double Perturbator::applyIterationRatio(const double iteration, double ratio,
        const FrtDecimalizeIterationMethod &decimalizeIterationMethod) {
        switch (decimalizeIterationMethod) {
            using enum FrtDecimalizeIterationMethod;
            case NONE : {
                ratio = 0;
                break;
            }
            case LINEAR : {
                break;
            }
            case SQUARE_ROOT : {
                ratio = sqrt(ratio);
                break;
            }
            case LOG : {
                ratio = log10(ratio + 1) / Constants::Num::LOG10_2;
                break;
            }
            case LOG_LOG : {
                constexpr double logBailout = Constants::Num::LOG10_2;
                ratio = log10(log10(ratio + 1) / logBailout + 1) / logBailout;
                break;
            }
            default : break;
        }

        if (!std::isfinite(ratio)) {
            ratio = 0.0;
        } else if (ratio < 0.0) {
            ratio = 0.0;
        } else if (ratio > 1.0) {
            ratio = 1.0;
        }

        return iteration + ratio;
    }
}
