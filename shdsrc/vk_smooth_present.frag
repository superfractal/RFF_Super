//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-18, 2026-09-23
//

#version 450
layout(push_constant) uniform ZoomTransform { vec4 transform; } zoom;
// Original RFF_Super affine presentation and area sampling, GPL-3.0-only; see NOTICE.

layout (set = 0, binding = 0) uniform sampler2D canvas;
layout (set = 0, binding = 1) uniform ResampleUBO{
    uvec2 extent;
    uint srgb_attachment;
} resample_attr;


layout (location = 0) out vec4 color;

// Hard cap on taps per axis.
const int MAX_TAPS = 16;

void main() {
    vec2 srcSize = vec2(textureSize(canvas, 0));
    vec2 dstSize = vec2(resample_attr.extent);
    vec2 ratio = srcSize / dstSize * zoom.transform.x;
    ivec2 taps = clamp(ivec2(ceil(ratio)), ivec2(1), ivec2(MAX_TAPS));
    vec2 srcCenter = gl_FragCoord.xy * ratio + zoom.transform.yz * srcSize;
    vec2 cell = ratio / vec2(taps);
    vec2 start = srcCenter - 0.5 * ratio;
    vec2 invSrc = 1.0 / srcSize;

    vec4 sum = vec4(0.0);
    for (int y = 0; y < taps.y; ++y) {
        for (int x = 0; x < taps.x; ++x) {
            vec2 texel = start + (vec2(x, y) + 0.5) * cell;
            sum += texture(canvas, texel * invSrc);
        }
    }

    color = sum / float(taps.x * taps.y);
    if (resample_attr.srgb_attachment != 0u) {
        // IEC 61966-2-1 display transfer; provenance: NOTICE.
        vec3 encoded = color.rgb;
        color.rgb = mix(encoded / 12.92,
                        pow(max((encoded + 0.055) / 1.055, vec3(0.0)), vec3(2.4)),
                        step(vec3(0.04045), encoded));
    }
}
