//
// Modified by Opus 5 on 2026-08-24
//

#version 450

layout (set = 0, binding = 0) uniform sampler2D normal;
layout (set = 0, binding = 1) uniform sampler2D zoomed;

layout (set = 1, binding = 0) uniform VideoUBO {
    float default_zoom_increment;
    float current_frame;
} video_attr;

// The same block the bloom pass reads, which is where a dynamic frame's headroom store is written.
layout (set = 2, binding = 0) uniform BloomUBO {
    float threshold;
    float radius;
    float softness;
    float intensity;
    float hdr;
    float headroom;
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


layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexcoord;

layout (location = 0) out vec4 color;

void main(){
    vec2 resolution = vec2(textureSize(normal, 0));
    vec2 coord = gl_FragCoord.xy / resolution;

    float r = int(max(0, video_attr.current_frame)) - video_attr.current_frame;

    float nsr = pow(video_attr.default_zoom_increment, r + 1);// r = 0 ~ 1
    float zsr = pow(video_attr.default_zoom_increment, r);// r = -1 ~ 0


    int off = 3;
    vec2 ntx = (coord - 0.5) / nsr + 0.5;
    vec2 ztx = (coord - 0.5) / zsr + 0.5;
    vec2 px = off / resolution;

    if (ztx.x >= 1 - px.x || ztx.y >= 1 - px.y || ztx.x <= px.x || ztx.y <= px.y || video_attr.current_frame < 1){
        color = texture(normal, ntx).bgra;
    } else {
        color = texture(normal, ntx).bgra * (-r) + texture(zoomed, ztx).bgra * (r + 1);
    }

    // A keyframe image skips the bloom pass, so the headroom store it makes is done here instead.
    if (bloom_attr.hdr > 0.5) {
        color = vec4(linear_to_srgb(srgb_to_linear(color.rgb) / max(bloom_attr.headroom, 1e-3)), color.a);
    }
    color = clamp(color, 0.0, 1.0);

}