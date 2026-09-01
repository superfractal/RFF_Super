//
// Modified by Opus 5 on 2026-08-07, 2026-08-08, 2026-08-12, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-23
// Modified by GPT-5 on 2026-08-16, 2026-08-23
//

#version 450

layout (set = 0, binding = 0) uniform sampler2D fog_canvas;
layout (set = 0, binding = 1) uniform sampler2D fog_blurred;

layout (set = 1, binding = 0) uniform FogUBO {
    float radius;
    float opacity;
    float rim_mask;
    float rim_mask_boost;
    float rim_blur;
    // Appended at the tail to keep existing offsets fixed (must match DescFog reserve order).
    float center_start;
    float center_invert;
    // Focus band: the defocus is cut on the iteration count, not on the screen.
    float focus_amount;
    float focus_ratio;
    float focus_range;
    float focus_falloff;
    float focus_blur;
    // 0 = Speed, 1 = Appearance. See blur_radius() and blur_rings() for what each spends.
    float blur_quality;
} fog_attr;

layout (set = 2, binding = 0) uniform IterUBO {
    uvec2 extent;
    double max_value;
    // Declared only so the tail fields below are reachable; the passes that need neither stop
    // at max_value.
    double max_value_normal;
    double max_value_zoomed;
    // The whole canvas this buffer is a piece of. Equal to extent for an ordinary frame; a
    // tiled export makes it the finished image, which is what the blur radii are measured
    // against - see the multiplier in main().
    uvec2 canvas_extent;
    ivec2 canvas_offset;
} iteration_info_attr;

layout (set = 2, binding = 1) buffer IterSSBO {
    double iterations[];
} iteration_attr;

// Mirrors vk_slope.frag's block (and DescSlope's reserve order) so the mask can reuse the
// slope pass's own rim shape instead of approximating it.
layout (set = 3, binding = 0) uniform SlopeUBO {
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
    float specular_color_r;
    float specular_color_g;
    float specular_color_b;
    float ao_intensity;
    float ambient_intensity;
    float sky_color_r;
    float sky_color_g;
    float sky_color_b;
    float ground_color_r;
    float ground_color_g;
    float ground_color_b;
    float specular_link;
    float specular_zenith;
    float specular_azimuth;
    float specular_anisotropy;
    float specular_anisotropy_angle;
    float macro_relief;
    float macro_radius;
    float shading_blend;
} slope_attr;


layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexcoord;

layout (location = 0) out vec4 color;

float grayScale(vec3 c) {
    return c.r * 0.3 + c.g * 0.59 + c.b * 0.11;
}

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

// log2(1 + x), as a series rather than through log(). Vulkan pins log() to an ABSOLUTE error of
// 2^-21 for arguments in [0.5, 2], so for an argument a hair above 1 - which is every tap in a
// deep, smooth view - it is allowed to be wrong by more than the answer itself. See the slope
// pass for the full account. Every term below is reached from x by multiplication alone, so the
// relative accuracy holds however small x gets.
float log2_1p(float x) {
    return x * (1.0 - x * (0.5 - x * (1.0 / 3.0 - x * (0.25 - x * 0.2)))) * 1.4426950408889634;
}

// Shading height of one tap in doublings, measured against the centre pixel instead of against
// zero. Mirrors the slope pass, including why: only differences of heights are taken from here,
// so the shared log2(centre) cancels, and leaving it out is what holds the precision. Taking log2
// of the absolute count meant casting a double count down to a float, which past about 8.4 million
// iterations quantises away the whole fractional part the gradient is made of, turning the height
// field into a staircase that the Sobel reads as spikes along each riser.
float height_rel(double it, double center) {
    double a = max(it, 1.0);
    double b = max(center, 1.0);
    // Sterbenz: while the two are within a factor of two the subtraction is exact in double, so
    // the ratio reaches float as a small number carrying its full relative precision.
    double dr = (a - b) / b;
    // 0.25 rather than 0.5: the five-term series is worth 0.45% at 0.5 and the switch would hand
    // that over as a step. Mirrors the slope pass, where the same bound is set out in full.
    if (abs(dr) <= 0.25) {
        return log2_1p(float(dr));
    }
    // Far apart, which happens where an interior tap sits beside an exterior one.
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

// Wide-scale gradient for the dual-scale relief. Mirrors the slope pass so the rim mask keeps
// tracking the surface the slope pass actually shades.
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
    vec2 s0 = macro_sobel(iter_coord, r0, center);
    vec2 s1 = (r1 == r0) ? s0 : macro_sobel(iter_coord, r1, center);
    vec2 s2 = (r2 == r1) ? s1 : ((r2 == r0) ? s0 : macro_sobel(iter_coord, r2, center));
    vec2 s3 = (r3 == r2) ? s2 : ((r3 == r1) ? s1 : ((r3 == r0) ? s0 : macro_sobel(iter_coord, r3, center)));
    vec2 g = s0 + s1 + s2 + s3;
    return g * 0.25;
}

