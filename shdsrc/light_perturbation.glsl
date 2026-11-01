//
// Created by Merutilm on 8/24/26.
// Modified by Opus 5.5 on 2026-10-06, 2026-10-07, 2026-10-12, 2026-10-16, 2026-10-29, 2026-11-01
//

// Derived from Merutilm RFF 2.3.0 shaders/include/desc_render_meta_double.glsl and desc_batch_result.glsl, GPL-3.0; see NOTICE.
// Shared by the three vk_light_perturbation*.comp shaders; bindings must match GpuLightPerturbator::Binding.
#ifndef LIGHT_PERTURBATION_INCLUDE
#define LIGHT_PERTURBATION_INCLUDE

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// ---- Pixel status, shared with GpuLightPerturbator::Status ----

const uint STATUS_RUNNING = 0;
const uint STATUS_FINISHED = 1;
const uint STATUS_INTERIOR = 2;
const uint STATUS_SKY = 3;
const uint STATUS_INTERPOLATED = 4;

// ---- Buffer layouts ----

struct LightPA {
    double anr;
    double ani;
    double bnr;
    double bni;
    double radius;
    uint64_t skip;
};

// LightPA narrowed to float for the single-precision shader.
struct LightPAF {
    float anr;
    float ani;
    float bnr;
    float bni;
    float radius;
    uint _pad;
    uint64_t skip;
};

// The levels of one table slot: entries [offset, offset + levels) of the table.
struct TableMapper {
    uint offset;
    uint levels;
};

struct TablePeriod {
    uint64_t period;
    uint64_t elements;
};

// Everything one pixel carries from one batch to the next.
struct PixelState {
    double dzr;
    double dzi;
    double pd;
    double cd;
    uint64_t iteration;
    uint64_t ref_iteration;
    uint64_t abs_iteration;
    uint status;
    // Orbit trap minima as log2, valid once trap_samples counts the samples they hold (two fills the second minimum).
    uint trap_samples;
    float trap_cross;
    float trap_point;
    float trap_second;
    uint trap_iteration;
    // The Orbit Trap Shape's minimum as log2, its wrapped iteration and arg(z - c) there, valid with trap_samples.
    float trap_shape;
    uint trap_shape_iteration;
    float trap_shape_angle;
};

// What the host reads back after each batch; it finishes the smoothing itself.
struct PixelResult {
    double pd;
    double cd;
    uint64_t count;
    uint status;
    // The angle of z where a finished orbit escaped, in radians; 0 while the pixel still runs.
    float escape_angle;
    // OrbitTrap in OrbitTrap.h: log2 distances, then one plus the wrapped iteration of the nearest origin approach.
    float trap_cross;
    float trap_point;
    float trap_second;
    float trap_iteration;
    // The shape planes of OrbitTrap: log2 distance, 1 + iteration, angle, then status (0 no data, 1 valid).
    float trap_shape;
    float trap_shape_iteration;
    float trap_shape_angle;
    float trap_shape_status;
};

layout (std430, set = 0, binding = 0) readonly buffer Params {
    uvec2 extent;
    uint selection_method;
    uint level_count;
    uint compression_method;
    uint absolute_mode;
    uint64_t max_iteration;
    uint64_t max_ref_iteration;
    uint64_t mapper_length;
    double bailout2;
    // TrapShapeParams: 0 measures no shape, else ShdPalOrbitTrapShape; the sides; then c, the radius and the unit normal.
    uint trap_shape_kind;
    uint trap_sides;
    double trap_center_r;
    double trap_center_i;
    double trap_radius;
    double trap_cos;
    double trap_sin;
    // Polygon and Star: one sector's turn and half of it as (cos, sin), and the inner corner of the folded segment.
    double trap_sector_cos;
    double trap_sector_sin;
    double trap_half_cos;
    double trap_half_sin;
    double trap_inner_x;
    double trap_inner_y;
} params;

