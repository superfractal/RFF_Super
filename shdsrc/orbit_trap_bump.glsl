//
// Created by Opus 5.5 on 2026-11-17
//

// Orbit Trap bump for vk_slope.frag: the trap raised into the relief's height field; original RFF_Super code, GPL-3.0-or-later.
// Needs studio_brdf from studio_ggx.glsl and openpbr_ggx_albedo from openpbr_surface.glsl for the Depth 0 light.
// Needs iteration_info_attr with trap_planes and trap_distance_source, orbit_trap_attr, trap_distance_attr, palette_attr and slope_attr.
#ifndef RFF_ORBIT_TRAP_BUMP
#define RFF_ORBIT_TRAP_BUMP
#include "orbit_trap_relief.glsl"

#define BUMP_TRAP_FAR_LOG2 64.0
#define BUMP_TRAP_CROSS_GLOW 1u
#define BUMP_TRAP_SHAPE 3u
#define BUMP_SOURCE_LIVE 1u
#define BUMP_SOURCE_MERGED 2u

// Cross Glow and Shape are the colorings measured as a distance to a line or shape.
bool orbit_trap_bump_on() {
    uint mode = palette_attr.orbit_trap_mode;
    return palette_attr.orbit_trap_bump_height != 0.0 && (mode == BUMP_TRAP_CROSS_GLOW || mode == BUMP_TRAP_SHAPE) &&
           iteration_info_attr.trap_distance_source.x != 0u;
}

// The trap's log2 distance at a pixel in get_iteration's coordinates, BUMP_TRAP_FAR_LOG2 where the pixel has none.
float orbit_trap_bump_log2(ivec2 coord) {
    ivec2 extent = ivec2(iteration_info_attr.extent);
    coord = clamp(coord, ivec2(0), extent - 1);
    uint index = uint(extent.y - 1 - coord.y) * uint(extent.x) + uint(coord.x);
    if (iteration_info_attr.trap_distance_source.x == BUMP_SOURCE_MERGED) {
        return trap_distance_attr.distances[index];
    }
    bool shape = palette_attr.orbit_trap_mode == BUMP_TRAP_SHAPE;
    if (iteration_info_attr.trap_planes.x < (shape ? 8u : 4u)) return BUMP_TRAP_FAR_LOG2;
    uint pixels = uint(extent.x) * uint(extent.y);
    // Plane 7 is Shape's status and plane 3 every trap's 1 + iteration; 0 in either marks a pixel without data.
    if (orbit_trap_attr.traps[index + (shape ? 7u : 3u) * pixels] == 0.0) return BUMP_TRAP_FAR_LOG2;
    return orbit_trap_attr.traps[index + (shape ? 4u : 0u) * pixels];
}

// The bump's height at a pixel in units of Height: (1 - (1 - t)^2)^Roundness, where t falls from 1 on the trap with
// u = distance / 2^Width: exp(-u) under Nearest, so the ridge tails off beyond Width, and 1 - u, ending at Width, under
// First and Last Touch. After the project owner's own Material Lab prototype; see NOTICE.
float orbit_trap_bump_profile(ivec2 coord) {
    float l = orbit_trap_bump_log2(coord);
    if (l >= BUMP_TRAP_FAR_LOG2) return 0.0;
    // Touches reach as far as the bump's colour (TrapShapeParams::touchWidth), so their profile ends there.
    bool touch = palette_attr.orbit_trap_statistic != 0u;
    float reach = palette_attr.orbit_trap_width +
                  (touch ? orbit_trap_bump_widen(palette_attr.orbit_trap_bump_height, palette_attr.orbit_trap_bump_roundness) : 0.0);
    float u = exp2(clamp(l - reach, -64.0, 8.0));
    float t = touch ? max(1.0 - u, 0.0) : exp(-u);
    float rise = 1.0 - (1.0 - t) * (1.0 - t);
    return rise <= 0.0 ? 0.0 : pow(rise, palette_attr.orbit_trap_bump_roundness);
}

// Screen-space bump scale: 14 per height unit per pixel at 1080 lines, so a trap some 17 pixels across reads as round
// at Height 1 while a wide region only swells; steep slopes are softened by s / (1 + 0.6 |s|) instead of cut off.
const float BUMP_SCALE = 14.0;
const float BUMP_COMPRESS = 0.6;

