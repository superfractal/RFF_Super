//
// Modified by Opus 5 on 2026-08-06
// Modified by GPT-6 on 2026-09-10, 2026-09-16, 2026-09-23
//

#version 450
#extension GL_GOOGLE_include_directive : require
#include "shader_layer.glsl"

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput canvas;

layout (set = 1, binding = 0) uniform ColorUBO {
    float gamma;
    float exposure;
    float hue;
    float saturation;
    float brightness;
    float contrast;
    float scene_linear;
} color_attr;

layout (location = 0) out vec4 color;

float grayscale(vec3 c) {
    return c.r * 0.3 + c.g * 0.59 + c.b * 0.11;
}

vec3 fix_color(vec3 col) {
    return clamp(col, vec3(0, 0, 0), vec3(1, 1, 1));
}

float get_hue(vec3 col) {
    float high = max(max(col.r, col.g), col.b);
    float low = min(min(col.r, col.g), col.b);
    if (high == low) {
        return 0;
    }

    float range = high - low;
    float mid = clamp(col.r + col.g + col.b - high - low, 0, 1);
    float rat = max(mid - low, 0) / range;

    float offset;
    // hue detection, 0 = low, 1 = high
    // ---------------------------------
    // index | 0   1   2   3   4   5
    // ---------------------------------
    // r     | 1 1 1 ~ 0 0 0 0 0 ~ 1 1
    // g     | 0 ~ 1 1 1 1 1 ~ 0 0 0 0
    // b     | 0 0 0 0 0 ~ 1 1 1 1 1 ~
    // ---------------------------------
    if (low == col.b) {
        if (high == col.r) {
            // 0 = b, ~ = g, 1 = r
            offset = rat;

        } else {
            // 0 = b, ~ = r, 1 = g
            offset = 2 - rat;
        }
    } else if (low == col.r) {
        if (high == col.g) {
            // 0 = r, ~ = b, 1 = g
            offset = 2 + rat;
        } else {
            // 0 = r, ~ = g, 1 = b
            offset = 4 - rat;
        }
    } else {
        if (high == col.b) {
            // 0 = g, ~ = r, 1 = b
            offset = 4 + rat;
        } else {
            // 0 = g, ~ = b, 1 = r
            offset = 6 - rat;
        }
    }

    return offset / 6;
}

vec3 add_hue(vec3 col, float add) {

    float high = max(max(col.r, col.g), col.b);
    float low = min(min(col.r, col.g), col.b);
    float hue = get_hue(col);
    float off = mod(hue + mod(add, 1), 1) * 6;
    int ioff = int(off);
    float doff = mod(off, 1);

    // ---------------------------------
    // index | 0   1   2   3   4   5
    // ---------------------------------
    // r     | 1 1 1 ~ 0 0 0 0 0 ~ 1 1
    // g     | 0 ~ 1 1 1 1 1 ~ 0 0 0 0
    // b     | 0 0 0 0 0 ~ 1 1 1 1 1 ~
    // ---------------------------------


    // Seeded with the input so a non-finite hue leaves the color untouched instead of undefined.
    vec3 result = col;
    switch (ioff) {
        case 0: {
                    result = vec3(high, low - (low - high) * doff, low);
                    break;
                }
        case 1: {
                    result = vec3(high - (high - low) * doff, high, low);
                    break;
                }
        case 2: {
                    result = vec3(low, high, low - (low - high) * doff);
                    break;
                }
        case 3: {
                    result = vec3(low, high - (high - low) * doff, high);
                    break;
                }
        case 4: {
                    result = vec3(low - (low - high) * doff, low, high);
                    break;
                }
        case 5: {
                    result = vec3(high, low, high - (high - low) * doff);
                    break;
                }
    }
    return result;
}

// IEC 61966-2-1 transfer, reused from the project's shaders under GPL; see NOTICE.
vec3 studio_decode(vec3 c) {
    c = max(c, 0.0);
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 studio_encode(vec3 c) {
    c = max(c, 0.0);
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

void main() {

    vec3 c = subpassLoad(canvas).rgb;
    if (ordered_layers() && !layer_is(23)) { color = vec4(c, 1); return; }
    float sceneScale = 1.0;
    if (color_attr.scene_linear > 0.5) {
        if (color_attr.gamma == 1.0 && color_attr.exposure == 0.0 && color_attr.hue == 0.0 &&
            color_attr.saturation == 0.0 && color_attr.brightness == 0.0 && color_attr.contrast == 0.0) {
            color = vec4(c, 1);
            return;
        }
        // Apply the native artistic grade to peak-normalized encoded color, preserving radiance above white.
        sceneScale = max(1.0, max(c.r, max(c.g, c.b)));
        c = studio_encode(c / sceneScale);
    }

    // Every divisor is bounded away from zero: the UI accepts any float, so gamma 0 and exposure/contrast 1 are reachable.
    float gamma = max(color_attr.gamma, 1e-3);
    float exposure = min(color_attr.exposure, 0.999);
    float contrast = min(color_attr.contrast, 0.999);

    c = fix_color(pow(c, vec3(1 / gamma)));
    c = fix_color(c * (1 + exposure) / (1 - exposure));
    c = fix_color(add_hue(c, color_attr.hue));
    float gray = grayscale(c);
    c = fix_color(c + (c - vec3(gray, gray, gray)) * color_attr.saturation);
    c = fix_color(c + color_attr.brightness);
    c = fix_color((c - 0.5) / (1 - contrast) * (1 + contrast) + 0.5);
    color = vec4(color_attr.scene_linear > 0.5 ? studio_decode(c) * sceneScale : c, 1);
}