// The rim light's Fresnel footprint, rebuilt here without its intensity factor: the mask marks
// where the rim lands even when the rim light itself is turned down to zero.
float rim_shape(uvec2 iter_coord) {
    // The same clamp the slope pass reads its relief through, so the border carries the rim of the nearest full 3x3 stencil rather than dropping to no rim and leaving a one-pixel frame in the mask.
    iter_coord = uvec2(
        clamp(int(iter_coord.x), 1, max(int(iteration_info_attr.extent.x) - 2, 1)),
        clamp(int(iter_coord.y), 1, max(int(iteration_info_attr.extent.y) - 2, 1))
    );

    double ld = get_iteration(iter_coord, ivec2(-1, -1));
    double d = get_iteration(iter_coord, ivec2(0, -1));
    double rd = get_iteration(iter_coord, ivec2(1, -1));
    double l = get_iteration(iter_coord, ivec2(-1, 0));
    double r = get_iteration(iter_coord, ivec2(1, 0));
    double lu = get_iteration(iter_coord, ivec2(-1, 1));
    double u = get_iteration(iter_coord, ivec2(0, 1));
    double ru = get_iteration(iter_coord, ivec2(1, 1));

    double mv = iteration_info_attr.max_value;
    double centerIter = get_iteration(iter_coord, ivec2(0, 0));
    // Interior pixels are skipped exactly as the rim light skips them, so no halo around black areas.
    bool nearInterior = (centerIter == 0.0 || centerIter >= mv) ||
                        (ld == 0.0 || ld >= mv) || (d == 0.0 || d >= mv) || (rd == 0.0 || rd >= mv) ||
                        (l == 0.0 || l >= mv) || (r == 0.0 || r >= mv) ||
                        (lu == 0.0 || lu >= mv) || (u == 0.0 || u >= mv) || (ru == 0.0 || ru >= mv);
    if (nearInterior) {
        return 0.0;
    }

    float multiplier = float(iteration_info_attr.extent.x) / 1280;
    // Heights relative to the centre pixel; see height_rel() for why the absolute form cannot hold
    // its precision at depth.
    float h_ld = height_rel(ld, centerIter);
    float h_d  = height_rel(d, centerIter);
    float h_rd = height_rel(rd, centerIter);
    float h_l  = height_rel(l, centerIter);
    float h_r  = height_rel(r, centerIter);
    float h_lu = height_rel(lu, centerIter);
    float h_u  = height_rel(u, centerIter);
    float h_ru = height_rel(ru, centerIter);
    float dx = (h_rd - h_ld) + 2.0 * (h_r - h_l) + (h_ru - h_lu);
    float dy = (h_lu - h_ld) + 2.0 * (h_u - h_d) + (h_ru - h_rd);

    if (slope_attr.macro_relief > 0.0) {
        vec2 m = macro_gradient(iter_coord, multiplier, centerIter);
        dx = mix(dx, m.x, slope_attr.macro_relief);
        dy = mix(dy, m.y, slope_attr.macro_relief);
    }

    float rimSlope = length(vec2(dx, dy)) * multiplier;
    float rimFactor = rimSlope / (rimSlope + 1.0);
    // The raw footprint peaks far below 1, so masking with it alone would only dim the fog
    // everywhere. The boost saturates it into a region selector: 1 = raw shape, higher = fuller band.
    float boost = max(fog_attr.rim_mask_boost, 1.0);
    float boostedFactor;
    if (rimFactor < 1.0e-4) {
        float scaled = rimFactor * boost;
        boostedFactor = scaled < 1.0e-4 ? scaled : 1.0 - exp(-scaled);
    } else {
        boostedFactor = 1.0 - pow(1.0 - rimFactor, boost);
    }
    return pow(boostedFactor, slope_attr.rim_power);
}

