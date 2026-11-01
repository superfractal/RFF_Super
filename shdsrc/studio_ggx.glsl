//
// Modified by GPT-6 on 2026-09-10, 2026-09-18, 2026-09-19, 2026-09-23
// Modified by Opus 5.5 on 2026-10-08
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

vec3 studio_ggx(vec3 n, vec3 v, vec3 l, float ax, float ay, float angle, vec3 f0) {
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

    vec3 scaledH = vec3(dot(h, t) / ax, dot(h, b) / ay, dot(h, n));
    float q = dot(scaledH, scaledH);
    float distribution = 1.0 / max(3.141592653589793 * ax * ay * q * q, 1e-20);

    float maskV = nl * length(vec3(ax * dot(v, t), ay * dot(v, b), nv));
    float maskL = nv * length(vec3(ax * dot(l, t), ay * dot(l, b), nl));
    float visibility = 0.5 / max(maskV + maskL, 1e-12);

    return distribution * visibility * studio_fresnel(f0, dot(v, h)) * nl;
}

vec3 studio_brdf(vec3 n, vec3 v, vec3 l, float roughness, float anisotropy, float angle, vec3 f0) {
    float alpha = max(0.02, roughness * roughness);
    float stretch = 1.0 + clamp(anisotropy, 0.0, 1.0);
    return studio_ggx(n, v, l, alpha * stretch, alpha / stretch, angle, f0);
}

// Clear-coat softbox: a rectangle facing the surface along lightDir, its long side horizontal, its half-extents in units of its distance.
const vec2 STUDIO_SOFTBOX_HALF_SIZE = vec2(0.2, 0.06);

// Representative-point area light (Karis 2013, see NOTICE): light from the box point nearest the mirror ray, lobe widened by the box.
vec3 studio_softbox(vec3 n, vec3 v, vec3 lightDir, float lightAzimuth, float roughness, vec3 f0) {
    vec3 across;
    vec3 up;
    studio_basis(lightDir, lightAzimuth + 1.5707963267948966, across, up);
    vec3 mirror = reflect(-v, n);
    vec2 hit = vec2(dot(mirror, across), dot(mirror, up)) / max(dot(mirror, lightDir), 1e-3);
    vec2 nearest = clamp(hit, -STUDIO_SOFTBOX_HALF_SIZE, STUDIO_SOFTBOX_HALF_SIZE);
    vec3 l = normalize(lightDir + across * nearest.x + up * nearest.y);
    float alpha = roughness * roughness;
    // The half vector turns half as far as the light, so the box widens each lobe axis by half its extent; scaling keeps the lobe's energy.
    vec2 widened = alpha + STUDIO_SOFTBOX_HALF_SIZE * 0.5;
    return studio_ggx(n, v, l, alpha, alpha, 0.0, f0) * (alpha * alpha / (widened.x * widened.y));
}

vec3 studio_environment_rotate(vec3 ray, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    ray.xy = mat2(c, -s, s, c) * ray.xy;
    return ray;
}

// Original hemispheric environment light retained without rectangular emitters; GPL-3.0-only, see NOTICE.
vec3 studio_environment(vec3 ray, float angle, vec3 sky, vec3 ground) {
    ray = studio_environment_rotate(ray, angle);
    return mix(ground, sky, clamp(0.5 + 0.5 * ray.y, 0.0, 1.0)) * 0.12;
}
