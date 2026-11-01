//
// Created by Opus 5.5 on 2026-10-07.
// Modified by Opus 5.5 on 2026-10-08
//

// Supersampling resolve shared by vk_smooth_present.frag and vk_image_rgba2bgr.comp; SsaaResolve.hpp does the same on the CPU.
#ifndef RFF_SSAA_RESOLVE
#define RFF_SSAA_RESOLVE

#define SSAA_FILTER_BOX_GAMMA 0u
#define SSAA_FILTER_MITCHELL_LINEAR 1u

// Only a shrink has samples to combine; at 1:1 or larger the Mitchell kernel would just soften the picture.
bool ssaa_use_mitchell(uint mode, vec2 ratio) {
    return mode == SSAA_FILTER_MITCHELL_LINEAR && max(ratio.x, ratio.y) > 1.0;
}

// Most source texels taken per axis; a wider footprint is sampled at a stride instead.
const int SSAA_MAX_TAPS = 32;

// IEC 61966-2-1 sRGB transfer, decode and encode; provenance: NOTICE.
vec3 ssaa_srgb_to_linear(vec3 c) {
    c = clamp(c, 0.0, 1.0);
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 ssaa_linear_to_srgb(vec3 l) {
    l = clamp(l, 0.0, 1.0);
    return mix(l * 12.92, 1.055 * pow(l, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), l));
}

// The curve the stored values carry, numbered as VidHdrTransfer: sRGB for an SDR picture, PQ or HLG for an HDR export.
#define SSAA_TRANSFER_SRGB 0u
#define SSAA_TRANSFER_PQ 1u
#define SSAA_TRANSFER_HLG 2u

// SMPTE ST 2084 EOTF and its inverse, with 1.0 as 10000 nits; provenance: NOTICE.
const float SSAA_PQ_M1 = 0.1593017578125;
const float SSAA_PQ_M2 = 78.84375;
const float SSAA_PQ_C1 = 0.8359375;
const float SSAA_PQ_C2 = 18.8515625;
const float SSAA_PQ_C3 = 18.6875;

vec3 ssaa_pq_to_linear(vec3 c) {
    vec3 p = pow(clamp(c, 0.0, 1.0), vec3(1.0 / SSAA_PQ_M2));
    return pow(max(p - SSAA_PQ_C1, 0.0) / max(SSAA_PQ_C2 - SSAA_PQ_C3 * p, 1e-6), vec3(1.0 / SSAA_PQ_M1));
}

vec3 ssaa_linear_to_pq(vec3 l) {
    vec3 y = pow(clamp(l, 0.0, 1.0), vec3(SSAA_PQ_M1));
    return pow((SSAA_PQ_C1 + SSAA_PQ_C2 * y) / (1.0 + SSAA_PQ_C3 * y), vec3(SSAA_PQ_M2));
}

// ARIB STD-B67 inverse OETF and OETF over scene light in [0, 1]; provenance: NOTICE.
const float SSAA_HLG_A = 0.17883277;
const float SSAA_HLG_B = 0.28466892;
const float SSAA_HLG_C = 0.55991073;

vec3 ssaa_hlg_to_linear(vec3 c) {
    c = clamp(c, 0.0, 1.0);
    vec3 high = (exp((c - SSAA_HLG_C) / SSAA_HLG_A) + SSAA_HLG_B) / 12.0;
    return mix(high, c * c / 3.0, step(c, vec3(0.5)));
}

vec3 ssaa_linear_to_hlg(vec3 l) {
    l = clamp(l, 0.0, 1.0);
    vec3 high = SSAA_HLG_A * log(max(12.0 * l - SSAA_HLG_B, 1e-6)) + SSAA_HLG_C;
    return mix(high, sqrt(3.0 * l), step(l, vec3(1.0 / 12.0)));
}

vec3 ssaa_decode(vec3 c, uint transfer) {
    if (transfer == SSAA_TRANSFER_PQ) return ssaa_pq_to_linear(c);
    if (transfer == SSAA_TRANSFER_HLG) return ssaa_hlg_to_linear(c);
    return ssaa_srgb_to_linear(c);
}

vec3 ssaa_encode(vec3 l, uint transfer) {
    if (transfer == SSAA_TRANSFER_PQ) return ssaa_linear_to_pq(l);
    if (transfer == SSAA_TRANSFER_HLG) return ssaa_linear_to_hlg(l);
    return ssaa_linear_to_srgb(l);
}

// Mitchell-Netravali cubic with B = C = 1/3 (Mitchell & Netravali, SIGGRAPH 1988; see NOTICE), zero from |x| = 2 on.
float ssaa_mitchell(float x) {
    x = abs(x);
    if (x < 1.0) {
        return (7.0 * x * x * x - 12.0 * x * x + 16.0 / 3.0) / 6.0;
    }
    if (x < 2.0) {
        return (-7.0 / 3.0 * x * x * x + 12.0 * x * x - 20.0 * x + 32.0 / 3.0) / 6.0;
    }
    return 0.0;
}

// Mitchell filter in linear light around center (source texels); the kernel widens by ratio when shrinking, and off-image taps drop out.
vec4 ssaa_mitchell_linear(sampler2D source, vec2 center, vec2 ratio, uint transfer) {
    ivec2 size = textureSize(source, 0);
    vec2 scale = max(ratio, vec2(1.0));
    vec2 radius = 2.0 * scale;
    ivec2 lo = max(ivec2(ceil(center - radius - 0.5)), ivec2(0));
    ivec2 hi = min(ivec2(floor(center + radius - 0.5)), size - 1);
    ivec2 stride = max((hi - lo + SSAA_MAX_TAPS) / SSAA_MAX_TAPS, ivec2(1));
    vec4 sum = vec4(0.0);
    float weightSum = 0.0;
    for (int y = lo.y; y <= hi.y; y += stride.y) {
        float wy = ssaa_mitchell((float(y) + 0.5 - center.y) / scale.y);
        for (int x = lo.x; x <= hi.x; x += stride.x) {
            float w = wy * ssaa_mitchell((float(x) + 0.5 - center.x) / scale.x);
            vec4 c = texelFetch(source, ivec2(x, y), 0);
            sum += vec4(ssaa_decode(c.rgb, transfer), c.a) * w;
            weightSum += w;
        }
    }
    if (weightSum <= 0.0) {
        vec4 c = texelFetch(source, clamp(ivec2(center), ivec2(0), size - 1), 0);
        return vec4(ssaa_decode(c.rgb, transfer), c.a);
    }
    // The negative lobes can overshoot past either end; linear light is clamped back into range here.
    return clamp(sum / weightSum, 0.0, 1.0);
}

#endif
