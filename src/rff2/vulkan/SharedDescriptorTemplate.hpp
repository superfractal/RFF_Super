//
// Created by Merutilm on 2025-07-19.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 4.8 on 2026-07-05
// Modified by GPT-5 on 2026-07-09, 2026-08-21
// Modified by Opus 5 on 2026-08-06, 2026-08-07, 2026-08-08, 2026-08-10, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-24, 2026-08-26, 2026-08-29, 2026-08-31
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-02
// Modified by GPT-6 on 2026-09-08, 2026-09-10, 2026-09-11, 2026-09-12, 2026-09-13, 2026-09-18, 2026-09-20, 2026-09-22, 2026-09-23, 2026-09-24
//

#pragma once
#include <memory>
#include <glm/glm.hpp>

#include "../attr/ShdPatternAttribute.h"
#include "../attr/ShdEffectsAttribute.h"
#include "../attr/ShdTextureAttribute.h"
#include "../../vulkan_helper/core/factory.hpp"
#include "../../vulkan_helper/manage/DescriptorManager.hpp"
#include "../../vulkan_helper/struct/DescriptorTemplate.hpp"

namespace merutilm::rff2::SharedDescriptorTemplate {
    struct DescEffects final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 11;
        void configure(const vkh::CoreRef core, std::vector<vkh::DescriptorManager> &managers) override {
            auto data = vkh::factory::create<vkh::HostDataObjectManager>();
            for (uint32_t i = 0; i < EFFECT_LAYER_COUNT * 7 + 1; ++i) data->reserve<glm::vec4>(i);
            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(data), vkh::BufferLock::LOCK_UNLOCK, false);
            auto manager = vkh::factory::create<vkh::DescriptorManager>();
            manager->appendUBO(0, VK_SHADER_STAGE_FRAGMENT_BIT, std::move(ubo));
            managers.emplace_back(std::move(manager));
        }
    };

    struct DescTime final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 1;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        static constexpr uint32_t BINDING_UBO_TIME = 0;

        static constexpr uint32_t TARGET_TIME_CURRENT = 0;
        // Accumulated animation phases, in iterations and in flow cycles. They are advanced on the
        // host instead of being derived as time * speed in the shader, so changing a speed keeps
        // the phase it already reached rather than jumping to elapsed * newSpeed.
        static constexpr uint32_t TARGET_TIME_PALETTE_PHASE = 1;
        static constexpr uint32_t TARGET_TIME_FLOW_PHASE = 2;
        // The same for the stripe offset, in iterations.
        static constexpr uint32_t TARGET_TIME_STRIPE_PHASE = 3;
        // And for the decor layers' scroll, in UV. Each layer holds u then v, the texture layers
        // first, then the pattern layers, then the one warp source.
        static constexpr uint32_t TARGET_TIME_TEXTURE_SCROLL = 4;
        static constexpr uint32_t TARGET_TIME_PATTERN_SCROLL = TARGET_TIME_TEXTURE_SCROLL +
                                                               TEXTURE_LAYER_COUNT * 2;
        static constexpr uint32_t TARGET_TIME_WARP_SCROLL = TARGET_TIME_PATTERN_SCROLL +
                                                            PATTERN_LAYER_COUNT * 2;
        static constexpr uint32_t TARGET_TIME_EFFECTS = TARGET_TIME_WARP_SCROLL + 2;
        static constexpr uint32_t TARGET_TIME_COUNT = TARGET_TIME_EFFECTS + EFFECT_LAYER_COUNT * 2;

        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            // Tightly packed floats; scalars need no padding between them, and the shader's block
            // declares them in this order.
            for (uint32_t target = 0; target < TARGET_TIME_COUNT; ++target) {
                bufferManager->reserve<float>(target);
            }
            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager),
                                                          vkh::BufferLock::ALWAYS_MUTABLE, true);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendUBO(BINDING_UBO_TIME, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescIteration final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 2;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        static constexpr uint32_t BINDING_UBO_ITERATION_INFO = 0;
        static constexpr uint32_t BINDING_SSBO_ITERATION_MATRIX = 1;

        static constexpr uint32_t TARGET_UBO_ITERATION_EXTENT = 0;
        static constexpr uint32_t TARGET_UBO_ITERATION_MAX = 1;
        static constexpr uint32_t TARGET_UBO_ITERATION_MAX_NORMAL = 2;
        static constexpr uint32_t TARGET_UBO_ITERATION_MAX_ZOOMED = 3;
        // Appended at the tail so the shaders that declare only the leading fields keep their offsets.
        static constexpr uint32_t TARGET_UBO_ITERATION_CANVAS_EXTENT = 4;
        static constexpr uint32_t TARGET_UBO_ITERATION_CANVAS_OFFSET = 5;

        static constexpr uint32_t TARGET_SSBO_ITERATION_BUFFER = 0;

        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();

            auto infoManager = vkh::factory::create<vkh::HostDataObjectManager>();
            infoManager->reserve<glm::uvec2>(TARGET_UBO_ITERATION_EXTENT);
            infoManager->reserve<double>(TARGET_UBO_ITERATION_MAX);
            infoManager->reserve<double>(TARGET_UBO_ITERATION_MAX_NORMAL);
            infoManager->reserve<double>(TARGET_UBO_ITERATION_MAX_ZOOMED);
            infoManager->reserve<glm::uvec2>(TARGET_UBO_ITERATION_CANVAS_EXTENT);
            infoManager->reserve<glm::ivec2>(TARGET_UBO_ITERATION_CANVAS_OFFSET);

            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserveArray<double>(TARGET_SSBO_ITERATION_BUFFER, 1);

            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(infoManager),
                                                                vkh::BufferLock::LOCK_UNLOCK, false);
            auto ssbo = vkh::factory::create<vkh::ShaderStorage>(core, std::move(bufferManager),
                                                                vkh::BufferLock::LOCK_ONLY, false);
            descManager->appendUBO(BINDING_UBO_ITERATION_INFO, STAGE, std::move(ubo));
            descManager->appendSSBO(BINDING_SSBO_ITERATION_MATRIX, STAGE, std::move(ssbo));

            managers.emplace_back(std::move(descManager));
        }
    };


    struct DescPalette final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 3;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        static constexpr uint32_t BINDING_SSBO_PALETTE = 0;

        // Must match ShdPaletteAttribute::MAX_STATIC_COLORS and the array size in the shaders.
        static constexpr uint32_t MAX_STATIC_COLORS = 16;

        static constexpr uint32_t TARGET_PALETTE_INTERVAL = 0;
        static constexpr uint32_t TARGET_PALETTE_OFFSET = 1;
        static constexpr uint32_t TARGET_PALETTE_SIZE = 2;
        static constexpr uint32_t TARGET_PALETTE_SMOOTHING = 3;
        static constexpr uint32_t TARGET_PALETTE_ANIMATION_SPEED = 4;
        static constexpr uint32_t TARGET_PALETTE_STATIC_COLOR_COUNT = 5;
        static constexpr uint32_t TARGET_PALETTE_STATIC_COLOR_TOLERANCE = 6;
        static constexpr uint32_t TARGET_PALETTE_ANIMATION_MODE = 7;
        static constexpr uint32_t TARGET_PALETTE_ANIMATION_FLOW_AMOUNT = 8;
        static constexpr uint32_t TARGET_PALETTE_ANIMATION_FLOW_SCALE = 9;
        static constexpr uint32_t TARGET_PALETTE_ANIMATION_FLOW_SPEED = 10;
        static constexpr uint32_t TARGET_PALETTE_ANIMATION_FLOW_SWIRL = 11;
        static constexpr uint32_t TARGET_PALETTE_MANDELBROT_COLOR = 12;
        static constexpr uint32_t TARGET_PALETTE_STATIC_COLOR_ITERATIONS = 13;
        // Appended ahead of the color array, whose target follows it now. Targets number the
        // reserve order, and the reserve carries the pad that drops the array onto the 16-byte
        // boundary the shader's own layout gives it.
        static constexpr uint32_t TARGET_PALETTE_CYCLE_BIAS = 14;
        static constexpr uint32_t TARGET_PALETTE_COLORS = 15;

        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();

            bufferManager->reserve<glm::vec4>(TARGET_PALETTE_INTERVAL);
            bufferManager->reserve<double>(TARGET_PALETTE_OFFSET);
            bufferManager->reserve<uint32_t>(TARGET_PALETTE_SIZE);
            bufferManager->reserve<uint32_t>(TARGET_PALETTE_SMOOTHING);
            bufferManager->reserve<float>(TARGET_PALETTE_ANIMATION_SPEED);
            bufferManager->reserve<uint32_t>(TARGET_PALETTE_STATIC_COLOR_COUNT);
            bufferManager->reserve<float>(TARGET_PALETTE_STATIC_COLOR_TOLERANCE);
            bufferManager->reserve<uint32_t>(TARGET_PALETTE_ANIMATION_MODE);
            bufferManager->reserve<float>(TARGET_PALETTE_ANIMATION_FLOW_AMOUNT);
            bufferManager->reserve<float>(TARGET_PALETTE_ANIMATION_FLOW_SCALE);
            bufferManager->reserve<float>(TARGET_PALETTE_ANIMATION_FLOW_SPEED);
            bufferManager->reserve<float>(TARGET_PALETTE_ANIMATION_FLOW_SWIRL);

            bufferManager->reserve<glm::vec4>(TARGET_PALETTE_MANDELBROT_COLOR);

            // Fixed-size double array sits before the runtime colors array (which must stay last).
            bufferManager->reserveArray<double>(TARGET_PALETTE_STATIC_COLOR_ITERATIONS, MAX_STATIC_COLORS);

            bufferManager->reserve<float>(TARGET_PALETTE_CYCLE_BIAS, 12);
            bufferManager->reserveArray<glm::vec4>(TARGET_PALETTE_COLORS, 0);

            auto ssbo = vkh::factory::create<vkh::ShaderStorage>(core, std::move(bufferManager),
                                                                 vkh::BufferLock::LOCK_UNLOCK, false);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();

            descManager->appendSSBO(BINDING_SSBO_PALETTE, STAGE, std::move(ssbo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescStripe final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 4;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        static constexpr uint32_t BINDING_UBO_STRIPE = 0;

        static constexpr uint32_t TARGET_STRIPE_TYPE = 0;
        static constexpr uint32_t TARGET_STRIPE_FIRST_INTERVAL = 1;
        static constexpr uint32_t TARGET_STRIPE_SECOND_INTERVAL = 2;
        static constexpr uint32_t TARGET_STRIPE_OPACITY = 3;
        static constexpr uint32_t TARGET_STRIPE_OFFSET = 4;
        static constexpr uint32_t TARGET_STRIPE_ANIMATION_SPEED = 5;


        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<uint32_t>(TARGET_STRIPE_TYPE);
            bufferManager->reserve<float>(TARGET_STRIPE_FIRST_INTERVAL);
            bufferManager->reserve<float>(TARGET_STRIPE_SECOND_INTERVAL);
            bufferManager->reserve<float>(TARGET_STRIPE_OPACITY);
            bufferManager->reserve<float>(TARGET_STRIPE_OFFSET);
            bufferManager->reserve<float>(TARGET_STRIPE_ANIMATION_SPEED);
            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK, false);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendUBO(BINDING_UBO_STRIPE, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescSlope final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 5;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT;

        static constexpr uint32_t BINDING_UBO_SLOPE = 0;

        static constexpr uint32_t TARGET_SLOPE_DEPTH = 0;
        static constexpr uint32_t TARGET_SLOPE_REFLECTION_RATIO = 1;
        static constexpr uint32_t TARGET_SLOPE_OPACITY = 2;
        static constexpr uint32_t TARGET_SLOPE_ZENITH = 3;
        static constexpr uint32_t TARGET_SLOPE_AZIMUTH = 4;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_INTENSITY = 5;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_POWER = 6;
        static constexpr uint32_t TARGET_SLOPE_RIM_INTENSITY = 7;
        static constexpr uint32_t TARGET_SLOPE_RIM_POWER = 8;

        static constexpr uint32_t TARGET_SLOPE_BRIGHTNESS = 9;
        static constexpr uint32_t TARGET_SLOPE_GAMMA = 10;
        static constexpr uint32_t TARGET_SLOPE_RIM_COLOR_R = 11;
        static constexpr uint32_t TARGET_SLOPE_RIM_COLOR_G = 12;
        static constexpr uint32_t TARGET_SLOPE_RIM_COLOR_B = 13;

        static constexpr uint32_t TARGET_SLOPE_SPECULAR_COLOR_R = 14;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_COLOR_G = 15;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_COLOR_B = 16;
        static constexpr uint32_t TARGET_SLOPE_AO_INTENSITY = 17;

        static constexpr uint32_t TARGET_SLOPE_AMBIENT_INTENSITY = 18;
        static constexpr uint32_t TARGET_SLOPE_SKY_COLOR_R = 19;
        static constexpr uint32_t TARGET_SLOPE_SKY_COLOR_G = 20;
        static constexpr uint32_t TARGET_SLOPE_SKY_COLOR_B = 21;
        static constexpr uint32_t TARGET_SLOPE_GROUND_COLOR_R = 22;
        static constexpr uint32_t TARGET_SLOPE_GROUND_COLOR_G = 23;
        static constexpr uint32_t TARGET_SLOPE_GROUND_COLOR_B = 24;

        static constexpr uint32_t TARGET_SLOPE_SPECULAR_LINK = 25;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_ZENITH = 26;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_AZIMUTH = 27;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_ANISOTROPY = 28;
        static constexpr uint32_t TARGET_SLOPE_SPECULAR_ANISOTROPY_ANGLE = 29;

        static constexpr uint32_t TARGET_SLOPE_MACRO_RELIEF = 30;
        static constexpr uint32_t TARGET_SLOPE_MACRO_RADIUS = 31;

        static constexpr uint32_t TARGET_SLOPE_SHADING_BLEND = 32;

        static constexpr uint32_t TARGET_SLOPE_RELIEF_RESPONSE = 33;
        static constexpr uint32_t TARGET_SLOPE_TERMINATOR_SOFTNESS = 34;
        static constexpr uint32_t TARGET_SLOPE_HIGHLIGHT_KNEE = 35;
        static constexpr uint32_t TARGET_SLOPE_LIGHT_BLEND = 36;

        static constexpr uint32_t TARGET_SLOPE_LUMA_AMOUNT = 37;
        static constexpr uint32_t TARGET_SLOPE_TINT_RESPONSE = 38;
        static constexpr uint32_t TARGET_SLOPE_SHADOW_CHROMA = 39;
        static constexpr uint32_t TARGET_SLOPE_TINT_BLEND = 40;

        // Appended at the tail to keep existing offsets fixed (must match the vk_slope.frag block).
        static constexpr uint32_t TARGET_SLOPE_FILL_INTENSITY = 41;
        static constexpr uint32_t TARGET_SLOPE_FILL_ZENITH = 42;
        static constexpr uint32_t TARGET_SLOPE_FILL_AZIMUTH = 43;

        // The gloss, appended at the tail for the same reason the block above it was.
        static constexpr uint32_t TARGET_SLOPE_GLOSS_INTENSITY = 44;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_SOURCE = 45;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_BANDS = 46;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_SHARPNESS = 47;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_PHASE = 48;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_COLOR_R = 49;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_COLOR_G = 50;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_COLOR_B = 51;
        static constexpr uint32_t TARGET_SLOPE_GLOSS_RELIEF = 52;
        static constexpr uint32_t TARGET_STUDIO_USE = 53;
        static constexpr uint32_t TARGET_STUDIO_ROUGHNESS = 54;
        static constexpr uint32_t TARGET_STUDIO_METALNESS = 55;
        static constexpr uint32_t TARGET_STUDIO_IOR = 56;
        static constexpr uint32_t TARGET_STUDIO_DIRECT_INTENSITY = 57;
        static constexpr uint32_t TARGET_STUDIO_ENVIRONMENT_INTENSITY = 58;
        static constexpr uint32_t TARGET_STUDIO_CLEARCOAT = 59;
        static constexpr uint32_t TARGET_STUDIO_CLEARCOAT_ROUGHNESS = 60;
        static constexpr uint32_t TARGET_SURFACE_IRIDESCENCE = 61;
        static constexpr uint32_t TARGET_SURFACE_FILM_THICKNESS = 62;
        static constexpr uint32_t TARGET_SURFACE_SPECULAR_AA = 63;
        static constexpr uint32_t TARGET_SURFACE_LUSTRE_RELIEF = 64;
        static constexpr uint32_t TARGET_SURFACE_RELIEF_DEPTH = 65;
        static constexpr uint32_t TARGET_SURFACE_NORMAL_SMOOTH = 66;
        static constexpr uint32_t TARGET_SURFACE_AO_RADIUS = 67;
        static constexpr uint32_t TARGET_SURFACE_RELIEF_WAVES = 68;
        static constexpr uint32_t TARGET_SURFACE_WAVE_FREQUENCY = 69;
        static constexpr uint32_t TARGET_SURFACE_INVERT_RELIEF = 70;
        static constexpr uint32_t TARGET_SURFACE_BOUNDARY_GUARD = 71;
        static constexpr uint32_t TARGET_SURFACE_ZOOM_LOG2 = 72;
        static constexpr uint32_t TARGET_GROOVE_ENABLED = 73;
        static constexpr uint32_t TARGET_GROOVE_DEPTH = 74;
        static constexpr uint32_t TARGET_GROOVE_WIDTH = 75;
        static constexpr uint32_t TARGET_GROOVE_AUTO = 76;
        static constexpr uint32_t TARGET_GROOVE_COUNT = 77;
        static constexpr uint32_t TARGET_SURFACE_STYLE = 78;
        static constexpr uint32_t TARGET_CHROME_STRENGTH = 79;
        static constexpr uint32_t TARGET_FILM_STRENGTH = 80;
        static constexpr uint32_t TARGET_REFLECTION_DETAIL = 81;
        static constexpr uint32_t TARGET_REFLECTION_CONTRAST = 82;
        static constexpr uint32_t TARGET_REFLECTION_BRIGHTNESS = 83;
        static constexpr uint32_t TARGET_REFLECTION_CURVE = 84;
        static constexpr uint32_t TARGET_SURFACE_PHASE = 85;
        static constexpr uint32_t TARGET_PRISM_WIDTH = 86;
        static constexpr uint32_t TARGET_PRISM_SPREAD = 87;
        static constexpr uint32_t TARGET_FILM_HUE = 88;
        static constexpr uint32_t TARGET_SPARKLE_STRENGTH = 89;
        static constexpr uint32_t TARGET_BACKGROUND_BRIGHTNESS = 90;
        static constexpr uint32_t TARGET_SHADOW_CRUSH = 91;
        static constexpr uint32_t TARGET_FLAME_STRENGTH = 92;
        static constexpr uint32_t TARGET_PHONK_RED = 93;
        static constexpr uint32_t TARGET_FROST_STRENGTH = 94;
        static constexpr uint32_t TARGET_FROST_THRESHOLD = 95;
        static constexpr uint32_t TARGET_GRUNGE_SCALE = 96;
        static constexpr uint32_t TARGET_PALETTE_COLOR_MIX = 97;
        static constexpr uint32_t TARGET_STYLE_COLOR_MIX = 98;
        static constexpr uint32_t TARGET_STYLE_HIGHLIGHT_MIX = 99;
        static constexpr uint32_t TARGET_STYLE_BACKGROUND_MIX = 100;
        static constexpr uint32_t TARGET_STYLE_INK_PRESERVE = 101;
        static constexpr uint32_t TARGET_STYLE_GLINT_SIZE = 102;
        static constexpr uint32_t TARGET_STYLE_DETAIL_LIGHT = 103;
        static constexpr uint32_t TARGET_STYLE_DETAIL_SUPPRESS = 104;
        static constexpr uint32_t TARGET_STYLE_DETAIL_THRESHOLD = 105;
        static constexpr uint32_t TARGET_STYLE_DAMAGE = 106;
        static constexpr uint32_t TARGET_SURFACE_BLEND = 107;
        static constexpr uint32_t TARGET_STYLE_COLOR_R = 108;
        static constexpr uint32_t TARGET_STYLE_COLOR_G = 109;
        static constexpr uint32_t TARGET_STYLE_COLOR_B = 110;
        static constexpr uint32_t TARGET_STYLE_HIGHLIGHT_COLOR_R = 111;
        static constexpr uint32_t TARGET_STYLE_HIGHLIGHT_COLOR_G = 112;
        static constexpr uint32_t TARGET_STYLE_HIGHLIGHT_COLOR_B = 113;
        static constexpr uint32_t TARGET_STYLE_BACKGROUND_COLOR_R = 114;
        static constexpr uint32_t TARGET_STYLE_BACKGROUND_COLOR_G = 115;
        static constexpr uint32_t TARGET_STYLE_BACKGROUND_COLOR_B = 116;
        static constexpr uint32_t TARGET_SEA_GLOW = 117;
        static constexpr uint32_t TARGET_SEA_THRESHOLD = 118;
        static constexpr uint32_t TARGET_SEA_BODY = 119;
        static constexpr uint32_t TARGET_SEA_PARTICLES = 120;
        static constexpr uint32_t TARGET_SEA_PARTICLE_SIZE = 121;
        static constexpr uint32_t TARGET_SEA_BALANCE = 122;
        static constexpr uint32_t TARGET_UKIYO_COLORS = 123;
        static constexpr uint32_t TARGET_UKIYO_FOAM = 124;
        static constexpr uint32_t TARGET_UKIYO_GRAIN = 125;
        static constexpr uint32_t TARGET_UKIYO_FLATNESS = 126;
        static constexpr uint32_t TARGET_UKIYO_BALANCE = 127;
        static constexpr uint32_t TARGET_SEA_BODY_COLOR_R = 128;
        static constexpr uint32_t TARGET_SEA_BODY_COLOR_G = 129;
        static constexpr uint32_t TARGET_SEA_BODY_COLOR_B = 130;
        static constexpr uint32_t TARGET_SEA_GLOW_COLOR_R = 131;
        static constexpr uint32_t TARGET_SEA_GLOW_COLOR_G = 132;
        static constexpr uint32_t TARGET_SEA_GLOW_COLOR_B = 133;
        static constexpr uint32_t TARGET_SEA_ACCENT_COLOR_R = 134;
        static constexpr uint32_t TARGET_SEA_ACCENT_COLOR_G = 135;
        static constexpr uint32_t TARGET_SEA_ACCENT_COLOR_B = 136;
        static constexpr uint32_t TARGET_PRINT_INK_R = 137;
        static constexpr uint32_t TARGET_PRINT_INK_G = 138;
        static constexpr uint32_t TARGET_PRINT_INK_B = 139;
        static constexpr uint32_t TARGET_PRINT_INDIGO_R = 140;
        static constexpr uint32_t TARGET_PRINT_INDIGO_G = 141;
        static constexpr uint32_t TARGET_PRINT_INDIGO_B = 142;
        static constexpr uint32_t TARGET_PRINT_ASAGI_R = 143;
        static constexpr uint32_t TARGET_PRINT_ASAGI_G = 144;
        static constexpr uint32_t TARGET_PRINT_ASAGI_B = 145;
        static constexpr uint32_t TARGET_PRINT_BLUE_R = 146;
        static constexpr uint32_t TARGET_PRINT_BLUE_G = 147;
        static constexpr uint32_t TARGET_PRINT_BLUE_B = 148;
        static constexpr uint32_t TARGET_PRINT_FOAM_R = 149;
        static constexpr uint32_t TARGET_PRINT_FOAM_G = 150;
        static constexpr uint32_t TARGET_PRINT_FOAM_B = 151;
        static constexpr uint32_t TARGET_PRINT_PAPER_R = 152;
        static constexpr uint32_t TARGET_PRINT_PAPER_G = 153;
        static constexpr uint32_t TARGET_PRINT_PAPER_B = 154;
        static constexpr uint32_t TARGET_LAYER_METAL = 155;
        static constexpr uint32_t TARGET_LAYER_SIGIL = 156;
        static constexpr uint32_t TARGET_LAYER_PHONK = 157;
        static constexpr uint32_t TARGET_LAYER_FROST = 158;
        static constexpr uint32_t TARGET_LAYER_SEA = 159;
        static constexpr uint32_t TARGET_LAYER_PRINT = 160;
        static constexpr uint32_t TARGET_LAYER_COMMON = 161;
        static constexpr uint32_t TARGET_STUDIO_ENVIRONMENT_ROTATION = 162;
        static constexpr uint32_t TARGET_STUDIO_ENVIRONMENT_FOLLOW = 163;
        static constexpr uint32_t TARGET_REPLACE_SURFACE_STYLE = 164;



        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<float>(TARGET_SLOPE_DEPTH);
            bufferManager->reserve<float>(TARGET_SLOPE_REFLECTION_RATIO);
            bufferManager->reserve<float>(TARGET_SLOPE_OPACITY);
            bufferManager->reserve<float>(TARGET_SLOPE_ZENITH);
            bufferManager->reserve<float>(TARGET_SLOPE_AZIMUTH);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_INTENSITY);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_POWER);
            bufferManager->reserve<float>(TARGET_SLOPE_RIM_INTENSITY);
            bufferManager->reserve<float>(TARGET_SLOPE_RIM_POWER);
            bufferManager->reserve<float>(TARGET_SLOPE_BRIGHTNESS);
            bufferManager->reserve<float>(TARGET_SLOPE_GAMMA);
            bufferManager->reserve<float>(TARGET_SLOPE_RIM_COLOR_R);
            bufferManager->reserve<float>(TARGET_SLOPE_RIM_COLOR_G);
            bufferManager->reserve<float>(TARGET_SLOPE_RIM_COLOR_B);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_COLOR_R);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_COLOR_G);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_COLOR_B);
            bufferManager->reserve<float>(TARGET_SLOPE_AO_INTENSITY);
            bufferManager->reserve<float>(TARGET_SLOPE_AMBIENT_INTENSITY);
            bufferManager->reserve<float>(TARGET_SLOPE_SKY_COLOR_R);
            bufferManager->reserve<float>(TARGET_SLOPE_SKY_COLOR_G);
            bufferManager->reserve<float>(TARGET_SLOPE_SKY_COLOR_B);
            bufferManager->reserve<float>(TARGET_SLOPE_GROUND_COLOR_R);
            bufferManager->reserve<float>(TARGET_SLOPE_GROUND_COLOR_G);
            bufferManager->reserve<float>(TARGET_SLOPE_GROUND_COLOR_B);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_LINK);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_ZENITH);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_AZIMUTH);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_ANISOTROPY);
            bufferManager->reserve<float>(TARGET_SLOPE_SPECULAR_ANISOTROPY_ANGLE);
            bufferManager->reserve<float>(TARGET_SLOPE_MACRO_RELIEF);
            bufferManager->reserve<float>(TARGET_SLOPE_MACRO_RADIUS);
            bufferManager->reserve<float>(TARGET_SLOPE_SHADING_BLEND);
            bufferManager->reserve<float>(TARGET_SLOPE_RELIEF_RESPONSE);
            bufferManager->reserve<float>(TARGET_SLOPE_TERMINATOR_SOFTNESS);
            bufferManager->reserve<float>(TARGET_SLOPE_HIGHLIGHT_KNEE);
            bufferManager->reserve<float>(TARGET_SLOPE_LIGHT_BLEND);
            bufferManager->reserve<float>(TARGET_SLOPE_LUMA_AMOUNT);
            bufferManager->reserve<float>(TARGET_SLOPE_TINT_RESPONSE);
            bufferManager->reserve<float>(TARGET_SLOPE_SHADOW_CHROMA);
            bufferManager->reserve<float>(TARGET_SLOPE_TINT_BLEND);
            bufferManager->reserve<float>(TARGET_SLOPE_FILL_INTENSITY);
            bufferManager->reserve<float>(TARGET_SLOPE_FILL_ZENITH);
            bufferManager->reserve<float>(TARGET_SLOPE_FILL_AZIMUTH);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_INTENSITY);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_SOURCE);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_BANDS);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_SHARPNESS);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_PHASE);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_COLOR_R);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_COLOR_G);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_COLOR_B);
            bufferManager->reserve<float>(TARGET_SLOPE_GLOSS_RELIEF);
            bufferManager->reserve<float>(TARGET_STUDIO_USE);
            bufferManager->reserve<float>(TARGET_STUDIO_ROUGHNESS);
            bufferManager->reserve<float>(TARGET_STUDIO_METALNESS);
            bufferManager->reserve<float>(TARGET_STUDIO_IOR);
            bufferManager->reserve<float>(TARGET_STUDIO_DIRECT_INTENSITY);
            bufferManager->reserve<float>(TARGET_STUDIO_ENVIRONMENT_INTENSITY);
            bufferManager->reserve<float>(TARGET_STUDIO_CLEARCOAT);
            bufferManager->reserve<float>(TARGET_STUDIO_CLEARCOAT_ROUGHNESS);
            bufferManager->reserve<float>(TARGET_SURFACE_IRIDESCENCE);
            bufferManager->reserve<float>(TARGET_SURFACE_FILM_THICKNESS);
            bufferManager->reserve<float>(TARGET_SURFACE_SPECULAR_AA);
            bufferManager->reserve<float>(TARGET_SURFACE_LUSTRE_RELIEF);
            bufferManager->reserve<float>(TARGET_SURFACE_RELIEF_DEPTH);
            bufferManager->reserve<float>(TARGET_SURFACE_NORMAL_SMOOTH);
            bufferManager->reserve<float>(TARGET_SURFACE_AO_RADIUS);
            bufferManager->reserve<float>(TARGET_SURFACE_RELIEF_WAVES);
            bufferManager->reserve<float>(TARGET_SURFACE_WAVE_FREQUENCY);
            bufferManager->reserve<float>(TARGET_SURFACE_INVERT_RELIEF);
            bufferManager->reserve<float>(TARGET_SURFACE_BOUNDARY_GUARD);
            bufferManager->reserve<float>(TARGET_SURFACE_ZOOM_LOG2);
            bufferManager->reserve<float>(TARGET_GROOVE_ENABLED);
            bufferManager->reserve<float>(TARGET_GROOVE_DEPTH);
            bufferManager->reserve<float>(TARGET_GROOVE_WIDTH);
            bufferManager->reserve<float>(TARGET_GROOVE_AUTO);
            bufferManager->reserve<float>(TARGET_GROOVE_COUNT);
            bufferManager->reserve<float>(TARGET_SURFACE_STYLE);
            bufferManager->reserve<float>(TARGET_CHROME_STRENGTH);
            bufferManager->reserve<float>(TARGET_FILM_STRENGTH);
            bufferManager->reserve<float>(TARGET_REFLECTION_DETAIL);
            bufferManager->reserve<float>(TARGET_REFLECTION_CONTRAST);
            bufferManager->reserve<float>(TARGET_REFLECTION_BRIGHTNESS);
            bufferManager->reserve<float>(TARGET_REFLECTION_CURVE);
            bufferManager->reserve<float>(TARGET_SURFACE_PHASE);
            bufferManager->reserve<float>(TARGET_PRISM_WIDTH);
            bufferManager->reserve<float>(TARGET_PRISM_SPREAD);
            bufferManager->reserve<float>(TARGET_FILM_HUE);
            bufferManager->reserve<float>(TARGET_SPARKLE_STRENGTH);
            bufferManager->reserve<float>(TARGET_BACKGROUND_BRIGHTNESS);
            bufferManager->reserve<float>(TARGET_SHADOW_CRUSH);
            bufferManager->reserve<float>(TARGET_FLAME_STRENGTH);
            bufferManager->reserve<float>(TARGET_PHONK_RED);
            bufferManager->reserve<float>(TARGET_FROST_STRENGTH);
            bufferManager->reserve<float>(TARGET_FROST_THRESHOLD);
            bufferManager->reserve<float>(TARGET_GRUNGE_SCALE);
            bufferManager->reserve<float>(TARGET_PALETTE_COLOR_MIX);
            bufferManager->reserve<float>(TARGET_STYLE_COLOR_MIX);
            bufferManager->reserve<float>(TARGET_STYLE_HIGHLIGHT_MIX);
            bufferManager->reserve<float>(TARGET_STYLE_BACKGROUND_MIX);
            bufferManager->reserve<float>(TARGET_STYLE_INK_PRESERVE);
            bufferManager->reserve<float>(TARGET_STYLE_GLINT_SIZE);
            bufferManager->reserve<float>(TARGET_STYLE_DETAIL_LIGHT);
            bufferManager->reserve<float>(TARGET_STYLE_DETAIL_SUPPRESS);
            bufferManager->reserve<float>(TARGET_STYLE_DETAIL_THRESHOLD);
            bufferManager->reserve<float>(TARGET_STYLE_DAMAGE);
            bufferManager->reserve<float>(TARGET_SURFACE_BLEND);
            bufferManager->reserve<float>(TARGET_STYLE_COLOR_R);
            bufferManager->reserve<float>(TARGET_STYLE_COLOR_G);
            bufferManager->reserve<float>(TARGET_STYLE_COLOR_B);
            bufferManager->reserve<float>(TARGET_STYLE_HIGHLIGHT_COLOR_R);
            bufferManager->reserve<float>(TARGET_STYLE_HIGHLIGHT_COLOR_G);
            bufferManager->reserve<float>(TARGET_STYLE_HIGHLIGHT_COLOR_B);
            bufferManager->reserve<float>(TARGET_STYLE_BACKGROUND_COLOR_R);
            bufferManager->reserve<float>(TARGET_STYLE_BACKGROUND_COLOR_G);
            bufferManager->reserve<float>(TARGET_STYLE_BACKGROUND_COLOR_B);
            bufferManager->reserve<float>(TARGET_SEA_GLOW);
            bufferManager->reserve<float>(TARGET_SEA_THRESHOLD);
            bufferManager->reserve<float>(TARGET_SEA_BODY);
            bufferManager->reserve<float>(TARGET_SEA_PARTICLES);
            bufferManager->reserve<float>(TARGET_SEA_PARTICLE_SIZE);
            bufferManager->reserve<float>(TARGET_SEA_BALANCE);
            bufferManager->reserve<float>(TARGET_UKIYO_COLORS);
            bufferManager->reserve<float>(TARGET_UKIYO_FOAM);
            bufferManager->reserve<float>(TARGET_UKIYO_GRAIN);
            bufferManager->reserve<float>(TARGET_UKIYO_FLATNESS);
            bufferManager->reserve<float>(TARGET_UKIYO_BALANCE);
            bufferManager->reserve<float>(TARGET_SEA_BODY_COLOR_R);
            bufferManager->reserve<float>(TARGET_SEA_BODY_COLOR_G);
            bufferManager->reserve<float>(TARGET_SEA_BODY_COLOR_B);
            bufferManager->reserve<float>(TARGET_SEA_GLOW_COLOR_R);
            bufferManager->reserve<float>(TARGET_SEA_GLOW_COLOR_G);
            bufferManager->reserve<float>(TARGET_SEA_GLOW_COLOR_B);
            bufferManager->reserve<float>(TARGET_SEA_ACCENT_COLOR_R);
            bufferManager->reserve<float>(TARGET_SEA_ACCENT_COLOR_G);
            bufferManager->reserve<float>(TARGET_SEA_ACCENT_COLOR_B);
            bufferManager->reserve<float>(TARGET_PRINT_INK_R);
            bufferManager->reserve<float>(TARGET_PRINT_INK_G);
            bufferManager->reserve<float>(TARGET_PRINT_INK_B);
            bufferManager->reserve<float>(TARGET_PRINT_INDIGO_R);
            bufferManager->reserve<float>(TARGET_PRINT_INDIGO_G);
            bufferManager->reserve<float>(TARGET_PRINT_INDIGO_B);
            bufferManager->reserve<float>(TARGET_PRINT_ASAGI_R);
            bufferManager->reserve<float>(TARGET_PRINT_ASAGI_G);
            bufferManager->reserve<float>(TARGET_PRINT_ASAGI_B);
            bufferManager->reserve<float>(TARGET_PRINT_BLUE_R);
            bufferManager->reserve<float>(TARGET_PRINT_BLUE_G);
            bufferManager->reserve<float>(TARGET_PRINT_BLUE_B);
            bufferManager->reserve<float>(TARGET_PRINT_FOAM_R);
            bufferManager->reserve<float>(TARGET_PRINT_FOAM_G);
            bufferManager->reserve<float>(TARGET_PRINT_FOAM_B);
            bufferManager->reserve<float>(TARGET_PRINT_PAPER_R);
            bufferManager->reserve<float>(TARGET_PRINT_PAPER_G);
            bufferManager->reserve<float>(TARGET_PRINT_PAPER_B);
            bufferManager->reserve<float>(TARGET_LAYER_METAL);
            bufferManager->reserve<float>(TARGET_LAYER_SIGIL);
            bufferManager->reserve<float>(TARGET_LAYER_PHONK);
            bufferManager->reserve<float>(TARGET_LAYER_FROST);
            bufferManager->reserve<float>(TARGET_LAYER_SEA);
            bufferManager->reserve<float>(TARGET_LAYER_PRINT);
            bufferManager->reserve<float>(TARGET_LAYER_COMMON);
            bufferManager->reserve<float>(TARGET_STUDIO_ENVIRONMENT_ROTATION);
            bufferManager->reserve<float>(TARGET_STUDIO_ENVIRONMENT_FOLLOW);
            bufferManager->reserve<float>(TARGET_REPLACE_SURFACE_STYLE);


            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK, false);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendUBO(BINDING_UBO_SLOPE, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescColor final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 6;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT;

        static constexpr uint32_t BINDING_UBO_COLOR = 0;

        static constexpr uint32_t TARGET_COLOR_GAMMA = 0;
        static constexpr uint32_t TARGET_COLOR_EXPOSURE = 1;
        static constexpr uint32_t TARGET_COLOR_HUE = 2;
        static constexpr uint32_t TARGET_COLOR_SATURATION = 3;
        static constexpr uint32_t TARGET_COLOR_BRIGHTNESS = 4;
        static constexpr uint32_t TARGET_COLOR_CONTRAST = 5;
        static constexpr uint32_t TARGET_COLOR_SCENE_LINEAR = 6;


        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<float>(TARGET_COLOR_GAMMA);
            bufferManager->reserve<float>(TARGET_COLOR_EXPOSURE);
            bufferManager->reserve<float>(TARGET_COLOR_HUE);
            bufferManager->reserve<float>(TARGET_COLOR_SATURATION);
            bufferManager->reserve<float>(TARGET_COLOR_BRIGHTNESS);
            bufferManager->reserve<float>(TARGET_COLOR_CONTRAST);
            bufferManager->reserve<float>(TARGET_COLOR_SCENE_LINEAR);
            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK, false);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendUBO(BINDING_UBO_COLOR, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescFog final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 7;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT;

        static constexpr uint32_t BINDING_UBO_FOG = 0;

        static constexpr uint32_t TARGET_FOG_RADIUS = 0;
        static constexpr uint32_t TARGET_FOG_OPACITY = 1;
        static constexpr uint32_t TARGET_FOG_RIM_MASK = 2;
        static constexpr uint32_t TARGET_FOG_RIM_MASK_BOOST = 3;
        static constexpr uint32_t TARGET_FOG_RIM_BLUR = 4;
        // Appended at the tail to keep existing offsets fixed (must match the vk_fog.frag block).
        static constexpr uint32_t TARGET_FOG_CENTER_START = 5;
        static constexpr uint32_t TARGET_FOG_CENTER_INVERT = 6;
        static constexpr uint32_t TARGET_FOG_FOCUS_AMOUNT = 7;
        static constexpr uint32_t TARGET_FOG_FOCUS_RATIO = 8;
        static constexpr uint32_t TARGET_FOG_FOCUS_RANGE = 9;
        static constexpr uint32_t TARGET_FOG_FOCUS_FALLOFF = 10;
        static constexpr uint32_t TARGET_FOG_FOCUS_BLUR = 11;
        static constexpr uint32_t TARGET_FOG_BLUR_QUALITY = 12;
        static constexpr uint32_t TARGET_FOG_CHAOS_AMOUNT = 13;
        static constexpr uint32_t TARGET_FOG_CHAOS_SCALE = 14;
        static constexpr uint32_t TARGET_FOG_CHAOS_THRESHOLD = 15;
        static constexpr uint32_t TARGET_FOG_CHAOS_TRANSITION = 16;
        static constexpr uint32_t TARGET_FOG_CHAOS_FEATHER = 17;
        static constexpr uint32_t TARGET_FOG_CHAOS_BLUR = 18;
        static constexpr uint32_t TARGET_FOG_CHAOS_HIGHLIGHTS = 19;
        static constexpr uint32_t TARGET_FOG_CHAOS_SHADE = 20;


        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();

            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<float>(TARGET_FOG_RADIUS);
            bufferManager->reserve<float>(TARGET_FOG_OPACITY);
            bufferManager->reserve<float>(TARGET_FOG_RIM_MASK);
            bufferManager->reserve<float>(TARGET_FOG_RIM_MASK_BOOST);
            bufferManager->reserve<float>(TARGET_FOG_RIM_BLUR);
            bufferManager->reserve<float>(TARGET_FOG_CENTER_START);
            bufferManager->reserve<float>(TARGET_FOG_CENTER_INVERT);
            bufferManager->reserve<float>(TARGET_FOG_FOCUS_AMOUNT);
            bufferManager->reserve<float>(TARGET_FOG_FOCUS_RATIO);
            bufferManager->reserve<float>(TARGET_FOG_FOCUS_RANGE);
            bufferManager->reserve<float>(TARGET_FOG_FOCUS_FALLOFF);
            bufferManager->reserve<float>(TARGET_FOG_FOCUS_BLUR);
            bufferManager->reserve<float>(TARGET_FOG_BLUR_QUALITY);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_AMOUNT);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_SCALE);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_THRESHOLD);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_TRANSITION);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_FEATHER);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_BLUR);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_HIGHLIGHTS);
            bufferManager->reserve<float>(TARGET_FOG_CHAOS_SHADE);

            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK, false);
            descManager->appendUBO(BINDING_UBO_FOG, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescBloom final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 8;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT;

        static constexpr uint32_t BINDING_UBO_BLOOM = 0;

        static constexpr uint32_t TARGET_BLOOM_THRESHOLD = 0;
        static constexpr uint32_t TARGET_BLOOM_RADIUS = 1;
        static constexpr uint32_t TARGET_BLOOM_SOFTNESS = 2;
        static constexpr uint32_t TARGET_BLOOM_INTENSITY = 3;
        static constexpr uint32_t TARGET_BLOOM_HDR = 4;
        static constexpr uint32_t TARGET_BLOOM_HEADROOM = 5;
        static constexpr uint32_t TARGET_BLOOM_LINEAR_ADD = 6;
        static constexpr uint32_t TARGET_BLOOM_SCENE_LINEAR = 7;


        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<float>(TARGET_BLOOM_THRESHOLD);
            bufferManager->reserve<float>(TARGET_BLOOM_RADIUS);
            bufferManager->reserve<float>(TARGET_BLOOM_SOFTNESS);
            bufferManager->reserve<float>(TARGET_BLOOM_INTENSITY);
            bufferManager->reserve<float>(TARGET_BLOOM_HDR);
            bufferManager->reserve<float>(TARGET_BLOOM_HEADROOM);
            bufferManager->reserve<float>(TARGET_BLOOM_LINEAR_ADD);
            bufferManager->reserve<float>(TARGET_BLOOM_SCENE_LINEAR);
            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK, false);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendUBO(BINDING_UBO_BLOOM, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescLinearInterpolation final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 9;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT;

        static constexpr uint32_t BINDING_UBO_LINEAR_INTERPOLATION = 0;

        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_USE = 0;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_DITHER = 1;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_HDR = 2;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_EXPOSURE = 3;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_HEADROOM = 4;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_TONE_MAP = 5;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_TRANSFER = 6;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_PEAK_NITS = 7;
        static constexpr uint32_t TARGET_LINEAR_INTERPOLATION_SCENE_LINEAR = 8;
        static constexpr uint32_t TARGET_DARK_STYLE = 9;
        static constexpr uint32_t TARGET_DARK_STRENGTH = 10;
        static constexpr uint32_t TARGET_DARK_VHS = 11;
        static constexpr uint32_t TARGET_DARK_CHROMA = 12;
        static constexpr uint32_t TARGET_DARK_PIXEL_MIX = 13;
        static constexpr uint32_t TARGET_DARK_PIXEL_SIZE = 14;
        static constexpr uint32_t TARGET_DARK_GRAIN = 15;
        static constexpr uint32_t TARGET_DARK_SCALE = 16;
        static constexpr uint32_t TARGET_DARK_MONOCHROME = 17;
        static constexpr uint32_t TARGET_DARK_DAMAGE = 18;
        static constexpr uint32_t TARGET_UKIYO_COLORS = 19;
        static constexpr uint32_t TARGET_UKIYO_FLATNESS = 20;
        static constexpr uint32_t TARGET_PRINT_INK_R = 21;
        static constexpr uint32_t TARGET_PRINT_INK_G = 22;
        static constexpr uint32_t TARGET_PRINT_INK_B = 23;
        static constexpr uint32_t TARGET_PRINT_INDIGO_R = 24;
        static constexpr uint32_t TARGET_PRINT_INDIGO_G = 25;
        static constexpr uint32_t TARGET_PRINT_INDIGO_B = 26;
        static constexpr uint32_t TARGET_PRINT_ASAGI_R = 27;
        static constexpr uint32_t TARGET_PRINT_ASAGI_G = 28;
        static constexpr uint32_t TARGET_PRINT_ASAGI_B = 29;
        static constexpr uint32_t TARGET_PRINT_BLUE_R = 30;
        static constexpr uint32_t TARGET_PRINT_BLUE_G = 31;
        static constexpr uint32_t TARGET_PRINT_BLUE_B = 32;
        static constexpr uint32_t TARGET_PRINT_FOAM_R = 33;
        static constexpr uint32_t TARGET_PRINT_FOAM_G = 34;
        static constexpr uint32_t TARGET_PRINT_FOAM_B = 35;
        static constexpr uint32_t TARGET_PRINT_PAPER_R = 36;
        static constexpr uint32_t TARGET_PRINT_PAPER_G = 37;
        static constexpr uint32_t TARGET_PRINT_PAPER_B = 38;
        static constexpr uint32_t TARGET_LAYER_QUANTIZE = 39;
        static constexpr uint32_t TARGET_LAYER_VHS = 40;
        static constexpr uint32_t TARGET_LAYER_MONO = 41;


        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<bool>(TARGET_LINEAR_INTERPOLATION_USE, 3);
            bufferManager->reserve<bool>(TARGET_LINEAR_INTERPOLATION_DITHER, 3);
            bufferManager->reserve<bool>(TARGET_LINEAR_INTERPOLATION_HDR, 3);
            bufferManager->reserve<float>(TARGET_LINEAR_INTERPOLATION_EXPOSURE);
            bufferManager->reserve<float>(TARGET_LINEAR_INTERPOLATION_HEADROOM);
            bufferManager->reserve<uint32_t>(TARGET_LINEAR_INTERPOLATION_TONE_MAP);
            bufferManager->reserve<uint32_t>(TARGET_LINEAR_INTERPOLATION_TRANSFER);
            bufferManager->reserve<float>(TARGET_LINEAR_INTERPOLATION_PEAK_NITS);
            bufferManager->reserve<float>(TARGET_LINEAR_INTERPOLATION_SCENE_LINEAR);
            bufferManager->reserve<float>(TARGET_DARK_STYLE);
            bufferManager->reserve<float>(TARGET_DARK_STRENGTH);
            bufferManager->reserve<float>(TARGET_DARK_VHS);
            bufferManager->reserve<float>(TARGET_DARK_CHROMA);
            bufferManager->reserve<float>(TARGET_DARK_PIXEL_MIX);
            bufferManager->reserve<float>(TARGET_DARK_PIXEL_SIZE);
            bufferManager->reserve<float>(TARGET_DARK_GRAIN);
            bufferManager->reserve<float>(TARGET_DARK_SCALE);
            bufferManager->reserve<float>(TARGET_DARK_MONOCHROME);
            bufferManager->reserve<float>(TARGET_DARK_DAMAGE);
            bufferManager->reserve<float>(TARGET_UKIYO_COLORS);
            bufferManager->reserve<float>(TARGET_UKIYO_FLATNESS);
            bufferManager->reserve<float>(TARGET_PRINT_INK_R);
            bufferManager->reserve<float>(TARGET_PRINT_INK_G);
            bufferManager->reserve<float>(TARGET_PRINT_INK_B);
            bufferManager->reserve<float>(TARGET_PRINT_INDIGO_R);
            bufferManager->reserve<float>(TARGET_PRINT_INDIGO_G);
            bufferManager->reserve<float>(TARGET_PRINT_INDIGO_B);
            bufferManager->reserve<float>(TARGET_PRINT_ASAGI_R);
            bufferManager->reserve<float>(TARGET_PRINT_ASAGI_G);
            bufferManager->reserve<float>(TARGET_PRINT_ASAGI_B);
            bufferManager->reserve<float>(TARGET_PRINT_BLUE_R);
            bufferManager->reserve<float>(TARGET_PRINT_BLUE_G);
            bufferManager->reserve<float>(TARGET_PRINT_BLUE_B);
            bufferManager->reserve<float>(TARGET_PRINT_FOAM_R);
            bufferManager->reserve<float>(TARGET_PRINT_FOAM_G);
            bufferManager->reserve<float>(TARGET_PRINT_FOAM_B);
            bufferManager->reserve<float>(TARGET_PRINT_PAPER_R);
            bufferManager->reserve<float>(TARGET_PRINT_PAPER_G);
            bufferManager->reserve<float>(TARGET_PRINT_PAPER_B);
            bufferManager->reserve<float>(TARGET_LAYER_QUANTIZE);
            bufferManager->reserve<float>(TARGET_LAYER_VHS);
            bufferManager->reserve<float>(TARGET_LAYER_MONO);
            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK, false);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendUBO(BINDING_UBO_LINEAR_INTERPOLATION, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };

    struct DescVideo final : public vkh::DescriptorTemplate {
        static constexpr uint32_t ID = 10;
        static constexpr VkShaderStageFlags STAGE = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        static constexpr uint32_t BINDING_UBO_VIDEO = 0;

        static constexpr uint32_t TARGET_VIDEO_DEFAULT_ZOOM_INCREMENT = 0;
        static constexpr uint32_t TARGET_VIDEO_CURRENT_FRAME = 1;
        static constexpr uint32_t TARGET_VIDEO_SAMPLE_JITTER = 2;
        static constexpr uint32_t TARGET_VIDEO_DITHER = 3;
        static constexpr uint32_t TARGET_VIDEO_CAMERA = 4;
        static constexpr uint32_t TARGET_VIDEO_COVERAGE = 5;


        void configure(const vkh::CoreRef core,
                                      std::vector<vkh::DescriptorManager> &managers) override {
            auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
            bufferManager->reserve<float>(TARGET_VIDEO_DEFAULT_ZOOM_INCREMENT);
            bufferManager->reserve<float>(TARGET_VIDEO_CURRENT_FRAME);
            bufferManager->reserve<glm::vec2>(TARGET_VIDEO_SAMPLE_JITTER);
            // The host bool uses one byte; 15 padding bytes align the following vec4 with std140.
            bufferManager->reserve<bool>(TARGET_VIDEO_DITHER, 15);
            bufferManager->reserve<glm::vec4>(TARGET_VIDEO_CAMERA);
            bufferManager->reserve<glm::vec4>(TARGET_VIDEO_COVERAGE);
            auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK, true);
            auto descManager = vkh::factory::create<vkh::DescriptorManager>();
            descManager->appendUBO(BINDING_UBO_VIDEO, STAGE, std::move(ubo));
            managers.emplace_back(std::move(descManager));
        }
    };


}
