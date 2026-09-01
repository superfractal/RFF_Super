//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-10, 2026-08-13, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-20, 2026-08-21, 2026-08-22, 2026-08-25, 2026-08-26, 2026-08-27, 2026-08-31
// Modified by ox-alpha on 2026-08-22.
//

#version 450
#define NONE 0
#define NORMAL 1
#define REVERSED 2
#define ANIMATION_LINEAR 0
#define ANIMATION_PSYCHEDELIC 1
#define ANIMATION_BREATHING 3
#define ANIMATION_TURBULENCE 4
#define TWO_PI 6.2831853071795864
#define TEXTURE_UV_CYCLE_ANGLE 0
#define TEXTURE_UV_CYCLE_SCREEN 1
#define TEXTURE_UV_SCREEN 2
#define TEXTURE_UV_CYCLE_BAND 3
#define TEXTURE_BLEND_MULTIPLY 0
#define TEXTURE_BLEND_OVERLAY 1
#define TEXTURE_BLEND_REPLACE 2
#define PATTERN_STRIPES 0
#define PATTERN_CHECKER 1
#define PATTERN_GRID 2
#define PATTERN_DOTS 3
#define PATTERN_DIAMOND 4
#define PATTERN_HONEYCOMB 5
#define PATTERN_WAVES 6
#define PATTERN_CLOUD 7
#define PATTERN_INK_SOLID 0
#define PATTERN_INK_PALETTE_SHIFT 1
#define TEXTURE_LAYERS 4
#define PATTERN_LAYERS 4
#define INTERP_RGB 0
#define INTERP_OKLAB 1
#define CYCLE_CURVE_POWER 0
#define CYCLE_CURVE_WAVE 1
#define ITER_COLORING_LINEAR 0
#define ITER_COLORING_SQUARE_ROOT 1
#define ITER_COLORING_CUBE_ROOT 2
#define ITER_COLORING_LOG 3
#define ITER_COLORING_LOG_LOG 4
#define ITER_COLORING_SMOOTHSTEP 5
#define ITER_COLORING_SMOOTHERSTEP 6
#define LN2 0.69314718055994531LF
#define INV_LN2 1.4426950408889634LF
#define INV_LOG_LOG_UNIT 1.8990140986034139LF
#define WARP_SOURCE_NOISE 0
#define WARP_MAX_OCTAVES 6

layout (set = 0, binding = 0) uniform IterUBO {
    uvec2 extent;
    double max_value;
    // Declared so the tail fields below are reachable; the other shaders stop at max_value.
    double max_value_normal;
    double max_value_zoomed;
    // The whole canvas this buffer is a piece of, and where the piece sits in it. Equal to extent
    // with a zero offset for an ordinary frame; a tiled export gives each tile its own place, so the
    // screen-space animation fields and decor UVs stay one continuous image instead of repeating.
    uvec2 canvas_extent;
    ivec2 canvas_offset;
} iteration_info_attr;

layout (set = 0, binding = 1) buffer IterSSBO {
    double iterations[];
} iteration_attr;

#define MAX_STATIC_COLORS 16

layout (set = 1, binding = 0) buffer PaletteSSBO {
    vec4 interval;
    double offset;
    uint size;
    uint smoothing; // low 8 bits = coloring method, bit 8 = palette interpolation space
    float animation_speed;
    uint static_color_count; // number of eyedropper-frozen colors
    float static_color_tolerance; // match radius in cycle units
    uint animation_mode;
    float animation_flow_amount;
    float animation_flow_scale;
    float animation_flow_speed;
    float animation_flow_swirl;
    vec4 mandelbrot_color;
    double static_color_iterations[MAX_STATIC_COLORS];
    // Appended ahead of the runtime color array, which must stay last. The layout pads after this
    // float so the array holds its 16-byte alignment; the host reserve carries the same pad.
    float cycle_bias;
    vec4 palette[];
} palette_attr;


layout (set = 2, binding = 0) uniform TimeUBO {
    float time;
    // Animation phases accumulated on the host, in iterations, in flow cycles, and in UV for the
    // decor layers' scroll. Using them instead of time * speed keeps every phase continuous when a
    // speed is edited mid-animation. Declared in the order the host packs them.
    float palette_phase;
    float flow_phase;
    float stripe_phase;
    float texture_scroll_u_0;
    float texture_scroll_v_0;
    float texture_scroll_u_1;
    float texture_scroll_v_1;
    float texture_scroll_u_2;
    float texture_scroll_v_2;
    float texture_scroll_u_3;
    float texture_scroll_v_3;
    float pattern_scroll_u_0;
    float pattern_scroll_v_0;
    float pattern_scroll_u_1;
    float pattern_scroll_v_1;
    float pattern_scroll_u_2;
    float pattern_scroll_v_2;
    float pattern_scroll_u_3;
    float pattern_scroll_v_3;
    float warp_scroll_u;
    float warp_scroll_v;
} time_attr;

// One binding per layer rather than one array binding, matching TextureDescriptor's one-descriptor
// -per-binding layout on the host.
layout (set = 3, binding = 0) uniform sampler2D exterior_texture_0;
layout (set = 3, binding = 1) uniform sampler2D exterior_texture_1;
layout (set = 3, binding = 2) uniform sampler2D exterior_texture_2;
layout (set = 3, binding = 3) uniform sampler2D exterior_texture_3;

