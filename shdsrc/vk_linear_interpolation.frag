//
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-15, 2026-08-19, 2026-08-21, 2026-08-24, 2026-08-31
// Modified by GPT-5 on 2026-08-21, 2026-08-23
// Modified by GPT-6 on 2026-09-10, 2026-09-13, 2026-09-16, 2026-09-19, 2026-09-23, 2026-09-25, 2026-09-30, 2026-10-03
// Modified by Opus 5.5 on 2026-11-08
//

#version 450
#extension GL_GOOGLE_include_directive : require
#include "shader_layer.glsl"

layout(set = 0, binding = 0) uniform sampler2D canvas;
layout(set = 1, binding = 0) uniform LinearInterpolationUBO{
    bool use;
    bool dither;
    bool hdr;
    float exposure;
    float headroom;
    uint tone_map;
    uint transfer;
    float peak_nits;
    float scene_linear;
    float dark_style;
    float dark_strength;
    float dark_vhs;
    float dark_chroma;
    float dark_pixel_mix;
    float dark_pixel_size;
    float dark_grain;
    float dark_scale;
    float dark_monochrome;
    float dark_damage;
    float ukiyo_colors;
    float ukiyo_flatness;
    float print_ink_r;
    float print_ink_g;
    float print_ink_b;
    float print_indigo_r;
    float print_indigo_g;
    float print_indigo_b;
    float print_asagi_r;
    float print_asagi_g;
    float print_asagi_b;
    float print_blue_r;
    float print_blue_g;
    float print_blue_b;
    float print_foam_r;
    float print_foam_g;
    float print_foam_b;
    float print_paper_r;
    float print_paper_g;
    float print_paper_b;
    float layer_quantize;
    float layer_vhs;
    float layer_mono;
    // Appended at the tail to keep existing offsets fixed (must match DescLinearInterpolation reserve order).
    float chromatic_aberration;
    float chromatic_aberration_start;
} linear_interpolation_attr;

layout(location = 0) out vec4 color;

// Rec.709 primaries to Rec.2020, in linear light. Column-major, so each argument is one column.
const mat3 BT709_TO_BT2020 = mat3(
    0.6274039, 0.0690970, 0.0163916,
    0.3292830, 0.9195406, 0.0880132,
    0.0433131, 0.0113624, 0.8955952
);

// IEC 61966-2-1 sRGB EOTF, acknowledged in NOTICE.
vec3 srgb_to_linear(vec3 c) {
    vec3 s = max(c, 0.0);
    return mix(s / 12.92, pow((s + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), s));
}

// Its inverse: the SDR swapchain and the SDR encoder read their bytes as sRGB, not as linear light.
vec3 linear_to_srgb(vec3 c) {
    vec3 s = clamp(c, 0.0, 1.0);
    return mix(s * 12.92, 1.055 * pow(s, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), s));
}

// Interleaved Gradient Noise: one evenly spread value per pixel, standing in for a blue-noise texture.
// Constants from Jorge Jimenez, "Next Generation Post Processing in Call of Duty: Advanced Warfare", SIGGRAPH 2014.
// Limited formula/constant use; no express source-code license established for the cited presentation, see NOTICE.
// Adapted from Alan Wolfe, IGNLDS main.py at a9f3ab9, MIT; GLSL fract and the project seed are retained, see NOTICE.
float ign(vec2 p, float seed) {
    const float ignScale = 52.9829189;
    const float ignX = 0.06711056;
    const float ignY = 0.00583715;
    return fract(ignScale * fract(ignX * p.x + ignY * p.y + seed));
}

// Half a step of noise before the 8-bit rounding, which scatters a slow gradient's crossings into a grain instead of bands.
vec4 apply_dither(vec4 c, ivec2 coord) {
    // A 10-bit HDR frame is finer than this step, so the grain would be louder than the banding it hides.
    if (!linear_interpolation_attr.dither || linear_interpolation_attr.transfer != 0u) {
        return c;
    }
    return vec4(c.rgb + (ign(vec2(coord), 0.5) - 0.5) / 255.0, c.a);
}

