//
// Modified by Opus 5 on 2026-08-07, 2026-08-08, 2026-08-12, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-23
// Modified by GPT-5 on 2026-08-16, 2026-08-23
// Modified by GPT-6 on 2026-09-11, 2026-09-16, 2026-09-20, 2026-09-23, 2026-10-03
// Modified by Opus 5.5 on 2026-10-02, 2026-10-03, 2026-10-05, 2026-10-06, 2026-10-08, 2026-10-09
//

#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#include "shader_layer.glsl"

layout (set = 0, binding = 0) uniform sampler2D fog_canvas;
layout (set = 0, binding = 1) uniform sampler2D fog_blurred;

layout (set = 1, binding = 0) uniform FogUBO {
    float radius;
    float opacity;
    float rim_mask;
    float rim_mask_boost;
    float rim_blur;
    // Appended at the tail to keep existing offsets fixed (must match DescFog reserve order).
    float center_start;
    float center_invert;
    // Focus band: the defocus is cut on the iteration count, not on the screen.
    float focus_amount;
    float focus_ratio;
    float focus_range;
    float focus_falloff;
    float focus_blur;
    // 0 = Speed, 1 = Appearance. See blur_radius() and blur_rings() for what each spends.
    float blur_quality;
    float chaos_amount;
    float chaos_scale;
    float chaos_threshold;
    float chaos_transition;
    float chaos_feather;
    float chaos_blur;
    float chaos_shade;
    // 0 = average the aperture in display gamma, 1 = in linear light.
    float chaos_linear;
    float chaos_interior;
    // 1 = Chaos Edge Protection: each aperture tap counts only as far as its own defocus reaches the pixel.
    float chaos_edges;
    // Bokeh shape: 0 = circle, otherwise the blade count; rotation in radians; aspect is height over width.
    float chaos_bokeh_blades;
    float chaos_bokeh_rotation;
    float chaos_bokeh_aspect;
    // Smooth Focus Across Keyframes, and the keyframe depth a video frame is drawn at (0 outside a video).
    float focus_smooth;
    float focus_frame;

} fog_attr;

layout (set = 2, binding = 0) uniform IterUBO {
    uvec2 extent;
    double max_value;
    // Declared only so the tail fields below are reachable; the passes that need neither stop
    // at max_value.
    double max_value_normal;
    double max_value_zoomed;
    // The whole canvas this buffer is a piece of. Equal to extent for an ordinary frame; a
    // tiled export makes it the finished image, which is what the blur radii are measured
    // against - see the multiplier in main().
    uvec2 canvas_extent;
    ivec2 canvas_offset;
} iteration_info_attr;

layout (set = 2, binding = 1) buffer IterSSBO {
    double iterations[];
} iteration_attr;

// chaos_irregularity() at every pixel, filled by vk_chaos_irregularity.comp ahead of this pass.
layout (set = 2, binding = 3) buffer ChaosSSBO {
    float irregularity[];
} chaos_cache;

// Each pixel's defocus with a narrow feather, filled by vk_chaos_irregularity.comp's second stage while Chaos Edge Protection is on.
layout (set = 2, binding = 5) buffer ChaosDefocusSSBO {
    float defocus[];
} chaos_defocus_cache;

// Mirrors vk_slope.frag's block (and DescSlope's reserve order) so the mask can reuse the
// slope pass's own rim shape instead of approximating it.
layout (set = 3, binding = 0) uniform SlopeUBO {
    float depth;
    float reflection_ratio;
    float opacity;
    float zenith;
    float azimuth;
    float specular_intensity;
    float specular_power;
    float rim_intensity;
    float rim_power;
    float brightness;
    float gamma;
    float rim_color_r;
    float rim_color_g;
    float rim_color_b;
    float specular_color_r;
    float specular_color_g;
    float specular_color_b;
    float ao_intensity;
    float ambient_intensity;
    float sky_color_r;
    float sky_color_g;
    float sky_color_b;
    float ground_color_r;
    float ground_color_g;
    float ground_color_b;
    float specular_link;
    float specular_zenith;
    float specular_azimuth;
    float specular_anisotropy;
    float specular_anisotropy_angle;
    float macro_relief;
    float macro_radius;
    float shading_blend;
} slope_attr;


layout (location = 0) out vec4 color;

float grayScale(vec3 c) {
    return c.r * 0.3 + c.g * 0.59 + c.b * 0.11;
}

