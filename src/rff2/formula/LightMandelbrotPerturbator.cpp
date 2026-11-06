//
// Created by Merutilm on 2025-05-10.
// Modified by Sonnet 5 on 2026-07-06
// Modified by GPT-5 on 2026-07-11
// Modified by Opus 5 on 2026-09-04
// Modified by GPT-6 on 2026-09-18
// Modified by Opus 5.5 on 2026-10-07, 2026-10-12, 2026-10-16, 2026-11-06
//

#include "LightMandelbrotPerturbator.h"

#include <cmath>


#include "Perturbator.h"


namespace merutilm::rff2 {


    LightMandelbrotPerturbator::LightMandelbrotPerturbator(ParallelRenderState &state, const FractalAttribute &calc,
                                                           const double dcMax, const int exp10,
                                                           const uint64_t initialPeriod,
                                                           ApproxTableCache &tableRef,
                                                           std::function<void(uint64_t)> &&actionPerRefCalcIteration,
                                                           std::function<void(uint64_t, double)> &&
                                                           actionPerCreatingTableIteration,
                                                           const bool arbitraryPrecisionFPGBn,
                                                           std::unique_ptr<LightMandelbrotReference> reusedReference,
                                                           std::unique_ptr<LightMPATable> reusedTable,
                                                           const double offR,
                                                           const double offI) : MandelbrotPerturbator(state, calc),
                                                                                dcMax(dcMax), offR(offR), offI(offI) {
        if (reusedReference == nullptr) {
            reference = LightMandelbrotReference::createReference(state, calc, exp10, initialPeriod, dcMax,
                                                                  arbitraryPrecisionFPGBn,
                                                                  std::move(actionPerRefCalcIteration));
        } else {
            reference = std::move(reusedReference);
        }

        if (reference == Constants::NullPointer::PROCESS_TERMINATED_REFERENCE) {
            return;
        }

        if (reusedTable == nullptr) {
            table = std::make_unique<LightMPATable>(state, *reference, &calc.mpaAttribute, dcMax,
                                                    tableRef,
                                                    std::move(actionPerCreatingTableIteration));
        } else {
            table = std::move(reusedTable);
        }
    }