layout (set = 3, binding = 4) uniform TextureUBO {
    uint enabled_0;
    uint uv_mode_0;
    uint blend_mode_0;
    float opacity_0;
    float scale_u_0;
    float scale_v_0;
    float scroll_u_0;
    float scroll_v_0;
    float palette_follow_0;
    float period_0;
    uint pattern_enabled;
    uint pattern_type;
    uint pattern_uv_mode;
    uint pattern_blend_mode;
    float pattern_opacity;
    float pattern_scale_u;
    float pattern_scale_v;
    float pattern_scroll_u;
    float pattern_scroll_v;
    float pattern_palette_follow;
    float pattern_period;
    float pattern_sharpness;
    float pattern_color_r;
    float pattern_color_g;
    float pattern_color_b;
    uint pattern_ink_mode;
    float pattern_palette_shift;
    // Layers 1 and up sit behind the pattern block so layer 0 and the pattern keep the offsets they
    // had before the stack existed, and an older host that only writes those still lines up.
    uint enabled_1;
    uint uv_mode_1;
    uint blend_mode_1;
    float opacity_1;
    float scale_u_1;
    float scale_v_1;
    float scroll_u_1;
    float scroll_v_1;
    float palette_follow_1;
    float period_1;
    uint enabled_2;
    uint uv_mode_2;
    uint blend_mode_2;
    float opacity_2;
    float scale_u_2;
    float scale_v_2;
    float scroll_u_2;
    float scroll_v_2;
    float palette_follow_2;
    float period_2;
    uint enabled_3;
    uint uv_mode_3;
    uint blend_mode_3;
    float opacity_3;
    float scale_u_3;
    float scale_v_3;
    float scroll_u_3;
    float scroll_v_3;
    float palette_follow_3;
    float period_3;
    // Domain warp, appended behind the last layer for the same reason the layers sit behind the pattern.
    uint warp_enabled;
    uint warp_source;
    uint warp_uv_mode;
    float warp_amount;
    float warp_octaves;
    float warp_scale_u;
    float warp_scale_v;
    float warp_scroll_u;
    float warp_scroll_v;
    float warp_palette_follow;
    float warp_period;
    // Pattern outline, appended behind the warp for the same reason the layers sit behind the pattern.
    uint pattern_edge_enabled;
    float pattern_edge_color_r;
    float pattern_edge_color_g;
    float pattern_edge_color_b;
    float pattern_edge_width;
    float pattern_edge_opacity;
    uint pattern_edge_relative;
    // Pattern layers 1 and up, appended behind everything else for the same reason the texture
    // layers sit behind the pattern. Each is one contiguous run in the field order layer 0's two
    // blocks are read in, so one layer's shape and its outline sit together.
    uint pattern_enabled_1;
    uint pattern_type_1;
    uint pattern_uv_mode_1;
    uint pattern_blend_mode_1;
    float pattern_opacity_1;
    float pattern_scale_u_1;
    float pattern_scale_v_1;
    float pattern_scroll_u_1;
    float pattern_scroll_v_1;
    float pattern_palette_follow_1;
    float pattern_period_1;
    float pattern_sharpness_1;
    float pattern_color_r_1;
    float pattern_color_g_1;
    float pattern_color_b_1;
    uint pattern_ink_mode_1;
    float pattern_palette_shift_1;
    uint pattern_edge_enabled_1;
    float pattern_edge_color_r_1;
    float pattern_edge_color_g_1;
    float pattern_edge_color_b_1;
    float pattern_edge_width_1;
    float pattern_edge_opacity_1;
    uint pattern_edge_relative_1;
    uint pattern_enabled_2;
    uint pattern_type_2;
    uint pattern_uv_mode_2;
    uint pattern_blend_mode_2;
    float pattern_opacity_2;
    float pattern_scale_u_2;
    float pattern_scale_v_2;
    float pattern_scroll_u_2;
    float pattern_scroll_v_2;
    float pattern_palette_follow_2;
    float pattern_period_2;
    float pattern_sharpness_2;
    float pattern_color_r_2;
    float pattern_color_g_2;
    float pattern_color_b_2;
    uint pattern_ink_mode_2;
    float pattern_palette_shift_2;
    uint pattern_edge_enabled_2;
    float pattern_edge_color_r_2;
    float pattern_edge_color_g_2;
    float pattern_edge_color_b_2;
    float pattern_edge_width_2;
    float pattern_edge_opacity_2;
    uint pattern_edge_relative_2;
    uint pattern_enabled_3;
    uint pattern_type_3;
    uint pattern_uv_mode_3;
    uint pattern_blend_mode_3;
    float pattern_opacity_3;
    float pattern_scale_u_3;
    float pattern_scale_v_3;
    float pattern_scroll_u_3;
    float pattern_scroll_v_3;
    float pattern_palette_follow_3;
    float pattern_period_3;
    float pattern_sharpness_3;
    float pattern_color_r_3;
    float pattern_color_g_3;
    float pattern_color_b_3;
    uint pattern_ink_mode_3;
    float pattern_palette_shift_3;
    uint pattern_edge_enabled_3;
    float pattern_edge_color_r_3;
    float pattern_edge_color_g_3;
    float pattern_edge_color_b_3;
    float pattern_edge_width_3;
    float pattern_edge_opacity_3;
    uint pattern_edge_relative_3;
    // Every layer's Size and Keep Aspect, appended behind everything else so no offset above moves.
    float size_0;
    uint keep_aspect_0;
    float size_1;
    uint keep_aspect_1;
    float size_2;
    uint keep_aspect_2;
    float size_3;
    uint keep_aspect_3;
} texture_attr;

// One texture layer's parameters, pulled out of the flat UBO so the stack can be walked in a loop.
struct DecorLayer {
    uint enabled;
    uint uv_mode;
    uint blend_mode;
    float opacity;
    float scale_u;
    float scale_v;
    float scroll_u;
    float scroll_v;
    float palette_follow;
    float period;
    float size;
    uint keep_aspect;
};

DecorLayer texture_layer(int i) {
    switch (i) {
        case 1: return DecorLayer(texture_attr.enabled_1, texture_attr.uv_mode_1, texture_attr.blend_mode_1,
                                  texture_attr.opacity_1, texture_attr.scale_u_1, texture_attr.scale_v_1,
                                  texture_attr.scroll_u_1, texture_attr.scroll_v_1,
                                  texture_attr.palette_follow_1, texture_attr.period_1,
                                  texture_attr.size_1, texture_attr.keep_aspect_1);
        case 2: return DecorLayer(texture_attr.enabled_2, texture_attr.uv_mode_2, texture_attr.blend_mode_2,
                                  texture_attr.opacity_2, texture_attr.scale_u_2, texture_attr.scale_v_2,
                                  texture_attr.scroll_u_2, texture_attr.scroll_v_2,
                                  texture_attr.palette_follow_2, texture_attr.period_2,
                                  texture_attr.size_2, texture_attr.keep_aspect_2);
        case 3: return DecorLayer(texture_attr.enabled_3, texture_attr.uv_mode_3, texture_attr.blend_mode_3,
                                  texture_attr.opacity_3, texture_attr.scale_u_3, texture_attr.scale_v_3,
                                  texture_attr.scroll_u_3, texture_attr.scroll_v_3,
                                  texture_attr.palette_follow_3, texture_attr.period_3,
                                  texture_attr.size_3, texture_attr.keep_aspect_3);
        default: return DecorLayer(texture_attr.enabled_0, texture_attr.uv_mode_0, texture_attr.blend_mode_0,
                                   texture_attr.opacity_0, texture_attr.scale_u_0, texture_attr.scale_v_0,
                                   texture_attr.scroll_u_0, texture_attr.scroll_v_0,
                                   texture_attr.palette_follow_0, texture_attr.period_0,
                                   texture_attr.size_0, texture_attr.keep_aspect_0);
    }
}

// One pattern layer's parameters, pulled out of the flat UBO so the stack can be walked in a loop.
struct PatternLayer {
    uint enabled;
    uint type;
    uint uv_mode;
    uint blend_mode;
    float opacity;
    float scale_u;
    float scale_v;
    float scroll_u;
    float scroll_v;
    float palette_follow;
    float period;
    float sharpness;
    float color_r;
    float color_g;
    float color_b;
    uint ink_mode;
    float palette_shift;
    uint edge_enabled;
    float edge_color_r;
    float edge_color_g;
    float edge_color_b;
    float edge_width;
    float edge_opacity;
    uint edge_relative;
};

