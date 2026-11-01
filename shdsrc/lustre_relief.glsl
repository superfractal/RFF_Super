//
// Modified by GPT-6 on 2026-09-11, 2026-09-20, 2026-09-23
// Modified by Opus 5.5 on 2026-10-05, 2026-10-08
//

#include "div_reciprocal.glsl"

// Independent implementation of the supplied Lustre mathematical specification; provenance and limitations: NOTICE.
double lustre_reduce_phase(double phase) {
    const double tau = 6.283185307179586;
    const double tauTail = 2.4492935982947064e-16;
    const double invTau = 0.15915494309189535;
    double quotient = round(div_by_reciprocal(phase, tau, invTau));
    precise double remainder = fma(-quotient, tau, phase) - quotient * tauTail;
    double correction = round(div_by_reciprocal(remainder, tau, invTau));
    return fma(-correction, tau, remainder) - correction * tauTail;
}

// One tap's height twice over: x with the relief waves, y without them, both from a single read of the tap.
vec2 lustre_height(double value, double center, double c4, double invC4, double phaseC, bool withWaves) {
    if (value <= 0.0 || value >= iteration_info_attr.max_value) {
        return vec2(0.0);
    }
    double m = max(value, 1.0), c = max(center, 1.0);
    double difference = m - c;
    // On zoom-out, evaluate the original height differences at the saved reference interval; see NOTICE.
    if (slope_attr.surface_zoom_log2 < 0.0) {
        float z = slope_attr.surface_zoom_log2;
        difference *= ldexp(double(exp2(fract(z))), int(floor(z)));
        m = c + difference;
    }
    float x = float(div_by_reciprocal(difference, c4, invC4));
    float h;
    if (abs(x) < 0.125) {
        float term = x, sum = 0.0;
        for (int k = 1; k <= 12; ++k) {
            sum += term / float(k);
            term *= -x;
        }
        h = sum * 1.4426950408889634;
    } else {
        h = log2(float(div_by_reciprocal(m + 4.0, c4, invC4)));
    }
    float calm = h;
    if (withWaves) {
        double delta = difference * double(slope_attr.surface_wave_frequency);
        // phaseC is c * frequency already reduced once per stencil; a value inside (-pi, pi) is what the reduction returns unchanged.
        double middlePhase = phaseC + delta * 0.5;
        float middle = float(abs(middlePhase) < 3.14159 ? middlePhase : lustre_reduce_phase(middlePhase));
        double halfPhase = delta * 0.5;
        float halfDelta = float(abs(halfPhase) < 3.14159 ? halfPhase : lustre_reduce_phase(halfPhase));
        h += 0.48 * slope_attr.surface_relief_waves * cos(middle) * sin(halfDelta);
    }
    return slope_attr.surface_invert_relief > 0.5 ? -vec2(h, calm) : vec2(h, calm);
}

// The ring radius a stencil actually uses: the requested one, shrunk so every tap stays inside the image.
int lustre_ring_radius(uvec2 coord, int radius) {
    return max(1, min(radius, min(
        min(int(coord.x), int(coord.y)),
        min(int(iteration_info_attr.extent.x) - 1 - int(coord.x),
            int(iteration_info_attr.extent.y) - 1 - int(coord.y)))));
}