#include "fog_iteration.glsl"

// One 8-tap Sobel taken at radius r, divided by r so it estimates the same gradient whatever
// the stencil width is. That normalisation is what lets different radii be averaged against
// each other, and what keeps the macro term continuous when r has to shrink near a border.
vec2 macro_sobel(uvec2 iter_coord, int r, double center) {
    // Hoisting the reciprocal out of these taps measured slower on 2026-10-08, so each tap keeps its own division.
    float s_ld = height_rel(get_iteration(iter_coord, ivec2(-r, -r)), center);
    float s_d  = height_rel(get_iteration(iter_coord, ivec2( 0, -r)), center);
    float s_rd = height_rel(get_iteration(iter_coord, ivec2( r, -r)), center);
    float s_l  = height_rel(get_iteration(iter_coord, ivec2(-r,  0)), center);
    float s_r  = height_rel(get_iteration(iter_coord, ivec2( r,  0)), center);
    float s_lu = height_rel(get_iteration(iter_coord, ivec2(-r,  r)), center);
    float s_u  = height_rel(get_iteration(iter_coord, ivec2( 0,  r)), center);
    float s_ru = height_rel(get_iteration(iter_coord, ivec2( r,  r)), center);
    float inv = 1.0 / float(r);
    return vec2(((s_rd - s_ld) + 2.0 * (s_r - s_l) + (s_ru - s_lu)) * inv,
                ((s_lu - s_ld) + 2.0 * (s_u - s_d) + (s_ru - s_rd)) * inv);
}

// Wide-scale gradient for the dual-scale relief. Mirrors the slope pass so the rim mask keeps
// tracking the surface the slope pass actually shades.
vec2 macro_gradient(uvec2 iter_coord, float multiplier, double center) {
    int req = max(int(slope_attr.macro_radius * multiplier + 0.5), 1);

    // Keep every tap inside the image. get_iteration() answers an out-of-bounds tap with the
    // mirrored one, which makes the stencil symmetric about the pixel: the vertical pair
    // collapses to a single value and the diagonal terms cancel, so the macro gradient dies and
    // paints a flat band macro_radius wide along all four borders. Shrinking the radius to fit
    // keeps every tap real instead, and the 1/r above keeps the scale continuous as it shrinks.
    int limX = min(int(iter_coord.x), int(iteration_info_attr.extent.x) - 1 - int(iter_coord.x));
    int limY = min(int(iter_coord.y), int(iteration_info_attr.extent.y) - 1 - int(iter_coord.y));
    int mr = clamp(min(limX, limY), 1, req);

    // Average four radii instead of point-sampling one. A lone ring at radius mr samples the
    // iteration field with stride mr and aliases it, and it re-reads any single bad row or column
    // from mr pixels away, reprinting it as a band. Stacking radii low-passes across scale at a
    // tap count that stays fixed as mr grows. At mr = 1 all four collapse onto the fine Sobel.
    int r0 = mr;
    int r1 = max((mr * 3) / 4, 1);
    int r2 = max(mr / 2, 1);
    int r3 = max(mr / 4, 1);
    // The radii collapse onto each other as mr shrinks - at mr = 1 all four are 1, and every ring
    // near a border lands on a neighbour - so a ring already taken is reused instead of resampled.
    // The four terms and their order are unchanged, and a reused term is the same value the second
    // call would have returned, so the sum is bit for bit what it was.
    int radii[4] = int[4](r0, r1, r2, r3);
    vec2 samples[4];
    [[dont_unroll]] for (int i = 0; i < 4; ++i) {
        int reused = -1;
        [[dont_unroll]] for (int j = 0; j < i; ++j) if (radii[j] == radii[i]) reused = j;
        samples[i] = reused >= 0 ? samples[reused] : macro_sobel(iter_coord, radii[i], center);
    }
    vec2 g = samples[0] + samples[1] + samples[2] + samples[3];
    return g * 0.25;
}

