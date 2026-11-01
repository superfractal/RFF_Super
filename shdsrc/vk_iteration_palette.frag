//
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-10, 2026-08-13, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-20, 2026-08-21, 2026-08-22, 2026-08-25, 2026-08-26, 2026-08-27, 2026-08-31
// Modified by GPT-5 on 2026-08-21, 2026-08-23
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-06
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-16, 2026-09-23
// Modified by Opus 5.5 on 2026-10-07, 2026-10-12, 2026-10-16, 2026-10-22, 2026-10-24, 2026-10-25, 2026-10-29, 2026-11-01
//

#version 450
#extension GL_GOOGLE_include_directive : require
#include "shader_layer.glsl"
#define PALETTE_DATA_SET 1
#define PALETTE_TIME_SET 2
#define PALETTE_TEXTURE_SET 3
#include "palette_sampling.glsl"
#include "orbit_trap_relief.glsl"

#define ESCAPE_ANGLE_NONE 0u
#define ESCAPE_ANGLE_BINARY_DECOMPOSITION 1u
#define ESCAPE_ANGLE_TWIST 2u

// arg z where each pixel escaped (radians, iteration-buffer layout); 0 marks no angle, and an unfilled buffer is one element long.
layout (set = 0, binding = 4) buffer EscapeAngleSSBO {
    float angles[];
} escape_angle_attr;

// Escape Angle palette shift in iterations: binary decomposition (Peitgen & Saupe 1988, see NOTICE) moves Im z < 0 by Amount cycles, twist moves Amount cycles per turn.
double escape_angle_offset(uvec2 coord) {
    uint mode = palette_attr.escape_angle_mode;
    uint pixels = iteration_info_attr.extent.x * iteration_info_attr.extent.y;
    if (mode == ESCAPE_ANGLE_NONE || uint(escape_angle_attr.angles.length()) < pixels) {
        return 0.0;
    }
    coord.y = iteration_info_attr.extent.y - 1 - coord.y;
    float angle = escape_angle_attr.angles[coord.y * iteration_info_attr.extent.x + coord.x];
    float cycles = 0.0;
    if (mode == ESCAPE_ANGLE_BINARY_DECOMPOSITION) {
        cycles = angle < 0.0 ? palette_attr.escape_angle_amount : 0.0;
    } else if (mode == ESCAPE_ANGLE_TWIST) {
        cycles = palette_attr.escape_angle_amount * angle / float(TWO_PI);
    }
    return double(cycles) * g_interval.r;
}

#define ORBIT_TRAP_NONE 0u
#define ORBIT_TRAP_CROSS_GLOW 1u
#define ORBIT_TRAP_CELLULAR 2u
#define ORBIT_TRAP_SHAPE 3u
#define ORBIT_TRAP_BLEND_MIX 0u
#define ORBIT_TRAP_BLEND_MULTIPLY 1u
#define ORBIT_TRAP_BLEND_SCREEN 2u
#define ORBIT_TRAP_BLEND_ADD 3u

// OrbitTrap per pixel, plane-major: log2 axis distance, log2 nearest and second nearest origin distance, then 1 + iteration (0 = no data).
// Under Shape four more follow: log2 shape distance, 1 + its iteration, its angle, then its status (0 = no data, 1 = valid).
layout (set = 0, binding = 6) buffer OrbitTrapSSBO {
    float traps[];
} orbit_trap_attr;

// One plane's log2 distance at a pixel, clamped to the image and to the range an exact hit and a miss are stored at.
float orbit_trap_log2_at(ivec2 c, uint plane, uint pixels) {
    c = clamp(c, ivec2(0), ivec2(iteration_info_attr.extent) - 1);
    uint index = (iteration_info_attr.extent.y - 1u - uint(c.y)) * iteration_info_attr.extent.x + uint(c.x);
    return clamp(orbit_trap_attr.traps[index + plane * pixels], -4096.0, 64.0);
}