// The bump's slope (dz/dx, dz/dy) at a pixel, from the central difference of its height across the four neighbours.
vec2 orbit_trap_bump_slope(uvec2 coord) {
    if (!orbit_trap_bump_on()) return vec2(0.0);
    ivec2 c = ivec2(coord);
    vec2 grad = vec2(orbit_trap_bump_profile(c + ivec2(1, 0)) - orbit_trap_bump_profile(c - ivec2(1, 0)),
                     orbit_trap_bump_profile(c + ivec2(0, 1)) - orbit_trap_bump_profile(c - ivec2(0, 1)));
    float lines = float(min(iteration_info_attr.extent.x, iteration_info_attr.extent.y)) / 1080.0;
    grad *= 0.5 * palette_attr.orbit_trap_bump_height * BUMP_SCALE * lines;
    return grad / (1.0 + BUMP_COMPRESS * length(grad));
}

// Slope at Depth 0 lights the bump with the default rig of the project owner's Material Lab prototype (see NOTICE): its
// studio environment (three spherical Gaussian lights over a dim sky), a 2.2 key of colour #fff3e2 from Slope's Azimuth and
// Zenith and a 0.6 rim of colour #9fc0ff turned 165 degrees from it and 22 degrees up, on a dielectric of IOR 1.5 (F0 0.04)
// and roughness 0.3, with the key's cast shadow (0.55 deep, 140 long, softness 0.6) and contact shading (0.5 over 16) of the
// bump's own height. Slope's y runs opposite to the prototype's, so its directions are mirrored in y on the way in.
// The GGX lobe is studio_brdf and the environment's reflectance openpbr_ggx_albedo.
const float BUMP_KEY = 2.2;
const vec3 BUMP_KEY_COLOR = vec3(1.0, 0.896, 0.761);
const float BUMP_RIM = 0.6;
const vec3 BUMP_RIM_COLOR = vec3(0.346, 0.527, 1.0);
const float BUMP_RIM_TURN = -165.0;
const float BUMP_RIM_ELEVATION = 22.0;
const float BUMP_ROUGHNESS = 0.3;
const float BUMP_F0 = 0.04;
const float BUMP_SHADOW = 0.55;
const float BUMP_SHADOW_LENGTH = 140.0;
const float BUMP_SHADOW_SOFTNESS = 0.6;
const int BUMP_SHADOW_TAPS = 14;
const float BUMP_AO = 0.5;
const float BUMP_AO_RADIUS = 16.0;
const int BUMP_AO_TAPS = 8;

vec3 bump_light_direction(float azimuthDeg, float zenithDeg) {
    float a = radians(azimuthDeg);
    float z = radians(zenithDeg);
    return vec3(cos(a) * sin(z), sin(a) * sin(z), cos(z));
}

// One spherical Gaussian light of angular size `size`, widened by the roughness r it is seen through.
float bump_lobe(vec3 d, vec3 center, float size, float r) {
    float s2 = size * size + r * r * 0.7;
    return exp((dot(d, normalize(center)) - 1.0) / s2) * (size * size) / s2;
}

// The prototype's studio environment seen along d (Slope's axes) through roughness r; r = 1 gives the diffuse irradiance.
vec3 bump_studio(vec3 d, float r) {
    d = vec3(d.x, -d.y, d.z);
    float w = 0.12 + r * 0.9;
    vec3 c = mix(vec3(0.015, 0.016, 0.02), vec3(0.11, 0.11, 0.125), smoothstep(-w, w, d.z));
    c += vec3(1.0, 0.97, 0.93) * 7.0 * bump_lobe(d, vec3(-0.55, 0.5, 0.67), 0.36, r);
    c += vec3(0.62, 0.76, 1.0) * 2.2 * bump_lobe(d, vec3(0.85, -0.1, 0.5), 0.5, r);
    c += vec3(1.0, 0.72, 0.5) * 3.5 * bump_lobe(d, vec3(0.1, -0.95, 0.28), 0.22, r);
    return c;
}

// The bump's height at a pixel in the units the slope's 14-per-pixel scale works in.
float bump_height_units(ivec2 coord) {
    return BUMP_SCALE * palette_attr.orbit_trap_bump_height * orbit_trap_bump_profile(coord);
}

// The key light's cast shadow over the bump, 1 lit; the darkest clearance-over-distance along the march toward the light.
float bump_shadow(ivec2 c, vec3 key, float jitter, float lines) {
    float run = length(key.xy);
    if (run < 1.0e-4) return 1.0;
    vec2 toward = key.xy / run;
    float rise = key.z / run;
    float here = bump_height_units(c);
    float lit = 1.0;
    for (int i = 0; i < BUMP_SHADOW_TAPS; ++i) {
        float s = BUMP_SHADOW_LENGTH * pow((float(i) + jitter) / float(BUMP_SHADOW_TAPS), 1.6) + 1.0;
        float dz = bump_height_units(c + ivec2(floor(toward * s * lines + 0.5))) - here;
        lit = min(lit, clamp((s * rise - dz) / (s * BUMP_SHADOW_SOFTNESS) + 0.5, 0.0, 1.0));
    }
    return mix(1.0, lit, BUMP_SHADOW);
}

