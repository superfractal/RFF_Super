//
// Created by Opus 5.5 on 2026-10-24
//

// Orbit Trap Mapping and Relief, shared by vk_iteration_palette.frag and vk_2_map_iter_stripe.comp; reads no buffers.
// Original RFF_Super shading of the trap distance, GPL-3.0-or-later; the half vector is Blinn (1977), see NOTICE.
#ifndef RFF_ORBIT_TRAP_RELIEF
#define RFF_ORBIT_TRAP_RELIEF

#define ORBIT_TRAP_MAPPING_DISTANCE 0u
#define ORBIT_TRAP_MAPPING_ANGLE 1u
#define ORBIT_TRAP_MAPPING_BANDS 2u

// Palette cycles under Angle or Bands, before the trap's offset; Distance keeps its own expression in the callers.
float trap_mapping_cycles(uint mapping, float log2_distance, float angle, float arms, float density) {
    if (mapping == ORBIT_TRAP_MAPPING_ANGLE) {
        return arms * angle * 0.15915494309;
    }
    return -0.25 * density * log2_distance;
}

// Bands: 1 inside a band, falling to 0.35 across the seam where one palette cycle meets the next.
float trap_band_seam(float cycles) {
    float f = fract(cycles);
    return 0.35 + 0.65 * clamp(min(f, 1.0 - f) / 0.06, 0.0, 1.0);
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
