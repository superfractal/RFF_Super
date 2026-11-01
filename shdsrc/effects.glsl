//
// Modified by GPT-6 on 2026-09-11, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-23, 2026-09-30
// Modified by Opus 5.5 on 2026-10-20
//

// Original RFF_Super procedural effects, GPL-3.0-or-later; provenance is recorded in NOTICE.
layout(set = 3, binding = 0) uniform EffectsUBO {
    vec4 layer[24];
    vec4 context;
    vec4 rain[4];
} effects_attr;

layout(set = 4, binding = 0) uniform EffectsTimeUBO {
    float current_time;
    float palette_phase;
    float flow_phase;
    float stripe_phase;
    float t0u;
    float t0v;
    float t1u;
    float t1v;
    float t2u;
    float t2v;
    float t3u;
    float t3v;
    float p0u;
    float p0v;
    float p1u;
    float p1v;
    float p2u;
    float p2v;
    float p3u;
    float p3v;
    float wu;
    float wv;
    float e0t;
    float e0v;
    float e1t;
    float e1v;
    float e2t;
    float e2v;
    float e3t;
    float e3v;
} effects_time;

float effect_random(ivec2 cell, float seed) {
    uint n = uint(cell.x) * 197u + uint(cell.y) * 1999u + uint(seed) * 101u + 17u;
    n ^= n >> 11u;
    n *= 7919u;
    n ^= n >> 7u;
    n *= 104729u;
    return float(n & 0x00ffffffu) / 16777216.0;
}

float effect_noise(vec2 p, float seed) {
    ivec2 cell = ivec2(floor(p));
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(effect_random(cell, seed), effect_random(cell + ivec2(1, 0), seed), f.x),
               mix(effect_random(cell + ivec2(0, 1), seed), effect_random(cell + ivec2(1, 1), seed), f.x), f.y);
}

vec2 effect_phase(int i) {
    if (i == 0) {
        return vec2(effects_time.e0t, effects_time.e0v);
    }
    if (i == 1) {
        return vec2(effects_time.e1t, effects_time.e1v);
    }
    if (i == 2) {
        return vec2(effects_time.e2t, effects_time.e2v);
    }
    return vec2(effects_time.e3t, effects_time.e3v);
}

struct SurfaceEffects {
    float height;
    float wetness;
    float coverage;
    vec3 tint;
    vec3 emission;
    vec4 veil;
};

bool effects_enabled() {
    if (effects_attr.context.y < 0.5) {
        return false;
    }
    for (int i = 0; i < 4; ++i) {
        if (ordered_layers() && !layer_is(19 + i)) {
            continue;
        }
        if (effects_attr.layer[i * 6].x > 0.5 && effects_attr.layer[i * 6 + 1].x > 0.0) {
            return true;
        }
    }
    return false;
}

float effect_cycle(double iteration, float period) {
    double value = iteration / double(period);
    return float(value - floor(value));
}

float surface_field(vec2 uv, float time, float seed) {
    float u = uv.x * 6.28318530718;
    float v = uv.y * 6.28318530718;
    return 0.5 + 0.22 * sin(u + 2.0 * sin(v + time) + seed) +
           0.16 * sin(3.0 * u - 5.0 * v + time * 0.71 + seed * 0.17) +
           0.10 * sin(7.0 * u + 11.0 * v - time * 0.43);
}

float surface_rain(vec2 uv, float time, float seed, vec4 detail, float scale,
                   vec2 footprint, bool ember, vec2 rain, vec2 du, vec2 dv, vec2 slope) {
    float columns = max(4.0, round(48.0 / scale));
    float rows = 8.0;
    vec2 pos = vec2(uv.y * columns, uv.x * rows - time * (ember ? -0.8 : 1.5));
    pos.x += sin(radians(detail.w)) * sin(uv.x * 6.28318530718) * 1.5;
    ivec2 cell = ivec2(floor(pos));
    vec2 aa = max(vec2(footprint.y * columns, footprint.x * rows), vec2(0.006));
    float bendDerivative = sin(radians(detail.w)) * cos(uv.x * 6.28318530718) * 9.42477796077;
    mat2 surfaceToGrid = mat2(vec2(du.y * columns + bendDerivative * du.x, du.x * rows),
                             vec2(dv.y * columns + bendDerivative * dv.x, dv.x * rows));
    float determinant = determinant(surfaceToGrid);
    bool drops = !ember && rain.x > 0.5;
    if (drops && abs(determinant) < 1e-14) {
        return 0.0;
    }
    mat2 gridToSurface = drops ? inverse(surfaceToGrid) : mat2(1.0);
    float radius = rain.y * float(max(iteration_info_attr.canvas_extent.y, 1u)) / 720.0 * 3.0;
    radius = min(radius, 0.65 / max(length(surfaceToGrid[0]), length(surfaceToGrid[1])));
    float dropSupport = max(0.4, radius * 1.3) + 0.75;
    vec2 reach = drops
        ? dropSupport * vec2(length(vec2(surfaceToGrid[0].x, surfaceToGrid[1].x)),
                             length(vec2(surfaceToGrid[0].y, surfaceToGrid[1].y)))
        : vec2((ember ? 0.045 : 0.035) * detail.z + aa.x,
               (ember ? 0.09 : 0.44) * detail.y * 1.6 + aa.y);
    ivec2 searchRadius = max(ivec2(1), ivec2(ceil(reach)));
    float total = 0.0;
    [[dont_unroll]] for (int yi = 0; yi <= searchRadius.y * 2; ++yi) {
        int y = yi % 2 == 0 ? yi / 2 : -(yi + 1) / 2;
        [[dont_unroll]] for (int xi = 0; xi <= searchRadius.x * 2; ++xi) {
            int x = xi % 2 == 0 ? xi / 2 : -(xi + 1) / 2;
            ivec2 id = cell + ivec2(x, y);
            ivec2 wrapped = ivec2(mod(vec2(id), vec2(columns, rows)));
            float r = effect_random(wrapped, seed);
            if (r > detail.x) {
                continue;
            }
            vec2 center = vec2(id) + vec2(effect_random(wrapped, seed + 5.0),
                                          effect_random(wrapped, seed + 13.0));
            vec2 delta = pos - center;
            if (drops) {
                vec2 tangent = gridToSurface * delta;
                float projectedHeight = dot(clamp(slope, vec2(-2.0), vec2(2.0)), tangent);
                float distance = sqrt(dot(tangent, tangent) + projectedHeight * projectedHeight);
                float dropRadius = max(0.4, radius * (0.7 + 0.6 * r));
                float edge = 1.0 - smoothstep(max(0.0, dropRadius - 0.75), dropRadius + 0.75, distance);
                float dome = exp(-2.0 * distance * distance / (dropRadius * dropRadius));
                total += dome * edge;
                if (total >= 1.0) return 1.0;
                continue;
            }
            float width = (ember ? 0.045 : 0.035) * detail.z;
            float length = (ember ? 0.09 : 0.44) * detail.y * (0.6 + r);
            float cross = 1.0 - smoothstep(width, width + aa.x, abs(delta.x));
            float along = 1.0 - smoothstep(length * 0.12, length + aa.y, abs(delta.y));
            float filtering = min(1.0, 0.4 / max(aa.x, aa.y));
            total += cross * along * filtering;
            if (total >= 1.0) return 1.0;
        }
    }
    return clamp(total, 0.0, 1.0);
}

