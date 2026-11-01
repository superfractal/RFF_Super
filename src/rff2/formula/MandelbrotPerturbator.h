//
// Created by Merutilm on 2025-05-18.
// Modified by Opus 5.5 on 2026-10-07, 2026-10-12, 2026-10-16
//

#pragma once
#include "MandelbrotReference.h"
#include "OrbitTrap.h"
#include "Perturbator.h"
#include "../mrthy/MPATable.h"
#include "../parallel/ParallelRenderState.h"
#include "../attr/FractalAttribute.h"

namespace merutilm::rff2 {
    struct MandelbrotPerturbator : public Perturbator {
        ParallelRenderState &state;
        const FractalAttribute calc;
        // The Orbit Trap Shape iterate() measures; set before a compute starts and left alone while it runs.
        TrapShapeParams trapShape{};

        explicit MandelbrotPerturbator(ParallelRenderState &state,
                                       const FractalAttribute &calculationSettings) : state(state),
            calc(calculationSettings) {
        }

        ~MandelbrotPerturbator() override = default;

        virtual const MandelbrotReference *getReference() const = 0;

        const FractalAttribute &getCalculationSettings() const {
            return calc;
        };

        virtual dex getDcMaxAsDoubleExp() const = 0;

        void setTrapShape(const TrapShapeParams &shape) {
            trapShape = shape;
        }

        // escapeAngle, when given, receives Perturbator::escapeAngleOf for an escaping orbit and 0 for any other.
        // trap, when given, receives the orbit trap statistics; a formula without traps leaves it marked as no data.
        virtual double iterate(const dex &dcr, const dex &dci, float *escapeAngle, OrbitTrap *trap) const = 0;

        double iterate(const dex &dcr, const dex &dci, float *escapeAngle) const {
            return iterate(dcr, dci, escapeAngle, nullptr);
        }

        double iterate(const dex &dcr, const dex &dci) const {
            return iterate(dcr, dci, nullptr, nullptr);
        }
    };
}
