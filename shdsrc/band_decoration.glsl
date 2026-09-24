//
// Modified by GPT-6 on 2026-09-19, 2026-09-20, 2026-09-23
//

// Original RFF_Super sigil coverage adapted independently of material lighting, GPL-3.0-only; see NOTICE.
bool band_decoration_enabled() {
    return shader_layer.control.z != 0 &&
        shader_layer.line_params.y > 0.0 &&
        shader_layer.line_params.z > 0.0 &&
        (shader_layer.line_spines.x > 0.0 ||
         shader_layer.line_ornaments.x > 0.0 ||
         shader_layer.line_glow.x > 0.0) &&
        effects_attr.context.y > 0.5;
}

double decoration_cycle(ivec2 coord, double iteration, int channel, bool animate) {
    vec2 pixel = vec2(coord) + 0.5;
    double anim = animation_offset_iterations(pixel);
    vec2 gradient = vec2(0);
    if (texture_attr.warp_enabled != 0u && texture_attr.warp_uv_mode == TEXTURE_UV_CYCLE_BAND) {
        gradient = iteration_gradient(coord, iteration);
    }
    double warp = warp_offset(iteration, pixel, gradient, anim);
    double interval = g_interval[channel];
    double inverse = g_inv_interval[channel];
    double ratio = biased_ratio(mod(coloring_curve(div_r(smooth_iteration(iteration), interval, inverse)), 1.0LF));
    return mod(ratio + palette_attr.offset + div_r(warp, interval, inverse) -
                   (animate ? div_r(anim, interval, inverse) : 0.0LF), 1.0LF);
}

float band_ornament_coverage(uvec2 coord) {
    if (shader_layer.line_ornaments.x <= 0.0) {
        return 0.0;
    }
    vec2 uv = (vec2(coord) + 0.5 + vec2(iteration_info_attr.canvas_offset)) /
              max(vec2(iteration_info_attr.canvas_extent), vec2(1));
    float aspect = float(iteration_info_attr.canvas_extent.x) / max(float(iteration_info_attr.canvas_extent.y), 1.0);
    float scale = max(float(iteration_info_attr.canvas_extent.y) / 720.0, 0.001);
    float size = shader_layer.line_ornaments.y;
    float inset = shader_layer.line_ornaments.w;
    float stroke = shader_layer.line_params.y / 0.03;
    float aa = 0.65 / (scale * size) + shader_layer.line_params.w * stroke;
    int marks = clamp(int(round(6.0 * shader_layer.line_ornaments.z)), 2, 12);
    float spacing = marks == 6 ? 0.135 : 0.675 / float(marks - 1);
    float ink = 0.0;
    [[dont_unroll]] for (int side = 0; side < 2; ++side) {
        [[dont_unroll]] for (int i = 0; i < marks; ++i) {
            float y = 0.13 + float(i) * spacing + float(side) * 0.031;
            vec2 p = (uv - vec2(side == 0 ? inset : 0.998 - inset, y)) *
                     vec2(aspect, 1.0) * 720.0 / size;
            float reach = 28.0 + 12.0 * cos(float(i) * 2.0);
            float span = 6.0 + 4.0 * sin(float(i) * 1.7);
            float taper = pow(max(0.0, 1.0 - abs(p.y) / reach), 1.6);
            float width = (0.4 + span / pow(1.0 + abs(p.y) * 0.18, 1.45)) * taper * stroke;
            float glyph = (1.0 - smoothstep(max(0.0, width - aa), width + aa, abs(p.x))) *
                          (1.0 - smoothstep(reach - aa, reach + aa, abs(p.y)));
            if (i % 2 == 1) {
                vec2 radius = vec2(8.0, 31.0);
                float rho = length(p / radius);
                float distance = abs(rho - 1.0) /
                                 max(length(p / (radius * radius)) / max(rho, 0.0001), 0.0001);
                glyph = max(glyph, 1.0 - smoothstep(0.30 * stroke, 0.30 * stroke + aa, distance));
            }
            ink = max(ink, glyph);
        }
    }
    float strandDistance = min(abs(uv.x - inset), abs(uv.x - (0.998 - inset))) * aspect * 720.0;
    float strand = (1.0 - smoothstep(0.20 * stroke, 0.20 * stroke + aa * size, strandDistance)) *
                   smoothstep(0.035, 0.10, uv.y) *
                   (1.0 - smoothstep(0.87, 0.96, uv.y));
    return max(ink, strand) * shader_layer.line_ornaments.x;
}

