//
// Created by Opus 5.5 on 2026-10-08
//

// Environment Source for Studio reflections; original GPL-3.0-only code, the source/spread split follows a survey of GMT-fractals (see NOTICE).
vec3 studio_environment_image(int layer, vec3 ray, float spread);

vec3 studio_environment_gradient_at(float t, vec3 c1, vec3 c2, vec3 c3, vec3 c4) {
    vec4 p = vec4(slope_attr.environment_stop_1_position, slope_attr.environment_stop_2_position,
                  slope_attr.environment_stop_3_position, slope_attr.environment_stop_4_position);
    vec3 color = mix(c1, c2, clamp((t - p.x) / max(p.y - p.x, 1e-4), 0.0, 1.0));
    color = mix(color, c3, clamp((t - p.y) / max(p.z - p.y, 1e-4), 0.0, 1.0));
    return mix(color, c4, clamp((t - p.z) / max(p.w - p.z, 1e-4), 0.0, 1.0));
}

// The gradient runs from straight down (0) to straight up (1); a 1-4-6-4-1 blur over the spread stands in for roughness, not a GGX convolution.
vec3 studio_environment_gradient(float t, float spread) {
    vec3 c1 = srgb_to_linear(vec3(slope_attr.environment_stop_1_r, slope_attr.environment_stop_1_g, slope_attr.environment_stop_1_b));
    vec3 c2 = srgb_to_linear(vec3(slope_attr.environment_stop_2_r, slope_attr.environment_stop_2_g, slope_attr.environment_stop_2_b));
    vec3 c3 = srgb_to_linear(vec3(slope_attr.environment_stop_3_r, slope_attr.environment_stop_3_g, slope_attr.environment_stop_3_b));
    vec3 c4 = srgb_to_linear(vec3(slope_attr.environment_stop_4_r, slope_attr.environment_stop_4_g, slope_attr.environment_stop_4_b));
    const float offsets[5] = float[](-1.0, -0.5, 0.0, 0.5, 1.0);
    const float weights[5] = float[](1.0, 4.0, 6.0, 4.0, 1.0);
    vec3 sum = vec3(0);
    for (int i = 0; i < 5; ++i) {
        sum += weights[i] * studio_environment_gradient_at(t + offsets[i] * spread, c1, c2, c3, c4);
    }
    return sum / 16.0;
}

// Two-Color keeps the original light; the others blur over the reflected lobe (about 2 alpha radians) or the pixel's footprint, whichever is wider.
vec3 studio_environment_source(vec3 ray, float angle, vec3 sky, vec3 ground, float alpha, float footprint) {
    int source = int(slope_attr.studio_environment_source + 0.5);
    if (source == 0) {
        return studio_environment(ray, angle, sky, ground);
    }
    ray = studio_environment_rotate(ray, angle);
    float spread = max(2.0 * alpha, footprint);
    if (source == 1) {
        return studio_environment_gradient(0.5 + 0.5 * ray.y, 0.5 * spread) * slope_attr.environment_brightness;
    }
    return studio_environment_image(source - 2, ray, spread) * slope_attr.environment_brightness;
}
