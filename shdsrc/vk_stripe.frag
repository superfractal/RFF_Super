//
// Modified by Opus 5 on 2026-08-06, 2026-08-26
// Modified by GPT-5 on 2026-08-23
//

#version 450

#define DOUBLE_PI 6.2831853071795864
#define NONE 0
#define SINGLE_DIRECTION 1
#define SMOOTH 2
#define SMOOTH_SQUARED 3

layout (set = 0, binding = 0) uniform sampler2D canvas;

layout (set = 1, binding = 0) uniform IterUBO {
    uvec2 extent;
    double max_value;
} iteration_info_attr;

layout (set = 1, binding = 1) buffer IterSSBO {
    double iterations[];
} iteration_attr;

layout (set = 2, binding = 0) uniform StripeUBO {
    uint type;
    float first_interval;
    float second_interval;
    float opacity;
    float offset;
    float animation_speed;
} stripe_attr;

layout (set = 3, binding = 0) uniform TimeUBO {
    float time;
    // Accumulated animation phases, in the order the host packs them; only the stripe's is read
    // here. Added up on the host instead of taken as time * speed, so editing the speed does not
    // jump the stripes by the whole elapsed time.
    float palette_phase;
    float flow_phase;
    float stripe_phase;
} time_attr;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexcoord;

layout (location = 0) out vec4 color;


double get_iteration(uvec2 iterCoord){
    iterCoord.y = iteration_info_attr.extent.y - 1 - iterCoord.y;
    return iteration_attr.iterations[iterCoord.y * iteration_info_attr.extent.x + iterCoord.x];
}


void main() {

    uvec2 iter_coord = uvec2(gl_FragCoord.xy);
    double iteration = get_iteration(iter_coord);

    // Interior pixels carry no meaningful iteration count, so they keep the palette's Mandelbrot color.
    if (stripe_attr.type == NONE || iteration == 0 || iteration >= iteration_info_attr.max_value) {
        color = texelFetch(canvas, ivec2(gl_FragCoord.xy), 0);
        return;
    }

    double iter_curr = iteration - (stripe_attr.offset + time_attr.stripe_phase);
    float black = 0.0;
    float rat1 = float(mod(iter_curr, stripe_attr.first_interval)) / stripe_attr.first_interval;
    float rat2 = float(mod(iter_curr, stripe_attr.second_interval)) / stripe_attr.second_interval;

    switch (stripe_attr.type) {
        case SINGLE_DIRECTION: {
                                   black = rat1 * rat2;
                                   break;
                               }
        case SMOOTH: {
                                   black = pow((sin(rat1 * DOUBLE_PI) + 1) * (sin(rat2 * DOUBLE_PI) + 1) / 4, 2);
                                   break;
                               }
        case SMOOTH_SQUARED: {
                                   black = pow((sin(rat1 * DOUBLE_PI) + 1) * (sin(rat2 * DOUBLE_PI) + 1) / 4, 4);
                                   break;
                               }
        default: break;
    }

    color = vec4((texelFetch(canvas, ivec2(gl_FragCoord.xy), 0).rgb * (1 - black * stripe_attr.opacity)), 1);
}
