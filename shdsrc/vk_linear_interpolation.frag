//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23.
// Modified by Opus 5 on 2026-08-15, 2026-08-19, 2026-08-21, 2026-08-24, 2026-08-31
//

#version 450

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
} linear_interpolation_attr;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexcoord;

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
float ign(vec2 p, float seed) {
    return fract(52.9829189 * fract(0.06711056 * p.x + 0.00583715 * p.y + seed));
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

// Krzysztof Narkowicz's 2015 curve fit to the ACES RRT+ODT, not the ACES transform itself.
vec3 aces_curve(vec3 x) {
    return (x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14);
}

// John Hable's Uncharted 2 filmic curve, presented at GDC 2010.
vec3 filmic_curve(vec3 x) {
    const float a = 0.15;
    const float b = 0.50;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.02;
    const float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
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

void main() {

    ivec2 coord = ivec2(gl_FragCoord.xy);
    ivec2 texSize = textureSize(canvas, 0);

    vec4 c;
    if (linear_interpolation_attr.use) {
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

    color = clamp(apply_dither(vec4(output_transform(c.rgb), c.a), coord), 0.0, 1.0);
}
