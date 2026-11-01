//
// Created by Opus 5.5 on 2026-10-13.
//

#version 450

// Stands in for vk_slope.frag while a skipped preparation is still compiling it: the coloured
// picture passes through without relief, lighting or materials until the full shader is ready.
layout (set = 0, binding = 0) uniform sampler2D canvas;

layout (location = 0) out vec4 color;

void main() {
    color = vec4(texelFetch(canvas, ivec2(gl_FragCoord.xy), 0).rgb, 1.0);
}