PatternLayer pattern_layer(int i) {
    switch (i) {
        case 1: return PatternLayer(texture_attr.pattern_enabled_1, texture_attr.pattern_type_1, texture_attr.pattern_uv_mode_1,
                                  texture_attr.pattern_blend_mode_1, texture_attr.pattern_opacity_1, texture_attr.pattern_scale_u_1,
                                  texture_attr.pattern_scale_v_1, texture_attr.pattern_scroll_u_1, texture_attr.pattern_scroll_v_1,
                                  texture_attr.pattern_palette_follow_1, texture_attr.pattern_period_1, texture_attr.pattern_sharpness_1,
                                  texture_attr.pattern_color_r_1, texture_attr.pattern_color_g_1, texture_attr.pattern_color_b_1,
                                  texture_attr.pattern_ink_mode_1, texture_attr.pattern_palette_shift_1, texture_attr.pattern_edge_enabled_1,
                                  texture_attr.pattern_edge_color_r_1, texture_attr.pattern_edge_color_g_1, texture_attr.pattern_edge_color_b_1,
                                  texture_attr.pattern_edge_width_1, texture_attr.pattern_edge_opacity_1, texture_attr.pattern_edge_relative_1);
        case 2: return PatternLayer(texture_attr.pattern_enabled_2, texture_attr.pattern_type_2, texture_attr.pattern_uv_mode_2,
                                  texture_attr.pattern_blend_mode_2, texture_attr.pattern_opacity_2, texture_attr.pattern_scale_u_2,
                                  texture_attr.pattern_scale_v_2, texture_attr.pattern_scroll_u_2, texture_attr.pattern_scroll_v_2,
                                  texture_attr.pattern_palette_follow_2, texture_attr.pattern_period_2, texture_attr.pattern_sharpness_2,
                                  texture_attr.pattern_color_r_2, texture_attr.pattern_color_g_2, texture_attr.pattern_color_b_2,
                                  texture_attr.pattern_ink_mode_2, texture_attr.pattern_palette_shift_2, texture_attr.pattern_edge_enabled_2,
                                  texture_attr.pattern_edge_color_r_2, texture_attr.pattern_edge_color_g_2, texture_attr.pattern_edge_color_b_2,
                                  texture_attr.pattern_edge_width_2, texture_attr.pattern_edge_opacity_2, texture_attr.pattern_edge_relative_2);
        case 3: return PatternLayer(texture_attr.pattern_enabled_3, texture_attr.pattern_type_3, texture_attr.pattern_uv_mode_3,
                                  texture_attr.pattern_blend_mode_3, texture_attr.pattern_opacity_3, texture_attr.pattern_scale_u_3,
                                  texture_attr.pattern_scale_v_3, texture_attr.pattern_scroll_u_3, texture_attr.pattern_scroll_v_3,
                                  texture_attr.pattern_palette_follow_3, texture_attr.pattern_period_3, texture_attr.pattern_sharpness_3,
                                  texture_attr.pattern_color_r_3, texture_attr.pattern_color_g_3, texture_attr.pattern_color_b_3,
                                  texture_attr.pattern_ink_mode_3, texture_attr.pattern_palette_shift_3, texture_attr.pattern_edge_enabled_3,
                                  texture_attr.pattern_edge_color_r_3, texture_attr.pattern_edge_color_g_3, texture_attr.pattern_edge_color_b_3,
                                  texture_attr.pattern_edge_width_3, texture_attr.pattern_edge_opacity_3, texture_attr.pattern_edge_relative_3);
        default: return PatternLayer(texture_attr.pattern_enabled, texture_attr.pattern_type, texture_attr.pattern_uv_mode,
                                  texture_attr.pattern_blend_mode, texture_attr.pattern_opacity, texture_attr.pattern_scale_u,
                                  texture_attr.pattern_scale_v, texture_attr.pattern_scroll_u, texture_attr.pattern_scroll_v,
                                  texture_attr.pattern_palette_follow, texture_attr.pattern_period, texture_attr.pattern_sharpness,
                                  texture_attr.pattern_color_r, texture_attr.pattern_color_g, texture_attr.pattern_color_b,
                                  texture_attr.pattern_ink_mode, texture_attr.pattern_palette_shift, texture_attr.pattern_edge_enabled,
                                  texture_attr.pattern_edge_color_r, texture_attr.pattern_edge_color_g, texture_attr.pattern_edge_color_b,
                                  texture_attr.pattern_edge_width, texture_attr.pattern_edge_opacity, texture_attr.pattern_edge_relative);
    }
}

// Samplers cannot be selected at runtime or returned, so the layer index picks one here. The explicit
// LOD keeps this legal under the non-uniform control flow the stack walk introduces.
// Alpha comes back with the color: an image is loaded RGBA, so a layer drawn from one that carries
// transparency has to leave what is under it alone where the image is not there.
vec4 sample_texture_layer(int i, vec2 uv) {
    switch (i) {
        case 1: return textureLod(exterior_texture_1, uv, 0.0);
        case 2: return textureLod(exterior_texture_2, uv, 0.0);
        case 3: return textureLod(exterior_texture_3, uv, 0.0);
        default: return textureLod(exterior_texture_0, uv, 0.0);
    }
}

// The layer image's own width:height, which Keep Aspect draws the picture at.
float texture_layer_aspect(int i) {
    ivec2 s;
    switch (i) {
        case 1: s = textureSize(exterior_texture_1, 0); break;
        case 2: s = textureSize(exterior_texture_2, 0); break;
        case 3: s = textureSize(exterior_texture_3, 0); break;
        default: s = textureSize(exterior_texture_0, 0); break;
    }
    return s.y > 0 ? float(s.x) / float(s.y) : 1.0;
}

// Asked once per invocation in main(). Walking a four-layer stack to find every layer switched off
// costs a ten-field and a twenty-four-field struct rebuild per layer.
bool any_texture_enabled() {
    for (int i = 0; i < TEXTURE_LAYERS; i++) {
        DecorLayer l = texture_layer(i);
        if (l.enabled != 0u && l.opacity > 0.0) {
            return true;
        }
    }
    return false;
}

bool any_pattern_enabled() {
    for (int i = 0; i < PATTERN_LAYERS; i++) {
        PatternLayer p = pattern_layer(i);
        if (p.enabled != 0u && p.opacity > 0.0) {
            return true;
        }
    }
    return false;
}

// True when any layer asks for the band-aligned UV, which is what makes the iteration gradient worth taking.
bool any_texture_needs_band() {
    for (int i = 0; i < TEXTURE_LAYERS; i++) {
        DecorLayer l = texture_layer(i);
        if (l.enabled != 0u && l.opacity > 0.0 && l.uv_mode == TEXTURE_UV_CYCLE_BAND) {
            return true;
        }
    }
    return false;
}

// The same question for the pattern stack: every layer is asked, not only the first, or a layer
// further up the stack asking for the band-aligned UV would be handed a gradient of zero.
bool any_pattern_needs_band() {
    for (int i = 0; i < PATTERN_LAYERS; i++) {
        PatternLayer p = pattern_layer(i);
        if (p.enabled != 0u && p.opacity > 0.0 && p.uv_mode == TEXTURE_UV_CYCLE_BAND) {
            return true;
        }
    }
    return false;
}

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexcoord;

layout (location = 0) out vec4 color;

// The palette intervals and their reciprocals, taken once per invocation in main(). Every palette
// lookup used to divide by one of them - three times over in get_color_channel, once per channel -
// and the divisor is the same for every invocation in the draw.
dvec4 g_interval = dvec4(1.0);
dvec4 g_inv_interval = dvec4(1.0);

// a / b, rebuilt from the hoisted reciprocal r = 1/b. q = a*r lands within an ulp; the residual
// a - b*q is exact in an FMA, and folding it back lands on the correctly rounded quotient - the
// same double OpFDiv returns, without OpFDiv's cost. precise keeps the compiler from unfusing the
// two FMAs, which is what the identity rests on.
double div_r(double a, double b, double r) {
    precise double q = a * r;
    precise double e = fma(-b, q, a);
    precise double q2 = fma(e, r, q);
    return q2;
}

// Smoothing transform shared by coloring and freeze matching.
double smooth_iteration(double iteration) {
    switch (palette_attr.smoothing & 0xFFu) {
        case NONE: return iteration - mod(iteration, 1);
        case REVERSED: return iteration + 1 - 2 * mod(iteration, 1);
        default: return iteration;
    }
}

