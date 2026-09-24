//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-23
//

#version 450

layout (set = 0, binding = 0) uniform sampler2D canvas;
layout (set = 0, binding = 1) uniform ResampleUBO{
    uvec2 extent;
} resample_attr;


layout (location = 0) out vec4 color;

// Hard cap on taps per axis.
const int MAX_TAPS = 16;

void main() {
    vec2 srcSize = vec2(textureSize(canvas, 0));
    vec2 dstSize = vec2(resample_attr.extent);
    vec2 ratio = srcSize / dstSize;
    ivec2 taps = clamp(ivec2(ceil(ratio)), ivec2(1), ivec2(MAX_TAPS));
    vec2 srcCenter = gl_FragCoord.xy * ratio;
    vec2 cell = ratio / vec2(taps);
    vec2 start = srcCenter - 0.5 * ratio;
    vec2 invSrc = 1.0 / srcSize;

    if (max(ratio.x, ratio.y) > float(MAX_TAPS * 2)) {
        vec2 end = start + ratio;
        ivec2 first = max(ivec2(floor(start)), ivec2(0));
        ivec2 last = min(ivec2(ceil(end)), ivec2(srcSize)) - 1;
        vec4 areaSum = vec4(0.0);
        for (int y = first.y; y <= last.y; ++y) {
            float height = max(0.0, min(end.y, float(y + 1)) - max(start.y, float(y)));
            for (int x = first.x; x <= last.x; ++x) {
                float width = max(0.0, min(end.x, float(x + 1)) - max(start.x, float(x)));
                areaSum += texelFetch(canvas, ivec2(x, y), 0) * (width * height);
            }
        }
        color = areaSum / (ratio.x * ratio.y);
        return;
    }

    vec4 sum = vec4(0.0);
    for (int y = 0; y < taps.y; ++y) {
        for (int x = 0; x < taps.x; ++x) {
            vec2 texel = start + (vec2(x, y) + 0.5) * cell;
            sum += texture(canvas, texel * invSrc);
        }
    }

    color = sum / float(taps.x * taps.y);
}
