//
// Modified by GPT-6 on 2026-09-12, 2026-09-13, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-23
//

// Original analytic chrome ribbons and tapered contours, GPL-3.0-only; artistic thin-film provenance: NOTICE.
vec3 chrome_native_cycle(float style, uvec2 coord, float cycle, vec2 cycleGradient, vec3 n, vec3 v, vec2 direction, float ao) {
    if (style > 4.5) {
        return sea_print_surface(style, coord, cycle, cycleGradient, n, direction);
    }
    if (style > 2.5) {
        return dark_surface(style, coord, cycle, cycleGradient, n, direction);
    }
    cycle += slope_attr.surface_phase;
    if (style < 1.5) {
        n.xy *= pow(max(length(n.xy) * 28.0, 0.000001), -0.35);
    }
    n = normalize(vec3(n.xy * 28.0 * slope_attr.reflection_curve, n.z));
    float bend = (dot(direction, vec2(0.43, 0.62)) + length(n.xy) * 0.9) * slope_attr.reflection_detail;
    float wave = sin(6.2831853 * (cycle + 0.8 * bend));
    float ribbon = 0.5 + 0.5 * sin(6.2831853 * (cycle * 2.0 + bend * 1.1));
    float highlight = pow(ribbon, mix(32.0, 3.0, slope_attr.studio_roughness));
    float silver = 0.045 + 0.95 * pow(0.5 + 0.5 * wave, 1.8) + 3.8 * highlight;
    silver *= mix(0.7, 1.0, ao);
    silver = pow(silver, slope_attr.reflection_contrast);
    vec3 metal = vec3(silver) * mix(vec3(0.81, 0.86, 0.96), vec3(1.0, 0.97, 0.91), ribbon);
    float tilt = length(n.xy);
    float detail = smoothstep(0.10, 0.50, tilt);
    float broadReflection = pow(0.5 + 0.5 * sin(6.2831853 * (bend * 2.1 + cycle)), 8.0);
    metal += (0.18 + 0.75 * broadReflection) * (1.0 - detail);
    float cosine = clamp(dot(n, v), 0.0, 1.0);
    float transmitted = sqrt(max(0.0, 1.0 - (1.0 - cosine * cosine) / (1.38 * 1.38)));
    float thickness = slope_attr.surface_film_thickness + 850.0 * (0.5 + 0.5 * sin(6.2831853 * cycle)) + 400.0 * (0.5 + 0.5 * sin(6.2831853 * bend));
    vec3 tint = 0.5 + 0.5 * cos(6.2831853 * (2.0 * 1.38 * thickness * transmitted / vec3(650, 510, 475) + slope_attr.film_hue));
    float film = slope_attr.film_strength * detail * (0.65 + 0.35 * pow(1.0 - ribbon, 2.0));
    if (style > 1.5) {
        film *= 0.16;
    }
    metal *= mix(vec3(1), 0.04 + 3.0 * pow(tint, vec3(2.0)), film);
    if (style < 1.5) {
        float sweep = cycle * 2.0 + bend * 1.7;
        float bright = pow(0.5 + 0.5 * cos(6.2831853 * sweep), 80.0);
        float shoulder = pow(0.5 + 0.5 * cos(6.2831853 * (sweep - 0.11)), 8.0 / slope_attr.prism_width);
        vec3 prism = pow(0.5 + 0.5 * cos(6.2831853 * (sweep * 4.0 + bend * 0.13 + slope_attr.film_hue + vec3(0.0, 0.333333333, 0.666666667))), vec3(1.8));
        float glow = (0.5 + 0.5 * broadReflection) * (1.0 - detail * 0.6);
        float prismMask = clamp(slope_attr.film_strength * 1.4 * shoulder, 0.0, 1.0) * (1.0 - detail * 0.88) * slope_attr.prism_spread;
        metal = mix(metal, metal * (0.04 + 2.0 * prism) + 0.7 * prism * glow, prismMask);
        metal += 6.8 * bright * (1.0 - detail * 0.88);
        float sparkX = sin(6.2831853 * (cycle * max(slope_attr.groove_count, 1.0) + 0.28));
        float directionAngle = direction.x == 0.0 && direction.y == 0.0 ? 0.0 : atan(direction.y, direction.x);
        float sparkY = sin(directionAngle * 3.0 + 0.9);
        float glintArea = slope_attr.style_glint_size * slope_attr.style_glint_size;
        float sparkle = exp((-sparkX * sparkX * 320.0 - sparkY * sparkY * 40.0) / glintArea) + exp((-sparkX * sparkX * 30.0 - sparkY * sparkY * 520.0) / glintArea);
        metal += 8.0 * sparkle * (1.0 - detail) * slope_attr.sparkle_strength;
        metal *= mix(vec3(1), 0.08 + 2.0 * pow(tint, vec3(1.6)), slope_attr.film_strength * detail * 0.40);
    }
    metal *= slope_attr.reflection_brightness;
    float count = max(slope_attr.groove_count, 1.0);
    vec2 gradient = (cycleGradient - floor(cycleGradient + 0.5)) * count;
    float rate = max(length(gradient), 0.00001);
    vec3 base = material_canvas_color(ivec2(coord));
    if (style > 1.5) {
        float white = slope_attr.background_brightness;
        float folds = smoothstep(0.025, 0.38, tilt);
        metal = mix(vec3(white), 1.2 * pow(max(metal * 0.7, vec3(0)), vec3(2.1)), folds);
        vec3 detailContrast = 1.45 * metal * metal / (metal + 0.35);
        metal = mix(metal, detailContrast, smoothstep(0.005, 0.04, rate));
        metal *= mix(vec3(1), vec3(0.98, 0.99, 1.0), 0.4);
    }
    float originalInk = 1.0 - smoothstep(0.018, 0.075, max(base.r, max(base.g, base.b)));
    metal = mix(metal, vec3(0.001), originalInk * slope_attr.style_ink_preserve);
    return metal;
}