// ln(1+x) in double: the float builtin's own ulp is thousands of iterations wide at depth and would step the bands this feeds.
double log1p_d(double x) {
    if (x <= 0.0LF) {
        return 0.0LF;
    }
    double s;
    double hi = 0.0LF;
    if (x < 0.25LF) {
        // The same reduction without forming 1 + x, whose rounding is the whole error for a small x.
        s = x / (2.0LF + x);
    } else {
        double u = 1.0LF + x;
        int e = int(floor(log2(float(u))));
        // exp2 of a whole number is exact in float and in double, so this scaling costs no bits.
        double m = u * double(exp2(float(-e)));
        // The float exponent can miss by one either way; these put the mantissa back in [1/sqrt2, sqrt2).
        if (m >= 1.4142135623730951LF) {
            m *= 0.5LF;
            e += 1;
        }
        if (m < 0.70710678118654746LF) {
            m *= 2.0LF;
            e -= 1;
        }
        s = (m - 1.0LF) / (m + 1.0LF);
        hi = double(e) * LN2;
    }
    double t = s * s;
    // ln((1+s)/(1-s)) = 2s * (1 + s^2/3 + s^4/5 + ...), taken past double's last bit for every s that reduction leaves.
    double p = 1.0LF / 21.0LF;
    p = fma(p, t, 1.0LF / 19.0LF);
    p = fma(p, t, 1.0LF / 17.0LF);
    p = fma(p, t, 1.0LF / 15.0LF);
    p = fma(p, t, 1.0LF / 13.0LF);
    p = fma(p, t, 1.0LF / 11.0LF);
    p = fma(p, t, 1.0LF / 9.0LF);
    p = fma(p, t, 1.0LF / 7.0LF);
    p = fma(p, t, 1.0LF / 5.0LF);
    p = fma(p, t, 1.0LF / 3.0LF);
    p = fma(p, t, 1.0LF);
    return fma(2.0LF * s, p, hi);
}

// Cube root in double: the float builtin seeds it, and two Newton steps on y^3 = x each double the digits that seed holds.
double cbrt_d(double x) {
    if (x < 1.0e-20LF) {
        return 0.0LF;
    }
    double y = double(pow(float(x), 1.0 / 3.0));
    y = (2.0LF * y + x / (y * y)) / 3.0LF;
    y = (2.0LF * y + x / (y * y)) / 3.0LF;
    return y;
}

// Smoothstep and Smootherstep ease the count inside each cycle rather than compress the count itself: the whole part is kept, so every cycle boundary lands exactly where Linear puts it.
double eased_cycle(double ratio, const bool smoother) {
    double whole = floor(ratio);
    double u = ratio - whole;
    // u^2(3-2u) and u^3(u(6u-15)+10): both reach 0 and 1 with zero slope, and the second with zero curvature as well.
    double e = smoother
        ? u * u * u * (u * (6.0LF * u - 15.0LF) + 10.0LF)
        : u * u * (3.0LF - 2.0LF * u);
    return whole + e;
}

// Iteration Coloring: widens the bands as the count climbs, taken on the count already divided by the cycle length so one Cycle Length stays the first cycle's width.
double coloring_curve(double ratio) {
    switch ((palette_attr.smoothing >> 10) & 0xFu) {
        case ITER_COLORING_SQUARE_ROOT: return sqrt(max(ratio, 0.0LF));
        case ITER_COLORING_CUBE_ROOT: return cbrt_d(ratio);
        case ITER_COLORING_LOG: return log1p_d(ratio) * INV_LN2;
        case ITER_COLORING_LOG_LOG: return log1p_d(log1p_d(ratio)) * INV_LOG_LOG_UNIT;
        case ITER_COLORING_SMOOTHSTEP: return eased_cycle(ratio, false);
        case ITER_COLORING_SMOOTHERSTEP: return eased_cycle(ratio, true);
        default: return ratio;
    }
}

// Cycle Bias: the curve the cycle position is read through. Power bends it by pow(ratio, bias):
// above 1.00 a cycle lingers on its first colors and rushes its last; below 1.00 the reverse -
// but its slope dies at one end of the cycle, so under color animation the boundaries there race
// while the packed ones crawl. Wave tilts the density sinusoidally towards whichever end the bias
// names instead, with every band width held within a factor of the bias of another, which costs
// the extremes their drama and keeps all boundary speeds inside that factor.
// Either curve is the straight mapping at 1.00. Taken on the wrapped ratio in float: every step
// after it, the entry index and the blend between two entries, is float anyway, and the double
// work above this point is what keeps the wrap exact.
double biased_ratio(double ratio) {
    float x = clamp(float(ratio), 0.0, 1.0);
    float b = clamp(palette_attr.cycle_bias, 0.01, 16.0);
    if (((palette_attr.smoothing >> 9) & 1u) == uint(CYCLE_CURVE_WAVE)) {
        // x + A*sin(2*pi*x)/(2*pi), A = (b - 1) / (b + 1): monotone for |A| < 1 and periodic, so
        // it meets the wrap with its value and its slope both intact. The slope spans
        // 1 - |A| to 1 + |A|, whose ratio is exactly b - the widest band over the narrowest.
        float amp = (b - 1.0) / (b + 1.0);
        return double(x + amp * sin(TWO_PI * x) / TWO_PI);
    }
    if (b == 1.0) {
        return ratio;
    }
    return double(pow(x, b));
}

// The sRGB transfer function itself, not the 2.2 power that stands in for it: the standard's linear
// segment keeps the bottom of the range the power curve crushes, which is where two dark palette
// colors are blended. Duplicated in vk_slope.frag and vk_2_map_iter_stripe.comp; keep the three in step.
vec3 srgb_to_linear(vec3 c) {
    vec3 s = max(c, 0.0);
    return mix(s / 12.92, pow((s + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), s));
}

vec3 linear_to_srgb(vec3 c) {
    vec3 l = max(c, 0.0);
    return mix(l * 12.92, 1.055 * pow(l, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), l));
}

// OKLab: mixing in this perceptually uniform space holds the chroma an RGB mix of two strong hues loses to gray.
// Matrices from Björn Ottosson, "A perceptual color space for image processing" (2020), published as MIT / public domain.
vec3 linear_to_oklab(vec3 c) {
    mat3 m1 = mat3(0.4122214708, 0.2119034982, 0.0883024619,
                   0.5363325363, 0.6806995451, 0.2817188376,
                   0.0514459929, 0.1073969566, 0.6299787005);
    vec3 lms = pow(max(m1 * c, 0.0), vec3(1.0 / 3.0));
    mat3 m2 = mat3(0.2104542553,  1.9779984951,  0.0259040371,
                   0.7936177850, -2.4285922050,  0.7827717662,
                  -0.0040720468,  0.4505937099, -0.8086757660);
    return m2 * lms;
}

vec3 oklab_to_linear(vec3 lab) {
    mat3 m2i = mat3(1.0, 1.0, 1.0,
                    0.3963377774, -0.1055613458, -0.0894841775,
                    0.2158037573, -0.0638541728, -1.2914855480);
    vec3 lms = m2i * lab;
    lms = lms * lms * lms;
    mat3 m1i = mat3( 4.0767416621, -1.2684380046, -0.0041960863,
                    -3.3077115913,  2.6097574011, -0.7034186147,
                     0.2309699292, -0.3413193965,  1.7076147010);
    return m1i * lms;
}

