//
// Modified by GPT-6 on 2026-09-11, 2026-09-20, 2026-09-23
// Modified by Opus 5.5 on 2026-10-22, 2026-10-23
//

// Original RFF_Super groove profile and bounded differential lighting, GPL-3.0-or-later; see NOTICE.
bool groove_enabled() {
    return slope_attr.groove_enabled > 0.5 && slope_attr.groove_depth > 0.0 &&
           (slope_attr.groove_width > 0.0 || slope_attr.groove_rounded > 0.5) && slope_attr.groove_count > 0.0 &&
           effects_attr.context.y > 0.5;
}

// Rounded Bands: each band between two lines is a rounded cord, lit against the surface it sits on.
float rounded_band_light(double phase, vec2 grad, float rate, vec2 baseSlope, float scale) {
    float bandWidth = 1.0 / rate;
    float fade = smoothstep(1.8 * scale, 5.0 * scale, bandWidth);
    float height = 0.2 * slope_attr.groove_depth * bandWidth;
    // Crease Blur widens the slope's averaging window past one pixel; 0 keeps the one-pixel window.
    float window = slope_attr.groove_crease_blur > 0.0 ? max(1.0, slope_attr.groove_crease_blur * scale) : 1.0;
    // Height change across the window, so the crease between two cords stays filtered.
    double halfStep = double(0.5 * rate * window);
    float rise = height * fade * (sin(float(PI) * float(fract(phase + halfStep))) -
                                  sin(float(PI) * float(fract(phase - halfStep)))) / window;
    float az = radians(slope_attr.azimuth);
    float ze = radians(slope_attr.zenith);
    vec3 light = vec3(cos(az) * sin(ze), sin(az) * sin(ze), cos(ze));
    vec3 flatNormal = normalize(vec3(-baseSlope, 1));
    vec3 cordNormal = normalize(vec3(-(baseSlope + grad / rate * rise), 1));
    const float ambient = 0.4;
    float lit = ambient + (1.0 - ambient) * max(dot(cordNormal, light), 0.0);
    float base = ambient + (1.0 - ambient) * max(dot(flatNormal, light), 0.0);
    float position = float(fract(phase));
    float creaseDistance = min(position, 1.0 - position) * bandWidth;
    float crease = 1.0 - 0.28 * (1.0 - smoothstep(0.0, 3.3 * scale + (window - 1.0) * 0.5, creaseDistance)) * fade;
    return log(max(lit / base * crease, 1e-3));
}

double groove_cycle(ivec2 coord, double iteration, bool animate) {
    vec2 pixel = vec2(coord) + 0.5;
    double anim = animation_offset_iterations(pixel);
    vec2 gradient = vec2(0);
    if (texture_attr.warp_enabled != 0u && texture_attr.warp_uv_mode == TEXTURE_UV_CYCLE_BAND) {
        gradient = iteration_gradient(coord, iteration);
    }
    double warp = warp_offset(iteration, pixel, gradient, anim);
    double inv = g_inv_interval.r;
    double interval = g_interval.r;
    double ratio = biased_ratio(mod(coloring_curve(div_r(smooth_iteration(iteration), interval, inv)), 1));
    double offset = palette_attr.offset - (animate ? div_r(anim, interval, inv) : 0.0LF) + div_r(warp, interval, inv);
    return mod(ratio + offset, 1);
}