// ---- Effect styles: alternative looks chosen per type; style 0 keeps the classic code in surface_effects ----

struct EffectStyle {
    float coverage;
    float height;
    float wetness;
    float glow;
    vec3 tint;
    float tintWeight;
    vec3 glowTint;
    vec4 veil;
};

EffectStyle effect_style_none(vec3 primary) {
    return EffectStyle(0.0, 0.0, 0.0, 0.0, primary, -1.0, vec3(-1.0), vec4(0.0));
}

// Value noise on an integer lattice that wraps at period, so the band and aspect seams stay continuous.
float effect_periodic_noise(vec2 p, vec2 period, float seed) {
    vec2 cell = floor(p);
    vec2 f = p - cell;
    f = f * f * (3.0 - 2.0 * f);
    vec2 c0 = mod(cell, period);
    vec2 c1 = mod(cell + 1.0, period);
    float a = effect_random(ivec2(c0), seed);
    float b = effect_random(ivec2(c1.x, c0.y), seed);
    float c = effect_random(ivec2(c0.x, c1.y), seed);
    float d = effect_random(ivec2(c1), seed);
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractional Brownian motion sum of the periodic noise; background in NOTICE.
float effect_fbm(vec2 p, vec2 period, float seed, int octaves) {
    float sum = 0.0;
    float amplitude = 0.5;
    float norm = 0.0;
    [[dont_unroll]] for (int i = 0; i < octaves; ++i) {
        sum += amplitude * effect_periodic_noise(p, period, seed + float(i) * 7.0);
        norm += amplitude;
        p *= 2.0;
        period *= 2.0;
        amplitude *= 0.5;
    }
    return sum / norm;
}

// Distance between the nearest and second-nearest moving cell points (Worley F2 - F1); see NOTICE.
float effect_cell_edge(vec2 p, vec2 period, float seed, float time) {
    vec2 cell = floor(p);
    vec2 f = p - cell;
    float f1 = 8.0;
    float f2 = 8.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            ivec2 wrapped = ivec2(mod(cell + vec2(x, y), period));
            vec2 o = vec2(effect_random(wrapped, seed), effect_random(wrapped, seed + 1.0));
            o = 0.5 + 0.4 * sin(time * (0.6 + 0.8 * o.yx) + 6.28318530718 * o);
            float d = length(vec2(x, y) + o - f);
            if (d < f1) {
                f2 = f1;
                f1 = d;
            } else if (d < f2) {
                f2 = d;
            }
        }
    }
    return f2 - f1;
}

struct EffectGrid {
    vec2 pos;
    mat2 toGrid;
    mat2 toScreen;
    bool valid;
};

// The rain lattice mapping with its pixel Jacobian, so particles stay round on screen.
EffectGrid effect_grid(vec2 uv, vec2 du, vec2 dv, float columns, float rows, float bend) {
    EffectGrid g;
    float bendAmount = sin(radians(bend));
    g.pos = vec2(uv.y * columns + bendAmount * sin(uv.x * 6.28318530718) * 1.5, uv.x * rows);
    float bendDerivative = bendAmount * cos(uv.x * 6.28318530718) * 9.42477796077;
    g.toGrid = mat2(vec2(du.y * columns + bendDerivative * du.x, du.x * rows),
                    vec2(dv.y * columns + bendDerivative * dv.x, dv.x * rows));
    g.valid = abs(determinant(g.toGrid)) > 1e-14;
    g.toScreen = g.valid ? inverse(g.toGrid) : mat2(1.0);
    return g;
}

