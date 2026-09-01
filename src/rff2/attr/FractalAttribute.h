//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-31
//

#pragma once

#include "FrtDecimalizeIterationMethod.h"
#include "FrtMPAAttribute.h"
#include "FrtReferenceCompAttribute.h"
#include "FrtPanoramaLayout.h"
#include "FrtProjectionMethod.h"
#include "FrtReuseReferenceMethod.h"
#include "../calc/fp_complex.h"


namespace merutilm::rff2 {
    enum class FractalFormulaType {
        MANDELBROT,
        CUSTOM
    };

    struct FractalAttribute final{
        fp_complex center;
        float logZoom;
        uint64_t maxIteration;
        float bailout;
        FrtDecimalizeIterationMethod decimalizeIterationMethod;
        FrtMPAAttribute mpaAttribute;
        FrtReferenceCompAttribute referenceCompAttribute;
        FrtReuseReferenceMethod reuseReferenceMethod;
        bool autoMaxIteration;
        uint16_t autoIterationMultiplier;
        bool absoluteIterationMode;
        float rotation;
        FractalFormulaType formulaType = FractalFormulaType::MANDELBROT;
        std::string customFormula = "z^3+c";
        FrtProjectionMethod projectionMethod = FrtProjectionMethod::PLANAR;
        float panoramaRange = 3.0f; // log10 of the furthest radius the view reaches, in units of the horizon radius
        float panoramaPitch = -90.0f; // degrees the 360 camera looks up (+) or down (-); -90 faces the view center
        float panoramaFov = 100.0f; // horizontal field of view of the 360 camera, in degrees
        FrtPanoramaLayout panoramaLayout = FrtPanoramaLayout::GROUND;
    };
}
