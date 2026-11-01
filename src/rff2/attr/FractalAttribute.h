//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21
// Modified by Opus 5 on 2026-08-31
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-10-26
//

#pragma once

#include <cstdint>
#include <string>

#include "../calc/fp_complex.h"
#include "FrtDecimalizeIterationMethod.h"
#include "FrtMPAAttribute.h"
#include "FrtPanoramaLayout.h"
#include "FrtProjectionMethod.h"
#include "FrtReferenceCompAttribute.h"
#include "FrtReuseReferenceMethod.h"

namespace merutilm::rff2 {
    enum class FractalFormulaType {
        MANDELBROT = 0,
        CUSTOM = 1,
        BUDDHABROT = 2
    };

    // Buddhabrot stays out of the menus, panels and settings files until its coloring is finished.
    inline constexpr bool BUDDHABROT_AVAILABLE = false;

    struct FractalAttribute final {
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
        uint32_t buddhabrotSamples = 16; // random orbits started per iteration-buffer pixel
        uint64_t buddhabrotMinIteration = 0; // orbits escaping in this many iterations or fewer are left out
    };
}