layout (std430, set = 0, binding = 1) readonly buffer Orbit { dvec2 orbit[]; } reference;
layout (std430, set = 0, binding = 2) readonly buffer Table { LightPA pa[]; } table;
layout (std430, set = 0, binding = 3) readonly buffer Mapper { TableMapper mapper[]; } mapping;
layout (std430, set = 0, binding = 4) readonly buffer Periods { TablePeriod period[]; } periods;
layout (std430, set = 0, binding = 5) readonly buffer Dc { dvec2 dc[]; } offsets;
layout (std430, set = 0, binding = 6) buffer States { PixelState state[]; } states;
layout (std430, set = 0, binding = 7) buffer Results { PixelResult result[]; } results;
layout (std430, set = 0, binding = 8) readonly buffer OrbitF { vec2 orbit[]; } reference_f;
layout (std430, set = 0, binding = 9) readonly buffer TableF { LightPAF pa[]; } table_f;
layout (std430, set = 0, binding = 10) readonly buffer DcF { vec2 dc[]; } offsets_f;

layout (push_constant) uniform Batch {
    uint batch_size;
    uint ignore_mpa;
} batch;

// ---- MPA table indexing, as MPATable and LightMPATable index it ----

const uint64_t NOT_FOUND = 0xFFFFFFFFFFFFFFFFul;
const uint MSM_LOWEST = 0;
const uint MSM_HIGHEST = 1;
const uint COMPRESSION_NONE = 0;
const uint64_t REQUIRED_PERTURBATION = 2;

// Mirrors MPATable::iterationToPulledTableIndex.
uint64_t pulled_table_index(const uint64_t iteration) {
    if (iteration == 0) {
        return NOT_FOUND;
    }
    uint64_t index = 0;
    uint64_t remainder = iteration;
    for (uint i = params.level_count; i > 0; --i) {
        const uint64_t period = periods.period[i - 1].period;
        if (remainder < period) {
            continue;
        }
        if (i < params.level_count &&
            remainder + periods.period[0].period - REQUIRED_PERTURBATION + 1 > periods.period[i].period) {
            return NOT_FOUND;
        }
        index += remainder / period * periods.period[i - 1].elements;
        remainder %= period;
    }
    return remainder == 1 ? index : NOT_FOUND;
}

// The table slot of ref_iteration; an empty slot has levels 0.
TableMapper table_slot(const uint64_t ref_iteration) {
    if (ref_iteration == 0) {
        return TableMapper(0, 0);
    }
    const uint64_t index = params.compression_method == COMPRESSION_NONE ? ref_iteration
                                                                         : pulled_table_index(ref_iteration);
    if (index >= params.mapper_length) {
        return TableMapper(0, 0);
    }
    return mapping.mapper[uint(index)];
}

// ---- Orbit traps, as OrbitTrapAccumulator gathers them ----

const float TRAP_FAR_LOG2 = 64.0;
const float TRAP_HIT_LOG2 = -1.0e6;
const uint TRAP_ITERATION_WRAP = 1u << 22;

// log2 of a double past float's exponent range, through frexp.
float log2_wide(const double v) {
    if (!(v > 0.0LF)) {
        return TRAP_HIT_LOG2;
    }
    int e;
    const double m = frexp(v, e);
    return float(e) + log2(float(m));
}

const uint TRAP_SHAPE_POINT = 1;
const uint TRAP_SHAPE_CIRCLE = 2;
const uint TRAP_SHAPE_CROSS = 3;
const uint TRAP_SHAPE_LINE = 4;
const uint TRAP_SHAPE_GRID_POINTS = 5;
const uint TRAP_SHAPE_GRID_LINES = 6;
const uint TRAP_SHAPE_RINGS = 7;
const uint TRAP_SHAPE_POLYGON = 8;
const uint TRAP_SHAPE_STAR = 9;
const uint TRAP_SHAPE_SPIRAL = 10;
const double TRAP_PI = 3.14159265358979323846LF;

// orbit_trap_shape::magnitude in OrbitTrapShape.h: the plain root unless the squares left double's normal range.
double trap_magnitude(const double r, const double i) {
    precise const double norm = r * r + i * i;
    if (norm >= 1.0e-300LF && norm <= 1.0e300LF) {
        return sqrt(norm);
    }
    const double m = max(abs(r), abs(i));
    if (m == 0.0LF) {
        return m;
    }
    precise const double a = r / m;
    precise const double b = i / m;
    return m * sqrt(a * a + b * b);
}