float effect_cell_pixels(EffectGrid g) {
    return 1.0 / max(1e-6, max(length(g.toGrid[0]), length(g.toGrid[1])));
}

EffectStyle effect_flame_turbulent(vec2 uv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary, vec3 secondary) {
    EffectStyle s = effect_style_none(primary);
    vec2 period = vec2(max(1.0, round(12.0 / (max(detail.z, 0.1) * scale))), 8.0);
    vec2 q = vec2(uv.y * period.x, uv.x * 8.0 - mod(phase.x * 0.9, 8.0));
    float warp = effect_fbm(q + vec2(0.0, mod(phase.y * 0.3 - phase.x * 0.6, 8.0)), period, seed + 3.0, 3) - 0.5;
    q.x += warp * 2.2 * detail.y;
    float n = effect_fbm(q, period, seed, 5);
    float flame = (n - 0.5) * 1.8 + 0.45 * detail.x - fract(uv.x * 2.0);
    float body = smoothstep(0.0, 0.3, flame);
    float heat = clamp(flame * 2.0, 0.0, 1.0);
    vec3 ramp = heat < 0.5 ? mix(secondary * 0.7, secondary, heat * 2.0) : mix(secondary, primary, heat * 2.0 - 1.0);
    float smoke = smoothstep(-0.28, -0.02, flame) * (1.0 - body) * 0.8;
    s.coverage = body;
    s.glow = body * (0.35 + 0.65 * heat);
    s.height = body * 0.3;
    s.glowTint = ramp;
    s.tint = mix(vec3(0.35, 0.32, 0.3), ramp, body);
    s.tintWeight = max(body * 0.25, smoke);
    return s;
}

EffectStyle effect_flame_burning_edge(vec2 uv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary, vec3 secondary) {
    EffectStyle s = effect_style_none(primary);
    float across = max(1.0, round(24.0 / scale));
    float front = fract(phase.x * 0.05);
    float rag = effect_fbm(vec2(uv.y * across, 0.5), vec2(across, 1.0), seed, 4) - 0.5;
    float d = uv.x - front + rag * 0.14 * detail.y;
    d -= round(d);
    float w = 0.01 * detail.z;
    float flicker = 0.7 + 0.3 * effect_fbm(vec2(uv.y * across * 2.0, uv.x * 4.0 + mod(phase.y * 2.0, 4.0)),
                                           vec2(across * 2.0, 4.0), seed + 9.0, 3);
    float edge = exp(-d * d / (w * w)) * flicker;
    float lick = smoothstep(0.0, 0.04 * detail.y, d) * (1.0 - smoothstep(0.04 * detail.y, 0.1 * detail.y, d));
    float tongueAcross = max(1.0, round(30.0 / scale));
    float tongue = lick * smoothstep(0.55, 0.85, effect_fbm(vec2(uv.y * tongueAcross, uv.x * 6.0 - mod(phase.x * 0.8, 6.0)),
                                                             vec2(tongueAcross, 6.0), seed + 4.0, 4));
    float behind = -d;
    float charred = smoothstep(0.0, 0.02, behind) * (1.0 - smoothstep(0.08, 0.45, behind));
    float sparkAcross = max(1.0, round(160.0 / scale));
    float sparks = smoothstep(0.82, 0.95, effect_periodic_noise(vec2(uv.y * sparkAcross, uv.x * 40.0 + mod(phase.y * 3.0, 40.0)),
                                                                vec2(sparkAcross, 40.0), seed + 21.0)) *
                   charred * (1.0 - smoothstep(0.02, 0.2, behind));
    float strength = clamp(detail.x * 2.0, 0.0, 1.0);
    float lit = clamp(edge + tongue * 0.8 + sparks, 0.0, 1.0) * strength;
    s.coverage = lit;
    s.glow = lit;
    s.height = edge * 0.25 - charred * 0.08;
    s.glowTint = mix(secondary, primary, clamp(edge * 1.4, 0.0, 1.0));
    s.tint = mix(vec3(0.18, 0.12, 0.09), s.glowTint, lit);
    s.tintWeight = max(lit * 0.25, charred * 0.75 * strength);
    return s;
}