// Blend between two adjacent palette entries only: the per-channel combine and the freeze matching are untouched.
vec4 blend_palette(vec4 cc, vec4 nc, float t) {
    if (((palette_attr.smoothing >> 8) & 1u) == uint(INTERP_RGB)) {
        return mix(cc, nc, t);
    }
    vec3 a = linear_to_oklab(srgb_to_linear(cc.rgb));
    vec3 b = linear_to_oklab(srgb_to_linear(nc.rgb));
    return vec4(clamp(linear_to_srgb(oklab_to_linear(mix(a, b, t))), 0.0, 1.0), mix(cc.a, nc.a, t));
}

// Static (un-animated) palette position [0,1) for a channel; identifies the color.
double static_ratio(double iteration, double interval, double inv_interval) {
    return mod(coloring_curve(div_r(smooth_iteration(iteration), interval, inv_interval)) + palette_attr.offset, 1);
}

float cycle_dist(double a, double b) {
    float d = float(abs(a - b));
    return min(d, 1.0 - d);
}

// Soft freeze weight in [0,1]: 1 = hold static, 0 = animate. Ramps smoothly across the tolerance
// band so the static/animated boundary fades instead of flipping (no flicker during zoom).
float freeze_weight(double iteration) {
    float tol = palette_attr.static_color_tolerance;
    if (palette_attr.static_color_count == 0u || tol <= 0.0) {
        return 0.0;
    }
    // The pixel's own three ratios do not depend on which frozen colour is being tested, so they
    // come out of the loop instead of being rebuilt once per frozen colour.
    double ir = static_ratio(iteration, g_interval.r, g_inv_interval.r);
    double ig = static_ratio(iteration, g_interval.g, g_inv_interval.g);
    double ib = static_ratio(iteration, g_interval.b, g_inv_interval.b);
    float w = 0.0;
    for (uint i = 0; i < palette_attr.static_color_count && i < MAX_STATIC_COLORS; i++) {
        double t = palette_attr.static_color_iterations[i];
        float md = max(cycle_dist(ir, static_ratio(t, g_interval.r, g_inv_interval.r)),
                   max(cycle_dist(ig, static_ratio(t, g_interval.g, g_inv_interval.g)),
                       cycle_dist(ib, static_ratio(t, g_interval.b, g_inv_interval.b))));
        w = max(w, 1.0 - smoothstep(0.0, tol, md));
    }
    return w;
}

float turbulence_field(vec2 p, float t) {
    const mat2 turn = mat2(0.8, -0.6, 0.6, 0.8);
    float result = 0.0;
    float amplitude = 0.55;
    for (int octave = 0; octave < 4; octave++) {
        result += amplitude * sin(p.x + sin(p.y + t)) * cos(p.y - cos(p.x - t));
        p = turn * p * 1.93 + vec2(0.71, -0.43) * t;
        t *= 1.37;
        amplitude *= 0.5;
    }
    return result / 1.03125;
}

// Fragment position within the whole canvas, which is the fragment itself outside a tiled export.
vec2 canvas_coord(vec2 coord) {
    return coord + vec2(iteration_info_attr.canvas_offset);
}

double animation_offset_iterations(vec2 rawCoord) {
    vec2 coord = canvas_coord(rawCoord);
    float t = time_attr.flow_phase;
    double offset_iterations = double(time_attr.palette_phase);
    if (palette_attr.animation_mode == ANIMATION_BREATHING) {
        offset_iterations += double(sin(t * float(TWO_PI)) * palette_attr.animation_flow_amount);
    }
    if (palette_attr.animation_mode == ANIMATION_TURBULENCE) {
        vec2 center = vec2(iteration_info_attr.canvas_extent) * 0.5;
        float extent = max(float(iteration_info_attr.canvas_extent.x), float(iteration_info_attr.canvas_extent.y));
        vec2 p = extent > 0.0 ? (coord - center) / extent : vec2(0.0);
        float field = turbulence_field(p * max(0.0, palette_attr.animation_flow_scale) * 8.0, t);
        offset_iterations += double(field * palette_attr.animation_flow_amount);
    }
    if (palette_attr.animation_mode == ANIMATION_PSYCHEDELIC) {
        vec2 center = vec2(iteration_info_attr.canvas_extent) * 0.5;
        float scale = max(float(iteration_info_attr.canvas_extent.x), float(iteration_info_attr.canvas_extent.y));
        vec2 p = scale > 0.0 ? (coord - center) / scale : vec2(0.0);
        float r = length(p);
        float a = atan(p.y, p.x);
        float s = max(0.0, palette_attr.animation_flow_scale);
        float swirl = palette_attr.animation_flow_swirl;
        float twisted = a + r * swirl * 8.0 + t * 0.8;
        float radial = sin((r * s - t) * float(TWO_PI));
        float spiral = sin((twisted / float(TWO_PI) + r * s * 0.75 + t) * float(TWO_PI));
        float liquid = sin((p.x * cos(t) + p.y * sin(t)) * s * float(TWO_PI) + sin((p.y - p.x) * s * 3.0 + t));
        float pulse = 0.75 + 0.25 * sin(t * float(TWO_PI) * 0.25);
        offset_iterations += double((radial * 0.4 + spiral * 0.4 + liquid * 0.2) * pulse * palette_attr.animation_flow_amount);
    }
    return offset_iterations;
}

vec4 get_color_channel(double iteration, double interval, double inv_interval, bool animate, double anim_iters, double warp_iters) {
    if (iteration == 0 || iteration >= iteration_info_attr.max_value) {
        return vec4(0.0); // Should be handled by discard in fetch
    }
    if (palette_attr.size == 0u) {
        return palette_attr.mandelbrot_color;
    }

    // The warp rides outside the animate branch: a frozen color is held against the palette's own
    // animation, not against a displacement that is part of the coloring itself.
    double time_term = animate ? div_r(anim_iters, interval, inv_interval) : 0.0;
    double timed_offset_ratio = palette_attr.offset - time_term + div_r(warp_iters, interval, inv_interval);
    // The bias rides the iteration term alone, so Offset and the animation still slide the palette
    // straight through the curved mapping instead of dragging the curve along with them.
    double iter_ratio = biased_ratio(mod(coloring_curve(div_r(smooth_iteration(iteration), interval, inv_interval)), 1));
    double palette_offset_ratio = mod(iter_ratio + timed_offset_ratio, 1);
    if (isnan(palette_offset_ratio) || isinf(palette_offset_ratio)) {
        return palette_attr.mandelbrot_color;
    }
    double palette_offset = palette_offset_ratio * double(palette_attr.size);
    float palette_offset_decimal = float(mod(palette_offset, 1));

    // mod() rounds to exactly 1 for a ratio a hair below zero, which would index one past the end.
    uint cpl = min(uint(palette_offset_ratio * palette_attr.size), palette_attr.size - 1u);
    uint npl = (cpl + 1) % palette_attr.size;

    vec4 cc = palette_attr.palette[cpl];
    vec4 nc = palette_attr.palette[npl];

    return blend_palette(cc, nc, palette_offset_decimal);
}