// The rim light's Fresnel footprint, rebuilt here without its intensity factor: the mask marks
// where the rim lands even when the rim light itself is turned down to zero.
float rim_shape(uvec2 iter_coord) {
    // The same clamp the slope pass reads its relief through, so the border carries the rim of the nearest full 3x3 stencil rather than dropping to no rim and leaving a one-pixel frame in the mask.
    iter_coord = uvec2(
        clamp(int(iter_coord.x), 1, max(int(iteration_info_attr.extent.x) - 2, 1)),
        clamp(int(iter_coord.y), 1, max(int(iteration_info_attr.extent.y) - 2, 1))
    );

    double ld = get_iteration(iter_coord, ivec2(-1, -1));
    double d = get_iteration(iter_coord, ivec2(0, -1));
    double rd = get_iteration(iter_coord, ivec2(1, -1));
    double l = get_iteration(iter_coord, ivec2(-1, 0));
    double r = get_iteration(iter_coord, ivec2(1, 0));
    double lu = get_iteration(iter_coord, ivec2(-1, 1));
    double u = get_iteration(iter_coord, ivec2(0, 1));
    double ru = get_iteration(iter_coord, ivec2(1, 1));

    double mv = iteration_info_attr.max_value;
    double centerIter = get_iteration(iter_coord, ivec2(0, 0));
    // Interior pixels are skipped exactly as the rim light skips them, so no halo around black areas.
    bool nearInterior = (centerIter == 0.0 || centerIter >= mv) ||
                        (ld == 0.0 || ld >= mv) || (d == 0.0 || d >= mv) || (rd == 0.0 || rd >= mv) ||
                        (l == 0.0 || l >= mv) || (r == 0.0 || r >= mv) ||
                        (lu == 0.0 || lu >= mv) || (u == 0.0 || u >= mv) || (ru == 0.0 || ru >= mv);
    if (nearInterior) {
        return 0.0;
    }

    float multiplier = float(iteration_info_attr.extent.x) / 1280;
    // Heights relative to the centre pixel; see height_rel() for why the absolute form cannot hold
    // its precision at depth.
    // The centre's clamp and reciprocal, taken once for all eight taps.
    double centerClamped = max(centerIter, 1.0);
    double invCenter = 1.0 / centerClamped;
    float h_ld = height_rel_hoisted(ld, centerClamped, invCenter);
    float h_d  = height_rel_hoisted(d, centerClamped, invCenter);
    float h_rd = height_rel_hoisted(rd, centerClamped, invCenter);
    float h_l  = height_rel_hoisted(l, centerClamped, invCenter);
    float h_r  = height_rel_hoisted(r, centerClamped, invCenter);
    float h_lu = height_rel_hoisted(lu, centerClamped, invCenter);
    float h_u  = height_rel_hoisted(u, centerClamped, invCenter);
    float h_ru = height_rel_hoisted(ru, centerClamped, invCenter);
    float dx = (h_rd - h_ld) + 2.0 * (h_r - h_l) + (h_ru - h_lu);
    float dy = (h_lu - h_ld) + 2.0 * (h_u - h_d) + (h_ru - h_rd);

    if (slope_attr.macro_relief > 0.0) {
        vec2 m = macro_gradient(iter_coord, multiplier, centerIter);
        dx = mix(dx, m.x, slope_attr.macro_relief);
        dy = mix(dy, m.y, slope_attr.macro_relief);
    }

    float rimSlope = length(vec2(dx, dy)) * multiplier;
    float rimFactor = rimSlope / (rimSlope + 1.0);
    // The raw footprint peaks far below 1, so masking with it alone would only dim the fog
    // everywhere. The boost saturates it into a region selector: 1 = raw shape, higher = fuller band.
    float boost = max(fog_attr.rim_mask_boost, 1.0);
    float boostedFactor;
    if (rimFactor < 1.0e-4) {
        float scaled = rimFactor * boost;
        boostedFactor = scaled < 1.0e-4 ? scaled : 1.0 - exp(-scaled);
    } else {
        boostedFactor = 1.0 - pow(1.0 - rimFactor, boost);
    }
    return pow(boostedFactor, slope_attr.rim_power);
}

// The maximum iteration the focus band's ends are fractions of. A video draws each frame from two
// keyframes, and max_value is the larger of their two counts, so with Auto Max Iteration it steps at
// every keyframe switch and the band jumps with it. Smoothed, the reference glides from the zoomed
// keyframe's count to the normal one's across the interval, geometrically because the counts grow by
// factors, and meets the next interval's start exactly. Interior tests keep max_value.
double focus_reference(double mv) {
    if (fog_attr.focus_smooth < 0.5 || fog_attr.focus_frame < 1.0) {
        return mv;
    }
    float zoomed = float(iteration_info_attr.max_value_zoomed);
    float normal = float(iteration_info_attr.max_value_normal);
    if (zoomed <= 0.0 || normal <= 0.0) {
        return mv;
    }
    return double(exp(mix(log(zoomed), log(normal), fract(fog_attr.focus_frame))));
}