EffectStyle effect_embers_rising(vec2 uv, vec2 du, vec2 dv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary, vec3 secondary) {
    EffectStyle s = effect_style_none(primary);
    float columns = max(4.0, round(32.0 / scale));
    float rows = 6.0;
    EffectGrid g = effect_grid(uv, du, dv, columns, rows, detail.w);
    if (!g.valid) {
        return s;
    }
    float pixelScale = float(max(iteration_info_attr.canvas_extent.y, 1u)) / 720.0;
    float radius = max(0.6, 1.3 * pixelScale * detail.z);
    float rise = 1.6 * detail.y / 0.65;
    ivec2 cell = ivec2(floor(g.pos));
    int reachY = min(int(ceil(rise)) + 1, 6);
    float total = 0.0;
    vec3 colorSum = vec3(0.0);
    [[dont_unroll]] for (int y = -1; y <= reachY; ++y) {
        [[dont_unroll]] for (int x = -1; x <= 1; ++x) {
            ivec2 id = cell + ivec2(x, y);
            ivec2 wrapped = ivec2(mod(vec2(id), vec2(columns, rows)));
            float life = phase.x * 0.18 * (0.6 + 0.8 * effect_random(wrapped, seed + 3.0)) + effect_random(wrapped, seed + 5.0);
            float generation = floor(life);
            float age = life - generation;
            float spawn = seed + 20.0 + mod(generation, 977.0) * 3.0;
            if (effect_random(wrapped, spawn) > detail.x) {
                continue;
            }
            vec2 start = vec2(id) + vec2(effect_random(wrapped, spawn + 1.0), effect_random(wrapped, spawn + 2.0));
            vec2 head = start + vec2(0.35 * sin(age * 9.0 + effect_random(wrapped, spawn + 3.0) * 6.28318530718) * age, -age * rise);
            vec2 toPixel = g.toScreen * (g.pos - head);
            vec2 tail = g.toScreen * vec2(0.0, 0.18 * detail.y / 0.65);
            float along = clamp(dot(toPixel, tail) / max(dot(tail, tail), 1e-6), 0.0, 1.0);
            float distance = length(toPixel - tail * along);
            float spot = (1.0 - smoothstep(radius * 0.5, radius + 0.8, distance)) * mix(1.0, 0.3, along);
            float twinkle = 0.65 + 0.35 * sin(phase.y * 9.0 + effect_random(wrapped, spawn + 4.0) * 6.28318530718 + age * 30.0);
            float bright = spot * smoothstep(0.0, 0.05, age) * (1.0 - smoothstep(0.7, 1.0, age)) * twinkle;
            float hot = (1.0 - age) * (1.0 - age);
            total += bright;
            colorSum += mix(secondary * 0.75, mix(primary, vec3(1.0), 0.45), hot) * bright;
        }
    }
    float coverage = clamp(total, 0.0, 1.0);
    s.coverage = coverage;
    s.glow = coverage;
    s.height = coverage * 0.15;
    s.tint = total > 0.0 ? colorSum / total : secondary;
    return s;
}

EffectStyle effect_embers_burst(vec2 uv, vec2 du, vec2 dv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary, vec3 secondary) {
    EffectStyle s = effect_style_none(primary);
    float columns = max(4.0, round(16.0 / scale));
    float rows = 4.0;
    EffectGrid g = effect_grid(uv, du, dv, columns, rows, detail.w);
    if (!g.valid) {
        return s;
    }
    float pixelScale = float(max(iteration_info_attr.canvas_extent.y, 1u)) / 720.0;
    float reach = min(30.0 * pixelScale * detail.y / 0.65, effect_cell_pixels(g) * 0.95);
    float width = max(0.7, 1.1 * pixelScale * detail.z);
    ivec2 cell = ivec2(floor(g.pos));
    float total = 0.0;
    vec3 colorSum = vec3(0.0);
    [[dont_unroll]] for (int y = -1; y <= 1; ++y) {
        [[dont_unroll]] for (int x = -1; x <= 1; ++x) {
            ivec2 id = cell + ivec2(x, y);
            ivec2 wrapped = ivec2(mod(vec2(id), vec2(columns, rows)));
            float life = phase.x * 0.3 * (0.6 + 0.8 * effect_random(wrapped, seed + 3.0)) + effect_random(wrapped, seed + 5.0);
            float generation = floor(life);
            float age = life - generation;
            float spawn = seed + 40.0 + mod(generation, 977.0) * 3.0;
            if (effect_random(wrapped, spawn) > detail.x * 1.2) {
                continue;
            }
            vec2 center = vec2(id) + vec2(effect_random(wrapped, spawn + 1.0), effect_random(wrapped, spawn + 2.0));
            vec2 toPixel = g.toScreen * (g.pos - center);
            float distance = length(toPixel);
            if (distance > reach * 1.2) {
                continue;
            }
            float fade = pow(1.0 - age, 1.5) * smoothstep(0.0, 0.02, age);
            float flash = exp(-distance * distance / (9.0 * pixelScale * pixelScale + 1.0)) * (1.0 - smoothstep(0.0, 0.08, age)) * 1.5;
            float sector = floor((atan(toPixel.y, toPixel.x) / 6.28318530718 + 0.5) * 9.0);
            float spark = 0.0;
            for (int j = -1; j <= 1; ++j) {
                float index = mod(sector + float(j), 9.0);
                ivec2 ray = ivec2(int(index), wrapped.x * 7 + wrapped.y);
                if (effect_random(ray, spawn + 5.0) > 0.8) {
                    continue;
                }
                float angle = (index + 0.5 + (effect_random(ray, spawn + 6.0) - 0.5) * 0.7) / 9.0 * 6.28318530718 - 3.14159265359;
                vec2 direction = vec2(cos(angle), sin(angle));
                float along = dot(toPixel, direction);
                float side = abs(dot(toPixel, vec2(-direction.y, direction.x)));
                float travelled = reach * (0.55 + 0.45 * effect_random(ray, spawn + 7.0)) * (1.0 - (1.0 - age) * (1.0 - age));
                float trail = reach * 0.35 * (1.0 - age) + 1.0;
                float segment = smoothstep(travelled - trail, travelled, along) * (1.0 - smoothstep(travelled, travelled + 1.0, along));
                spark = max(spark, segment * (1.0 - smoothstep(width * 0.5, width * 0.5 + 0.8, side)));
            }
            float bright = (spark + flash) * fade;
            float hot = (1.0 - age) * (1.0 - age);
            total += bright;
            colorSum += mix(secondary * 0.8, mix(primary, vec3(1.0), 0.4), hot) * bright;
        }
    }
    float coverage = clamp(total, 0.0, 1.0);
    s.coverage = coverage;
    s.glow = coverage;
    s.height = coverage * 0.1;
    s.tint = total > 0.0 ? colorSum / total : secondary;
    return s;
}

