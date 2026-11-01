//
// Created by Merutilm on 2025-05-04.
// Modified by Opus 5.5 on 2026-10-25
//

#include "fp_complex.h"

#include <iostream>

#include "dex.h"

namespace merutilm::rff2 {
    fp_complex::fp_complex(const std::string &re, const std::string &im, const int exp10) : real(fp_decimal(re, exp10)),
                                                                                            imag(fp_decimal(im, exp10)) {
    }

    fp_complex::fp_complex(const fp_complex_calculator &calc) : real(calc.getRealClone()), imag(calc.getImagClone()) {
    }



    fp_complex fp_complex::addCenterDouble(const dex re, const dex im, const int exp10) const {
        auto calc = edit(exp10);
        const auto add = fp_complex_calculator(re, im, exp10);
        calc += add;
        return fp_complex(calc);
    }


    fp_complex_calculator fp_complex::edit(const int exp10) const {
        return fp_complex_calculator(real.edit(), imag.edit(), exp10);
    }
}