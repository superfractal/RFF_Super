//
// Modified by GPT-6 on 2026-09-16, 2026-09-23
//

// Independent implementation of the user-supplied MFR-HDR 1.0 display equations (2026-09-16); see NOTICE.
vec3 mfr_display(vec3 source, uint mode, float exposure, float peak_nits) {
    vec3 c = BT709_TO_BT2020 * max(source, vec3(0.0)) * exp2(clamp(exposure, -32.0, 32.0));
    float luminance = dot(c, vec3(0.2627, 0.6780, 0.0593));
    if (mode == 7u) {
        if (luminance <= 0.0) {
            return vec3(0.0);
        }

        float band = clamp((log2(luminance) + 16.0) / 8.0, 0.0, 4.0);
        // Original RFF_Super EV legend: navy, blue, neutral gray, yellow, red; no transfer or dither.
        const vec3 stops[5] = vec3[5](
            vec3(0.03, 0.02, 0.15),
            vec3(0.05, 0.35, 1.0),
            vec3(0.5),
            vec3(1.0, 0.85, 0.05),
            vec3(1.0, 0.05, 0.02));
        int index = min(int(band), 3);
        return mix(stops[index], stops[index + 1], band - float(index));
    }
    float peak = max(peak_nits / 203.0, 1e-6);
    float target = luminance;
    if (mode == 4u) {
        target = (luminance / (1.0 + luminance)) / (peak / (1.0 + peak));
    }

    if (mode == 5u) {
        // The small-value log1p series avoids losing dark luminance when 1 + luminance rounds to 1.
        float log_light = luminance < 0.01
            ? luminance * (1.0 + luminance * (-0.5 + luminance * (1.0 / 3.0 - luminance / 4.0)))
            : log(1.0 + luminance);
        target = log_light / log(1.0 + peak);
    }
    target = clamp(target, 0.0, 1.0);
    c *= target / max(luminance, 1e-20);
    vec3 rgb = vec3(
        dot(c, vec3(1.660491, -0.587641, -0.072850)),
        dot(c, vec3(-0.124550, 1.132900, -0.008349)),
        dot(c, vec3(-0.018151, -0.100579, 1.118730)));
    vec3 delta = rgb - vec3(target);
    float saturation = 1.0;
    for (int channel = 0; channel < 3; ++channel) {
        if (delta[channel] > 0.0) {
            saturation = min(saturation, (1.0 - target) / delta[channel]);
        }
        if (delta[channel] < 0.0) {
            saturation = min(saturation, -target / delta[channel]);
        }
    }
    return linear_to_srgb(vec3(target) + max(saturation, 0.0) * delta);
}