// Safe texel fetch that clamps coordinates to texture bounds
vec4 safeTexelFetch(sampler2D tex, ivec2 coord, ivec2 texSize) {
    ivec2 clampedCoord = clamp(coord, ivec2(0), texSize - ivec2(1));
    return texelFetch(tex, clampedCoord, 0);
}

// The canvas sampler is nearest, so sub-pixel offsets are blended here from the four surrounding texels.
vec3 canvas_bilinear(vec2 pixel, ivec2 texSize) {
    vec2 p = pixel - 0.5;
    ivec2 base = ivec2(floor(p));
    vec2 f = p - vec2(base);
    vec3 a = safeTexelFetch(canvas, base, texSize).rgb;
    vec3 b = safeTexelFetch(canvas, base + ivec2(1, 0), texSize).rgb;
    vec3 c = safeTexelFetch(canvas, base + ivec2(0, 1), texSize).rgb;
    vec3 d = safeTexelFetch(canvas, base + ivec2(1, 1), texSize).rgb;
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Original RFF_Super lens fringe, GPL-3.0-only; see NOTICE.
// Returns the change to add: samples along the radius at most 1.5 pixels apart, so red is drawn toward the center and blue toward the edges.
vec3 chromatic_fringe(ivec2 coord, ivec2 texSize) {
    vec2 size = vec2(texSize);
    vec2 center = 0.5 * size;
    vec2 offset = vec2(coord) + 0.5 - center;
    float r = length(offset) / length(center);
    float start = clamp(linear_interpolation_attr.chromatic_aberration_start, 0.0, 0.95);
    float k = linear_interpolation_attr.chromatic_aberration * size.y / 1080.0 * smoothstep(start, 1.0, r) / length(center);
    if (k <= 0.0) {
        return vec3(0.0);
    }
    int count = clamp(int(ceil(2.0 * k * length(offset) / 1.5)) + 1, 9, 65);
    vec3 base = texelFetch(canvas, coord, 0).rgb;
    vec3 sum = vec3(0.0);
    vec3 weight = vec3(0.0);
    for (int i = 0; i < count; i++) {
        float t = float(i) / float(count - 1);
        vec2 at = center + offset * (1.0 + (t - 0.5) * 2.0 * k);
        // A sample past the frame takes the pixel's own color, so the edge row is not smeared into a colored band.
        bool inside = all(greaterThanEqual(at, vec2(0.0))) && all(lessThanEqual(at, size));
        vec3 w = max(vec3(0.0), 1.0 - abs(vec3(t) - vec3(1.0, 0.5, 0.0)) * 1.6);
        sum += (inside ? canvas_bilinear(at, texSize) : base) * w;
        weight += w;
    }
    return sum / weight - base;
}

// Krzysztof Narkowicz's 2015 curve fit to the ACES RRT+ODT, not the ACES transform itself.
// The author's 2016 publication offers CC0 or MIT; this project selects CC0 for the fit, see NOTICE.
vec3 aces_curve(vec3 x) {
    return (x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14);
}

// John Hable's Uncharted 2 filmic curve, presented at GDC 2010.
// Published curve/constant reference; no express source-code license established for the cited article, see NOTICE.
// Adapted from Sascha Willems, Vulkan-glTF-PBR at 128db90, Copyright (c) 2018-2024, MIT; see NOTICE.
vec3 filmic_curve(vec3 x) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Every curve is normalized so the headroom value lands exactly on display white.
vec3 tone_map(vec3 c, float w) {
    switch (linear_interpolation_attr.tone_map) {
        // Extended Reinhard: eq. 4 of Reinhard, Stark, Shirley & Ferwerda, "Photographic Tone Reproduction for Digital Images", SIGGRAPH 2002.
        case 1u: return c * (1.0 + c / (w * w)) / (1.0 + c);
        case 2u: return aces_curve(c) / aces_curve(vec3(w));
        case 3u: return filmic_curve(c) / filmic_curve(vec3(w));
        default: return c / w;
    }
}

// SMPTE ST 2084 inverse EOTF: absolute brightness in nits to the stored code value.
vec3 pq_encode(vec3 nits) {
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    vec3 y = pow(clamp(nits / 10000.0, vec3(0.0), vec3(1.0)), vec3(m1));
    return pow((c1 + c2 * y) / (1.0 + c3 * y), vec3(m2));
}

// ARIB STD-B67 OETF over scene light already normalized so 1.0 is the headroom.
vec3 hlg_encode(vec3 e) {
    const float a = 0.17883277;
    const float b = 0.28466892;
    const float c = 0.55991073;
    e = clamp(e, vec3(0.0), vec3(1.0));
    vec3 low = sqrt(3.0 * e);
    vec3 high = a * log(max(12.0 * e - b, vec3(1e-6))) + c;
    return mix(high, low, step(e, vec3(1.0 / 12.0)));
}

// The one place the graded picture becomes something a display or an encoder can carry.
vec3 output_transform(vec3 c) {
    if (linear_interpolation_attr.scene_linear > 0.5) {
        c = max(c, 0.0) * exp2(clamp(linear_interpolation_attr.exposure, -32.0, 32.0));
        float w = max(linear_interpolation_attr.headroom, 1e-3);
        if (linear_interpolation_attr.transfer == 1u) {
            return pq_encode(BT709_TO_BT2020 * c / w * linear_interpolation_attr.peak_nits);
        }
        if (linear_interpolation_attr.transfer == 2u) {
            return hlg_encode(BT709_TO_BT2020 * c / w);
        }
        if (linear_interpolation_attr.hdr) return linear_to_srgb(tone_map(c, w));
        // A single peak shoulder for Studio SDR preserves chromatic ratios and leaves shadows unchanged.
        float peak = max(c.r, max(c.g, c.b));
        if (peak > 0.75) {
            float excess = peak - 0.75;
            c *= (0.75 + 0.25 * excess / (0.25 + excess)) / peak;
        }
        return linear_to_srgb(c);
    }
    if (!linear_interpolation_attr.hdr) {
        return clamp(c, vec3(0.0), vec3(1.0));
    }
    float w = max(linear_interpolation_attr.headroom, 1e-3);
    // The bloom pass stored its sum as a fraction of the headroom, which is undone here: past this
    // line the value is scene light again, with the headroom as the level that reads as white.
    c = srgb_to_linear(c) * w * exp2(linear_interpolation_attr.exposure);
    if (linear_interpolation_attr.transfer == 1u) {
        return pq_encode(BT709_TO_BT2020 * c / w * linear_interpolation_attr.peak_nits);
    }
    if (linear_interpolation_attr.transfer == 2u) {
        return hlg_encode(BT709_TO_BT2020 * c / w);
    }
    return linear_to_srgb(tone_map(c, w));
}

#include "mfr_display.glsl"
#include "dark_finish.glsl"
#include "print_finish.glsl"

void main() {

    ivec2 coord = ivec2(gl_FragCoord.xy);
    ivec2 texSize = textureSize(canvas, 0);

    vec4 c;
    if (linear_interpolation_attr.use && (!ordered_layers() || layer_is(101))) {
        vec4 c1 = safeTexelFetch(canvas, coord + ivec2(1, 0), texSize);
        vec4 c2 = safeTexelFetch(canvas, coord + ivec2(0, 1), texSize);
        vec4 c3 = safeTexelFetch(canvas, coord + ivec2(-1, 0), texSize);
        vec4 c4 = safeTexelFetch(canvas, coord + ivec2(0, -1), texSize);
        vec4 c5 = safeTexelFetch(canvas, coord + ivec2(1, 1), texSize);
        vec4 c6 = safeTexelFetch(canvas, coord + ivec2(1, -1), texSize);
        vec4 c7 = safeTexelFetch(canvas, coord + ivec2(-1, 1), texSize);
        vec4 c8 = safeTexelFetch(canvas, coord + ivec2(-1, -1), texSize);
        vec4 center = texelFetch(canvas, coord, 0) * 9;
        vec4 near = (c1 + c2 + c3 + c4) * 3;
        vec4 diagonal = (c5 + c6 + c7 + c8);
        c = (center + near + diagonal) / 25;
    } else {
        c = texelFetch(canvas, coord, 0);
    }

    // With Layer Order on, the fringe goes with the Tone Map layer, which sits where the unordered path applies it.
    bool fringe = linear_interpolation_attr.chromatic_aberration > 0.0 && (!ordered_layers() || layer_is(26));
    if (fringe) {
        c.rgb += chromatic_fringe(coord, texSize);
    }

    if (ordered_layers()) {
        if (layer_is(26)) {
            if (linear_interpolation_attr.transfer != 0u) {
                c.rgb *= exp2(clamp(linear_interpolation_attr.exposure, -32.0, 32.0));
            } else if (linear_interpolation_attr.hdr && linear_interpolation_attr.tone_map >= 4u && linear_interpolation_attr.tone_map <= 7u) {
                c.rgb = srgb_to_linear(mfr_display(c.rgb, linear_interpolation_attr.tone_map, linear_interpolation_attr.exposure, linear_interpolation_attr.peak_nits));
            } else if (linear_interpolation_attr.hdr || shader_layer.control.y != 0) {
                c.rgb = srgb_to_linear(output_transform(c.rgb));
            }
        } else if (layer_is(27)) {
            c.rgb = srgb_to_linear(print_finish(linear_to_srgb(c.rgb)));
        } else if (layer_is(28)) {
            c.rgb = dark_sample(c.rgb, coord, texSize);
            c.rgb = srgb_to_linear(dark_finish_layer(linear_to_srgb(c.rgb), coord, 3.0, linear_interpolation_attr.layer_vhs * linear_interpolation_attr.dark_strength));
        } else if (layer_is(29)) {
            c.rgb = srgb_to_linear(dark_finish_layer(linear_to_srgb(c.rgb), coord, 4.0, linear_interpolation_attr.layer_mono * linear_interpolation_attr.dark_strength));
        } else if (layer_is(101)) {
            if (linear_interpolation_attr.transfer == 1u) {
                c.rgb = pq_encode(BT709_TO_BT2020 * max(c.rgb, 0.0) / max(linear_interpolation_attr.headroom, 1e-3) * linear_interpolation_attr.peak_nits);
            } else if (linear_interpolation_attr.transfer == 2u) {
                c.rgb = hlg_encode(BT709_TO_BT2020 * max(c.rgb, 0.0) / max(linear_interpolation_attr.headroom, 1e-3));
            } else {
                c.rgb = linear_to_srgb(c.rgb);
            }
            color = linear_interpolation_attr.hdr && linear_interpolation_attr.tone_map == 7u && linear_interpolation_attr.transfer == 0u
                ? clamp(c, 0.0, 1.0)
                : clamp(apply_dither(c, coord), 0.0, 1.0);
            return;
        }
        color = vec4(clamp(c.rgb, 0.0, 60000.0), c.a);
        return;
    }
    if (linear_interpolation_attr.hdr && linear_interpolation_attr.transfer == 0u && linear_interpolation_attr.tone_map == 7u) {
        color = vec4(mfr_display(c.rgb, linear_interpolation_attr.tone_map, linear_interpolation_attr.exposure, linear_interpolation_attr.peak_nits), c.a);
        return;
    }
    c.rgb = dark_sample(c.rgb, coord, texSize);
    // Original finish routing for MFR SDR, GPL-3.0-only; see NOTICE.
    vec3 display = linear_interpolation_attr.hdr && linear_interpolation_attr.transfer == 0u && linear_interpolation_attr.tone_map >= 4u && linear_interpolation_attr.tone_map <= 6u
        ? mfr_display(c.rgb, linear_interpolation_attr.tone_map, linear_interpolation_attr.exposure, linear_interpolation_attr.peak_nits)
        : output_transform(c.rgb);
    vec3 finished = dark_finish(display, coord);
    color = clamp(apply_dither(vec4(finished, c.a), coord), 0.0, 1.0);
    color.rgb = print_finish(color.rgb);
}
