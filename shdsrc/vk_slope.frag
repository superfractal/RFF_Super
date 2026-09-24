//
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-07, 2026-08-08, 2026-08-11, 2026-08-12, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-20, 2026-08-21, 2026-08-22, 2026-08-23, 2026-08-29
// Modified by GPT-5 on 2026-08-16, 2026-08-21, 2026-08-23, 2026-09-02
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-02
// Modified by GPT-6 on 2026-09-05, 2026-09-10, 2026-09-11, 2026-09-12, 2026-09-13, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-23, 2026-09-24
//

#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#include "shader_layer.glsl"
#include "studio_ggx.glsl"
#define PI 3.141592653589793238
#define SHADING_BLEND_OKLAB_LIGHTNESS 2
#define LIGHT_BLEND_LINEAR 1
#define TINT_BLEND_OKLAB 1
#define GLOSS_SOURCE_RELIEF 1
#define GLOSS_SOURCE_ASPECT 2
#define GLOSS_SOURCE_SHADING_FINE 3

layout (set = 0, binding = 0) uniform sampler2D canvas;

layout (set = 1, binding = 0) uniform IterUBO {
    uvec2 extent;
    double max_value;
    double max_value_normal;
    double max_value_zoomed;
    uvec2 canvas_extent;
    ivec2 canvas_offset;
} iteration_info_attr;

layout (set = 1, binding = 1) buffer IterSSBO {
    double iterations[];
} iteration_attr;

layout (set = 2, binding = 0) uniform SlopeUBO {
    float depth;
    float reflection_ratio;
    float opacity;
    float zenith;
    float azimuth;
    float specular_intensity;
    float specular_power;
    float rim_intensity;
    float rim_power;
    float brightness;
    float gamma;
    float rim_color_r;
    float rim_color_g;
    float rim_color_b;
    // Appended at the tail to keep existing offsets fixed (must match DescSlope reserve order).
    float specular_color_r;
    float specular_color_g;
    float specular_color_b;
    float ao_intensity;      // 0..1: ambient occlusion strength (darkens concave pits)
    float ambient_intensity; // 0..1: strength of the colored sky/ground ambient tint
    float sky_color_r;       // tint applied to light-facing (lit) areas
    float sky_color_g;
    float sky_color_b;
    float ground_color_r;    // tint applied to shadowed areas
    float ground_color_g;
    float ground_color_b;
    float specular_link;
    float specular_zenith;
    float specular_azimuth;
    float specular_anisotropy;
    float specular_anisotropy_angle;
    // Dual-scale relief (N3): blends a wider Sobel to add large-form undulation.
    float macro_relief;
    float macro_radius;
    // P6: composite mode the shading meets the palette color in.
    float shading_blend;
    // L1: the lighting rework. Appended at the tail like the block above.
    float relief_response;       // 0 = highlight tilt anchored on the half vector, 1 = the surface's own
    float terminator_softness;   // 0 = the original hard ambient floor, 1 = fully wrapped light
    float highlight_knee;        // linear level the highlight shoulder starts at; LIGHT_BLEND_LINEAR only
    float light_blend;           // where the highlight and rim are added; carried as a float like specular_link
    // L2: chromatic shading. Appended at the tail like the block above.
    float luma_amount;           // 1 = the directional shading as it was, 0 = no lightness shading at all
    float tint_response;         // curve on the lit/shadow mix the tint is read at
    float shadow_chroma;         // chroma the shadow side is scaled by; TINT_BLEND_OKLAB only
    float tint_blend;            // how the tint meets the color; carried as a float like specular_link
    // The fill light. Appended at the tail like the block above.
    float fill_intensity;        // 0 = the single key light, as every version before the fill drew
    float fill_zenith;
    float fill_azimuth;
    // The gloss. Appended at the tail like the block above it.
    float gloss_intensity;       // 0 = off, which is every version before the gloss
    float gloss_source;          // which relief coordinate the bands ride; carried as a float like specular_link
    float gloss_bands;           // bright bands across the coordinate's whole range
    float gloss_sharpness;       // exponent on each band; higher is a narrower line
    float gloss_phase;           // 0 - 1 slide of the bands along the coordinate
    float gloss_color_r;
    float gloss_color_g;
    float gloss_color_b;
    // Appended at the tail like the block above it.
    float gloss_relief;          // log2 gain on the gloss's own normal; GLOSS_SOURCE_SHADING_FINE only
    float studio_use;
    float studio_roughness;
    float studio_metalness;
    float studio_ior;
    float studio_direct_intensity;
    float studio_environment_intensity;
    float studio_clearcoat;
    float studio_clearcoat_roughness;
    float surface_iridescence;
    float surface_film_thickness;
    float surface_specular_aa;
    float surface_lustre_relief;
    float surface_relief_depth;
    float surface_normal_smooth;
    float surface_ao_radius;
    float surface_relief_waves;
    float surface_wave_frequency;
    float surface_invert_relief;
    float surface_boundary_guard;
    float surface_zoom_log2;
    float groove_enabled;
    float groove_depth;
    float groove_width;
    float groove_auto;
    float groove_count;
    float surface_style;
    float chrome_strength;
    float film_strength;
    float reflection_detail;
    float reflection_contrast;
    float reflection_brightness;
    float reflection_curve;
    float surface_phase;
    float prism_width;
    float prism_spread;
    float film_hue;
    float sparkle_strength;
    float background_brightness;
    float shadow_crush;
    float flame_strength;
    float phonk_red;
    float frost_strength;
    float frost_threshold;
    float grunge_scale;
    float palette_color_mix;
    float style_color_mix;
    float style_highlight_mix;
    float style_background_mix;
    float style_ink_preserve;
    float style_glint_size;
    float style_detail_light;
    float style_detail_suppress;
    float style_detail_threshold;
    float style_damage;
    float surface_blend;
    float style_color_r;
    float style_color_g;
    float style_color_b;
    float style_highlight_color_r;
    float style_highlight_color_g;
    float style_highlight_color_b;
    float style_background_color_r;
    float style_background_color_g;
    float style_background_color_b;
    float sea_glow;
    float sea_threshold;
    float sea_body;
    float sea_particles;
    float sea_particle_size;
    float sea_balance;
    float ukiyo_colors;
    float ukiyo_foam;
    float ukiyo_grain;
    float ukiyo_flatness;
    float ukiyo_balance;
    float sea_body_color_r;
    float sea_body_color_g;
    float sea_body_color_b;
    float sea_glow_color_r;
    float sea_glow_color_g;
    float sea_glow_color_b;
    float sea_accent_color_r;
    float sea_accent_color_g;
    float sea_accent_color_b;
    float print_ink_r;
    float print_ink_g;
    float print_ink_b;
    float print_indigo_r;
    float print_indigo_g;
    float print_indigo_b;
    float print_asagi_r;
    float print_asagi_g;
    float print_asagi_b;
    float print_blue_r;
    float print_blue_g;
    float print_blue_b;
    float print_foam_r;
    float print_foam_g;
    float print_foam_b;
    float print_paper_r;
    float print_paper_g;
    float print_paper_b;
    float layer_metal;
    float layer_sigil;
    float layer_phonk;
    float layer_frost;
    float layer_sea;
    float layer_print;
    float layer_common;
    float studio_environment_rotation;
    float studio_environment_follow;
    float replace_surface_style;


} slope_attr;

