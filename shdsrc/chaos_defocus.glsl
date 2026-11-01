//
// Modified by Opus 5.5 on 2026-10-09
//

// Moved from vk_fog.frag on 2026-10-09 (see its header for this code's earlier history) to share with vk_chaos_irregularity.comp; the includer declares fog_attr, iteration_info_attr and chaos_cache first.
#ifndef RFF_CHAOS_DEFOCUS
#define RFF_CHAOS_DEFOCUS

// Interleaved Gradient Noise (Jorge Jimenez, SIGGRAPH 2014), same as vk_linear_interpolation.frag; no code license, see NOTICE.
// Adapted from Alan Wolfe, IGNLDS main.py at a9f3ab9, MIT; GLSL fract and the project seed are retained, see NOTICE.
float ign(vec2 p, float seed) {
    const float ignScale = 52.9829189;
    const float ignX = 0.06711056;
    const float ignY = 0.00583715;
    return fract(ignScale * fract(ignX * p.x + ignY * p.y + seed));
}

// Golden-angle spiral from H. Vogel, Math. Biosci. 44 (1979); formula only, written here, no code copied, see NOTICE.
// The half-offset is the midpoint in squared radius of each equal-area annulus; Vogel supplies the angular arrangement, see NOTICE.
vec2 vogel_disc(int i, int n, float rotation) {
    float angle = float(i) * 2.39996323 + rotation;
    return sqrt((float(i) + 0.5) / float(n)) * vec2(cos(angle), sin(angle));
}

// The pixel's chaos_irregularity() as the compute pass stored it; its own inset clamp makes the edge clamp here exact.
float chaos_cached(ivec2 p) {
    ivec2 q = clamp(p, ivec2(0), ivec2(iteration_info_attr.extent) - 1);
    return chaos_cache.irregularity[q.y * int(iteration_info_attr.extent.x) + q.x];
}

// The defocus at a pixel centre: fragCoord is gl_FragCoord.xy in the fog pass and the invocation's pixel plus 0.5 in the compute pass.
// featherOverride replaces Chaos Feather in pixels when it is not negative; Chaos Edge Protection's per-tap values use a narrow one.
float chaos_defocus_at(vec2 fragCoord, int featherOverride) {
    if (fog_attr.chaos_amount <= 0.0) return 0.0;
    float scale = float(iteration_info_attr.canvas_extent.x) / 1280.0;
    int radius = max(int(fog_attr.chaos_scale * scale + 0.5), 1);
    int feather = featherOverride >= 0 ? featherOverride : max(int(fog_attr.chaos_feather * scale + 0.5), 0);
    float complexity = 0.0;
    if (feather == 0) {
        float tap = chaos_cached(ivec2(fragCoord));
        complexity = tap >= 0.0 ? tap : fog_attr.chaos_interior;
    } else {
        // A round Gaussian footprint; the old 3x3 grid stamped each edge nine times and squared the holes off.
        int taps = fog_attr.blur_quality > 0.5 ? 16 : 10;
        float rotation = ign(fragCoord, 0.5) * 6.28318531;
        float total = 0.0;
        [[dont_unroll]] for (int i = 0; i < taps; ++i) {
            vec2 offset = vogel_disc(i, taps, rotation);
            ivec2 p = clamp(ivec2(fragCoord + offset * float(feather)),
                            ivec2(0), ivec2(iteration_info_attr.extent) - 1);
            // Interior taps are stored as -1: Chaos Interior stands in for them, and at 0 they are skipped so black areas do not dilute the selection around them.
            float tap = chaos_cached(p);
            if (tap < 0.0 && fog_attr.chaos_interior > 0.0) tap = fog_attr.chaos_interior;
            float weight = tap >= 0.0 ? exp(-2.0 * dot(offset, offset)) : 0.0;
            complexity += max(tap, 0.0) * weight;
            total += weight;
        }
        complexity = total > 0.0 ? complexity / total : 0.0;
    }
    float depth = smoothstep(fog_attr.chaos_threshold,
                            fog_attr.chaos_threshold + max(fog_attr.chaos_transition, 0.01), complexity);
    return depth * fog_attr.chaos_amount;
}

#endif