vec4 get_color(double iteration, bool animate, double anim_iters, double warp_iters) {

    if (iteration == 0) {
        return palette_attr.mandelbrot_color;
    }
    if (iteration >= iteration_info_attr.max_value) {
        return palette_attr.mandelbrot_color;
    }

    float fi = float(iteration);
    if (isnan(fi) || isinf(fi)) {
        return palette_attr.mandelbrot_color;
    }

    vec4 cr = get_color_channel(iteration, g_interval.r, g_inv_interval.r, animate, anim_iters, warp_iters);
    vec4 cg = get_color_channel(iteration, g_interval.g, g_inv_interval.g, animate, anim_iters, warp_iters);
    vec4 cb = get_color_channel(iteration, g_interval.b, g_inv_interval.b, animate, anim_iters, warp_iters);
    vec4 ca = get_color_channel(iteration, g_interval.a, g_inv_interval.a, animate, anim_iters, warp_iters);

    return vec4(cr.r, cg.g, cb.b, ca.a);
}

// Palette cycle position, matching get_color_channel's R-channel math so a decor layer rides the
// same bands as the color animation. palette_follow scales how much of that animation U inherits.
// The tiling wrap is applied after the scale, which keeps a non-integer repeat seamless.
// period overrides the cycle length, letting the layer keep its size when the coloring changes.
// Shared by the image texture and the generated pattern, which carry their own copy of each param.
// warp_iters bends the band this rides exactly as it bends the coloring, so a warped view carries
// the layer along with the bands instead of leaving it on straight ones they no longer follow.
float decor_cycle_ratio(double iteration, float scale, double anim_iters, float period, float palette_follow,
                        double warp_iters) {
    float interval = period > 0.0 ? period : palette_attr.interval.r;
    // The layer's own interval, so this reciprocal is not g_inv_interval's; taking it once still
    // turns the two divisions below into one.
    double ivd = double(interval);
    double inv_interval = 1.0 / ivd;
    double time_term = div_r(anim_iters, ivd, inv_interval);
    double raw = biased_ratio(mod(coloring_curve(div_r(smooth_iteration(iteration) + warp_iters, ivd, inv_interval)), 1))
               + palette_attr.offset - time_term * double(palette_follow);
    return float(mod(raw * double(scale), 1));
}

// Bounds-clamped fetch matching get_iteration's row flip, for neighbour taps.
double fetch_iteration(ivec2 c) {
    ivec2 e = ivec2(iteration_info_attr.extent);
    c.x = clamp(c.x, 0, e.x - 1);
    c.y = clamp(c.y, 0, e.y - 1);
    return iteration_attr.iterations[(e.y - 1 - c.y) * e.x + c.x];
}

// Iteration gradient in iterations per pixel. Interior neighbours hold no usable value, so those
// sides fall back to a one-sided difference against the centre pixel.
vec2 iteration_gradient(ivec2 c, double center) {
    double mv = iteration_info_attr.max_value;
    double l = fetch_iteration(c + ivec2(-1, 0));
    double r = fetch_iteration(c + ivec2(1, 0));
    double d = fetch_iteration(c + ivec2(0, -1));
    double u = fetch_iteration(c + ivec2(0, 1));
    bool lv = l > 0 && l < mv;
    bool rv = r > 0 && r < mv;
    bool dv = d > 0 && d < mv;
    bool uv = u > 0 && u < mv;

    double gx = lv && rv ? (r - l) * 0.5 : (rv ? r - center : (lv ? center - l : 0));
    double gy = uv && dv ? (u - d) * 0.5 : (uv ? u - center : (dv ? center - d : 0));
    return vec2(float(gx), float(gy));
}

// Repeats of a V source that closes on itself. Only a whole number of them meets where the angle
// wraps; a fractional one leaves a step of fract(scale) along that seam, which cuts every shape
// standing on it. Rounded here rather than refused in the settings, so a look saved with a
// fractional value loses the seam rather than the row.
float angular_repeats(float scale) {
    return round(scale);
}

// Direction the field climbs, as a 0..1 ratio - not a distance along the band. It carries no
// absolute position on purpose: a position term would amplify every turn of the gradient by the
// distance from the screen center and tear the texture apart.
//
// The cost of that is worth knowing. Where the bands run parallel the gradient points the same way
// all along them, so this is constant along a band and only changes across one. A field read
// through it therefore cannot vary along a locally straight band, which is why Cycle x Band alone
// will not bend one along its own length; Cycle x Screen and Screen do change in two directions.
float band_ratio(vec2 g, float scale) {
    if (g.x == 0 && g.y == 0) {
        return 0.0;
    }
    float ang = atan(g.y, g.x) / float(TWO_PI) + 0.5;
    return fract(ang * angular_repeats(scale));
}

// Accumulated scroll, in UV, for one texture layer. The host adds up speed x elapsed and sends the
// sum, so a scroll speed changed while the layer is moving does not carry the whole elapsed time
// with it and jump the UV.
vec2 texture_scroll(int i) {
    switch (i) {
        case 1: return vec2(time_attr.texture_scroll_u_1, time_attr.texture_scroll_v_1);
        case 2: return vec2(time_attr.texture_scroll_u_2, time_attr.texture_scroll_v_2);
        case 3: return vec2(time_attr.texture_scroll_u_3, time_attr.texture_scroll_v_3);
        default: return vec2(time_attr.texture_scroll_u_0, time_attr.texture_scroll_v_0);
    }
}

// The same, for one pattern layer.
vec2 pattern_scroll(int i) {
    switch (i) {
        case 1: return vec2(time_attr.pattern_scroll_u_1, time_attr.pattern_scroll_v_1);
        case 2: return vec2(time_attr.pattern_scroll_u_2, time_attr.pattern_scroll_v_2);
        case 3: return vec2(time_attr.pattern_scroll_u_3, time_attr.pattern_scroll_v_3);
        default: return vec2(time_attr.pattern_scroll_u_0, time_attr.pattern_scroll_v_0);
    }
}

// scroll is the accumulated offset the layer has already scrolled to, not a speed: the phase is
// added up on the host so that editing the speed does not move what is already drawn.
vec2 decor_uv(double iteration, vec2 coord, vec2 band_grad, double anim_iters, double warp_iters,
              uint uv_mode, float su, float sv, float period, float palette_follow, vec2 scroll) {
    // Screen-anchored modes measure against the whole canvas, so a tiled export does not repeat them.
    vec2 extent = vec2(iteration_info_attr.canvas_extent);
    vec2 screen = extent.x > 0.0 && extent.y > 0.0 ? canvas_coord(coord) / extent : vec2(0.0);
    vec2 base;
    switch (uv_mode) {
        case TEXTURE_UV_CYCLE_BAND: {
            base = vec2(decor_cycle_ratio(iteration, su, anim_iters, period, palette_follow, warp_iters),
                        band_ratio(band_grad, sv));
            break;
        }
        case TEXTURE_UV_CYCLE_SCREEN: {
            base = vec2(decor_cycle_ratio(iteration, su, anim_iters, period, palette_follow, warp_iters),
                        screen.y * sv);
            break;
        }
        case TEXTURE_UV_SCREEN: {
            base = screen * vec2(su, sv);
            break;
        }
        default: {
            vec2 p = screen - 0.5;
            base = vec2(decor_cycle_ratio(iteration, su, anim_iters, period, palette_follow, warp_iters),
                        (atan(p.y, p.x) / float(TWO_PI) + 0.5) * angular_repeats(sv));
            break;
        }
    }
    return base + scroll;
}