layout (location = 0) out vec4 color;

// Bounds-checked fetch in buffer (top-left origin) coordinates. Returns -1 as a
// sentinel for out-of-bounds so callers can substitute.
double fetch_buffer(ivec2 bcoord){
    if (bcoord.x < 0 || bcoord.y < 0 ||
        bcoord.x >= int(iteration_info_attr.extent.x) ||
        bcoord.y >= int(iteration_info_attr.extent.y)) {
        return -1.0;
    }
    return iteration_attr.iterations[bcoord.y * iteration_info_attr.extent.x + bcoord.x];
}

double get_iteration_raw(ivec2 coord){
    coord.y = int(iteration_info_attr.extent.y) - 1 - coord.y; // -> buffer coords
    int cx = int(iteration_info_attr.extent.x) / 2;
    int cy = int(iteration_info_attr.extent.y) / 2;
    bool onCol = (coord.x == cx);
    bool onRow = (coord.y == cy);
    if (onCol && onRow) {
        double a = fetch_buffer(ivec2(cx - 1, cy - 1));
        double b = fetch_buffer(ivec2(cx + 1, cy - 1));
        double c = fetch_buffer(ivec2(cx - 1, cy + 1));
        double d = fetch_buffer(ivec2(cx + 1, cy + 1));
        if (a >= 0.0 && b >= 0.0 && c >= 0.0 && d >= 0.0) return (a + b + c + d) * 0.25;
    } else if (onCol) {
        double a = fetch_buffer(ivec2(cx - 1, coord.y));
        double b = fetch_buffer(ivec2(cx + 1, coord.y));
        if (a >= 0.0 && b >= 0.0) return (a + b) * 0.5;
    } else if (onRow) {
        double a = fetch_buffer(ivec2(coord.x, cy - 1));
        double b = fetch_buffer(ivec2(coord.x, cy + 1));
        if (a >= 0.0 && b >= 0.0) return (a + b) * 0.5;
    }

    return fetch_buffer(coord);
}

double get_iteration(uvec2 iter_coord, ivec2 offset){
    ivec2 coord = ivec2(iter_coord) + offset;
    double value = get_iteration_raw(coord);

    if (value >= 0.0) {
        return value;
    }

    ivec2 mirror_coord = ivec2(iter_coord) - offset;
    double mirror_value = get_iteration_raw(mirror_coord);
    if (mirror_value >= 0.0) {
        return mirror_value;
    }

    // Both sides OOB (image smaller than 3 pixels in this axis). Fall back to
    // the center pixel.
    ivec2 center = ivec2(iter_coord);
    center.y = int(iteration_info_attr.extent.y) - 1 - center.y;
    if (center.x >= 0 && center.y >= 0 &&
        center.x < int(iteration_info_attr.extent.x) &&
        center.y < int(iteration_info_attr.extent.y)) {
        return iteration_attr.iterations[center.y * iteration_info_attr.extent.x + center.x];
    }
    return 0.0;
}

// log2(1 + x), as a series rather than through log().
//
// Vulkan pins log() to an ABSOLUTE error of 2^-21 for arguments in [0.5, 2], not a relative one.
// Every tap in a deep, smooth view sits a hair above 1, so log() is being asked for an answer of
// around 1e-7 while it is allowed to be wrong by 4.8e-7 - it may come back several times too large,
// or negative. Any log1p identity built on it therefore multiplies the height by a garbage factor
// right across the band of pixels whose gradient falls in that range, and since that band is
// bounded by an iso-gradient contour, it reaches the screen as filaments tracing the iteration
// bands. Every term below is reached from x by multiplication alone, so the relative accuracy holds
// however small x gets, and at x = 1e-8 the second term is already 1e-8 of the first.
float log2_1p(float x) {
    return x * (1.0 - x * (0.5 - x * (1.0 / 3.0 - x * (0.25 - x * 0.2)))) * 1.4426950408889634;
}

// Shading height of one tap in doublings, measured against the centre pixel instead of against
// zero. log2 keeps the relief independent of max iteration and zoom depth.
//
// Only differences of heights are ever taken from here - the Sobel weights sum to zero along each
// axis, and the AO term is the neighbour average minus the centre - so the shared log2(centre) is
// a constant that cancels, and leaving it out is what makes this hold its precision. Taking log2
// of the absolute count meant casting a double count down to a float first, and float carries 24
// bits: past about 8.4 million iterations its steps are a whole iteration wide, so the smooth
// fractional part that the relief is entirely made of was quantised away. The height field became
// a staircase, and the Sobel read zero across every flat tread and a spike along every riser,
// which drew thin dark dashes along the iteration bands - worse the deeper the zoom went, because
// the counts grew and the steps with them.
float height_rel(double it, double center) {
    double a = max(it, 1.0);
    double b = max(center, 1.0);
    // Sterbenz: while the two are within a factor of two the subtraction is exact in double, so
    // the ratio reaches float as a small number carrying its full relative precision - all 24 bits
    // spent on the difference itself rather than on the magnitude the two taps share.
    double dr = (a - b) / b;
    // The series is truncated at the fifth term, so it is worth 0.45% at dr = 0.5 and 0.66% at
    // -0.5, and the switch below would hand those over as a step. It is worth 7e-5 at 0.25, where
    // the far branch's own cancellation is smaller still, so the two meet with nothing to see.
    if (abs(dr) <= 0.25) {
        return log2_1p(float(dr));
    }
    // Far apart, which happens where an interior tap sits beside an exterior one. There is no
    // shared magnitude left to cancel; the gradient is enormous and the shading long since
    // saturated, so log2()'s absolute error bound costs nothing here.
    return log2(float(a)) - log2(float(b));
}

// One 8-tap Sobel taken at radius r, divided by r so it estimates the same gradient whatever
// the stencil width is. That normalisation is what lets different radii be averaged against
// each other, and what keeps the macro term continuous when r has to shrink near a border.
vec2 macro_sobel(uvec2 iter_coord, int r, double center) {
    float s_ld = height_rel(get_iteration(iter_coord, ivec2(-r, -r)), center);
    float s_d  = height_rel(get_iteration(iter_coord, ivec2( 0, -r)), center);
    float s_rd = height_rel(get_iteration(iter_coord, ivec2( r, -r)), center);
    float s_l  = height_rel(get_iteration(iter_coord, ivec2(-r,  0)), center);
    float s_r  = height_rel(get_iteration(iter_coord, ivec2( r,  0)), center);
    float s_lu = height_rel(get_iteration(iter_coord, ivec2(-r,  r)), center);
    float s_u  = height_rel(get_iteration(iter_coord, ivec2( 0,  r)), center);
    float s_ru = height_rel(get_iteration(iter_coord, ivec2( r,  r)), center);
    float inv = 1.0 / float(r);
    return vec2(((s_rd - s_ld) + 2.0 * (s_r - s_l) + (s_ru - s_lu)) * inv,
                ((s_lu - s_ld) + 2.0 * (s_u - s_d) + (s_ru - s_rd)) * inv);
}

