//
// Modified by GPT-6 on 2026-09-11, 2026-09-20, 2026-09-23
//

// Independent implementation of the supplied Lustre mathematical specification; provenance and limitations: NOTICE.
double lustre_reduce_phase(double phase) {
    const double tau = 6.283185307179586;
    const double tauTail = 2.4492935982947064e-16;
    double quotient = round(phase / tau);
    precise double remainder = fma(-quotient, tau, phase) - quotient * tauTail;
    double correction = round(remainder / tau);
    return fma(-correction, tau, remainder) - correction * tauTail;
}

float lustre_height(double value, double center) {
    if (value <= 0.0 || value >= iteration_info_attr.max_value) {
        return 0.0;
    }
    double m = max(value, 1.0), c = max(center, 1.0);
    double difference = m - c;
    // On zoom-out, evaluate the original height differences at the saved reference interval; see NOTICE.
    if (slope_attr.surface_zoom_log2 < 0.0) {
        float z = slope_attr.surface_zoom_log2;
        difference *= ldexp(double(exp2(fract(z))), int(floor(z)));
        m = c + difference;
    }
    float x = float(difference / (c + 4.0));
    float h;
    if (abs(x) < 0.125) {
        float term = x, sum = 0.0;
        for (int k = 1; k <= 12; ++k) {
            sum += term / float(k);
            term *= -x;
        }
        h = sum * 1.4426950408889634;
    } else {
        h = log2(float((m + 4.0) / (c + 4.0)));
    }
    if (slope_attr.surface_relief_waves > 0.0) {
        double delta = difference * double(slope_attr.surface_wave_frequency);
        double phase = c * double(slope_attr.surface_wave_frequency) + delta * 0.5;
        float middle = float(lustre_reduce_phase(phase));
        float halfDelta = float(lustre_reduce_phase(delta * 0.5));
        h += 0.48 * slope_attr.surface_relief_waves * cos(middle) * sin(halfDelta);
    }
    return slope_attr.surface_invert_relief > 0.5 ? -h : h;
}

vec3 lustre_sobel(uvec2 coord, int radius, double center) {
    int r = max(1, min(radius, min(
        min(int(coord.x), int(coord.y)),
        min(int(iteration_info_attr.extent.x) - 1 - int(coord.x),
            int(iteration_info_attr.extent.y) - 1 - int(coord.y)))));
    float nw = lustre_height(get_iteration(coord, ivec2(-r, -r)), center);
    float n = lustre_height(get_iteration(coord, ivec2(0, -r)), center);
    float ne = lustre_height(get_iteration(coord, ivec2(r, -r)), center);
    float w = lustre_height(get_iteration(coord, ivec2(-r, 0)), center);
    float e = lustre_height(get_iteration(coord, ivec2(r, 0)), center);
    float sw = lustre_height(get_iteration(coord, ivec2(-r, r)), center);
    float s = lustre_height(get_iteration(coord, ivec2(0, r)), center);
    float se = lustre_height(get_iteration(coord, ivec2(r, r)), center);
    vec3 sum = vec3((ne - nw) + 2.0 * (e - w) + (se - sw),
                    (sw - nw) + 2.0 * (s - n) + (se - ne),
                    ((nw + n) + (ne + w)) + ((e + sw) + (s + se)));
    return sum / vec3(float(8 * r), float(8 * r), 8);
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

vec3 lustre_relief(uvec2 coord, double center) {
    float scale = float(max(iteration_info_attr.canvas_extent.y, 1u)) / 800.0;
    int r = max(1, int(round(slope_attr.macro_radius * scale)));
    // Evaluate each enabled radius through one stencil body, retaining its original tap and sum order; see NOTICE.
    int radii[6] = int[6](
        1, 2, max(1, int(round(float(r) * 0.66))), r,
        max(1, int(round(float(r) * 0.33))),
        max(1, int(round(slope_attr.surface_ao_radius * scale))));
    vec3 samples[6];
    [[dont_unroll]] for (int i = 0; i < 6; ++i) {
        if (i == 1 && !(slope_attr.surface_normal_smooth > 0.0)) {
            samples[i] = samples[0];
            continue;
        }
        if ((i == 3 || i == 4) && !(slope_attr.macro_relief > 0.0)) {
            continue;
        }
        if (i == 5 && !(slope_attr.ao_intensity > 0.0)) {
            continue;
        }
        samples[i] = lustre_sobel(coord, radii[i], center);
    }
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