// Sobel gradient (xy) and tap mean (z) on a ring of radius r, for the waved relief and the calm one alike.
void lustre_sobel(uvec2 coord, int r, double center, bool withWaves, out vec3 waved, out vec3 calm) {
    double c4 = max(center, 1.0) + 4.0;
    double invC4 = 1.0 / c4;
    double phaseC = withWaves ? lustre_reduce_phase(max(center, 1.0) * double(slope_attr.surface_wave_frequency)) : 0.0;
    vec2 nw = lustre_height(get_iteration(coord, ivec2(-r, -r)), center, c4, invC4, phaseC, withWaves);
    vec2 n = lustre_height(get_iteration(coord, ivec2(0, -r)), center, c4, invC4, phaseC, withWaves);
    vec2 ne = lustre_height(get_iteration(coord, ivec2(r, -r)), center, c4, invC4, phaseC, withWaves);
    vec2 w = lustre_height(get_iteration(coord, ivec2(-r, 0)), center, c4, invC4, phaseC, withWaves);
    vec2 e = lustre_height(get_iteration(coord, ivec2(r, 0)), center, c4, invC4, phaseC, withWaves);
    vec2 sw = lustre_height(get_iteration(coord, ivec2(-r, r)), center, c4, invC4, phaseC, withWaves);
    vec2 s = lustre_height(get_iteration(coord, ivec2(0, r)), center, c4, invC4, phaseC, withWaves);
    vec2 se = lustre_height(get_iteration(coord, ivec2(r, r)), center, c4, invC4, phaseC, withWaves);
    vec2 gx = (ne - nw) + 2.0 * (e - w) + (se - sw);
    vec2 gy = (sw - nw) + 2.0 * (s - n) + (se - ne);
    vec2 total = ((nw + n) + (ne + w)) + ((e + sw) + (s + se));
    vec3 divisor = vec3(float(8 * r), float(8 * r), 8);
    waved = vec3(gx.x, gy.x, total.x) / divisor;
    calm = vec3(gx.y, gy.y, total.y) / divisor;
}

// Independent Taylor evaluation preserves small relief and AO responses; provenance: NOTICE.
float lustre_one_minus_exp_negative(float x) {
    if (x < 0.125) {
        return x * (1.0 - x * (0.5 - x * (1.0 / 6.0 - x * (1.0 / 24.0 - x / 120.0))));
    }
    return 1.0 - exp(-x);
}

// Original smooth limit on additional relief preserves the uncompensated shape at extreme zoom; see NOTICE.
float lustre_compensated_magnitude(float magnitude, float zoomLog2, float depth) {
    if (zoomLog2 <= 0.0) {
        return magnitude;
    }
    float limit = log2(64.0 / max(depth, 0.000001));
    float linearEnd = limit - 1.0;
    float boundedZoom = zoomLog2 <= linearEnd ? zoomLog2 : limit - 1.0 / (zoomLog2 - linearEnd + 1.0);
    return magnitude * exp2(boundedZoom);
}

// Slope (xy) and AO (z) from the six rings: fine, smoothed, mid, wide, inner wide and AO.
vec3 lustre_relief_from_rings(vec3 samples[6], float scale) {
    vec3 fine = samples[0], smoothG = samples[1], mid = samples[2];
    vec2 gradient = mix(fine.xy, smoothG.xy, slope_attr.surface_normal_smooth);
    if (slope_attr.macro_relief > 0) {
        vec2 wide = (samples[3].xy + mid.xy + samples[4].xy) / 3.0;
        gradient = mix(gradient, wide, slope_attr.macro_relief);
    }
    vec2 gained = gradient * (68.0 * slope_attr.surface_relief_depth * scale);
    float component = max(abs(gained.x), abs(gained.y));
    float magnitude = component > 0.0 ? component * length(gained / component) : 0.0;
    if (magnitude > 0.0) {
        float compensated = magnitude;
        // Logarithmic zoom compensation avoids overflowing the bounded slope response at deep views; see NOTICE.
        if (slope_attr.surface_zoom_log2 != 0.0) {
            compensated = lustre_compensated_magnitude(magnitude, slope_attr.surface_zoom_log2, slope_attr.surface_relief_depth);
        }
        gained = (gained / magnitude) * (2.5 * lustre_one_minus_exp_negative(0.55 * pow(compensated, 0.70)));
    }
    float ao = 1.0;
    if (slope_attr.ao_intensity > 0) {
        float mean = samples[5].z;
        float concavity = max(0.0, 0.65 * mean + 0.35 * mid.z);
        ao = 1.0 - slope_attr.ao_intensity * lustre_one_minus_exp_negative(14.0 * slope_attr.surface_relief_depth * concavity);
    }
    return vec3(gained, ao);
}