// orbit_trap_shape::nearestStep: the whole number of steps nearest x / step, 0 where the quotient is not finite.
double trap_nearest_step(const double x, const double step) {
    const double k = floor(x / step + 0.5LF);
    return isinf(k) || isnan(k) ? 0.0LF : k;
}

double trap_step_residual(const double x, const double step) {
    precise const double r = x - step * trap_nearest_step(x, step);
    return r;
}

// atan(y / x) for 0 <= y <= x in double: the ratio is moved to within pi / 24 of a multiple of pi / 12, whose
// tangent is known, and the rest comes from the Taylor series of atan, whose tenth term is below 1e-18 there.
double trap_atan_unit(const double t) {
    const double TAN_15 = 0.26794919243112270LF;
    const double TAN_30 = 0.57735026918962576LF;
    double base = 0.0LF;
    double c = 0.0LF;
    if (t > 0.76732698797896042LF) {
        base = TRAP_PI / 4.0LF;
        c = 1.0LF;
    } else if (t > 0.41421356237309505LF) {
        base = TRAP_PI / 6.0LF;
        c = TAN_30;
    } else if (t > 0.13165249758739586LF) {
        base = TRAP_PI / 12.0LF;
        c = TAN_15;
    }
    precise const double r = (t - c) / (1.0LF + t * c);
    precise const double r2 = r * r;
    double sum = 0.0LF;
    for (int k = 19; k >= 1; k -= 2) {
        sum = 1.0LF / double(k) - r2 * sum;
    }
    precise const double a = base + r * sum;
    return a;
}

// orbit_trap_shape::angle in double, which GLSL has no atan for: arg(x + i y) in [-pi, pi].
double trap_atan2(const double y, const double x) {
    const double ax = abs(x);
    const double ay = abs(y);
    if (ax == 0.0LF && ay == 0.0LF) {
        return 0.0LF;
    }
    double a = ay > ax ? TRAP_PI / 2.0LF - trap_atan_unit(ax / ay) : trap_atan_unit(ay / ax);
    if (x < 0.0LF) {
        a = TRAP_PI - a;
    }
    return y < 0.0LF ? -a : a;
}

// orbit_trap_shape::sectorDistance: (u, v) folded into the half sector of a corner, then the distance to its segment.
double trap_sector_distance(const double u, const double v) {
    const double sides = double(params.trap_sides);
    const double k = floor(trap_atan2(v, u) * sides / (2.0LF * TRAP_PI) + 0.5LF);
    // The turn back by k sectors, as the k-th power of one sector's turn taken by squaring.
    double c = 1.0LF;
    double s = 0.0LF;
    double bc = params.trap_sector_cos;
    double bs = -params.trap_sector_sin;
    uint n = uint(abs(k));
    if (k < 0.0LF) {
        bs = -bs;
    }
    while (n != 0u) {
        if ((n & 1u) != 0u) {
            precise const double nc = c * bc - s * bs;
            precise const double ns = c * bs + s * bc;
            c = nc;
            s = ns;
        }
        precise const double qc = bc * bc - bs * bs;
        precise const double qs = 2.0LF * bc * bs;
        bc = qc;
        bs = qs;
        n >>= 1u;
    }
    precise double x = u * c - v * s;
    precise double y = abs(u * s + v * c);
    precise const double beyond = y * params.trap_half_cos - x * params.trap_half_sin;
    if (beyond > 0.0LF) {
        precise const double along = x * params.trap_half_cos + y * params.trap_half_sin;
        precise const double mx = along * (2.0LF * params.trap_half_cos) - x;
        precise const double my = along * (2.0LF * params.trap_half_sin) - y;
        x = mx;
        y = my;
    }
    precise const double ex = params.trap_inner_x - params.trap_radius;
    precise const double ey = params.trap_inner_y;
    precise const double ax = x - params.trap_radius;
    const double t = clamp((ax * ex + y * ey) / (ex * ex + ey * ey), 0.0LF, 1.0LF);
    precise const double qx = ax - ex * t;
    precise const double qy = y - ey * t;
    return trap_magnitude(qx, qy);
}

