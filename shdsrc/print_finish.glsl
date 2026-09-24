//
// Modified by GPT-6 on 2026-09-13, 2026-09-16, 2026-09-19, 2026-09-23
//

// Original RFF_Super nearest editable print palette, GPL-3.0-only; transfer helpers retain their existing provenance in NOTICE.
vec3 print_finish(vec3 color) {
    float amount = linear_interpolation_attr.layer_quantize * linear_interpolation_attr.dark_strength * linear_interpolation_attr.ukiyo_flatness;
    if (amount <= 0.0) {
        return color;
    }

    vec3 inks[6] = vec3[6](
        vec3(linear_interpolation_attr.print_ink_r, linear_interpolation_attr.print_ink_g, linear_interpolation_attr.print_ink_b),
        vec3(linear_interpolation_attr.print_indigo_r, linear_interpolation_attr.print_indigo_g, linear_interpolation_attr.print_indigo_b),
        vec3(linear_interpolation_attr.print_asagi_r, linear_interpolation_attr.print_asagi_g, linear_interpolation_attr.print_asagi_b),
        vec3(linear_interpolation_attr.print_blue_r, linear_interpolation_attr.print_blue_g, linear_interpolation_attr.print_blue_b),
        vec3(linear_interpolation_attr.print_foam_r, linear_interpolation_attr.print_foam_g, linear_interpolation_attr.print_foam_b),
        vec3(linear_interpolation_attr.print_paper_r, linear_interpolation_attr.print_paper_g, linear_interpolation_attr.print_paper_b));
    int levels = clamp(int(round(linear_interpolation_attr.ukiyo_colors)), 3, 6);
    vec3 nearest = color;
    float best = 1e30;
    for (int i = 0; i < levels; ++i) {
        int index = i == levels - 1 ? 5 : i;
        vec3 ink = inks[index];
        if (linear_interpolation_attr.transfer > 0.5 && !ordered_layers()) {
            ink = output_transform(linear_interpolation_attr.scene_linear > 0.5 ? srgb_to_linear(ink) : ink);
        }

        vec3 delta = color - ink;
        float distance = dot(delta, delta);
        if (distance < best) {
            best = distance;
            nearest = ink;
        }
    }
    return mix(color, nearest, amount);
}
