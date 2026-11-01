//
// Modified by Opus 5.5 on 2026-10-05, 2026-10-06
//

// Moved from vk_fog.frag on 2026-10-05 (see its header for this code's earlier history) to share with vk_chaos_irregularity.comp; the includer declares iteration_info_attr and iteration_attr first.
#ifndef RFF_FOG_ITERATION
#define RFF_FOG_ITERATION

#include "div_reciprocal.glsl"

// Bounds-checked fetch in buffer (top-left origin) coordinates. Returns -1 as a
// sentinel for out-of-bounds so callers can substitute.
double fetch_buffer(ivec2 bcoord){
    if (bcoord.x < 0 || bcoord.y < 0 ||
        bcoord.x >= int(iteration_info_attr.extent.x) ||
        bcoord.y >= int(iteration_info_attr.extent.y)) {
        return -1.0;
    }
    return iteration_attr.iterations[bcoord.y * iteration_info_attr.extent.x + bcoord.x];
}

double get_iteration_raw(ivec2 coord){
    coord.y = int(iteration_info_attr.extent.y) - 1 - coord.y; // -> buffer coords
    int cx = int(iteration_info_attr.extent.x) / 2;
    int cy = int(iteration_info_attr.extent.y) / 2;
    bool onCol = (coord.x == cx);
    bool onRow = (coord.y == cy);
    if (onCol && onRow) {
        double a = fetch_buffer(ivec2(cx - 1, cy - 1));
        double b = fetch_buffer(ivec2(cx + 1, cy - 1));
        double c = fetch_buffer(ivec2(cx - 1, cy + 1));
        double d = fetch_buffer(ivec2(cx + 1, cy + 1));
        if (a >= 0.0 && b >= 0.0 && c >= 0.0 && d >= 0.0) return (a + b + c + d) * 0.25;
    } else if (onCol) {
        double a = fetch_buffer(ivec2(cx - 1, coord.y));
        double b = fetch_buffer(ivec2(cx + 1, coord.y));
        if (a >= 0.0 && b >= 0.0) return (a + b) * 0.5;
    } else if (onRow) {
        double a = fetch_buffer(ivec2(coord.x, cy - 1));
        double b = fetch_buffer(ivec2(coord.x, cy + 1));
        if (a >= 0.0 && b >= 0.0) return (a + b) * 0.5;
    }

    return fetch_buffer(coord);
}

double get_iteration(uvec2 iter_coord, ivec2 offset){
    ivec2 coord = ivec2(iter_coord) + offset;
    double value = get_iteration_raw(coord);

    if (value >= 0.0) {
        return value;
    }

    ivec2 mirror_coord = ivec2(iter_coord) - offset;
    double mirror_value = get_iteration_raw(mirror_coord);
    if (mirror_value >= 0.0) {
        return mirror_value;
    }

    // Both sides OOB (image smaller than 3 pixels in this axis). Fall back to
    // the center pixel.
    ivec2 center = ivec2(iter_coord);
    center.y = int(iteration_info_attr.extent.y) - 1 - center.y;
    if (center.x >= 0 && center.y >= 0 &&
        center.x < int(iteration_info_attr.extent.x) &&
        center.y < int(iteration_info_attr.extent.y)) {
        return iteration_attr.iterations[center.y * iteration_info_attr.extent.x + center.x];
    }
    return 0.0;
}

// log2(1 + x), as a series rather than through log(). Vulkan pins log() to an ABSOLUTE error of
// 2^-21 for arguments in [0.5, 2], so for an argument a hair above 1 - which is every tap in a
// deep, smooth view - it is allowed to be wrong by more than the answer itself. See the slope
// pass for the full account. Every term below is reached from x by multiplication alone, so the
// relative accuracy holds however small x gets.
float log2_1p(float x) {
    // The series to x^11, which |x| <= 0.25 leaves within 2.3e-8 relative, inside float's own rounding.
    float p = 1.0 / 11.0;
    p = 1.0 / 10.0 - x * p;
    p = 1.0 / 9.0 - x * p;
    p = 1.0 / 8.0 - x * p;
    p = 1.0 / 7.0 - x * p;
    p = 1.0 / 6.0 - x * p;
    p = 0.2 - x * p;
    p = 0.25 - x * p;
    p = 1.0 / 3.0 - x * p;
    p = 0.5 - x * p;
    return x * (1.0 - x * p) * 1.4426950408889634;
}