    double LightMandelbrotPerturbator::iterate(const dex &dcr, const dex &dci, float *escapeAngle, OrbitTrap *trap) const {
        if (escapeAngle != nullptr) *escapeAngle = 0.0f;
        if (trap != nullptr) *trap = OrbitTrap{};
        if (state.interruptRequested()) return 0.0;

        const double dcr1 = static_cast<double>(dcr) + offR;
        const double dci1 = static_cast<double>(dci) + offI;

        uint64_t iteration = 0;
        uint64_t refIteration = 0;
        int absIteration = 0;
        const uint64_t maxRefIteration = reference->longestPeriod();

        double dzr = 0; // delta z
        double dzi = 0;
        double zr = 0; // z
        double zi = 0;

        double cd = 0;
        double pd = cd;
        const bool isAbs = calc.absoluteIterationMode;
        const uint64_t maxIteration = calc.maxIteration;
        const float bailout = calc.bailout;
        // Squared in double: a float square overflows to infinity once the bailout passes
        // sqrt(FLT_MAX) (~1.8e19), and an infinite threshold makes "cd > bailout2" never true, so
        // every pixel would come back as max iteration.
        const double bailout2 = static_cast<double>(bailout) * static_cast<double>(bailout);
        OrbitTrapAccumulator<double> traps(trap != nullptr ? &trapShape : nullptr);
        // Every return past this point reports the traps gathered up to it.
        const auto finish = [&](const double value) {
            if (trap != nullptr) *trap = traps.result();
            return value;
        };
        const auto sampleTrap = [&](const double r, const double i, const double norm) {
            if (trap == nullptr || !(norm <= bailout2)) return;
            traps.sample(std::min(std::abs(r), std::abs(i)), norm, iteration);
            if (traps.measuresShape()) traps.sampleShape(r - trapShape.centerReal, i - trapShape.centerImag, iteration);
        };


        while (iteration < maxIteration) {
            if (table != nullptr) {
                if (const LightPA *mpaPtr = table->lookup(refIteration, dzr, dzi); mpaPtr != nullptr) {
                    const LightPA &mpa = *mpaPtr;
                    const double dzr1 = mpa.anr * dzr - mpa.ani * dzi + mpa.bnr * dcr1 - mpa.bni * dci1;
                    const double dzi1 = mpa.anr * dzi + mpa.ani * dzr + mpa.bnr * dci1 + mpa.bni * dcr1;

                    dzr = dzr1;
                    dzi = dzi1;

                    iteration += mpa.skip;
                    refIteration += mpa.skip;
                    ++absIteration;

                    if (iteration >= maxIteration) {
                        return finish(static_cast<double>(isAbs ? absIteration : maxIteration));
                    }
                    // The skip's endpoint is the only point of its interval the trap sees.
                    if (trap != nullptr) {
                        const uint64_t index = ArrayCompressor::compress(reference->compressor, refIteration);
                        const double sr = reference->refReal[index] + dzr;
                        const double si = reference->refImag[index] + dzi;
                        sampleTrap(sr, si, sr * sr + si * si);
                    }
                    continue;
                }
            }


            if (refIteration != maxRefIteration) {
                const uint64_t index = ArrayCompressor::compress(reference->compressor, refIteration);
                const double zr1 = reference->refReal[index] * 2 + dzr;
                const double zi1 = reference->refImag[index] * 2 + dzi;

                const double zr2 = zr1 * dzr - zi1 * dzi + dcr1;
                const double zi2 = zr1 * dzi + zi1 * dzr + dci1;

                dzr = zr2;
                dzi = zi2;


                ++refIteration;
                ++iteration;
                ++absIteration;
            }

            const uint64_t index = ArrayCompressor::compress(reference->compressor, refIteration);
            zr = reference->refReal[index] + dzr;
            zi = reference->refImag[index] + dzi;


            // Only a real c keeps a real orbit; at Re(c) = -0.5 the second z is real for every Im(c).
            if (zi == 0 && zr < 0.25 && zr >= -2 &&
                reference->refImag[ArrayCompressor::compress(reference->compressor, 1)] + dci1 == 0) {
                //IT IS NOT SATISFIED MPA SKIP RADIUS CONDITION.
                //WHEN THE MAX ITERATION IS HIGH, REPEATS SEMI-INFINITELY.
                return finish(static_cast<double>(maxIteration));
            }

            pd = cd;
            cd = zr * zr + zi * zi;
            sampleTrap(zr, zi, cd);

            if (refIteration == maxRefIteration || cd < dzr * dzr + dzi * dzi) {
                refIteration = 0;
                dzr = zr;
                dzi = zi;
            }


            if (cd > bailout2) {
                if (escapeAngle != nullptr) *escapeAngle = escapeAngleOf(zr, zi);
                break;
            }

            if (absIteration % Constants::Fractal::EXIT_CHECK_INTERVAL == 0 && state.interruptRequested()) return 0.0;
        }

        if (isAbs) {
            return finish(absIteration);
        }

        if (iteration >= maxIteration) {
            return finish(static_cast<double>(maxIteration));
        }

        pd = sqrt(pd);
        cd = sqrt(cd);

        return finish(getQuadraticDoubleValueIteration(iteration, pd, cd, calc.decimalizeIterationMethod, bailout));
    }


    std::unique_ptr<LightMandelbrotPerturbator> LightMandelbrotPerturbator::reuse(
        const FractalAttribute &calc, const double dcMax, ApproxTableCache &tableRef) {

        const int exp10 = logZoomToExp10(calc.logZoom);
        double offR = 0;
        double offI = 0;
        uint64_t longestPeriod = 1;
        std::unique_ptr<LightMandelbrotReference> reusedReference = nullptr;

        if (reference == Constants::NullPointer::PROCESS_TERMINATED_REFERENCE) {
            //try to use process-terminated reference
            MessageBox(nullptr, "Please do not try to use PROCESS-TERMINATED Reference.", "Warning",
                       MB_OK | MB_ICONWARNING);
        } else {
            fp_complex_calculator centerOffset = calc.center.edit(exp10);
            centerOffset -= reference->center.edit(exp10);
            offR = centerOffset.getReal().double_value();
            offI = centerOffset.getImag().double_value();
            longestPeriod = reference->longestPeriod();
            reusedReference = std::move(reference);
        }


        // Rebuild approximation radii when switching between normal and relaxed CPU settings.
        if (table && table->mpaSettings != calc.mpaAttribute) table.reset();
        return std::make_unique<LightMandelbrotPerturbator>(state, calc, dcMax, exp10, longestPeriod, tableRef,
                                                            [](uint64_t) {
                                                                //no action because the reference is already declared
                                                            }, [](uint64_t, double) {
                                                                //same reason
                                                            }, false, std::move(reusedReference),
                                                            std::move(table), offR, offI);
    }
}