// Wide-scale gradient for the dual-scale relief.
vec2 macro_gradient(uvec2 iter_coord, float multiplier, double center) {
    int req = max(int(slope_attr.macro_radius * multiplier + 0.5), 1);

    // Keep every tap inside the image. get_iteration() answers an out-of-bounds tap with the
    // mirrored one, which makes the stencil symmetric about the pixel: the vertical pair
    // collapses to a single value and the diagonal terms cancel, so the macro gradient dies and
    // paints a flat band macro_radius wide along all four borders. Shrinking the radius to fit
    // keeps every tap real instead, and the 1/r above keeps the scale continuous as it shrinks.
    int limX = min(int(iter_coord.x), int(iteration_info_attr.extent.x) - 1 - int(iter_coord.x));
    int limY = min(int(iter_coord.y), int(iteration_info_attr.extent.y) - 1 - int(iter_coord.y));
    int mr = clamp(min(limX, limY), 1, req);

    // Average four radii instead of point-sampling one. A lone ring at radius mr samples the
    // iteration field with stride mr and aliases it, and it re-reads any single bad row or column
    // from mr pixels away, reprinting it as a band. Stacking radii low-passes across scale at a
    // tap count that stays fixed as mr grows. At mr = 1 all four collapse onto the fine Sobel.
    int r0 = mr;
    int r1 = max((mr * 3) / 4, 1);
    int r2 = max(mr / 2, 1);
    int r3 = max(mr / 4, 1);
    // The radii collapse onto each other as mr shrinks - at mr = 1 all four are 1, and every ring
    // near a border lands on a neighbour - so a ring already taken is reused instead of resampled.
    // The four terms and their order are unchanged, and a reused term is the same value the second
    // call would have returned, so the sum is bit for bit what it was.
    int radii[4] = int[4](r0, r1, r2, r3);
    vec2 samples[4];
    [[dont_unroll]] for (int i = 0; i < 4; ++i) {
        int reused = -1;
        for (int j = 0; j < i; ++j) if (radii[j] == radii[i]) reused = j;
        samples[i] = reused >= 0 ? samples[reused] : macro_sobel(iter_coord, radii[i], center);
    }
    vec2 g = samples[0] + samples[1] + samples[2] + samples[3];
    return g * 0.25;
}

// The sRGB transfer function itself, not the 2.2 power that stands in for it. The two part company
// at the bottom of the range, where the standard's linear segment keeps the tone the power curve
// crushes: at 0.04 the power reads 0.00084 against the true 0.0031. The pair is only ever used
// where light is added or mixed in linear space, and that is exactly where the round trip no
// longer cancels the error out.
vec3 srgb_to_linear(vec3 c) {
    vec3 s = max(c, 0.0);
    return mix(s / 12.92, pow((s + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), s));
}

vec3 linear_to_srgb(vec3 c) {
    vec3 l = max(c, 0.0);
    return mix(l * 12.92, 1.055 * pow(l, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), l));
}

// Duplicated from vk_iteration_palette.frag: one GLSL file cannot include another here.
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

// Soft shoulder over the lit result, in linear space. Only the brightest channel decides the
// compression and all three are scaled by the same factor, so an over-range highlight rolls to
// white along its own hue instead of clipping one channel at a time and skewing on the way up.
//
// It is a curve over the whole colour, not a term applied to the light that was added: anything
// already above the knee is compressed by it whether a highlight put it there or the palette did.
// Since the curve asymptotes to 1, a knee below 1 is the only way to get a shoulder at all, so
// touching the bright end of the base is inherent rather than incidental. Below the knee nothing
// is changed, and a knee of 1 leaves the plain hard clip in place.
vec3 highlight_rolloff(vec3 c, float knee) {
    float k = clamp(knee, 0.0, 1.0);
    float m = max(max(c.r, c.g), c.b);
    if (k >= 1.0 || m <= k) {
        return c;
    }
    float over = m - k;
    // Asymptotes to 1 as over grows, and matches value and slope with the identity at m == k.
    float rolled = k + over / (1.0 + over / (1.0 - k));
    return c * (rolled / m);
}

// Lays the shading on the palette color. Overlay works per sRGB channel; the OKLab form scales lightness alone and holds hue and chroma.
vec3 shade_composite(vec3 base, float shade) {
    if (int(slope_attr.shading_blend + 0.5) == SHADING_BLEND_OKLAB_LIGHTNESS) {
        vec3 lab = linear_to_oklab(srgb_to_linear(base));
        lab.x *= shade;
        return clamp(linear_to_srgb(oklab_to_linear(lab)), 0.0, 1.0);
    }
    float overlayVal = clamp(shade * 0.5, 0.0, 1.0);
    vec3 ov = vec3(overlayVal);
    return mix(
        2.0 * base * ov,
        1.0 - 2.0 * (1.0 - base) * (1.0 - ov),
        step(0.5, base)
    );
}

#include "studio_surface.glsl"
#include "lustre_relief.glsl"
#include "effects.glsl"
#define PALETTE_EXTERNAL_ITERATION
#define PALETTE_LIBRARY
#define PALETTE_DATA_SET 5
#define PALETTE_TEXTURE_SET 6
#define PALETTE_TIME_SET 7
#define srgb_to_linear palette_srgb_to_linear
#define linear_to_srgb palette_linear_to_srgb
#define linear_to_oklab palette_linear_to_oklab
#define oklab_to_linear palette_oklab_to_linear
#include "palette_sampling.glsl"
#undef srgb_to_linear
#undef linear_to_srgb
#undef linear_to_oklab
#undef oklab_to_linear
#include "band_groove.glsl"
vec3 surface_canvas_color(ivec2 coord) {
    vec3 c = texelFetch(canvas, coord, 0).rgb;
    return ordered_layers() ? linear_to_srgb(c) : c;
}
bool replace_surface_style() {
    return slope_attr.replace_surface_style > 0.5 && slope_attr.surface_style > 0.5;
}
vec3 material_canvas_color(ivec2 coord) {
    return replace_surface_style() ? vec3(0.5) : surface_canvas_color(coord);
}
#include "dark_surface.glsl"
#include "sea_print_surface.glsl"
#include "chrome_surface.glsl"

vec2 groove_base_slope = vec2(0);

// Share material evaluation after either composition prepares its inputs; original project code, see NOTICE.
struct PendingSurface {
    bool enabled;
    bool ordered;
    bool material;
    double iteration;
    vec3 normal;
    vec3 view;
    vec2 terrain;
    float ao;
    vec3 base;
    vec3 radiance;
    vec3 highlights;
    SurfaceEffects effects;
    vec2 materialCycles;
    vec2 movingCycleGradient;
    vec2 heldCycleGradient;
};
PendingSurface pendingSurface;