// How far out of focus this pixel is, in 0..1. The band is measured on the iteration count rather
// than on the screen, so the sharp part is a depth of the fractal itself and not a place in the
// frame: it holds the same structure as the view zooms, which a screen-space falloff cannot. The
// ends are fractions of the frame's own maximum iteration, so the band does not have to be re-set
// every time the count grows. The interior carries no usable count and is left sharp.
float focus_defocus(uvec2 iter_coord) {
    if (fog_attr.focus_amount <= 0.0) {
        return 0.0;
    }
    double mv = iteration_info_attr.max_value;
    double it = get_iteration(iter_coord, ivec2(0, 0));
    if (it <= 0.0 || it >= mv || mv <= 0.0) {
        return 0.0;
    }
    double ref = focus_reference(mv);
    double focus = double(clamp(fog_attr.focus_ratio, 0.0, 1.0)) * ref;
    double range = double(max(fog_attr.focus_range, 1.0e-4)) * ref;
    // Kept in double up to the ratio: the difference of two counts is exact there, and past about
    // 8.4 million iterations a float cannot hold the counts themselves closely enough to subtract.
    float d = clamp(float(abs(it - focus) / range), 0.0, 1.0);
    return pow(d, max(fog_attr.focus_falloff, 0.01)) * clamp(fog_attr.focus_amount, 0.0, 1.0);
}

// Circular Gaussian over the full-resolution canvas. The shared fog blur is only
// GAUSSIAN_MAX_WIDTH wide, so the band needs its own neighbourhood to stay crisp. A row-plus-column
// kernel would be cheaper but leaves a plaid of shared rows and columns, so this walks a real 2D
// disc: the stride of 2 lands each tap on a texel corner, where the linear sampler folds a 2x2
// block into one fetch and no texel is skipped.
// max_rings caps the tap grid. Past it the taps spread over the same radius instead of multiplying,
// so a wide kernel keeps its true width at any render resolution for a fixed number of fetches.
vec3 local_blur(vec2 coord, float radius, int max_rings) {
    // Held to a radius no frame is wider than: the setting takes any number, and the disc test below
    // squares this one.
    int n = int(min(radius, 4096.0));
    if (n <= 0) {
        return texture(fog_canvas, coord).rgb;
    }
    vec2 texel = 1.0 / vec2(textureSize(fog_canvas, 0));
    float sigma = max(float(n) * 0.5, 0.5);
    float inv2s2 = 1.0 / (2.0 * sigma * sigma);
    vec3 sum = vec3(0.0);
    float weight = 0.0;
    // The taps straddle the fragment at +-0.5, +-2.5, ... rather than running 0, +-2 from it. Both
    // land on texel corners, but a run starting at the fragment puts its first 2x2 block wholly on
    // one side, and since the Gaussian is symmetric about the fragment while the blocks are not,
    // the whole kernel came out displaced half a texel down and to the right. Straddling makes the
    // set symmetric, and the weight is read at the block's own centre so the two agree.
    int rings = int((float(n) - 0.5) * 0.5) + 1;
    // The stride is the tap spacing in texels; 2 is the value the straddling above is written for.
    float stride = 2.0;
    if (rings > max_rings) {
        stride = 2.0 * float(rings) / float(max_rings);
        rings = max_rings;
    }
    [[dont_unroll]] for (int jy = -rings; jy < rings; ++jy) {
        float sy = stride * (float(jy) + (jy < 0 ? 0.75 : 0.25));
        [[dont_unroll]] for (int jx = -rings; jx < rings; ++jx) {
            float sx = stride * (float(jx) + (jx < 0 ? 0.75 : 0.25));
            float d2 = sx * sx + sy * sy;
            if (d2 > float(n * n)) {
                continue;
            }
            float w = exp(-d2 * inv2s2);
            sum += texture(fog_canvas, coord + vec2(sx, sy) * texel).rgb * w;
            weight += w;
        }
    }
    // The innermost ring sits at 0.5 on each axis, so d2 is 0.5 and n >= 1 always keeps it.
    return sum / weight;
}

// Speed keeps the 16-texel ceiling the two blurs have always had. It is a ceiling in texels, not in
// the frame, so at a video's render resolution it lands as a far smaller part of the picture than the
// same setting does in the preview; Appearance spends the radius the setting asks for instead.
float blur_radius(float requested) {
    return fog_attr.blur_quality > 0.5 ? requested : min(requested, 16.0);
}

