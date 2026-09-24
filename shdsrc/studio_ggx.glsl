//
// Modified by GPT-6 on 2026-09-10, 2026-09-18, 2026-09-19, 2026-09-23
//

// GGX (Walter et al.), correlated Smith (Heitz), Schlick Fresnel; independent GPL implementation, sources/licenses in NOTICE.
vec3 studio_fresnel(vec3 f0, float cosine) {
    float x = 1.0 - clamp(cosine, 0.0, 1.0);
    float x2 = x * x;
    return f0 + (1.0 - f0) * x2 * x2 * x;
}

void studio_basis(vec3 n, float angle, out vec3 t, out vec3 b) {
    vec3 seed = vec3(cos(angle), sin(angle), 0);
    t = seed - n * dot(seed, n);

    if (dot(t, t) < 1e-8) {
        vec3 axis = abs(n.z) < 0.9 ? vec3(0, 0, 1) : vec3(0, 1, 0);
        t = cross(axis, n);
    }

    t = normalize(t);
    b = cross(n, t);
}

vec3 studio_brdf(vec3 n, vec3 v, vec3 l, float roughness, float anisotropy, float angle, vec3 f0) {
    float nv = dot(n, v);
    float nl = dot(n, l);
    vec3 sum = v + l;
    float sum2 = dot(sum, sum);

    if (nv <= 0.0 || nl <= 0.0 || sum2 < 1e-12) {
        return vec3(0);
    }

    vec3 h = sum * inversesqrt(sum2);
    vec3 t;
    vec3 b;
    studio_basis(n, angle, t, b);

    float alpha = max(0.02, roughness * roughness);
    float stretch = 1.0 + clamp(anisotropy, 0.0, 1.0);
    float ax = alpha * stretch;
    float ay = alpha / stretch;

    vec3 scaledH = vec3(dot(h, t) / ax, dot(h, b) / ay, dot(h, n));
    float q = dot(scaledH, scaledH);
    float distribution = 1.0 / max(3.141592653589793 * ax * ay * q * q, 1e-20);

    float maskV = nl * length(vec3(ax * dot(v, t), ay * dot(v, b), nv));
    float maskL = nv * length(vec3(ax * dot(l, t), ay * dot(l, b), nl));
    float visibility = 0.5 / max(maskV + maskL, 1e-12);

    return distribution * visibility * studio_fresnel(f0, dot(v, h)) * nl;
}

// Original hemispheric environment light retained without rectangular emitters; GPL-3.0-only, see NOTICE.
vec3 studio_environment(vec3 ray, float angle, vec3 sky, vec3 ground) {
    float c = cos(angle);
    float s = sin(angle);
    ray.xy = mat2(c, -s, s, c) * ray.xy;
    return mix(ground, sky, clamp(0.5 + 0.5 * ray.y, 0.0, 1.0)) * 0.12;
}