void shade_surface() {

    pendingSurface.enabled = false;
    uvec2 iter_coord = uvec2(gl_FragCoord.xy);


    // depth and opacity are the layer's off switches; reflection_ratio only raises the ambient floor.
    if((slope_attr.depth == 0 || slope_attr.opacity <= 0.0) && !(replace_surface_style() && slope_attr.studio_use > 0.5) && !effects_enabled()){
        color = texelFetch(canvas, ivec2(iter_coord), 0);
        if (slope_attr.studio_use > 0.5 && !ordered_layers()) color.rgb = srgb_to_linear(color.rgb);
        return;
    }

    float multiplier = float(iteration_info_attr.extent.x) / 1280;
    if (slope_attr.studio_use > 0.5) multiplier = float(max(iteration_info_attr.canvas_extent.x, 1u)) / 1280;

    float aRad = radians(slope_attr.azimuth);
    float zRad = radians(slope_attr.zenith);

    // Relief is read at the nearest pixel that still has a full 3x3 stencil, because an out-of-bounds tap comes back mirrored and cancels that axis of the Sobel; zeroing the gradient on the border instead left it a flat plane at cos(zenith) and drew a one-pixel frame around the image.
    uvec2 relief_coord = uvec2(
        clamp(int(iter_coord.x), 1, max(int(iteration_info_attr.extent.x) - 2, 1)),
        clamp(int(iter_coord.y), 1, max(int(iteration_info_attr.extent.y) - 2, 1))
    );

    double ld = get_iteration(relief_coord, ivec2(-1, -1));
    double d = get_iteration(relief_coord, ivec2(0, -1));
    double rd = get_iteration(relief_coord, ivec2(1, -1));
    double l = get_iteration(relief_coord, ivec2(-1, 0));
    double r = get_iteration(relief_coord, ivec2(1, 0));
    double lu = get_iteration(relief_coord, ivec2(-1, 1));
    double u = get_iteration(relief_coord, ivec2(0, 1));
    double ru = get_iteration(relief_coord, ivec2(1, 1));
    double centerIt = get_iteration(relief_coord, ivec2(0, 0));

    // Heights relative to the centre pixel, which is what keeps the fractional part of a large
    // iteration count alive. h_c is therefore zero by construction.
    float h_ld = height_rel(ld, centerIt);
    float h_d  = height_rel(d, centerIt);
    float h_rd = height_rel(rd, centerIt);
    float h_l  = height_rel(l, centerIt);
    float h_r  = height_rel(r, centerIt);
    float h_lu = height_rel(lu, centerIt);
    float h_u  = height_rel(u, centerIt);
    float h_ru = height_rel(ru, centerIt);

    // Use larger Sobel operator to grab a slightly wider average for less micro-jitter
    float dx = (h_rd - h_ld) + 2.0 * (h_r - h_l) + (h_ru - h_lu);
    float dy = (h_lu - h_ld) + 2.0 * (h_u - h_d) + (h_ru - h_rd);

    // Dual-scale relief: blend a wider-radius Sobel for broad undulation. macro_relief=0 keeps existing output.
    if (slope_attr.macro_relief > 0.0) {
        vec2 m = macro_gradient(relief_coord, multiplier, centerIt);
        dx = mix(dx, m.x, slope_attr.macro_relief);
        dy = mix(dy, m.y, slope_attr.macro_relief);
    }

    // log2 shrinks the gradient by 1/(iter*ln2), so this gain keeps depth on its previous scale near the boundary.
    const float DEPTH_GAIN = 1.0e5 * 0.69314718;
    float dzDx = dx * DEPTH_GAIN * slope_attr.depth * multiplier;
    float dzDy = dy * DEPTH_GAIN * slope_attr.depth * multiplier;
    // Preserve slopes through 45 degrees, then approach atan(2) smoothly without changing their direction.
    float slopeMagnitude = length(vec2(dzDx, dzDy));
    if (slopeMagnitude > 1.0) {
        float excess = slopeMagnitude - 1.0;
        float softenedMagnitude = 1.0 + excess / (1.0 + excess);
        float slopeScale = softenedMagnitude / slopeMagnitude;
        dzDx *= slopeScale;
        dzDy *= slopeScale;
    }
    float aoFactor = 1.0;
    if (slope_attr.ao_intensity > 0.0) {
        double mv = iteration_info_attr.max_value;
        bool aoInterior = (centerIt == 0.0 || centerIt >= mv) ||
                          (ld == 0.0 || ld >= mv) || (d == 0.0 || d >= mv) || (rd == 0.0 || rd >= mv) ||
                          (l == 0.0 || l >= mv) || (r == 0.0 || r >= mv) ||
                          (lu == 0.0 || lu >= mv) || (u == 0.0 || u >= mv) || (ru == 0.0 || ru >= mv);
        if (!aoInterior) {
            // Heights are already relative to the centre, so the centre's own height is 0 and the
            // neighbour average is the Laplacian outright.
            float laplacian = (h_ld + h_d + h_rd + h_l + h_r + h_lu + h_u + h_ru) * 0.125; // > 0 => pit (concave)
            float gradMag = length(vec2(dx, dy));                 // log-height gradient
            float curvature = laplacian / (gradMag + 1.0);        // dimensionless, +1 epsilon
            // Soft, bounded mapping; only pits (curvature > 0) occlude, ridges stay lit.
            float occ = smoothstep(0.0, 1.0, curvature * 3.0);
            aoFactor = 1.0 - slope_attr.ao_intensity * occ;
        }
    }

    // atan()+cos/sin round trip folded to one inversesqrt: expanding cos(aRad + aspect) cancels the gradient length exactly.
    if(slope_attr.surface_lustre_relief>0.5) {
        vec3 detail=lustre_relief(relief_coord,centerIt);
        dzDx=detail.x; dzDy=detail.y; aoFactor=detail.z;
    }
    SurfaceEffects surfaceEffects = surface_effects(centerIt, vec2(dx, dy), vec2(dzDx, dzDy));
    vec3 beforeEffectNormal = normalize(vec3(-dzDx, -dzDy, 1));
    vec2 effectSlope = vec2(dFdx(surfaceEffects.height), dFdy(surfaceEffects.height));
    if (centerIt <= 0.0 || centerIt >= iteration_info_attr.max_value) effectSlope = vec2(0.0);
    effectSlope *= float(max(iteration_info_attr.canvas_extent.y, 1u)) * 0.025;
    effectSlope *= 0.4 * inversesqrt(0.16 + dot(effectSlope, effectSlope));
    dzDx += effectSlope.x;
    dzDy += effectSlope.y;
    groove_base_slope = vec2(dzDx, dzDy);
    vec3 materialNormal=normalize(vec3(-dzDx,-dzDy,1));
    // The base-material layer consumes the same initialized cycle and gradients as the added materials.
    if ((ordered_layers() && shader_layer.control.x >= 11 && shader_layer.control.x <= 18) ||
        (!ordered_layers() && slope_attr.studio_use > 0.5 &&
         (slope_attr.surface_style > 0.5 || surface_layers_enabled()))) {
        double materialIteration = ordered_layers() ? centerIt : get_iteration_raw(ivec2(iter_coord));
        materialIteration = materialIteration > 0.0 && materialIteration < iteration_info_attr.max_value ? materialIteration : 1.0;
        g_interval = dvec4(palette_attr.interval);
        g_inv_interval = 1.0 / g_interval;
        pendingSurface.materialCycles = vec2(float(groove_cycle(ivec2(iter_coord), materialIteration, true)),
                                             float(groove_cycle(ivec2(iter_coord), materialIteration, false)));
        pendingSurface.movingCycleGradient = vec2(dFdx(pendingSurface.materialCycles.x), dFdy(pendingSurface.materialCycles.x));
        pendingSurface.heldCycleGradient = vec2(dFdx(pendingSurface.materialCycles.y), dFdy(pendingSurface.materialCycles.y));
    }
    if (ordered_layers() && !layer_is(11)) {
        color = texelFetch(canvas, ivec2(iter_coord), 0);
        if (shader_layer.control.x >= 12 && shader_layer.control.x <= 18 && centerIt > 0.0 && centerIt < iteration_info_attr.max_value) {
            vec2 uv = (gl_FragCoord.xy + vec2(iteration_info_attr.canvas_offset)) / max(vec2(iteration_info_attr.canvas_extent), vec2(1));
            vec3 v = normalize(vec3((uv - 0.5) * vec2(-0.1, -0.06), 1));
            pendingSurface.enabled = true;
            pendingSurface.ordered = true;
            pendingSurface.material = true;
            pendingSurface.iteration = centerIt;
            pendingSurface.normal = materialNormal;
            pendingSurface.view = v;
            pendingSurface.terrain = -vec2(dx, dy);
            pendingSurface.ao = aoFactor;
            pendingSurface.base = color.rgb;
            pendingSurface.radiance = color.rgb;
        }
        if (shader_layer.control.x >= 19 && shader_layer.control.x <= 22) {
            color.rgb = surface_effect_relief(color.rgb, beforeEffectNormal, materialNormal);
            color.rgb = surface_effect_finish(color.rgb, surfaceEffects, materialNormal, true);
        }
        return;
    }
    vec3 normalDx=dFdx(materialNormal), normalDy=dFdy(materialNormal);
    float normalVariance=min(0.12,0.19*(dot(normalDx,normalDx)+dot(normalDy,normalDy)))*slope_attr.surface_specular_aa;
    float invSlopeLen = inversesqrt(1.0 + dzDx * dzDx + dzDy * dzDy);

    // Diffuse shading. This expands to dot(N, L) for N = normalize(vec3(-dzDx, -dzDy, 1)), so the
    // diffuse term is already Lambert against the real surface; only the specular used to invent
    // its own normal.
    float rawShade = (cos(zRad) - sin(zRad) * (cos(aRad) * dzDx + sin(aRad) * dzDy)) * invSlopeLen;

    // The fill light: a second diffuse source folded in ahead of the floor, the terminator and the
    // gamma, so every one of them shapes the two lights as the one lit surface they now form. A
    // surface turned away from the fill adds nothing - a fill lifts shadows, it never casts its own.
    if (slope_attr.fill_intensity > 0.0) {
        float fZRad = radians(slope_attr.fill_zenith);
        float fARad = radians(slope_attr.fill_azimuth);
        float rawFill = (cos(fZRad) - sin(fZRad) * (cos(fARad) * dzDx + sin(fARad) * dzDy)) * invSlopeLen;
        rawShade = clamp(rawShade + clamp(rawFill, 0.0, 1.0) * clamp(slope_attr.fill_intensity, 0.0, 1.0), 0.0, 1.0);
    }

    // Effectively bounds the diffuse light between reflection_ratio (ambient) and 1.0. This form
    // clips the ramp to the floor rather than lifting it onto it, so every pixel below the floor
    // flattens onto one value and a kink is left along the rawShade == reflection_ratio isoline.
    float shade = max(slope_attr.reflection_ratio, clamp(rawShade, 0.0, 1.0));

    // Terminator Softness eases both of those away, and is blended in rather than switched in, so
    // that 0 is exactly the line above and the two forms meet with no step at the bottom of the
    // slider. Wrapping pulls the light round past N.L = 0; lifting puts the whole ramp on top of
    // the floor instead of cutting it off there.
    if (slope_attr.terminator_softness > 0.0) {
        float w = slope_attr.terminator_softness;
        float ndl = clamp((rawShade + w) / (1.0 + w), 0.0, 1.0);
        shade = mix(shade, mix(slope_attr.reflection_ratio, 1.0, ndl), w);
    }

    // Tone mapping: gamma shapes the shading curve here, brightness lands on the shaded color below.
    if (slope_attr.gamma > 0.0) {
        shade = pow(shade, 1.0 / slope_attr.gamma);
    }

    // L2: how much of the relief is carried by lightness at all. It is taken here, after the curve
    // and the floor, so lowering it fades out everything above rather than any one of them: at 0 the
    // palette color is left at its own lightness and the relief is left to the tint, the cavity and
    // the highlight, none of which are touched. rawShade below still carries the full Lambert term,
    // so the tint keeps reading the light's direction after the lightness has stopped showing it.
    shade = mix(1.0, shade, clamp(slope_attr.luma_amount, 0.0, 1.0));

    // Shading composite: shade=1.0 → neutral, shade<1.0 → darken
    vec3 base = surface_canvas_color(ivec2(iter_coord));
    // Opacity is held back to the end of main(), where it blends the finished slope result against this base.
    vec3 baseColor = shade_composite(base, shade);

    bool linearLight = int(slope_attr.light_blend + 0.5) == LIGHT_BLEND_LINEAR;

    // Brightness scales the shaded color; folded into the blend factor above it clamped at 1.0, where overlay maps every channel >= 0.5 to pure white and both the relief and the palette color stopped responding.
    baseColor *= slope_attr.brightness;
    if (!linearLight && slope_attr.studio_use < 0.5) {
        // The same shoulder LIGHT_BLEND_LINEAR ends on, taken here over the lifted base alone.
        // Dividing by the peak channel held the hue but cancelled the shading outright: the composite
        // is a scalar multiple of the base, so once brightness pushed the peak past 1 the divisor
        // carried that very factor and every shade above 1/(brightness * peak) landed on one color.
        baseColor = highlight_rolloff(baseColor, slope_attr.highlight_knee);
    }

    // Specular highlight and rim light, both as plain 0..1 amounts. Where they are added to the
    // shaded color is Light Blend's business, at the end of main().
    float specular = 0.0;
    float rim = 0.0;

    if (slope_attr.specular_intensity > 0.0 || slope_attr.rim_intensity > 0.0) {
        vec3 viewDir = vec3(0.0, 0.0, 1.0);

        // Specular calculation
        if (slope_attr.specular_intensity > 0.0 && slope_attr.studio_use < 0.5) {
            // Only the specular lobe still needs the gradient direction, so the atan stays inside this branch.
            float aspect = atan(dzDy, -dzDx);
            bool specUnlinked = slope_attr.specular_link < 0.5;
            float specZRad = specUnlinked ? radians(slope_attr.specular_zenith) : zRad;
            float specARad = specUnlinked ? radians(slope_attr.specular_azimuth) : aRad;

            vec3 specLightDir = normalize(vec3(
                cos(specARad) * sin(specZRad),
                sin(specARad) * sin(specZRad),
                cos(specZRad)
            ));
            vec3 specHalf = specLightDir + viewDir;
            float specHalfLengthSquared = dot(specHalf, specHalf);
            vec3 specHalfDir = specHalfLengthSquared > 1.0e-12 ? specHalf * inversesqrt(specHalfLengthSquared) : vec3(0.0);

            // The highlight's normal is anchored at half the light's zenith, which is exactly the
            // half vector's own tilt, so N.H reaches 1 wherever the slope faces the light and the
            // highlight comes out as a clean lobe in aspect with a peak that is actually reached.
            //
            // That anchor is doing real work and is not a shortcut. Shading Depth carries a 1e5
            // gain and runs to 10000, so over most of a view the gradient magnitude is saturated
            // and its steepness carries almost no information, while the direction it points stays
            // exact. Taking the tilt from the magnitude instead only lights the thin contour where
            // the surface happens to pass through half the zenith, which reads as filaments rather
            // than as a highlight, and leaves the saturated remainder too far from the half vector
            // to light at all. Relief Response dials the tilt off the anchor and onto the surface's
            // own for the views where the magnitude is not saturated; 0 keeps the anchor.
            float tilt = specZRad * 0.5;
            if (slope_attr.relief_response > 0.0) {
                float trueTilt = atan(length(vec2(dzDx, dzDy)));
                tilt = mix(tilt, trueTilt, clamp(slope_attr.relief_response, 0.0, 1.0));
            }
            vec3 spec_normal = vec3(
                sin(tilt) * cos(aspect),
                -sin(tilt) * sin(aspect),
                cos(tilt)
            );

            float NdotH = clamp(dot(spec_normal, specHalfDir), 0.0, 1.0);
            NdotH = smoothstep(0.0, 1.0, NdotH);
            float power = max(slope_attr.specular_power, 0.1);
            if (slope_attr.specular_anisotropy > 0.0) {
                float axisRad = radians(slope_attr.specular_anisotropy_angle);
                float dir = cos(2.0 * (aspect - axisRad)); // +1 parallel, -1 perpendicular
                float k = 1.0 + slope_attr.specular_anisotropy * 3.0;
                power = max(power * pow(k, -dir), 0.1);
            }
            specular = pow(NdotH, power) * slope_attr.specular_intensity;
            // aspect is noise on a flat patch, so fade the highlight out with the slope it read.
            float tan_slope = length(vec2(dzDx, dzDy));
            specular *= tan_slope / (tan_slope + 0.05);

            // Brightness is folded back in here so the highlight's shadow falloff is unchanged.
            specular *= clamp(shade * slope_attr.brightness, 0.0, 1.0); // Reduce specular in shadow
        }

        // Rim Light Calculation (Fresnel effect)
        // Light up edges where surface normal is perpendicular to view direction
        if (slope_attr.rim_intensity > 0.0) {
            // Check if current pixel or any neighbor is in Mandelbrot interior
            // to avoid white rim around black areas
            double mv = iteration_info_attr.max_value;
            bool nearInterior = (centerIt == 0.0 || centerIt >= mv) ||
                                (ld == 0.0 || ld >= mv) || (d == 0.0 || d >= mv) || (rd == 0.0 || rd >= mv) ||
                                (l == 0.0 || l >= mv) || (r == 0.0 || r >= mv) ||
                                (lu == 0.0 || lu >= mv) || (u == 0.0 || u >= mv) || (ru == 0.0 || ru >= mv);

            if (!nearInterior) {
                // Rim reads the fine Sobel without the depth gain on it. That is deliberate: the
                // gained gradient is saturated over most of a view, so a rim keyed off it would be
                // one flat value everywhere, while this one still has its range.
                float rimSlope = length(vec2(dx, dy)) * multiplier;
                // Bounded ramp: unlike the old inversesqrt it never pins at 1, so Rim Power can still narrow the band.
                float rimFactor = rimSlope / (rimSlope + 1.0);
                rim = pow(rimFactor, slope_attr.rim_power) * slope_attr.rim_intensity;
            }
        }
    }

    bool oklabTint = int(slope_attr.tint_blend + 0.5) == TINT_BLEND_OKLAB;
    if (slope_attr.ambient_intensity > 0.0 || (oklabTint && slope_attr.shadow_chroma != 1.0)) {
        float t = clamp(rawShade, 0.0, 1.0);
        // Shading Depth's gain saturates the gradient magnitude, so this term swings between its
        // ends over a very short band and the two tints meet in a hard seam. The curve is where
        // that seam is widened or tightened, and it moves nothing else about the light.
        if (slope_attr.tint_response != 1.0) {
            t = pow(t, max(slope_attr.tint_response, 0.01));
        }
        vec3 skyColor = vec3(slope_attr.sky_color_r, slope_attr.sky_color_g, slope_attr.sky_color_b);
        vec3 groundColor = vec3(slope_attr.ground_color_r, slope_attr.ground_color_g, slope_attr.ground_color_b);
        vec3 ambientColor = mix(groundColor, skyColor, t);
        if (oklabTint) {
            // Multiplying a saturated palette color by a tint mostly darkens it: the two hues fight
            // and what survives the product is its lightness. Splitting the tint into a lightness
            // and a chroma of its own lets the lightness scale as the multiply did while the chroma
            // is added, so a shadow takes the tint's hue on instead of being dimmed towards it.
            vec3 lab = linear_to_oklab(srgb_to_linear(max(baseColor, 0.0)));
            vec3 tintLab = linear_to_oklab(srgb_to_linear(max(ambientColor, 0.0)));
            lab.x *= mix(1.0, tintLab.x, slope_attr.ambient_intensity);
            lab.yz += tintLab.yz * slope_attr.ambient_intensity;
            // Chroma alone, so a shadow deepens in color without moving in lightness. Above 1 it is
            // the way a shadow behaves in paint and in skin, which no lightness curve can imitate.
            lab.yz *= mix(max(slope_attr.shadow_chroma, 0.0), 1.0, t);
            baseColor = max(linear_to_srgb(oklab_to_linear(lab)), 0.0);
        } else {
            baseColor *= mix(vec3(1.0), ambientColor, slope_attr.ambient_intensity);
        }
    }

    // The gloss. Palette Gloss lays the same narrow spike along the palette's own cycle, which is
    // what ties that look to the coloring: the bands slide as the palette animates, and at another
    // location they land wherever the iteration count happens to put them. Every coordinate below
    // belongs to the relief instead, so a band sits on the surface itself and neither the colors
    // nor the location move it. 0 intensity leaves every earlier version's picture untouched.
    float gloss = 0.0;
    if (slope_attr.gloss_intensity > 0.0) {
        int glossSource = int(slope_attr.gloss_source + 0.5);
        float bands = max(slope_attr.gloss_bands, 0.01);
        float glossSlope = length(vec2(dzDx, dzDy));
        float glossFacing = shade;
        float g;
        if (glossSource == GLOSS_SOURCE_RELIEF) {
            // The Sobel with no depth gain on it, which is the one relief term this shader does not
            // drive past its range, so a band picks out the same detail density at any zoom depth.
            float detail = length(vec2(dx, dy)) * multiplier;
            g = detail / (detail + 1.0);
        } else if (glossSource == GLOSS_SOURCE_ASPECT) {
            // Which way the slope faces, as one turn over 0..1. This coordinate joins back onto
            // itself, so a fractional band count would leave a seam along the half turn.
            g = atan(dzDy, -dzDx) * (0.5 / PI) + 0.5;
            bands = max(floor(bands + 0.5), 1.0);
        } else if (glossSource == GLOSS_SOURCE_SHADING_FINE) {
            // The same Lambert term as SHADING, but on a normal with a gain of its own in place of
            // Shading Depth's. That gain saturates the gradient and leaves rawShade at 0 or 1 almost
            // everywhere, so the bands had only the terminator to sit on; this one keeps the
            // mid-range wide, so they ring every form from its crest outward.
            vec2 gz = vec2(dx, dy) * exp2(clamp(slope_attr.gloss_relief, 0.0, 16.0)) * multiplier;
            // Fine Shading gloss uses the same reference-space Lustre gradient as the material.
            // That experiment is reverted after visual regressions; the legacy gloss gradient above is retained.
            float inv = inversesqrt(1.0 + dot(gz, gz));
            g = clamp((cos(zRad) - sin(zRad) * (cos(aRad) * gz.x + sin(aRad) * gz.y)) * inv, 0.0, 1.0);
            // Fine Shading keeps both its flat-patch fade and shadow mask independent of Shading Depth.
            glossSlope = length(gz);
            glossFacing = g;
        } else {
            // What the surface itself answers the light with, taken before the floor and the curve
            // shape it, so the bands follow the form rather than where the shading was clipped.
            g = clamp(rawShade, 0.0, 1.0);
        }
        float band = 0.5 + 0.5 * sin((g * bands + slope_attr.gloss_phase) * 2.0 * PI);
        gloss = pow(band, max(slope_attr.gloss_sharpness, 1.0)) * slope_attr.gloss_intensity;
        // All three coordinates are noise on a flat patch, so the gloss fades out with the slope it
        // was read from, exactly as the specular highlight does.
        gloss *= glossSlope / (glossSlope + 0.05);
        // It is a highlight, so it dies in shadow rather than lighting the side turned away.
        gloss *= clamp(glossFacing * slope_attr.brightness, 0.0, 1.0);
    }

    // Ambient occlusion darkens the diffuse/base term only; the specular highlight and
    // rim light are additive light sources, so they stay crisp on top of the occluded base.
    vec3 specColor = vec3(slope_attr.specular_color_r, slope_attr.specular_color_g, slope_attr.specular_color_b);
    vec3 rimColor = vec3(slope_attr.rim_color_r, slope_attr.rim_color_g, slope_attr.rim_color_b);
    vec3 glossColor = vec3(slope_attr.gloss_color_r, slope_attr.gloss_color_g, slope_attr.gloss_color_b);
    vec3 shaded = baseColor * aoFactor;

    vec3 slopeColor;
    if (slope_attr.studio_use > 0.5) {
        vec3 baseLinear = srgb_to_linear(base);
        double pixelIteration = get_iteration_raw(ivec2(iter_coord));
        if (pixelIteration <= 0.0 || pixelIteration >= iteration_info_attr.max_value) {
            color = vec4(baseLinear, 1);
            if (slope_attr.surface_style > 2.5 || replace_surface_style()) color.rgb = mix(baseLinear, vec3(0), (replace_surface_style() ? 1.0 : slope_attr.chrome_strength * slope_attr.opacity));
            return;
        }
        vec3 n = normalize(vec3(-dzDx, -dzDy, 1));
        vec2 uv = (gl_FragCoord.xy + vec2(iteration_info_attr.canvas_offset)) / max(vec2(iteration_info_attr.canvas_extent), vec2(1));
        // Lustre uses the orthographic view of the fractal plane, independent of a point's screen position.
        // That experiment is reverted after visual regressions; the established studio view below is retained.
        vec3 v = normalize(vec3((uv - 0.5) * vec2(-0.1, -0.06), 1));
        float boundaryMask=boundary_reflection_mask(iter_coord);
        vec3 radiance = studio_surface(baseLinear, srgb_to_linear(max(baseColor, 0.0)), n, v, aoFactor, normalVariance, boundaryMask, surfaceEffects.wetness);
        pendingSurface.enabled = true;
        pendingSurface.ordered = false;
        pendingSurface.material = slope_attr.surface_style > 0.5 || surface_layers_enabled();
        pendingSurface.iteration = pixelIteration;
        pendingSurface.normal = n;
        pendingSurface.view = v;
        pendingSurface.terrain = -vec2(dx, dy);
        pendingSurface.ao = aoFactor;
        pendingSurface.base = baseLinear;
        pendingSurface.radiance = radiance;
        pendingSurface.highlights = (srgb_to_linear(rimColor) * rim + srgb_to_linear(glossColor) * gloss) * boundaryMask;
        pendingSurface.effects = surfaceEffects;
        return;
    }
    if (linearLight) {
        // Add the light in proportion to light rather than on the encoded values. Summing on the
        // encoded values overstates it by roughly the encoding curve, which is why a highlight
        // reaches white early under DIRECT and then sits there as a flat plate with a hard edge
        // and no tint left in it.
        //
        // The encode/decode round trip is the identity, but the shoulder below is not: it is a
        // curve over the whole lit result, so anything already above the knee is pulled down by it
        // whether or not a highlight put it there. At knee 0.75 a white base leaves here at 0.94.
        // That is what a shoulder is for - it is the room the highlight rolls off into - but it
        // means LINEAR is not DIRECT plus a highlight, and a bright scene with no specular and no
        // rim does not come out identical between the two.
        vec3 lit = srgb_to_linear(max(shaded, 0.0))
                 + srgb_to_linear(specColor) * specular
                 + srgb_to_linear(rimColor) * rim
                 + srgb_to_linear(glossColor) * gloss;
        lit = highlight_rolloff(lit, slope_attr.highlight_knee);
        slopeColor = clamp(linear_to_srgb(lit), 0.0, 1.0);
    } else {
        // DIRECT: added on the encoded values and cut at white, as every version up to 2.0.8 did.
        slopeColor = clamp(shaded + specColor * specular + rimColor * rim + glossColor * gloss, 0.0, 1.0);
    }
    color = vec4(clamp(mix(base, slopeColor, slope_attr.opacity), 0.0, 1.0), 1.0);
    color.rgb = clamp(surface_effect_finish(color.rgb, surfaceEffects, materialNormal, false), 0.0, 1.0);
}