EffectStyle effect_mist_drifting(vec2 uv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary) {
    EffectStyle s = effect_style_none(primary);
    vec2 period = vec2(max(1.0, round(6.0 / scale)), 3.0);
    vec2 p = uv.yx * period;
    vec2 warp = vec2(effect_fbm(p + vec2(mod(phase.x * 0.15, period.x), 0.0), period, seed + 11.0, 4),
                     effect_fbm(p + vec2(3.0, mod(phase.y * 0.1, period.y)), period, seed + 23.0, 4));
    float broad = effect_fbm(p + (warp - 0.5) * 2.2 * detail.y / 0.65 + vec2(mod(phase.x * 0.08, period.x), 0.0), period, seed, 5);
    vec2 finePeriod = period * 2.0;
    float fine = effect_fbm(uv.yx * finePeriod + vec2(mod(-phase.x * 0.25, finePeriod.x), mod(phase.y * 0.05, finePeriod.y)),
                            finePeriod, seed + 31.0, 4);
    float density = smoothstep(0.45 - detail.x * 0.3, 0.8, broad * 0.7 + fine * 0.3);
    density = pow(density, 1.0 / max(detail.z, 0.1));
    s.coverage = density;
    s.height = (density - 0.5) * 0.15;
    s.tintWeight = 0.0;
    s.veil = vec4(primary, density * 0.85);
    return s;
}

EffectStyle effect_mist_pooled(vec2 uv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary) {
    EffectStyle s = effect_style_none(primary);
    float across = max(1.0, round(10.0 / scale));
    float level = fract(uv.x * 2.0);
    float drift = effect_fbm(vec2(uv.y * across - mod(phase.x * 0.2, across), uv.x * 4.0), vec2(across, 4.0), seed, 4);
    float wispAcross = max(1.0, round(24.0 / scale));
    float wisps = effect_fbm(vec2(uv.y * wispAcross + mod(phase.x * 0.35, wispAcross), uv.x * 8.0 + mod(phase.y * 0.05, 8.0)),
                             vec2(wispAcross, 8.0), seed + 9.0, 3);
    float top = (0.2 + 0.5 * detail.x) * (0.6 + 0.8 * drift) * detail.y / 0.65;
    float density = 1.0 - smoothstep(top * 0.35, top, level + (wisps - 0.5) * 0.18 * detail.z);
    density *= (0.6 + 0.4 * wisps) * smoothstep(0.0, 0.025, level);
    s.coverage = density;
    s.tintWeight = 0.0;
    s.veil = vec4(primary, density * 0.8);
    return s;
}

EffectStyle effect_ripples_interference(vec2 uv, vec2 phase, vec4 detail, float seed, vec3 primary) {
    EffectStyle s = effect_style_none(primary);
    float wave = 0.0;
    for (int i = 0; i < 4; ++i) {
        ivec2 key = ivec2(i, 77);
        vec2 source = vec2(effect_random(key, seed), effect_random(key, seed + 1.0)) +
                      0.06 * vec2(sin(phase.y * 0.3 + float(i) * 1.7), cos(phase.y * 0.23 + float(i)));
        vec2 d = uv - source;
        d -= round(d);
        float r = length(d);
        wave += sin(r * 6.28318530718 * 10.0 * 0.65 / detail.y - phase.x * 3.0 + float(i)) / (1.0 + r * 5.0);
    }
    wave *= 0.5;
    float crest = pow(clamp(wave, 0.0, 1.0), 2.0 / detail.z);
    s.height = clamp(wave, -1.0, 1.0) * detail.x * 1.4;
    s.coverage = crest * detail.x;
    s.wetness = s.coverage;
    s.tint = vec3(1.0);
    s.tintWeight = 0.0;
    return s;
}

EffectStyle effect_ripples_caustics(vec2 uv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary) {
    EffectStyle s = effect_style_none(primary);
    float fineAcross = max(2.0, round(14.0 * 0.65 / (detail.y * scale)));
    float coarseAcross = max(1.0, round(fineAcross * 0.65));
    float fine = effect_cell_edge(vec2(uv.y * fineAcross, uv.x * 5.0), vec2(fineAcross, 5.0), seed, phase.x * 0.8);
    float coarse = effect_cell_edge(vec2(uv.y * coarseAcross, uv.x * 3.0), vec2(coarseAcross, 3.0), seed + 5.0, phase.x * 0.6 + 2.0);
    float light = pow(1.0 - smoothstep(0.0, 0.12 * detail.z, fine), 1.5) * 0.75 +
                  pow(1.0 - smoothstep(0.0, 0.16 * detail.z, coarse), 1.5) * 0.5;
    float strength = detail.x / 0.45;
    s.coverage = clamp(light, 0.0, 1.0);
    s.height = light * 0.08;
    s.tint = vec3(1.0) + primary * light * 1.6 * strength;
    s.tintWeight = 1.0;
    s.glow = light * 0.25 * strength;
    s.glowTint = primary;
    return s;
}