// How far out of focus this pixel is, in 0..1. The band is measured on the iteration count rather
// than on the screen, so the sharp part is a depth of the fractal itself and not a place in the
// frame: it holds the same structure as the view zooms, which a screen-space falloff cannot. The
// ends are fractions of the frame's own maximum iteration, so the band does not have to be re-set
// every time the count grows. The interior carries no usable count and is left sharp.
float focus_defocus(uvec2 iter_coord) {
    if (fog_attr.focus_amount <= 0.0) {
        return 0.0;
    }
    double mv = iteration_info_attr.max_value;
    double it = get_iteration(iter_coord, ivec2(0, 0));
    if (it <= 0.0 || it >= mv || mv <= 0.0) {
        return 0.0;
    }
    double focus = double(clamp(fog_attr.focus_ratio, 0.0, 1.0)) * mv;
    double range = double(max(fog_attr.focus_range, 1.0e-4)) * mv;
    // Kept in double up to the ratio: the difference of two counts is exact there, and past about
    // 8.4 million iterations a float cannot hold the counts themselves closely enough to subtract.
    float d = clamp(float(abs(it - focus) / range), 0.0, 1.0);
    return pow(d, max(fog_attr.focus_falloff, 0.01)) * clamp(fog_attr.focus_amount, 0.0, 1.0);
}

// Circular Gaussian over the full-resolution canvas. The shared fog blur is only
// GAUSSIAN_MAX_WIDTH wide, so the band needs its own neighbourhood to stay crisp. A row-plus-column
// kernel would be cheaper but leaves a plaid of shared rows and columns, so this walks a real 2D
// disc: the stride of 2 lands each tap on a texel corner, where the linear sampler folds a 2x2
// block into one fetch and no texel is skipped.
// max_rings caps the tap grid. Past it the taps spread over the same radius instead of multiplying,
// so a wide kernel keeps its true width at any render resolution for a fixed number of fetches.
vec3 local_blur(vec2 coord, float radius, int max_rings) {
    // Held to a radius no frame is wider than: the setting takes any number, and the disc test below
    // squares this one.
    int n = int(min(radius, 4096.0));
    if (n <= 0) {
        return texture(fog_canvas, coord).rgb;
    }
    vec2 texel = 1.0 / vec2(textureSize(fog_canvas, 0));
    float sigma = max(float(n) * 0.5, 0.5);
    float inv2s2 = 1.0 / (2.0 * sigma * sigma);
    vec3 sum = vec3(0.0);
    float weight = 0.0;
    // The taps straddle the fragment at +-0.5, +-2.5, ... rather than running 0, +-2 from it. Both
    // land on texel corners, but a run starting at the fragment puts its first 2x2 block wholly on
    // one side, and since the Gaussian is symmetric about the fragment while the blocks are not,
    // the whole kernel came out displaced half a texel down and to the right. Straddling makes the
    // set symmetric, and the weight is read at the block's own centre so the two agree.
    int rings = int((float(n) - 0.5) * 0.5) + 1;
    // The stride is the tap spacing in texels; 2 is the value the straddling above is written for.
    float stride = 2.0;
    if (rings > max_rings) {
        stride = 2.0 * float(rings) / float(max_rings);
        rings = max_rings;
    }
    for (int jy = -rings; jy < rings; ++jy) {
        float sy = stride * (float(jy) + (jy < 0 ? 0.75 : 0.25));
        for (int jx = -rings; jx < rings; ++jx) {
            float sx = stride * (float(jx) + (jx < 0 ? 0.75 : 0.25));
            float d2 = sx * sx + sy * sy;
            if (d2 > float(n * n)) {
                continue;
            }
            float w = exp(-d2 * inv2s2);
            sum += texture(fog_canvas, coord + vec2(sx, sy) * texel).rgb * w;
            weight += w;
        }
    }
    // The innermost ring sits at 0.5 on each axis, so d2 is 0.5 and n >= 1 always keeps it.
    return sum / weight;
}

// Speed keeps the 16-texel ceiling the two blurs have always had. It is a ceiling in texels, not in
// the frame, so at a video's render resolution it lands as a far smaller part of the picture than the
// same setting does in the preview; Appearance spends the radius the setting asks for instead.
float blur_radius(float requested) {
    return fog_attr.blur_quality > 0.5 ? requested : min(requested, 16.0);
}