// Ordered compositing reuses the palette phase and the original line profile; see NOTICE.
float ordered_band_coverage(double cycle) {
    float width = shader_layer.line_params.y;
    if (shader_layer.control.z == 0 || width <= 0.0 || shader_layer.line_params.x <= 0.0) return 0.0;
    double f = mod(cycle * double(shader_layer.line_params.x), 1.0LF);
    float x = float(min(f, 1.0LF - f)) * 2.0 / width;
    float opacity = clamp(shader_layer.line_params.z, 0.0, 1.0);
    float softness = clamp(shader_layer.line_params.w, 0.0, 1.0);
    if (softness <= 0.0) return x < 1.0 ? opacity : 0.0;
    float t = clamp((1.0 + softness - x) / (2.0 * softness), 0.0, 1.0);
    float shoulder = t * t * (3.0 - 2.0 * t);
    return opacity * shoulder * shoulder * shoulder * shoulder;
}

float ordered_band_channel(double iteration, double interval, double inverse, double anim, double warp, float frozen) {
    double ratio = biased_ratio(mod(coloring_curve(div_r(smooth_iteration(iteration), interval, inverse)), 1.0LF));
    double held = ratio + palette_attr.offset + div_r(warp, interval, inverse);
    return mix(ordered_band_coverage(held - div_r(anim, interval, inverse)), ordered_band_coverage(held), frozen);
}