float groove_wall_light(uvec2 coord, double center, vec2 baseSlope, bool animate) {
    double cycle = 0.0LF;
    vec2 grad = vec2(0);
    [[dont_unroll]] for (int sampleIndex = 0; sampleIndex < 10; ++sampleIndex) {
        int x = (sampleIndex - 1) % 3 - 1;
        int y = (sampleIndex - 1) / 3 - 1;
        if (sampleIndex != 0 && x == 0 && y == 0) {
            continue;
        }
        ivec2 tap = sampleIndex == 0
            ? ivec2(coord)
            : clamp(ivec2(coord) + ivec2(x, y), ivec2(0), ivec2(iteration_info_attr.extent) - 1);
        double it = sampleIndex == 0 ? center : get_palette_iteration(uvec2(tap));
        if (sampleIndex != 0 && (it <= 0.0 || it >= iteration_info_attr.max_value)) {
            return 0.0;
        }
        double sampled = groove_cycle(tap, it, animate);
        if (sampleIndex == 0) {
            cycle = sampled;
            if (isnan(cycle) || isinf(cycle)) {
                return 0.0;
            }
        } else {
            double delta = sampled - cycle;
            delta -= floor(delta + 0.5LF);
            grad += vec2(
                float(x) * (y == 0 ? 2.0 : 1.0),
                float(y) * (x == 0 ? 2.0 : 1.0)
            ) * float(delta);
        }
    }
    grad *= 0.125 * slope_attr.groove_count;
    float rate = length(grad);
    if (isnan(rate) || isinf(rate) || rate <= 1e-10) {
        return 0.0;
    }
    float scale = max(float(iteration_info_attr.canvas_extent.y) / 720.0, 0.001);
    if (slope_attr.groove_rounded > 0.5) {
        return rounded_band_light(cycle * double(slope_attr.groove_count), grad, rate, baseSlope, scale);
    }
    float width = slope_attr.groove_width / rate;
    if (slope_attr.groove_auto > 0.5) {
        width = min(width, 12.0 * scale);
    }
    float visibility = smoothstep(scale, 3.0 * scale, width);
    float halfWidth = max(width * 0.5, 0.001);
    double phase = cycle * double(slope_attr.groove_count);
    float distance = float(phase - floor(phase + 0.5LF)) / rate;
    vec3 normal = normalize(vec3(-baseSlope, 1));
    vec3 axis = vec3(-grad / rate, 0);
    float az = radians(slope_attr.azimuth);
    float ze = radians(slope_attr.zenith);
    vec3 light = vec3(cos(az) * sin(ze), sin(az) * sin(ze), cos(ze));
    float response = dot(axis - normal * dot(normal, axis), light);
    bool smoothWalls = slope_attr.groove_smooth > 0.5;
    float asymmetry = smoothWalls ? 0.0 : smoothstep(0.04, 0.25, abs(response));
    halfWidth *= mix(1.0, distance * response > 0.0 ? 1.7 : 0.6, asymmetry);
    float x = clamp(distance / halfWidth, -1.0, 1.0);
    float roundness = mix(2.4, 2.0, smoothstep(2.0 * scale, 5.0 * scale, width));
    float depth = slope_attr.groove_depth * scale;
    if (slope_attr.groove_auto > 0.5) {
        depth *= min(width / (4.0 * scale), 1.0);
    }
    float derivative;
    if (smoothWalls) {
        // Height difference across one pixel: the wall slope box-filtered, so a groove a few pixels wide does not alias into a dark line.
        float above = clamp((distance + 0.5) / halfWidth, -1.0, 1.0);
        float below = clamp((distance - 0.5) / halfWidth, -1.0, 1.0);
        derivative = depth * (pow(max(0.0, 1.0 - below * below), roundness) - pow(max(0.0, 1.0 - above * above), roundness));
    } else {
        derivative = depth * 2.0 * roundness * x *
            pow(max(0.0, 1.0 - x * x), roundness - 1.0) / halfWidth;
    }
    float value = 1.2 * derivative * visibility * response / sqrt(1.0 + dot(baseSlope, baseSlope));
    // Depths past 5 open the range up in step, so the deeper groove keeps its profile instead of flattening.
    float limit = 0.45 * max(1.0, slope_attr.groove_depth / 5.0);
    if (smoothWalls) {
        // Saturates smoothly instead of clipping, with the shadow held shallower than the highlight.
        float shadow = 0.35 * max(1.0, slope_attr.groove_depth / 5.0);
        return value < 0.0 ? -shadow * tanh(-value / shadow) : limit * tanh(value / limit);
    }
    return clamp(value, -limit, limit);
}

// The groove light applied to a linear color; Smooth Grooves and Rounded Bands also raise the shadow's saturation so it keeps the band's hue.
vec3 groove_shade(vec3 linearColor, float light) {
    vec3 lit = linearColor * light;
    if ((slope_attr.groove_smooth < 0.5 && slope_attr.groove_rounded < 0.5) || light >= 1.0) {
        return lit;
    }
    float luminance = dot(lit, vec3(0.2126, 0.7152, 0.0722));
    return max(luminance + (lit - luminance) * (1.0 + 0.9 * (1.0 - light)), 0.0);
}

float groove_light(uvec2 coord, vec2 baseSlope) {
    double center = get_palette_iteration(coord);
    if (center <= 0.0 || center >= iteration_info_attr.max_value || isnan(center) || isinf(center)) {
        return 1.0;
    }
    float frozen = freeze_weight(center);
    // Share the animated and frozen stencil without duplicating the warp graph; see NOTICE.
    float samples[2] = float[2](0.0, 0.0);
    [[dont_unroll]] for (int i = 0; i < 2; ++i) {
        if (i == 0 ? frozen < 1.0 : frozen > 0.0) {
            samples[i] = groove_wall_light(coord, center, baseSlope, i == 0);
        }
    }
    float animated = samples[0];
    float held = samples[1];
    return exp(mix(animated, held, frozen));
}