EffectStyle effect_flow_chase(vec2 uv, vec2 phase, vec4 detail, vec3 primary, vec3 secondary) {
    EffectStyle s = effect_style_none(primary);
    float bands = 10.0;
    float band = floor(uv.x * bands);
    float within = fract(uv.x * bands);
    float age = mod(phase.x * 1.5 - band, bands);
    float afterglow = exp(-age * 0.9 * 0.65 / detail.y);
    float edge = pow(abs(within - 0.5) * 2.0, 3.0 / detail.z);
    float shimmer = 0.75 + 0.25 * sin(uv.y * 6.28318530718 * 6.0 + phase.y * 2.0);
    float lit = afterglow * (0.15 + 0.85 * edge) * shimmer * clamp(detail.x * 2.0, 0.0, 1.0);
    s.coverage = lit;
    s.glow = lit;
    s.height = lit * 0.1;
    s.tint = mix(secondary, primary, afterglow);
    return s;
}

EffectStyle effect_aurora_curtain(vec2 uv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary, vec3 secondary) {
    EffectStyle s = effect_style_none(primary);
    float across = max(1.0, round(6.0 / scale));
    float hem = 0.35 + 0.15 * sin(uv.y * 6.28318530718 * 2.0 + phase.y * 0.4 + seed) +
                0.5 * (effect_fbm(vec2(uv.y * across + mod(phase.x * 0.05, across), 0.5), vec2(across, 4096.0), seed + 3.0, 4) - 0.5) *
                detail.y / 0.65;
    float d = uv.x - hem;
    d -= round(d);
    float rise = 0.22 * detail.z;
    float body = smoothstep(-0.012, 0.0, d) * exp(-max(d, 0.0) / rise * 2.2);
    float rayAcross = max(1.0, round(90.0 / scale));
    float rays = pow(effect_fbm(vec2(uv.y * rayAcross + mod(phase.x * 0.4, rayAcross), mod(phase.y * 0.2, 4096.0)),
                                vec2(rayAcross, 4096.0), seed + 7.0, 3), 1.6);
    float hemLight = exp(-abs(d) / 0.006);
    float lit = clamp((body * (0.25 + 1.2 * rays) + hemLight * 0.5 * rays) * detail.x * 1.6, 0.0, 1.0);
    s.coverage = lit;
    s.glow = lit;
    s.height = lit * 0.1;
    s.tint = mix(primary, secondary, smoothstep(0.02, rise, d));
    return s;
}

EffectStyle effect_aurora_ribbons(vec2 uv, vec2 phase, vec4 detail, float seed, float scale, vec3 primary, vec3 secondary) {
    EffectStyle s = effect_style_none(primary);
    float across = max(1.0, round(5.0 / scale));
    float foldCount = max(1.0, round(24.0 / scale));
    float total = 0.0;
    vec3 colorSum = vec3(0.0);
    for (int i = 0; i < 3; ++i) {
        float fi = float(i);
        float sway = effect_fbm(vec2(uv.y * across + mod(phase.x * 0.04 * (1.0 + fi), across), fi * 3.0),
                                vec2(across, 4096.0), seed + fi * 13.0, 4) - 0.5;
        float center = (fi + 0.5) / 3.0 + 0.08 * sin(uv.y * 6.28318530718 * (2.0 + fi) + phase.y * 0.5 + fi * 2.1) +
                       0.24 * sway * detail.y / 0.65;
        float d = uv.x - center;
        d -= round(d);
        float w = 0.035 * detail.z;
        float direction = i == 1 ? -1.0 : 1.0;
        float folds = 0.55 + 0.45 * sin(uv.y * 6.28318530718 * foldCount + sway * 8.0 - phase.x * 1.5 * direction);
        float v = exp(-d * d / (w * w)) * folds;
        total += v;
        colorSum += mix(primary, secondary, 0.5 + 0.5 * sin(uv.y * 6.28318530718 + fi * 2.0 + phase.y * 0.2)) * v;
    }
    float lit = clamp(total * detail.x * 1.6, 0.0, 1.0);
    s.coverage = lit;
    s.glow = lit;
    s.height = lit * 0.1;
    s.tint = total > 1e-4 ? colorSum / total : primary;
    return s;
}

EffectStyle effect_style(int type, int style, vec2 uv, vec2 du, vec2 dv, vec2 phase, vec4 detail, float seed, float scale,
                         vec3 primary, vec3 secondary) {
    if (type == 1) {
        if (style == 1) return effect_flame_turbulent(uv, phase, detail, seed, scale, primary, secondary);
        return effect_flame_burning_edge(uv, phase, detail, seed, scale, primary, secondary);
    }
    if (type == 2) {
        if (style == 1) return effect_embers_rising(uv, du, dv, phase, detail, seed, scale, primary, secondary);
        return effect_embers_burst(uv, du, dv, phase, detail, seed, scale, primary, secondary);
    }
    if (type == 3) {
        if (style == 1) return effect_mist_drifting(uv, phase, detail, seed, scale, primary);
        return effect_mist_pooled(uv, phase, detail, seed, scale, primary);
    }
    if (type == 5) {
        if (style == 1) return effect_ripples_interference(uv, phase, detail, seed, primary);
        return effect_ripples_caustics(uv, phase, detail, seed, scale, primary);
    }
    if (type == 6) {
        return effect_flow_chase(uv, phase, detail, primary, secondary);
    }
    if (style == 1) return effect_aurora_curtain(uv, phase, detail, seed, scale, primary, secondary);
    return effect_aurora_ribbons(uv, phase, detail, seed, scale, primary, secondary);
}

