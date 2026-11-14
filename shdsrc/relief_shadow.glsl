//
// Created by Opus 5.5 on 2026-11-14.
//

// Reference pixels of height per doubling of the iteration count on the classic relief, at Shadow Depth 0.
const float RELIEF_SHADOW_CLASSIC_GAIN = 16.0;

// Interleaved Gradient Noise (Jorge Jimenez, SIGGRAPH 2014) with the constants of chaos_defocus.glsl; see NOTICE.
float relief_shadow_ign(vec2 p) {
    const float ignScale = 52.9829189;
    const float ignX = 0.06711056;
    const float ignY = 0.00583715;
    return fract(ignScale * fract(ignX * p.x + ignY * p.y));
}

// The key light's cast shadow on the relief, 1 lit to 0 shadowed: a march toward the light over the same height the normal comes from; independent code, see NOTICE.
float relief_shadow(uvec2 coord, double center, bool lustre, bool withWaves) {
    float strength = clamp(slope_attr.shadow_strength, 0.0, 1.0);
    if (!(strength > 0.0) || center <= 0.0 || center >= iteration_info_attr.max_value) return 1.0;
    float zRad = radians(slope_attr.zenith);
    float run = sin(zRad);
    // A light straight overhead casts nothing.
    if (abs(run) < 1.0e-4) return 1.0;
    float aRad = radians(slope_attr.azimuth);
    // A zenith past 180 swings the light to the opposite side, as rawShade's sin(zenith) does.
    vec2 toward = vec2(cos(aRad), sin(aRad)) * sign(run);
    float rise = cos(zRad) / abs(run);
    // Lengths are in pixels of a 1280-wide picture, as the other Surface settings are.
    float pixelsPerReference = float(max(iteration_info_attr.canvas_extent.x, 1u)) / 1280.0;
    float gain = exp2(clamp(slope_attr.shadow_depth, -8.0, 8.0));
    double c4 = 1.0, invC4 = 1.0, phaseC = 0.0;
    if (lustre) {
        // Lustre's own linear slope gain, moved from its 800-high reference pixels onto the 1280-wide ones used here.
        float zoomGain = slope_attr.surface_zoom_log2 > 0.0
            ? lustre_compensated_magnitude(1.0, slope_attr.surface_zoom_log2, slope_attr.surface_relief_depth) : 1.0;
        float lustreScale = float(max(iteration_info_attr.canvas_extent.y, 1u)) / 800.0;
        gain *= 68.0 * slope_attr.surface_relief_depth * zoomGain * lustreScale / pixelsPerReference;
        double capped = lustre_capped(max(center, 1.0));
        c4 = capped + 4.0;
        invC4 = 1.0 / c4;
        phaseC = withWaves ? lustre_reduce_phase(capped * double(slope_attr.surface_wave_frequency)) : 0.0;
    } else {
        gain *= RELIEF_SHADOW_CLASSIC_GAIN;
    }
    int taps = clamp(int(slope_attr.shadow_quality + 0.5), 8, 32);
    float reach = clamp(slope_attr.shadow_length, 10.0, 500.0);
    float softness = clamp(slope_attr.shadow_softness, 0.05, 2.0);
    float jitter = relief_shadow_ign(vec2(coord) + 0.5);
    ivec2 extent = ivec2(iteration_info_attr.extent);
    ivec2 previous = ivec2(0);
    float lit = 1.0;
    for (int i = 0; i < taps; ++i) {
        // Taps crowd near the pixel, where a shadow's edge is decided, and thin out toward Shadow Length.
        float along = 1.0 + reach * pow((float(i) + jitter) / float(taps), 1.6);
        ivec2 offset = ivec2(round(toward * (along * pixelsPerReference)));
        if (offset == previous) continue;
        previous = offset;
        ivec2 tap = ivec2(coord) + offset;
        // get_iteration would mirror a tap outside the frame and invent an occluder; the ray only leaves the frame further on.
        if (any(lessThan(tap, ivec2(0))) || any(greaterThanEqual(tap, extent))) break;
        double it = get_iteration_raw(tap);
        if (it <= 0.0 || it >= iteration_info_attr.max_value) continue;
        float s = length(vec2(offset)) / pixelsPerReference;
        float dh;
        if (lustre) {
            vec2 h = lustre_height(it, center, c4, invC4, phaseC, withWaves);
            dh = withWaves ? h.x : h.y;
        } else {
            dh = height_rel(it, center);
        }
        lit = min(lit, clamp((s * rise - gain * dh) / (softness * s) + 0.5, 0.0, 1.0));
        if (lit <= 0.0) break;
    }
    return mix(1.0, lit, strength);
}