vec3 blend_layer(vec3 base, vec3 layer, uint blend_mode) {
    switch (blend_mode) {
        case TEXTURE_BLEND_REPLACE: return layer;
        case TEXTURE_BLEND_OVERLAY: return mix(2.0 * base * layer, 1.0 - 2.0 * (1.0 - base) * (1.0 - layer), step(0.5, base));
        default: return base * layer;
    }
}

// Half-width of the shape's smoothstep ramp. 0 leaves it a soft gradient, 1 cuts it to a hard edge.
float pattern_edge(float sharpness) {
    return mix(0.5, 0.01, clamp(sharpness, 0.0, 1.0));
}

// lowbias32 from Chris Wellons' hash-prospector (https://github.com/skeeto/hash-prospector), released into the public domain under the Unlicense.
uint pattern_hash_u32(uint x) {
    x ^= x >> 16u;
    x *= 0x7feb352du;
    x ^= x >> 15u;
    x *= 0x846ca68bu;
    x ^= x >> 16u;
    return x;
}

// One value in [0,1) per lattice cell. The argument is already integral, so rounding it is exact, and the top 24 bits are what a float can carry without rounding back up to 1.
float pattern_hash(vec2 p) {
    uvec2 u = uvec2(round(max(p, 0.0)));
    return float(pattern_hash_u32(u.x ^ pattern_hash_u32(u.y)) >> 8u) * (1.0 / 16777216.0);
}

// Value noise whose lattice wraps every "period" cells, so the cloud tiles with the unit UV cell.
float pattern_noise(vec2 p, float period) {
    vec2 i = floor(p);
    vec2 f = p - i;
    vec2 w = f * f * (3.0 - 2.0 * f);
    float a = pattern_hash(mod(i, period));
    float b = pattern_hash(mod(i + vec2(1.0, 0.0), period));
    float c = pattern_hash(mod(i + vec2(0.0, 1.0), period));
    float d = pattern_hash(mod(i + vec2(1.0, 1.0), period));
    return mix(mix(a, b, w.x), mix(c, d, w.x), w.y);
}

// Signed shape field: positive inside the ink, negative outside, zero exactly on the border. Every
// branch is periodic with period 1 on both axes, so the shape stays continuous where the cycle
// ratio wraps from 1 back to 0 instead of showing a seam there. Each is scaled so that thresholding
// it at +/- the ramp half-width reproduces the coverage this was cut out of, unchanged.
float pattern_field(vec2 uv, uint type) {
    float u = uv.x;
    float v = uv.y;
    switch (type) {
        case PATTERN_CHECKER: {
            return sin(u * float(TWO_PI)) * sin(v * float(TWO_PI));
        }
        case PATTERN_GRID: {
            float du = abs(fract(u) - 0.5) * 2.0;
            float dv = abs(fract(v) - 0.5) * 2.0;
            return max(du, dv) - 0.7;
        }
        case PATTERN_DOTS: {
            vec2 c = fract(uv) - 0.5;
            return (0.35 - length(c)) * 2.0;
        }
        case PATTERN_DIAMOND: {
            vec2 c = abs(fract(uv) - 0.5);
            return (0.35 - (c.x + c.y)) * 2.0;
        }
        case PATTERN_HONEYCOMB: {
            // Three waves 60 degrees apart at integer frequencies, so the lattice tiles exactly.
            float h = cos(float(TWO_PI) * 2.0 * u)
                    + cos(float(TWO_PI) * (u + 2.0 * v))
                    + cos(float(TWO_PI) * (u - 2.0 * v));
            return (h / 3.0 + 0.15) / 3.0;
        }
        case PATTERN_WAVES: {
            return sin(float(TWO_PI) * (u + 0.15 * sin(float(TWO_PI) * 2.0 * v)));
        }
        case PATTERN_CLOUD: {
            float f = 0.0;
            float amp = 0.5;
            float per = 4.0;
            for (int o = 0; o < 4; o++) {
                f += amp * pattern_noise(uv * per, per);
                per *= 2.0;
                amp *= 0.5;
            }
            return f / 0.9375 - 0.5;
        }
        default: {
            return sin(u * float(TWO_PI));
        }
    }
}

// Ink coverage in [0,1].
float pattern_coverage(float field, float sharpness) {
    float e = pattern_edge(sharpness);
    return smoothstep(-e, e, field);
}

// Weight of the outline in [0,1]. The band is centred on the border, so it eats equally into the
// ink and into the color beside it and reads as a seam between them. Its own ramp is the shape's,
// which keeps the outline as hard or as soft as the shape it follows. That ramp is also why width
// is allowed below 0: at 0 the band is still one ramp wide, and only a negative width draws the
// ramp back through the border to thin the line away to nothing.
//
// Relative width measures against these instead of against the raw field. Each shape climbs to a
// height of its own - a Honeycomb reaches 0.38 where a Diamond's outer side reaches 1.30 - and the
// two sides of one shape rarely match, so a width in raw units cuts a different part of every one
// of them. Divided through, the width that leaves only the peaks standing is one minus the shape's
// own soft border for all of them, which is the setting the outline breaks into isolated grain at.
float pattern_field_peak(uint type, bool positive) {
    switch (type) {
        case PATTERN_GRID: return positive ? 0.3 : 0.7;
        case PATTERN_DOTS: return positive ? 0.7 : 0.7142136;
        case PATTERN_DIAMOND: return positive ? 0.7 : 1.3;
        case PATTERN_HONEYCOMB: return positive ? 0.3833333 : 0.1166667;
        case PATTERN_CLOUD: return 0.5;
        default: return 1.0;
    }
}

float pattern_outline(float field, float sharpness, float width, uint type, bool relative) {
    float e = pattern_edge(sharpness);
    float f = abs(field);
    if (relative) {
        f /= max(pattern_field_peak(type, field >= 0.0), 1.0e-4);
    }
    return 1.0 - smoothstep(width, width + e, f);
}

// fBm over the tiling value noise, normalized to 0..1. Each octave wraps with the unit UV cell, so
// the warp stays seamless where the color cycle wraps back on itself.
float warp_fbm(vec2 uv, int octaves) {
    float f = 0.0;
    float amp = 0.5;
    float per = 4.0;
    float norm = 0.0;
    for (int o = 0; o < octaves; o++) {
        f += amp * pattern_noise(uv * per, per);
        norm += amp;
        per *= 2.0;
        amp *= 0.5;
    }
    return norm > 0.0 ? f / norm : 0.5;
}

// Iterations the palette lookup is displaced by at this pixel. Reading a field over the same UV the
// decor layers use bends the color bands themselves, rather than painting anything over them.
double warp_offset(double iteration, vec2 coord, vec2 band_grad, double anim_iters) {
    if (texture_attr.warp_enabled == 0u || texture_attr.warp_amount == 0.0) {
        return 0.0;
    }
    // Read off the unbent bands, with a warp of zero: this is the displacement that bends them.
    vec2 uv = decor_uv(iteration, coord, band_grad, anim_iters, 0.0, texture_attr.warp_uv_mode,
                       texture_attr.warp_scale_u, texture_attr.warp_scale_v,
                       texture_attr.warp_period, texture_attr.warp_palette_follow,
                       vec2(time_attr.warp_scroll_u, time_attr.warp_scroll_v));
    float field;
    if (texture_attr.warp_source == uint(WARP_SOURCE_NOISE)) {
        int octaves = clamp(int(texture_attr.warp_octaves + 0.5), 1, WARP_MAX_OCTAVES);
        field = warp_fbm(uv, octaves);
    } else {
        vec4 src = sample_texture_layer(int(texture_attr.warp_source) - 1, uv);
        // A transparent part of the source holds no field, so it falls back to the midpoint, which
        // is the value that displaces nothing rather than the hidden color under it.
        field = mix(0.5, dot(src.rgb, vec3(0.2126, 0.7152, 0.0722)), src.a);
    }
    // Centred on the field's midpoint so the warp pushes both ways and the mean coloring is unmoved.
    return double((field - 0.5) * 2.0 * texture_attr.warp_amount * palette_attr.interval.r);
}

