//
// Modified by GPT-6 on 2026-09-16, 2026-09-19, 2026-09-23
//

#ifndef RFF_SHADER_LAYER
#define RFF_SHADER_LAYER
layout(push_constant) uniform ShaderLayerPush {
    ivec4 control;
    vec4 line_color;
    vec4 line_params;
    vec4 line_spines;
    vec4 line_ornaments;
    vec4 line_glow;
    vec4 line_glow_color;
} shader_layer;

bool ordered_layers() {
    return shader_layer.control.x != 0;
}

bool layer_is(int id) {
    return shader_layer.control.x == id;
}

#endif
