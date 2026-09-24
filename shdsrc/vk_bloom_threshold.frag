//
// Modified by GPT-6 on 2026-09-23
//

#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput canvas;

layout (set = 1, binding = 0) uniform BloomUBO {
    float threshold;
    float radius;
    float softness;
    float intensity;
} bloom_attr;

layout (location = 0) out vec4 color;

float grayScale(vec3 c) {
    return c.r * 0.3 + c.g * 0.59 + c.b * 0.11;
}

void main() {
    vec4 c = subpassLoad(canvas);
    if (grayScale(c.rgb) < bloom_attr.threshold) {
        color = vec4(0, 0, 0, 1);
        return;
    }
    color = c;
}