// Original editable material composition and density-limited light, GPL-3.0-only; see NOTICE.
vec3 material_palette_color(uvec2 coord) {
    if (!ordered_layers()) {
        return srgb_to_linear(material_canvas_color(ivec2(coord)));
    }
    // Read the palette independently of material passes; original project code, GPL-3.0-only, see NOTICE.
    double iteration = get_palette_iteration(coord);
    vec2 pixel = vec2(coord) + 0.5;
    double anim = animation_offset_iterations(pixel);
    vec2 gradient = vec2(0);
    if (texture_attr.warp_enabled != 0u && texture_attr.warp_uv_mode == TEXTURE_UV_CYCLE_BAND) {
        gradient = iteration_gradient(ivec2(coord), iteration);
    }
    double warp = warp_offset(iteration, pixel, gradient, anim);
    float frozen = freeze_weight(iteration);
    vec3 palette = vec3(0);
    [[dont_unroll]] for (int sampleIndex = 0; sampleIndex < 2; ++sampleIndex) {
        if (sampleIndex == 0 || frozen > 0.0) {
            vec3 sampled = get_color(iteration, sampleIndex == 0, anim, warp).rgb;
            if (sampleIndex == 0) {
                palette = sampled;
            } else {
                palette = mix(palette, sampled, frozen);
            }
        }
    }
    return srgb_to_linear(palette);
}