#include "band_decoration.glsl"

void ordered_palette_layer(uvec2 coord) {
    color = texelFetch(canvas, ivec2(coord), 0);
    double iteration = get_palette_iteration(coord);
    if (iteration <= 0.0 || iteration >= iteration_info_attr.max_value) return;
    g_interval = dvec4(palette_attr.interval);
    g_inv_interval = 1.0 / g_interval;
    vec2 pixel = vec2(coord) + 0.5;
    double anim = animation_offset_iterations(pixel);
    vec2 gradient = iteration_gradient(ivec2(coord), iteration);
    double warp = warp_offset(iteration, pixel, gradient, anim);
    vec4 encoded = vec4(linear_to_srgb(color.rgb), color.a);
    if (layer_is(1)) {
        float frozen = freeze_weight(iteration);
        vec3 coverage = vec3(ordered_band_channel(iteration, g_interval.r, g_inv_interval.r, anim, warp, frozen),
                             ordered_band_channel(iteration, g_interval.g, g_inv_interval.g, anim, warp, frozen),
                             ordered_band_channel(iteration, g_interval.b, g_inv_interval.b, anim, warp, frozen));
        encoded.rgb = mix(encoded.rgb, shader_layer.line_color.rgb, coverage);
    } else if (shader_layer.control.x <= 5) {
        encoded = apply_texture(encoded, iteration, pixel, gradient, anim, warp);
    } else {
        encoded = apply_pattern(encoded, iteration, pixel, gradient, anim, warp);
    }
    color = vec4(srgb_to_linear(encoded.rgb), color.a);
}