vec3 band_shape_coverage(uvec2 coord, double center, int channel, bool animate, float ornament) {
    double cycle = 0.0LF;
    vec2 grad = vec2(0);
    bool valid = true;
    bool stencil = shader_layer.line_spines.x > 0.0 || shader_layer.line_glow.x > 0.0;
    [[dont_unroll]] for (int sampleIndex = 0; sampleIndex < 10; ++sampleIndex) {
        if (sampleIndex != 0 && !stencil) {
            break;
        }
        int x = (sampleIndex - 1) % 3 - 1;
        int y = (sampleIndex - 1) / 3 - 1;
        if (sampleIndex != 0 && x == 0 && y == 0) {
            continue;
        }
        ivec2 tap = sampleIndex == 0 ? ivec2(coord) :
                    clamp(ivec2(coord) + ivec2(x, y), ivec2(0), ivec2(iteration_info_attr.extent) - 1);
        double it = sampleIndex == 0 ? center : get_palette_iteration(uvec2(tap));
        if (sampleIndex != 0 && (it <= 0.0 || it >= iteration_info_attr.max_value || isnan(it) || isinf(it))) {
            valid = false;
            continue;
        }
        double sampled = decoration_cycle(tap, it, channel, animate);
        if (sampleIndex == 0) {
            cycle = sampled;
            if (isnan(cycle) || isinf(cycle)) {
                return vec3(0);
            }
        } else {
            double delta = sampled - cycle;
            delta -= floor(delta + 0.5LF);
            grad += vec2(float(x) * (y == 0 ? 2.0 : 1.0),
                         float(y) * (x == 0 ? 2.0 : 1.0)) * float(delta);
        }
    }
    float plain = shader_layer.control.w != 0 ? 0.0 : ordered_band_coverage(cycle);
    float shape = ornament;
    float glow = 0.0;
    if (stencil) {
        grad *= 0.125 * shader_layer.line_params.x;
        float rate = length(grad);
        if (valid && rate > 1e-10 && !isnan(rate) && !isinf(rate)) {
            float scale = max(float(iteration_info_attr.canvas_extent.y) / 720.0, 0.001);
            float angle = atan(grad.y, grad.x);
            float phase = float(fract(cycle * double(shader_layer.line_params.x)));
            float distance = min(phase, 1.0 - phase);
            float t = float(cycle);
            glow = exp(-distance / max(rate * scale * shader_layer.line_glow.y, 0.00001)) *
                   (1.0 - smoothstep(0.18, 0.70, rate));
            float cusp = pow(1.0 + 27.0 *
                             abs(sin(angle * round(5.0 * shader_layer.line_spines.z) + t * 6.2831853)), -1.4);
            cusp = max(cusp, 0.65 * pow(1.0 + 38.0 *
                       abs(sin(angle * round(9.0 * shader_layer.line_spines.z) - t * 6.2831853 + 0.8)), -1.5));
            float halfWidth = shader_layer.line_params.y * 0.5;
            float width = halfWidth * (1.0 + 80.0 * cusp * shader_layer.line_spines.y);
            width = min(width, rate * scale * (1.2 + 160.0 * cusp * shader_layer.line_spines.y));
            float aa = max(abs(grad.x) + abs(grad.y), 0.00001) + width * shader_layer.line_params.w;
            float ink = (1.0 - smoothstep(width, width + aa, distance)) * (1.0 - smoothstep(0.2, 0.8, rate));
            float offset = 0.14 * pow(0.5 + 0.5 * cos(angle * 3.0 + t * 6.2831853), 2.0);
            float railWidth = min(halfWidth * 0.4, rate * scale * 0.75);
            float rails = (1.0 - smoothstep(railWidth, railWidth + aa, abs(distance - offset))) * smoothstep(0.006, 0.03, offset);
            rails *= (1.0 - smoothstep(0.035, 0.12, rate)) * shader_layer.line_spines.w;
            shape = max(shape, max(ink, rails) * shader_layer.line_spines.x);
        }
    }
    return vec3(plain, max(plain, shape * shader_layer.line_params.z), glow * shader_layer.line_params.z * shader_layer.line_glow.x);
}

vec3 apply_band_decoration(uvec2 coord, vec3 inputColor, bool linearInput) {
    if (!band_decoration_enabled()) {
        return inputColor;
    }
    double center = get_palette_iteration(coord);
    if (center <= 0.0 || center >= iteration_info_attr.max_value || isnan(center) || isinf(center)) {
        return inputColor;
    }
    g_interval = dvec4(palette_attr.interval);
    g_inv_interval = 1.0 / g_interval;
    float frozen = freeze_weight(center);
    float ornament = band_ornament_coverage(coord);
    vec3 coverage = vec3(0);
    vec3 glow = vec3(0);
    [[dont_unroll]] for (int channel = 0; channel < 3; ++channel) {
        vec3 alpha = vec3(0);
        [[dont_unroll]] for (int sampleIndex = 0; sampleIndex < 2; ++sampleIndex) {
            float weight = sampleIndex == 0 ? 1.0 - frozen : frozen;
            if (weight > 0.0) {
                alpha += band_shape_coverage(coord, center, channel, sampleIndex == 0, ornament) * weight;
            }
        }
        glow[channel] = alpha.z;
        coverage[channel] = clamp((alpha.y - alpha.x) / max(1.0 - alpha.x, 0.00001), 0.0, 1.0);
    }
    vec3 encoded = linearInput ? linear_to_srgb(inputColor) : inputColor;
    encoded = mix(encoded, shader_layer.line_color.rgb, coverage);
    if (shader_layer.line_glow.x <= 0.0) {
        return linearInput ? srgb_to_linear(encoded) : encoded;
    }
    vec3 lit = srgb_to_linear(encoded) + srgb_to_linear(shader_layer.line_glow_color.rgb) * glow;
    return linearInput ? lit : linear_to_srgb(lit);
}