// The tap budget each mode is allowed. Speed's 8 is what its own 16-texel ceiling already implies, so
// its kernel is unchanged; Appearance pays four times the fetches and spreads them past that.
int blur_rings() {
    return fog_attr.blur_quality > 0.5 ? 16 : 8;
}

#include "chaos_defocus.glsl"

// The stored defocus at a pixel, edge-clamped like chaos_cached().
float chaos_defocus_stored(ivec2 p) {
    ivec2 q = clamp(p, ivec2(0), ivec2(iteration_info_attr.extent) - 1);
    return chaos_defocus_cache.defocus[q.y * int(iteration_info_attr.extent.x) + q.x];
}

// Chaos Edge Protection's gather: chaos_blur() with each tap weighted by how far its own stored defocus reaches.
// Kept apart from chaos_blur() so the Off path compiles exactly as before; floorRadius is the focus or rim radius the gather also serves.
vec3 chaos_blur_protected(vec2 coord, float radius, float floorRadius) {
    vec2 texel = 1.0 / vec2(textureSize(fog_canvas, 0));
    int taps = fog_attr.blur_quality > 0.5 ? 160 : 48;
    float rotation = ign(gl_FragCoord.xy, 0.0) * 6.28318531;
    vec3 sum = vec3(0.0);
    float weight = 0.0;
    bool linearAverage = fog_attr.chaos_linear > 0.5;
    float scale = float(iteration_info_attr.canvas_extent.x) / 1280.0;
    [[dont_unroll]] for (int i = 0; i < taps; ++i) {
        vec2 aperture = vogel_disc(i, taps, rotation);
        float w = 1.0 - smoothstep(0.85, 1.0, length(aperture));
        if (i > 0) {
            // Scatter-as-gather: a tap whose own radius falls short of this pixel drops out, so sharp surfaces do not bleed into the blur; see NOTICE.
            float reach = max(blur_radius(fog_attr.chaos_blur * chaos_defocus_stored(ivec2(gl_FragCoord.xy + aperture * radius)) * scale), floorRadius);
            w *= clamp((reach - length(aperture) * radius) / max(0.25 * radius, 1.0) + 1.0, 0.0, 1.0);
        }
        vec3 tap = texture(fog_canvas, coord + aperture * radius * texel).rgb;
        sum += (linearAverage ? tap : pow(max(tap, vec3(0.0)), vec3(1.0 / 2.2))) * w;
        weight += w;
    }
    return linearAverage ? sum / weight : pow(sum / weight, vec3(2.2));
}

