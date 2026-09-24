//
// Modified by GPT-6 on 2026-09-13, 2026-09-16, 2026-09-19, 2026-09-20, 2026-09-23
//

// Original RFF_Super luminous-organ and woodcut fields, GPL-3.0-only; existing noise provenance: NOTICE.
vec3 sea_print_surface(
    float style,
    uvec2 coord,
    float cycle,
    vec2 cycleGradient,
    vec3 n,
    vec2 direction
) {
    cycle += slope_attr.surface_phase;
    float count = max(slope_attr.groove_count, 1.0);
    vec2 gradient = (cycleGradient - floor(cycleGradient + 0.5)) * count;
    float rate = max(length(gradient), 0.00001);
    float scale = max(float(iteration_info_attr.canvas_extent.y) / 720.0, 0.001);
    float density = rate * scale;
    float phase = fract(cycle * count);
    float distance = min(phase, 1.0 - phase);
    vec2 p = (vec2(coord) + vec2(iteration_info_attr.canvas_offset)) / scale;
    vec2 boundary = min(
        p,
        (vec2(iteration_info_attr.canvas_extent) - 1.0) / scale - p
    );
    float edgeGuard = smoothstep(0.0, 2.0, min(boundary.x, boundary.y));
    vec3 base = material_canvas_color(ivec2(coord));
    vec3 neighbor = vec3(0);
    ivec2 extent = textureSize(canvas, 0);
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            neighbor += material_canvas_color(
                clamp(ivec2(coord) + ivec2(x, y), ivec2(0), extent - 1)
            ) / 9.0;
        }
    }
    float peak = max(base.r, max(base.g, base.b));
    float fine = smoothstep(0.015, 0.18, density);
    float lace = max(
        smoothstep(0.04, 0.32, length(base - neighbor)),
        smoothstep(0.40, 0.86, peak) * fine
    );
    vec3 normal = normalize(vec3(n.xy * 28.0 * slope_attr.reflection_curve, n.z));
    float folds = smoothstep(0.025, 0.38, length(normal.xy));
    float bend = (dot(direction, vec2(0.43, 0.62)) + length(normal.xy) * 0.9) * slope_attr.reflection_detail;
    float ribbon = 0.5 + 0.5 * sin(6.2831853 * (cycle * 2.0 + bend * 1.1));
    float originalInk = 1.0 - smoothstep(0.018, 0.075, peak);
    if (style < 5.5) {
        vec3 bodyColor = srgb_to_linear(vec3(
            slope_attr.sea_body_color_r,
            slope_attr.sea_body_color_g,
            slope_attr.sea_body_color_b
        ));
        vec3 glowA = srgb_to_linear(vec3(
            slope_attr.sea_glow_color_r,
            slope_attr.sea_glow_color_g,
            slope_attr.sea_glow_color_b
        ));
        vec3 glowB = srgb_to_linear(vec3(
            slope_attr.sea_accent_color_r,
            slope_attr.sea_accent_color_g,
            slope_attr.sea_accent_color_b
        ));
        float organ = pow(
            smoothstep(slope_attr.sea_threshold * 0.7, 0.65 + slope_attr.sea_threshold * 0.35, lace),
            1.5
        );
        organ *= 0.25 + 0.75 * smoothstep(0.04, 0.24, length(base - neighbor));
        float balance = clamp(
            slope_attr.sea_balance + 0.45 * sin(6.2831853 * (cycle * 3.0 + bend)),
            0.0,
            1.0
        );
        vec3 lightColor = mix(glowA, glowB, balance);
        float reflection = pow(ribbon, mix(180.0, 35.0, slope_attr.studio_roughness))
            * (0.1 + 0.9 * folds);
        float halo = exp(-distance / max(density * 24.0, 0.00001))
            * (1.0 - smoothstep(0.04, 0.2, density));
        float particles = 0.0;
        vec2 cell = floor(p / 24.0);
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                ivec2 id = ivec2(cell) + ivec2(x, y);
                float seed = effect_random(id, 263.0);
                vec2 center = (
                    vec2(id) + vec2(effect_random(id, 269.0), effect_random(id, 271.0))
                ) * 24.0;
                float radius = (0.3 + 1.8 * seed) * slope_attr.sea_particle_size;
                float d = length(p - center);
                float dotLight = exp(-d * d / max(radius * radius, 0.0001));
                particles += dotLight * smoothstep(1.0 - slope_attr.sea_particles, 1.001, seed);
            }
        }
        vec3 body = bodyColor * slope_attr.sea_body
            * (0.05 + (0.6 + 5.0 * folds) * pow(ribbon, 3.0))
            * slope_attr.reflection_brightness;
        vec3 glow = (
            lightColor * organ * (0.35 + 0.65 * peak)
            + glowA * reflection * 0.4
            + lightColor * particles * halo * 1.8
        ) * slope_attr.sea_glow;
        return mix(
            body + glow * edgeGuard,
            vec3(0.0001),
            originalInk * slope_attr.style_ink_preserve
        );
    }
    vec3 inks[6] = vec3[6](
        vec3(slope_attr.print_ink_r, slope_attr.print_ink_g, slope_attr.print_ink_b),
        vec3(slope_attr.print_indigo_r, slope_attr.print_indigo_g, slope_attr.print_indigo_b),
        vec3(slope_attr.print_asagi_r, slope_attr.print_asagi_g, slope_attr.print_asagi_b),
        vec3(slope_attr.print_blue_r, slope_attr.print_blue_g, slope_attr.print_blue_b),
        vec3(slope_attr.print_foam_r, slope_attr.print_foam_g, slope_attr.print_foam_b),
        vec3(slope_attr.print_paper_r, slope_attr.print_paper_g, slope_attr.print_paper_b));
    float grain = (effect_noise(p * vec2(0.9, 0.13), 277.0) - 0.5)
        * slope_attr.ukiyo_grain;
    float tone = mix(0.98, 0.14 + 0.64 * ribbon, folds);
    tone = clamp(
        tone + (slope_attr.ukiyo_balance - 0.55) * 0.8 + grain * 0.22,
        0.0,
        1.0
    );
    tone = mix(
        tone,
        1.0,
        slope_attr.ukiyo_foam * smoothstep(0.12, 0.75, lace)
    );
    int levels = clamp(int(round(slope_attr.ukiyo_colors)), 3, 6);
    int band = min(int(floor(tone * float(levels))), levels - 1);
    int index = band == levels - 1 ? 5 : band;
    vec3 flatColor = srgb_to_linear(inks[index]);
    float continuous = tone * 5.0;
    int lower = min(int(floor(continuous)), 4);
    vec3 softColor = srgb_to_linear(
        mix(inks[lower], inks[lower + 1], continuous - float(lower))
    );
    vec3 paper = mix(softColor, flatColor, slope_attr.ukiyo_flatness);
    float ink = originalInk * slope_attr.style_ink_preserve;
    return mix(paper, srgb_to_linear(inks[0]), ink);
}
