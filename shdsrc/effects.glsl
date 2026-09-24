//
// Modified by GPT-6 on 2026-09-11, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-23
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

SurfaceEffects surface_effects(double iteration, vec2 gradient, vec2 slope) {
    SurfaceEffects result = SurfaceEffects(0.0, 0.0, 0.0, vec3(1.0), vec3(0.0));
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
        if (!exterior || mode.x < 0.5 || shape.x <= 0.0 || detail.x <= 0.0 || int(mode.w) == 2) {
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
        if (type == 0 || type == 2) {
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
        } else if (type == 3 || type == 4) {
            vec2 cloudUV = vec2(uv.x - phase.x * 0.03, uv.y + sin(uv.x * 6.28318530718 + phase.y) * detail.y * 0.2);
            field = pow(clamp(surface_field(cloudUV, phase.y, extra.z), 0.0, 1.0), 1.0 / detail.z);
            coverage = field * detail.x;
            height = (field - 0.5) * detail.x * (type == 4 ? 1.0 : 0.15);
            if (type == 4) {
                tint = vec3(1.0);
            }
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
        float tintAmount = coverage * amount * (type == 0 || type == 4 || type == 5 ? 0.0 : 0.25);
        result.tint *= mix(vec3(1.0), tint, tintAmount);
        if (glow > 0.0) {
            vec3 emission = tint * glow * amount * extra.y;
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
    vec3 halfDirection = normalize(light + vec3(0.0, 0.0, 1.0));
    float reflected = pow(max(dot(normal, halfDirection), 0.0), 64.0) * effects.wetness * 0.35;
    vec3 emission = linear ? srgb_to_linear(clamp(effects.emission, 0.0, 1.0)) * max(1.0, max(effects.emission.r, max(effects.emission.g, effects.emission.b))) : effects.emission;
    vec3 result = base * effects.tint * (1.0 - effects.wetness * 0.08) + vec3(reflected) + emission;
    return max(result, 0.0);
}