void shade_layers() {
    if (ordered_layers()) {
        uvec2 coord = uvec2(gl_FragCoord.xy);
        if (layer_is(100)) {
            color = texelFetch(canvas, ivec2(coord), 0);
            color.rgb = srgb_to_linear(color.rgb);
            return;
        }
        if (shader_layer.control.x >= 1 && shader_layer.control.x <= 9 &&
            !(layer_is(1) && shader_layer.control.w != 0)) {
            ordered_palette_layer(coord);
            return;
        }
        if (!(layer_is(1) || (shader_layer.control.x >= 11 && shader_layer.control.x <= 22))) {
            color = texelFetch(canvas, ivec2(coord), 0);
            return;
        }
    }
    // Share one surface call so the driver compiles the material graph only once; see NOTICE.
    shade_surface();
    if (pendingSurface.enabled) {
        vec3 radiance = pendingSurface.radiance;
        if (pendingSurface.material) {
            radiance = chrome_surface(uvec2(gl_FragCoord.xy), pendingSurface.iteration,
                                      pendingSurface.normal, pendingSurface.view,
                                      pendingSurface.terrain, radiance, pendingSurface.ao,
                                      pendingSurface.materialCycles, pendingSurface.movingCycleGradient,
                                      pendingSurface.heldCycleGradient);
        }
        bool fullReplacement = replace_surface_style() && (!ordered_layers() || layer_is(11));
        float materialOpacity = clamp(slope_attr.opacity, 0.0, 1.0);
        if (pendingSurface.ordered) {
            color.rgb = fullReplacement ? radiance : mix(pendingSurface.base, radiance, materialOpacity);
        } else {
            radiance += pendingSurface.highlights;
            color = vec4(clamp(fullReplacement ? radiance : mix(pendingSurface.base, radiance, materialOpacity), 0.0, 60000.0), 1);
            color.rgb = surface_effect_finish(color.rgb, pendingSurface.effects, pendingSurface.normal, true);
        }
    }
    // Both compositions use one groove evaluation with the same inputs and transfer functions; see NOTICE.
    float grooveLight = 1.0;
    if (ordered_layers() ? layer_is(1) : groove_enabled()) {
        g_interval = dvec4(palette_attr.interval);
        g_inv_interval = 1.0 / g_interval;
        grooveLight = groove_light(uvec2(gl_FragCoord.xy), groove_base_slope);
    }
    if (ordered_layers()) {
        if (layer_is(1)) {
            g_interval = dvec4(palette_attr.interval);
            g_inv_interval = 1.0 / g_interval;
            color.rgb *= grooveLight;
        } else if (layer_is(11) && slope_attr.studio_use < 0.5) {
            color.rgb = srgb_to_linear(color.rgb);
        }
        color.rgb = clamp(color.rgb, 0.0, 60000.0);
        return;
    }
    if (groove_enabled()) {
        g_interval = dvec4(palette_attr.interval);
        g_inv_interval = 1.0 / g_interval;
        float light = grooveLight;
        if (slope_attr.studio_use > 0.5) {
            color.rgb = clamp(color.rgb * light, 0.0, 60000.0);
        } else {
            color.rgb = clamp(linear_to_srgb(srgb_to_linear(color.rgb) * light), 0.0, 1.0);
        }
    }
    if (effects_attr.context.x > 0.5 && slope_attr.studio_use < 0.5) {
        color.rgb = srgb_to_linear(color.rgb);
    }
}

void main() {
    shade_layers();
    if (!ordered_layers() || layer_is(1)) {
        color.rgb = apply_band_decoration(uvec2(gl_FragCoord.xy), color.rgb,
                                          ordered_layers() || slope_attr.studio_use > 0.5 ||
                                          effects_attr.context.x > 0.5);
    }
}