// Callers must exclude interior pixels; only the escaping region is textured. The layers are blended
// bottom-up in index order, each over the result of the ones below it.
vec4 apply_texture(vec4 base, double iteration, vec2 coord, vec2 band_grad, double anim_iters,
                   double warp_iters) {
    for (int i = 0; i < TEXTURE_LAYERS; i++) {
        DecorLayer l = texture_layer(i);
        if (l.enabled == 0u || l.opacity <= 0.0) {
            continue;
        }
        // Nothing of the image is left to draw at or below zero.
        if (l.size <= 0.0) {
            continue;
        }
        vec2 uv = decor_uv(iteration, coord, band_grad, anim_iters, warp_iters, l.uv_mode, l.scale_u,
                           l.scale_v, l.period, l.palette_follow, texture_scroll(i));
        // Size is how much of the cell one repeat spans the image fills, centred in it, so it is drawn larger or smaller without changing how many of it there are.
        vec2 fit = vec2(l.size);
        if (l.keep_aspect != 0u) {
            // Fitted inside the cell rather than by reshaping it: reshaping changes the whole number of repeats a V that closes on itself rounds to, which moves every copy.
            bool closed = l.uv_mode == TEXTURE_UV_CYCLE_BAND || l.uv_mode == TEXTURE_UV_CYCLE_ANGLE;
            float vrep = closed ? angular_repeats(l.scale_v) : l.scale_v;
            float cell = l.scale_u > 0.0 && vrep > 0.0 ? vrep / l.scale_u : 1.0;
            // Screen measures both axes in pixels, so the canvas's own shape is part of the cell's.
            vec2 canvas = vec2(iteration_info_attr.canvas_extent);
            if (l.uv_mode == TEXTURE_UV_SCREEN && canvas.x > 0.0 && canvas.y > 0.0) {
                cell *= canvas.x / canvas.y;
            }
            float ratio = texture_layer_aspect(i) / cell;
            fit = ratio >= 1.0 ? vec2(l.size, l.size / ratio) : vec2(l.size * ratio, l.size);
        }
        vec2 tuv = (fract(uv) - 0.5) / fit + 0.5;
        // Below 1 the image no longer reaches the edges of its cell, and what is under it stands there instead.
        if (any(lessThan(tuv, vec2(0.0))) || any(greaterThan(tuv, vec2(1.0)))) {
            continue;
        }
        vec4 tex = sample_texture_layer(i, tuv);
        base = vec4(mix(base.rgb, blend_layer(base.rgb, tex.rgb, l.blend_mode), l.opacity * tex.a), base.a);
    }
    return base;
}

// Callers must exclude interior pixels; only the escaping region is patterned. The layers are
// blended bottom-up in index order, each over the result of the ones below it, so one layer can
// draw a shape while another cuts the same shape down to the grain its outline leaves.
vec4 apply_pattern(vec4 base, double iteration, vec2 coord, vec2 band_grad, double anim_iters,
                   double warp_iters) {
    for (int i = 0; i < PATTERN_LAYERS; i++) {
        PatternLayer p = pattern_layer(i);
        if (p.enabled == 0u || p.opacity <= 0.0) {
            continue;
        }
        vec2 uv = decor_uv(iteration, coord, band_grad, anim_iters, warp_iters, p.uv_mode, p.scale_u, p.scale_v,
                           p.period, p.palette_follow, pattern_scroll(i));
        float field = pattern_field(uv, p.type);
        float m = pattern_coverage(field, p.sharpness);
        vec3 ink = vec3(p.color_r, p.color_g, p.color_b);
        if (p.ink_mode == PATTERN_INK_PALETTE_SHIFT) {
            // Riding the animation offset rotates the palette lookup without touching the interior and
            // out-of-range guards get_color already applies. R is shifted by exactly the slider amount;
            // G and B span different intervals, so they land elsewhere and the ink comes out colored
            // rather than a plain lightness step.
            double shift = double(p.palette_shift * palette_attr.interval.r);
            ink = get_color(iteration, true, anim_iters + shift, warp_iters).rgb;
        }
        // Coverage doubles as the layer's alpha, so an uncovered pixel is untouched under every blend mode.
        vec3 painted = mix(base.rgb, blend_layer(base.rgb, ink, p.blend_mode), p.opacity * m);
        if (p.edge_enabled != 0u && p.edge_opacity > 0.0) {
            // Laid over the painted result rather than blended into it: the outline separates the two
            // sides, so folding it through the pattern's own blend mode would tint it with what it is
            // meant to be dividing.
            vec3 edge_ink = vec3(p.edge_color_r, p.edge_color_g, p.edge_color_b);
            float w = pattern_outline(field, p.sharpness, p.edge_width, p.type, p.edge_relative != 0u);
            painted = mix(painted, edge_ink, w * p.edge_opacity * p.opacity);
        }
        base = vec4(painted, base.a);
    }
    return base;
}

double get_iteration(uvec2 iterCoord) {
    iterCoord.y = iteration_info_attr.extent.y - 1 - iterCoord.y;
    return iteration_attr.iterations[iterCoord.y * iteration_info_attr.extent.x + iterCoord.x];
}

void main() {

    g_interval = dvec4(palette_attr.interval);
    g_inv_interval = 1.0 / g_interval;

    uvec2 iter_coord = uvec2(gl_FragCoord.xy);

    float x = iter_coord.x;
    float y = iter_coord.y;

    double iteration = get_iteration(iter_coord);

    if (iteration == 0) {
        color = palette_attr.mandelbrot_color;
        return;
    }

    bool any_texture = any_texture_enabled();
    bool any_pattern = any_pattern_enabled();

    double anim_iters = animation_offset_iterations(gl_FragCoord.xy);
    // Band-aligned decor UV needs the iteration gradient. Taken once here: every decor layer and the
    // warp can ask for it, and each of them used to pay for four taps of its own.
    vec2 band_grad = vec2(0);
    if (any_texture_needs_band() || any_pattern_needs_band() ||
        (texture_attr.warp_enabled != 0u && texture_attr.warp_uv_mode == TEXTURE_UV_CYCLE_BAND)) {
        band_grad = iteration_gradient(ivec2(gl_FragCoord.xy), iteration);
    }
    double warp_iters = warp_offset(iteration, gl_FragCoord.xy, band_grad, anim_iters);
    float fw = freeze_weight(iteration);
    vec4 animated = get_color(iteration, true, anim_iters, warp_iters);
    color = fw > 0.0 ? mix(animated, get_color(iteration, false, anim_iters, warp_iters), fw) : animated;

    if (iteration < iteration_info_attr.max_value) {
        if (any_texture) {
            color = apply_texture(color, iteration, gl_FragCoord.xy, band_grad, anim_iters, warp_iters);
        }
        if (any_pattern) {
            color = apply_pattern(color, iteration, gl_FragCoord.xy, band_grad, anim_iters, warp_iters);
        }
    }
}
