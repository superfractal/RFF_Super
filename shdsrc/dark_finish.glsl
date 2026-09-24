//
// Modified by GPT-6 on 2026-09-13, 2026-09-19, 2026-09-23
//

// Original RFF_Super VHS dropout, nearest patch sampling and film grain; GPL-3.0-only, see NOTICE.
layout(set = 2, binding = 0) uniform DarkCanvasUBO {
    uvec2 extent;
    double max_value;
    double max_value_normal;
    double max_value_zoomed;
    uvec2 canvas_extent;
    ivec2 canvas_offset;
} dark_canvas;

// Same original project integer noise as effects.glsl; see NOTICE.
float dark_random(ivec2 cell, float seed) {
    uint n = uint(cell.x) * 197u + uint(cell.y) * 1999u + uint(seed) * 101u + 17u;
    n ^= n >> 11u;
    n *= 7919u;
    n ^= n >> 7u;
    n *= 104729u;
    return float(n & 0x00ffffffu) / 16777216.0;
}

vec2 dark_position(ivec2 coord) {
    float scale = max(float(dark_canvas.canvas_extent.y) / 720.0, 0.001);
    return (vec2(coord) + vec2(dark_canvas.canvas_offset)) / scale;
}

vec3 dark_sample(vec3 c, ivec2 coord, ivec2 texSize) {
    float strength = linear_interpolation_attr.layer_vhs * linear_interpolation_attr.dark_strength;
    if (strength <= 0.0) {
        return c;
    }

    vec2 p = dark_position(coord);
    float scale = max(float(dark_canvas.canvas_extent.y) / 720.0, 0.001);
    float row = dark_random(ivec2(0, floor(p.y * 0.7)), 31.0);
    float dropout = smoothstep(0.96, 1.0, row);
    float tracking = smoothstep(0.62, 0.9, dark_random(ivec2(0, floor(p.y / 14.0)), 211.0));
    tracking *= smoothstep(0.4, 0.85, dark_random(ivec2(floor(p / vec2(27, 2))), 223.0));
    dropout = max(dropout, tracking * 0.7 * linear_interpolation_attr.dark_damage);
    float shift = dropout
        * (dark_random(ivec2(floor(p / vec2(80, 2))), 19.0) - 0.5)
        * 45.0 * linear_interpolation_attr.dark_vhs;
    ivec2 shifted = coord + ivec2(round(shift * scale), 0);
    vec3 result = c;
    if (shift != 0.0) {
        result = safeTexelFetch(canvas, shifted, texSize).rgb;
    }

    float separation = linear_interpolation_attr.dark_chroma * scale
        * (1.0 + 4.0 * dropout
            * mix(1.0, linear_interpolation_attr.dark_vhs, linear_interpolation_attr.dark_damage));
    if (separation > 0.0) {
        int dx = int(round(separation));
        result.r = safeTexelFetch(canvas, shifted + ivec2(dx, 0), texSize).r;
        result.b = safeTexelFetch(canvas, shifted - ivec2(dx, 0), texSize).b;
    }
    if (linear_interpolation_attr.dark_pixel_mix > 0.0) {
        float block = max(linear_interpolation_attr.dark_pixel_size, 1.0);
        vec2 coarse = (floor(p / block) + 0.5) * block;
        ivec2 sampleCoord = ivec2(coarse * scale) - dark_canvas.canvas_offset;
        float region = smoothstep(0.45, 0.8, dark_random(ivec2(floor(p / vec2(72, 36))), 53.0));
        result = mix(result, safeTexelFetch(canvas, sampleCoord, texSize).rgb,
                     region * linear_interpolation_attr.dark_pixel_mix);
    }
    return mix(c, result, strength);
}

vec3 dark_finish_layer(vec3 c, ivec2 coord, float style, float strength) {
    if (style < 2.5 || style > 4.5 || strength <= 0.0) {
        return c;
    }

    vec2 p = dark_position(coord);
    float scale = max(linear_interpolation_attr.dark_scale, 0.25);
    float grain = dark_random(ivec2(floor(p / scale)), 83.0) - 0.5;
    vec3 result = c;
    if (style > 3.5) {
        float gray = dot(c, vec3(0.2126, 0.7152, 0.0722));
        gray *= 1.0 + grain * linear_interpolation_attr.dark_grain * 1.4;
        gray += pow(max(grain * 2.0, 0.0), 28.0) * linear_interpolation_attr.dark_grain * 0.11;
        vec3 colored = c * (1.0 + grain * linear_interpolation_attr.dark_grain * 1.4);
        colored += pow(max(grain * 2.0, 0.0), 28.0) * linear_interpolation_attr.dark_grain * 0.11;
        result = mix(colored, vec3(gray), linear_interpolation_attr.dark_monochrome);
    } else {
        float row = dark_random(ivec2(0, floor(p.y * 0.7)), 31.0);
        float tear = smoothstep(0.96, 1.0, row)
            * smoothstep(0.35, 0.85, dark_random(ivec2(floor(p / vec2(90, 2))), 43.0));
        float margin = pow(abs(p.x / max(float(dark_canvas.canvas_extent.x) * 720.0
                                       / float(dark_canvas.canvas_extent.y), 1.0) - 0.5) * 2.0, 2.0);
        float tracking = smoothstep(0.62, 0.9, dark_random(ivec2(0, floor(p.y / 14.0)), 211.0));
        float blocks = smoothstep(0.45, 0.90, dark_random(ivec2(floor(p / vec2(27, 2))), 223.0));
        float flecks = dark_random(ivec2(floor(p / vec2(3, 1))), 227.0);
        float damage = tracking * blocks * (0.12 + 0.88 * margin) * linear_interpolation_attr.dark_damage;
        float scan = 0.5 + 0.5 * cos(p.y * 3.14159265);
        result *= 1.0 - linear_interpolation_attr.dark_vhs * 0.16 * scan;
        result += vec3(1.0, 0.55, 0.6) * tear * linear_interpolation_attr.dark_vhs * 0.8;
        result *= 1.0 - damage * linear_interpolation_attr.dark_vhs * 0.65;
        result += mix(vec3(0.55, 0.005, 0.025), vec3(1.0, 0.9, 0.92), pow(flecks, 7.0))
            * damage * linear_interpolation_attr.dark_vhs * 1.4;
        result += grain * linear_interpolation_attr.dark_grain * (0.08 + 0.22 * sqrt(max(result, vec3(0))));
    }
    return mix(c, max(result, vec3(0)), strength);
}

vec3 dark_finish(vec3 c, ivec2 coord) {
    float phonk = linear_interpolation_attr.layer_vhs;
    float mono = linear_interpolation_attr.layer_mono;
    if (phonk > 0.0) {
        c = dark_finish_layer(c, coord, 3.0,
                              phonk * linear_interpolation_attr.dark_strength);
    }
    if (mono > 0.0) {
        c = dark_finish_layer(c, coord, 4.0,
                              mono * linear_interpolation_attr.dark_strength);
    }
    return c;
}
