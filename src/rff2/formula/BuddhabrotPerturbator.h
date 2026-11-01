//
// Created by Opus 5.5 on 2026-10-26.
//

#pragma once
#include <atomic>
#include <memory>

#include "MandelbrotPerturbator.h"
#include "DummyReference.h"
#include "../data/Matrix.h"

namespace merutilm::rff2 {
    // Iteration-buffer pixel (x, y) lies at center + origin + x * axisX + y * axisY on the plane.
    struct BuddhabrotPlane {
        double originReal;
        double originImag;
        double axisXReal;
        double axisXImag;
        double axisYReal;
        double axisYImag;
    };

    // Buddhabrot: the density of escaping z^2+c orbits. Technique by Melinda Green (1993); see NOTICE.
    class BuddhabrotPerturbator : public MandelbrotPerturbator {
        std::unique_ptr<DummyReference> dummyReference;
        double dcMax;

    public:
        BuddhabrotPerturbator(ParallelRenderState &state, const FractalAttribute &calc, double dcMax);

        ~BuddhabrotPerturbator() override = default;

        const MandelbrotReference *getReference() const override;
        dex getDcMaxAsDoubleExp() const override;

        // A Buddhabrot pixel has no orbit of its own, so every point reads as inside; render() draws the view.
        double iterate(const dex &dcr, const dex &dci, float *escapeAngle, OrbitTrap *trap) const override;
        using MandelbrotPerturbator::iterate;

        // Fills matrix with 1 + 4 x the orbit density, below Max Iteration; false when interrupted.
        bool render(const BuddhabrotPlane &plane, Matrix<double> &matrix, uint32_t threads,
                    std::atomic<int> &renderedPixels) const;
    };
}