vec3 chrome_cycle(uvec2 coord, float cycle, vec2 cycleGradient, vec3 n, vec3 v, vec2 direction, float ao, vec3 original, float bandLight, vec3 paletteColor) {
    bool singleMaterial = ordered_layers() && shader_layer.control.x >= 12 && shader_layer.control.x <= 17;
    int first = singleMaterial ? shader_layer.control.x - 11 : 0;
    int last = ordered_layers() ? first : 6;
    float amounts[7] = float[7](1.0, slope_attr.layer_metal, slope_attr.layer_sigil, slope_attr.layer_phonk, slope_attr.layer_frost, slope_attr.layer_sea, slope_attr.layer_print);
    vec3 material = original;
    // Keep material evaluation shared without expanding a copy for every style; see NOTICE.
    [[dont_unroll]] for (int layer = first; layer <= last; ++layer) {
        float style = layer == 0 ? slope_attr.surface_style : float(layer);
        bool materialEnabled = layer == 0 ? style >= 0.5 && (!ordered_layers() || layer_is(11)) : amounts[layer] > 0.0;
        if (singleMaterial || materialEnabled) {
            vec3 nativeMaterial = chrome_native_cycle(style, coord, cycle, cycleGradient, n, v, direction, ao);
            if (singleMaterial) {
                return nativeMaterial;
            }
            if (layer == 0) {
                material = nativeMaterial * bandLight;
            } else if (layer == 5) {
                material += nativeMaterial * amounts[layer] * bandLight;
            } else {
                material = mix(material, nativeMaterial * bandLight, amounts[layer]);
            }
        }
    }
    if (ordered_layers() && !layer_is(18)) {
        return material;
    }
    float peak = max(material.r, max(material.g, material.b));
    vec3 bodyColor = srgb_to_linear(vec3(slope_attr.style_color_r, slope_attr.style_color_g, slope_attr.style_color_b));
    vec3 highlightColor = srgb_to_linear(vec3(slope_attr.style_highlight_color_r, slope_attr.style_highlight_color_g, slope_attr.style_highlight_color_b));
    vec3 backgroundColor = srgb_to_linear(vec3(slope_attr.style_background_color_r, slope_attr.style_background_color_g, slope_attr.style_background_color_b));
    if (slope_attr.style_color_mix > 0.0) {
        material = mix(material, bodyColor * peak, slope_attr.style_color_mix);
    }
    if (!replace_surface_style() && slope_attr.palette_color_mix > 0.0) {
        vec3 palette = paletteColor;
        palette /= max(max(palette.r, max(palette.g, palette.b)), 0.00001);
        material = mix(material, palette * peak, slope_attr.palette_color_mix);
    }
    if (slope_attr.style_highlight_mix > 0.0) {
        material = mix(material, highlightColor * peak, slope_attr.style_highlight_mix * smoothstep(0.2, 1.2, peak));
    }
    float count = max(slope_attr.groove_count, 1.0);
    vec2 gradient = (cycleGradient - floor(cycleGradient + 0.5)) * count;
    float scale = max(float(iteration_info_attr.canvas_extent.y) / 720.0, 0.001);
    float rate = max(length(gradient), 0.00001);
    float density = rate * scale;
    float phase = fract((cycle + slope_attr.surface_phase) * count);
    float distance = min(phase, 1.0 - phase);
    float resolved = 1.0 - smoothstep(0.18, 0.70, rate);
    if (slope_attr.style_background_mix > 0.0) {
        float flatMask = (1.0 - smoothstep(0.025, 0.38, length(normalize(vec3(n.xy * 28.0 * slope_attr.reflection_curve, n.z)).xy))) * (1.0 - smoothstep(0.005, 0.04, density));
        material = mix(material, backgroundColor * slope_attr.background_brightness * bandLight, slope_attr.style_background_mix * flatMask);
    }
    if (slope_attr.style_detail_light != 1.0 || slope_attr.style_detail_suppress > 0.0) {
        float fine = smoothstep(0.005, 0.08, density);
        float crowded = smoothstep(slope_attr.style_detail_threshold, slope_attr.style_detail_threshold * 4.0, density);
        float gain = mix(1.0, slope_attr.style_detail_light, fine) * (1.0 - crowded * slope_attr.style_detail_suppress);
        vec3 dark = min(material, vec3(0.015));
        material = dark + (material - dark) * gain;
    }
    return material;
}

bool surface_layers_enabled() {
    if (ordered_layers()) {
        return false;
    }
    return slope_attr.layer_common > 0.0 || slope_attr.layer_metal > 0.0 || slope_attr.layer_sigil > 0.0 || slope_attr.layer_phonk > 0.0 || slope_attr.layer_frost > 0.0 || slope_attr.layer_sea > 0.0 || slope_attr.layer_print > 0.0;
}

