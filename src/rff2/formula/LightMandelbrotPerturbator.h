//
// Created by Merutilm on 2025-05-10.
// Modified by GPT-6 on 2026-09-18
// Modified by Opus 5.5 on 2026-10-06, 2026-10-07, 2026-10-12
//

#pragma once

#include <array>

#include "LightMandelbrotReference.h"
#include "MandelbrotPerturbator.h"
#include "../mrthy/LightMPATable.h"

namespace merutilm::rff2 {
    class LightMandelbrotPerturbator final : public MandelbrotPerturbator{
        std::unique_ptr<LightMandelbrotReference> reference = nullptr;
        std::unique_ptr<LightMPATable> table = nullptr;

        const double dcMax;
        const double offR;
        const double offI;

    public:

        explicit LightMandelbrotPerturbator(ParallelRenderState &state, const FractalAttribute &calc, double dcMax, int exp10,
                                   uint64_t initialPeriod, ApproxTableCache &tableRef, std::function<void(uint64_t)> &&actionPerRefCalcIteration,
                                   std::function<void(uint64_t, double)> &&actionPerCreatingTableIteration,
                                   bool arbitraryPrecisionFPGBn = false, std::unique_ptr<LightMandelbrotReference> reusedReference = nullptr, std::unique_ptr<LightMPATable> reusedTable = nullptr,
                                   double offR = 0, double offI = 0);


        using MandelbrotPerturbator::iterate;

        double iterate(const dex &dcr, const dex &dci, float *escapeAngle, OrbitTrap *trap) const override;

        std::unique_ptr<LightMandelbrotPerturbator> reuse(const FractalAttribute &calc, double dcMax, ApproxTableCache &tableRef);

        const LightMandelbrotReference *getReference() const override;

        LightMPATable &getTable() const;

        // The table is absent when the reference was terminated before one was built.
        const LightMPATable *getTableOrNull() const { return table.get(); }

        // The reuse offset iterate() adds to every dc.
        std::array<double, 2> getOffset() const { return {offR, offI}; }

        double getDcMax() const;

        dex getDcMaxAsDoubleExp() const override;
    };



    inline const LightMandelbrotReference *LightMandelbrotPerturbator::getReference() const {
        return reference.get();
    }

    inline LightMPATable &LightMandelbrotPerturbator::getTable() const {
        return *table;
    }

    inline double LightMandelbrotPerturbator::getDcMax() const {
        return dcMax;
    }

    inline dex LightMandelbrotPerturbator::getDcMaxAsDoubleExp() const {
        return dex::value(dcMax);
    }
}