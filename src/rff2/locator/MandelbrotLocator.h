//
// Created by Merutilm on 2025-05-16.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#pragma once
#include "../formula/LightMandelbrotPerturbator.h"
#include "../calc/fp_complex.h"
#include "../data/ApproxTableCache.h"

namespace merutilm::rff2 {
    struct MandelbrotLocator {
        static constexpr float MINIBROT_LOG_ZOOM_OFFSET = 1.5f;
        static constexpr float ZOOM_INCREMENT_LIMIT = 0.01f;
        static constexpr int CENTER_FIX_COUNT_LIMIT = 50;
        static constexpr long long CENTER_FIX_FAST_FAIL_MS = 10000000;

        std::unique_ptr<MandelbrotPerturbator> perturbator;

        static std::unique_ptr<fp_complex> findCenter(const MandelbrotPerturbator *perturbator);

        static std::unique_ptr<fp_complex> findCenterOffset(const MandelbrotPerturbator &perturbator);

        static std::unique_ptr<MandelbrotLocator> locateMinibrot(ParallelRenderState &state,
                                                                 const MandelbrotPerturbator *perturbator,
                                                                 ApproxTableCache &approxTableCache,
                                                                 const std::function<void(uint64_t, int)> &
                                                                 actionWhileFindingMinibrotCenter,
                                                                 const std::function<void(uint64_t, float)> &actionWhileCreatingTable,
                                                                 const std::function<void(float)> &actionWhileFindingMinibrotZoom);

    private:
        static std::unique_ptr<MandelbrotPerturbator> findAccurateCenterPerturbator(ParallelRenderState &state,
            const MandelbrotPerturbator *perturbator,
            ApproxTableCache &approxTableCache,
            const std::function<void(uint64_t, int)> &
            actionWhileFindingMinibrotCenter, const std::function<void(uint64_t, float)> &
            actionWhileCreatingTable);

        static bool checkMaxIterationOnly(const MandelbrotPerturbator &perturbator, uint64_t maxIteration);
    };
}
