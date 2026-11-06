//
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-18, 2026-09-19, 2026-09-23
// Modified by Opus 5.5 on 2026-10-08, 2026-11-04
//

// Artist-directed GGX and dielectric coating over native relief; equation provenance is recorded in NOTICE.
vec3 studio_surface(
    vec3 base,
    vec3 diffuseBase,
    vec3 n,
    vec3 v,
    float aoFactor,
    float variance,
    float footprint,
    float boundaryMask,
    float wetness
) {
    float specZenith = radians(slope_attr.specular_link < 0.5 ? slope_attr.specular_zenith : slope_attr.zenith);
    float specAngle = radians(slope_attr.specular_link < 0.5 ? slope_attr.specular_azimuth : slope_attr.azimuth);
    vec3 keyDir = vec3(
        cos(specAngle) * sin(specZenith),
        sin(specAngle) * sin(specZenith),
        cos(specZenith)
    );
    vec3 fillDir = vec3(
        cos(radians(slope_attr.fill_azimuth)) * sin(radians(slope_attr.fill_zenith)),
        sin(radians(slope_attr.fill_azimuth)) * sin(radians(slope_attr.fill_zenith)),
        cos(radians(slope_attr.fill_zenith))
    );
    vec3 ns = n;
    float xy = length(n.xy);
    if (xy > 1e-6) {
        float actualTilt = atan(xy, max(n.z, 0.0));
        float anchor = acos(clamp(keyDir.z, -1.0, 1.0)) * 0.5;
        float tilt = mix(anchor, actualTilt, clamp(slope_attr.relief_response, 0.0, 1.0));
        tilt = mix(actualTilt, tilt, xy / (xy + 0.03));
        ns = vec3(n.xy / xy * sin(tilt), cos(tilt));
    }
    float rough = mix(clamp(slope_attr.studio_roughness, 0.04, 1.0), 0.09, wetness * 0.8);
    if (variance > 0.0) {
        rough = clamp(pow(pow(rough, 4.0) + variance, 0.25), 0.04, 1.0);
    }
    if (slope_attr.studio_model > 0.5) {
        return openpbr_surface(base, diffuseBase, n, ns, v, keyDir, fillDir, specAngle, rough, aoFactor, variance, footprint, boundaryMask);
    }
    float metal = clamp(slope_attr.studio_metalness, 0.0, 1.0);
    float ior = clamp(slope_attr.studio_ior, 1.0, 3.0);
    float dielectric = (ior - 1.0) / (ior + 1.0);
    vec3 f0 = mix(vec3(dielectric * dielectric), clamp(base, 0.0, 1.0), metal);
    // Artistic three-band thin-film tint from optical path difference; independent code, references in NOTICE.
    if (slope_attr.surface_iridescence > 0.0) {
        float cosine = clamp(dot(ns, v), 0.0, 1.0);
        float transmitted = sqrt(max(0.0, 1.0 - (1.0 - cosine * cosine) / (1.38 * 1.38)));
        vec3 phase = 6.28318530718 * 2.0 * 1.38
                   * slope_attr.surface_film_thickness * transmitted / vec3(650, 510, 475);
        vec3 tint = 0.5 + 0.5 * cos(phase);
        f0 = mix(f0, clamp(f0 * (0.5 + tint), 0.0, 1.0), slope_attr.surface_iridescence);
    }
    vec3 key = srgb_to_linear(vec3(
        slope_attr.specular_color_r, slope_attr.specular_color_g, slope_attr.specular_color_b
    ));
    vec3 sky = srgb_to_linear(vec3(
        slope_attr.sky_color_r, slope_attr.sky_color_g, slope_attr.sky_color_b
    ));
    vec3 ground = srgb_to_linear(vec3(
        slope_attr.ground_color_r, slope_attr.ground_color_g, slope_attr.ground_color_b
    ));
    vec2 anisotropy = studio_anisotropy(n);
    vec3 direct = studio_brdf(ns, v, keyDir, rough, anisotropy.x, anisotropy.y, f0) * key;
    direct += studio_brdf(ns, v, fillDir, rough, anisotropy.x, anisotropy.y, f0)
            * sky * clamp(slope_attr.fill_intensity, 0.0, 1.0);
    direct *= slope_attr.studio_direct_intensity * 3.0;
    float environmentAngle = radians(slope_attr.studio_environment_rotation)
                           + specAngle * clamp(slope_attr.studio_environment_follow, 0.0, 1.0);
    vec3 env = studio_environment_source(reflect(-v, ns), environmentAngle, sky, ground, rough * rough, footprint)
             * studio_fresnel(f0, dot(ns, v)) * slope_attr.studio_environment_intensity * mix(1.0, aoFactor, 0.3);
    float coat = clamp(slope_attr.studio_clearcoat, 0.0, 1.0);
    vec3 coatLight = vec3(0);
    float attenuation = 1.0;
    if (coat > 0.0) {
        float coatRough = clamp(slope_attr.studio_clearcoat_roughness, 0.04, 1.0);
        if (variance > 0.0) {
            coatRough = clamp(pow(pow(coatRough, 4.0) + variance, 0.25), 0.04, 1.0);
        }
        // Waves on Clear Coat Only keeps the waves here alone, the rest of the surface having been shaded without them; prior art in NOTICE.
        vec3 coatNormal = slope_attr.surface_waves_coat_only > 0.5 && slope_attr.surface_relief_waves > 0.0 &&
                          slope_attr.surface_lustre_relief > 0.5
                          ? normalize(vec3(-coat_wave_slope, 1.0)) : n;
        vec3 coatF = studio_fresnel(vec3(0.04), dot(coatNormal, v));
        attenuation = pow(1.0 - coat * coatF.r, 2.0);
        // The coating uses the actual normal for its basis, Fresnel and reflection direction together.
        // Coat Softbox reflects key and fill as rectangles; off keeps the point lights and their lobe-width floor.
        bool softbox = slope_attr.studio_coat_softbox > 0.5;
        coatLight = (softbox ? studio_softbox(coatNormal, v, keyDir, specAngle, coatRough, vec3(0.04))
                             : studio_brdf(coatNormal, v, keyDir, coatRough, 0.0, 0.0, vec3(0.04))) * key;
        coatLight += (softbox ? studio_softbox(coatNormal, v, fillDir, radians(slope_attr.fill_azimuth), coatRough, vec3(0.04))
                              : studio_brdf(coatNormal, v, fillDir, coatRough, 0.0, 0.0, vec3(0.04)))
                   * sky * clamp(slope_attr.fill_intensity, 0.0, 1.0);
        coatLight *= slope_attr.studio_direct_intensity * 3.0;
        coatLight += studio_environment_source(reflect(-v, coatNormal), environmentAngle, sky, ground, coatRough * coatRough, footprint)
                   * coatF * slope_attr.studio_environment_intensity;
        coatLight *= coat;
    }
    vec3 diffuse = diffuseBase * aoFactor * (1.0 - metal);
    return (diffuse + (direct + env) * max(slope_attr.specular_intensity, 0.0) * boundaryMask)
         * attenuation + coatLight * boundaryMask;
}