// 2D take on GMT Fractals' geometric orbit trap, as in OrbitTrapShape.h; see NOTICE.
// orbit_trap_shape::distance: the distance from z - c = (dr, di) to the trap.
double trap_shape_distance(const double dr, const double di) {
    const uint kind = params.trap_shape_kind;
    if (kind == TRAP_SHAPE_POINT) {
        return trap_magnitude(dr, di);
    }
    if (kind == TRAP_SHAPE_CIRCLE) {
        return abs(trap_magnitude(dr, di) - params.trap_radius);
    }
    const bool sized = params.trap_radius > 0.0LF;
    if (kind == TRAP_SHAPE_RINGS) {
        return sized ? abs(trap_step_residual(trap_magnitude(dr, di), params.trap_radius)) : trap_magnitude(dr, di);
    }
    precise const double u = dr * params.trap_cos + di * params.trap_sin;
    precise const double v = di * params.trap_cos - dr * params.trap_sin;
    if (kind == TRAP_SHAPE_CROSS || (kind == TRAP_SHAPE_GRID_LINES && !sized)) {
        return min(abs(u), abs(v));
    }
    if (kind == TRAP_SHAPE_GRID_LINES) {
        return min(abs(trap_step_residual(u, params.trap_radius)), abs(trap_step_residual(v, params.trap_radius)));
    }
    if (kind == TRAP_SHAPE_GRID_POINTS) {
        return sized ? trap_magnitude(trap_step_residual(u, params.trap_radius), trap_step_residual(v, params.trap_radius))
                     : trap_magnitude(dr, di);
    }
    if (kind == TRAP_SHAPE_POLYGON || kind == TRAP_SHAPE_STAR) {
        return sized && params.trap_sides != 0u ? trap_sector_distance(u, v) : trap_magnitude(dr, di);
    }
    if (kind == TRAP_SHAPE_SPIRAL) {
        const double rho = trap_magnitude(dr, di);
        if (!sized) {
            return rho;
        }
        double turns = trap_atan2(v, u) / (2.0LF * TRAP_PI);
        if (turns < 0.0LF) {
            turns += 1.0LF;
        }
        const double k = max(trap_nearest_step(rho / params.trap_radius - turns, 1.0LF), 0.0LF);
        precise const double arm = rho - params.trap_radius * (k + turns);
        return min(abs(arm), rho);
    }
    return abs(u);
}

// arg(dr + i di) in float, with the pair scaled first so a tiny pair keeps its direction.
float trap_angle(const double dr, const double di) {
    const double m = max(abs(dr), abs(di));
    if (m == 0.0LF) {
        return 0.0;
    }
    return atan(float(di / m), float(dr / m));
}

// One orbit point inside the bailout; norm is |z|^2.
void sample_trap(inout PixelState s, const double zr, const double zi, const double norm) {
    if (!(norm <= params.bailout2)) {
        return;
    }
    const float cross = log2_wide(min(abs(zr), abs(zi)));
    const float point = log2_wide(norm);
    const uint iteration = uint(s.iteration % uint64_t(TRAP_ITERATION_WRAP));
    if (params.trap_shape_kind != 0) {
        precise const double dr = zr - params.trap_center_r;
        precise const double di = zi - params.trap_center_i;
        const float shape = log2_wide(trap_shape_distance(dr, di));
        if (s.trap_samples == 0 || shape < s.trap_shape) {
            s.trap_shape = shape;
            s.trap_shape_iteration = iteration;
            s.trap_shape_angle = trap_angle(dr, di);
        }
    }
    if (s.trap_samples == 0) {
        s.trap_cross = cross;
        s.trap_point = point;
        s.trap_iteration = iteration;
        s.trap_samples = 1;
        return;
    }
    s.trap_cross = min(s.trap_cross, cross);
    if (point < s.trap_point) {
        s.trap_second = s.trap_point;
        s.trap_point = point;
        s.trap_iteration = iteration;
        s.trap_samples = 2;
    } else if (s.trap_samples == 1 || point < s.trap_second) {
        s.trap_second = point;
        s.trap_samples = 2;
    }
}

