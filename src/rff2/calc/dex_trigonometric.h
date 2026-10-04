//
// Created by Merutilm on 2025-05-18.
// Modified by GPT-6 on 2026-09-29, 2026-10-03, 2026-10-04
//

#pragma once
#include <limits>
#include "dex_std.h"
#include "dex.h"

namespace merutilm::rff2 {
    struct dex_trigonometric {

        explicit dex_trigonometric() = delete;

        static void atan2(dex *result, const dex &y, const dex &x);

        static dex atan2(const dex &y, const dex &x);

        static void sin(dex *result, const dex &v);

        static dex sin(const dex &v);

        static void cos(dex *result, const dex &v);

        static dex cos(const dex &v);

        static void tan(dex *result, const dex &v);

        static dex tan(const dex &v);

        static void hypot_approx(dex *result, const dex &a, const dex &b);

        static dex hypot_approx(const dex &a, const dex &b);

        static void hypot(dex *result, const dex &a, const dex &b);

        static dex hypot(const dex &a, const dex &b);

        static void hypot2(dex *result, const dex &a, const dex &b);

        static dex hypot2(const dex &a, const dex &b);
    };


    inline void dex_trigonometric::atan2(dex *result, const dex &y, const dex &x) {
        if (!std::isfinite(y.mantissa) || !std::isfinite(x.mantissa) || y.mantissa == 0 || x.mantissa == 0) {
            dex::cpy(result, 0, std::atan2(y.mantissa, x.mantissa));
            return;
        }
        int yExponent = 0, xExponent = 0;
        const double ym = std::frexp(y.mantissa, &yExponent);
        const double xm = std::frexp(x.mantissa, &xExponent);
        const int64_t difference = static_cast<int64_t>(y.exp2) + yExponent - x.exp2 - xExponent;
        if (difference < -54 && xm > 0) {
            // At this scale atan(y/x) rounds to y/x; retain angles below the binary64 exponent range.
            int quotientExponent = 0;
            const double fraction = std::frexp(ym / xm, &quotientExponent);
            const int64_t exponent = difference + quotientExponent;
            const int storedExponent = static_cast<int>(std::max<int64_t>(exponent, std::numeric_limits<int>::min()));
            dex::cpy(result, storedExponent, std::ldexp(fraction, static_cast<int>(std::max<int64_t>(-1075, exponent - storedExponent))));
            return;
        }
        const double scaledY = difference < 0 ? std::ldexp(ym, static_cast<int>(std::max<int64_t>(-1075, difference))) : ym;
        const double scaledX = difference > 0 ? std::ldexp(xm, static_cast<int>(std::max<int64_t>(-1075, -difference))) : xm;
        dex::cpy(result, 0, std::atan2(scaledY, scaledX));
    }

    inline dex dex_trigonometric::atan2(const dex &y, const dex &x) {
        dex result = dex::ZERO;
        atan2(&result, y, x);
        return result;
    }


    inline void dex_trigonometric::sin(dex *result, const dex &v) {
        const double dv = std::sin(static_cast<double>(v));
        if (dv == 0) {
            dex::cpy(result, v);
        }
        dex::cpy(result, dv);
    }


    inline dex dex_trigonometric::sin(const dex &v) {
        dex result = dex::ZERO;
        sin(&result, v);
        return result;
    }

    inline void dex_trigonometric::cos(dex *result, const dex &v) {
        const double dv = std::cos(static_cast<double>(v));
        dex::cpy(result, dv);
    }

    inline dex dex_trigonometric::cos(const dex &v) {
        dex result = dex::ZERO;
        cos(&result, v);
        return result;
    }

    inline void dex_trigonometric::tan(dex *result, const dex &v) {
        const double dv = std::tan(static_cast<double>(v));
        if (dv == 0) {
            dex::cpy(result, v);
        }
        dex::cpy(result, dv);
    }

    inline dex dex_trigonometric::tan(const dex &v) {
        dex result = dex::ZERO;
        tan(&result, v);
        return result;
    }

    // Extended-precision form of the inherited RFF-2.0 GPL norm approximation in rff_math.h; see NOTICE.
    // Matching mathematical publication: Edgar Bonet, Stack Overflow answer 26607206 (2014), CC BY-SA 3.0; this is a source reference, see NOTICE.
    // 2026-10-04 clarification: the owner relays the original author's independent discovery of 0.428; the Bonet match does not establish copying, see NOTICE.
    inline void dex_trigonometric::hypot_approx(dex *result, const dex &a, const dex &b) {
        const dex a_abs = dex_std::abs(a);
        const dex b_abs = dex_std::abs(b);
        const dex mn = dex_std::min(a_abs, b_abs);
        const dex mx = dex_std::max(a_abs, b_abs);


        if (mn.is_zero()) {
            dex::cpy(result, mx);
            return;
        }

        if (mx.is_zero()) {
            dex::cpy(result, dex::ZERO);
            return;
        }
        dex::sqr(result, mn);
        *result *= 0.428;
        dex::div(result, *result, mx);
        dex::add(result, *result, mx);
    }

    inline dex dex_trigonometric::hypot_approx(const dex &a, const dex &b) {
        dex result = dex::ZERO;
        hypot_approx(&result, a, b);
        return result;
    }

    inline void dex_trigonometric::hypot(dex *result, const dex &a, const dex &b) {
        hypot2(result, a, b);
        dex::sqrt(result, *result);
    }

    inline dex dex_trigonometric::hypot(const dex &a, const dex &b) {
        dex result = dex::ZERO;
        hypot(&result, a, b);
        return result;
    }

    inline void dex_trigonometric::hypot2(dex *result, const dex &a, const dex &b) {
        dex sqr_temp = dex::ZERO;
        dex::sqr(&sqr_temp, a);
        dex::sqr(result, b);
        dex::add(result, *result, sqr_temp);
    }

    inline dex dex_trigonometric::hypot2(const dex &a, const dex &b) {
        dex result = dex::ZERO;
        hypot2(&result, a, b);
        return result;
    }
}
