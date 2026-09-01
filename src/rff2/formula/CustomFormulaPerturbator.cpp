//
// Created and modified by AI; earlier exact dates unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-06, 2026-08-16
//

#include "CustomFormulaPerturbator.h"
#include <windows.h>

namespace merutilm::rff2 {

    CustomFormulaPerturbator::CustomFormulaPerturbator(ParallelRenderState& state, const FractalAttribute& calc,
                                                       double dcMax)
        : MandelbrotPerturbator(state, calc), dcMax(dcMax) {
        dummyReference = std::make_unique<DummyReference>();
        
        // Parse with error handling
        if (!parser.parse(calc.customFormula)) {
            hasParseError = true;
            std::string errMsg = "This formula cannot be used:\n" + calc.customFormula + 
                                "\n\nError: " + parser.getErrorMessage();
            MessageBoxA(nullptr, errMsg.c_str(), "Formula Error", MB_OK | MB_ICONWARNING);
        }
    }

    const MandelbrotReference* CustomFormulaPerturbator::getReference() const {
        return dummyReference.get();
    }

    dex CustomFormulaPerturbator::getDcMaxAsDoubleExp() const {
        return dex::value(dcMax);
    }

    double CustomFormulaPerturbator::iterate(const dex& dcr, const dex& dci) const {
         // If parse failed, return max iteration (treat all points as inside set)
         if (hasParseError) {
             return static_cast<double>(calc.maxIteration);
         }
         
         // Custom formula calculation using standard double
         double offR = static_cast<double>(dcr);
         double offI = static_cast<double>(dci);
         
         double centerR = calc.center.real.edit().double_value();
         double centerI = calc.center.imag.edit().double_value();
         
         std::complex<double> c(centerR + offR, centerI + offI);
         std::complex<double> z(0, 0);
         
         uint64_t maxIteration = calc.maxIteration;
         double bailout = calc.bailout;
         double bailout2 = bailout * bailout;
         
         double pd = 0.0; // previous distance
         double cd = 0.0; // current distance
         
         for (uint64_t i = 0; i < maxIteration; ++i) {
             if (state.interruptRequested()) return 0.0;
             
             z = parser.evaluate(z, c);

             // z now holds step i + 1, and that is the count the standard path reports for it:
             // it raises its own counter with the step, before testing the bailout. Returning the
             // loop index instead put every escaping pixel one whole iteration below the built-in
             // Mandelbrot, so the same z^2+c typed into the custom field came out a band off.
             const uint64_t escapeIteration = i + 1;

             // Check for NaN or infinity (invalid formula result)
             if (!std::isfinite(z.real()) || !std::isfinite(z.imag())) {
                 return static_cast<double>(escapeIteration);
             }
             
             pd = cd;
             cd = std::norm(z);
             
             if (cd > bailout2) {
                 // Apply smooth coloring like LightMandelbrotPerturbator
                 pd = std::sqrt(pd);
                 cd = std::sqrt(cd);
                 return getPotentialDoubleValueIteration(escapeIteration, pd, cd, calc.decimalizeIterationMethod, bailout);
             }
         }
         
         return static_cast<double>(maxIteration);
    }
}
