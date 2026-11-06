//
// Created by Opus 5.5 on 2026-11-04
// Modified by Opus 5.5 on 2026-11-06
//

// Original RFF_Super band-following anisotropy axis, GPL-3.0-only; see NOTICE.
// Anisotropy Direction 0 keeps Anisotropy Angle fixed on screen; 1 and 2 turn the stretch along or across the iteration bands, the angle adding a twist.
// A band runs across the relief gradient, and the normal tilts along that gradient, so the band's direction comes from the normal alone.
// Returns the anisotropy, eased out where the surface is too flat for a band to have a direction, and the stretch axis in radians.
vec2 studio_anisotropy(vec3 n) {
    float anisotropy = slope_attr.specular_anisotropy;
    float axis = radians(slope_attr.specular_anisotropy_angle);
    int direction = int(slope_attr.specular_anisotropy_direction + 0.5);
    if (direction == 0) {
        return vec2(anisotropy, axis);
    }
    float xy = length(n.xy);
    if (xy < 1e-6) {
        return vec2(0.0, axis);
    }
    float bands = atan(n.y, n.x) + (direction == 1 ? 1.5707963267948966 : 0.0);
    return vec2(anisotropy * xy / (xy + 0.03), bands + axis);
}