// The relief with its waves (waved) and without them (calm); wantWaves false leaves the waves out of both.
void lustre_relief_pair(uvec2 coord, double center, bool wantWaves, out vec3 waved, out vec3 calm) {
    bool withWaves = wantWaves && slope_attr.surface_relief_waves > 0.0;
    float scale = float(max(iteration_info_attr.canvas_extent.y, 1u)) / 800.0;
    int r = max(1, int(round(slope_attr.macro_radius * scale)));
    // Evaluate each enabled radius through one stencil body, retaining its original tap and sum order; see NOTICE.
    int radii[6] = int[6](
        1, 2, max(1, int(round(float(r) * 0.66))), r,
        max(1, int(round(float(r) * 0.33))),
        max(1, int(round(slope_attr.surface_ao_radius * scale))));
    vec3 wavedRings[6], calmRings[6];
    // Ring radius each slot was evaluated at, 0 where it was copied or skipped; a later equal radius reuses that slot.
    int evaluated[6] = int[6](0, 0, 0, 0, 0, 0);
    [[dont_unroll]] for (int i = 0; i < 6; ++i) {
        if (i == 1 && !(slope_attr.surface_normal_smooth > 0.0)) {
            wavedRings[i] = wavedRings[0];
            calmRings[i] = calmRings[0];
            continue;
        }
        if ((i == 3 || i == 4) && !(slope_attr.macro_relief > 0.0)) {
            continue;
        }
        // The mid radius feeds only the macro blend and the AO concavity, so it is skipped when both are off.
        if (i == 2 && !(slope_attr.macro_relief > 0.0) && !(slope_attr.ao_intensity > 0.0)) {
            continue;
        }
        if (i == 5 && !(slope_attr.ao_intensity > 0.0)) {
            continue;
        }
        int ring = lustre_ring_radius(coord, radii[i]);
        int reused = -1;
        for (int j = 0; j < i; ++j) if (evaluated[j] == ring) reused = j;
        if (reused >= 0) {
            wavedRings[i] = wavedRings[reused];
            calmRings[i] = calmRings[reused];
        } else {
            lustre_sobel(coord, ring, center, withWaves, wavedRings[i], calmRings[i]);
            evaluated[i] = ring;
        }
    }
    waved = lustre_relief_from_rings(wavedRings, scale);
    calm = lustre_relief_from_rings(calmRings, scale);
}

// The relief as the clear coat sees it, waves included when they are on.
vec3 lustre_relief(uvec2 coord, double center) {
    vec3 waved, calm;
    lustre_relief_pair(coord, center, true, waved, calm);
    return waved;
}

// The relief without its waves, for Waves on Clear Coat Only.
vec3 lustre_relief_calm(uvec2 coord, double center) {
    vec3 waved, calm;
    lustre_relief_pair(coord, center, false, waved, calm);
    return calm;
}

// Distance to observed interior texel footprints, not a fractal distance estimator; see NOTICE.
float boundary_reflection_mask(uvec2 coord) {
    if (slope_attr.surface_boundary_guard <= 0.0) {
        return 1.0;
    }
    float distance = 3.0;
    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            ivec2 q = ivec2(coord) + ivec2(x, y);
            if (any(lessThan(q, ivec2(0))) || any(greaterThanEqual(q, ivec2(iteration_info_attr.extent)))) {
                continue;
            }
            double it = get_iteration_raw(q);
            if (it >= iteration_info_attr.max_value) {
                distance = min(distance, length(max(abs(vec2(x, y)) - 0.5, vec2(0))));
            }
        }
    }
    return mix(1.0, smoothstep(0.0, 1.5, distance), slope_attr.surface_boundary_guard);
}