// Shading height of one tap in doublings, measured against the centre pixel instead of against
// zero. Mirrors the slope pass, including why: only differences of heights are taken from here,
// so the shared log2(centre) cancels, and leaving it out is what holds the precision. Taking log2
// of the absolute count meant casting a double count down to a float, which past about 8.4 million
// iterations quantises away the whole fractional part the gradient is made of, turning the height
// field into a staircase that the Sobel reads as spikes along each riser.
float height_rel(double it, double center) {
    double a = max(it, 1.0);
    double b = max(center, 1.0);
    // Sterbenz: while the two are within a factor of two the subtraction is exact in double, so
    // the ratio reaches float as a small number carrying its full relative precision.
    double dr = (a - b) / b;
    // 0.25 rather than 0.5: the five-term series is worth 0.45% at 0.5 and the switch would hand
    // that over as a step. Mirrors the slope pass, where the same bound is set out in full.
    if (abs(dr) <= 0.25) {
        return log2_1p(float(dr));
    }
    // Far apart, which happens where an interior tap sits beside an exterior one.
    return log2(float(a)) - log2(float(b));
}


// height_rel with the centre's clamp and reciprocal taken once for all the taps around it.
float height_rel_hoisted(double it, double b, double invB) {
    double a = max(it, 1.0);
    double dr = div_by_reciprocal(a - b, b, invB);
    if (abs(dr) <= 0.25) {
        return log2_1p(float(dr));
    }
    return log2(float(a)) - log2(float(b));
}

// Relative double-precision iteration differences preserve local structure even at very deep zoom.
// Interior taps carry no count, so they are left out of the fit; where interior and exterior taps mix, how evenly they mix counts as irregularity too, and -1 marks a stencil that is all interior.
float chaos_irregularity(uvec2 coord, int radius) {
    ivec2 extent = ivec2(iteration_info_attr.extent);
    if (min(extent.x, extent.y) < 3) return 0.0;
    radius = min(radius, (min(extent.x, extent.y) - 1) / 2);
    int inset = min(radius + 2, (min(extent.x, extent.y) - 1) / 2);
    coord = uvec2(clamp(ivec2(coord), ivec2(inset), extent - inset - 1));
    double center = get_iteration(coord, ivec2(0));
    bool centerCounted = center > 0.0 && center < iteration_info_attr.max_value;
    double centerClamped = max(center, 1.0);
    double invCenter = 1.0 / centerClamped;
    float h[9];
    bool counted[9];
    int taps = 0;
    float energy = 0.0;
    [[dont_unroll]] for (int y = -1; y <= 1; ++y) {
        [[dont_unroll]] for (int x = -1; x <= 1; ++x) {
            int i = (y + 1) * 3 + x + 1;
            double iteration = get_iteration(coord, ivec2(x, y) * radius);
            counted[i] = iteration > 0.0 && iteration < iteration_info_attr.max_value;
            h[i] = counted[i] && centerCounted ? height_rel_hoisted(iteration, centerClamped, invCenter) : 0.0;
            taps += counted[i] ? 1 : 0;
            energy += abs(h[i]);
        }
    }
    if (taps == 0) return -1.0;
    // 1 for an even split of interior and exterior taps, falling to 0 as either side takes the whole stencil.
    float mixing = 1.0 - abs(float(2 * taps - 9)) / 9.0;
    if (!centerCounted || taps < 4) return taps == 9 ? 0.0 : mixing;
    // Central differences where both sides count, one-sided against the centre (height 0) where only one does.
    vec2 gradient = vec2(
        counted[3] && counted[5] ? (h[5] - h[3]) * 0.5 : counted[5] ? h[5] : counted[3] ? -h[3] : 0.0,
        counted[1] && counted[7] ? (h[7] - h[1]) * 0.5 : counted[7] ? h[7] : counted[1] ? -h[1] : 0.0);
    float residual = 0.0;
    [[dont_unroll]] for (int y = -1; y <= 1; ++y) {
        [[dont_unroll]] for (int x = -1; x <= 1; ++x) {
            int i = (y + 1) * 3 + x + 1;
            residual += counted[i] ? abs(h[i] - dot(gradient, vec2(x, y))) : 0.0;
        }
    }
    float fit = energy > 0.0 ? clamp(residual / energy, 0.0, 1.0) : 0.0;
    return taps == 9 ? fit : max(fit, mixing);
}

#endif
