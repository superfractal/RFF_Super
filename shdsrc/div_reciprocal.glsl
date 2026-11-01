//
// Modified by Opus 5.5 on 2026-10-05
//

#ifndef RFF_DIV_RECIPROCAL
#define RFF_DIV_RECIPROCAL

// a / b correctly rounded from a hoisted reciprocal r = 1/b by Markstein's two-FMA step; precise keeps the FMAs intact (see NOTICE).
double div_by_reciprocal(double a, double b, double r) {
    precise double q = a * r;
    precise double e = fma(-b, q, a);
    precise double q2 = fma(e, r, q);
    return q2;
}

#endif