SurfaceEffects surface_effects(double iteration, vec2 gradient, vec2 slope) {
    SurfaceEffects result = SurfaceEffects(0.0, 0.0, 0.0, vec3(1.0), vec3(0.0), vec4(0.0));
    if (!effects_enabled()) {
        return result;
    }
    float aspect = dot(gradient, gradient) > 1e-30 ? atan(gradient.y, gradient.x) / 6.28318530718 + 0.5 : 0.0;
    bool exterior = iteration > 0.0 && iteration < iteration_info_attr.max_value && effects_attr.context.y > 0.5;
    [[dont_unroll]] for (int i = 0; i < 4; ++i) {
        if (ordered_layers() && !layer_is(19 + i)) {
            continue;
        }
        vec4 mode = effects_attr.layer[i * 6];
        vec4 shape = effects_attr.layer[i * 6 + 1];
        vec4 detail = effects_attr.layer[i * 6 + 2];
        vec4 extra = effects_attr.layer[i * 6 + 3];
        vec4 primary = effects_attr.layer[i * 6 + 4];
        vec3 secondary = effects_attr.layer[i * 6 + 5].rgb;
        vec2 phase = effect_phase(i);
        vec2 uv = vec2(effect_cycle(max(iteration, 0.0), extra.w * shape.y), aspect);
        vec2 du = dFdx(uv);
        vec2 dv = dFdy(uv);
        du -= round(du);
        dv -= round(dv);
        vec2 footprint = abs(du) + abs(dv);
        if (!exterior || mode.x < 0.5 || shape.x <= 0.0 || detail.x <= 0.0 || int(mode.w) == 2 || int(mode.y) == 4) {
            continue;
        }
        float mask = 1.0;
        if (int(mode.w) == 3) {
            mask = 0.5 + 0.5 * cos(uv.x * 6.28318530718);
        }
        if (int(mode.w) == 4) {
            mask = smoothstep(0.00002, 0.005, length(gradient));
        }
        float field = surface_field(uv, phase.y, extra.z);
        float coverage = 0.0;
        float height = 0.0;
        float wetness = 0.0;
        float glow = 0.0;
        vec3 tint = primary.rgb;
        int type = int(mode.y);
        int style = int(effects_attr.rain[i].z + 0.5);
        float tintWeight = -1.0;
        vec3 glowTint = vec3(-1.0);
        vec4 veil = vec4(0.0);
        if (type != 0 && style > 0) {
            EffectStyle styled = effect_style(type, style, uv, du, dv, phase, detail, extra.z, shape.y, primary.rgb, secondary);
            coverage = styled.coverage;
            height = styled.height;
            wetness = styled.wetness;
            glow = styled.glow;
            tint = styled.tint;
            tintWeight = styled.tintWeight;
            glowTint = styled.glowTint;
            veil = styled.veil;
        } else if (type == 0 || type == 2) {
            coverage = surface_rain(uv, phase.x, extra.z, detail, shape.y, footprint, type == 2, effects_attr.rain[i].xy, du, dv, slope);
            if (type == 0) {
                height = coverage * (0.5 + 0.5 * field);
                wetness = coverage;
                tint = vec3(1.0);
            } else {
                coverage *= 0.6 + 0.4 * sin(phase.y * 2.0 + field * 12.0);
                height = coverage * 0.15;
                glow = coverage;
                tint = mix(secondary, primary.rgb, coverage);
            }
        } else if (type == 1) {
            float travel = (uv.x - phase.x * 0.1) * 6.28318530718;
            float tongues = 0.5 + 0.5 * sin(uv.y * 18.8495559215 + sin(travel + phase.y) * detail.y * 3.0 + extra.z);
            float rising = tongues * (0.55 + 0.45 * surface_field(vec2(uv.x - phase.x * 0.1, uv.y), phase.y, extra.z));
            rising = pow(max(rising, 0.0), 1.0 / detail.z);
            coverage = smoothstep(0.65 - detail.x * 0.35, 0.9, rising);
            height = coverage * 0.3;
            glow = coverage;
            tint = mix(secondary, primary.rgb, clamp(coverage * 1.7, 0.0, 1.0));
        } else if (type == 3) {
            vec2 cloudUV = vec2(uv.x - phase.x * 0.03, uv.y + sin(uv.x * 6.28318530718 + phase.y) * detail.y * 0.2);
            field = pow(clamp(surface_field(cloudUV, phase.y, extra.z), 0.0, 1.0), 1.0 / detail.z);
            coverage = field * detail.x;
            height = (field - 0.5) * detail.x * 0.15;
        } else if (type == 5) {
            vec2 torus = sin(uv * 6.28318530718 + vec2(extra.z, extra.z * 0.3));
            float radius = length(torus);
            float wave = sin(radius * 16.0 / detail.y - phase.x * 3.0);
            float fade = 0.5 + 0.5 * sin(radius * 2.0 - phase.y);
            float ring = pow(0.5 + 0.5 * wave, 2.0 / detail.z);
            coverage = ring * fade * detail.x;
            height = (ring * 2.0 - 1.0) * fade * detail.x;
            wetness = coverage;
            tint = vec3(1.0);
        } else if (type == 6) {
            float travel = fract(uv.x - phase.x * 0.1);
            float distance = min(travel, 1.0 - travel);
            float width = detail.z * 0.008;
            coverage = (1.0 - smoothstep(width, width + max(footprint.x, 0.003), distance)) * detail.x;
            coverage *= pow(0.5 + 0.5 * sin(uv.y * 18.8495559215 + phase.y), 0.65 / detail.y);
            height = coverage * 0.1;
            glow = coverage;
        } else {
            float curtain = uv.x * 6.28318530718 + sin(uv.y * 12.5663706144 + phase.y) * detail.y;
            coverage = pow(0.5 + 0.5 * sin(curtain - phase.x * 0.4), 8.0 / detail.z) * field * detail.x;
            height = coverage * 0.15;
            glow = coverage;
            tint = mix(secondary, primary.rgb, field);
        }
        float amount = shape.x * mask * primary.a;
        float addedHeight = height * extra.x * amount;
        if (int(mode.z) == 0) {
            result.height = mix(result.height, height * extra.x, amount);
        } else if (int(mode.z) == 2) {
            result.height += (1.0 - clamp(result.height, 0.0, 1.0)) * addedHeight;
        } else if (int(mode.z) == 3) {
            result.height = (1.0 + result.height) * (1.0 + addedHeight) - 1.0;
        } else {
            result.height += addedHeight;
        }
        result.wetness = clamp(result.wetness + wetness * amount, 0.0, 1.0);
        result.coverage = max(result.coverage, coverage * amount);
        float tintAmount = tintWeight >= 0.0 ? tintWeight * amount : coverage * amount * (type == 0 || type == 5 ? 0.0 : 0.25);
        result.tint *= mix(vec3(1.0), tint, tintAmount);
        if (veil.a > 0.0) {
            float cover = clamp(veil.a * amount, 0.0, 1.0);
            result.veil = vec4(result.veil.rgb * (1.0 - cover) + veil.rgb * cover, result.veil.a * (1.0 - cover) + cover);
        }
        if (glow > 0.0) {
            vec3 emission = (glowTint.x >= 0.0 ? glowTint : tint) * glow * amount * extra.y;
            if (int(mode.z) == 0) {
                result.emission = mix(result.emission, emission, amount);
            } else if (int(mode.z) == 3) {
                result.emission *= mix(vec3(1.0), emission, amount);
            } else if (int(mode.z) == 2) {
                result.emission += max(vec3(0.0), vec3(1.0) - result.emission) * emission;
            } else {
                result.emission += emission;
            }
        }
    }
    return result;
}

