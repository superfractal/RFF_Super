//
// Created by Opus 5.5 on 2026-11-05
// Modified by Opus 5.5 on 2026-11-06
//

// The zoom between two keyframes calculated in a 360 projection, shared by vk_2_map_iter_stripe.comp and vk_static_2_image.frag.
// Original RFF_Super inverse of RenderScene's 360 projections, GPL-3.0-only, elementary pinhole and stereographic geometry; see NOTICE.
// Reads video_attr.keyframe_camera (projection, yaw, pitch, tan of half the field of view) and video_attr.keyframe_view (layout, fold radius).
#ifndef RFF_KEYFRAME_PROJECTION
#define RFF_KEYFRAME_PROJECTION

bool keyframes_projected() {
    return video_attr.keyframe_camera.x > 0.5;
}

// The view direction of a keyframe position, given as ((px + 0.5) / w, (py + 0.5) / h) with buffer row 0 at the bottom, as RenderScene casts it.
vec3 keyframe_direction(vec2 n, vec2 size) {
    float yaw = video_attr.keyframe_camera.y;
    if (video_attr.keyframe_camera.x < 1.5) {
        float lambda = 6.283185307179586 * (n.x - 0.5) + yaw;
        float alpha = 3.141592653589793 * n.y;
        return vec3(sin(alpha) * sin(lambda), -cos(alpha), sin(alpha) * cos(lambda));
    }
    float pitch = video_attr.keyframe_camera.z;
    float ta = video_attr.keyframe_camera.w;
    vec3 forward = vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
    vec3 right = vec3(cos(yaw), 0.0, -sin(yaw));
    vec3 up = vec3(-sin(pitch) * sin(yaw), cos(pitch), -sin(pitch) * cos(yaw));
    float a = (2.0 * n.x - 1.0) * ta;
    float b = (2.0 * n.y - 1.0) * ta * size.y / size.x;
    return normalize(forward + a * right + b * up);
}

// The keyframe position a direction is seen at, the inverse of keyframe_direction; false behind the camera.
bool keyframe_position(vec3 d, vec2 size, out vec2 n) {
    float yaw = video_attr.keyframe_camera.y;
    if (video_attr.keyframe_camera.x < 1.5) {
        n = vec2(fract((atan(d.x, d.z) - yaw) / 6.283185307179586 + 0.5), acos(clamp(-d.y, -1.0, 1.0)) / 3.141592653589793);
        return true;
    }
    float pitch = video_attr.keyframe_camera.z;
    float ta = video_attr.keyframe_camera.w;
    vec3 forward = vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
    vec3 right = vec3(cos(yaw), 0.0, -sin(yaw));
    vec3 up = vec3(-sin(pitch) * sin(yaw), cos(pitch), -sin(pitch) * cos(yaw));
    float f = dot(d, forward);
    if (f <= 1.0e-6) {
        n = vec2(-1.0);
        return false;
    }
    n = vec2(dot(d, right) / (f * ta), dot(d, up) / (f * ta * size.y / size.x)) * 0.5 + 0.5;
    return true;
}

// Where the plane point under keyframe position n lands once the plane is shrunk by sr about the zoom center; n itself
// for sky and the folded ring, which do not move, and a negative position where the keyframe never saw that point.
vec2 keyframe_plane_zoom(vec2 n, vec2 size, float sr) {
    vec3 d = keyframe_direction(n, size);
    bool ground = video_attr.keyframe_view.x < 0.5;
    float denom = ground ? -d.y : 1.0 - d.y;
    float horizontal = length(d.xz);
    float limit = video_attr.keyframe_view.y;
    if (denom <= 0.0 || horizontal <= 0.0 || !(horizontal < denom * limit)) return n;
    float t = horizontal / denom / sr;
    if (!(t < limit)) return vec2(-1.0);
    // Ground is the tangent of the angle from straight down, Full Sphere the tangent of half of it.
    float theta = ground ? atan(t) : 2.0 * atan(t);
    vec2 heading = d.xz / horizontal;
    vec2 m;
    return keyframe_position(vec3(heading.x * sin(theta), -cos(theta), heading.y * sin(theta)), size, m) ? m : vec2(-1.0);
}

#endif