// Original palette-relative Band Line light transfer, GPL-3.0-only; see NOTICE.
float surface_band_light(uvec2 coord, double iteration, float frozen) {
    if (ordered_layers()) {
        return 1.0;
    }
    if (uint(palette_attr.palette.length()) <= palette_attr.size || effects_attr.context.y < 0.5) {
        return 1.0;
    }
    vec2 pixel = vec2(coord) + 0.5;
    double anim = animation_offset_iterations(pixel);
    vec2 gradient = vec2(0);
    if (texture_attr.warp_enabled != 0u && texture_attr.warp_uv_mode == TEXTURE_UV_CYCLE_BAND) {
        gradient = iteration_gradient(ivec2(coord), iteration);
    }
    double warp = warp_offset(iteration, pixel, gradient, anim);
    vec3 samples[2] = vec3[2](vec3(0), vec3(0));
    [[dont_unroll]] for (int sampleIndex = 0; sampleIndex < 4; ++sampleIndex) {
        bool held = sampleIndex >= 2;
        int channel = sampleIndex % 2;
        if (held ? frozen > 0.0 : frozen < 1.0) {
            vec3 sampled = get_color(iteration, !held, anim, warp, channel == 1).rgb;
            samples[channel] += sampled * (held ? frozen : 1.0 - frozen);
        }
    }
    vec3 lined = samples[0];
    vec3 unlined = samples[1];
    float before = max(unlined.r, max(unlined.g, unlined.b));
    float after = max(lined.r, max(lined.g, lined.b));
    if (before <= 0.0 || before == after) {
        return 1.0;
    }
    vec3 light = srgb_to_linear(vec3(max(after, 0.0), before, 0.0));
    return min(light.r / max(light.g, 1e-20), 60000.0);
}

vec3 chrome_surface(uvec2 coord, double iteration, vec3 n, vec3 v, vec2 terrain, vec3 original, float ao,
                    vec2 cycles, vec2 movingGradient, vec2 heldGradient) {
    g_interval = dvec4(palette_attr.interval);
    g_inv_interval = 1.0 / g_interval;
    float frozen = freeze_weight(iteration);
    float bandLight = surface_band_light(coord, iteration, frozen);
    vec3 paletteColor = vec3(0);
    if (!replace_surface_style() && slope_attr.palette_color_mix > 0.0 && (!ordered_layers() || layer_is(18))) {
        paletteColor = material_palette_color(coord);
    }
    vec2 direction = terrain / max(length(terrain), 1e-20);
    float movingCycle = cycles.x;
    float heldCycle = cycles.y;
    vec3 moving = vec3(0);
    vec3 held = vec3(0);
    [[dont_unroll]] for (int sampleIndex = 0; sampleIndex < 2; ++sampleIndex) {
        bool isHeld = sampleIndex == 1;
        if (isHeld ? frozen > 0.0 : frozen < 1.0) {
            vec3 sampled = chrome_cycle(coord, isHeld ? heldCycle : movingCycle, isHeld ? heldGradient : movingGradient, n, v, direction, ao, original, bandLight, paletteColor);
            if (isHeld) {
                held = sampled;
            } else {
                moving = sampled;
            }
        }
    }
    vec3 material = mix(moving, held, frozen);
    if (replace_surface_style() && (!ordered_layers() || layer_is(11))) {
        return material;
    }
    float amount = slope_attr.chrome_strength;
    if (ordered_layers()) {
        if (layer_is(18)) {
            return mix(original, material, amount);
        }
        if (layer_is(12)) {
            amount *= slope_attr.layer_metal;
        }
        if (layer_is(13)) {
            amount *= slope_attr.layer_sigil;
        }
        if (layer_is(14)) {
            amount *= slope_attr.layer_phonk;
        }
        if (layer_is(15)) {
            amount *= slope_attr.layer_frost;
        }
        if (layer_is(16)) {
            return original + material * amount * slope_attr.layer_sea;
        }
        if (layer_is(17)) {
            amount *= slope_attr.layer_print;
        }
    }
    if (slope_attr.surface_blend > 2.5) {
        return original + material * amount - min(original, vec3(1)) * min(material * amount, vec3(1));
    }
    if (slope_attr.surface_blend > 1.5) {
        return original * mix(vec3(1), material, amount);
    }
    if (slope_attr.surface_blend > 0.5) {
        return original + material * amount;
    }
    return mix(original, material, amount);
}
