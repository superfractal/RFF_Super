//
// Modified by GPT-6 on 2026-09-13, 2026-09-16, 2026-09-19, 2026-09-20, 2026-09-23
//

// Original RFF_Super dark chrome and frost fields, GPL-3.0-only; noise implementation provenance: NOTICE.
vec3 dark_surface(float style, uvec2 coord, float cycle, vec2 cycleGradient, vec3 n, vec2 direction) {
    cycle += slope_attr.surface_phase;
    vec2 gradient = cycleGradient - floor(cycleGradient + 0.5);
    float count = max(round(slope_attr.groove_count), 1.0);
    gradient *= count;
    float rate = max(length(gradient), 0.00001);
    float scale = max(float(iteration_info_attr.canvas_extent.y) / 720.0, 0.001);
    vec2 p = (vec2(coord) + vec2(iteration_info_attr.canvas_offset)) / scale;
    vec2 boundary = min(p, (vec2(iteration_info_attr.canvas_extent) - 1.0) / scale - p);
    float stencilFade = smoothstep(0.0, 2.0, min(boundary.x, boundary.y));
    vec2 dirtCoord = p / max(slope_attr.grunge_scale, 0.25);
    float dirt = effect_noise(dirtCoord * 0.32, 27.0);
    float fineDirt = effect_noise(dirtCoord * 1.2, 71.0);
    float angle = direction.x == 0.0 && direction.y == 0.0 ? 0.0 : atan(direction.y, direction.x);
    n = normalize(vec3(n.xy * 28.0 * slope_attr.reflection_curve, n.z));
    float bend = (dot(direction, vec2(0.43, 0.62)) + length(n.xy) * 0.9) * slope_attr.reflection_detail;
    float reflection = 0.5 + 0.5 * sin(6.2831853 * (cycle * 2.0 + bend * 1.1));
    float sharp = mix(240.0, 60.0, slope_attr.studio_roughness);
    float highlight = pow(reflection, sharp);
    vec3 base = material_canvas_color(ivec2(coord));
    float peak = max(base.r, max(base.g, base.b));
    vec3 neighbor = vec3(0);
    ivec2 extent = ivec2(iteration_info_attr.extent);
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            neighbor += material_canvas_color(
                clamp(ivec2(coord) + ivec2(x, y), ivec2(0), extent - 1)
            ) / 9.0;
        }
    }
    float localEdge = length(base - neighbor);
    float fineMask = smoothstep(0.02, 0.22, rate);
    float lace = max(
        smoothstep(0.04, 0.36, localEdge),
        smoothstep(0.38, 0.85, peak) * fineMask
    );
    float signedPhase = fract(cycle * count + 0.5) - 0.5;
    float contourDistance = signedPhase / max(rate * scale, 0.00001);
    float cloud = effect_noise(dirtCoord * 0.10, 107.0) * 0.50
        + effect_noise(dirtCoord * 0.33, 139.0) * 0.30
        + effect_noise(dirtCoord * 0.9, 149.0) * 0.20;
    float vein = 1.0 - smoothstep(0.003, 0.028, abs(cloud - 0.51));
    vein *= smoothstep(0.28, 0.68, effect_noise(dirtCoord * 0.028, 157.0));
    float halo = exp(-abs(contourDistance) / 35.0) * (1.0 - smoothstep(0.06, 0.25, rate));
    if (style < 3.5) {
        highlight *= mix(
            1.0,
            0.12 + 0.88 * max(fineMask, pow(0.5 + 0.5 * cos(angle * 3.0 + 0.9), 60.0)),
            slope_attr.style_damage
        );
        float crush = mix(1.3, 6.0, slope_attr.shadow_crush) * slope_attr.reflection_contrast;
        float body = pow(reflection, crush) * 0.65;
        vec3 purple = vec3(0.13, 0.001, 0.23);
        vec3 red = vec3(0.95, 0.002, 0.012);
        vec3 tint = mix(purple, red, slope_attr.phonk_red * (0.4 + 0.6 * reflection));
        vec3 chrome = tint * body + vec3(0.006, 0.0002, 0.012) * (1.0 - slope_attr.shadow_crush);
        chrome += purple * 0.18 * pow(0.5 + 0.5 * cos(6.2831853 * (cycle + bend * 2.0)), 3.0);
        chrome += red * pow(reflection, 12.0) * (0.15 + 0.6 * fineMask);
        chrome += (
            vec3(1.0, 0.85, 0.90) * highlight * 1.1
            + vec3(1.4, 0.006, 0.012) * lace
            + vec3(1.2) * pow(lace, 8.0)
        ) * slope_attr.flame_strength;
        chrome *= 0.45 + 0.9 * dirt;
        float damage = slope_attr.style_damage;
        float scars = smoothstep(0.50, 0.73, cloud) * (0.3 + 1.8 * halo);
        chrome *= mix(1.0, 0.25 + 1.1 * smoothstep(0.22, 0.7, cloud), damage);
        chrome += red * (scars * (0.12 + 0.7 * dirt) + vein * halo * 1.4) * damage;
        chrome += vec3(1.0, 0.65, 0.7) * highlight * pow(fineDirt, 6.0) * damage * 7.0;
        return chrome * slope_attr.reflection_brightness * stencilFade;
    }
    float threshold = slope_attr.frost_threshold;
    float frost = smoothstep(threshold * 0.65, 0.35 + threshold * 0.65, lace);
    float whites = frost * (0.12 + 0.65 * dirt);
    float body = pow(reflection, 8.0) * (1.0 - slope_attr.shadow_crush) * 0.035;
    return vec3((body + whites * slope_attr.frost_strength * 1.2) * slope_attr.reflection_brightness * stencilFade);
}
