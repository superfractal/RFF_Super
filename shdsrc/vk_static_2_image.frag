//
// Modified by Opus 5 on 2026-08-24
// Modified by GPT-6 on 2026-09-08, 2026-09-17, 2026-09-23
//

#version 450

layout (set = 0, binding = 0) uniform sampler2D normal;
layout (set = 0, binding = 1) uniform sampler2D zoomed;

layout (set = 1, binding = 0) uniform VideoUBO {
    float default_zoom_increment;
    float current_frame;
    vec2 sample_jitter;
    bool dither;
    vec4 camera;
    vec4 coverage;
} video_attr;

// The same block the bloom pass reads, which is where a dynamic frame's headroom store is written.
layout (set = 2, binding = 0) uniform BloomUBO {
    float threshold;
    float radius;
    float softness;
    float intensity;
    float hdr;
    float headroom;
    float linear_add;
    float scene_linear;
} bloom_attr;

// IEC 61966-2-1 sRGB EOTF and its inverse, acknowledged in NOTICE.
vec3 srgb_to_linear(vec3 c) {
    vec3 s = max(c, 0.0);
    return mix(s / 12.92, pow((s + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), s));
}

vec3 linear_to_srgb(vec3 c) {
    vec3 s = max(c, 0.0);
    return mix(s * 12.92, 1.055 * pow(s, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), s));
}


layout (location = 0) out vec4 color;


// Original spherical/plane projection; matches RenderScene, with a finite saved-map coverage limit (NOTICE).
vec2 camera_offset(vec2 p, vec2 size, out bool sky) {
    sky = false;
    float scale = max(video_attr.coverage.x, 1.0);
    float yaw = radians(video_attr.camera.x);
    if (video_attr.camera.y < 0.5) {
        return mat2(cos(yaw), -sin(yaw), sin(yaw), cos(yaw)) * p / scale;
    }
    vec3 direction;
    if (video_attr.camera.y < 1.5) {
        float longitude = 6.283185307179586 * p.x / size.x + yaw;
        float alpha = 3.141592653589793 * (0.5 - p.y / size.y);
        direction = vec3(sin(alpha) * sin(longitude), -cos(alpha), sin(alpha) * cos(longitude));
    } else {
        float pitch = radians(video_attr.camera.z);
        vec3 forward = vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
        vec3 right = vec3(cos(yaw), 0.0, -sin(yaw));
        vec3 up = vec3(-sin(pitch) * sin(yaw), cos(pitch), -sin(pitch) * cos(yaw));
        vec2 q = 2.0 * vec2(p.x, -p.y) / size.x * tan(radians(video_attr.camera.w) * 0.5);
        direction = normalize(forward + q.x * right + q.y * up);
    }
    float denom = video_attr.coverage.z < 0.5 ? -direction.y : 1.0 - direction.y;
    if (video_attr.coverage.z < 0.5 && denom <= 0.0) {
        sky = true;
        return vec2(0.0);
    }
    float horizontal = length(direction.xz);
    float limit = min(pow(10.0, video_attr.coverage.y), max(0.0, (min(size.x, size.y) - 8.0) * scale / size.y));
    float radius = denom > 0.0 ? min(horizontal / denom, limit) : limit;
    return horizontal > 0.0 ? size.y * 0.5 / scale * radius * vec2(direction.x, -direction.z) / horizontal : vec2(0.0);
}

void main(){
    vec2 resolution = vec2(textureSize(normal, 0));
    bool sky;
    vec2 coord = camera_offset(gl_FragCoord.xy + video_attr.sample_jitter - resolution * 0.5, resolution, sky) / resolution + 0.5;
    if (sky) { color = vec4(0.0, 0.0, 0.0, 1.0); return; }

    float r = int(max(0, video_attr.current_frame)) - video_attr.current_frame;

    float normalScale = pow(video_attr.default_zoom_increment, r + 1);// r = 0 ~ 1
    float zoomedScale = pow(video_attr.default_zoom_increment, r);// r = -1 ~ 0


    vec2 normalUv = (coord - 0.5) / normalScale + 0.5;
    vec2 zoomedUv = (coord - 0.5) / zoomedScale + 0.5;
    vec4 normalColor = texture(normal, normalUv).bgra;
    color = normalColor;
    if (video_attr.current_frame >= 1) {
        vec2 edgeDistance = min(zoomedUv, vec2(1.0) - zoomedUv) * resolution;
        float feather = smoothstep(3.0, 6.0, min(edgeDistance.x, edgeDistance.y));
        float zoomWeight = (r + 1.0) * feather;
        if (zoomWeight > 0.0) {
            color = mix(normalColor, texture(zoomed, zoomedUv).bgra, zoomWeight);
        }
    }

    // A keyframe image skips the bloom pass, so the headroom store it makes is done here instead.
    if (bloom_attr.hdr > 0.5 && bloom_attr.scene_linear < 0.5) {
        color = vec4(linear_to_srgb(srgb_to_linear(color.rgb) / max(bloom_attr.headroom, 1e-3)), color.a);
    }
    color = clamp(color, 0.0, 1.0);

}
