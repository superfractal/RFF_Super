//
// Created by Opus 5.5 on 2026-10-24
// Modified by Opus 5.5 on 2026-11-17
// Modified by Gemini 4 Neon on 2026-11-19
//

// Orbit Trap Mapping and Relief, shared by vk_iteration_palette.frag and vk_2_map_iter_stripe.comp; reads no buffers.
// Original RFF_Super shading of the trap distance, GPL-3.0-or-later; the half vector is Blinn (1977), see NOTICE.
#ifndef RFF_ORBIT_TRAP_RELIEF
#define RFF_ORBIT_TRAP_RELIEF

#define ORBIT_TRAP_MAPPING_DISTANCE 0u
#define ORBIT_TRAP_MAPPING_ANGLE 1u
#define ORBIT_TRAP_MAPPING_BANDS 2u
#define ORBIT_TRAP_MAPPING_ITERATION 3u
#define ORBIT_TRAP_MAPPING_ITERATION_DISTANCE 4u
#define ORBIT_TRAP_MAPPING_POLAR_PETALS 5u
#define ORBIT_TRAP_MAPPING_POLAR_CHECKER 6u

// Mappings that read arg(z - c) from plane 6.
bool trap_mapping_needs_angle(uint mapping) {
    return mapping == ORBIT_TRAP_MAPPING_ANGLE || mapping == ORBIT_TRAP_MAPPING_POLAR_PETALS ||
           mapping == ORBIT_TRAP_MAPPING_POLAR_CHECKER;
}

// Mappings that read the hit iteration from plane 5.
bool trap_mapping_needs_iteration(uint mapping) {
    return mapping == ORBIT_TRAP_MAPPING_ITERATION || mapping == ORBIT_TRAP_MAPPING_ITERATION_DISTANCE ||
           mapping == ORBIT_TRAP_MAPPING_POLAR_PETALS || mapping == ORBIT_TRAP_MAPPING_POLAR_CHECKER;
}

// Palette cycles under non-Distance mappings, before the trap's offset; shade carries motif darkening in (0, 1].
float trap_mapping_cycles(uint mapping, float log2_distance, float width, float inside_trap, bool touch,
                          float angle, float shape_iteration, float arms, float density, out float shade) {
    shade = 1.0;
    if (mapping == ORBIT_TRAP_MAPPING_ANGLE) {
        return arms * angle * 0.15915494309;
    }
    if (mapping == ORBIT_TRAP_MAPPING_BANDS) {
        return -0.25 * density * log2_distance;
    }
    float iter_cycle = fract(max(shape_iteration - 1.0, 0.0) / 12.0);
    if (mapping == ORBIT_TRAP_MAPPING_ITERATION) {
        return density * iter_cycle;
    }
    if (mapping == ORBIT_TRAP_MAPPING_ITERATION_DISTANCE) {
        return touch ? density * (iter_cycle + 0.5 * inside_trap)
                     : density * (iter_cycle - 0.08 * log2_distance);
    }
    if (mapping == ORBIT_TRAP_MAPPING_POLAR_PETALS) {
        float u = exp2(clamp(log2_distance - width, -16.0, 4.0));
        float petal = sin(arms * angle + u * 2.8 * density);
        shade = 0.72 + 0.28 * abs(petal);
        return fract(max(shape_iteration - 1.0, 0.0) / 14.0) + 0.26 * petal - 0.10 * log2_distance;
    }
    if (mapping == ORBIT_TRAP_MAPPING_POLAR_CHECKER) {
        float rad_idx = floor(max(-4096.0, -log2_distance * 1.15 * density));
        float ang_idx = floor(((angle + 3.14159265) * 0.15915494309) * arms * 2.0);
        float chk = mod(rad_idx + ang_idx, 2.0);
        shade = chk >= 0.5 ? 1.0 : 0.44;
        return iter_cycle + (chk >= 0.5 ? 0.0 : 0.42);
    }
    return -0.25 * density * log2_distance;
}

// Bands: 1 inside a band, falling to 0.35 across the seam where one palette cycle meets the next.
float trap_band_seam(float cycles) {
    float f = fract(cycles);
    return 0.35 + 0.65 * clamp(min(f, 1.0 - f) / 0.06, 0.0, 1.0);
}

// orbitTrapBumpWiden in OrbitTrapShape.h: how far, in log2 of Width, the trap's colour reaches once its bump stands at
// height, out to where |height| (1 - (1 - e^-u)^2)^roundness falls to 0.1; 0 at height 0 or inside Width.
float orbit_trap_bump_widen(float height, float roundness) {
    float h = abs(height);
    if (!(h > 0.1) || !(roundness > 0.0)) return 0.0;
    float rise = pow(0.1 / h, 1.0 / roundness);
    float t = 1.0 - sqrt(max(1.0 - rise, 0.0));
    if (!(t > 0.0)) return 0.0;
    return max(log2(-log(t)), 0.0);
}

// Slope's light from its azimuth and zenith in degrees, on the axes Slope lights its own relief with.
vec3 trap_relief_light(float azimuth, float zenith) {
    float a = radians(azimuth);
    float z = radians(zenith);
    return vec3(cos(a) * sin(z), sin(a) * sin(z), cos(z));
}

// The trap as a round tube: t is the distance in tube radii (below 1 inside), grad the distance's slope across pixels.
vec3 trap_relief_shade(vec3 albedo, float t, vec2 grad, vec3 light, vec3 highlight) {
    float g = length(grad);
    vec2 across = g > 1e-20 ? grad / g : vec2(0.0);
    float tt = clamp(t, 0.0, 1.0);
    // The normal tips away from the trap's centerline and lies flat at the tube's edge.
    vec3 n = normalize(vec3(tt * across, sqrt(max(1.0 - tt * tt, 0.0)) + 1e-6));
    float diffuse = max(dot(n, light), 0.0);
    float specular = pow(max(dot(n, normalize(light + vec3(0.0, 0.0, 1.0))), 0.0), 60.0);
    float rim = pow(1.0 - n.z, 3.0);
    return albedo * (0.12 + 1.1 * diffuse + 0.4 * rim) + highlight * (1.5 * specular);
}

// The shadow the tube casts just outside itself, fading over about half a radius; amount is Orbit Trap Amount.
float trap_relief_shadow(float t, float amount) {
    return 1.0 - 0.55 * amount * exp(-min(t - 1.0, 16.0) * 4.0);
}

#endif