// ---- Per-pixel entry and exit ----

// The pixel this invocation works on, or false when it is outside the image or already settled.
bool load_running_pixel(out uint index, out PixelState s) {
    const uvec2 pixel = gl_GlobalInvocationID.xy;
    index = 0;
    if (pixel.x >= params.extent.x || pixel.y >= params.extent.y) {
        return false;
    }
    index = pixel.y * params.extent.x + pixel.x;
    s = states.state[index];
    return s.status == STATUS_RUNNING;
}

void store_pixel(const uint index, const PixelState s) {
    states.state[index] = s;
    PixelResult r;
    r.pd = s.pd;
    r.cd = s.cd;
    r.count = params.absolute_mode != 0 ? s.abs_iteration : s.iteration;
    r.status = s.status;
    r.escape_angle = 0.0;
    r.trap_cross = s.trap_samples > 0 ? max(s.trap_cross, TRAP_HIT_LOG2) : TRAP_FAR_LOG2;
    r.trap_point = s.trap_samples > 0 ? max(0.5 * s.trap_point, TRAP_HIT_LOG2) : TRAP_FAR_LOG2;
    r.trap_second = s.trap_samples > 1 ? max(0.5 * s.trap_second, TRAP_HIT_LOG2) : TRAP_FAR_LOG2;
    r.trap_iteration = float(s.trap_iteration + 1u);
    const bool shaped = params.trap_shape_kind != 0 && s.trap_samples > 0;
    r.trap_shape = shaped ? max(s.trap_shape, TRAP_HIT_LOG2) : TRAP_FAR_LOG2;
    r.trap_shape_iteration = shaped ? float(s.trap_shape_iteration + 1u) : 0.0;
    r.trap_shape_angle = shaped ? s.trap_shape_angle : 0.0;
    r.trap_shape_status = shaped ? 1.0 : 0.0;
    if (s.status == STATUS_FINISHED) {
        // After the last step z is the reference point plus dz, the same sum step_double tested against the bailout.
        const dvec2 z = reference.orbit[uint(s.ref_iteration)] + dvec2(s.dzr, s.dzi);
        r.escape_angle = atan(float(z.y), float(z.x));
    }
    results.result[index] = r;
}

// ---- One step of LightMandelbrotPerturbator::iterate in double, after the MPA skip ----

// Advances the reference by one, then rebases or ends the pixel exactly as the CPU loop does.
void step_double(inout PixelState s, const dvec2 dc) {
    if (s.ref_iteration != params.max_ref_iteration) {
        const dvec2 ref = reference.orbit[uint(s.ref_iteration)];
        precise const double zr1 = ref.x * 2 + s.dzr;
        precise const double zi1 = ref.y * 2 + s.dzi;
        precise const double zr2 = zr1 * s.dzr - zi1 * s.dzi + dc.x;
        precise const double zi2 = zr1 * s.dzi + zi1 * s.dzr + dc.y;
        s.dzr = zr2;
        s.dzi = zi2;
        ++s.ref_iteration;
        ++s.iteration;
        ++s.abs_iteration;
    }

    const dvec2 ref = reference.orbit[uint(s.ref_iteration)];
    precise const double zr = ref.x + s.dzr;
    precise const double zi = ref.y + s.dzi;

    // The CPU's shortcut for a point on the real axis inside the set.
    if (zi == 0 && zr < 0.25LF && zr >= -2.0LF) {
        s.status = STATUS_INTERIOR;
        return;
    }

    s.pd = s.cd;
    precise const double cd = zr * zr + zi * zi;
    s.cd = cd;
    sample_trap(s, zr, zi, cd);

    precise const double dzNorm = s.dzr * s.dzr + s.dzi * s.dzi;
    if (s.ref_iteration == params.max_ref_iteration || cd < dzNorm) {
        s.ref_iteration = 0;
        s.dzr = zr;
        s.dzi = zi;
    }

    if (cd > params.bailout2) {
        s.status = STATUS_FINISHED;
    }
}

#endif