// Maps a point of the unit disc onto the bokeh: a regular polygon with its corners on the unit circle, stretched to the aspect ratio, then turned.
// Area-preserving polar remap written here from the polygon's own geometry: tan(phi) = tan(h) * t / h keeps the taps evenly spread; see NOTICE, Chaos blur.
vec2 bokeh_shape(vec2 disc) {
    float r = length(disc);
    float angle = atan(disc.y, disc.x);
    if (fog_attr.chaos_bokeh_blades >= 3.0) {
        float half_segment = 3.14159265 / fog_attr.chaos_bokeh_blades;
        // Measured from an edge centre straight down, so at 0 degrees every polygon stands on a flat edge.
        float local = mod(angle + 1.57079633 + half_segment, 2.0 * half_segment) - half_segment;
        float phi = atan(tan(half_segment) * local / half_segment);
        angle += phi - local;
        r *= cos(half_segment) / cos(phi);
    }
    float stretch = sqrt(max(fog_attr.chaos_bokeh_aspect, 0.01));
    vec2 p = r * vec2(cos(angle) / stretch, sin(angle) * stretch);
    float c = cos(fog_attr.chaos_bokeh_rotation);
    float s = sin(fog_attr.chaos_bokeh_rotation);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

// The gather for a polygonal or stretched bokeh, with or without Chaos Edge Protection; the circle keeps its own loops so it draws exactly as before.
vec3 chaos_blur_shaped(vec2 coord, float radius, float floorRadius, bool edges) {
    vec2 texel = 1.0 / vec2(textureSize(fog_canvas, 0));
    int taps = fog_attr.blur_quality > 0.5 ? 160 : 48;
    float rotation = ign(gl_FragCoord.xy, 0.0) * 6.28318531;
    vec3 sum = vec3(0.0);
    float weight = 0.0;
    bool linearAverage = fog_attr.chaos_linear > 0.5;
    float scale = float(iteration_info_attr.canvas_extent.x) / 1280.0;
    [[dont_unroll]] for (int i = 0; i < taps; ++i) {
        vec2 disc = vogel_disc(i, taps, rotation);
        // The rim softening follows the disc radius, so every blade edge fades the same way.
        float w = 1.0 - smoothstep(0.85, 1.0, length(disc));
        vec2 aperture = bokeh_shape(disc);
        if (edges && i > 0) {
            float reach = max(blur_radius(fog_attr.chaos_blur * chaos_defocus_stored(ivec2(gl_FragCoord.xy + aperture * radius)) * scale), floorRadius);
            w *= clamp((reach - length(aperture) * radius) / max(0.25 * radius, 1.0) + 1.0, 0.0, 1.0);
        }
        vec3 tap = texture(fog_canvas, coord + aperture * radius * texel).rgb;
        sum += (linearAverage ? tap : pow(max(tap, vec3(0.0)), vec3(1.0 / 2.2))) * w;
        weight += w;
    }
    return linearAverage ? sum / weight : pow(sum / weight, vec3(2.2));
}

// Original RFF_Super artistic circular-aperture gather, GPL-3.0-only; see NOTICE, Chaos blur.
vec3 chaos_blur(vec2 coord, float radius, float floorRadius) {
    if (radius <= 0.0) return texture(fog_canvas, coord).rgb;
    radius = min(radius, 4096.0);
    if (fog_attr.chaos_bokeh_blades >= 3.0 || fog_attr.chaos_bokeh_aspect != 1.0) {
        return chaos_blur_shaped(coord, radius, floorRadius, fog_attr.chaos_edges > 0.5);
    }
    if (fog_attr.chaos_edges > 0.5) return chaos_blur_protected(coord, radius, floorRadius);
    vec2 texel = 1.0 / vec2(textureSize(fog_canvas, 0));
    int taps = fog_attr.blur_quality > 0.5 ? 160 : 48;
    float rotation = ign(gl_FragCoord.xy, 0.0) * 6.28318531;
    vec3 sum = vec3(0.0);
    float weight = 0.0;
    bool linearAverage = fog_attr.chaos_linear > 0.5;
    [[dont_unroll]] for (int i = 0; i < taps; ++i) {
        vec2 aperture = vogel_disc(i, taps, rotation);
        float w = 1.0 - smoothstep(0.85, 1.0, length(aperture));
        vec3 tap = texture(fog_canvas, coord + aperture * radius * texel).rgb;
        // Display gamma keeps bright specks from lifting the dark gaps between them to grey.
        sum += (linearAverage ? tap : pow(max(tap, vec3(0.0)), vec3(1.0 / 2.2))) * w;
        weight += w;
    }
    return linearAverage ? sum / weight : pow(sum / weight, vec3(2.2));
}

vec3 blend_chaos_blur(vec2 coord, float legacyRadius, float chaosRadius) {
    float blend = smoothstep(0.0, max(legacyRadius, 1.0), chaosRadius);
    if (blend >= 1.0) return chaos_blur(coord, max(legacyRadius, chaosRadius), legacyRadius);
    return mix(local_blur(coord, legacyRadius, blur_rings()),
               chaos_blur(coord, max(legacyRadius, chaosRadius), legacyRadius), blend);
}

float chaos_defocus() {
    if (fog_attr.chaos_amount <= 0.0) return 0.0;
    return chaos_defocus_at(gl_FragCoord.xy, -1);
}

void main() {
    if (ordered_layers() && !layer_is(24)) { color = texelFetch(fog_canvas, ivec2(gl_FragCoord.xy), 0); return; }

    vec2 coord = gl_FragCoord.xy / textureSize(fog_canvas, 0);

    float x = coord.x;
    float y = coord.y;

    if (x < 0 || y < 0) {
        discard;
    }
    if (x >= 1 || y >= 1) {
        discard;
    }
    vec3 blurredColor = texture(fog_blurred, coord).rgb;
    color = texture(fog_canvas, coord);
    float chaosDefocus = chaos_defocus();
    float chaosRadius = 0.0;
    if (chaosDefocus > 0.0) {
        float focusRadius = fog_attr.focus_blur * focus_defocus(uvec2(gl_FragCoord.xy));
        chaosRadius = fog_attr.chaos_blur * chaosDefocus;
        float scale = float(iteration_info_attr.canvas_extent.x) / 1280.0;
        color.rgb = blend_chaos_blur(coord, blur_radius(focusRadius * scale), blur_radius(chaosRadius * scale));
    }

    // rim_mask fades the fog toward the rim footprint: 0 keeps the full-frame fog, 1 confines it to the rim.
    float shape = fog_attr.rim_mask > 0.0 ? rim_shape(uvec2(gl_FragCoord.xy)) : 0.0;
    float band = shape * fog_attr.rim_mask;
    float amount = fog_attr.opacity * mix(1.0, shape, fog_attr.rim_mask);

    // center_start holds the fog off until this far out from the frame centre, ramping to full at the
    // frame edge. The ellipse follows the frame, so the falloff looks the same at any aspect ratio.
    // Distance saturates at the edge rather than the corner: normalising to the corner would leave
    // every value past 0.71 identical outside the corner triangles, wasting most of the slider.
    if (fog_attr.center_start > 0.0) {
        float dist = min(length((coord - 0.5) * 2.0), 1.0);
        // smoothstep is undefined when both edges meet, so the ramp keeps a sliver of width.
        float ramp = smoothstep(min(fog_attr.center_start, 0.99), 1.0, dist);
        // Inverted, the haze sits on the deep side instead: solid over the centre and clearing
        // outwards, so center_start becomes the radius of the fully fogged core.
        amount *= fog_attr.center_invert > 0.5 ? 1.0 - ramp : ramp;
    }

    // The band dissolves into its own full-res neighbourhood; only the unmasked haze uses the shared
    // low-resolution blur, which is where that blur is soft enough not to read as a resolution drop.
    // Against the finished image rather than the buffer being rendered. A tiled export draws one
    // tile at a time, and a radius taken from the tile would cover that tile the way the preview's
    // covers the frame - a fraction of that across the image the tiles add up to. The rim shape
    // above keeps the buffer-relative multiplier: it tracks the slope pass, which works in buffer
    // space, and the export's tile overlap is sized from the same number.
    float multiplier = float(iteration_info_attr.canvas_extent.x) / 1280;
    vec3 target = blurredColor;
    if (band > 0.0 && amount > 0.0) {
        if (chaosDefocus > 0.0) {
            target = mix(blurredColor, blend_chaos_blur(coord, blur_radius(fog_attr.rim_blur * multiplier),
                                                      blur_radius(chaosRadius * multiplier)), band);
        } else {
            target = mix(blurredColor, local_blur(coord, blur_radius(fog_attr.rim_blur * multiplier), blur_rings()), band);
        }
    }

    vec3 cf = color.rgb - (color.rgb - target) * amount;

    // Away from the band the fog stays the brighten-only haze it has always been.
    vec3 lifted = grayScale(color.rgb) < grayScale(cf) ? cf : color.rgb;
    // Inside it the blur replaces the pixel in both directions, so the band washes out instead of only glowing.
    // Which of the two applies is rim_mask's business alone. Crossfading on band instead put the
    // rim footprint into the darkening half a second time - amount already carries it - so the same
    // Opacity moved a pixel by shape one way and by shape squared the other.
    color = vec4(mix(lifted, cf, fog_attr.rim_mask), 1);

    // The defocus is laid over the finished fog rather than mixed into it: haze is something in
    // front of the subject and the lens is behind the eye, so the haze belongs under the blur. Its
    // radius carries the strength, which is what makes this read as a lens and not as a fade to a
    // blurred copy - a pixel just outside the band is softened, not half-replaced.
    float defocus = focus_defocus(uvec2(gl_FragCoord.xy));
    if (chaosDefocus > 0.0) {
        // Shade follows the selection alone, so Chaos Amount sets the blur without capping how dark it goes.
        color.rgb *= 1.0 - fog_attr.chaos_shade * clamp(chaosDefocus / fog_attr.chaos_amount, 0.0, 1.0);
    } else if (defocus > 0.0) {
        vec3 source = texture(fog_canvas, coord).rgb;
        vec3 blurred = local_blur(coord, blur_radius(fog_attr.focus_blur * defocus * multiplier), blur_rings());
        // Preserve the finished fog delta while the radius alone defocuses the underlying canvas.
        color = vec4(blurred + (color.rgb - source), 1);
    }
}