// The pixel's color under the orbit trap, mixed over base by Amount; base comes back unchanged without trap data.
vec4 apply_orbit_trap(vec4 base, uvec2 coord, double iteration, double anim_iters) {
    uint mode = palette_attr.orbit_trap_mode;
    uint pixels = iteration_info_attr.extent.x * iteration_info_attr.extent.y;
    bool shape = mode == ORBIT_TRAP_SHAPE;
    if (mode == ORBIT_TRAP_NONE || iteration_info_attr.orbit_trap_planes.x < (shape ? 8u : 4u)) {
        return base;
    }
    bool inside = iteration >= iteration_info_attr.max_value;
    // Region 0 is Outside Only and 2 is Inside Only.
    if ((palette_attr.orbit_trap_interior == 0u && inside) || (palette_attr.orbit_trap_interior == 2u && !inside)) {
        return base;
    }
    ivec2 pixel = ivec2(coord);
    coord.y = iteration_info_attr.extent.y - 1 - coord.y;
    uint index = coord.y * iteration_info_attr.extent.x + coord.x;
    float nearest_iteration = orbit_trap_attr.traps[index + 3u * pixels];
    // A shape the orbit never sampled draws as with the trap off.
    if ((shape ? orbit_trap_attr.traps[index + 7u * pixels] : nearest_iteration) == 0.0) {
        return base;
    }
    float width = palette_attr.orbit_trap_width;
    float density = palette_attr.orbit_trap_density;
    float sharpness = palette_attr.orbit_trap_glow_sharpness;
    vec3 glow_light = palette_attr.orbit_trap_glow_strength * palette_attr.orbit_trap_glow_color.rgb;
    float shift = palette_attr.orbit_trap_offset + time_attr.orbit_trap_phase;
    // Hold leaves the trap on the still palette while its own phase keeps running.
    double trap_anim = palette_attr.orbit_trap_hold != 0u ? 0.0LF : anim_iters;
    vec4 trap;
    if (mode == ORBIT_TRAP_CROSS_GLOW || shape) {
        // Shape glows along its own distance exactly as Cross Glow does along the axes'.
        float cross_log2 = orbit_trap_attr.traps[index + (shape ? 4u : 0u) * pixels];
        // Logistic falloff in log2 distance: 1/(1 + (d/w)^sharpness), kept in log space so deep-zoom distances do not underflow.
        float glow = 1.0 / (1.0 + exp2(clamp(sharpness * (cross_log2 - width), -64.0, 64.0)));
        float cycles = -0.1 * density * cross_log2 + shift;
        uint mapping = shape ? palette_attr.orbit_trap_mapping : ORBIT_TRAP_MAPPING_DISTANCE;
        if (mapping != ORBIT_TRAP_MAPPING_DISTANCE) {
            float angle = mapping == ORBIT_TRAP_MAPPING_ANGLE ? orbit_trap_attr.traps[index + 6u * pixels] : 0.0;
            cycles = trap_mapping_cycles(mapping, cross_log2, angle, palette_attr.orbit_trap_arms, density) + shift;
        }
        trap = get_color(1.0, true, trap_anim, double(cycles) * g_interval.r);
        if (mapping == ORBIT_TRAP_MAPPING_BANDS) {
            trap.rgb *= trap_band_seam(cycles);
        }
        if (palette_attr.orbit_trap_relief != 0u) {
            // Relief: outside the tube only its shadow falls on base; inside, the lit tube is blended as the glow would be.
            float t = exp2(clamp(cross_log2 - width, -64.0, 64.0));
            if (t >= 1.0) {
                return vec4(base.rgb * trap_relief_shadow(t, clamp(palette_attr.orbit_trap_amount, 0.0, 1.0)), base.a);
            }
            uint plane = shape ? 4u : 0u;
            vec2 grad = vec2(orbit_trap_log2_at(pixel + ivec2(1, 0), plane, pixels) - orbit_trap_log2_at(pixel - ivec2(1, 0), plane, pixels),
                             orbit_trap_log2_at(pixel + ivec2(0, 1), plane, pixels) - orbit_trap_log2_at(pixel - ivec2(0, 1), plane, pixels));
            vec3 light = trap_relief_light(palette_attr.orbit_trap_light_azimuth, palette_attr.orbit_trap_light_zenith);
            trap.rgb = trap_relief_shade(trap.rgb, t, grad, light, glow_light);
        } else {
            trap.rgb = trap.rgb * (0.25 + 0.75 * glow) + glow_light * (glow * glow * glow);
        }
    } else {
        float point_log2 = orbit_trap_attr.traps[index + pixels];
        float second_log2 = orbit_trap_attr.traps[index + 2u * pixels];
        // F1/F2 cells after Steven Worley, "A Cellular Texture Basis Function" (SIGGRAPH 1996); written independently, see NOTICE.
        // d1/d2 runs from 0 at a cell's center to 1 on the border where the two nearest approaches tie.
        float ratio = exp2(clamp(point_log2 - second_log2, -64.0, 0.0));
        float cycles = density * fract((nearest_iteration - 1.0) / 12.0) + shift;
        trap = get_color(1.0, true, trap_anim, double(cycles) * g_interval.r);
        float halo = 1.0 / (1.0 + exp2(clamp(sharpness * (point_log2 - width + 2.7), -64.0, 64.0)));
        // Border Width 1 darkens by ratio^2; a wider border lowers the power so the darkening reaches further in.
        float border = pow(ratio, 2.0 / max(palette_attr.orbit_trap_cell_border_width, 0.01));
        trap.rgb = trap.rgb * (1.0 - palette_attr.orbit_trap_cell_border_darkness * border) + glow_light * (halo / 1.2);
    }
    float amount = clamp(palette_attr.orbit_trap_amount, 0.0, 1.0);
    vec3 blended;
    switch (palette_attr.orbit_trap_blend) {
        case ORBIT_TRAP_BLEND_MULTIPLY: blended = mix(base.rgb, base.rgb * trap.rgb, amount); break;
        case ORBIT_TRAP_BLEND_SCREEN: blended = mix(base.rgb, 1.0 - (1.0 - base.rgb) * (1.0 - trap.rgb), amount); break;
        case ORBIT_TRAP_BLEND_ADD: blended = base.rgb + trap.rgb * amount; break;
        default: blended = mix(base.rgb, trap.rgb, amount); break;
    }
    return vec4(blended, base.a);
}

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
    // The angle shift rides with the warp on the color only; the texture and pattern layers below stay on the iteration bands.
    double color_iters = warp_iters + escape_angle_offset(iter_coord);
    float fw = freeze_weight(iteration);
    vec4 animated = get_color(iteration, true, anim_iters, color_iters);
    color = fw > 0.0 ? mix(animated, get_color(iteration, false, anim_iters, color_iters), fw) : animated;
    color = apply_orbit_trap(color, iter_coord, iteration, anim_iters);

    if (iteration < iteration_info_attr.max_value) {
        if (any_texture) {
            color = apply_texture(color, iteration, gl_FragCoord.xy, band_grad, anim_iters, warp_iters);
        }
        if (any_pattern) {
            color = apply_pattern(color, iteration, gl_FragCoord.xy, band_grad, anim_iters, warp_iters);
        }
    }
}
