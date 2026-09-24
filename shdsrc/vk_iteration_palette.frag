//
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-10, 2026-08-13, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-20, 2026-08-21, 2026-08-22, 2026-08-25, 2026-08-26, 2026-08-27, 2026-08-31
// Modified by GPT-5 on 2026-08-21, 2026-08-23
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-06
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-16, 2026-09-23
//

#version 450
#extension GL_GOOGLE_include_directive : require
#include "shader_layer.glsl"
#define PALETTE_DATA_SET 1
#define PALETTE_TIME_SET 2
#define PALETTE_TEXTURE_SET 3
#include "palette_sampling.glsl"

void main() {

    g_interval = dvec4(palette_attr.interval);
    g_inv_interval = 1.0 / g_interval;

    uvec2 iter_coord = uvec2(gl_FragCoord.xy);

    double iteration = get_palette_iteration(iter_coord);

    if (iteration == 0) {
        color = palette_attr.mandelbrot_color;
        return;
    }

    bool any_texture = !ordered_layers() && SPEC_DECOR && any_texture_enabled();
    bool any_pattern = !ordered_layers() && SPEC_DECOR && any_pattern_enabled();

    double anim_iters = animation_offset_iterations(gl_FragCoord.xy);
    // Band-aligned decor UV needs the iteration gradient. Taken once here: every decor layer and the
    // warp can ask for it, and each of them used to pay for four taps of its own.
    vec2 band_grad = vec2(0);
    if (SPEC_DECOR && (any_texture_needs_band() || any_pattern_needs_band() ||
        (texture_attr.warp_enabled != 0u && texture_attr.warp_uv_mode == TEXTURE_UV_CYCLE_BAND))) {
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