// Original ordered relief-light transfer using the existing diffuse controls, GPL-3.0-only; see NOTICE.
float surface_effect_light(vec3 normal) {
    float z = radians(slope_attr.zenith);
    float a = radians(slope_attr.azimuth);
    float light = dot(normal, vec3(sin(z) * cos(a), sin(z) * sin(a), cos(z)));
    if (slope_attr.fill_intensity > 0.0) {
        float fz = radians(slope_attr.fill_zenith);
        float fa = radians(slope_attr.fill_azimuth);
        float fill = dot(normal, vec3(sin(fz) * cos(fa), sin(fz) * sin(fa), cos(fz)));
        light = clamp(light + clamp(fill, 0.0, 1.0) * clamp(slope_attr.fill_intensity, 0.0, 1.0), 0.0, 1.0);
    }
    float shade = max(slope_attr.reflection_ratio, clamp(light, 0.0, 1.0));
    float softness = slope_attr.terminator_softness;
    if (softness > 0.0) {
        shade = mix(shade, mix(slope_attr.reflection_ratio, 1.0,
                               clamp((light + softness) / (1.0 + softness), 0.0, 1.0)), softness);
    }
    if (slope_attr.gamma > 0.0) {
        shade = pow(shade, 1.0 / slope_attr.gamma);
    }
    return mix(1.0, shade, clamp(slope_attr.luma_amount, 0.0, 1.0));
}

vec3 surface_effect_relief(vec3 base, vec3 before, vec3 after) {
    float original = surface_effect_light(before);
    float changed = surface_effect_light(after);
    if (original == changed) {
        return base;
    }
    return base * clamp(changed / max(original, 0.0001), 0.0, 60000.0);
}

vec3 surface_effect_finish(vec3 base, SurfaceEffects effects, vec3 normal, bool linear) {
    vec3 light = vec3(sin(radians(slope_attr.zenith)) * cos(radians(slope_attr.azimuth)),
                      sin(radians(slope_attr.zenith)) * sin(radians(slope_attr.azimuth)), cos(radians(slope_attr.zenith)));
    // Blinn (1977) half-vector power highlight; mathematical background in NOTICE; project license unchanged.
    vec3 halfDirection = normalize(light + vec3(0.0, 0.0, 1.0));
    float reflected = pow(max(dot(normal, halfDirection), 0.0), 64.0) * effects.wetness * 0.35;
    vec3 emission = linear ? srgb_to_linear(clamp(effects.emission, 0.0, 1.0)) * max(1.0, max(effects.emission.r, max(effects.emission.g, effects.emission.b))) : effects.emission;
    if (effects.veil.a > 0.0) {
        vec3 veilColor = clamp(effects.veil.rgb / effects.veil.a, 0.0, 1.0);
        veilColor = linear ? srgb_to_linear(veilColor) : veilColor;
        base = mix(base, veilColor * (0.55 + 0.45 * clamp(dot(normal, light), 0.0, 1.0)), effects.veil.a);
    }
    vec3 result = base * effects.tint * (1.0 - effects.wetness * 0.08) + vec3(reflected) + emission;
    return max(result, 0.0);
}

