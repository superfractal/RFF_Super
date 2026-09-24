//
// Modified by Opus 5 on 2026-08-16, 2026-08-19, 2026-08-24
// Modified by GPT-6 on 2026-09-10, 2026-09-16, 2026-09-23
//

#version 450
#extension GL_GOOGLE_include_directive : require
#include "shader_layer.glsl"

layout (set = 0, binding = 0) uniform sampler2D bloom_canvas;
layout (set = 0, binding = 1) uniform sampler2D bloom_blurred;
layout (set = 1, binding = 0) uniform BloomUBO {
    float threshold;
    float radius;
    float softness;
    float intensity;
// Carried as floats rather than a bool and a float, the way the slope pass carries its own flags.
    float hdr;
    float headroom;
    float linear_add;
    float scene_linear;
} bloom_attr;


layout (location = 0) out vec4 color;

// Matches vk_bloom_threshold.frag, which is what decided the blurred half of the glow.
float grayScale(vec3 c) {
    return c.r * 0.3 + c.g * 0.59 + c.b * 0.11;
}

// IEC 61966-2-1 sRGB EOTF and its inverse, acknowledged in NOTICE.
vec3 srgb_to_linear(vec3 c) {
    vec3 s = max(c, 0.0);
    return mix(s / 12.92, pow((s + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), s));
}

vec3 linear_to_srgb(vec3 c) {
    vec3 s = max(c, 0.0);
    return mix(s * 12.92, 1.055 * pow(s, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), s));
}

void main() {
    if (ordered_layers() && !layer_is(25)) { color = texelFetch(bloom_canvas, ivec2(gl_FragCoord.xy), 0); return; }

    vec2 normalizedCoord = gl_FragCoord.xy / textureSize(bloom_canvas, 0);

    float normalizedX = normalizedCoord.x;
    float normalizedY = normalizedCoord.y;

    if (normalizedX < 0 || normalizedY < 0){
        discard;
    }

    if (normalizedX >= 1 || normalizedY >= 1){
        discard;
    }

    color = texture(bloom_canvas, normalizedCoord);
    vec3 blurredGlow = texture(bloom_blurred, normalizedCoord).rgb;
    // The sharp end of the glow is the thresholded image, not the canvas: only the blurred half
    // ever went through the threshold pass, so mixing the canvas in handed every pixel below the
    // threshold a share of its own light, and at softness 1 lifted the whole frame by intensity.
    vec3 thresholdedGlow = grayScale(color.rgb) < bloom_attr.threshold ? vec3(0.0) : color.rgb;
    // Softness runs from that hard glow towards the blurred one, the direction the name reads in.
    vec3 glow = mix(thresholdedGlow, blurredGlow, bloom_attr.softness);
    if (bloom_attr.scene_linear > 0.5) {
        color = vec4(clamp(color.rgb + glow * bloom_attr.intensity, 0.0, 60000.0), 1);
        return;
    }
    if (bloom_attr.linear_add > 0.5) {
        // Summed in proportion to light, as vk_slope.frag's LIGHT_BLEND_LINEAR is, and re-encoded past 1 for the store below to carry.
        color = vec4(linear_to_srgb(srgb_to_linear(color.rgb) + srgb_to_linear(glow) * bloom_attr.intensity), 1);
    } else {
        color = color + vec4(glow * bloom_attr.intensity, 1);
    }
    // This is the one pass that makes light brighter than white, and the image it writes into holds
    // 0 to 1. With HDR on the sum is stored as its fraction of the headroom, so that light survives
    // the store at the image's own precision instead of being cut off at white; the final pass
    // multiplies it back before the tone map.
    if (bloom_attr.hdr > 0.5) {
        // In linear light, because the final pass decodes sRGB before it multiplies the headroom back.
        color = vec4(linear_to_srgb(srgb_to_linear(color.rgb) / max(bloom_attr.headroom, 1e-3)), color.a);
    }
    // The video's images hold values outside 0 to 1 where the preview's cut them off on every store,
    // so the ceiling both of them draw under is set here rather than left to the image.
    color = clamp(color, 0.0, 1.0);

}