// Contact shading: how much the bump rises around a pixel within about BUMP_AO_RADIUS, 1 open.
float bump_occlusion(ivec2 c, float jitter, float lines) {
    float here = bump_height_units(c);
    float occluded = 0.0;
    for (int i = 0; i < BUMP_AO_TAPS; ++i) {
        float a = jitter * 6.2831853 + float(i) * 0.7853982;
        float s = BUMP_AO_RADIUS * (0.3 + 0.7 * fract(jitter * 7.0 + float(i) * 0.618034));
        float dz = bump_height_units(c + ivec2(floor(vec2(cos(a), sin(a)) * s * lines + 0.5))) - here;
        occluded += clamp(dz / s, 0.0, 1.0);
    }
    return 1.0 - BUMP_AO * clamp(occluded / float(BUMP_AO_TAPS) * 1.6, 0.0, 1.0);
}

// Diffuse light (to multiply the albedo by) and highlight a surface of normal n receives from the rig.
void bump_rig(vec3 n, vec3 key, vec3 rim, float shadow, float occlusion, out vec3 diffuse, out vec3 highlight) {
    vec3 v = vec3(0.0, 0.0, 1.0);
    float nv = clamp(n.z, 1.0e-3, 1.0);
    vec3 albedo = openpbr_ggx_albedo(nv, BUMP_ROUGHNESS * BUMP_ROUGHNESS);
    float reflectance = BUMP_F0 * albedo.x + albedo.y;
    float rimShadow = mix(1.0, occlusion, 0.5);
    diffuse = bump_studio(n, 1.0) * occlusion + BUMP_KEY * BUMP_KEY_COLOR * max(dot(n, key), 0.0) * shadow +
              BUMP_RIM * BUMP_RIM_COLOR * max(dot(n, rim), 0.0) * rimShadow;
    highlight = bump_studio(reflect(-v, n), BUMP_ROUGHNESS) * reflectance * mix(1.0, occlusion, 0.75) +
                3.14159265 * (BUMP_KEY * BUMP_KEY_COLOR * studio_brdf(n, v, key, BUMP_ROUGHNESS, 0.0, 0.0, vec3(BUMP_F0)).g * shadow +
                              BUMP_RIM * BUMP_RIM_COLOR * studio_brdf(n, v, rim, BUMP_ROUGHNESS, 0.0, 0.0, vec3(BUMP_F0)).g * rimShadow);
}

// The canvas color (linear) lit by the rig relative to flat open ground: flat ground keeps its color, the bump takes the
// rig's diffuse ratio with its shadow and contact shading, and the highlight above flat ground's own, in the rig's proportions.
// Light brighter than the canvas rolls off toward 1 from the larger of Highlight Knee and the canvas's own level.
vec3 orbit_trap_bump_lit(vec3 base, uvec2 coord) {
    ivec2 c = ivec2(coord);
    float lines = float(min(iteration_info_attr.extent.x, iteration_info_attr.extent.y)) / 1080.0;
    float jitter = relief_shadow_ign(vec2(coord) + 0.5);
    vec2 s = orbit_trap_bump_slope(coord);
    vec3 n = normalize(vec3(-s, 1.0));
    vec3 key = bump_light_direction(slope_attr.azimuth, slope_attr.zenith);
    vec3 rim = bump_light_direction(slope_attr.azimuth + BUMP_RIM_TURN, 90.0 - BUMP_RIM_ELEVATION);
    vec3 diffuse, highlight, flatDiffuse, flatHighlight;
    bump_rig(n, key, rim, bump_shadow(c, key, jitter, lines), bump_occlusion(c, jitter, lines), diffuse, highlight);
    bump_rig(vec3(0.0, 0.0, 1.0), key, rim, 1.0, 1.0, flatDiffuse, flatHighlight);
    vec3 lit = base * (diffuse / flatDiffuse) + max(highlight - flatHighlight, vec3(0.0)) / flatDiffuse;
    float level = max(max(base.r, base.g), base.b);
    float m = max(max(lit.r, lit.g), lit.b);
    float k = clamp(max(slope_attr.highlight_knee, level), 0.0, 0.999);
    if (m <= k) return lit;
    float over = m - k;
    return lit * ((k + over / (1.0 + over / (1.0 - k))) / m);
}

#endif