// The tap budget each mode is allowed. Speed's 8 is what its own 16-texel ceiling already implies, so
// its kernel is unchanged; Appearance pays four times the fetches and spreads them past that.
int blur_rings() {
    return fog_attr.blur_quality > 0.5 ? 16 : 8;
}

void main() {

    vec2 coord = gl_FragCoord.xy / textureSize(fog_canvas, 0);

    float x = coord.x;
    float y = coord.y;

    if (x < 0 || y < 0) {
        discard;
    }
    if (x >= 1 || y >= 1) {
        discard;
    }
    vec3 blurredColor = texture(fog_blurred, coord).rgb;
    color = texture(fog_canvas, coord);

    // rim_mask fades the fog toward the rim footprint: 0 keeps the full-frame fog, 1 confines it to the rim.
    float shape = fog_attr.rim_mask > 0.0 ? rim_shape(uvec2(gl_FragCoord.xy)) : 0.0;
    float band = shape * fog_attr.rim_mask;
    float amount = fog_attr.opacity * mix(1.0, shape, fog_attr.rim_mask);

    // center_start holds the fog off until this far out from the frame centre, ramping to full at the
    // frame edge. The ellipse follows the frame, so the falloff looks the same at any aspect ratio.
    // Distance saturates at the edge rather than the corner: normalising to the corner would leave
    // every value past 0.71 identical outside the corner triangles, wasting most of the slider.
    if (fog_attr.center_start > 0.0) {
        float dist = min(length((coord - 0.5) * 2.0), 1.0);
        // smoothstep is undefined when both edges meet, so the ramp keeps a sliver of width.
        float ramp = smoothstep(min(fog_attr.center_start, 0.99), 1.0, dist);
        // Inverted, the haze sits on the deep side instead: solid over the centre and clearing
        // outwards, so center_start becomes the radius of the fully fogged core.
        amount *= fog_attr.center_invert > 0.5 ? 1.0 - ramp : ramp;
    }

    // The band dissolves into its own full-res neighbourhood; only the unmasked haze uses the shared
    // low-resolution blur, which is where that blur is soft enough not to read as a resolution drop.
    // Against the finished image rather than the buffer being rendered. A tiled export draws one
    // tile at a time, and a radius taken from the tile would cover that tile the way the preview's
    // covers the frame - a fraction of that across the image the tiles add up to. The rim shape
    // above keeps the buffer-relative multiplier: it tracks the slope pass, which works in buffer
    // space, and the export's tile overlap is sized from the same number.
    float multiplier = float(iteration_info_attr.canvas_extent.x) / 1280;
    vec3 target = blurredColor;
    if (band > 0.0 && amount > 0.0) {
        target = mix(blurredColor, local_blur(coord, blur_radius(fog_attr.rim_blur * multiplier), blur_rings()), band);
    }

    vec3 cf = color.rgb - (color.rgb - target) * amount;

    // Away from the band the fog stays the brighten-only haze it has always been.
    vec3 lifted = grayScale(color.rgb) < grayScale(cf) ? cf : color.rgb;
    // Inside it the blur replaces the pixel in both directions, so the band washes out instead of only glowing.
    // Which of the two applies is rim_mask's business alone. Crossfading on band instead put the
    // rim footprint into the darkening half a second time - amount already carries it - so the same
    // Opacity moved a pixel by shape one way and by shape squared the other.
    color = vec4(mix(lifted, cf, fog_attr.rim_mask), 1);

    // The defocus is laid over the finished fog rather than mixed into it: haze is something in
    // front of the subject and the lens is behind the eye, so the haze belongs under the blur. Its
    // radius carries the strength, which is what makes this read as a lens and not as a fade to a
    // blurred copy - a pixel just outside the band is softened, not half-replaced.
    float defocus = focus_defocus(uvec2(gl_FragCoord.xy));
    if (defocus > 0.0) {
        vec3 source = texture(fog_canvas, coord).rgb;
        vec3 blurred = local_blur(coord, blur_radius(fog_attr.focus_blur * defocus * multiplier), blur_rings());
        // Preserve the finished fog delta while the radius alone defocuses the underlying canvas.
        color = vec4(blurred + (color.rgb - source), 1);
    }
}
